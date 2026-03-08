/**
 * Copyright (c) 2021-, Haibin Wen, sunnypilot, and a number of other contributors.
 *
 * This file is part of sunnypilot and is licensed under the MIT License.
 * See the LICENSE.md file in the root directory for more details.
 */

#include "selfdrive/ui/sunnypilot/qt/offroad/settings/lateral/lane_change_settings.h"

LaneChangeSettings::LaneChangeSettings(QWidget *parent) : QWidget(parent) {
  QVBoxLayout *main_layout = new QVBoxLayout(this);
  main_layout->setMargin(20);

  // Back button
  back_btn = new QPushButton(tr("Back"));
  back_btn->setStyleSheet(R"(
    QPushButton {
      font-size: 50px;
      font-weight: 400;
      border-radius: 30px;
      background-color: #393939;
      padding: 20px;
    }
    QPushButton:pressed {
      background-color: #4a4a4a;
    }
  )");
  main_layout->addWidget(back_btn);
  connect(back_btn, &QPushButton::clicked, [=]() { emit backPress(); });
}
