/**
 * Copyright (c) 2021-, Haibin Wen, sunnypilot, and a number of other contributors.
 *
 * This file is part of sunnypilot and is licensed under the MIT License.
 * See the LICENSE.md file in the root directory for more details.
 */
#include <QPainterPath>

#include "selfdrive/ui/sunnypilot/qt/onroad/hud.h"

#include "selfdrive/ui/qt/util.h"


HudRendererSP::HudRendererSP() {
  plus_arrow_up_img = loadPixmap("../../sunnypilot/selfdrive/assets/img_plus_arrow_up", {90, 90});
  minus_arrow_down_img = loadPixmap("../../sunnypilot/selfdrive/assets/img_minus_arrow_down", {90, 90});

  int size = e2e_alert_size * 2 - 40;
  green_light_alert_img = loadPixmap("../../sunnypilot/selfdrive/assets/images/green_light.png", {size, size});
  lead_depart_alert_img = loadPixmap("../../sunnypilot/selfdrive/assets/images/lead_depart.png", {size, size});

  // Turn-by-turn navigation icons
  turn_l_img = loadPixmap("../../sunnypilot/selfdrive/assets/images/turn_l.png", {128, 128});
  turn_r_img = loadPixmap("../../sunnypilot/selfdrive/assets/images/turn_r.png", {128, 128});
  turn_u_img = loadPixmap("../../sunnypilot/selfdrive/assets/images/turn_u.png", {128, 128});
  lane_change_l_img = loadPixmap("../../sunnypilot/selfdrive/assets/images/lane_change_l.png", {128, 128});
  lane_change_r_img = loadPixmap("../../sunnypilot/selfdrive/assets/images/lane_change_r.png", {128, 128});
}

void HudRendererSP::updateState(const UIState &s) {
  HudRenderer::updateState(s);

  float speedConv = is_metric ? MS_TO_KPH : MS_TO_MPH;
  devUiInfo = s.scene.dev_ui_info;
  roadName = s.scene.road_name;
  showTurnSignals = s.scene.turn_signals;
  carrotPanelSide = s.scene.carrot_panel_side;
  speedLimitMode = static_cast<SpeedLimitMode>(s.scene.speed_limit_mode);
  speedUnit = is_metric ? tr("km/h") : tr("mph");
  standstillTimer = s.scene.standstill_timer;

  const SubMaster &sm = *(s.sm);
  const auto cs = sm["controlsState"].getControlsState();
  const auto car_state = sm["carState"].getCarState();
  const auto car_control = sm["carControl"].getCarControl();
  const auto radar_state = sm["radarState"].getRadarState();
  const auto is_gps_location_external = sm.rcv_frame("gpsLocationExternal") > 1;
  const char *gps_source = is_gps_location_external ? "gpsLocationExternal" : "gpsLocation";
  const auto gpsLocation = is_gps_location_external ? sm[gps_source].getGpsLocationExternal() : sm[gps_source].getGpsLocation();
  const auto ltp = sm["liveTorqueParameters"].getLiveTorqueParameters();
  const auto car_params = sm["carParams"].getCarParams();
  const auto car_params_sp = sm["carParamsSP"].getCarParamsSP();
  const auto lp_sp = sm["longitudinalPlanSP"].getLongitudinalPlanSP();
  const auto lmd = sm["liveMapDataSP"].getLiveMapDataSP();

  if (sm.updated("carParams")) {
    steerControlType = car_params.getSteerControlType();
  }

  if (sm.updated("carParamsSP")) {
    pcmCruiseSpeed = car_params_sp.getPcmCruiseSpeed();
  }

  if (sm.updated("longitudinalPlanSP")) {
    speedLimit = lp_sp.getSpeedLimit().getResolver().getSpeedLimit() * speedConv;
    speedLimitLast = lp_sp.getSpeedLimit().getResolver().getSpeedLimitLast() * speedConv;
    speedLimitOffset = lp_sp.getSpeedLimit().getResolver().getSpeedLimitOffset() * speedConv;
    speedLimitValid = lp_sp.getSpeedLimit().getResolver().getSpeedLimitValid();
    speedLimitLastValid = lp_sp.getSpeedLimit().getResolver().getSpeedLimitLastValid();
    speedLimitFinalLast = lp_sp.getSpeedLimit().getResolver().getSpeedLimitFinalLast() * speedConv;
    speedLimitSource = lp_sp.getSpeedLimit().getResolver().getSource();
    speedLimitAssistState = lp_sp.getSpeedLimit().getAssist().getState();
    speedLimitAssistActive = lp_sp.getSpeedLimit().getAssist().getActive();
    smartCruiseControlVisionEnabled = lp_sp.getSmartCruiseControl().getVision().getEnabled();
    smartCruiseControlVisionActive = lp_sp.getSmartCruiseControl().getVision().getActive();
    smartCruiseControlMapEnabled = lp_sp.getSmartCruiseControl().getMap().getEnabled();
    smartCruiseControlMapActive = lp_sp.getSmartCruiseControl().getMap().getActive();
  }
  greenLightAlert = lp_sp.getE2eAlerts().getGreenLightAlert();
  leadDepartAlert = lp_sp.getE2eAlerts().getLeadDepartAlert();

  if (sm.updated("liveMapDataSP")) {
    roadNameStr = QString::fromStdString(lmd.getRoadName());
    speedLimitAheadValid = lmd.getSpeedLimitAheadValid();
    speedLimitAhead = lmd.getSpeedLimitAhead() * speedConv;
    speedLimitAheadDistance = lmd.getSpeedLimitAheadDistance();
    if (speedLimitAheadDistance < speedLimitAheadDistancePrev && speedLimitAheadValidFrame < SPEED_LIMIT_AHEAD_VALID_FRAME_THRESHOLD) {
      speedLimitAheadValidFrame++;
    } else if (speedLimitAheadDistance > speedLimitAheadDistancePrev && speedLimitAheadValidFrame > 0) {
      speedLimitAheadValidFrame--;
    }
  }
  speedLimitAheadDistancePrev = speedLimitAheadDistance;

  static int reverse_delay = 0;
  bool reverse_allowed = false;
  if (car_state.getGearShifter() != cereal::CarState::GearShifter::REVERSE) {
    reverse_delay = 0;
    reverse_allowed = false;
  } else {
    reverse_delay += 50;
    if (reverse_delay >= 1000) {
      reverse_allowed = true;
    }
  }

  reversing = reverse_allowed;

  if (sm.updated("liveParameters")) {
    roll = sm["liveParameters"].getLiveParameters().getRoll();
  }

  if (sm.updated("deviceState")) {
    memoryUsagePercent = sm["deviceState"].getDeviceState().getMemoryUsagePercent();
  }

  if (sm.updated(gps_source)) {
    gpsAccuracy = is_gps_location_external ? gpsLocation.getHorizontalAccuracy() : 1.0;  // External reports accuracy, internal does not.
    altitude = gpsLocation.getAltitude();
    bearingAccuracyDeg = gpsLocation.getBearingAccuracyDeg();
    bearingDeg = gpsLocation.getBearingDeg();
  }

  if (sm.updated("liveTorqueParameters")) {
    torquedUseParams = ltp.getUseParams();
    latAccelFactorFiltered = ltp.getLatAccelFactorFiltered();
    frictionCoefficientFiltered = ltp.getFrictionCoefficientFiltered();
    liveValid = ltp.getLiveValid();
  }

  latActive = car_control.getLatActive();
  actuators = car_control.getActuators();
  longOverride = car_control.getCruiseControl().getOverride();
  carControlEnabled = car_control.getEnabled();

  steerOverride = car_state.getSteeringPressed();
  lead_d_rel = radar_state.getLeadOne().getDRel();
  lead_v_rel = radar_state.getLeadOne().getVRel();
  lead_status = radar_state.getLeadOne().getStatus();
  torqueLateral = steerControlType == cereal::CarParams::SteerControlType::TORQUE;
  angleSteers = car_state.getSteeringAngleDeg();
  desiredCurvature = cs.getDesiredCurvature();
  curvature = cs.getCurvature();
  vEgo = car_state.getVEgo();
  aEgo = car_state.getAEgo();
  steeringTorqueEps = car_state.getSteeringTorqueEps();

  isStandstill = car_state.getStandstill();
  if (!s.scene.started) standstillElapsedTime = 0.0;

  // override stock current speed values
  float v_ego = (v_ego_cluster_seen && !s.scene.trueVEgoUI) ? car_state.getVEgoCluster() : car_state.getVEgo();
  speed = std::max<float>(0.0f, v_ego * (is_metric ? MS_TO_KPH : MS_TO_MPH));
  hideVEgoUI = s.scene.hideVEgoUI;

  leftBlinkerOn = car_state.getLeftBlinker();
  rightBlinkerOn = car_state.getRightBlinker();
  leftBlindspot = car_state.getLeftBlindspot();
  rightBlindspot = car_state.getRightBlindspot();

  speedCluster = car_state.getCruiseState().getSpeedCluster() * speedConv;

  // CarrotMan navigation data
  carrotManAlive = sm.alive("carrotMan");
  if (carrotManAlive && sm.updated("carrotMan")) {
    const auto cm = sm["carrotMan"].getCarrotMan();
    carrotActiveCarrot = cm.getActiveCarrot();
    carrotNaviSpeedLimit = cm.getNRoadLimitSpeed();
    carrotSpdType = cm.getXSpdType();
    carrotSpdLimit = cm.getXSpdLimit();
    carrotSpdDist = cm.getXSpdDist();
    carrotSpdCountDown = cm.getXSpdCountDown();
    carrotTurnInfo = cm.getXTurnInfo();
    carrotDistToTurn = cm.getXDistToTurn();
    carrotTrafficState = cm.getTrafficState();
    carrotTrafficCountdown = cm.getTrafficCountdown();
    carrotDesiredSpeed = cm.getDesiredSpeed();
    carrotDestDist = cm.getNGoPosDist();
    carrotDestTime = cm.getNGoPosTime();
    carrotLeftSec = cm.getLeftSec();
    carrotRoadCate = cm.getRoadCate();
    carrotVTurnSpeed = cm.getVTurnSpeed();
    carrotLeftBlind = cm.getLeftBlind();
    carrotRightBlind = cm.getRightBlind();
    carrotExtBlinker = cm.getExtBlinker();
    carrotRoadName = QString::fromUtf8(cm.getSzPosRoadName().cStr());
    carrotTBTText = QString::fromUtf8(cm.getSzTBTMainText().cStr());
    carrotDesiredSource = QString::fromUtf8(cm.getDesiredSource().cStr());
    carrotSdiDescr = QString::fromUtf8(cm.getSzSdiDescr().cStr());
    carrotAtcType = QString::fromUtf8(cm.getAtcType().cStr());
    carrotGoalName = QString::fromUtf8(cm.getSzGoalName().cStr());
    carrotTBTTextNext = QString::fromUtf8(cm.getSzTBTMainTextNext().cStr());
    carrotNearDirName = QString::fromUtf8(cm.getSzNearDirName().cStr());
    carrotTurnCountDown = cm.getXTurnCountDown();
  } else if (!carrotManAlive) {
    carrotActiveCarrot = 0;
    carrotSpdType = -1;
    carrotSpdLimit = 0;
    carrotSpdDist = 0;
    carrotSpdCountDown = 0;
    carrotTurnInfo = -1;
    carrotDistToTurn = 0;
    carrotTrafficState = 0;
    carrotTrafficCountdown = 0;
    carrotDestDist = 0;
    carrotDestTime = 0;
    carrotDesiredSpeed = 0;
    carrotLeftSec = 0;
    carrotRoadCate = 0;
    carrotVTurnSpeed = 0;
    carrotLeftBlind = 0;
    carrotRightBlind = 0;
    carrotExtBlinker = 0;
    carrotRoadName.clear();
    carrotTBTText.clear();
    carrotGoalName.clear();
    carrotTBTTextNext.clear();
    carrotNearDirName.clear();
    carrotTurnCountDown = 0;
    carrotSdiDescr.clear();
    carrotAtcType.clear();
  }

  // Supplement road name from carrot navigation (carrot takes priority)
  if (carrotManAlive && !carrotRoadName.isEmpty()) {
    roadNameStr = carrotRoadName;
  }

  // When carrotMan provides road speed limit and SP map limit is unavailable,
  // feed carrot's nRoadLimitSpeed into the SP speed limit sign display
  if (carrotManAlive && carrotNaviSpeedLimit > 0 && !speedLimitValid && !speedLimitLastValid) {
    speedLimitLast = carrotNaviSpeedLimit;
    speedLimitLastValid = true;
    speedLimitFinalLast = carrotNaviSpeedLimit;
    speedLimitOffset = 0;
  }

  allow_e2e_alerts = sm["selfdriveState"].getSelfdriveState().getAlertSize() == cereal::SelfdriveState::AlertSize::NONE &&
                     sm.rcv_frame("driverStateV2") > s.scene.started_frame && !reversing;
}

