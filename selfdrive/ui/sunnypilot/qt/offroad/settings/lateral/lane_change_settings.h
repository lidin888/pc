/**
 * Copyright (c) 2021-, Haibin Wen, sunnypilot, and a number of other contributors.
 *
 * This file is part of sunnypilot and is licensed under the MIT License.
 * See the LICENSE.md file in the root directory for more details.
 */

#pragma once

#include <QWidget>

#include "selfdrive/ui/qt/offroad/settings.h"
#include "selfdrive/ui/sunnypilot/qt/widgets/controls.h"

class LaneChangeSettings : public QWidget {
  Q_OBJECT

public:
  explicit LaneChangeSettings(QWidget *parent = nullptr);

signals:
  void backPress();

private:
  QPushButton *back_btn;
};
