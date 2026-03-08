/**
 * Copyright (c) 2021-, Haibin Wen, sunnypilot, and a number of other contributors.
 *
 * This file is part of sunnypilot and is licensed under the MIT License.
 * See the LICENSE.md file in the root directory for more details.
 */

#include "spp_settings_panel.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QMessageBox>

#include "common/params.h"
#include "common/util.h"
#include "selfdrive/ui/qt/widgets/scrollview.h"
#include "selfdrive/ui/qt/offroad/settings.h"
#include "selfdrive/ui/qt/widgets/controls.h"

SPPSettingsPanel::SPPSettingsPanel(QWidget *parent) : QWidget(parent) {
  QVBoxLayout *main_layout = new QVBoxLayout(this);
  main_layout->setContentsMargins(50, 20, 50, 20);

  ListWidget *list = new ListWidget(this);
  ScrollView *scroller = new ScrollView(list, this);
  main_layout->addWidget(scroller);

  // 标准控件不需要文件监控

  // Steering Learning Title
  LabelControl *steering_title = new LabelControl(tr("Steering Learning Settings"), tr("Adjust steering response and learning parameters"));
  list->addItem(steering_title);

  // Lane Turn Desire toggle
  lane_turn_desire_toggle = new ParamControl("LaneTurnDesire", tr("Use Lane Turn Desire"),
    tr("Enable lane turn desire learning to improve steering response"),
    "../assets/offroad/icon_shell.png");
  list->addItem(lane_turn_desire_toggle);

  // Lane Turn Value control
  bool is_metric_initial = Params().getBool("IsMetric");
  int max_value_mph = 20;
  const float K = 1.609344f;
  int per_value_change_scaled = is_metric_initial ? static_cast<int>(std::round((1.0f / K) * 100.0f)) : 100;
  std::pair<int, int> range = {5 * 100, max_value_mph * 100};

  float current_value = QString::fromStdString(Params().get("LaneTurnValue")).toFloat();
  current_value = std::max(5.0f, std::min(current_value, 20.0f));

  lane_turn_value_control = new CValueControl("LaneTurnValue", tr("Adjust Steering Speed"),
    tr("Set the maximum speed for lane turn desire. Default is 19 %1.").arg(is_metric_initial ? "km/h" : "mph"),
    range.first, range.second, per_value_change_scaled);

  Params().put("LaneTurnValue", QString::number(current_value).toStdString());
  lane_turn_value_control->showDescription();
  list->addItem(lane_turn_value_control);

  refreshLaneTurnValueControl();
  connect(lane_turn_desire_toggle, &ParamControl::toggleFlipped, this, &SPPSettingsPanel::refreshLaneTurnValueControl);

  // Monitor unit changes
  QTimer *unit_timer = new QTimer(this);
  connect(unit_timer, &QTimer::timeout, [this]() {
    static bool last_is_metric = Params().getBool("IsMetric");
    bool current_is_metric = Params().getBool("IsMetric");
    if (last_is_metric != current_is_metric) {
      refreshLaneTurnValueControl();
      last_is_metric = current_is_metric;
    }
  });
  unit_timer->start(1000);

  // ---------------------------------------------------------
  // 1. Live Learning Steer Delay (Toggle)
  // ---------------------------------------------------------
  lagd_toggle_control = new ParamControl("LagdToggle", tr("Live Learning Steer Delay"),
    tr("Enable: System automatically calculates delay (hides manual settings below).\nDisable: Need to manually set software delay below."),
    "../assets/offroad/icon_shell.png");
  list->addItem(lagd_toggle_control);

  // ---------------------------------------------------------
  // 2. 调整软件延迟 (滑块 - 仅在关闭实时学习时显示)
  // ---------------------------------------------------------
  int min_val = 1;   // 0.01s
  int max_val = 50;  // 0.50s

  std::string delayStr = Params().get("LagdToggleDelay");
  float current_delay = 0.2f;
  if (!delayStr.empty()) {
    try {
      current_delay = std::stof(delayStr);
    } catch (...) {
      current_delay = 0.2f;
    }
    current_delay = std::max(0.01f, std::min(current_delay, 0.5f));
  }

  Params().put("LagdToggleDelay", std::to_string(current_delay));

  delay_control = new CValueControl("LagdToggleDelay", tr("Adjust Software Delay"),
                                     tr("Adjust the software delay when Live Learning Steer Delay is toggled off. Default software delay value is 0.2"),
                                     min_val, max_val, 1);

  list->addItem(delay_control);

  // 连接开关信号（使用 toggleFlipped 确保在参数保存后触发）
  connect(lagd_toggle_control, &ParamControl::toggleFlipped, this, &SPPSettingsPanel::updateSettings);

  // ---------------------------------------------------------
  // 3. Delay Description (显示总延迟计算)
  // ---------------------------------------------------------
  delay_info = new LabelControl(tr("Delay Calculation"), "");
  delay_info->showDescription();
  list->addItem(delay_info);

  // 连接开关信号（使用 toggleFlipped 确保在参数保存后触发）
  connect(lagd_toggle_control, &ParamControl::toggleFlipped, this, &SPPSettingsPanel::updateSettings);

  // 初始化界面状态
  updateSettings();

  // Toyota Specific Title
  LabelControl *toyota_title = new LabelControl(tr("Toyota Specific Settings"), tr("Toyota vehicle specific adjustment options"));
  list->addItem(toyota_title);

  toyota_drive_mode = new ParamControl("ToyotaDriveMode", tr("Toyota: Drive Mode Button Link"),
    tr("Link the vehicle's drive mode button with acceleration personality (Easy, Standard, Sport) for a seamless driving experience"),
    "../assets/offroad/icon_shell.png");
  list->addItem(toyota_drive_mode);

  toyota_auto_hold = new ParamControl("ToyotaAutoHold", tr("Toyota: Auto Brake Hold (TSS2 Hybrid)"),
    tr("Automatically hold the vehicle when it comes to a stop. This feature is designed for TSS2 hybrid vehicles"),
    "../assets/offroad/icon_shell.png");
  list->addItem(toyota_auto_hold);

  toyota_enhanced_bsm = new ParamControl("ToyotaEnhancedBsm", tr("Toyota: Enhanced BSM Support"),
    tr("Add enhanced blind spot monitoring support for Toyota vehicles, especially Prius TSS2 and some TSS-P models"),
    "../assets/offroad/icon_shell.png");
  list->addItem(toyota_enhanced_bsm);

  toyota_tss2_long = new ParamControl("ToyotaTSS2Long", tr("Toyota: TSS2 Longitudinal Control"),
    tr("Enable optimized longitudinal control for Toyota TSS2 vehicles"),
    "../assets/offroad/icon_shell.png");
  list->addItem(toyota_tss2_long);

  toyota_stock_long = new ParamControl("ToyotaStockLongitudinal", tr("Toyota: Stock Longitudinal Control"),
    tr("Use Toyota stock longitudinal control parameters"),
    "../assets/offroad/icon_shell.png");
  list->addItem(toyota_stock_long);

  // End to End Title
  LabelControl *end_to_end_title = new LabelControl(tr("End to End Settings"), tr("End to end control related options"));
  list->addItem(end_to_end_title);

  end_to_end_toggle = new ParamControl("EndToEndToggle", tr("Enable End to End Control"),
    tr("Enable end-to-end neural network control for a more natural steering experience"),
    "../assets/offroad/icon_shell.png");
  list->addItem(end_to_end_toggle);

  end_to_end_force_lane_change = new ParamControl("EndToEndForceLaneChange", tr("End to End Force Lane Change"),
    tr("Force lane change control in end-to-end mode"),
    "../assets/offroad/icon_shell.png");
  list->addItem(end_to_end_force_lane_change);
}

