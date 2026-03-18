#!/usr/bin/env python3
import os
import math
import json
import numpy as np

import cereal.messaging as messaging
from cereal import car, log
from openpilot.common.params import Params
from openpilot.common.realtime import config_realtime_process, DT_MDL
from openpilot.selfdrive.locationd.models.car_kf import CarKalman, ObservationKind, States
from openpilot.selfdrive.locationd.models.constants import GENERATED_DIR
from openpilot.common.swaglog import cloudlog


MAX_ANGLE_OFFSET_DELTA = 20 * DT_MDL  # Max 20 deg/s
ROLL_MAX_DELTA = math.radians(20.0) * DT_MDL  # 20deg in 1 second is well within curvature limits
ROLL_MIN, ROLL_MAX = math.radians(-10), math.radians(10)
ROLL_LOWERED_MAX = math.radians(8)
ROLL_STD_MAX = math.radians(1.5)
LATERAL_ACC_SENSOR_THRESHOLD = 4.0
OFFSET_MAX = 10.0
OFFSET_LOWERED_MAX = 8.0
MIN_ACTIVE_SPEED = 1.0
LOW_ACTIVE_SPEED = 10.0
OFFSET_DIFF_HIGH = 1.0  # 偏移差值高阈值：高于此值认为已进入不稳定区，需要优先保护
OFFSET_DIFF_LOW = 0.80  # 偏移差值低阈值：低于此值才认为恢复，用于滞回防抖
OFFSET_DIFF_JUMP_HIGH = 0.12  # 单次更新突跳阈值：用于捕获“低值突然跳大”的场景
OFFSET_DIFF_JUMP_FROM_MAX = 1.2  # 突跳前值上限：仅在前一拍仍处于低值区时，突跳触发才生效
OFFSET_FALLBACK_ENTER_COUNT = 3  # 进入回退计数：连续满足异常条件 N 次才进入回退
OFFSET_FALLBACK_EXIT_COUNT = 24  # 退出回退计数：连续满足恢复条件 N 次才退出回退
YAW_RESIDUAL_HIGH = 0.10  # rad/s，yaw 残差高阈值：超过后累计坏计数，触发学习熔断
YAW_RESIDUAL_LOW = 0.06   # rad/s，yaw 残差低阈值：低于后累计好计数，作为熔断恢复条件
KF_GUARD_ENTER_COUNT = 5  # 学习熔断进入计数：连续坏计数达到该值后暂停关键观测更新
KF_GUARD_EXIT_COUNT = 20  # 学习熔断退出计数：连续好计数达到该值后恢复关键观测更新
KF_GUARD_MAX_HOLD = 180  # 学习熔断最大持续帧数：防止长时间卡住（约 10s @18Hz）
OFFSET_FALLBACK_MAX_HOLD = 180  # 偏移回退最大持续帧数：防止长时间锁在平均偏移


