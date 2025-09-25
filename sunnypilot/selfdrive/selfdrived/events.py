import cereal.messaging as messaging
from cereal import log, car, custom
from openpilot.common.constants import CV
from openpilot.sunnypilot.selfdrive.selfdrived.events_base import EventsBase, Priority, ET, Alert, \
  NoEntryAlert, ImmediateDisableAlert, EngagementAlert, NormalPermanentAlert, AlertCallbackType, wrong_car_mode_alert
from openpilot.sunnypilot.selfdrive.controls.lib.speed_limit import PCM_LONG_REQUIRED_MAX_SET_SPEED


AlertSize = log.SelfdriveState.AlertSize
AlertStatus = log.SelfdriveState.AlertStatus
VisualAlert = car.CarControl.HUDControl.VisualAlert
AudibleAlert = car.CarControl.HUDControl.AudibleAlert
EventNameSP = custom.OnroadEventSP.EventName


# get event name from enum
EVENT_NAME_SP = {v: k for k, v in EventNameSP.schema.enumerants.items()}


def speed_limit_adjust_alert(CP: car.CarParams, CS: car.CarState, sm: messaging.SubMaster, metric: bool, soft_disable_time: int, personality) -> Alert:
  speedLimit = sm['longitudinalPlanSP'].speedLimit.resolver.speedLimit
  speed = round(speedLimit * (CV.MS_TO_KPH if metric else CV.MS_TO_MPH))
  message = f'调整至 {speed} {"km/h" if metric else "mph"} 速度限制'
  return Alert(
    message,
    "",
    AlertStatus.normal, AlertSize.small,
    Priority.LOW, VisualAlert.none, AudibleAlert.none, 4.)


def speed_limit_pre_active_alert(CP: car.CarParams, CS: car.CarState, sm: messaging.SubMaster, metric: bool, soft_disable_time: int, personality) -> Alert:
  if CP.openpilotLongitudinalControl and CP.pcmCruise:
    # PCM long
    pcm_long_required_max_set_speed_conv = round(PCM_LONG_REQUIRED_MAX_SET_SPEED[metric] * (CV.MS_TO_KPH if metric else CV.MS_TO_MPH))
    speed_unit = "km/h" if metric else "mph"
    alert_2_str = f"手动将设定速度更改为 {pcm_long_required_max_set_speed_conv} {speed_unit} 以激活"
  else:
    # Non PCM long
    alert_2_str = "操作+/-巡航控制按钮以激活"

  return Alert(
    "限速辅助：需激活",
    alert_2_str,
    AlertStatus.normal, AlertSize.mid,
    Priority.LOW, VisualAlert.none, AudibleAlert.none, .1)


class EventsSP(EventsBase):
  def __init__(self):
    super().__init__()
    self.event_counters = dict.fromkeys(EVENTS_SP.keys(), 0)

  def get_events_mapping(self) -> dict[int, dict[str, Alert | AlertCallbackType]]:
    return EVENTS_SP

  def get_event_name(self, event: int):
    return EVENT_NAME_SP[event]

  def get_event_msg_type(self):
    return custom.OnroadEventSP.Event


