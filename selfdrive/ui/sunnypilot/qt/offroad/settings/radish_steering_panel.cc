/**
 * Copyright (c) 2021-, Haibin Wen, sunnypilot, and a number of other contributors.
 *
 * This file is part of sunnypilot and is licensed under the MIT License.
 * See the LICENSE.md file in the root directory for more details.
 */

#include <QVBoxLayout>

#include "selfdrive/ui/sunnypilot/qt/offroad/settings/radish_steering_panel.h"
#include "selfdrive/ui/qt/widgets/scrollview.h"

RadishSteeringPanel::RadishSteeringPanel(QWidget *parent) : QWidget(parent) {
  QVBoxLayout *main_layout = new QVBoxLayout(this);
  main_layout->setContentsMargins(0, 0, 0, 0);

  // 直接添加转向学习设置（包含丰田特调）
  steering_settings = new SteeringSettings(this);
  main_layout->addWidget(steering_settings);
}

void RadishSteeringPanel::updateSettings() {
  if (steering_settings) {
    steering_settings->updateSettings();
  }
}