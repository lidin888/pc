from openpilot.common.params import Params
from openpilot.system.ui.widgets import Widget
from openpilot.system.ui.widgets.list_view import multiple_button_item, toggle_item
from openpilot.system.ui.widgets.scroller import Scroller


class CarrotLayout(Widget):
  def __init__(self):
    super().__init__()
    self._params = Params()

    speed_from_pcm = int(self._params.get("SpeedFromPCM") or b"1")
    panel_side = int(self._params.get("CarrotPanelSide") or b"0")
    auto_navi_speed_mode = int(self._params.get("AutoNaviSpeedCtrlMode") or b"0")
    cruise_eco = int(self._params.get("CruiseEcoControl") or b"0")
    auto_turn_ctrl = int(self._params.get("AutoTurnControl") or b"0")
    traffic_mode = int(self._params.get("TrafficLightDetectMode") or b"0")
    driving_mode = int(self._params.get("MyDrivingMode") or b"3")

    items = [
      # === Panel Side ===
      multiple_button_item(
        "导航面板位置 (CarrotPanelSide)",
        "选择CarrotMan导航面板显示位置:\n"
        "0: 左侧（默认）  1: 右侧",
        buttons=["左侧", "右侧"],
        button_width=160,
        callback=self._set_panel_side,
        selected_index=panel_side,
      ),

      # === Cruise Control ===
      multiple_button_item(
        "定速巡航来源 (SpeedFromPCM)",
        "选择巡航速度来源:\n"
        "0: 使用CP计算的速度（最保守）\n"
        "1: 使用原车PCM速度（丰田推荐）\n"
        "2: 使用CP期望速度，最低30km/h（默认）\n"
        "3: 停车用原车速度，其它用CP（本田推荐）",
        buttons=["CP", "原车", "CP30", "混合"],
        button_width=160,
        callback=self._set_speed_from_pcm,
        selected_index=speed_from_pcm,
      ),

      # === Navigation Speed Control ===
      multiple_button_item(
        "导航限速模式 (AutoNaviSpeedCtrlMode)",
        "导航限速控制方式:\n"
        "0: 关闭\n"
        "1: 减速到限速\n"
        "2: 减速并保持\n"
        "3: 自动调节",
        buttons=["关闭", "减速", "保持", "自动"],
        button_width=140,
        callback=self._set_navi_speed_mode,
        selected_index=auto_navi_speed_mode,
      ),

      # === Driving Mode ===
      multiple_button_item(
        "驾驶模式 (MyDrivingMode)",
        "控制纵向跟车风格:\n"
        "0: 节能  1: 安全  2: 标准  3: 运动",
        buttons=["节能", "安全", "标准", "运动"],
        button_width=140,
        callback=self._set_driving_mode,
        selected_index=driving_mode,
      ),

      # === Cruise Eco Control ===
      multiple_button_item(
        "巡航节能偏移 (CruiseEcoControl)",
        "在限速基础上额外减少的速度偏移(km/h):\n"
        "0=关闭, 1~5 对应偏移值",
        buttons=["关", "1", "2", "3", "4", "5"],
        button_width=100,
        callback=self._set_cruise_eco,
        selected_index=cruise_eco,
      ),

      # === Traffic Light Detection ===
      multiple_button_item(
        "红绿灯检测 (TrafficLightDetectMode)",
        "红绿灯检测模式:\n"
        "0: 关闭  1: 仅警告  2: 自动制动",
        buttons=["关闭", "警告", "制动"],
        button_width=140,
        callback=self._set_traffic_mode,
        selected_index=traffic_mode,
      ),

      # === Auto Turn Control ===
      multiple_button_item(
        "自动转弯控制 (AutoTurnControl)",
        "导航引导时自动转弯:\n"
        "0: 关闭  1: 仅减速  2: 自动转向",
        buttons=["关闭", "减速", "转向"],
        button_width=140,
        callback=self._set_auto_turn,
        selected_index=auto_turn_ctrl,
      ),

      # === Dynamic TFollow ===
      toggle_item(
        "动态跟车距离 (DynamicTFollow)",
        "根据路况自动调节与前车的跟车时间距离。",
        self._params.get_bool("DynamicTFollow"),
        callback=lambda v: self._params.put_bool("DynamicTFollow", v),
      ),

      # === Stop Distance ===
      multiple_button_item(
        "停车距离 (StopDistanceCarrot)",
        "与前车停车时保持的距离(cm):\n"
        "350=近  450=标准  550=默认  650=远  750=最远",
        buttons=["350", "450", "550", "650", "750"],
        button_width=120,
        callback=self._set_stop_distance,
        selected_index=self._get_stop_distance_index(),
      ),
    ]

    self._scroller = Scroller(items, line_separator=True, spacing=0)

  def _render(self, rect):
    self._scroller.render(rect)

  def _set_panel_side(self, idx: int):
    self._params.put("CarrotPanelSide", str(idx))

  def _set_speed_from_pcm(self, idx: int):
    self._params.put("SpeedFromPCM", str(idx))

  def _set_navi_speed_mode(self, idx: int):
    self._params.put("AutoNaviSpeedCtrlMode", str(idx))

  def _set_driving_mode(self, idx: int):
    self._params.put("MyDrivingMode", str(idx))

  def _set_cruise_eco(self, idx: int):
    self._params.put("CruiseEcoControl", str(idx))

  def _set_traffic_mode(self, idx: int):
    self._params.put("TrafficLightDetectMode", str(idx))

  def _set_auto_turn(self, idx: int):
    self._params.put("AutoTurnControl", str(idx))

  def _set_stop_distance(self, idx: int):
    distances = [350, 450, 550, 650, 750]
    self._params.put("StopDistanceCarrot", str(distances[idx]))

  def _get_stop_distance_index(self) -> int:
    val = int(self._params.get("StopDistanceCarrot") or b"550")
    distances = [350, 450, 550, 650, 750]
    if val in distances:
      return distances.index(val)
    return 2  # default 550