class ParamsLearner:
  def __init__(self, CP, steer_ratio, stiffness_factor, angle_offset, P_initial=None):
    self.kf = CarKalman(GENERATED_DIR, steer_ratio, stiffness_factor, angle_offset, P_initial)

    self.kf.filter.set_global("mass", CP.mass)
    self.kf.filter.set_global("rotational_inertia", CP.rotationalInertia)
    self.kf.filter.set_global("center_to_front", CP.centerToFront)
    self.kf.filter.set_global("center_to_rear", CP.wheelbase - CP.centerToFront)
    self.kf.filter.set_global("stiffness_front", CP.tireStiffnessFront)
    self.kf.filter.set_global("stiffness_rear", CP.tireStiffnessRear)

    self.active = False

    self.speed = 0.0
    self.yaw_rate = 0.0
    self.yaw_rate_std = 0.0
    self.roll = 0.0
    self.steering_angle = 0.0
    self.kf_guard_active = False  # 学习熔断状态：True 时暂停关键观测更新
    self.kf_guard_bad_count = 0  # 异常残差连续计数
    self.kf_guard_good_count = 0  # 恢复残差连续计数
    self.kf_guard_hold_count = 0  # 熔断持续计数：用于最大持续时间保险丝

  def handle_log(self, t, which, msg):
    if which == 'liveLocationKalman':
      self.yaw_rate = msg.angularVelocityCalibrated.value[2]
      self.yaw_rate_std = msg.angularVelocityCalibrated.std[2]

      # 这里用“模型 yaw + 测量 yaw”是因为二者符号相反（路面坐标 vs 设备坐标）。
      yaw_residual = abs(float(self.kf.x[States.YAW_RATE].item()) + self.yaw_rate)
      if yaw_residual > YAW_RESIDUAL_HIGH:
        self.kf_guard_bad_count += 1
        self.kf_guard_good_count = 0
      elif yaw_residual < YAW_RESIDUAL_LOW:
        self.kf_guard_good_count += 1
        self.kf_guard_bad_count = 0
      else:
        # 中间区自然衰减，避免计数长期冻结导致误触发或难退出
        self.kf_guard_bad_count = max(0, self.kf_guard_bad_count - 1)
        self.kf_guard_good_count = max(0, self.kf_guard_good_count - 1)

      if (not self.kf_guard_active) and (self.kf_guard_bad_count >= KF_GUARD_ENTER_COUNT):  # 连续异常达到阈值，进入学习熔断
        self.kf_guard_active = True
        self.kf_guard_bad_count = 0
        self.kf_guard_good_count = 0
        self.kf_guard_hold_count = 0
      elif self.kf_guard_active and (self.kf_guard_good_count >= KF_GUARD_EXIT_COUNT):  # 连续恢复达到阈值，退出学习熔断
        self.kf_guard_active = False
        self.kf_guard_bad_count = 0
        self.kf_guard_good_count = 0
        self.kf_guard_hold_count = 0

      if self.kf_guard_active:
        self.kf_guard_hold_count += 1
        if self.kf_guard_hold_count >= KF_GUARD_MAX_HOLD:  # 保险丝：熔断持续过久则强制退出
          self.kf_guard_active = False
          self.kf_guard_bad_count = 0
          self.kf_guard_good_count = 0
          self.kf_guard_hold_count = 0

      localizer_roll = msg.orientationNED.value[0]
      localizer_roll_std = np.radians(1) if np.isnan(msg.orientationNED.std[0]) else msg.orientationNED.std[0]
      roll_valid = (localizer_roll_std < ROLL_STD_MAX) and (ROLL_MIN < localizer_roll < ROLL_MAX) and msg.sensorsOK
      if roll_valid:
        roll = localizer_roll
        # Experimentally found multiplier of 2 to be best trade-off between stability and accuracy or similar?
        roll_std = 2 * localizer_roll_std
      else:
        # This is done to bound the road roll estimate when localizer values are invalid
        roll = 0.0
        roll_std = np.radians(10.0)
      self.roll = np.clip(roll, self.roll - ROLL_MAX_DELTA, self.roll + ROLL_MAX_DELTA)

      if self.active:
        if msg.posenetOK and not self.kf_guard_active:  # 熔断期间不接收 yaw/roll 观测，避免把坏数据喂入 KF
          self.kf.predict_and_observe(t,
                                      ObservationKind.ROAD_FRAME_YAW_RATE,
                                      np.array([[-self.yaw_rate]]),
                                      np.array([np.atleast_2d(self.yaw_rate_std**2)]))

          self.kf.predict_and_observe(t,
                                      ObservationKind.ROAD_ROLL,
                                      np.array([[self.roll]]),
                                      np.array([np.atleast_2d(roll_std**2)]))
        if not self.kf_guard_active:  # 熔断期间同时暂停快速偏移状态更新
          self.kf.predict_and_observe(t, ObservationKind.ANGLE_OFFSET_FAST, np.array([[0]]))

        # We observe the current stiffness and steer ratio (with a high observation noise) to bound
        # the respective estimate STD. Otherwise the STDs keep increasing, causing rapid changes in the
        # states in longer routes (especially straight stretches).
        stiffness = float(self.kf.x[States.STIFFNESS].item())
        steer_ratio = float(self.kf.x[States.STEER_RATIO].item())
        self.kf.predict_and_observe(t, ObservationKind.STIFFNESS, np.array([[stiffness]]))
        self.kf.predict_and_observe(t, ObservationKind.STEER_RATIO, np.array([[steer_ratio]]))

    elif which == 'carState':
      self.steering_angle = msg.steeringAngleDeg
      self.speed = msg.vEgo

      in_linear_region = abs(self.steering_angle) < 45
      self.active = self.speed > MIN_ACTIVE_SPEED and in_linear_region

      if self.active:
        self.kf.predict_and_observe(t, ObservationKind.STEER_ANGLE, np.array([[math.radians(msg.steeringAngleDeg)]]))
        self.kf.predict_and_observe(t, ObservationKind.ROAD_FRAME_X_SPEED, np.array([[self.speed]]))

    if not self.active:
      # Reset time when stopped so uncertainty doesn't grow
      self.kf.filter.set_filter_time(t)
      self.kf.filter.reset_rewind()


