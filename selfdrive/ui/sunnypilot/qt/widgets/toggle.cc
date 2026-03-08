/**
 * Copyright (c) 2021-, Haibin Wen, sunnypilot, and a number of other contributors.
 *
 * This file is part of sunnypilot and is licensed under the MIT License.
 * See the LICENSE.md file in the root directory for more details.
 */

#include "selfdrive/ui/sunnypilot/qt/widgets/toggle.h"

#include <QMouseEvent>
#include <QPainter>

ToggleSP::ToggleSP(QWidget *parent) : QAbstractButton(parent) {
  setCheckable(true);
  setChecked(false);
  on = false;
  setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  setFixedSize(120, 60);
  setStyleSheet(R"(
    QAbstractButton {
      background-color: #393939;
      border-radius: 30px;
    }
    QAbstractButton:checked {
      background-color: #4E82F6;
    }
  )");
}

void ToggleSP::setOnState(bool state) {
  if (on != state) {
    on = state;
    setChecked(state);  // 更新 Qt 状态，触发 checkStateSet 但不会发出信号
  }
}

void ToggleSP::togglePosition() {
  on = !on;
  setChecked(on);
  emit stateChanged(on);
}

void ToggleSP::paintEvent(QPaintEvent *e) {
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);

  // 背景色基于 checked 状态（此时已与 on 同步）
  QColor bgColor = isChecked() ? QColor(78, 130, 246) : QColor(57, 57, 57);
  painter.setBrush(bgColor);
  painter.setPen(Qt::NoPen);
  painter.drawRoundedRect(rect(), 30, 30);

  // 手柄
  QColor handleColor = QColor(255, 255, 255);
  painter.setBrush(handleColor);
  int handleWidth = 50;
  int handleHeight = 50;
  int handleX = isChecked() ? width() - handleWidth - 5 : 5;
  int handleY = (height() - handleHeight) / 2;
  QRect handleRect(handleX, handleY, handleWidth, handleHeight);
  painter.drawEllipse(handleRect);
}

void ToggleSP::mouseReleaseEvent(QMouseEvent *e) {
  if (hitButton(e->pos())) {
    togglePosition();  // 切换状态并发出信号
  }
  QAbstractButton::mouseReleaseEvent(e);
}

void ToggleSP::checkStateSet() {
  // 当 setChecked 被调用时，同步 on 变量
  on = isChecked();
}

bool ToggleSP::hitButton(const QPoint &pos) const {
  return rect().contains(pos);
}