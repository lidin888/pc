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
#include "selfdrive/ui/sunnypilot/qt/widgets/controls.h"
#include "selfdrive/ui/qt/offroad/settings.h"

// Forward declaration
class CValueControl;
class OptionControlSP;

class SPPSettingsPanel : public QWidget {
  Q_OBJECT

public:
  explicit SPPSettingsPanel(QWidget *parent = nullptr);
  void updateSettings();
  void refreshLaneTurnValueControl();

private:
  bool offroad = false;

  // 初始化默认参数
  void initDefaultParams();

  // UI controls
  ParamControl *lane_turn_desire_toggle;
  CValueControl *lane_turn_value_control;
  ParamControl *lagd_toggle_control;
  CValueControl *delay_control;
  LabelControl *delay_info;

  // Toyota controls
  ParamControl *toyota_drive_mode;
  ParamControl *toyota_auto_hold;
  ParamControl *toyota_enhanced_bsm;
  ParamControl *toyota_tss2_long;
  ParamControl *toyota_stock_long;

  // End to End controls
  ParamControl *end_to_end_toggle;
  ParamControl *end_to_end_force_lane_change;
};