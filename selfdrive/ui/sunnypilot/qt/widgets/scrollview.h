/**
 * Copyright (c) 2021-, Haibin Wen, sunnypilot, and a number of other contributors.
 *
 * This file is part of sunnypilot and is licensed under the MIT License.
 * See the LICENSE.md file in the root directory for more details.
 */

#pragma once

#include <QScrollArea>

class ScrollViewSP : public QScrollArea {
  Q_OBJECT

public:
  explicit ScrollViewSP(QWidget *content, QWidget *parent = nullptr);
};