# Neural Network Lateral Control
import numpy as np

LOW_SPEED_X = [0, 10, 20, 30]
LOW_SPEED_Y = [12, 3, 1, 0]

class NeuralNetworkLateralControl:
  def __init__(self, lac_torque, CP, CP_SP, CI):
    self.lac_torque = lac_torque
    self.CP = CP
    self.CP_SP = CP_SP
    self.CI = CI
    self.future_times = [0.3, 0.6, 1.0, 1.5]
    self.nn_future_times = [i + 0.2 for i in self.future_times]

  def update_calculations(self, CS, VM, desired_lateral_accel):
    pass

  def update_neural_network_feedforward(self, CS, params, calibrated_pose):
    pass

  @property
  def _ff(self):
    return 0.0

  @property
  def _pid_log(self):
    return None
