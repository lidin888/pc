/**
 * Copyright (c) 2021-, Haibin Wen, sunnypilot, and a number of other contributors.
 *
 * This file is part of sunnypilot and is licensed under the MIT License.
 * See the LICENSE.md file in the root directory for more details.
 */

#pragma once

#include <QWidget>
#include "common/params.h"

#include "selfdrive/ui/qt/util.h"
#include "selfdrive/ui/sunnypilot/qt/ui.h"
#include "selfdrive/ui/sunnypilot/qt/offroad/settings/settings.h"
#include "selfdrive/ui/qt/widgets/controls.h"

// 前向声明CValueControl
class CValueControl;

class SteeringSettings : public QWidget {
  Q_OBJECT

public:
  explicit SteeringSettings(QWidget *parent = nullptr);
  void updateSettings();
  void refreshLaneTurnValueControl();

private:
  bool offroad = false;
  Params params;

  // UI controls
  CValueControl *lane_turn_value_control;
  ParamControl *delay_control;
};