void HudRendererSP::draw(QPainter &p, const QRect &surface_rect) {
  HudRenderer::draw(p, surface_rect);

  e2eAlertDisplayTimer = std::max(0, e2eAlertDisplayTimer - 1);

  p.save();

  if (is_cruise_available) {
    drawSetSpeedSP(p, surface_rect);
  }

  if (!hideVEgoUI) {
    drawCurrentSpeedSP(p, surface_rect);
  }

  if (!reversing) {
    // Smart Cruise Control
    int x_offset = -260;
    int y1_offset = -80;
    int y2_offset = -140;

    int y_scc_v = 0, y_scc_m = 0;
    const int orders[2] = {y1_offset, y2_offset};
    int i = 0;
    // SCC-V takes first order
    if (smartCruiseControlVisionEnabled) y_scc_v = orders[i++];
    if (smartCruiseControlMapEnabled) y_scc_m = orders[i++];

    // Smart Cruise Control - Vision
    bool scc_vision_active_pulse = pulseElement(smartCruiseControlVisionFrame);
    if ((smartCruiseControlVisionEnabled && !smartCruiseControlVisionActive) || (smartCruiseControlVisionActive && scc_vision_active_pulse)) {
      drawSmartCruiseControlOnroadIcon(p, surface_rect, x_offset, y_scc_v, "SCC-V");
    }
    smartCruiseControlVisionFrame = smartCruiseControlVisionActive ? (smartCruiseControlVisionFrame + 1) : 0;

    // Smart Cruise Control - Map
    bool scc_map_active_pulse = pulseElement(smartCruiseControlMapFrame);
    if ((smartCruiseControlMapEnabled && !smartCruiseControlMapActive) || (smartCruiseControlMapActive && scc_map_active_pulse)) {
      drawSmartCruiseControlOnroadIcon(p, surface_rect, x_offset, y_scc_m, "SCC-M");
    }
    smartCruiseControlMapFrame = smartCruiseControlMapActive ? (smartCruiseControlMapFrame + 1) : 0;

    // Bottom Dev UI
    if (devUiInfo == 2) {
      QRect rect_bottom(surface_rect.left(), surface_rect.bottom() - 60, surface_rect.width(), 61);
      p.setPen(Qt::NoPen);
      p.setBrush(QColor(0, 0, 0, 100));
      p.drawRect(rect_bottom);
      drawBottomDevUI(p, rect_bottom.left(), rect_bottom.center().y());
    }

    // Right Dev UI
    if (devUiInfo != 0) {
      QRect rect_right(surface_rect.right() - (UI_BORDER_SIZE * 2), UI_BORDER_SIZE * 1.5, 184, 170);
      drawRightDevUI(p, surface_rect.right() - 184 - UI_BORDER_SIZE * 2, UI_BORDER_SIZE * 2 + rect_right.height());
    }

    // Speed Limit
    bool showSpeedLimit;
    bool speed_limit_assist_pre_active_pulse = pulseElement(speedLimitAssistFrame);

    // Position speed limit sign next to set speed box
    const int sign_width = is_metric ? 200 : 172;
    const int sign_x = is_metric ? 280 : 272;
    const int sign_y = 45;
    const int sign_height = 204;
    QRect sign_rect(sign_x, sign_y, sign_width, sign_height);

    if (speedLimitAssistState == cereal::LongitudinalPlanSP::SpeedLimit::AssistState::PRE_ACTIVE) {
      speedLimitAssistFrame++;
      showSpeedLimit = speed_limit_assist_pre_active_pulse;
      drawSpeedLimitPreActiveArrow(p, sign_rect);
    } else {
      speedLimitAssistFrame = 0;
      showSpeedLimit = speedLimitMode != SpeedLimitMode::OFF;
    }

    if (showSpeedLimit) {
      drawSpeedLimitSigns(p, sign_rect);

      // do not show during SLA's preActive state
      if (speedLimitAssistState != cereal::LongitudinalPlanSP::SpeedLimit::AssistState::PRE_ACTIVE) {
        drawUpcomingSpeedLimit(p);
      }
    }

    // Road Name
    drawRoadName(p, surface_rect);

    // CarrotMan Navigation Panel (bottom-left)
    if (carrotManAlive) {
      drawCarrotPanel(p, surface_rect);

      // ===== Blind Spot Warning (screen edge overlay) =====
      if (carrotLeftBlind > 0 || carrotRightBlind > 0) {
        p.save();
        p.setRenderHint(QPainter::Antialiasing, true);
        int bsH = surface_rect.height() * 2 / 3;
        int bsW = 40;
        int bsY = surface_rect.height() / 6;

        if (carrotLeftBlind > 0) {
          QLinearGradient gradL(0, 0, bsW, 0);
          gradL.setColorAt(0.0, QColor(0, 100, 255, 180));
          gradL.setColorAt(1.0, QColor(0, 100, 255, 0));
          p.setPen(Qt::NoPen);
          p.setBrush(gradL);
          p.drawRect(QRect(0, bsY, bsW, bsH));
        }
        if (carrotRightBlind > 0) {
          QLinearGradient gradR(surface_rect.width() - bsW, 0, surface_rect.width(), 0);
          gradR.setColorAt(0.0, QColor(0, 100, 255, 0));
          gradR.setColorAt(1.0, QColor(0, 100, 255, 180));
          p.setPen(Qt::NoPen);
          p.setBrush(gradR);
          p.drawRect(QRect(surface_rect.width() - bsW, bsY, bsW, bsH));
        }
        p.restore();
      }

      // ===== External Blinker Indicator =====
      if (carrotExtBlinker > 0) {
        p.save();
        p.setRenderHint(QPainter::Antialiasing, true);
        int blinkY = 30;
        int arrowW = 60, arrowH = 40;
        int centerX = surface_rect.width() / 2;
        QColor blinkColor(0, 220, 80, 200);

        auto drawArrow = [&](int cx, int cy, bool pointRight) {
          QPolygon arrow;
          if (pointRight) {
            arrow << QPoint(cx - arrowW/2, cy - arrowH/4)
                  << QPoint(cx, cy - arrowH/4)
                  << QPoint(cx, cy - arrowH/2)
                  << QPoint(cx + arrowW/2, cy)
                  << QPoint(cx, cy + arrowH/2)
                  << QPoint(cx, cy + arrowH/4)
                  << QPoint(cx - arrowW/2, cy + arrowH/4);
          } else {
            arrow << QPoint(cx + arrowW/2, cy - arrowH/4)
                  << QPoint(cx, cy - arrowH/4)
                  << QPoint(cx, cy - arrowH/2)
                  << QPoint(cx - arrowW/2, cy)
                  << QPoint(cx, cy + arrowH/2)
                  << QPoint(cx, cy + arrowH/4)
                  << QPoint(cx + arrowW/2, cy + arrowH/4);
          }
          p.setPen(Qt::NoPen);
          p.setBrush(blinkColor);
          p.drawPolygon(arrow);
        };

        // 1=left, 2=right, 3=both(hazard)
        if (carrotExtBlinker == 1 || carrotExtBlinker == 3) {
          drawArrow(centerX - 120, blinkY + arrowH/2, false);
          drawArrow(centerX - 190, blinkY + arrowH/2, false);
        }
        if (carrotExtBlinker == 2 || carrotExtBlinker == 3) {
          drawArrow(centerX + 120, blinkY + arrowH/2, true);
          drawArrow(centerX + 190, blinkY + arrowH/2, true);
        }
        p.restore();
      }
    }

    // Green Light & Lead Depart Alerts
    if (greenLightAlert || leadDepartAlert) {
      e2eAlertDisplayTimer = 3 * UI_FREQ;
      // reset onroad sleep timer for e2e alerts
      uiStateSP()->reset_onroad_sleep_timer();
    }

    if (e2eAlertDisplayTimer > 0) {
      e2eAlertFrame++;
      if (greenLightAlert) {
        alert_text = tr("GREEN\nLIGHT");
        alert_img = green_light_alert_img;
      }
      else if (leadDepartAlert) {
        alert_text = tr("LEAD VEHICLE\nDEPARTING");
        alert_img = lead_depart_alert_img;
      }
      drawE2eAlert(p, surface_rect);
    }
    // Standstill Timer
    else if (standstillTimer && isStandstill) {
      alert_img = QPixmap();

      standstillElapsedTime += 1.0 / UI_FREQ;
      int minute = static_cast<int>(standstillElapsedTime / 60);
      int second = static_cast<int>(standstillElapsedTime - (minute * 60));
      alert_text = QString("%1:%2").arg(minute, 1, 10, QChar('0')).arg(second, 2, 10, QChar('0'));
      drawE2eAlert(p, surface_rect, tr("STOPPED"));
      e2eAlertFrame++;
    }
    // No Alerts displayed
    else {
      e2eAlertFrame = 0;
      if (!isStandstill) standstillElapsedTime = 0.0;
    }

    // Blinker
    if (showTurnSignals) {
      drawBlinker(p, surface_rect);
    }
  }

  p.restore();
}

