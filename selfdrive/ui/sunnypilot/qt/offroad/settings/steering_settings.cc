/**
 * Copyright (c) 2021-, Haibin Wen, sunnypilot, and a number of other contributors.
 *
 * This file is part of sunnypilot and is licensed under the MIT License.
 * See the LICENSE.md file in the root directory for more details.
 */

#include "steering_settings.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QSlider>
#include <QPushButton>
#include <QMessageBox>

#include "common/params.h"
#include "common/util.h"
#include "selfdrive/ui/qt/widgets/scrollview.h"
#include "selfdrive/ui/qt/offroad/settings.h"

SteeringSettings::SteeringSettings(QWidget *parent) : QWidget(parent) {
  QVBoxLayout *main_layout = new QVBoxLayout(this);
  main_layout->setContentsMargins(50, 20, 50, 20);

  ListWidget *list = new ListWidget(this);
  ScrollView *scroller = new ScrollView(list, this);
  main_layout->addWidget(scroller);

  // 转向学习标题
  LabelControl *steering_title = new LabelControl(tr("转向学习设置"), tr("调整转向响应和学习参数"));
  list->addItem(steering_title);

  // Lane Turn Value control
  bool is_metric_initial = params.getBool("IsMetric");
  lane_turn_value_control = new CValueControl("LaneTurnValue", tr("调整转向速度"),
    tr("设置车道转向意图的最大速度。默认值为19 %1。").arg(is_metric_initial ? "km/h" : "mph"),
    0, 30, 1);
  list->addItem(lane_turn_value_control);

  // 软件延迟控制
  delay_control = new ParamControl("LagdToggleDelay", tr("调整软件延迟"),
                                   tr("当关闭实时学习转向延迟时调整软件延迟。"
                                      "\n默认软件延迟值为0.2秒"),
                                   "../assets/offroad/icon_road.png");

  // Update delay control label when value changes
  delay_control->refresh();

  // 初始状态设置
  bool lagd_enabled = params.getBool("LagdToggle");
  delay_control->setVisible(!lagd_enabled);
  delay_control->setEnabled(!lagd_enabled);

  delay_control->showDescription();
  list->addItem(delay_control);
}

void SteeringSettings::refreshLaneTurnValueControl() {
  if (!lane_turn_value_control) return;

  // 更新可见性
  lane_turn_value_control->setVisible(params.getBool("LaneTurnDesire"));
}

void SteeringSettings::updateSettings() {
  // Update settings when needed
}