EVENTS_SP: dict[int, dict[str, Alert | AlertCallbackType]] = {
  # sunnypilot
  EventNameSP.lkasEnable: {
    ET.ENABLE: EngagementAlert(AudibleAlert.engage),
  },

  EventNameSP.lkasDisable: {
    ET.USER_DISABLE: EngagementAlert(AudibleAlert.disengage),
  },

  EventNameSP.manualSteeringRequired: {
    ET.USER_DISABLE: Alert(
      "自动车道居中功能已关闭",
      "需要手动转向",
      AlertStatus.normal, AlertSize.mid,
      Priority.LOW, VisualAlert.none, AudibleAlert.disengage, 1.),
  },

  EventNameSP.manualLongitudinalRequired: {
    ET.WARNING: Alert(
      "智能/自适应巡航控制：关闭",
      "需要手动速度控制",
      AlertStatus.normal, AlertSize.mid,
      Priority.LOW, VisualAlert.none, AudibleAlert.none, 1.),
  },

  EventNameSP.silentLkasEnable: {
    ET.ENABLE: EngagementAlert(AudibleAlert.none),
  },

  EventNameSP.silentLkasDisable: {
    ET.USER_DISABLE: EngagementAlert(AudibleAlert.none),
  },

  EventNameSP.silentBrakeHold: {
    ET.WARNING: EngagementAlert(AudibleAlert.none),
    ET.NO_ENTRY: NoEntryAlert("刹车保持激活"),
  },

  EventNameSP.silentWrongGear: {
    ET.WARNING: Alert(
      "",
      "",
      AlertStatus.normal, AlertSize.none,
      Priority.LOWEST, VisualAlert.none, AudibleAlert.none, 0.),
    ET.NO_ENTRY: Alert(
      "档位不在D档",
      "openpilot不可用",
      AlertStatus.normal, AlertSize.mid,
      Priority.LOW, VisualAlert.none, AudibleAlert.none, 0.),
  },

  EventNameSP.silentReverseGear: {
    ET.PERMANENT: Alert(
      "倒车中",
      "",
      AlertStatus.normal, AlertSize.full,
      Priority.LOWEST, VisualAlert.none, AudibleAlert.none, .2, creation_delay=0.5),
    ET.NO_ENTRY: NoEntryAlert("倒车中"),
  },

  EventNameSP.silentDoorOpen: {
    ET.WARNING: Alert(
      "",
      "",
      AlertStatus.normal, AlertSize.none,
      Priority.LOWEST, VisualAlert.none, AudibleAlert.none, 0.),
    ET.NO_ENTRY: NoEntryAlert("车门已打开"),
  },

  EventNameSP.silentSeatbeltNotLatched: {
    ET.WARNING: Alert(
      "",
      "",
      AlertStatus.normal, AlertSize.none,
      Priority.LOWEST, VisualAlert.none, AudibleAlert.none, 0.),
    ET.NO_ENTRY: NoEntryAlert("请系好安全带"),
  },

  EventNameSP.silentParkBrake: {
    ET.WARNING: Alert(
      "",
      "",
      AlertStatus.normal, AlertSize.none,
      Priority.LOWEST, VisualAlert.none, AudibleAlert.none, 0.),
    ET.NO_ENTRY: NoEntryAlert("手刹已启用"),
  },

  EventNameSP.controlsMismatchLateral: {
    ET.IMMEDIATE_DISABLE: ImmediateDisableAlert("控制装置不匹配：横向"),
    ET.NO_ENTRY: NoEntryAlert("控制装置不匹配：横向"),
  },

  EventNameSP.experimentalModeSwitched: {
    ET.WARNING: NormalPermanentAlert("实验模式切换", duration=1.5)
  },

  EventNameSP.wrongCarModeAlertOnly: {
    ET.WARNING: wrong_car_mode_alert,
  },

  EventNameSP.pedalPressedAlertOnly: {
    ET.WARNING: NoEntryAlert("踏板被踩下")
  },

  EventNameSP.laneTurnLeft: {
    ET.WARNING: Alert(
      "左转",
      "",
      AlertStatus.normal, AlertSize.small,
      Priority.LOW, VisualAlert.none, AudibleAlert.none, 1.),
  },

  EventNameSP.laneTurnRight: {
    ET.WARNING: Alert(
      "右转",
      "",
      AlertStatus.normal, AlertSize.small,
      Priority.LOW, VisualAlert.none, AudibleAlert.none, 1.),
  },

  EventNameSP.speedLimitActive: {
    ET.WARNING: Alert(
      "自动调整以适应标示的速度限制",
      "",
      AlertStatus.normal, AlertSize.small,
      Priority.LOW, VisualAlert.none, AudibleAlert.none, 5.),
  },

  EventNameSP.speedLimitChanged: {
    ET.WARNING: Alert(
      "设置速度已更改",
      "",
      AlertStatus.normal, AlertSize.small,
      Priority.LOW, VisualAlert.none, AudibleAlert.none, 5.),
  },

  EventNameSP.speedLimitPreActive: {
    ET.WARNING: speed_limit_pre_active_alert,
  },

  EventNameSP.speedLimitPending: {
    ET.WARNING: Alert(
      "自动调整到之前的限速",
      "",
      AlertStatus.normal, AlertSize.small,
      Priority.LOW, VisualAlert.none, AudibleAlert.none, 5.),
  },
}