void HudRendererSP::drawText(QPainter &p, int x, int y, const QString &text, QColor color) {
  QRect real_rect = p.fontMetrics().boundingRect(text);
  real_rect.moveCenter({x, y - real_rect.height() / 2});
  p.setPen(color);
  p.drawText(real_rect.x(), real_rect.bottom(), text);
}

bool HudRendererSP::pulseElement(int frame) {
  if (frame % UI_FREQ < (UI_FREQ / 2.5)) {
    return false;
  }

  return true;
}

void HudRendererSP::drawSmartCruiseControlOnroadIcon(QPainter &p, const QRect &surface_rect, int x_offset, int y_offset, std::string name) {
  int x = surface_rect.center().x();
  int y = surface_rect.height() / 4;

  QString text = QString::fromStdString(name);
  QFont font = InterFont(36, QFont::Bold);
  p.setFont(font);

  QFontMetrics fm(font);

  int padding_v = 5;
  int box_width = 160;
  int box_height = fm.height() + padding_v * 2;

  QRectF bg_rect(x - (box_width / 2) + x_offset,
                 y - (box_height / 2) + y_offset,
                 box_width, box_height);

  QPainterPath boxPath;
  boxPath.addRoundedRect(bg_rect, 10, 10);

  int text_w = fm.horizontalAdvance(text);
  qreal baseline_y = bg_rect.top() + padding_v + fm.ascent();
  qreal text_x = bg_rect.center().x() - (text_w / 2.0);

  QPainterPath textPath;
  textPath.addText(QPointF(text_x, baseline_y), font, text);
  boxPath = boxPath.subtracted(textPath);

  p.setPen(Qt::NoPen);
  p.setBrush(longOverride ? QColor(0x91, 0x9b, 0x95, 0xf1) : QColor(0, 0xff, 0, 0xff));
  p.drawPath(boxPath);
}

int HudRendererSP::drawRightDevUIElement(QPainter &p, int x, int y, const QString &value, const QString &label, const QString &units, QColor &color) {

  p.setFont(InterFont(28, QFont::Bold));
  x += 92;
  y += 80;
  drawText(p, x, y, label);

  p.setFont(InterFont(30 * 2, QFont::Bold));
  y += 65;
  drawText(p, x, y, value, color);

  p.setFont(InterFont(28, QFont::Bold));

  if (units.length() > 0) {
    p.save();
    x += 120;
    y -= 25;
    p.translate(x, y);
    p.rotate(-90);
    drawText(p, 0, 0, units);
    p.restore();
  }

  return 130;
}

void HudRendererSP::drawRightDevUI(QPainter &p, int x, int y) {
  int rh = 5;
  int ry = y;

  UiElement dRelElement = DeveloperUi::getDRel(lead_status, lead_d_rel);
  rh += drawRightDevUIElement(p, x, ry, dRelElement.value, dRelElement.label, dRelElement.units, dRelElement.color);
  ry = y + rh;

  UiElement vRelElement = DeveloperUi::getVRel(lead_status, lead_v_rel, is_metric, speedUnit);
  rh += drawRightDevUIElement(p, x, ry, vRelElement.value, vRelElement.label, vRelElement.units, vRelElement.color);
  ry = y + rh;

  UiElement steeringAngleDegElement = DeveloperUi::getSteeringAngleDeg(angleSteers, latActive, steerOverride);
  rh += drawRightDevUIElement(p, x, ry, steeringAngleDegElement.value, steeringAngleDegElement.label, steeringAngleDegElement.units, steeringAngleDegElement.color);
  ry = y + rh;

  UiElement actuatorsOutputLateralElement = DeveloperUi::getActuatorsOutputLateral(steerControlType, actuators, desiredCurvature, vEgo, roll, latActive, steerOverride);
  rh += drawRightDevUIElement(p, x, ry, actuatorsOutputLateralElement.value, actuatorsOutputLateralElement.label, actuatorsOutputLateralElement.units, actuatorsOutputLateralElement.color);
  ry = y + rh;

  UiElement actualLateralAccelElement = DeveloperUi::getActualLateralAccel(curvature, vEgo, roll, latActive, steerOverride);
  rh += drawRightDevUIElement(p, x, ry, actualLateralAccelElement.value, actualLateralAccelElement.label, actualLateralAccelElement.units, actualLateralAccelElement.color);
}

