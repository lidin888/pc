#!/usr/bin/env bash

# 独立模式启动脚本
# 确保使用当前目录的项目而不是sunnypilot-pc文件夹

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null && pwd )"

echo "启动独立模式，使用项目目录: $DIR"

# 检查虚拟环境是否存在
if [ ! -f "$DIR/.venv/bin/activate" ]; then
    echo "错误: 在当前目录没有找到虚拟环境"
    echo "请先运行: tools/op.sh --standalone venv 或 tools/op.sh --standalone setup"
    exit 1
fi

# 激活当前目录的虚拟环境
source "$DIR/.venv/bin/activate"

# 强制设置Python路径和库路径
export PYTHONPATH="$DIR"
export LD_LIBRARY_PATH="$DIR/third_party/acados/x86_64/lib:$DIR/selfdrive/controls/lib/longitudinal_mpc_lib/c_generated_code:$DIR/selfdrive/controls/lib/lateral_mpc_lib/c_generated_code:$LD_LIBRARY_PATH"

# 检查是否在当前目录有manager.py文件
if [ ! -f "$DIR/system/manager/manager.py" ]; then
    echo "错误: 在当前目录没有找到manager.py文件"
    echo "请确保你在正确的项目目录中"
    exit 1
fi

# 设置环境变量（根据你的需求调整）
export USE_WEBCAM=1
export ROAD_CAM=4
export NO_DM=0

echo "环境变量设置:"
echo "  虚拟环境: $DIR/.venv"
echo "  USE_WEBCAM=$USE_WEBCAM"
echo "  ROAD_CAM=$ROAD_CAM"
echo "  NO_DM=$NO_DM"
echo "  PYTHONPATH=$PYTHONPATH"
echo "  LD_LIBRARY_PATH=$LD_LIBRARY_PATH"

# 检查关键库文件是否存在
ACADOS_LIB="$DIR/third_party/acados/x86_64/lib/libacados.so"
if [ ! -f "$ACADOS_LIB" ]; then
    echo "错误: libacados.so 不存在: $ACADOS_LIB"
    echo "请检查 acados 第三方库是否完整"
    exit 1
else
    echo "✓ 找到 libacados.so: $ACADOS_LIB"
fi

# 检查lagd模块是否存在，如果不存在则临时修复配置文件
LAGD_FILE="$DIR/selfdrive/locationd/lagd.py"
if [ ! -f "$LAGD_FILE" ]; then
    echo "警告: lagd模块不存在，临时修复配置文件..."
    # 备份原始配置文件
    cp "$DIR/system/manager/process_config.py" "$DIR/system/manager/process_config.py.backup"
    # 临时注释掉lagd模块
    sed -i 's/PythonProcess("lagd", "selfdrive.locationd.lagd", only_onroad),/#PythonProcess("lagd", "selfdrive.locationd.lagd", only_onroad),  # lagd模块不存在，暂时注释掉/' "$DIR/system/manager/process_config.py"
    echo "已临时注释掉lagd模块"
fi

# 进入manager目录并启动
cd "$DIR/system/manager"

echo "启动manager..."
python3 manager.py

# 恢复原始配置文件（如果修改过）
if [ -f "$DIR/system/manager/process_config.py.backup" ]; then
    mv "$DIR/system/manager/process_config.py.backup" "$DIR/system/manager/process_config.py"
    echo "已恢复原始配置文件"
fi

echo "独立模式启动完成"
4.修复sunnypilot-dev开发版本模型下载问题
替换/sunnypilot/models/目录下，manager.py和fetcher.py两个文件

5.解决和减轻画龙的问题

修改selfdrive/controls/lib/latcontrol_pid.py文件，增加pid参数自学习迭代和定期更新功能
这个文件几乎被完全重写了，完整示例代码：
import math
import numpy as np

from cereal import log
from openpilot.selfdrive.controls.lib.latcontrol import LatControl
from openpilot.common.pid import PIDController
from openpilot.common.params import Params
from openpilot.common.swaglog import cloudlog


