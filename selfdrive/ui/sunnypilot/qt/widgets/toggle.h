/**
 * Copyright (c) 2021-, Haibin Wen, sunnypilot, and a number of other contributors.
 *
 * This file is part of sunnypilot and is licensed under the MIT License.
 * See the LICENSE.md file in the root directory for more details.
 */

#pragma once

#include <QAbstractButton>

class ToggleSP : public QAbstractButton {
  Q_OBJECT

public:
  explicit ToggleSP(QWidget *parent = nullptr);
  void togglePosition();
  void setOnState(bool state);  // 设置状态（不发出信号）
  bool on;                      // 保持公开，但内部与 checked 同步

signals:
  void stateChanged(bool state);

protected:
  void paintEvent(QPaintEvent *e) override;
  void mouseReleaseEvent(QMouseEvent *e) override;
  void checkStateSet() override;
  bool hitButton(const QPoint &pos) const override;
};