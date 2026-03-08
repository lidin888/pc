/**
 * Copyright (c) 2021-, Haibin Wen, sunnypilot, and a number of other contributors.
 *
 * This file is part of sunnypilot and is licensed under the MIT License.
 * See the LICENSE.md file in the root directory for more details.
 */

#include "selfdrive/ui/sunnypilot/qt/widgets/controls.h"

#include <QPainter>
#include <QStyleOption>
#include <QTimer>
#include <fstream>

#include "common/params.h"

QFrame *horizontal_line(QWidget *parent) {
  QFrame *line = new QFrame(parent);
  line->setFrameShape(QFrame::StyledPanel);
  line->setStyleSheet(R"(
    border-width: 2px;
    border-bottom-style: solid;
    border-color: gray;
  )");
  line->setFixedHeight(10);
  return line;
}

QFrame *vertical_space(int height, QWidget *parent) {
  QFrame *v_space = new QFrame(parent);
  v_space->setFrameShape(QFrame::StyledPanel);
  v_space->setFixedHeight(height);
  return v_space;
}

// AbstractControlSP
std::vector<AbstractControlSP*> AbstractControlSP::advancedControls;
AbstractControlSP::~AbstractControlSP() { UnregisterAdvancedControl(this); }

void AbstractControlSP::RegisterAdvancedControl(AbstractControlSP *ctrl) { advancedControls.push_back(ctrl); }

void AbstractControlSP::UnregisterAdvancedControl(AbstractControlSP *ctrl) {
  advancedControls.erase(std::remove(advancedControls.begin(), advancedControls.end(), ctrl), advancedControls.end());
}

void AbstractControlSP::UpdateAllAdvancedControls() {
  bool visibility = Params().getBool("ShowAdvancedControls");
  advancedControls.erase(std::remove(advancedControls.begin(), advancedControls.end(), nullptr), advancedControls.end());
  for (auto *ctrl : advancedControls) ctrl->setVisible(visibility);
}

AbstractControlSP::AbstractControlSP(const QString &title, const QString &desc, const QString &icon, QWidget *parent, bool advancedControl)
    : AbstractControl(title, desc, icon, parent), isAdvancedControl(advancedControl) {
  if (isAdvancedControl) RegisterAdvancedControl(this);
}

void AbstractControlSP::hideEvent(QHideEvent *e) {
  // Since description is private in AbstractControl, we'll use the base class hideEvent
  AbstractControl::hideEvent(e);
}

// ElidedLabelSP
ElidedLabelSP::ElidedLabelSP(QWidget *parent) : QLabel(parent) {
  setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
}

ElidedLabelSP::ElidedLabelSP(const QString &text, QWidget *parent) : QLabel(text, parent) {
  setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
}

void ElidedLabelSP::paintEvent(QPaintEvent *event) {
  QPainter painter(this);
  QFontMetrics fontMetrics(font());
  int flags = Qt::TextSingleLine | Qt::AlignLeft | Qt::AlignVCenter;
  QString elidedText = fontMetrics.elidedText(text(), Qt::ElideRight, width());
  painter.drawText(rect(), flags, elidedText);
}

void ElidedLabelSP::resizeEvent(QResizeEvent *event) {
  QLabel::resizeEvent(event);
  update();
}

// ButtonControlSP
ButtonControlSP::ButtonControlSP(const QString &title, const QString &text, const QString &desc, const QString &icon, QWidget *parent)
    : AbstractControlSP(title, desc, icon, parent) {
  QPushButton *btn = new QPushButton(text);
  btn->setStyleSheet(R"(
    QPushButton {
      padding: 0;
      border-radius: 50px;
      font-size: 35px;
      font-weight: 500;
      color: #E4E4E4;
      background-color: #393939;
    }
    QPushButton:pressed {
      background-color: #4E4E4E;
    }
  )");
  hlayout->addWidget(btn);

  connect(btn, &QPushButton::clicked, this, &ButtonControlSP::clicked);
}

ButtonControlSP::ButtonControlSP(const QString &title, const QString &text, const QString &desc, const QString &icon, std::function<void()> onClick, QWidget *parent)
    : ButtonControlSP(title, text, desc, icon, parent) {
  connect(this, &ButtonControlSP::clicked, onClick);
}

void ButtonControlSP::showEvent(QShowEvent *event) {
  AbstractControlSP::showEvent(event);
}

// ButtonParamControlSP
ButtonParamControlSP::ButtonParamControlSP(const QString &param, const QString &title, const QString &desc, const QString &icon, QWidget *parent)
    : ButtonControlSP(title, "", desc, icon, parent), param(param) {
  connect(this, &ButtonControlSP::clicked, [param]() {
    Params params;
    params.put(param.toStdString(), "1");
  });
}

