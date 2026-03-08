/**
 * Copyright (c) 2021-, Haibin Wen, sunnypilot, and a number of other contributors.
 *
 * This file is part of sunnypilot and is licensed under the MIT License.
 * See the LICENSE.md file in the root directory for more details.
 */

#include <QVBoxLayout>

#include "selfdrive/ui/sunnypilot/qt/offroad/settings/vehicle/toyota_settings.h"
#include "selfdrive/ui/qt/widgets/scrollview.h"
#include "selfdrive/ui/qt/widgets/controls.h"

ToyotaSettings::ToyotaSettings(QWidget *parent) : QWidget(parent) {
  QVBoxLayout *main_layout = new QVBoxLayout(this);
  main_layout->setContentsMargins(50, 20, 50, 20);

  ListWidget *list = new ListWidget(this);
  ScrollView *scroller = new ScrollView(list, this);
  main_layout->addWidget(scroller);

  // Toyota Drive Mode
  ParamControl *toyota_drive_mode = new ParamControl("ToyotaDriveMode",
    tr("Toyota: Drive Mode Button Link"),
    tr("Links car's drive mode button with acceleration personalities (Relaxed, Standard, Sport) for seamless driving experience."),
    "../assets/offroad/icon_shell.png", this);
  list->addItem(toyota_drive_mode);

  // Toyota Auto Hold
  ParamControl *toyota_auto_hold = new ParamControl("ToyotaAutoHold",
    tr("Toyota: Auto Brake Hold (TSS2 Hybrid)"),
    tr("Automatically hold the vehicle at a stop when the lead car is stopped. This feature is specifically designed for TSS2 Hybrid vehicles."),
    "../assets/offroad/icon_shell.png", this);
  list->addItem(toyota_auto_hold);

  // Toyota Enhanced BSM
  ParamControl *toyota_enhanced_bsm = new ParamControl("ToyotaEnhancedBsm",
    tr("Toyota: Enhanced BSM Support"),
    tr("Add enhanced Blind Spot Monitoring support for Toyota vehicles, particularly for Prius TSS2 and some TSS-P models."),
    "../assets/offroad/icon_shell.png", this);
  list->addItem(toyota_enhanced_bsm);

  // Toyota TSS2 Longitudinal
  ParamControl *toyota_tss2_long = new ParamControl("ToyotaTSS2Long",
    tr("Toyota: TSS2 Custom Tune"),
    tr("Enable custom longitudinal tuning for Toyota TSS2 vehicles. This provides optimized acceleration and braking behavior for better driving experience."),
    "../assets/offroad/icon_shell.png", this);
  list->addItem(toyota_tss2_long);

  // Toyota Stock Longitudinal
  ParamControl *toyota_stock_long = new ParamControl("ToyotaStockLongitudinal",
    tr("Toyota: Stock Toyota Longitudinal"),
    tr("Use the stock Toyota longitudinal control instead of sunnypilot longitudinal control for a more factory-like driving experience."),
    "../assets/offroad/icon_shell.png", this);
  list->addItem(toyota_stock_long);
}

void ToyotaSettings::updateSettings() {
  // Update settings when needed
}