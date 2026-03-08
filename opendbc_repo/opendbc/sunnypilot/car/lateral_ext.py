"""
Copyright (c) 2021-, Haibin Wen, sunnypilot, and a number of other contributors.

This file is part of sunnypilot and is licensed under the MIT License.
See the LICENSE.md file in the root directory for more details.
"""
import numpy as np
from opendbc.car import structs


def apply_center_deadzone(error: float, deadzone: float) -> float:
    """Apply center deadzone to error value"""
    if abs(error) < deadzone:
        return 0.0
    elif error > 0:
        return error - deadzone
    else:
        return error + deadzone


def get_friction(lateral_accel_error: float, lateral_accel_deadzone: float, friction_threshold: float,
                 torque_params: structs.CarParams.LateralTorqueTuning) -> float:
    """
    Extended friction calculation for torque space
    Used by neural network lateral control system
    """
    # TODO torque params' friction should be in lateral accel space, not torque space
    friction_interp = np.interp(
        apply_center_deadzone(lateral_accel_error, lateral_accel_deadzone),
        [-friction_threshold, friction_threshold],
        [-torque_params.friction * torque_params.latAccelFactor, torque_params.friction * torque_params.latAccelFactor]
    )
    return float(friction_interp)