int HudRendererSP::drawBottomDevUIElement(QPainter &p, int x, int y, const QString &value, const QString &label, const QString &units, QColor &color) {
  p.setFont(InterFont(38, QFont::Bold));
  QFontMetrics fm(p.font());
  QRect init_rect = fm.boundingRect(label + " ");
  QRect real_rect = fm.boundingRect(init_rect, 0, label + " ");
  real_rect.moveCenter({x, y});

  QRect init_rect2 = fm.boundingRect(value);
  QRect real_rect2 = fm.boundingRect(init_rect2, 0, value);
  real_rect2.moveTop(real_rect.top());
  real_rect2.moveLeft(real_rect.right() + 10);

  QRect init_rect3 = fm.boundingRect(units);
  QRect real_rect3 = fm.boundingRect(init_rect3, 0, units);
  real_rect3.moveTop(real_rect.top());
  real_rect3.moveLeft(real_rect2.right() + 10);

  p.setPen(Qt::white);
  p.drawText(real_rect, Qt::AlignLeft | Qt::AlignVCenter, label);

  p.setPen(color);
  p.drawText(real_rect2, Qt::AlignRight | Qt::AlignVCenter, value);
  p.drawText(real_rect3, Qt::AlignLeft | Qt::AlignVCenter, units);
  return 430;
}

void HudRendererSP::drawBottomDevUI(QPainter &p, int x, int y) {
  int rw = 90;

  UiElement aEgoElement = DeveloperUi::getAEgo(aEgo);
  rw += drawBottomDevUIElement(p, rw, y, aEgoElement.value, aEgoElement.label, aEgoElement.units, aEgoElement.color);

  UiElement vEgoLeadElement = DeveloperUi::getVEgoLead(lead_status, lead_v_rel, vEgo, is_metric, speedUnit);
  rw += drawBottomDevUIElement(p, rw, y, vEgoLeadElement.value, vEgoLeadElement.label, vEgoLeadElement.units, vEgoLeadElement.color);

  if (torqueLateral && torquedUseParams) {
    UiElement frictionCoefficientFilteredElement = DeveloperUi::getFrictionCoefficientFiltered(frictionCoefficientFiltered, liveValid);
    rw += drawBottomDevUIElement(p, rw, y, frictionCoefficientFilteredElement.value, frictionCoefficientFilteredElement.label, frictionCoefficientFilteredElement.units, frictionCoefficientFilteredElement.color);

    UiElement latAccelFactorFilteredElement = DeveloperUi::getLatAccelFactorFiltered(latAccelFactorFiltered, liveValid);
    rw += drawBottomDevUIElement(p, rw, y, latAccelFactorFilteredElement.value, latAccelFactorFilteredElement.label, latAccelFactorFilteredElement.units, latAccelFactorFilteredElement.color);
  } else {
    UiElement steeringTorqueEpsElement = DeveloperUi::getSteeringTorqueEps(steeringTorqueEps);
    rw += drawBottomDevUIElement(p, rw, y, steeringTorqueEpsElement.value, steeringTorqueEpsElement.label, steeringTorqueEpsElement.units, steeringTorqueEpsElement.color);

    UiElement bearingDegElement = DeveloperUi::getBearingDeg(bearingAccuracyDeg, bearingDeg);
    rw += drawBottomDevUIElement(p, rw, y, bearingDegElement.value, bearingDegElement.label, bearingDegElement.units, bearingDegElement.color);
  }

  UiElement altitudeElement = DeveloperUi::getAltitude(gpsAccuracy, altitude);
  rw += drawBottomDevUIElement(p, rw, y, altitudeElement.value, altitudeElement.label, altitudeElement.units, altitudeElement.color);
}

void HudRendererSP::drawSpeedLimitSigns(QPainter &p, QRect &sign_rect) {
  bool speedLimitWarningEnabled = speedLimitMode >= SpeedLimitMode::WARNING;  // TODO-SP: update to include SpeedLimitMode::ASSIST
  bool hasSpeedLimit = speedLimitValid || speedLimitLastValid;
  bool overspeed = hasSpeedLimit && std::nearbyint(speedLimitFinalLast) < std::nearbyint(speed);
  QString speedLimitStr = hasSpeedLimit ? QString::number(std::nearbyint(speedLimitLast)) : "---";

  // Offset display text
  QString speedLimitSubText = "";
  if (speedLimitOffset != 0) {
    speedLimitSubText = (speedLimitOffset > 0 ? "" : "-") + QString::number(std::nearbyint(speedLimitOffset));
  }

  float speedLimitSubTextFactor = is_metric ? 0.5 : 0.6;
  if (speedLimitSubText.size() >= 3) {
    speedLimitSubTextFactor = 0.475;
  }

  int alpha = 255;
  QColor red_color = QColor(255, 0, 0, alpha);
  QColor speed_color = (speedLimitWarningEnabled && overspeed) ? red_color :
                       (!speedLimitValid && speedLimitLastValid ? QColor(0x91, 0x9b, 0x95, 0xf1) : QColor(0, 0, 0, alpha));

  if (is_metric) {
    // EU Vienna Convention style circular sign
    QRect vienna_rect = sign_rect;
    int circle_size = std::min(vienna_rect.width(), vienna_rect.height());
    QRect circle_rect(vienna_rect.x(), vienna_rect.y(), circle_size, circle_size);

    if (vienna_rect.width() > vienna_rect.height()) {
      circle_rect.moveLeft(vienna_rect.x() + (vienna_rect.width() - circle_size) / 2);
    } else if (vienna_rect.height() > vienna_rect.width()) {
      circle_rect.moveTop(vienna_rect.y() + (vienna_rect.height() - circle_size) / 2);
    }

    // White background circle
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(255, 255, 255, alpha));
    p.drawEllipse(circle_rect);

    // Red border ring with color coding
    QRect red_ring = circle_rect;

    p.setBrush(red_color);
    p.drawEllipse(red_ring);

    // Center white circle for text
    int ring_size = circle_size * 0.12;
    QRect center_circle = red_ring.adjusted(ring_size, ring_size, -ring_size, -ring_size);
    p.setBrush(QColor(255, 255, 255, alpha));
    p.drawEllipse(center_circle);

    // Speed value, smaller font for 3+ digits
    int font_size = (speedLimitStr.size() >= 3) ? 70 : 85;
    p.setFont(InterFont(font_size, QFont::Bold));

    p.setPen(speed_color);
    p.drawText(center_circle, Qt::AlignCenter, speedLimitStr);

    // Offset value in small circular box
    if (!speedLimitSubText.isEmpty() && hasSpeedLimit) {
      int offset_circle_size = circle_size * 0.4;
      int overlap = offset_circle_size * 0.25;
      QRect offset_circle_rect(
        circle_rect.right() - offset_circle_size/1.25 + overlap,
        circle_rect.top() - offset_circle_size/1.75 + overlap,
        offset_circle_size,
        offset_circle_size
      );

      p.setPen(QPen(QColor(77, 77, 77, 255), 6));
      p.setBrush(QColor(0, 0, 0, alpha));
      p.drawEllipse(offset_circle_rect);

      p.setFont(InterFont(offset_circle_size * speedLimitSubTextFactor, QFont::Bold));
      p.setPen(QColor(255, 255, 255, alpha));
      p.drawText(offset_circle_rect, Qt::AlignCenter, speedLimitSubText);
    }
  } else {
    // US/Canada MUTCD style sign
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(255, 255, 255, alpha));
    p.drawRoundedRect(sign_rect, 32, 32);

    // Inner border with violation color coding
    QRect inner_rect = sign_rect.adjusted(10, 10, -10, -10);
    QColor border_color = QColor(0, 0, 0, alpha);

    p.setPen(QPen(border_color, 4));
    p.setBrush(QColor(255, 255, 255, alpha));
    p.drawRoundedRect(inner_rect, 22, 22);

    // "SPEED LIMIT" text
    p.setFont(InterFont(40, QFont::DemiBold));
    p.setPen(QColor(0, 0, 0, alpha));
    p.drawText(inner_rect.adjusted(0, 10, 0, 0), Qt::AlignTop | Qt::AlignHCenter, tr("SPEED"));
    p.drawText(inner_rect.adjusted(0, 50, 0, 0), Qt::AlignTop | Qt::AlignHCenter, tr("LIMIT"));

    // Speed value with color coding
    p.setFont(InterFont(90, QFont::Bold));

    p.setPen(speed_color);
    p.drawText(inner_rect.adjusted(0, 80, 0, 0), Qt::AlignTop | Qt::AlignHCenter, speedLimitStr);

    // Offset value in small box
    if (!speedLimitSubText.isEmpty() && hasSpeedLimit) {
      int offset_box_size = sign_rect.width() * 0.4;
      int overlap = offset_box_size * 0.25;
      QRect offset_box_rect(
        sign_rect.right() - offset_box_size/1.5 + overlap,
        sign_rect.top() - offset_box_size/1.25 + overlap,
        offset_box_size,
        offset_box_size
      );

      int corner_radius = offset_box_size * 0.2;
      p.setPen(QPen(QColor(77, 77, 77, 255), 6));
      p.setBrush(QColor(0, 0, 0, alpha));
      p.drawRoundedRect(offset_box_rect, corner_radius, corner_radius);

      p.setFont(InterFont(offset_box_size * speedLimitSubTextFactor, QFont::Bold));
      p.setPen(QColor(255, 255, 255, alpha));
      p.drawText(offset_box_rect, Qt::AlignCenter, speedLimitSubText);
    }
  }
}

