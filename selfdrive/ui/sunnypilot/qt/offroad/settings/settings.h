/**
 * Copyright (c) 2021-, Haibin Wen, sunnypilot, and a number of other contributors.
 *
 * This file is part of sunnypilot and is licensed under the MIT License.
 * See the LICENSE.md file in the root directory for more details.
 */

#pragma once

#include <QMainWindow>
#include <QStackedWidget>

class SettingsWindowSP : public QMainWindow {
  Q_OBJECT

public:
  explicit SettingsWindowSP(QWidget *parent = nullptr);

private:
  QStackedWidget *content;
};