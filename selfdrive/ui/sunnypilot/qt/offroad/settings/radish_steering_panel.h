/**
 * Copyright (c) 2021-, Haibin Wen, sunnypilot, and a number of other contributors.
 *
 * This file is part of sunnypilot and is licensed under the MIT License.
 * See the LICENSE.md file in the root directory for more details.
 */

#pragma once

#include <QWidget>

#include "selfdrive/ui/sunnypilot/qt/offroad/settings/steering_settings.h"

class RadishSteeringPanel : public QWidget {
  Q_OBJECT

public:
  explicit RadishSteeringPanel(QWidget *parent = nullptr);
  void updateSettings();

private:
  SteeringSettings *steering_settings;
};