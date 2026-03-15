import os
import json

from openpilot.common.basedir import BASEDIR
from openpilot.common.params import Params
from openpilot.selfdrive.ui.onroad.driver_camera_dialog import DriverCameraDialog
from openpilot.selfdrive.ui.ui_state import ui_state
from openpilot.selfdrive.ui.widgets.pairing_dialog import PairingDialog
from openpilot.system.hardware import TICI
from openpilot.system.ui.lib.application import gui_app
from openpilot.system.ui.widgets import Widget, DialogResult
from openpilot.system.ui.widgets.confirm_dialog import confirm_dialog, alert_dialog
from openpilot.system.ui.widgets.html_render import HtmlRenderer
from openpilot.system.ui.widgets.list_view import text_item, button_item, dual_button_item
from openpilot.system.ui.widgets.option_dialog import MultiOptionDialog
from openpilot.system.ui.widgets.scroller import Scroller

# Description constants
DESCRIPTIONS = {
  'pair_device': "将您的设备与 comma connect (connect.comma.ai) 配对并领取您的 comma prime 优惠。",
  'driver_camera': "预览驾驶员面部摄像头以确保驾驶员监控有良好的可见性。（车辆必须熄火）",
  'reset_calibration': (
      "openpilot 要求设备安装在左或右 4° 内，以及向上 5° 或向下 9° 内。 " +
      "openpilot 正在持续校准，重置很少需要。"
  ),
  'review_guide': "查看 openpilot 的规则、功能和限制",
}


class DeviceLayout(Widget):
  def __init__(self):
    super().__init__()

    self._params = Params()
    self._select_language_dialog: MultiOptionDialog | None = None
    self._driver_camera: DriverCameraDialog | None = None
    self._pair_device_dialog: PairingDialog | None = None
    self._fcc_dialog: HtmlRenderer | None = None

    items = self._initialize_items()
    self._scroller = Scroller(items, line_separator=True, spacing=0)

  def _initialize_items(self):
    dongle_id = self._params.get("DongleId") or "N/A"
    serial = self._params.get("HardwareSerial") or "N/A"

    items = [
      text_item("设备 ID", dongle_id),
      text_item("序列号", serial),
      button_item("配对设备", "配对", DESCRIPTIONS['pair_device'], callback=self._pair_device),
      button_item("驾驶员摄像头", "预览", DESCRIPTIONS['driver_camera'], callback=self._show_driver_camera, enabled=ui_state.is_offroad),
      button_item("重置校准", "重置", DESCRIPTIONS['reset_calibration'], callback=self._reset_calibration_prompt),
      regulatory_btn := button_item("监管", "查看", callback=self._on_regulatory),
      button_item("查看训练指南", "查看", DESCRIPTIONS['review_guide'], self._on_review_training_guide),
      button_item("更改语言", "更改", callback=self._show_language_selection, enabled=ui_state.is_offroad),
      dual_button_item("重启", "关机", left_callback=self._reboot_prompt, right_callback=self._power_off_prompt),
    ]
    regulatory_btn.set_visible(TICI)
    return items

  def _render(self, rect):
    self._scroller.render(rect)

  def _show_language_selection(self):
    try:
      languages_file = os.path.join(BASEDIR, "selfdrive/ui/translations/languages.json")
      with open(languages_file, encoding='utf-8') as f:
        languages = json.load(f)

      self._select_language_dialog = MultiOptionDialog("选择一种语言", languages)
      gui_app.set_modal_overlay(self._select_language_dialog, callback=self._handle_language_selection)
    except FileNotFoundError:
      pass

  def _handle_language_selection(self, result: int):
    if result == 1 and self._select_language_dialog:
      selected_language = self._select_language_dialog.selection
      self._params.put("LanguageSetting", selected_language)

    self._select_language_dialog = None

  def _show_driver_camera(self):
    if not self._driver_camera:
      self._driver_camera = DriverCameraDialog()

    gui_app.set_modal_overlay(self._driver_camera, callback=lambda result: setattr(self, '_driver_camera', None))

  def _reset_calibration_prompt(self):
    if ui_state.engaged:
      gui_app.set_modal_overlay(lambda: alert_dialog("脱离驾驶以重置校准"))
      return

    gui_app.set_modal_overlay(
      lambda: confirm_dialog("您确定要重置校准吗？", "重置"),
      callback=self._reset_calibration,
    )

  def _reset_calibration(self, result: int):
    if ui_state.engaged or result != DialogResult.CONFIRM:
      return

    self._params.remove("CalibrationParams")
    self._params.remove("LiveTorqueParameters")
    self._params.remove("LiveParameters")
    self._params.remove("LiveParametersV2")
    self._params.remove("LiveDelay")
    self._params.put_bool("OnroadCycleRequested", True)

  def _reboot_prompt(self):
    if ui_state.engaged:
      gui_app.set_modal_overlay(lambda: alert_dialog("脱离驾驶以重启"))
      return

    gui_app.set_modal_overlay(
      lambda: confirm_dialog("您确定要重启吗？", "重启"),
      callback=self._perform_reboot,
    )

  def _perform_reboot(self, result: int):
    if not ui_state.engaged and result == DialogResult.CONFIRM:
      self._params.put_bool_nonblocking("DoReboot", True)

  def _power_off_prompt(self):
    if ui_state.engaged:
      gui_app.set_modal_overlay(lambda: alert_dialog("脱离驾驶以关机"))
      return

    gui_app.set_modal_overlay(
      lambda: confirm_dialog("您确定要关机吗？", "关机"),
      callback=self._perform_power_off,
    )

  def _perform_power_off(self, result: int):
    if not ui_state.engaged and result == DialogResult.CONFIRM:
      self._params.put_bool_nonblocking("DoShutdown", True)

  def _pair_device(self):
    if not self._pair_device_dialog:
      self._pair_device_dialog = PairingDialog()
    gui_app.set_modal_overlay(self._pair_device_dialog, callback=lambda result: setattr(self, '_pair_device_dialog', None))

  def _on_regulatory(self):
    if not self._fcc_dialog:
      self._fcc_dialog = HtmlRenderer(os.path.join(BASEDIR, "selfdrive/assets/offroad/fcc.html"))

    gui_app.set_modal_overlay(self._fcc_dialog,
      callback=lambda result: setattr(self, '_fcc_dialog', None),
    )

  def _on_review_training_guide(self): pass
