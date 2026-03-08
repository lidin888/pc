/**
 * Copyright (c) 2021-, Haibin Wen, sunnypilot, and a number of other contributors.
 *
 * This file is part of sunnypilot and is licensed under the MIT License.
 * See the LICENSE.md file in the root directory for more details.
 */

#include "selfdrive/ui/sunnypilot/qt/widgets/scrollview.h"

ScrollViewSP::ScrollViewSP(QWidget *content, QWidget *parent) : QScrollArea(parent) {
  setWidget(content);
  setWidgetResizable(true);
}