/**
 * Copyright (c) 2021-, Haibin Wen, sunnypilot, and a number of other contributors.
 *
 * This file is part of sunnypilot and is licensed under the MIT License.
 * See the LICENSE.md file in the root directory for more details.
 */

#pragma once

#include <map>
#include <optional>
#include <string>
#include <vector>

#include <QAbstractButton>
#include <QButtonGroup>
#include <QScrollArea>
#include <QSlider>

#include "common/params.h"
#include "selfdrive/ui/qt/widgets/controls.h"
#include "selfdrive/ui/qt/widgets/input.h"
#include "selfdrive/ui/sunnypilot/qt/widgets/toggle.h"

QFrame *horizontal_line(QWidget *parent = nullptr);
QFrame *vertical_space(int height = 10, QWidget *parent = nullptr);

inline void ReplaceWidget(QWidget *old_widget, QWidget *new_widget) {
  if (old_widget && old_widget->parentWidget() && old_widget->parentWidget()->layout()) {
    old_widget->parentWidget()->layout()->replaceWidget(old_widget, new_widget);
    old_widget->hide();
    old_widget->deleteLater();
  }
}

class ElidedLabelSP : public QLabel {
  Q_OBJECT

public:
  explicit ElidedLabelSP(QWidget *parent = 0);
  explicit ElidedLabelSP(const QString &text, QWidget *parent = 0);

  void setColor(const QString &color) {
    setStyleSheet("QLabel { color : " + color + "; }");
  }

signals:
  void clicked();

protected:
  void paintEvent(QPaintEvent *event) override;
  void resizeEvent(QResizeEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override {
    if (rect().contains(event->pos())) {
      emit clicked();
    }
  }
  QString lastText_, elidedText_;
};

class AbstractControlSP : public AbstractControl {
  Q_OBJECT

public:
  ~AbstractControlSP();
  void setDescription(const QString &desc) {
    AbstractControl::setDescription(desc);
  }

  void setValue(const QString &val, std::optional<QString> color = std::nullopt) {
    AbstractControl::setValue(val);
    if (color.has_value()) {
      setStyleSheet("QLabel { color : " + color.value() + "; }");
    }
  }

  const QString getDescription() {
    return AbstractControl::getDescription();
  }

  void hideDescription() {
    // Since description is private, we'll need to use a different approach
    // For now, let's assume we can access it through the base class
    // This might need further adjustment based on the actual implementation
  }

public slots:
  void showDescription() {
    AbstractControl::showDescription();
  }

  void setVisible(bool visible) override {
    bool _visible = visible;
    if (isAdvancedControl && !params.getBool("ShowAdvancedControls")) {
      _visible = false;
    }
    AbstractControl::setVisible(_visible);
  }

  static void RegisterAdvancedControl(AbstractControlSP *ctrl);
  static void UnregisterAdvancedControl(AbstractControlSP *ctrl);
  static void UpdateAllAdvancedControls();

protected:
  AbstractControlSP(const QString &title, const QString &desc = "", const QString &icon = "", QWidget *parent = nullptr, bool advancedControl = false);
  void hideEvent(QHideEvent *e) override;

private:
  bool isAdvancedControl = false;
  Params params;
  static std::vector<AbstractControlSP*> advancedControls;
};

class ButtonControlSP : public AbstractControlSP {
  Q_OBJECT

public:
  ButtonControlSP(const QString &title, const QString &text, const QString &desc = "", const QString &icon = "", QWidget *parent = nullptr);
  ButtonControlSP(const QString &title, const QString &text, const QString &desc, const QString &icon, std::function<void()> onClick, QWidget *parent = nullptr);

signals:
  void clicked();

public slots:
  void showDescription() {
    AbstractControl::showDescription();
  }

protected:
  void showEvent(QShowEvent *event) override;
};

class ButtonParamControlSP : public ButtonControlSP {
  Q_OBJECT

public:
  ButtonParamControlSP(const QString &param, const QString &title, const QString &desc, const QString &icon, QWidget *parent = nullptr);

private:
  QString param;
};

class ParamControlSP : public AbstractControlSP {
  Q_OBJECT

public:
  ParamControlSP(const QString &param, const QString &title, const QString &desc, const QString &icon, QWidget *parent = nullptr);

signals:
  void toggleFlipped(bool state);

private:
  QString param;
  ToggleSP *toggle;
};

class OptionControlSP : public AbstractControlSP {
  Q_OBJECT

public:
  OptionControlSP(const QString &param, const QString &title, const QString &desc, const QString &icon,
                  const std::pair<int, int> &range, const int per_value_change = 1, const bool inline_layout = false,
                  const QMap<QString, QString> *valMap = nullptr, bool scale_float = false, QWidget *parent = nullptr);

  void setLabel(const QString &text);

signals:
  void updateLabels();

private:
  QString param;
  QSlider *slider;
  QLabel *label;
};

class ListWidgetSP : public QWidget {
  Q_OBJECT

public:
  explicit ListWidgetSP(QWidget *parent = nullptr, bool horizontal = false);
  void addItem(QWidget *item);
  void addItem(QWidget *item, const QString &key);
  void removeItem(QWidget *item);
  void clear();
  QWidget *getItem(const QString &key);

private:
  QScrollArea *scroll_area;
  QWidget *content;
  QVBoxLayout *layout;
  QMap<QString, QWidget*> items;
};