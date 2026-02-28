#!/usr/bin/env python3
import os
import sys
import threading
import time
from enum import IntEnum

import pyray as rl

from openpilot.system.hardware import PC
from openpilot.system.ui.lib.application import gui_app, FontWeight, FONT_SCALE
from openpilot.system.ui.widgets import Widget
from openpilot.system.ui.widgets.button import Button, ButtonStyle
from openpilot.system.ui.widgets.label import gui_label, gui_text_box

USERDATA = "/dev/disk/by-partlabel/userdata"
TIMEOUT = 3*60


class ResetMode(IntEnum):
  USER_RESET = 0  # user initiated a factory reset from openpilot
  RECOVER = 1     # userdata is corrupt for some reason, give a chance to recover
  FORMAT = 2      # finish up a factory reset from a tool that doesn't flash an empty partition to userdata


class ResetState(IntEnum):
  NONE = 0
  CONFIRM = 1
  RESETTING = 2
  FAILED = 3


class Reset(Widget):
  def __init__(self, mode):
    super().__init__()
    self._mode = mode
    self._previous_reset_state = None
    self._reset_state = ResetState.NONE
    self._cancel_button = Button("取消", gui_app.request_close)
    self._confirm_button = Button("确认", self._confirm, button_style=ButtonStyle.PRIMARY)
    self._reboot_button = Button("重启", lambda: os.system("sudo reboot"))

  def _do_erase(self):
    if PC:
      return

    # Removing data and formatting
    rm = os.system("sudo rm -rf /data/*")
    os.system(f"sudo umount {USERDATA}")
    fmt = os.system(f"yes | sudo mkfs.ext4 {USERDATA}")

    if rm == 0 or fmt == 0:
      os.system("sudo reboot")
    else:
      self._reset_state = ResetState.FAILED

  def start_reset(self):
    self._reset_state = ResetState.RESETTING
    threading.Timer(0.1, self._do_erase).start()

  def _update_state(self):
    if self._reset_state != self._previous_reset_state:
      self._previous_reset_state = self._reset_state
      self._timeout_st = time.monotonic()
    elif self._reset_state != ResetState.RESETTING and (time.monotonic() - self._timeout_st) > TIMEOUT:
      exit(0)

  def _render(self, _):
    content_rect = rl.Rectangle(45, 200, self._rect.width - 90, self._rect.height - 245)

    label_rect = rl.Rectangle(content_rect.x + 140, content_rect.y, content_rect.width - 280, 100 * FONT_SCALE)
    gui_label(label_rect, "系统重置", 100, font_weight=FontWeight.BOLD)

    text_rect = rl.Rectangle(content_rect.x + 140, content_rect.y + 140, content_rect.width - 280, content_rect.height - 90 - 100 * FONT_SCALE)
    gui_text_box(text_rect, self._get_body_text(), 90)

    button_height = 160
    button_spacing = 50
    button_top = content_rect.y + content_rect.height - button_height
    button_width = (content_rect.width - button_spacing) / 2.0

    if self._reset_state != ResetState.RESETTING:
      if self._mode == ResetMode.RECOVER:
        self._reboot_button.render(rl.Rectangle(content_rect.x, button_top, button_width, button_height))
      elif self._mode == ResetMode.USER_RESET:
        self._cancel_button.render(rl.Rectangle(content_rect.x, button_top, button_width, button_height))

      if self._reset_state != ResetState.FAILED:
        self._confirm_button.render(rl.Rectangle(content_rect.x + button_width + 50, button_top, button_width, button_height))
      else:
        self._reboot_button.render(rl.Rectangle(content_rect.x, button_top, content_rect.width, button_height))

  def _confirm(self):
    if self._reset_state == ResetState.CONFIRM:
      self.start_reset()
    else:
      self._reset_state = ResetState.CONFIRM

  def _get_body_text(self):
    if self._reset_state == ResetState.CONFIRM:
      return "您确定要重置设备吗？"
    if self._reset_state == ResetState.RESETTING:
      return "正在重置设备...\n这可能需要一分钟。"
    if self._reset_state == ResetState.FAILED:
      return "重置失败。重启后重试。"
    if self._mode == ResetMode.RECOVER:
      return "无法挂载数据分区。分区可能已损坏。按确认以擦除并重置设备。"
    return "已触发系统重置。按确认以擦除所有内容和设置。按取消以继续启动。"


def main():
  mode = ResetMode.USER_RESET
  if len(sys.argv) > 1:
    if sys.argv[1] == '--recover':
      mode = ResetMode.RECOVER
    elif sys.argv[1] == "--format":
      mode = ResetMode.FORMAT

  gui_app.init_window("系统重置", 20)
  reset = Reset(mode)

  if mode == ResetMode.FORMAT:
    reset.start_reset()

  gui_app.push_widget(reset)

  for _ in gui_app.render():
    pass


if __name__ == "__main__":
  main()
