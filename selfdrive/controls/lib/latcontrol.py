import numpy as np
from abc import abstractmethod, ABC

from openpilot.common.realtime import DT_CTRL

MIN_LATERAL_CONTROL_SPEED = 0.3  # m/s


class LatControl(ABC):
  def __init__(self, CP, CI, CP_SP=None):
    self.sat_count_rate = 1.0 * DT_CTRL
    self.sat_limit = CP.steerLimitTimer
    self.sat_count = 0.
    self.sat_check_min_speed = 10.

    # we define steer torque scale as [-1.0...1.0]
    self.steer_max = 1.0

    # SunnyPilot additions
    self.CP_SP = CP_SP if CP_SP is not None else type('CP_SP', (), {'flags': 0})()

  @abstractmethod
  def update(self, active, CS, VM, params, steer_limited_by_controls, desired_curvature, llk, curvature_limited, model_data=None):
    pass

  def reset(self):
    self.sat_count = 0.

  def _check_saturation(self, saturated, CS, steer_limited_by_safety, curvature_limited):
    # Smart saturation detection: longer tolerance for sustained curves, stricter for city driving
    # Allow longer sustained control for highway curves and spiral ramps
    if CS.vEgo > 15:  # Highway/spiral ramp speeds
      # More permissive for sustained curves: slower accumulation, longer limit
      sat_rate = self.sat_count_rate * 0.3  # 3x slower accumulation
      sat_limit = self.sat_limit * 2.5    # 2.5x longer tolerance (20s at 100km/h)
    else:  # City driving
      # Normal sensitivity for city driving
      sat_rate = self.sat_count_rate
      sat_limit = self.sat_limit

    # Saturated only if control output is not being limited by car torque/angle rate limits
    if (saturated or curvature_limited) and CS.vEgo > self.sat_check_min_speed and not steer_limited_by_safety and not CS.steeringPressed:
      self.sat_count += sat_rate
    else:
      self.sat_count -= sat_rate
    self.sat_count = np.clip(self.sat_count, 0.0, sat_limit)
    return self.sat_count > (sat_limit - 1e-3)