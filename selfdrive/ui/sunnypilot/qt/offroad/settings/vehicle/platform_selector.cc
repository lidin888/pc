/**
 * Copyright (c) 2021-, Haibin Wen, sunnypilot, and a number of other contributors.
 *
 * This file is part of sunnypilot and is licensed under the MIT License.
 * See the LICENSE.md file in the root directory for more details.
 */

#include "selfdrive/ui/sunnypilot/qt/offroad/settings/vehicle/platform_selector.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
#include <QFile>
#include <QDir>
#include <QTextStream>
#include <QRegExp>

#include "selfdrive/ui/qt/util.h"

QVariant PlatformSelector::getPlatformBundle(const QString &key) {
  QString platform_bundle = QString::fromStdString(params.get("CarPlatformBundle"));
  if (!platform_bundle.isEmpty()) {
    QJsonDocument json = QJsonDocument::fromJson(platform_bundle.toUtf8());
    if (!json.isNull() && json.isObject()) {
      return json.object().value(key).toVariant();
    }
  }
  return {};
}

QMap<QString, QVariantMap> PlatformSelector::loadPlatformList() {
  QMap<QString, QVariantMap> platform_list;

  // 尝试从多个来源加载车辆数据
  QStringList car_files = {
    "/data/openpilot/selfdrive/car/values.py",
    "/data/openpilot/selfdrive/ui/offroad/assets/SupportedCars",
    "/data/openpilot/selfdrive/ui/offroad/assets/SupportedCars_toyota",
    "/data/openpilot/selfdrive/ui/offroad/assets/SupportedCars_gm",
    "/data/openpilot/selfdrive/ui/offroad/assets/SupportedCars_honda",
    "/data/openpilot/selfdrive/ui/offroad/assets/SupportedCars_hyundai",
    "/data/openpilot/selfdrive/ui/offroad/assets/SupportedCars_mazda",
    "/data/openpilot/selfdrive/ui/offroad/assets/SupportedCars_nissan",
    "/data/openpilot/selfdrive/ui/offroad/assets/SupportedCars_volkswagen",
    "/data/openpilot/selfdrive/ui/offroad/assets/SupportedCars_tesla",
    "/data/openpilot/selfdrive/ui/offroad/assets/SupportedCars_ford",
    "/data/openpilot/selfdrive/ui/offroad/assets/SupportedCars_subaru",
    "/data/openpilot/selfdrive/ui/offroad/assets/SupportedCars_chrysler",
    "/data/openpilot/selfdrive/ui/offroad/assets/SupportedCars_rivian"
  };

  // 尝试从参数路径加载
  QString param_path = QString::fromStdString(params.getParamPath());
  if (!param_path.isEmpty()) {
    car_files.append(param_path + "/SupportedCars");
    car_files.append(param_path + "/SupportedCars_toyota");
    car_files.append(param_path + "/SupportedCars_gm");
    car_files.append(param_path + "/SupportedCars_honda");
    car_files.append(param_path + "/SupportedCars_hyundai");
    car_files.append(param_path + "/SupportedCars_mazda");
    car_files.append(param_path + "/SupportedCars_nissan");
    car_files.append(param_path + "/SupportedCars_volkswagen");
    car_files.append(param_path + "/SupportedCars_tesla");
    car_files.append(param_path + "/SupportedCars_ford");
    car_files.append(param_path + "/SupportedCars_subaru");
    car_files.append(param_path + "/SupportedCars_chrysler");
    car_files.append(param_path + "/SupportedCars_rivian");
  }

  for (const QString &file_path : car_files) {
    QFile file(file_path);
    if (file.exists() && file.open(QIODevice::ReadOnly | QIODevice::Text)) {
      QTextStream in(&file);
      while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith("#")) {
          continue;
        }

        // 解析车辆信息
        QStringList parts = line.split(" ", QString::SkipEmptyParts);
        if (parts.size() >= 2) {
          QString car_name = line;
          QString make = parts.first();
          QString model = parts.size() > 1 ? parts[1] : "";
          QString car_platform = parts.size() > 2 ? parts[2] : "";

          QVariantMap car_data;
          car_data["name"] = car_name;
          car_data["make"] = make;
          car_data["model"] = model;
          car_data["platform"] = car_platform;
          car_data["brand"] = make.toLower();

          // 尝试从文件名推断品牌
          if (file_path.contains("toyota", Qt::CaseInsensitive)) {
            car_data["brand"] = "toyota";
          } else if (file_path.contains("gm", Qt::CaseInsensitive)) {
            car_data["brand"] = "gm";
          } else if (file_path.contains("honda", Qt::CaseInsensitive)) {
            car_data["brand"] = "honda";
          } else if (file_path.contains("hyundai", Qt::CaseInsensitive)) {
            car_data["brand"] = "hyundai";
          } else if (file_path.contains("mazda", Qt::CaseInsensitive)) {
            car_data["brand"] = "mazda";
          } else if (file_path.contains("nissan", Qt::CaseInsensitive)) {
            car_data["brand"] = "nissan";
          } else if (file_path.contains("volkswagen", Qt::CaseInsensitive)) {
            car_data["brand"] = "volkswagen";
          } else if (file_path.contains("tesla", Qt::CaseInsensitive)) {
            car_data["brand"] = "tesla";
          } else if (file_path.contains("ford", Qt::CaseInsensitive)) {
            car_data["brand"] = "ford";
          } else if (file_path.contains("subaru", Qt::CaseInsensitive)) {
            car_data["brand"] = "subaru";
          } else if (file_path.contains("chrysler", Qt::CaseInsensitive)) {
            car_data["brand"] = "chrysler";
          } else if (file_path.contains("rivian", Qt::CaseInsensitive)) {
            car_data["brand"] = "rivian";
          }

          // 尝试从名称中提取年份
          QRegExp yearRegex("(\\d{4})");
          if (yearRegex.indexIn(car_name) != -1) {
            QString year = yearRegex.cap(1);
            QVariantList years;
            years.append(year);
            car_data["year"] = years;
          }

          platform_list[car_name] = car_data;
        }
      }
      file.close();
    }
  }

  return platform_list;
}