void HudRendererSP::drawUpcomingSpeedLimit(QPainter &p) {
  bool speed_limit_ahead = speedLimitAheadValid && speedLimitAhead > 0 && speedLimitAhead != speedLimit && speedLimitAheadValidFrame > 0 &&
                           speedLimitSource == cereal::LongitudinalPlanSP::SpeedLimit::Source::MAP;
  if (!speed_limit_ahead) {
    return;
  }

  auto roundToInterval = [&](float distance, int interval, int threshold) {
    int base = static_cast<int>(distance / interval) * interval;
    return (distance - base >= threshold) ? base + interval : base;
  };

  auto outputDistance = [&] {
    if (is_metric) {
      if (speedLimitAheadDistance < 50) return tr("Near");
      if (speedLimitAheadDistance >= 1000) return QString::number(speedLimitAheadDistance * METER_TO_KM, 'f', 1) + tr("km");

      int rounded = (speedLimitAheadDistance < 200) ? std::max(10, roundToInterval(speedLimitAheadDistance, 10, 5)) : roundToInterval(speedLimitAheadDistance, 100, 50);
      return QString::number(rounded) + tr("m");
    } else {
      float distance_ft = speedLimitAheadDistance * METER_TO_FOOT;
      if (distance_ft < 100) return tr("Near");
      if (distance_ft >= 900) return QString::number(speedLimitAheadDistance * METER_TO_MILE, 'f', 1) + tr("mi");

      int rounded = (distance_ft < 500) ? std::max(50, roundToInterval(distance_ft, 50, 25)) : roundToInterval(distance_ft, 100, 50);
      return QString::number(rounded) + tr("ft");
    }
  };

  QString speedStr = QString::number(std::nearbyint(speedLimitAhead));
  QString distanceStr = outputDistance();

  // Position below current speed limit sign
  const int sign_width = is_metric ? 200 : 172;
  const int sign_x = is_metric ? 280 : 272;
  const int sign_y = 45;
  const int sign_height = 204;

  const int ahead_width = 170;
  const int ahead_height = 160;
  const int ahead_x = sign_x + (sign_width - ahead_width) / 2;
  const int ahead_y = sign_y + sign_height + 10;

  QRect ahead_rect(ahead_x, ahead_y, ahead_width, ahead_height);
  p.setPen(QPen(QColor(255, 255, 255, 100), 3));
  p.setBrush(QColor(0, 0, 0, 180));
  p.drawRoundedRect(ahead_rect, 16, 16);

  // "AHEAD" label
  p.setFont(InterFont(40, QFont::DemiBold));
  p.setPen(QColor(200, 200, 200, 255));
  p.drawText(ahead_rect.adjusted(0, 4, 0, 0), Qt::AlignTop | Qt::AlignHCenter, tr("AHEAD"));

  // Speed value
  p.setFont(InterFont(70, QFont::Bold));
  p.setPen(QColor(255, 255, 255, 255));
  p.drawText(ahead_rect.adjusted(0, 38, 0, 0), Qt::AlignTop | Qt::AlignHCenter, speedStr);

  // Distance
  p.setFont(InterFont(40, QFont::Normal));
  p.setPen(QColor(180, 180, 180, 255));
  p.drawText(ahead_rect.adjusted(0, 110, 0, 0), Qt::AlignTop | Qt::AlignHCenter, distanceStr);
}

void HudRendererSP::drawRoadName(QPainter &p, const QRect &surface_rect) {
  if (!roadName || roadNameStr.isEmpty()) return;

  // When carrotMan is alive, suppress generic/empty road names at the top
  // since the carrot panel handles road display
  if (carrotManAlive) return;

  // Measure text to size container
  p.setFont(InterFont(46, QFont::DemiBold));
  QFontMetrics fm(p.font());

  int text_width = fm.horizontalAdvance(roadNameStr);
  int padding = 40;
  int rect_width = text_width + padding;

  // Constrain to reasonable bounds
  int min_width = 200;
  int max_width = surface_rect.width() - 40;
  rect_width = std::max(min_width, std::min(rect_width, max_width));

  // Center at top of screen
  QRect road_rect(surface_rect.width() / 2 - rect_width / 2, -4, rect_width, 60);

  p.setPen(Qt::NoPen);
  p.setBrush(QColor(0, 0, 0, 120));
  p.drawRoundedRect(road_rect, 12, 12);

  p.setPen(QColor(255, 255, 255, 200));

  // Truncate if still too long
  QString truncated = fm.elidedText(roadNameStr, Qt::ElideRight, road_rect.width() - 20);
  p.drawText(road_rect, Qt::AlignCenter, truncated);
}

void HudRendererSP::drawSpeedLimitPreActiveArrow(QPainter &p, QRect &sign_rect) {
  const int sign_margin = 12;
  const int arrow_spacing = sign_margin * 1.4;
  int arrow_x = sign_rect.right() + arrow_spacing;

  int _set_speed = std::nearbyint(set_speed);
  int _speed_limit_final_last = std::nearbyint(speedLimitFinalLast);

  // Calculate the vertical offset using a sinusoidal function for smooth bouncing
  double bounce_frequency = 2.0 * M_PI / UI_FREQ;  // 20 frames for one full oscillation
  int bounce_offset = 20 * sin(speedLimitAssistFrame * bounce_frequency);  // Adjust the amplitude (20 pixels) as needed

  if (_set_speed < _speed_limit_final_last) {
    QPoint iconPosition(arrow_x, sign_rect.center().y() - plus_arrow_up_img.height() / 2 + bounce_offset);
    p.drawPixmap(iconPosition, plus_arrow_up_img);
  } else if (_set_speed > _speed_limit_final_last) {
    QPoint iconPosition(arrow_x, sign_rect.center().y() - minus_arrow_down_img.height() / 2 - bounce_offset);
    p.drawPixmap(iconPosition, minus_arrow_down_img);
  }
}

void HudRendererSP::drawSetSpeedSP(QPainter &p, const QRect &surface_rect) {
  // Draw outer box + border to contain set speed
  const QSize default_size = {172, 204};
  QSize set_speed_size = is_metric ? QSize(200, 204) : default_size;
  QRect set_speed_rect(QPoint(60 + (default_size.width() - set_speed_size.width()) / 2, 45), set_speed_size);

  // Draw set speed box
  p.setPen(QPen(QColor(255, 255, 255, 75), 6));
  p.setBrush(QColor(0, 0, 0, 166));
  p.drawRoundedRect(set_speed_rect, 32, 32);

  // Colors based on status
  QColor max_color = QColor(0xa6, 0xa6, 0xa6, 0xff);
  QColor set_speed_color = QColor(0x72, 0x72, 0x72, 0xff);
  if (is_cruise_set) {
    set_speed_color = QColor(255, 255, 255);
    if (speedLimitAssistActive) {
      set_speed_color = longOverride ? QColor(0x91, 0x9b, 0x95, 0xff) : QColor(0, 0xff, 0, 0xff);
      max_color = longOverride ? QColor(0x91, 0x9b, 0x95, 0xff) : QColor(0x80, 0xd8, 0xa6, 0xff);
    } else {
      if (status == STATUS_DISENGAGED) {
        max_color = QColor(255, 255, 255);
      } else if (status == STATUS_OVERRIDE) {
        max_color = QColor(0x91, 0x9b, 0x95, 0xff);
      } else {
        max_color = QColor(0x80, 0xd8, 0xa6, 0xff);
      }
    }
  }

  // Draw "MAX" or carState.cruiseState.speedCluster (when ICBM is active) text
  if (!pcmCruiseSpeed && carControlEnabled) {
    if (std::nearbyint(set_speed) != std::nearbyint(speedCluster)) {
      icbm_active_counter = 3 * UI_FREQ;
    } else if (icbm_active_counter > 0) {
      icbm_active_counter--;
    }
  } else {
    icbm_active_counter = 0;
  }
  int max_str_size = (icbm_active_counter != 0) ? 60 : 40;
  int max_str_y = (icbm_active_counter != 0) ? 15 : 27;
  QString max_str = (icbm_active_counter != 0) ? QString::number(std::nearbyint(speedCluster)) : tr("MAX");

  p.setFont(InterFont(max_str_size, QFont::DemiBold));
  p.setPen(max_color);
  p.drawText(set_speed_rect.adjusted(0, max_str_y, 0, 0), Qt::AlignTop | Qt::AlignHCenter, max_str);

  // Draw set speed
  QString setSpeedStr = is_cruise_set ? QString::number(std::nearbyint(set_speed)) : "–";
  p.setFont(InterFont(90, QFont::Bold));
  p.setPen(set_speed_color);
  p.drawText(set_speed_rect.adjusted(0, 77, 0, 0), Qt::AlignTop | Qt::AlignHCenter, setSpeedStr);
}