// ParamControlSP
ParamControlSP::ParamControlSP(const QString &param, const QString &title, const QString &desc, const QString &icon, QWidget *parent)
    : AbstractControlSP(title, desc, icon, parent), param(param) {
  toggle = new ToggleSP(this);
  std::string param_key = param.toStdString();
  std::string param_path = Params().getParamPath(param_key);
  std::string param_value_raw = Params().get(param_key);
  bool param_value = Params().getBool(param_key);

  printf("SP ParamControlSP: Loading bool param %s\n", param_key.c_str());
  printf("  - Full path: %s\n", param_path.c_str());
  printf("  - Raw value: '%s'\n", param_value_raw.c_str());
  printf("  - Raw value length: %zu\n", param_value_raw.length());
  printf("  - Compare with '1': %d\n", param_value_raw == "1");
  printf("  - Bool value: %s\n", param_value ? "true" : "false");

  // 直接读取文件内容
  std::ifstream file(param_path);
  if (file.good()) {
    std::string file_content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    printf("  - Direct file read: '%s'\n", file_content.c_str());
    file.close();
  } else {
    printf("  - ERROR - File NOT found at %s\n", param_path.c_str());
  }

  // 先设置内部状态，避免checkStateSet触发信号
  toggle->setOnState(param_value);
  // 然后设置UI状态
  toggle->setChecked(param_value);

  hlayout->addWidget(toggle);

  connect(toggle, &ToggleSP::stateChanged, [=](int state) {
    std::string key = param.toStdString();
    std::string val = state ? "1" : "0";
    std::string paramPath = Params().getParamPath(key);

    printf("SP ParamControlSP: stateChanged triggered with state=%d\n", state);
    printf("SP ParamControlSP: Saving bool param %s = %s to %s\n", key.c_str(), val.c_str(), paramPath.c_str());

    int result = Params().putBool(key, state);
    printf("SP ParamControlSP: putBool returned %d\n", result);

    // 验证保存后的值
    std::string saved_raw = Params().get(key);
    bool saved_bool = Params().getBool(key);
    printf("SP ParamControlSP: Verification - raw: '%s', bool: %s\n", saved_raw.c_str(), saved_bool ? "true" : "false");

    // 直接读取文件内容
    std::ifstream file(paramPath);
    if (file.good()) {
      std::string file_content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
      printf("SP ParamControlSP: Direct file read: '%s'\n", file_content.c_str());
      file.close();
    } else {
      printf("SP ParamControlSP: ERROR - File NOT found at %s\n", paramPath.c_str());
    }

    // 使用QTimer::singleShot确保在UI更新后再发出toggleFlipped信号
    QTimer::singleShot(0, [this, state]() {
      emit toggleFlipped(state);
    });
  });
}

// OptionControlSP
OptionControlSP::OptionControlSP(const QString &param, const QString &title, const QString &desc, const QString &icon,
                                   const std::pair<int, int> &range, const int per_value_change, const bool inline_layout,
                                   const QMap<QString, QString> *valMap, bool scale_float, QWidget *parent)
    : AbstractControlSP(title, desc, icon, parent), param(param) {

  slider = new QSlider(Qt::Horizontal);
  slider->setRange(range.first, range.second);
  slider->setSingleStep(per_value_change);

  // 获取参数值，如果是浮点数则进行缩放处理
  std::string param_key = param.toStdString();
  std::string param_value = Params().get(param_key);
  printf("SP OptionControlSP: Loading param %s = %s from %s\n", param_key.c_str(), param_value.c_str(), Params().getParamPath().c_str());
  int slider_value;
  if (scale_float && !param_value.empty()) {
    // 如果是浮点数缩放模式，将存储的浮点数转换为滑块值
    float float_value = std::stof(param_value);
    slider_value = static_cast<int>(float_value * 100.0f); // 乘以100转换为整数
  } else {
    slider_value = QString::fromStdString(param_value).toInt();
  }

  slider->setValue(slider_value);

  label = new QLabel();
  label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
  label->setMinimumWidth(100);

  hlayout->addWidget(slider);
  hlayout->addWidget(label);

  connect(slider, &QSlider::valueChanged, [=](int value) {
    std::string key = param.toStdString();
    std::string paramPath = Params().getParamPath(key);
    if (scale_float) {
      // 如果是浮点数缩放模式，将滑块值转换为浮点数存储
      float float_value = static_cast<float>(value) / 100.0f;
      std::string val = QString::number(float_value, 'f', 2).toStdString();
      Params().put(key, val);
      printf("SP OptionControlSP: Saved float param %s = %s to %s\n", key.c_str(), val.c_str(), paramPath.c_str());
    } else {
      std::string val = std::to_string(value);
      Params().put(key, val);
      printf("SP OptionControlSP: Saved int param %s = %s to %s\n", key.c_str(), val.c_str(), paramPath.c_str());
    }
    emit updateLabels();
  });

  emit updateLabels();
}

void OptionControlSP::setLabel(const QString &text) {
  if (label) {
    label->setText(text);
  }
}

// ListWidgetSP
ListWidgetSP::ListWidgetSP(QWidget *parent, bool horizontal) : QWidget(parent) {
  QVBoxLayout *main_layout = new QVBoxLayout(this);
  main_layout->setContentsMargins(0, 0, 0, 0);

  content = new QWidget();
  layout = new QVBoxLayout(content);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);

  scroll_area = new QScrollArea();
  scroll_area->setWidget(content);
  scroll_area->setWidgetResizable(true);
  scroll_area->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  scroll_area->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  scroll_area->setStyleSheet("QScrollArea { background-color: transparent; }");

  main_layout->addWidget(scroll_area);
}

void ListWidgetSP::addItem(QWidget *item) {
  layout->addWidget(item);
}

void ListWidgetSP::addItem(QWidget *item, const QString &key) {
  items[key] = item;
  addItem(item);
}

void ListWidgetSP::removeItem(QWidget *item) {
  layout->removeWidget(item);
  // Remove from items map if present
  for (auto it = items.begin(); it != items.end(); ++it) {
    if (it.value() == item) {
      items.erase(it);
      break;
    }
  }
}

void ListWidgetSP::clear() {
  QLayoutItem *item;
  while ((item = layout->takeAt(0)) != nullptr) {
    if (item->widget()) {
      item->widget()->setParent(nullptr);
    }
    delete item;
  }
  items.clear();
}

QWidget *ListWidgetSP::getItem(const QString &key) {
  return items.value(key, nullptr);
}