from openpilot.common.params import Params
from openpilot.system.ui.widgets import Widget
from openpilot.system.ui.widgets.list_view import multiple_button_item, toggle_item
from openpilot.system.ui.widgets.scroller import Scroller

# Description constants
DESCRIPTIONS = {
  "OpenpilotEnabledToggle": (
    "使用 openpilot 系统进行自适应巡航控制和车道保持驾驶辅助。 " +
    "您始终需要保持注意力。"
  ),
  "ExperimentalMode": "启用实验性功能，可能不稳定或不完整。",
  "DisengageOnAccelerator": "启用时，踩下油门踏板将脱离 openpilot。",
  "LongitudinalPersonality": (
    "标准是推荐的。在激进模式下，openpilot 会更接近前车跟随并对油门和刹车更激进。 " +
    "在放松模式下，openpilot 会远离前车。在支持的车上，您可以使用方向盘距离按钮循环这些个性。"
  ),
  "IsLdwEnabled": (
    "当车道偏离警告激活时，在超过 31 mph (50 km/h) 驾驶时接收转向回到车道的警报。"
  ),
  "AlwaysOnDM": "即使 openpilot 未接合，也启用驾驶员监控。",
  'RecordFront': "上传驾驶员面部摄像头数据，帮助改进驾驶员监控算法。",
  "IsMetric": "以 km/h 而不是 mph 显示速度。",
  "RecordAudio": "在驾驶时记录并存储麦克风音频。音频将包含在 comma connect 中的 dashcam 视频中。",
}


class TogglesLayout(Widget):
  def __init__(self):
    super().__init__()
    self._params = Params()
    items = [
      toggle_item(
        "启用 openpilot",
        DESCRIPTIONS["OpenpilotEnabledToggle"],
        self._params.get_bool("OpenpilotEnabledToggle"),
        icon="chffr_wheel.png",
      ),
      toggle_item(
        "实验模式",
        DESCRIPTIONS["ExperimentalMode"],
        self._params.get_bool("ExperimentalMode"),
        icon="experimental_white.png",
      ),
      toggle_item(
        "踩油门时脱离",
        DESCRIPTIONS["DisengageOnAccelerator"],
        self._params.get_bool("DisengageOnAccelerator"),
        icon="disengage_on_accelerator.png",
      ),
      multiple_button_item(
        "驾驶个性",
        DESCRIPTIONS["LongitudinalPersonality"],
        buttons=["激进", "标准", "放松"],
        button_width=255,
        callback=self._set_longitudinal_personality,
        selected_index=self._params.get("LongitudinalPersonality", return_default=True),
        icon="speed_limit.png"
      ),
      toggle_item(
        "启用车道偏离警告",
        DESCRIPTIONS["IsLdwEnabled"],
        self._params.get_bool("IsLdwEnabled"),
        icon="warning.png",
      ),
      toggle_item(
        "始终开启驾驶员监控",
        DESCRIPTIONS["AlwaysOnDM"],
        self._params.get_bool("AlwaysOnDM"),
        icon="monitoring.png",
      ),
      toggle_item(
        "记录并上传驾驶员摄像头",
        DESCRIPTIONS["RecordFront"],
        self._params.get_bool("RecordFront"),
        icon="monitoring.png",
      ),
      toggle_item(
        "记录麦克风音频",
        DESCRIPTIONS["RecordAudio"],
        self._params.get_bool("RecordAudio"),
        icon="microphone.png",
      ),
      toggle_item(
        "使用公制系统", DESCRIPTIONS["IsMetric"], self._params.get_bool("IsMetric"), icon="metric.png"
      ),
    ]

    self._scroller = Scroller(items, line_separator=True, spacing=0)

  def _render(self, rect):
    self._scroller.render(rect)

  def _set_longitudinal_personality(self, button_index: int):
    self._params.put("LongitudinalPersonality", button_index)
