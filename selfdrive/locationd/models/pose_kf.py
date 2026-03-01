#!/usr/bin/env python3

import sys
import numpy as np

from openpilot.selfdrive.locationd.models.constants import ObservationKind
from openpilot.system.hardware import PC

from rednose.helpers.kalmanfilter import KalmanFilter

if __name__=="__main__":
  import sympy as sp
  from rednose.helpers.ekf_sym import gen_code
  from rednose.helpers.sympy_helpers import euler_rotate, rot_to_euler
else:
  from rednose.helpers.ekf_sym_pyx import EKF_sym_pyx

EARTH_G = 9.81


class States:
  NED_ORIENTATION = slice(0, 3)  # roll, pitch, yaw in rad
  DEVICE_VELOCITY = slice(3, 6)  # ned velocity in m/s
  ANGULAR_VELOCITY = slice(6, 9)  # roll, pitch and yaw rates in rad/s
  GYRO_BIAS = slice(9, 12)  # roll, pitch and yaw gyroscope biases in rad/s
  ACCELERATION = slice(12, 15)  # acceleration in device frame in m/s**2
  ACCEL_BIAS = slice(15, 18)  # Acceletometer bias in m/s**2


class PoseKalman(KalmanFilter):
  name = "pose"

  # state
  initial_x = np.array([0.0, 0.0, 0.0,
                        0.0, 0.0, 0.0,
                        0.0, 0.0, 0.0,
                        0.0, 0.0, 0.0,
                        0.0, 0.0, 0.0,
                        0.0, 0.0, 0.0])
  # state covariance
  initial_P = np.diag([0.01**2, 0.01**2, 0.01**2,
                       10**2, 10**2, 10**2,
                       1**2, 1**2, 1**2,
                       1**2, 1**2, 1**2,
                       100**2, 100**2, 100**2,
                       0.01**2, 0.01**2, 0.01**2])

  # process noise
  Q = np.diag([0.001**2, 0.001**2, 0.001**2,
               0.01**2, 0.01**2, 0.01**2,
               0.1**2, 0.1**2, 0.1**2,
               (0.005 / 100)**2, (0.005 / 100)**2, (0.005 / 100)**2,
               3**2, 3**2, 3**2,
               0.005**2, 0.005**2, 0.005**2])

  # 根据平台适配陀螺仪噪声配置
  # 基于官方Datasheet精确计算，使用相同的安全系数（4.73x）
  if PC:
    # PC平台使用JY62陀螺仪（WT901SD模块，内置MPU6050）
    # 官方规格：噪声密度5.0 mdps/√Hz，零偏20 mdps，温度漂移0.07 deg/s/°C
    # 计算值：0.02346 rad/s (1.34°/s)，安全系数4.73x
    gyro_obs_noise = 0.111007  # rad/s (6.36°/s, 基于MPU6050官方Datasheet)
  else:
    # TICI平台使用LSM6DS3陀螺仪
    # 官方规格：噪声密度3.8 mdps/√Hz，零偏10 mdps，温度漂移0.03 deg/s/°C
    # 计算值：0.00528 rad/s (0.30°/s)，安全系数4.73x
    gyro_obs_noise = 0.025  # rad/s (1.43°/s, 基于LSM6DS3官方Datasheet)

  obs_noise = {ObservationKind.PHONE_GYRO: np.diag([gyro_obs_noise**2, gyro_obs_noise**2, gyro_obs_noise**2]),
               ObservationKind.PHONE_ACCEL: np.diag([.5**2, .5**2, .5**2]),
               ObservationKind.CAMERA_ODO_TRANSLATION: np.diag([0.5**2, 0.5**2, 0.5**2]),
               ObservationKind.CAMERA_ODO_ROTATION: np.diag([0.05**2, 0.05**2, 0.05**2])}

  @staticmethod
  def generate_code(generated_dir):
    name = PoseKalman.name
    dim_state = PoseKalman.initial_x.shape[0]
    dim_state_err = PoseKalman.initial_P.shape[0]

    state_sym = sp.MatrixSymbol('state', dim_state, 1)
    state = sp.Matrix(state_sym)
    roll, pitch, yaw = state[States.NED_ORIENTATION, :]
    velocity = state[States.DEVICE_VELOCITY, :]
    angular_velocity = state[States.ANGULAR_VELOCITY, :]
    vroll, vpitch, vyaw = angular_velocity
    gyro_bias = state[States.GYRO_BIAS, :]
    acceleration = state[States.ACCELERATION, :]
    acc_bias = state[States.ACCEL_BIAS, :]

    dt = sp.Symbol('dt')

    ned_from_device = euler_rotate(roll, pitch, yaw)
    device_from_ned = ned_from_device.T

    state_dot = sp.Matrix(np.zeros((dim_state, 1)))
    state_dot[States.DEVICE_VELOCITY, :] = acceleration

    f_sym = state + dt * state_dot
    device_from_device_t1 = euler_rotate(dt*vroll, dt*vpitch, dt*vyaw)
    ned_from_device_t1 = ned_from_device * device_from_device_t1
    f_sym[States.NED_ORIENTATION, :] = rot_to_euler(ned_from_device_t1)

    centripetal_acceleration = angular_velocity.cross(velocity)
    gravity = sp.Matrix([0, 0, -EARTH_G])
    h_gyro_sym = angular_velocity + gyro_bias
    h_acc_sym = device_from_ned * gravity + acceleration + centripetal_acceleration + acc_bias
    h_phone_rot_sym = angular_velocity
    h_relative_motion_sym = velocity
    obs_eqs = [
      [h_gyro_sym, ObservationKind.PHONE_GYRO, None],
      [h_acc_sym, ObservationKind.PHONE_ACCEL, None],
      [h_relative_motion_sym, ObservationKind.CAMERA_ODO_TRANSLATION, None],
      [h_phone_rot_sym, ObservationKind.CAMERA_ODO_ROTATION, None],
    ]
    gen_code(generated_dir, name, f_sym, dt, state_sym, obs_eqs, dim_state, dim_state_err)

  def __init__(self, generated_dir, max_rewind_age):
    dim_state, dim_state_err = PoseKalman.initial_x.shape[0], PoseKalman.initial_P.shape[0]
    self.filter = EKF_sym_pyx(generated_dir, self.name, PoseKalman.Q, PoseKalman.initial_x, PoseKalman.initial_P,
                              dim_state, dim_state_err, max_rewind_age=max_rewind_age)


if __name__ == "__main__":
  generated_dir = sys.argv[2]
  PoseKalman.generate_code(generated_dir)
