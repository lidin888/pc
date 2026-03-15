#pragma once

#include "selfdrive/ui/sunnypilot/qt/offroad/settings/settings.h"
#include "selfdrive/ui/sunnypilot/qt/widgets/scrollview.h"

class CarrotPanel : public QWidget {
  Q_OBJECT

public:
  explicit CarrotPanel(QWidget *parent = nullptr);
  void showEvent(QShowEvent *event) override;

private:
  Params params;
  QStackedLayout *main_layout = nullptr;
  QWidget *mainScreen = nullptr;
  ScrollViewSP *mainScroller = nullptr;
  OptionControlSP *tFollowGap1 = nullptr;
  OptionControlSP *tFollowGap2 = nullptr;
  OptionControlSP *tFollowGap3 = nullptr;
  OptionControlSP *tFollowGap4 = nullptr;
  OptionControlSP *dynamicTFollowCtrl = nullptr;
  OptionControlSP *stopDistanceCtrl = nullptr;
};