class AdaptivePIDController:
  """自适应PID控制器，实现参数自学习和画龙行为抑制"""

  def __init__(self, kp_bp, kp_v, ki_bp, ki_v, kf, params=None):
    self.base_kp_bp = kp_bp
    self.base_kp_v = kp_v
    self.base_ki_bp = ki_bp
    self.base_ki_v = ki_v
    self.base_kf = kf
    self.params = params

    # 尝试从持久化存储加载学习参数
    self.kp_scale = self._load_param("AdaptivePIDKpScale", 1.0)
    self.ki_scale = self._load_param("AdaptivePIDKiScale", 1.0)
    self.kd_scale = self._load_param("AdaptivePIDKdScale", 1.0)

    # 学习统计
    self.total_learning_time = self._load_param("AdaptivePIDLearningTime", 0)
    self.oscillation_detected_count = self._load_param("AdaptivePIDOscillationCount", 0)

    # 学习参数
    self.learning_rate = 0.001
    self.oscillation_threshold = 0.5  # 画龙检测阈值
    self.min_kp_scale = 0.3
    self.max_kp_scale = 3.0

    # 状态跟踪
    self.error_history = []
    self.max_history_length = 50
    self.last_errors = []
    self.oscillation_count = 0

    # 增量学习相关
    self.learning_epoch = 0
    self.best_performance_score = float('inf')
    self.best_kp_scale = self.kp_scale
    self.best_ki_scale = self.ki_scale

    # 性能历史记录
    self.performance_history = []
    self.max_performance_history = 1000

  def _load_param(self, param_name, default_value):
    """从持久化存储加载参数"""
    if self.params:
      try:
        value = self.params.get_float(param_name)
        return value if value is not None else default_value
      except:
        pass
    return default_value

  def _save_param(self, param_name, value):
    """保存参数到持久化存储"""
    if self.params:
      try:
        self.params.put_float(param_name, float(value))
      except:
        pass

  def save_learned_parameters(self):
    """保存学习到的参数"""
    self._save_param("AdaptivePIDKpScale", self.kp_scale)
    self._save_param("AdaptivePIDKiScale", self.ki_scale)
    self._save_param("AdaptivePIDKdScale", self.kd_scale)
    self._save_param("AdaptivePIDLearningTime", self.total_learning_time)
    self._save_param("AdaptivePIDOscillationCount", self.oscillation_detected_count)

    # 保存最佳参数（用于回滚）
    self._save_param("AdaptivePIDBestKpScale", self.best_kp_scale)
    self._save_param("AdaptivePIDBestKiScale", self.best_ki_scale)
    self._save_param("AdaptivePIDBestPerformance", self.best_performance_score)

  def detect_oscillation(self, current_error):
    """检测画龙行为"""
    if len(self.last_errors) < 3:
      self.last_errors.append(current_error)
      return False

    # 计算误差变化模式
    error_changes = [abs(self.last_errors[i] - self.last_errors[i-1]) for i in range(1, len(self.last_errors))]

    # 检测周期性振荡
    if len(error_changes) >= 2:
      oscillation_score = np.std(error_changes) / (np.mean(error_changes) + 1e-6)

      # 更新误差历史
      self.last_errors.pop(0)
      self.last_errors.append(current_error)

      return oscillation_score > self.oscillation_threshold

    self.last_errors.append(current_error)
    return False

  def adapt_parameters(self, error, speed, steering_rate):
    """自适应调整PID参数，实现增量学习"""
    # 检测画龙行为
    is_oscillating = self.detect_oscillation(error)

    if is_oscillating:
      # 抑制画龙：降低P增益，增加D增益
      self.kp_scale = max(self.kp_scale * 0.95, self.min_kp_scale)
      self.kd_scale = min(self.kd_scale * 1.05, 2.0)
      self.oscillation_count += 1
      self.oscillation_detected_count += 1
    else:
      # 正常情况下的自适应
      error_abs = abs(error)

      # 根据误差大小调整
      if error_abs > 2.0:  # 大误差
        self.kp_scale = min(self.kp_scale * 1.02, self.max_kp_scale)
      elif error_abs < 0.5:  # 小误差
        self.kp_scale = max(self.kp_scale * 0.98, self.min_kp_scale)

      # 根据车速调整
      if speed > 20:  # 高速
        self.ki_scale = max(self.ki_scale * 0.95, 0.5)
      else:  # 低速
        self.ki_scale = min(self.ki_scale * 1.02, 1.5)

      self.oscillation_count = max(self.oscillation_count - 1, 0)

    # 增量学习：定期评估性能并优化参数
    self.learning_epoch += 1
    if self.learning_epoch % 100 == 0:  # 每100个周期评估一次
      self._evaluate_performance()

    # 更新学习时间
    self.total_learning_time += 1

    # 获取调整后的参数
    kp = np.interp(speed, self.base_kp_bp, self.base_kp_v) * self.kp_scale
    ki = np.interp(speed, self.base_ki_bp, self.base_ki_v) * self.ki_scale

    return kp, ki, self.base_kf

  def _evaluate_performance(self):
    """评估控制性能并优化参数"""
    if len(self.error_history) < 10:
      return

    # 计算性能指标
    error_rms = np.sqrt(np.mean(np.square(self.error_history)))
    error_max = np.max(np.abs(self.error_history))

    # 综合性能评分（越小越好）
    performance_score = error_rms + 0.1 * error_max + 0.5 * self.oscillation_count

    # 记录性能历史
    self.performance_history.append(performance_score)
    if len(self.performance_history) > self.max_performance_history:
      self.performance_history.pop(0)

    # 更新最佳参数
    if performance_score < self.best_performance_score:
      self.best_performance_score = performance_score
      self.best_kp_scale = self.kp_scale
      self.best_ki_scale = self.ki_scale

      # 保存最佳参数
      self.save_learned_parameters()

    # 如果性能持续下降，考虑回滚到最佳参数
    if len(self.performance_history) >= 50:
      recent_performance = np.mean(self.performance_history[-50:])
      historical_performance = np.mean(self.performance_history[-100:-50])

      if recent_performance > historical_performance * 1.2:  # 性能下降20%
        # 回滚到最佳参数
        self.kp_scale = self.best_kp_scale
        self.ki_scale = self.best_ki_scale
        self.performance_history.clear()  # 重置性能历史

  def get_learning_summary(self):
    """获取学习摘要"""
    return {
      'total_learning_time': self.total_learning_time,
      'oscillation_detected_count': self.oscillation_detected_count,
      'current_kp_scale': self.kp_scale,
      'current_ki_scale': self.ki_scale,
      'best_performance_score': self.best_performance_score,
      'performance_history_length': len(self.performance_history)
    }


