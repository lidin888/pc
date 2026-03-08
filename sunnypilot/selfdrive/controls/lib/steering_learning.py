# Steering Learning Implementation
# Copyright (c) 2021-, Haibin Wen, sunnypilot, and a number of other contributors.

import numpy as np
from openpilot.common.params import Params
from opendbc.car import structs

class SteeringLearning:
  def __init__(self):
    self.params = Params()
    self.lane_turn_desire_enabled = False
    self.live_delay_enabled = False
    self.lane_turn_value = 19.0  # Default 19 mph
    self.software_delay = 0.2  # Default 0.2s

    # Load saved parameters
    self.load_parameters()

  def load_parameters(self):
    """Load parameters from storage"""
    try:
      self.lane_turn_desire_enabled = self.params.get_bool("LaneTurnDesire")
      self.live_delay_enabled = self.params.get_bool("LagdToggle")

      lane_turn_value_str = self.params.get("LaneTurnValue")
      if lane_turn_value_str:
        self.lane_turn_value = float(lane_turn_value_str)

      delay_str = self.params.get("LagdToggleDelay")
      if delay_str:
        self.software_delay = float(delay_str)
    except:
      # Use default values if parameters not found
      pass

  def get_lane_turn_speed_limit(self):
    """Get lane turn speed limit in m/s"""
    # Convert from mph to m/s
    return self.lane_turn_value * 0.44704

  def get_effective_delay(self, measured_delay=None):
    """Get effective steering delay based on learning settings"""
    if self.live_delay_enabled and measured_delay is not None:
      # Use live learned delay
      return measured_delay
    else:
      # Use software delay setting
      return self.software_delay

  def update_live_delay(self, lateral_delay):
    """Update live learned delay"""
    if self.live_delay_enabled:
      # Save the learned delay
      self.params.put("LiveDelay", str(lateral_delay))