// 标准控件不需要文件监控，直接移除这个函数

void SPPSettingsPanel::updateSettings() {
  // 获取实时学习状态：LagdToggle = true 表示实时学习开启（自动模式）
  bool liveLearningEnabled = Params().getBool("LagdToggle");

  // 手动模式 = 实时学习关闭
  bool manualMode = !liveLearningEnabled;

  // 设置滑块和延迟信息的可见性
  delay_control->setVisible(manualMode);
  delay_info->setVisible(manualMode);

  // 获取当前软件延迟值
  std::string delayStr = Params().get("LagdToggleDelay");
  float softwareDelay = 0.2f;
  if (!delayStr.empty()) {
    try {
      softwareDelay = std::stof(delayStr);
    } catch (...) {
      softwareDelay = 0.2f;
    }
    softwareDelay = std::max(0.01f, std::min(softwareDelay, 0.5f));
  }

  // 更新开关的描述
  QString desc;
  if (liveLearningEnabled) {
    desc = tr("<b>Live Learning Enabled</b><br>System is automatically calculating optimal steering delay.");
  } else {
    desc = tr("<b>Manual Setting Mode</b><br>Drag the slider above to adjust software delay.");
  }
  lagd_toggle_control->setDescription(desc);

  // 更新延迟计算信息（仅当手动模式时显示详细计算）
  if (manualMode) {
    float hardwareDelay = 0.1f;
    float totalLag = hardwareDelay + softwareDelay;

    QString details = tr("Total delay = Hardware actuator delay (0.1s) + Software delay<br><br>") +
                      tr("<span style=\"font-size:14px; color:#e0e0e0\">") +
                      tr("Hardware Delay: ") + QString::number(hardwareDelay, 'f', 2) + "s<br>" +
                      tr("Software Delay: ") + QString::number(softwareDelay, 'f', 2) + "s<br>" +
                      tr("-----------------------------<br>") +
                      tr("<b>Total Delay: <font color=\"#4fc3f7\">") + QString::number(totalLag, 'f', 2) + "s</font></b>" +
                      tr("</span>");
    delay_info->setText(details);
  } else {
    delay_info->setText("");  // 实时学习时清空文本
  }

  // 强制刷新布局
  if (delay_info->parentWidget()) {
    delay_info->parentWidget()->adjustSize();
  }
}

void SPPSettingsPanel::refreshLaneTurnValueControl() {
  if (!lane_turn_value_control) return;

  float stored_value = QString::fromStdString(Params().get("LaneTurnValue")).toFloat();
  bool is_metric = Params().getBool("IsMetric");
  QString unit = is_metric ? "km/h" : "mph";
  float display_value = stored_value;

  if (is_metric) {
    display_value = stored_value * 1.609344f;
  }

  display_value = std::max(5.0f, std::min(display_value, 32.0f));

  // CValueControl会自动显示值，我们只需要更新参数值
  // 可见性依赖 LaneTurnDesire 开关
  bool desireEnabled = Params().getBool("LaneTurnDesire");
  lane_turn_value_control->setVisible(desireEnabled);
}