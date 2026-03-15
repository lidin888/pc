import pyray as rl
import time
import threading

from openpilot.common.api import api_get
from openpilot.common.params import Params
from openpilot.common.swaglog import cloudlog
from openpilot.selfdrive.ui.ui_state import ui_state
from openpilot.system.athena.registration import UNREGISTERED_DONGLE_ID
from openpilot.system.ui.lib.application import gui_app, FontWeight
from openpilot.system.ui.lib.scroll_panel import GuiScrollPanel
from openpilot.system.ui.lib.wrap_text import wrap_text
from openpilot.system.ui.widgets import Widget
from openpilot.selfdrive.ui.lib.api_helpers import get_token

TITLE = "数据上传模式"
DESCRIPTION = (
  "openpilot 通过观察人类（如您）驾驶来学习驾驶。\n\n"
  + "数据上传模式允许您最大化训练数据上传，以改进 "
  + "openpilot 的驾驶模型。更多数据意味着更大的模型，这意味着更好的实验模式。"
)
INSTRUCTIONS = (
  "为了获得最大效果，请每周将设备带入室内并连接到良好的 USB-C 适配器和 Wi-Fi。\n\n"
  + "数据上传模式也可以在驾驶时工作（如果连接到热点或无限 SIM 卡）。\n\n"
  + "常见问题解答\n\n"
  + "驾驶方式或地点重要吗？不重要，只需像平常一样驾驶即可。\n\n"
  + "所有片段都会在数据上传模式下被拉取吗？不，我们有选择地拉取您的片段子集。\n\n"
  + "好的 USB-C 适配器是什么？任何快速的手机或笔记本电脑充电器都可以。\n\n"
  + "运行什么软件重要吗？是的，只有上游 openpilot（和特定分支）能够用于训练。"
)


class FirehoseLayout(Widget):
  PARAM_KEY = "ApiCache_FirehoseStats"
  GREEN = rl.Color(46, 204, 113, 255)
  RED = rl.Color(231, 76, 60, 255)
  GRAY = rl.Color(68, 68, 68, 255)
  LIGHT_GRAY = rl.Color(228, 228, 228, 255)
  UPDATE_INTERVAL = 30  # seconds

  def __init__(self):
    super().__init__()
    self.params = Params()
    self.segment_count = self._get_segment_count()
    self.scroll_panel = GuiScrollPanel()

    self.running = True
    self.update_thread = threading.Thread(target=self._update_loop, daemon=True)
    self.update_thread.start()
    self.last_update_time = 0

  def _get_segment_count(self) -> int:
    stats = self.params.get(self.PARAM_KEY)
    if not stats:
      return 0
    try:
      return int(stats.get("firehose", 0))
    except Exception:
      cloudlog.exception(f"Failed to decode firehose stats: {stats}")
      return 0

  def __del__(self):
    self.running = False
    if self.update_thread and self.update_thread.is_alive():
      self.update_thread.join(timeout=1.0)

  def _render(self, rect: rl.Rectangle):
    # Calculate content dimensions
    content_width = rect.width - 80
    content_height = self._calculate_content_height(int(content_width))
    content_rect = rl.Rectangle(rect.x, rect.y, rect.width, content_height)

    # Handle scrolling and render with clipping
    scroll_offset = self.scroll_panel.handle_scroll(rect, content_rect)
    rl.begin_scissor_mode(int(rect.x), int(rect.y), int(rect.width), int(rect.height))
    self._render_content(rect, scroll_offset)
    rl.end_scissor_mode()

  def _calculate_content_height(self, content_width: int) -> int:
    height = 80  # Top margin

    # Title
    height += 100 + 40

    # Description
    desc_font = gui_app.font(FontWeight.NORMAL)
    desc_lines = wrap_text(desc_font, DESCRIPTION, 45, content_width)
    height += len(desc_lines) * 45 + 40

    # Status section
    height += 32  # Separator
    status_text, _ = self._get_status()
    status_lines = wrap_text(gui_app.font(FontWeight.BOLD), status_text, 60, content_width)
    height += len(status_lines) * 60 + 20

    # Contribution count (if available)
    if self.segment_count > 0:
      contrib_text = f"{self.segment_count} segment(s) of your driving is in the training dataset so far."
      contrib_lines = wrap_text(gui_app.font(FontWeight.BOLD), contrib_text, 52, content_width)
      height += len(contrib_lines) * 52 + 20

    # Instructions section
    height += 32  # Separator
    inst_lines = wrap_text(gui_app.font(FontWeight.NORMAL), INSTRUCTIONS, 40, content_width)
    height += len(inst_lines) * 40 + 40  # Bottom margin

    return height

  def _render_content(self, rect: rl.Rectangle, scroll_offset: rl.Vector2):
    x = int(rect.x + 40)
    y = int(rect.y + 40 + scroll_offset.y)
    w = int(rect.width - 80)

    # Title
    title_font = gui_app.font(FontWeight.MEDIUM)
    rl.draw_text_ex(title_font, TITLE, rl.Vector2(x, y), 100, 0, rl.WHITE)
    y += 140

    # Description
    y = self._draw_wrapped_text(x, y, w, DESCRIPTION, gui_app.font(FontWeight.NORMAL), 45, rl.WHITE)
    y += 40

    # Separator
    rl.draw_rectangle(x, y, w, 2, self.GRAY)
    y += 30

    # Status
    status_text, status_color = self._get_status()
    y = self._draw_wrapped_text(x, y, w, status_text, gui_app.font(FontWeight.BOLD), 60, status_color)
    y += 20

    # Contribution count (if available)
    if self.segment_count > 0:
      contrib_text = f"迄今为止，您的驾驶中有 {self.segment_count} 个片段已进入训练数据集。"
      y = self._draw_wrapped_text(x, y, w, contrib_text, gui_app.font(FontWeight.BOLD), 52, rl.WHITE)
      y += 20

    # Separator
    rl.draw_rectangle(x, y, w, 2, self.GRAY)
    y += 30

    # Instructions
    self._draw_wrapped_text(x, y, w, INSTRUCTIONS, gui_app.font(FontWeight.NORMAL), 40, self.LIGHT_GRAY)

  def _draw_wrapped_text(self, x, y, width, text, font, size, color):
    wrapped = wrap_text(font, text, size, width)
    for line in wrapped:
      rl.draw_text_ex(font, line, rl.Vector2(x, y), size, 0, color)
      y += size
    return y

  def _get_status(self) -> tuple[str, rl.Color]:
    network_type = ui_state.sm["deviceState"].networkType
    network_metered = ui_state.sm["deviceState"].networkMetered

    if not network_metered and network_type != 0:  # Not metered and connected
      return "激活", self.GREEN
    else:
      return "未激活：连接到非计量网络", self.RED

  def _fetch_firehose_stats(self):
    try:
      dongle_id = self.params.get("DongleId")
      if not dongle_id or dongle_id == UNREGISTERED_DONGLE_ID:
        return
      identity_token = get_token(dongle_id)
      response = api_get(f"v1/devices/{dongle_id}/firehose_stats", access_token=identity_token)
      if response.status_code == 200:
        data = response.json()
        self.segment_count = data.get("firehose", 0)
        self.params.put(self.PARAM_KEY, data)
    except Exception as e:
      cloudlog.error(f"Failed to fetch firehose stats: {e}")

  def _update_loop(self):
    while self.running:
      if not ui_state.started:
        self._fetch_firehose_stats()
      time.sleep(self.UPDATE_INTERVAL)
