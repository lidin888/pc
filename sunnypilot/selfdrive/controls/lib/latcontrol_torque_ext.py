from openpilot.sunnypilot.selfdrive.controls.lib.nnlc.nnlc import NeuralNetworkLateralControl


class LatControlTorqueExt(NeuralNetworkLateralControl):
  def __init__(self, lac_torque, CP, CP_SP, CI):
    super().__init__(lac_torque, CP, CP_SP, CI)

  def update(self, CS, VM, params, ff, pid_log, setpoint, measurement, calibrated_pose, roll_compensation,
             desired_lateral_accel, actual_lateral_accel, lateral_accel_deadzone, gravity_adjusted_lateral_accel):
    # Update neural network feedforward
    self.update_calculations(CS, VM, desired_lateral_accel)
    self.update_neural_network_feedforward(CS, params, calibrated_pose)

    return self._ff, self._pid_log
