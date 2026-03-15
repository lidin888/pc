#include "selfdrive/ui/sunnypilot/qt/offroad/settings/carrot_panel.h"

CarrotPanel::CarrotPanel(QWidget *parent) : QWidget(parent) {
  main_layout = new QStackedLayout(this);
  ListWidgetSP *list = new ListWidgetSP(this, false);

  mainScreen = new QWidget(this);
  QVBoxLayout *vlayout = new QVBoxLayout(mainScreen);
  vlayout->setContentsMargins(50, 20, 50, 20);

  // === 定速巡航来源 ===
  auto *panelSide = new ButtonParamControlSP(
    "CarrotPanelSide",
    QString::fromUtf8("导航面板位置"),
    QString::fromUtf8("选择CarrotMan导航面板显示位置:\n"
       "左侧: 面板显示在屏幕左下角\n"
       "右侧: 面板显示在屏幕右下角"),
    "",
    {QString::fromUtf8("左侧"), QString::fromUtf8("右侧")},
    160
  );
  list->addItem(panelSide);
  list->addItem(horizontal_line());

  // === 定速巡航来源 ===
  auto *speedFromPcm = new ButtonParamControlSP(
    "SpeedFromPCM",
    QString::fromUtf8("定速巡航来源"),
    QString::fromUtf8("选择巡航速度来源:\n"
       "CP: 使用CP计算的速度\n"
       "原车: 使用原车PCM速度（丰田推荐）\n"
       "CP30: 使用CP速度，最低30km/h\n"
       "混合: 停车用原车，其它用CP（本田推荐）"),
    "",
    {"CP", QString::fromUtf8("原车"), "CP30", QString::fromUtf8("混合")},
    160
  );
  list->addItem(speedFromPcm);
  list->addItem(horizontal_line());

  // === 导航限速模式 ===
  auto *naviSpeedMode = new ButtonParamControlSP(
    "AutoNaviSpeedCtrlMode",
    QString::fromUtf8("导航限速模式"),
    QString::fromUtf8("导航限速控制方式:\n"
       "关闭: 不启用\n"
       "减速: 减速到限速\n"
       "保持: 减速并保持\n"
       "自动: 自动调节"),
    "",
    {QString::fromUtf8("关闭"), QString::fromUtf8("减速"), QString::fromUtf8("保持"), QString::fromUtf8("自动")},
    160
  );
  list->addItem(naviSpeedMode);
  list->addItem(horizontal_line());

  // === 驾驶模式 ===
  auto *drivingMode = new ButtonParamControlSP(
    "MyDrivingMode",
    QString::fromUtf8("驾驶模式"),
    QString::fromUtf8("控制纵向跟车风格:\n"
       "节能: 省油驾驶\n"
       "安全: 安全优先\n"
       "标准: 常规驾驶\n"
       "运动: 运动驾驶"),
    "",
    {QString::fromUtf8("节能"), QString::fromUtf8("安全"), QString::fromUtf8("标准"), QString::fromUtf8("运动")},
    160
  );
  list->addItem(drivingMode);
  list->addItem(horizontal_line());

  // === 巡航节能偏移 ===
  auto *cruiseEco = new ButtonParamControlSP(
    "CruiseEcoControl",
    QString::fromUtf8("巡航节能偏移"),
    QString::fromUtf8("在限速基础上额外减少的速度偏移(km/h):\n"
       "0=关闭, 1~5 对应偏移值"),
    "",
    {QString::fromUtf8("关"), "1", "2", "3", "4", "5"},
    100
  );
  list->addItem(cruiseEco);
  list->addItem(horizontal_line());

  // === 红绿灯检测 ===
  auto *trafficMode = new ButtonParamControlSP(
    "TrafficLightDetectMode",
    QString::fromUtf8("红绿灯检测"),
    QString::fromUtf8("红绿灯检测模式:\n"
       "关闭: 不启用\n"
       "警告: 仅提醒\n"
       "制动: 自动刹车"),
    "",
    {QString::fromUtf8("关闭"), QString::fromUtf8("警告"), QString::fromUtf8("制动")},
    160
  );
  list->addItem(trafficMode);
  list->addItem(horizontal_line());

  // === 自动转弯控制 ===
  auto *autoTurn = new ButtonParamControlSP(
    "AutoTurnControl",
    QString::fromUtf8("自动转弯控制"),
    QString::fromUtf8("导航引导时自动转弯:\n"
       "关闭: 不启用\n"
       "减速: 仅减速\n"
       "转向: 自动转向"),
    "",
    {QString::fromUtf8("关闭"), QString::fromUtf8("减速"), QString::fromUtf8("转向")},
    160
  );
  list->addItem(autoTurn);
  list->addItem(horizontal_line());

  // === 跟车时间距离 - 激进档 ===
  tFollowGap1 = new OptionControlSP(
    "TFollowGap1",
    QString::fromUtf8("跟车距离-激进"),
    QString::fromUtf8("激进模式下的跟车时间间距。\n"
       "默认110 = 1.1秒，数值越小跟车越近。"),
    "",
    {80, 200},
    10
  );
  connect(tFollowGap1, &OptionControlSP::updateLabels, [=]() {
    tFollowGap1->setLabel(QString::fromStdString(params.get("TFollowGap1")));
  });
  tFollowGap1->setLabel(QString::fromStdString(params.get("TFollowGap1")));
  list->addItem(tFollowGap1);
  list->addItem(horizontal_line());

  // === 跟车时间距离 - 标准档 ===
  tFollowGap2 = new OptionControlSP(
    "TFollowGap2",
    QString::fromUtf8("跟车距离-标准"),
    QString::fromUtf8("标准模式下的跟车时间间距。\n"
       "默认130 = 1.3秒。"),
    "",
    {80, 250},
    10
  );
  connect(tFollowGap2, &OptionControlSP::updateLabels, [=]() {
    tFollowGap2->setLabel(QString::fromStdString(params.get("TFollowGap2")));
  });
  tFollowGap2->setLabel(QString::fromStdString(params.get("TFollowGap2")));
  list->addItem(tFollowGap2);
  list->addItem(horizontal_line());

  // === 跟车时间距离 - 舒适档 ===
  tFollowGap3 = new OptionControlSP(
    "TFollowGap3",
    QString::fromUtf8("跟车距离-舒适"),
    QString::fromUtf8("舒适模式下的跟车时间间距。\n"
       "默认145 = 1.45秒。"),
    "",
    {100, 300},
    10
  );
  connect(tFollowGap3, &OptionControlSP::updateLabels, [=]() {
    tFollowGap3->setLabel(QString::fromStdString(params.get("TFollowGap3")));
  });
  tFollowGap3->setLabel(QString::fromStdString(params.get("TFollowGap3")));
  list->addItem(tFollowGap3);
  list->addItem(horizontal_line());

  // === 跟车时间距离 - 宽松档 ===
  tFollowGap4 = new OptionControlSP(
    "TFollowGap4",
    QString::fromUtf8("跟车距离-宽松"),
    QString::fromUtf8("宽松模式下的跟车时间间距。\n"
       "默认160 = 1.6秒，数值越大跟车越远。"),
    "",
    {100, 350},
    10
  );
  connect(tFollowGap4, &OptionControlSP::updateLabels, [=]() {
    tFollowGap4->setLabel(QString::fromStdString(params.get("TFollowGap4")));
  });
  tFollowGap4->setLabel(QString::fromStdString(params.get("TFollowGap4")));
  list->addItem(tFollowGap4);
  list->addItem(horizontal_line());

  // === 动态跟车系数 ===
  dynamicTFollowCtrl = new OptionControlSP(
    "DynamicTFollow",
    QString::fromUtf8("动态跟车系数"),
    QString::fromUtf8("根据前车距离动态调整跟车距离。\n"
       "0=关闭，100=标准（1.0x），数值越大调整幅度越大。"),
    "",
    {0, 200},
    10
  );
  connect(dynamicTFollowCtrl, &OptionControlSP::updateLabels, [=]() {
    dynamicTFollowCtrl->setLabel(QString::fromStdString(params.get("DynamicTFollow")));
  });
  dynamicTFollowCtrl->setLabel(QString::fromStdString(params.get("DynamicTFollow")));
  list->addItem(dynamicTFollowCtrl);
  list->addItem(horizontal_line());

  // === 停车距离 ===
  stopDistanceCtrl = new OptionControlSP(
    "StopDistanceCarrot",
    QString::fromUtf8("停车距离 (厘米)"),
    QString::fromUtf8("与前车停车时保持的距离。\n"
       "默认550 = 5.5米。"),
    "",
    {350, 750},
    50
  );
  connect(stopDistanceCtrl, &OptionControlSP::updateLabels, [=]() {
    stopDistanceCtrl->setLabel(QString::fromStdString(params.get("StopDistanceCarrot")));
  });
  stopDistanceCtrl->setLabel(QString::fromStdString(params.get("StopDistanceCarrot")));
  list->addItem(stopDistanceCtrl);

  mainScroller = new ScrollViewSP(list, this);
  vlayout->addWidget(mainScroller);
  main_layout->addWidget(mainScreen);
}

void CarrotPanel::showEvent(QShowEvent *event) {
  QWidget::showEvent(event);
}