void HudRendererSP::drawE2eAlert(QPainter &p, const QRect &surface_rect, const QString &alert_alt_text) {
  if (!allow_e2e_alerts) return;

  int x = surface_rect.right() - e2e_alert_size - (devUiInfo > 0 ? 180 : 100) - (UI_BORDER_SIZE * 3);
  int y = surface_rect.center().y() + 20;
  QRect alertRect(x - e2e_alert_size, y - e2e_alert_size, e2e_alert_size * 2, e2e_alert_size * 2);

  // Alert Circle
  QPoint center = alertRect.center();
  QColor frameColor;
  if (!alert_alt_text.isEmpty()) frameColor = QColor(255, 255, 255, 75);
  else frameColor = pulseElement(e2eAlertFrame) ? QColor(255, 255, 255, 75) : QColor(0, 255, 0, 75);
  p.setPen(QPen(frameColor, 15));
  p.setBrush(QColor(0, 0, 0, 190));
  p.drawEllipse(center, e2e_alert_size, e2e_alert_size);

  // Alert Text
  QColor txtColor;
  QFont font;
  int alert_bottom_adjustment;
  if (!alert_alt_text.isEmpty()) {
    font = InterFont(100, QFont::Bold);
    alert_bottom_adjustment = 5;
    txtColor = QColor(255, 255, 255, 255);
  } else {
    font = InterFont(48, QFont::Bold);
    alert_bottom_adjustment = 7;
    txtColor = pulseElement(e2eAlertFrame) ? QColor(255, 255, 255, 255) : QColor(0, 255, 0, 190);
  }
  p.setPen(txtColor);
  p.setFont(font);
  QFontMetrics fm(p.font());
  QRect textRect = fm.boundingRect(alertRect, Qt::TextWordWrap, alert_text);
  textRect.moveCenter({alertRect.center().x(), alertRect.center().y()});
  textRect.moveBottom(alertRect.bottom() - alertRect.height() / alert_bottom_adjustment);
  p.drawText(textRect, Qt::AlignCenter, alert_text);

  if (!alert_alt_text.isEmpty()) {
    // Alert Alternate Text
    p.setFont(InterFont(80, QFont::Bold));
    p.setPen(QColor(255, 175, 3, 240));
    QFontMetrics fmt(p.font());
    QRect topTextRect = fmt.boundingRect(alertRect, Qt::TextWordWrap, alert_alt_text);
    topTextRect.moveCenter({alertRect.center().x(), alertRect.center().y()});
    topTextRect.moveTop(alertRect.top() + alertRect.height() / 3.5);
    p.drawText(topTextRect, Qt::AlignCenter, alert_alt_text);
  } else {
    // Alert Image instead of Top Text
    QPointF pixmapCenterOffset = QPointF(alert_img.width() / 2.0, alert_img.height() / 2.0);
    QPointF drawPoint = center - pixmapCenterOffset;
    p.drawPixmap(drawPoint, alert_img);
  }
}

void HudRendererSP::drawCurrentSpeedSP(QPainter &p, const QRect &surface_rect) {
  QString speedStr = QString::number(std::nearbyint(speed));

  p.setFont(InterFont(176, QFont::Bold));
  HudRenderer::drawText(p, surface_rect.center().x(), 210, speedStr);

  p.setFont(InterFont(66));
  HudRenderer::drawText(p, surface_rect.center().x(), 290, is_metric ? tr("km/h") : tr("mph"), 200);
}

void HudRendererSP::drawBlinker(QPainter &p, const QRect &surface_rect) {
  const bool hazard = leftBlinkerOn && rightBlinkerOn;
  int blinkerStatus = hazard ? 2 : (leftBlinkerOn || rightBlinkerOn) ? 1 : 0;

  if (!leftBlinkerOn && !rightBlinkerOn) {
    blinkerFrameCounter = 0;
    lastBlinkerStatus = 0;
    return;
  }

  if (blinkerStatus != lastBlinkerStatus) {
    blinkerFrameCounter = 0;
    lastBlinkerStatus = blinkerStatus;
  }

  ++blinkerFrameCounter;

  const int BLINKER_COOLDOWN_FRAMES = UI_FREQ / 10;
  if (blinkerFrameCounter < BLINKER_COOLDOWN_FRAMES) {
    return;
  }

  const int circleRadius = 60;
  const int arrowLength = 60;
  const int x_gap = 160;
  const int y_offset = 272;

  const int centerX = surface_rect.center().x();

  const QPen bgBorder(Qt::white, 5);
  const QPen arrowPen(Qt::NoPen);

  p.save();

  auto drawArrow = [&](int cx, int cy, int dir, const QBrush &arrowBrush) {
    const int bodyLength = arrowLength / 2;
    const int bodyWidth = arrowLength / 2;
    const int headLength = arrowLength / 2;
    const int headWidth = arrowLength;

    QPolygon arrow;
    arrow.reserve(7);
    arrow << QPoint(cx - dir * bodyLength, cy - bodyWidth / 2)
        << QPoint(cx, cy - bodyWidth / 2)
        << QPoint(cx, cy - headWidth / 2)
        << QPoint(cx + dir * headLength, cy)
        << QPoint(cx, cy + headWidth / 2)
        << QPoint(cx, cy + bodyWidth / 2)
        << QPoint(cx - dir * bodyLength, cy + bodyWidth / 2);

    p.setPen(arrowPen);
    p.setBrush(arrowBrush);
    p.drawPolygon(arrow);
  };

  auto drawCircle = [&](int cx, int cy, const QBrush &bgBrush) {
    p.setPen(bgBorder);
    p.setBrush(bgBrush);
    p.drawEllipse(QPoint(cx, cy), circleRadius, circleRadius);
  };

  struct BlinkerSide { bool on; int dir; bool blocked; int cx; };
  const std::array<BlinkerSide, 2> sides = {{
    {leftBlinkerOn, -1, hazard ? true : (leftBlinkerOn  && leftBlindspot), centerX - x_gap},
    {rightBlinkerOn, 1, hazard ? true : (rightBlinkerOn && rightBlindspot), centerX + x_gap},
  }};

  for (const auto &s: sides) {
    if (!s.on) continue;

    QColor bgColor = s.blocked ? QColor(135, 23, 23) : QColor(23, 134, 68);
    QColor arrowColor = s.blocked ? QColor(66, 12, 12) : QColor(12, 67, 34);
    if (pulseElement(blinkerFrameCounter)) arrowColor = Qt::white;

    const QBrush bgBrush(bgColor);
    const QBrush arrowBrush(arrowColor);

    drawCircle(s.cx, y_offset, bgBrush);
    drawArrow(s.cx, y_offset, s.dir, arrowBrush);
  }

  p.restore();
}

// ==================== CarrotMan Navigation Panel (Bottom-Right) ====================