def check_valid_with_hysteresis(current_valid: bool, val: float, threshold: float, lowered_threshold: float):
  if current_valid:
    current_valid = abs(val) < threshold
  else:
    current_valid = abs(val) < lowered_threshold
  return current_valid


def main():
  config_realtime_process([0, 1, 2, 3], 5)

  DEBUG = bool(int(os.getenv("DEBUG", "0")))
  REPLAY = bool(int(os.getenv("REPLAY", "0")))

  pm = messaging.PubMaster(['liveParameters'])
  sm = messaging.SubMaster(['liveLocationKalman', 'carState'], poll='liveLocationKalman')

  params_reader = Params()
  # wait for stats about the car to come in from controls
  cloudlog.info("paramsd is waiting for CarParams")
  CP = messaging.log_from_bytes(params_reader.get("CarParams", block=True), car.CarParams)
  cloudlog.info("paramsd got CarParams")

  min_sr, max_sr = 0.5 * CP.steerRatio, 2.0 * CP.steerRatio

  params = params_reader.get("LiveParameters")

  # Check if car model matches
  if params is not None:
    params = json.loads(params)
    if params.get('carFingerprint', None) != CP.carFingerprint:
      cloudlog.info("Parameter learner found parameters for wrong car.")
      params = None

  # Check if starting values are sane
  if params is not None:
    try:
      steer_ratio_sane = min_sr <= params['steerRatio'] <= max_sr
      if not steer_ratio_sane:
        cloudlog.info(f"Invalid starting values found {params}")
        params = None
    except Exception as e:
      cloudlog.info(f"Error reading params {params}: {str(e)}")
      params = None

  # TODO: cache the params with the capnp struct
  if params is None:
    params = {
      'carFingerprint': CP.carFingerprint,
      'steerRatio': CP.steerRatio,
      'stiffnessFactor': 1.0,
      'angleOffsetAverageDeg': 0.0,
    }
    cloudlog.info("Parameter learner resetting to default values")

  if not REPLAY:
    # When driving in wet conditions the stiffness can go down, and then be too low on the next drive
    # Without a way to detect this we have to reset the stiffness every drive
    params['stiffnessFactor'] = 1.0

  pInitial = None
  if DEBUG:
    pInitial = np.array(params['debugFilterState']['std']) if 'debugFilterState' in params else None

  learner = ParamsLearner(CP, params['steerRatio'], params['stiffnessFactor'], math.radians(params['angleOffsetAverageDeg']), pInitial)
  angle_offset_average = params['angleOffsetAverageDeg']
  angle_offset = angle_offset_average
  roll = 0.0
  avg_offset_valid = True
  total_offset_valid = True
  roll_valid = True
  offset_fallback_active = False  # 输出回退状态：True 时发布平均偏移
  offset_bad_count = 0  # 偏移异常连续计数
  offset_good_count = 0  # 偏移恢复连续计数
  prev_offset_diff = None  # 记录上一拍差值，用于突跳检测
  offset_fallback_hold_count = 0  # 回退持续计数：用于最大持续时间保险丝
  params_memory = Params("/dev/shm/params")
  params_memory.remove("LastGPSPosition")

  while True:
    sm.update()
    if sm.all_checks():
      for which in sorted(sm.updated.keys(), key=lambda x: sm.logMonoTime[x]):
        if sm.updated[which]:
          t = sm.logMonoTime[which] * 1e-9
          learner.handle_log(t, which, sm[which])

    if sm.updated['liveLocationKalman']:
      location = sm['liveLocationKalman']
      if (location.status == log.LiveLocationKalman.Status.valid) and location.positionGeodetic.valid and location.gpsOK:
        bearing = math.degrees(location.calibratedOrientationNED.value[2])
        lat = location.positionGeodetic.value[0]
        lon = location.positionGeodetic.value[1]
        params_memory.put("LastGPSPosition", json.dumps({"latitude": lat, "longitude": lon, "bearing": bearing}))

      x = learner.kf.x
      P = np.sqrt(learner.kf.P.diagonal())
      if not all(map(math.isfinite, x)):
        cloudlog.error("NaN in liveParameters estimate. Resetting to default values")
        learner = ParamsLearner(CP, CP.steerRatio, 1.0, 0.0)
        x = learner.kf.x

      angle_offset_average = np.clip(math.degrees(x[States.ANGLE_OFFSET].item()),
                                  angle_offset_average - MAX_ANGLE_OFFSET_DELTA, angle_offset_average + MAX_ANGLE_OFFSET_DELTA)
      angle_offset = np.clip(math.degrees(x[States.ANGLE_OFFSET].item() + x[States.ANGLE_OFFSET_FAST].item()),
                          angle_offset - MAX_ANGLE_OFFSET_DELTA, angle_offset + MAX_ANGLE_OFFSET_DELTA)
      roll = np.clip(float(x[States.ROAD_ROLL].item()), roll - ROLL_MAX_DELTA, roll + ROLL_MAX_DELTA)
      roll_std = float(P[States.ROAD_ROLL].item())
      if learner.active and learner.speed > LOW_ACTIVE_SPEED:
        # Account for the opposite signs of the yaw rates
        # At low speeds, bumping into a curb can cause the yaw rate to be very high
        sensors_valid = bool(abs(learner.speed * (x[States.YAW_RATE].item() + learner.yaw_rate)) < LATERAL_ACC_SENSOR_THRESHOLD)
      else:
        sensors_valid = True
      avg_offset_valid = check_valid_with_hysteresis(avg_offset_valid, angle_offset_average, OFFSET_MAX, OFFSET_LOWERED_MAX)
      total_offset_valid = check_valid_with_hysteresis(total_offset_valid, angle_offset, OFFSET_MAX, OFFSET_LOWERED_MAX)
      roll_valid = check_valid_with_hysteresis(roll_valid, roll, ROLL_MAX, ROLL_LOWERED_MAX)

      offset_diff = abs(angle_offset - angle_offset_average)  # 当前瞬时偏移与平均偏移的差值
      prev_diff = prev_offset_diff  # 上一拍差值（用于判断是否出现突跳）
      if prev_diff is None:
        offset_diff_jump = 0.0
      else:
        offset_diff_jump = offset_diff - prev_diff  # 单次更新差值增量（突跳幅度）
      prev_offset_diff = offset_diff  # 更新上一拍差值缓存
      jump_trigger = (prev_diff is not None and
                      prev_diff < OFFSET_DIFF_JUMP_FROM_MAX and
                      offset_diff_jump > OFFSET_DIFF_JUMP_HIGH)  # 只在“低值区->突然抬升”时认为是异常突跳，避免高位抖动误触发

      if (learner.active and learner.speed > LOW_ACTIVE_SPEED and sensors_valid and
          (jump_trigger or (offset_diff > OFFSET_DIFF_HIGH))):  # 高速且传感器可信时，突跳或超阈都计入异常
        offset_bad_count += 1
        offset_good_count = 0
      elif offset_diff < OFFSET_DIFF_LOW:  # 回落到低阈值以下时计入恢复
        offset_good_count += 1
        offset_bad_count = 0
      else:
        # 中间区自然衰减，避免回退状态机在边界附近“粘住”
        offset_bad_count = max(0, offset_bad_count - 1)
        offset_good_count = max(0, offset_good_count - 1)

      if (not offset_fallback_active) and (offset_bad_count >= OFFSET_FALLBACK_ENTER_COUNT):  # 异常持续后进入输出回退
        offset_fallback_active = True
        offset_bad_count = 0
        offset_good_count = 0
        offset_fallback_hold_count = 0
      elif offset_fallback_active and (offset_good_count >= OFFSET_FALLBACK_EXIT_COUNT):  # 恢复持续后退出输出回退
        offset_fallback_active = False
        offset_bad_count = 0
        offset_good_count = 0
        offset_fallback_hold_count = 0

      if offset_fallback_active:
        offset_fallback_hold_count += 1
        if offset_fallback_hold_count >= OFFSET_FALLBACK_MAX_HOLD:  # 保险丝：回退持续过久则强制退出
          offset_fallback_active = False
          offset_bad_count = 0
          offset_good_count = 0
          offset_fallback_hold_count = 0

      effective_angle_offset = angle_offset_average if offset_fallback_active else angle_offset  # 回退时优先用平均偏移抑制发散

      msg = messaging.new_message('liveParameters')

      liveParameters = msg.liveParameters
      liveParameters.posenetValid = True
      liveParameters.sensorValid = sensors_valid
      liveParameters.steerRatio = float(x[States.STEER_RATIO].item())
      liveParameters.steerRatioValid = min_sr <= liveParameters.steerRatio <= max_sr
      liveParameters.stiffnessFactor = float(x[States.STIFFNESS].item())
      liveParameters.stiffnessFactorValid = 0.2 <= liveParameters.stiffnessFactor <= 5.0
      liveParameters.roll = float(roll)
      liveParameters.angleOffsetAverageDeg = float(angle_offset_average)
      liveParameters.angleOffsetAverageValid = bool(avg_offset_valid)
      liveParameters.angleOffsetDeg = float(effective_angle_offset)  # 对外发布的瞬时偏移：回退激活时改发平均偏移，抑制突跳带来的控制抖动
      liveParameters.angleOffsetValid = bool(total_offset_valid)
      liveParameters.valid = all((
        liveParameters.angleOffsetAverageValid,
        liveParameters.angleOffsetValid ,
        roll_valid,
        roll_std < ROLL_STD_MAX,
        liveParameters.stiffnessFactorValid,
        liveParameters.steerRatioValid,
      ))
      liveParameters.steerRatioStd = float(P[States.STEER_RATIO].item())
      liveParameters.stiffnessFactorStd = float(P[States.STIFFNESS].item())
      liveParameters.angleOffsetAverageStd = float(P[States.ANGLE_OFFSET].item())
      liveParameters.angleOffsetFastStd = float(P[States.ANGLE_OFFSET_FAST].item())
      if DEBUG:
        liveParameters.debugFilterState = log.LiveParametersData.FilterState.new_message()
        liveParameters.debugFilterState.value = x.tolist()
        liveParameters.debugFilterState.std = P.tolist()

      msg.valid = sm.all_checks()

      if sm.frame % 1200 == 0:  # once a minute
        params = {
          'carFingerprint': CP.carFingerprint,
          'steerRatio': liveParameters.steerRatio,
          'stiffnessFactor': liveParameters.stiffnessFactor,
          'angleOffsetAverageDeg': liveParameters.angleOffsetAverageDeg,
        }
        params_reader.put_nonblocking("LiveParameters", json.dumps(params))

      pm.send('liveParameters', msg)


if __name__ == "__main__":
  main()