PlatformSelector::PlatformSelector() : ButtonControl(tr("Vehicle"), "", "") {
  platforms = loadPlatformList();

  QObject::connect(this, &ButtonControl::clicked, [=]() {
    if (text() == tr("SEARCH")) {
      QString query = InputDialog::getText(tr("Search your vehicle"), this, tr("Enter model year (e.g., 2021) and model name (Toyota Corolla):"), false);
      if (query.length() > 0) {
        setText(tr("SEARCHING"));
        setEnabled(false);
        searchPlatforms(query);
        refresh(offroad);
      }
    } else {
      params.remove("CarPlatformBundle");
      refresh(offroad);
    }
  });
}

void PlatformSelector::refresh(bool _offroad) {
  QString name = getPlatformBundle("name").toString();
  platform = unrecognized_str;
  QString platform_color = YELLOW_PLATFORM;

  if (!name.isEmpty()) {
    platform = name;
    platform_color = BLUE_PLATFORM;
    brand = getPlatformBundle("brand").toString();
    setText(tr("REMOVE"));
  } else {
    setText(tr("SEARCH"));

    platform = unrecognized_str;
    brand = "";
    auto cp_bytes = params.get("CarParamsPersistent");
    if (!cp_bytes.empty()) {
      // 简化版本，不使用capnp解析
      platform = tr("Unknown");

      for (auto it = platforms.constBegin(); it != platforms.constEnd(); ++it) {
        if (it.value()["platform"].toString() == platform) {
          brand = it.value()["brand"].toString();
          break;
        }
      }

      if (platform == "MOCK") {
        platform = unrecognized_str;
      } else {
        platform_color = GREEN_PLATFORM;
      }
    }
  }
  setValue(platform);
  setEnabled(true);
  emit refreshPanel();

  offroad = _offroad;

  FingerprintStatus cur_status;
  if (platform_color == GREEN_PLATFORM) {
    cur_status = FingerprintStatus::AUTO_FINGERPRINT;
  } else if (platform_color == BLUE_PLATFORM) {
    cur_status = FingerprintStatus::MANUAL_FINGERPRINT;
  } else {
    cur_status = FingerprintStatus::UNRECOGNIZED;
  }

  setDescription(platformDescription(cur_status));
  showDescription();
}

