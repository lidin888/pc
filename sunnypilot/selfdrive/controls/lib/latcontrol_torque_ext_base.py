# Lateral Control Torque Extension Base

class LatControlTorqueExtBase:
  def __init__(self, lac_torque, CP, CP_SP, CI):
    self.lac_torque = lac_torque
    self.CP = CP
    self.CP_SP = CP_SP
    self.CI = CI
    self.desired_lat_jerk_time = 0.2

  def update_lateral_lag(self, lag):
    self.lateral_lag = lag

  def torque_from_lateral_accel_in_torque_space(self, lat_inputs, torque_params, gravity_adjusted=False):
    return 0.0

def sign(x):
  return 1 if x >= 0 else -1