void HudRendererSP::drawCarrotPanel(QPainter &p, const QRect &surface_rect) {
  // Show panel if we have any useful carrot data
  bool hasDest = (carrotDestDist > 0 && carrotDestTime > 0);
  bool hasGoalName = !carrotGoalName.isEmpty();
  bool hasTurn = (carrotTurnInfo > 0);
  bool hasCamera = (carrotSpdLimit > 0 && carrotSpdDist > 0);
  bool hasTraffic = (carrotTrafficState > 0);
  bool hasRoad = !carrotRoadName.isEmpty();
  bool hasSdi = !carrotSdiDescr.isEmpty();
  bool hasRoadLimit = (carrotNaviSpeedLimit > 0);
  bool hasApply = (carrotDesiredSpeed > 0 && !carrotDesiredSource.isEmpty());
  if (!hasDest && !hasGoalName && !hasTurn && !hasCamera && !hasTraffic && !hasRoad && !hasSdi && !hasRoadLimit && !hasApply) return;

  p.save();
  p.setRenderHint(QPainter::Antialiasing, true);

  // --- Panel position: left or right side ---
  const int panelW = 790;
  const int panelH = 240;
  int panelX = (carrotPanelSide == 1) ? (surface_rect.width() - panelW + 11) : -11;
  int panelY = surface_rect.height() - panelH - 59;

  // Draw panel background (semi-transparent Mediterranean blue)
  p.setPen(QPen(QColor(255, 255, 255, 60), 1));
  p.setBrush(QColor(0, 105, 148, 100));
  p.drawRoundedRect(QRect(panelX, panelY - 60, panelW, panelH + 60), 30, 30);

  // ========== TBT Main Text (above main content) ==========
  if (!carrotTBTText.isEmpty()) {
    p.setFont(InterFont(40, QFont::Bold));
    p.setPen(Qt::white);
    QFontMetrics fm(p.font());
    QString tbtDisplay = carrotTBTText;
    // Append near direction name if available
    if (!carrotNearDirName.isEmpty()) {
      tbtDisplay += QString(" → %1").arg(carrotNearDirName);
    }
    QString truncTBT = fm.elidedText(tbtDisplay, Qt::ElideRight, panelW - 40);
    p.drawText(QRect(panelX + 20, panelY - 55, panelW - 40, 45), Qt::AlignLeft | Qt::AlignVCenter, truncTBT);
  }

  // ========== Turn Icon + Distance (left side) ==========
  if (carrotTurnInfo > 0) {
    int iconX = panelX + 20;
    int iconY = panelY + 20;
    int iconSz = 128;

    // Green background for turn icon area when atcType is set
    if (!carrotAtcType.isEmpty()) {
      QColor greenBg = carrotAtcType.contains("prepare") ? QColor(0, 180, 0, 100) : QColor(0, 180, 0, 255);
      p.setPen(QPen(Qt::black, 1));
      p.setBrush(greenBg);
      p.drawRoundedRect(QRect(iconX - 16, iconY - 26, 160, 230), 15, 15);
    }

    // Draw turn icon (PNG)
    QPixmap *icon = nullptr;
    switch (carrotTurnInfo) {
      case 1: icon = &turn_l_img; break;
      case 2: icon = &turn_r_img; break;
      case 3: icon = &lane_change_l_img; break;
      case 4: icon = &lane_change_r_img; break;
      case 7: icon = &turn_u_img; break;
      default: break;
    }

    if (icon && !icon->isNull()) {
      p.drawPixmap(iconX, iconY, iconSz, iconSz, *icon);
    } else {
      // Fallback text for turn types without icons
      p.setFont(InterFont(35, QFont::Bold));
      p.setPen(Qt::white);
      QString turnLabel;
      if (carrotTurnInfo == 6) turnLabel = "TG";
      else if (carrotTurnInfo == 8) turnLabel = tr("目的地");
      else turnLabel = QString(tr("减速:%1")).arg(carrotTurnInfo);
      p.drawText(QRect(iconX, iconY, iconSz, iconSz), Qt::AlignCenter, turnLabel);
    }

    // v-Turn speed (curve advisory speed) below turn icon
    // Only show when it's a meaningful advisory (below 120 km/h, meaning actual curve ahead)
    if (carrotVTurnSpeed > 0 && carrotVTurnSpeed < 120) {
      p.setFont(InterFont(34, QFont::Bold));
      p.setPen(QColor(255, 200, 50));
      p.drawText(QRect(iconX - 16, iconY + iconSz + 4, 160, 32),
                 Qt::AlignCenter, QString("%1km/h").arg(carrotVTurnSpeed));
    }

    // Turn distance below icon
    QString turnDist;
    if (carrotDistToTurn > 0) {
      if (is_metric) {
        if (carrotDistToTurn < 1000)
          turnDist = QString::number(carrotDistToTurn) + " m";
        else
          turnDist = QString::number(carrotDistToTurn / 1000.0, 'f', 1) + " km";
      } else {
        if (carrotDistToTurn < 1609)
          turnDist = QString::number((int)(carrotDistToTurn * 3.28084)) + " ft";
        else
          turnDist = QString::number(carrotDistToTurn / 1609.344, 'f', 1) + " mi";
      }
      p.setFont(InterFont(40, QFont::Bold));
      p.setPen(Qt::white);
      p.drawText(QRect(iconX - 16, iconY + iconSz + 10, 160, 45),
                 Qt::AlignCenter, turnDist);

    // Turn countdown (seconds)
    if (carrotTurnCountDown > 0) {
      p.setFont(InterFont(30, QFont::Bold));
      p.setPen(QColor(255, 220, 100));
      p.drawText(QRect(iconX - 16, iconY + iconSz + 55, 160, 35),
                 Qt::AlignCenter, QString("%1s").arg(carrotTurnCountDown));
    }
    }
  }

  // ========== ETA + Destination Info (right side) ==========
  int infoX = panelX + 190;
  int infoY = panelY + 15;

  if (hasDest) {
  // ETA: arrival time calculation
  {
    time_t now = time(NULL);
    struct tm localTime;
    localtime_r(&now, &localTime);
    int remaining_minutes = carrotDestTime / 60;
    localTime.tm_min += remaining_minutes;
    mktime(&localTime);

    QString etaStr;
    if (remaining_minutes >= 60) {
      int hours = remaining_minutes / 60;
      int mins = remaining_minutes % 60;
      etaStr = QString("ETA: %1h%2m(%3:%4)")
                 .arg(hours).arg(mins, 2, 10, QChar('0'))
                 .arg(localTime.tm_hour, 2, 10, QChar('0'))
                 .arg(localTime.tm_min, 2, 10, QChar('0'));
    } else {
      etaStr = QString("ETA: %1min(%2:%3)")
                 .arg(remaining_minutes)
                 .arg(localTime.tm_hour, 2, 10, QChar('0'))
                 .arg(localTime.tm_min, 2, 10, QChar('0'));
    }

    p.setFont(InterFont(50, QFont::Bold));
    p.setPen(Qt::white);
    p.drawText(QRect(infoX, infoY, panelW - 200, 55), Qt::AlignLeft | Qt::AlignVCenter, etaStr);
  }

  // Destination distance + Goal Name (same line)
  {
    QString destDist;
    if (is_metric) {
      destDist = QString::number(carrotDestDist / 1000.0, 'f', 1) + " km";
    } else {
      destDist = QString::number(carrotDestDist / 1000.0 * 0.621371, 'f', 1) + " mi";
    }

    // Combine: "12.5 km  🏁 目的地名"
    if (hasGoalName) {
      destDist += QString("  \U0001F3C1 ") + carrotGoalName;
    }

    p.setFont(InterFont(40, QFont::Bold));
    p.setPen(Qt::white);
    QFontMetrics fmDest(p.font());
    QString truncDest = fmDest.elidedText(destDist, Qt::ElideRight, panelW - 210);
    p.drawText(QRect(infoX, infoY + 55, panelW - 210, 48),
               Qt::AlignLeft | Qt::AlignVCenter, truncDest);
  }
  } // hasDest

  // ========== Goal Name only (no destination) ==========
  if (!hasDest && hasGoalName) {
    p.setFont(InterFont(36, QFont::DemiBold));
    p.setPen(QColor(180, 230, 255));
    QFontMetrics fmGoal(p.font());
    QString truncGoal = fmGoal.elidedText(QString("\U0001F3C1 ") + carrotGoalName, Qt::ElideRight, panelW - 210);
    p.drawText(QRect(infoX, infoY, panelW - 210, 42),
               Qt::AlignLeft | Qt::AlignVCenter, truncGoal);
  }

  // ========== SDI Description (green background) or Road Name ==========
  int bottomY = panelY + 190;
  if (!carrotSdiDescr.isEmpty()) {
    p.setFont(InterFont(40, QFont::Bold));
    QFontMetrics fm(p.font());
    int textW = fm.horizontalAdvance(carrotSdiDescr);
    int textH = fm.height();
    int sdiX = infoX;
    int sdiY = bottomY;

    // Green background behind SDI text
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0, 180, 0));
    p.drawRoundedRect(QRect(sdiX - 10, sdiY - 2, textW + 20, textH + 6), 10, 10);

    p.setPen(Qt::white);
    p.drawText(QRect(sdiX, sdiY, textW + 4, textH + 4), Qt::AlignLeft | Qt::AlignVCenter, carrotSdiDescr);
  } else if (!carrotRoadName.isEmpty()) {
    // Road category label
    QString roadCateLabel;
    switch (carrotRoadCate) {
      case 0: roadCateLabel = ""; break;
      case 1: roadCateLabel = tr("高速"); break;
      case 2: roadCateLabel = tr("城快"); break;
      case 3: roadCateLabel = tr("国道"); break;
      case 4: roadCateLabel = tr("省道"); break;
      case 5: roadCateLabel = tr("县道"); break;
      case 6: roadCateLabel = tr("乡道"); break;
      default: roadCateLabel = ""; break;
    }

    int cateW = 0;
    if (!roadCateLabel.isEmpty()) {
      p.setFont(InterFont(28, QFont::Bold));
      QFontMetrics fmCate(p.font());
      cateW = fmCate.horizontalAdvance(roadCateLabel) + 16;
      // Category tag with color
      QColor cateBg;
      if (carrotRoadCate == 1) cateBg = QColor(0, 130, 0, 200);
      else if (carrotRoadCate == 2) cateBg = QColor(200, 130, 0, 200);
      else cateBg = QColor(100, 100, 100, 200);
      p.setPen(Qt::NoPen);
      p.setBrush(cateBg);
      p.drawRoundedRect(QRect(infoX, bottomY + 2, cateW, 30), 8, 8);
      p.setPen(Qt::white);
      p.drawText(QRect(infoX, bottomY + 2, cateW, 30), Qt::AlignCenter, roadCateLabel);
    }

    p.setFont(InterFont(40, QFont::Bold));
    p.setPen(Qt::white);
    QFontMetrics fm(p.font());
    int roadNameX = infoX + cateW + (cateW > 0 ? 8 : 0);
    QString truncRoad = fm.elidedText(carrotRoadName, Qt::ElideRight, panelW - 210 - cateW);
    p.drawText(QRect(roadNameX, bottomY, panelW - 210 - cateW, fm.height() + 4),
               Qt::AlignLeft | Qt::AlignVCenter, truncRoad);
  }

  // ========== Road Speed Limit + Desired Speed (middle-right area) ==========
  if (hasRoadLimit || hasApply) {
    int rsY = panelY + 140;

    // Current road speed limit (LIMIT box)
    if (hasRoadLimit) {
      bool overLimit = (vEgo * 3.6 > carrotNaviSpeedLimit + 2);
      QColor limitBg = overLimit ? QColor(255, 50, 50, 210) : QColor(255, 255, 255, 210);
      QColor limitText = overLimit ? Qt::white : Qt::black;

      // Label
      p.setFont(InterFont(24, QFont::Bold));
      p.setPen(Qt::white);
      p.drawText(QRect(infoX, rsY - 28, 90, 24), Qt::AlignCenter, "LIMIT");

      // Speed box
      p.setPen(QPen(Qt::black, 2));
      p.setBrush(limitBg);
      p.drawRoundedRect(QRect(infoX, rsY, 90, 42), 12, 12);
      p.setFont(InterFont(36, QFont::Bold));
      p.setPen(limitText);
      p.drawText(QRect(infoX, rsY, 90, 42), Qt::AlignCenter, QString::number(carrotNaviSpeedLimit));
    }

    // Desired/applied speed + source
    if (hasApply) {
      int applyX = hasRoadLimit ? infoX + 110 : infoX;
      p.setFont(InterFont(24, QFont::Bold));
      p.setPen(QColor(255, 180, 50));
      QFontMetrics fmSrc(p.font());
      QString truncSrc = fmSrc.elidedText(carrotDesiredSource, Qt::ElideRight, 120);
      p.drawText(QRect(applyX, rsY - 28, 120, 24), Qt::AlignCenter, truncSrc);

      QColor applyBg(255, 180, 50, 210);
      p.setPen(QPen(Qt::black, 2));
      p.setBrush(applyBg);
      p.drawRoundedRect(QRect(applyX, rsY, 90, 42), 12, 12);
      p.setFont(InterFont(36, QFont::Bold));
      p.setPen(Qt::white);
      p.drawText(QRect(applyX, rsY, 90, 42), Qt::AlignCenter, QString::number(carrotDesiredSpeed));
    }
  }

  // ========== Speed Camera Info (middle area, if present) ==========
  if (carrotSpdLimit > 0 && carrotSpdDist > 0) {
    int camY = panelY + 190;

    // Speed limit circle
    int circleR = 35;
    int circleCx = infoX + circleR;
    int circleCy = camY + circleR;

    bool overspeed = std::nearbyint(speed) > carrotSpdLimit;

    // Red circle border
    p.setPen(QPen(overspeed ? QColor(255, 50, 50) : QColor(255, 80, 80), 4));
    p.setBrush(QColor(255, 255, 255, 200));
    p.drawEllipse(QPoint(circleCx, circleCy), circleR, circleR);

    // Speed limit number
    p.setFont(InterFont(40, QFont::Bold));
    p.setPen(overspeed ? QColor(255, 50, 50) : Qt::black);
    p.drawText(QRect(circleCx - circleR, circleCy - circleR, circleR * 2, circleR * 2),
               Qt::AlignCenter, QString::number(carrotSpdLimit));

    // Camera distance
    QString camDist;
    if (carrotSpdDist >= 1000)
      camDist = QString::number(carrotSpdDist / 1000.0, 'f', 1) + " km";
    else
      camDist = QString::number(carrotSpdDist) + " m";

    if (carrotSpdCountDown > 0)
      camDist += QString(" %1s").arg(carrotSpdCountDown);

    p.setFont(InterFont(36, QFont::Bold));
    p.setPen(Qt::white);
    p.drawText(QRect(circleCx + circleR + 15, circleCy - 20, panelW - 300, 40),
               Qt::AlignLeft | Qt::AlignVCenter, camDist);

    // Camera type text
    QString typeLabel;
    if (carrotSpdType == 2 || carrotSpdType == 4) typeLabel = tr("区间测速");
    else if (carrotSpdType == 22) typeLabel = tr("减速带");
    else if (carrotSpdType == 100) typeLabel = tr("移动测速");

    if (!typeLabel.isEmpty()) {
      p.setFont(InterFont(26, QFont::Normal));
      p.setPen(QColor(255, 200, 50));
      p.drawText(QRect(circleCx + circleR + 15, circleCy + 18, panelW - 300, 30),
                 Qt::AlignLeft | Qt::AlignVCenter, typeLabel);
    }
  }

  // ========== Traffic Light (compact, next to speed camera area) ==========
  if (carrotTrafficState > 0) {
    int tlY = (carrotSpdLimit > 0 && carrotSpdDist > 0) ? panelY + 190 + 80 : panelY + 190;

    QColor lightColor;
    QString lightText;
    switch (carrotTrafficState) {
      case 1: lightColor = QColor(255, 50, 50);  lightText = tr("红灯"); break;
      case 2: lightColor = QColor(50, 255, 50);  lightText = tr("绿灯"); break;
      case 3: lightColor = QColor(50, 255, 100); lightText = tr("左转绿灯"); break;
      default: lightColor = QColor(200, 200, 50); lightText = tr("信号灯"); break;
    }

    int lightR = 14;
    int lightCx = infoX + lightR;
    int lightCy = tlY + lightR;
    p.setPen(QPen(QColor(60, 60, 60), 2));
    p.setBrush(lightColor);
    p.drawEllipse(QPoint(lightCx, lightCy), lightR, lightR);

    p.setFont(InterFont(32, QFont::DemiBold));
    p.setPen(lightColor);
    p.drawText(QRect(lightCx + lightR + 10, tlY, 200, lightR * 2),
               Qt::AlignLeft | Qt::AlignVCenter, lightText);

    if (carrotTrafficCountdown > 0) {
      p.setFont(InterFont(30, QFont::Bold));
      p.setPen(Qt::white);
      p.drawText(QRect(lightCx + lightR + 10 + 140, tlY, 100, lightR * 2),
                 Qt::AlignLeft | Qt::AlignVCenter, QString("%1s").arg(carrotTrafficCountdown));
    } else if (carrotLeftSec > 0) {
      p.setFont(InterFont(30, QFont::Bold));
      p.setPen(Qt::white);
      p.drawText(QRect(lightCx + lightR + 10 + 140, tlY, 100, lightR * 2),
                 Qt::AlignLeft | Qt::AlignVCenter, QString("%1s").arg(carrotLeftSec));
    }
  }

  p.restore();
}
