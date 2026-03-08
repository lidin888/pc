#!/usr/bin/env python3

import os

def create_tn_ui_files():
    """创建tn相关的UI文件"""
    print("创建tn相关的UI文件...")

    # 1. 创建转向学习设置头文件
    steering_settings_h = """/**
 * Copyright (c) 2021-, Haibin Wen, sunnypilot, and a number of other contributors.
 *
 * This file is part of sunnypilot and is licensed under the MIT License.
 * See the LICENSE.md file in the root directory for more details.
 */

#pragma once

#include "selfdrive/ui/sunnypilot/qt/offroad/settings/vehicle/brand_settings_interface.h"

#include "selfdrive/ui/qt/util.h"
#include "selfdrive/ui/sunnypilot/ui.h"
#include "selfdrive/ui/sunnypilot/qt/offroad/settings/settings.h"
#include "selfdrive/ui/sunnypilot/qt/widgets/controls.h"

class SteeringSettings : public QWidget {
  Q_OBJECT

public:
  explicit SteeringSettings(QWidget *parent = nullptr);
  void updateSettings();

private:
  bool offroad = false;
};
"""

    # 2. 创建转向学习设置实现文件
    steering_settings_cc = """/**
 * Copyright (c) 2021-, Haibin Wen, sunnypilot, and a number of other contributors.
 *
 * This file is part of sunnypilot and is licensed under the MIT License.
 * See the LICENSE.md file in the root directory for more details.
 */

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>

#include "selfdrive/ui/sunnypilot/qt/offroad/settings/steering_settings.h"
#include "selfdrive/ui/sunnypilot/qt/widgets/scrollview.h"
#include "common/params.h"

SteeringSettings::SteeringSettings(QWidget *parent) : QWidget(parent) {
  QVBoxLayout *main_layout = new QVBoxLayout(this);
  main_layout->setContentsMargins(50, 20, 50, 20);

  ListWidgetSP *list = new ListWidgetSP(this, false);
  ScrollViewSP *scroller = new ScrollViewSP(list, this);
  main_layout->addWidget(scroller);

  // Lane Turn Desire toggle
  lane_turn_desire_toggle = new ParamControlSP("LaneTurnDesire", tr("Use Lane Turn Desires"),
    tr("Enable lane turn desire learning for improved steering response"),
    "../assets/offroad/icon_shell.png");
  list->addItem(lane_turn_desire_toggle);

  // Lane Turn Value control
  int max_value_mph = 20;
  bool is_metric_initial = params.getBool("IsMetric");
  const float K = 1.609344f;
  int per_value_change_scaled = is_metric_initial ? static_cast<int>(std::round((1.0f / K) * 100.0f)) : 100; // 100 -> 1 mph
  lane_turn_value_control = new OptionControlSP("LaneTurnValue", tr("Adjust Lane Turn Speed"),
    tr("Set the maximum speed for lane turn desires. Default is 19 %1.").arg(is_metric_initial ? "km/h" : "mph"),
    "", {5 * 100, max_value_mph * 100}, per_value_change_scaled, false, nullptr, true, true);
  lane_turn_value_control->showDescription();
  list->addItem(lane_turn_value_control);

  // Show based on toggle
  refreshLaneTurnValueControl();
  connect(lane_turn_desire_toggle, &ParamControlSP::toggleFlipped, this, &SteeringSettings::refreshLaneTurnValueControl);
  connect(lane_turn_value_control, &OptionControlSP::updateLabels, this, &SteeringSettings::refreshLaneTurnValueControl);

  // LiveDelay toggle
  lagd_toggle_control = new ParamControlSP("LagdToggle", tr("Live Learning Steer Delay"),
    tr("Enable real-time learning of steering delay for improved control"),
    "../assets/offroad/icon_shell.png");
  lagd_toggle_control->showDescription();
  list->addItem(lagd_toggle_control);

  // Software delay control
  int liveDelayMaxInt = 30;
  std::string liveDelayBytes = params.get("LiveDelay");
  if (!liveDelayBytes.empty()) {
    capnp::FlatArrayMessageReader msg(kj::ArrayPtr<const capnp::word>(
      reinterpret_cast<const capnp::word*>(liveDelayBytes.data()),
      liveDelayBytes.size() / sizeof(capnp::word)));
    auto event = msg.getRoot<cereal::Event>();
    if (event.hasLiveDelay()) {
      auto liveDelay = event.getLiveDelay();
      float lateralDelay = liveDelay.getLateralDelay();
      liveDelayMaxInt = static_cast<int>(lateralDelay * 100.0f) + 20;
    }
  }
  delay_control = new OptionControlSP("LagdToggleDelay", tr("Adjust Software Delay"),
                                     tr("Adjust the software delay when Live Learning Steer Delay is toggled off."
                                        "\\nThe default software delay value is 0.2"),
                                     "", {5, liveDelayMaxInt}, 1, false, nullptr, true, true);

  connect(delay_control, &OptionControlSP::updateLabels, [=]() {
    float value = QString::fromStdString(params.get("LagdToggleDelay")).toFloat();
    delay_control->setLabel(QString::number(value, 'f', 2) + "s");
  });
  connect(lagd_toggle_control, &ParamControlSP::toggleFlipped, [=](bool state) {
    delay_control->setVisible(!state);
  });
  delay_control->showDescription();
  list->addItem(delay_control);
}

void SteeringSettings::refreshLaneTurnValueControl() {
  if (!lane_turn_value_control) return;
  float stored_mph = QString::fromStdString(params.get("LaneTurnValue")).toFloat();
  bool is_metric = params.getBool("IsMetric");
  QString unit = is_metric ? "km/h" : "mph";
  float display_value = stored_mph;
  if (is_metric) {
    display_value = stored_mph * 1.609344f;
  }
  lane_turn_value_control->setLabel(QString::number(static_cast<int>(std::round(display_value))) + " " + unit);
  lane_turn_value_control->setVisible(params.getBool("LaneTurnDesire"));
}

void SteeringSettings::updateSettings() {
  // Update settings when needed
}
"""

    # 3. 创建转向学习功能实现文件
    steering_learning_py = """# Steering Learning Implementation
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
    \"\"\"Load parameters from storage\"\"\"
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
    \"\"\"Get lane turn speed limit in m/s\"\"\"
    # Convert from mph to m/s
    return self.lane_turn_value * 0.44704

  def get_effective_delay(self, measured_delay=None):
    \"\"\"Get effective steering delay based on learning settings\"\"\"
    if self.live_delay_enabled and measured_delay is not None:
      # Use live learned delay
      return measured_delay
    else:
      # Use software delay setting
      return self.software_delay

  def update_live_delay(self, lateral_delay):
    \"\"\"Update live learned delay\"\"\"
    if self.live_delay_enabled:
      # Save the learned delay
      self.params.put("LiveDelay", str(lateral_delay))
"""

    # 4. 创建转向学习辅助函数文件
    steering_helpers_py = """# Steering Learning Helper Functions
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
    \"\"\"Update delay estimate based on steering angle and desired angle\"\"\"
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
    \"\"\"Find delay in samples using cross-correlation\"\"\"
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
    \"\"\"Update lane turn desire learning\"\"\"
    self.steering_history.append(steering_angle)
    self.desired_angle_history.append(desired_angle)
    self.lane_width_history.append(lane_width)
    self.speed_history.append(v_ego)

    if len(self.steering_history) < self.window_size:
      return

    # Update factors based on driving data
    self._update_factors()

  def _update_factors(self):
    \"\"\"Update learning factors based on collected data\"\"\"
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
    \"\"\"Get enhanced desired angle based on learned factors\"\"\"
    # Apply learned factors
    speed_adjustment = 1.0 / (1.0 + self.speed_factor * (v_ego - 10.0))  # Normalize around 10 m/s
    lane_width_adjustment = 1.0 / (1.0 + self.lane_width_factor * (lane_width - 3.5))  # Normalize around 3.5m
    curvature_adjustment = 1.0 + self.curvature_factor * np.abs(desired_angle)  # Increase response for higher curvature

    enhanced_angle = desired_angle * speed_adjustment * lane_width_adjustment * curvature_adjustment

    return enhanced_angle
"""

    # 创建文件
    files_to_create = [
        ("selfdrive/ui/sunnypilot/qt/offroad/settings/steering_settings.h", steering_settings_h),
        ("selfdrive/ui/sunnypilot/qt/offroad/settings/steering_settings.cc", steering_settings_cc),
        ("sunnypilot/selfdrive/controls/lib/steering_learning.py", steering_learning_py),
        ("sunnypilot/selfdrive/controls/lib/steering_helpers.py", steering_helpers_py),
    ]

    for file_path, content in files_to_create:
        os.makedirs(os.path.dirname(file_path), exist_ok=True)
        with open(file_path, 'w') as f:
            f.write(content)
        print(f"已创建文件: {file_path}")

    print("tn相关的UI文件创建完成！")

def main():
    """主函数"""
    print("创建tn转向学习功能相关文件...")
    create_tn_ui_files()

if __name__ == "__main__":
    main()