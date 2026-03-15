from openpilot.common.params import Params
from openpilot.selfdrive.ui.widgets.ssh_key import ssh_key_item
from openpilot.system.ui.widgets import Widget
from openpilot.system.ui.widgets.list_view import toggle_item
from openpilot.system.ui.widgets.scroller import Scroller

# Description constants
DESCRIPTIONS = {
  'enable_adb': (
    "ADB (Android Debug Bridge) 允许通过 USB 或网络连接到您的设备。 " +
    "有关更多信息，请参见 https://docs.comma.ai/how-to/connect-to-comma。"
  ),
  'joystick_debug_mode': "启用操纵杆调试模式，用于测试和调试驾驶输入。",
  'ssh_key': (
    "警告：这会授予 SSH 访问权限给您 GitHub 设置中的所有公钥。切勿输入除您自己以外的 GitHub 用户名。 " +
    "comma 员工绝不会要求您添加他们的 GitHub 用户名。"
  ),
}


class DeveloperLayout(Widget):
  def __init__(self):
    super().__init__()
    self._params = Params()
    items = [
      toggle_item(
        "启用 ADB",
        description=DESCRIPTIONS["enable_adb"],
        initial_state=self._params.get_bool("AdbEnabled"),
        callback=self._on_enable_adb,
      ),
      ssh_key_item("SSH 密钥", description=DESCRIPTIONS["ssh_key"]),
      toggle_item(
        "操纵杆调试模式",
        description=DESCRIPTIONS["joystick_debug_mode"],
        initial_state=self._params.get_bool("JoystickDebugMode"),
        callback=self._on_joystick_debug_mode,
      ),
      toggle_item(
        "纵向机动模式",
        description="",
        initial_state=self._params.get_bool("LongitudinalManeuverMode"),
        callback=self._on_long_maneuver_mode,
      ),
      toggle_item(
        "openpilot 纵向控制 (Alpha)",
        description="",
        initial_state=self._params.get_bool("AlphaLongitudinalEnabled"),
        callback=self._on_alpha_long_enabled,
      ),
    ]

    self._scroller = Scroller(items, line_separator=True, spacing=0)

  def _render(self, rect):
    self._scroller.render(rect)

  def _on_enable_adb(self): pass
  def _on_joystick_debug_mode(self): pass
  def _on_long_maneuver_mode(self): pass
  def _on_alpha_long_enabled(self): pass