void PlatformSelector::setPlatform(const QString &_platform) {
  QVariantMap platform_data = platforms[_platform];

  const QString offroad_msg = offroad ? tr("This setting will take effect immediately.") :
                                        tr("This setting will take effect once the device enters offroad state.");
  const QString msg = QString("<b>%1</b><br><br>%2")
                      .arg(_platform, offroad_msg);

  QString content("<body><h2 style=\"text-align: center;\">" + tr("Vehicle Selector") + "</h2><br>"
                  "<p style=\"text-align: center; margin: 0 128px; font-size: 50px;\">" + msg + "</p></body>");

  if (ConfirmationDialog(content, tr("Confirm"), tr("Cancel"), true, this).exec()) {
    QJsonObject json_bundle;
    json_bundle["platform"] = platform_data["platform"].toString();
    json_bundle["name"] = _platform;
    json_bundle["make"] = platform_data["make"].toString();
    json_bundle["brand"] = platform_data["brand"].toString();
    json_bundle["model"] = platform_data["model"].toString();
    json_bundle["package"] = platform_data["package"].toString();

    QVariantList yearList = platform_data["year"].toList();
    QJsonArray yearArray;
    for (const QVariant &year : yearList) {
      yearArray.append(year.toString());
    }
    json_bundle["year"] = yearArray;

    QString json_bundle_str = QString::fromUtf8(QJsonDocument(json_bundle).toJson(QJsonDocument::Compact));

    params.put("CarPlatformBundle", json_bundle_str.toStdString());
  }
}

void PlatformSelector::searchPlatforms(const QString &query) {
  if (query.isEmpty()) {
    return;
  }

  QSet<QString> matched_cars;

  QString normalized_query = query.simplified().toLower();
  QStringList tokens = normalized_query.split(" ", QString::SkipEmptyParts);

  int search_year = -1;
  QStringList search_terms;

  for (const QString &token : tokens) {
    bool ok;
    int year = token.toInt(&ok);
    if (ok && year >= 1900 && year <= 2100) {
      search_year = year;
    } else {
      search_terms << token;
    }
  }

  for (auto it = platforms.constBegin(); it != platforms.constEnd(); ++it) {
    QString platform_name = it.key();
    QVariantMap platform_data = it.value();

    if (search_year != -1) {
      QVariantList year_list = platform_data["year"].toList();
      bool year_match = false;
      for (const QVariant &year_var : year_list) {
        int year = year_var.toString().toInt();
        if (year == search_year) {
          year_match = true;
          break;
        }
      }
      if (!year_match) continue;
    }

    QString normalized_make = platform_data["make"].toString().normalized(QString::NormalizationForm_KD).toLower();
    QString normalized_model = platform_data["model"].toString().normalized(QString::NormalizationForm_KD).toLower();
    normalized_make.remove(QRegExp("[^a-zA-Z0-9\\s]"));
    normalized_model.remove(QRegExp("[^a-zA-Z0-9\\s]"));

    bool all_terms_match = true;
    for (const QString &term : search_terms) {
      QString normalized_term = term.normalized(QString::NormalizationForm_KD).toLower();
      normalized_term.remove(QRegExp("[^a-zA-Z0-9\\s]"));

      bool term_matched = false;

      if (normalized_make.contains(normalized_term, Qt::CaseInsensitive)) {
        term_matched = true;
      }

      if (!term_matched) {
        if (term.contains(QRegExp("[a-z]\\d|\\d[a-z]", Qt::CaseInsensitive))) {
          QString clean_model = normalized_model;
          QString clean_term = normalized_term;
          clean_model.remove(" ");
          clean_term.remove(" ");
          if (clean_model.contains(clean_term, Qt::CaseInsensitive)) {
            term_matched = true;
          }
        } else {
          if (normalized_model.contains(normalized_term, Qt::CaseInsensitive)) {
            term_matched = true;
          }
        }
      }

      if (!term_matched) {
        all_terms_match = false;
        break;
      }
    }

    if (all_terms_match) {
      matched_cars.insert(platform_name);
    }
  }

  QStringList results = matched_cars.toList();
  results.sort();

  if (results.isEmpty()) {
    ConfirmationDialog::alert(tr("No vehicles found for query: %1").arg(query), this);
    return;
  }

  QString selected_platform = MultiOptionDialog::getSelection(tr("Select a vehicle"), results, "", this);

  if (!selected_platform.isEmpty()) {
    setPlatform(selected_platform);
  }
}