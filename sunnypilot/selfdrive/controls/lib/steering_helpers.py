# Steering Learning Helper Functions
# Copyright (c) 2021-, Haibin Wen, sunnypilot, and a number of other contributors.

import numpy as np
from collections import deque

class SteeringDelayLearner:
  def __init__(self, window_size=100, learning_rate=0.01):
    self.window_size = window_size
    self.learning_rate = learning_rate
    self.steering_angle_history = deque(maxlen=window_size)
    self.desired_angle_history = deque(maxlen=window_size)
    self.delay_estimate = 0.2  # Initial estimate
    self.confidence = 0.0

  def update(self, steering_angle, desired_angle, v_ego):
    """Update delay estimate based on steering angle and desired angle"""
    self.steering_angle_history.append(steering_angle)
    self.desired_angle_history.append(desired_angle)

    if len(self.steering_angle_history) < self.window_size:
      return self.delay_estimate

    # Calculate cross-correlation to find delay
    delay_samples = self._find_delay_samples()
    delay_seconds = delay_samples * 0.01  # Assuming 100Hz update rate

    # Update estimate with learning rate
    self.delay_estimate = (1 - self.learning_rate) * self.delay_estimate + self.learning_rate * delay_seconds

    # Update confidence based on correlation strength
    self.confidence = min(1.0, len(self.steering_angle_history) / self.window_size)

    return self.delay_estimate

  def _find_delay_samples(self):
    """Find delay in samples using cross-correlation"""
    if len(self.steering_angle_history) < 10:
      return 2  # Default 2 samples (0.02s at 100Hz)

    # Simple cross-correlation implementation
    # In a real implementation, this would be more sophisticated
    steering_array = np.array(self.steering_angle_history)
    desired_array = np.array(self.desired_angle_history)

    # Find the delay that maximizes correlation
    max_corr = 0
    best_delay = 2  # Default

    for delay in range(1, 10):  # Check delays from 1 to 9 samples
      if delay >= len(desired_array):
        continue

      # Shift desired angle by delay samples
      shifted_desired = desired_array[delay:]
      steering_trimmed = steering_array[:len(shifted_desired)]

      if len(steering_trimmed) < 10:
        continue

      # Calculate correlation
      corr = np.corrcoef(steering_trimmed, shifted_desired)[0, 1]

      if not np.isnan(corr) and corr > max_corr:
        max_corr = corr
        best_delay = delay

    return best_delay

class LaneTurnDesireLearner:
  def __init__(self, learning_rate=0.01, window_size=200):
    self.learning_rate = learning_rate
    self.window_size = window_size
    self.steering_history = deque(maxlen=window_size)
    self.lane_width_history = deque(maxlen=window_size)
    self.speed_history = deque(maxlen=window_size)
    self.desired_angle_history = deque(maxlen=window_size)

    # Learned parameters
    self.speed_factor = 1.0
    self.lane_width_factor = 1.0
    self.curvature_factor = 1.0

  def update(self, steering_angle, desired_angle, lane_width, v_ego):
    """Update lane turn desire learning"""
    self.steering_history.append(steering_angle)
    self.desired_angle_history.append(desired_angle)
    self.lane_width_history.append(lane_width)
    self.speed_history.append(v_ego)

    if len(self.steering_history) < self.window_size:
      return

    # Update factors based on driving data
    self._update_factors()

  def _update_factors(self):
    """Update learning factors based on collected data"""
    # Extract arrays
    steering_array = np.array(self.steering_history)
    desired_array = np.array(self.desired_angle_history)
    lane_width_array = np.array(self.lane_width_history)
    speed_array = np.array(self.speed_history)

    # Calculate errors
    error = desired_array - steering_array

    # Update speed factor (lower speed = higher response)
    speed_effect = np.abs(error) / (speed_array + 1e-5)
    self.speed_factor = (1 - self.learning_rate) * self.speed_factor + self.learning_rate * np.mean(speed_effect)

    # Update lane width factor
    lane_width_effect = np.abs(error) / (lane_width_array + 1e-5)
    self.lane_width_factor = (1 - self.learning_rate) * self.lane_width_factor + self.learning_rate * np.mean(lane_width_effect)

    # Update curvature factor
    curvature_effect = np.abs(error) / (np.abs(desired_array) + 1e-5)
    self.curvature_factor = (1 - self.learning_rate) * self.curvature_factor + self.learning_rate * np.mean(curvature_effect)

  def get_enhanced_desired_angle(self, desired_angle, v_ego, lane_width):
    """Get enhanced desired angle based on learned factors"""
    # Apply learned factors
    speed_adjustment = 1.0 / (1.0 + self.speed_factor * (v_ego - 10.0))  # Normalize around 10 m/s
    lane_width_adjustment = 1.0 / (1.0 + self.lane_width_factor * (lane_width - 3.5))  # Normalize around 3.5m
    curvature_adjustment = 1.0 + self.curvature_factor * np.abs(desired_angle)  # Increase response for higher curvature

    enhanced_angle = desired_angle * speed_adjustment * lane_width_adjustment * curvature_adjustment

    return enhanced_angle
