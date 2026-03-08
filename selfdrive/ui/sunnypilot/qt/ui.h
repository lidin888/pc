/**
 * Copyright (c) 2021-, Haibin Wen, sunnypilot, and a number of other contributors.
 *
 * This file is part of sunnypilot and is licensed under the MIT License.
 * See the LICENSE.md file in the root directory for more details.
 */

#pragma once

#include <QApplication>
#include <QMainWindow>
#include <QTimer>

class UIStateSP : public QObject {
  Q_OBJECT

public:
  static UIStateSP *uiStateSP();
  static void initSP();

  void update();

signals:
  void uiUpdate();

private:
  UIStateSP();
  static UIStateSP *instance;

  QTimer *timer;
};