class LatControlPID(LatControl):
  def __init__(self, CP, CI):
    super().__init__(CP, CI)

    self.params = Params()

    # 创建自适应PID控制器（传入params用于持久化）
    self.adaptive_pid = AdaptivePIDController(
      CP.lateralTuning.pid.kpBP, CP.lateralTuning.pid.kpV,
      CP.lateralTuning.pid.kiBP, CP.lateralTuning.pid.kiV,
      CP.lateralTuning.pid.kf,
      params=self.params
    )

    # 基础PID控制器
    self.pid = PIDController((CP.lateralTuning.pid.kpBP, CP.lateralTuning.pid.kpV),
                             (CP.lateralTuning.pid.kiBP, CP.lateralTuning.pid.kiV),
                             k_f=CP.lateralTuning.pid.kf, pos_limit=self.steer_max, neg_limit=-self.steer_max)

    self.get_steer_feedforward = CI.get_steer_feedforward_function()

    # 性能监控
    self.performance_metrics = {
      'oscillation_detected': False,
      'adaptive_kp': 1.0,
      'adaptive_ki': 1.0,
      'error_rms': 0.0,
      'learning_summary': {}
    }

    # 定期保存计时器
    self.save_counter = 0
    self.save_interval = 300  # 每300个周期保存一次

  def reset(self):
    super().reset()
    self.pid.reset()

  def update(self, active, CS, VM, params, steer_limited_by_controls, desired_curvature, llk, curvature_limited, model_data=None):
    pid_log = log.ControlsState.LateralPIDState.new_message()
    pid_log.steeringAngleDeg = float(CS.steeringAngleDeg)
    pid_log.steeringRateDeg = float(CS.steeringRateDeg)

    angle_steers_des_no_offset = math.degrees(VM.get_steer_from_curvature(-desired_curvature, CS.vEgo, params.roll))
    angle_steers_des = angle_steers_des_no_offset + params.angleOffsetDeg
    error = angle_steers_des - CS.steeringAngleDeg

    # 更新误差历史用于性能分析
    self.adaptive_pid.error_history.append(error)
    if len(self.adaptive_pid.error_history) > self.adaptive_pid.max_history_length:
      self.adaptive_pid.error_history.pop(0)

    pid_log.steeringAngleDesiredDeg = angle_steers_des
    pid_log.angleError = error

    if not active:
      output_steer = 0.0
      pid_log.active = False
      self.pid.reset()
      # 重置自适应参数
      self.adaptive_pid.kp_scale = 1.0
      self.adaptive_pid.ki_scale = 1.0
      self.adaptive_pid.kd_scale = 1.0
      self.adaptive_pid.oscillation_count = 0
    else:
      # 自适应参数调整
      kp, ki, kf = self.adaptive_pid.adapt_parameters(error, CS.vEgo, CS.steeringRateDeg)

      # 更新PID控制器参数
      self.pid._k_p = (self.adaptive_pid.base_kp_bp, [v * self.adaptive_pid.kp_scale for v in self.adaptive_pid.base_kp_v])
      self.pid._k_i = (self.adaptive_pid.base_ki_bp, [v * self.adaptive_pid.ki_scale for v in self.adaptive_pid.base_ki_v])

      # offset does not contribute to resistive torque
      steer_feedforward = self.get_steer_feedforward(angle_steers_des_no_offset, CS.vEgo)

      output_steer = self.pid.update(error, override=CS.steeringPressed,
                                     feedforward=steer_feedforward, speed=CS.vEgo)

      # 更新性能监控
      self.performance_metrics['oscillation_detected'] = self.adaptive_pid.detect_oscillation(error)
      self.performance_metrics['adaptive_kp'] = self.adaptive_pid.kp_scale
      self.performance_metrics['adaptive_ki'] = self.adaptive_pid.ki_scale
      self.performance_metrics['error_rms'] = np.sqrt(np.mean(np.square(self.adaptive_pid.error_history)))
      self.performance_metrics['learning_summary'] = self.adaptive_pid.get_learning_summary()

      # 定期保存学习参数
      self.save_counter += 1
      if self.save_counter >= self.save_interval:
        self.adaptive_pid.save_learned_parameters()
        self.save_counter = 0

        # 记录保存日志
        cloudlog.info(f"自适应PID参数已保存: KP={self.adaptive_pid.kp_scale:.3f}, KI={self.adaptive_pid.ki_scale:.3f}, "
                     f"学习时间={self.adaptive_pid.total_learning_time}")

      # 记录自适应参数到日志
      pid_log.active = True
      pid_log.p = float(self.pid.p)
      pid_log.i = float(self.pid.i)
      pid_log.f = float(self.pid.f)
      pid_log.output = float(output_steer)
      pid_log.saturated = bool(self._check_saturation(self.steer_max - abs(output_steer) < 1e-3, CS, steer_limited_by_controls, curvature_limited))

      # 添加自适应参数到日志
      pid_log.adaptiveKpScale = float(self.adaptive_pid.kp_scale)
      pid_log.adaptiveKiScale = float(self.adaptive_pid.ki_scale)
      pid_log.oscillationCount = int(self.adaptive_pid.oscillation_count)
      pid_log.totalLearningTime = int(self.adaptive_pid.total_learning_time)

    return output_steer, angle_steers_des, pid_log

  def save_final_parameters(self):
    """在控制器关闭时保存最终参数"""
    if hasattr(self, 'adaptive_pid'):
      self.adaptive_pid.save_learned_parameters()
      cloudlog.info("自适应PID控制器: 最终参数已保存")

  def get_performance_report(self):
    """获取性能报告"""
    if hasattr(self, 'adaptive_pid'):
      return {
        'current_parameters': {
          'kp_scale': self.adaptive_pid.kp_scale,
          'ki_scale': self.adaptive_pid.ki_scale,
          'kd_scale': self.adaptive_pid.kd_scale
        },
        'learning_statistics': self.adaptive_pid.get_learning_summary(),
        'performance_metrics': self.performance_metrics
      }
return {}
