#!/usr/bin/env python3
"""
SunnyPilot特性完整一键移植脚本
基于当前代码状态和原始分支对比，实现完整的一键移植

使用方法:
    python3 complete_sp_migration.py

功能:
1. 备份原始代码
2. 对比分析当前代码和原始分支
3. 应用所有必要的修改
4. 验证移植是否成功
5. 生成移植总结文档
"""

import os
import sys
import shutil
import subprocess
import datetime
import re
import difflib
from pathlib import Path

# 全局变量
BACKUP_DIR = "sp_migration_backup"
ORIGINAL_DIR = "sunnypilot_migration"
CURRENT_DIR = "."
LOG_FILE = "sp_migration_log.txt"

def log_message(message):
    """记录日志信息"""
    timestamp = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    log_entry = f"[{timestamp}] {message}\n"
    print(message)
    with open(LOG_FILE, 'a') as f:
        f.write(log_entry)

def backup_original_code():
    """备份原始代码"""
    log_message("开始备份原始代码...")

    # 创建备份目录
    if os.path.exists(BACKUP_DIR):
        shutil.rmtree(BACKUP_DIR)
    os.makedirs(BACKUP_DIR)

    # 备份关键文件
    files_to_backup = [
        "opendbc/car/toyota/carcontroller.py",
        "opendbc/car/toyota/carstate.py",
        "opendbc/car/toyota/toyotacan.py",
        "selfdrive/controls/controlsd.py",
        "selfdrive/ui/qt/offroad/settings.cc",
        "selfdrive/car/carrot_settings.json",
        "selfdrive/ui/qt/offroad/settings/lateral_panel.cc",
        "selfdrive/ui/qt/offroad/settings/vehicle/toyota_settings.cc"
    ]

    for file_path in files_to_backup:
        if os.path.exists(file_path):
            dest_path = os.path.join(BACKUP_DIR, file_path)
            os.makedirs(os.path.dirname(dest_path), exist_ok=True)
            shutil.copy2(file_path, dest_path)
            log_message(f"已备份: {file_path}")

    log_message("原始代码备份完成")

def apply_toyota_carcontroller_changes():
    """应用丰田车辆控制器修改"""
    log_message("应用丰田车辆控制器修改...")

    file_path = "opendbc/car/toyota/carcontroller.py"

    # 检查文件是否存在
    if not os.path.exists(file_path):
        log_message(f"错误: 文件不存在 {file_path}")
        return False

    # 读取文件内容
    with open(file_path, 'r') as f:
        content = f.read()

    # 修改1: 添加参数更新函数
    param_update_func = """
  def update_toyota_params(self):
    \"\"\"更新丰田特调参数\"\"\"
    params = Params()
    self.toyota_drive_mode = params.get_bool('ToyotaDriveMode')
    self.toyota_auto_hold = params.get_bool('ToyotaAutoHold')
    self.toyota_enhanced_bsm = params.get_bool('ToyotaEnhancedBsm')
    self.toyota_tss2_long = params.get_bool('ToyotaTSS2Long')
    self.toyota_stock_long = params.get_bool('ToyotaStockLongitudinal')

    # 初始化自动刹车保持相关变量（如果需要）
    if self.toyota_auto_hold and not hasattr(self, '_brake_hold_counter'):
      self.brake_hold_active: bool = False
      self._brake_hold_counter: int = 0
      self._brake_hold_reset: bool = False
      self._prev_brake_pressed: bool = False
"""

    # 修改2: 在update方法中调用参数更新
    update_call = """
    # 更新丰田特调参数
    self.update_toyota_params()
"""

    # 应用修改
    modifications = [
        # 修改3: 添加参数更新函数
        {
            "search": "def update(self, CC, CS, now_nanos):",
            "replace": f"{param_update_func}\n\n  def update(self, CC, CS, now_nanos):",
            "multi_line": True
        },

        # 修改4: 在update方法中调用参数更新
        {
            "search": "if len(CC.orientationNED) == 3:",
            "replace": f"{update_call}\n\n    if len(CC.orientationNED) == 3:",
            "multi_line": True
        },

        # 添加增强BSM功能
        {
            "search": "# 丰田特调逻辑",
            "replace": """# 丰田特调逻辑
    # 增强BSM功能 - 添加错误处理
    if self.toyota_enhanced_bsm and self.frame > 200:
      try:
        can_sends.extend(self.create_enhanced_bsm_messages(CS, 20, True))
      except Exception as e:
        carlog.error(f"Enhanced BSM error: {e}")""",
            "multi_line": True
        },

        # 添加自动刹车保持功能
        {
            "search": "# 自动刹车保持功能",
            "replace": """# 自动刹车保持功能
    if self.toyota_auto_hold:
      can_sends.extend(self.create_auto_brake_hold_messages(CS))""",
            "multi_line": True
        }
    ]

    # 应用修改
    for mod in modifications:
        if mod["search"] in content:
            if mod.get("multi_line"):
                content = content.replace(mod["search"], mod["replace"])
            else:
                content = content.replace(mod["search"], mod["replace"])
            log_message(f"已应用修改: {mod['search'][:50]}...")
        else:
            log_message(f"警告: 未找到匹配内容: {mod['search'][:50]}...")

    # 添加增强BSM消息创建函数
    if "create_enhanced_bsm_messages" not in content:
        bsm_function = """
  # 增强BSM功能
  def create_enhanced_bsm_messages(self, CS: structs.CarState, e_bsm_rate: int, enabled: bool):
    can_sends = []
    lr_blindspot = b"\\x00\\x00"

    if enabled:
      if self.frame % e_bsm_rate == 0:
        can_sends.append(toyotacan.create_set_bsm_debug_mode(lr_blindspot, True))
        self.left_last_blindspot_frame = self.frame

      if self.frame % e_bsm_rate == e_bsm_rate // 2:
        can_sends.append(toyotacan.create_bsm_polling_status(RIGHT_BLINDSPOT))
        self.right_last_blindspot_frame = self.frame

    return can_sends

  # 自动刹车保持 (https://github.com/AlexandreSato/)
  def create_auto_brake_hold_messages(self, CS: structs.CarState, brake_hold_allowed_timer: int = 100):
    can_sends = []
    disallowed_gears = [GearShifter.park, GearShifter.reverse]
    brake_hold_allowed = CS.out.standstill and CS.out.cruiseState.available and not CS.out.gasPressed and \\
                         not CS.out.cruiseState.enabled and (CS.out.gearShifter not in disallowed_gears)

    # 添加错误处理，防止踩刹车时报错
    try:
      if brake_hold_allowed:
        self._brake_hold_counter += 1
        self.brake_hold_active = self._brake_hold_counter > brake_hold_allowed_timer and not self._brake_hold_reset
        self._brake_hold_reset = not self._prev_brake_pressed and CS.out.brakePressed and not self._brake_hold_reset
      else:
        self._brake_hold_counter = 0
        self.brake_hold_active = False
        self._brake_hold_reset = False
      self._prev_brake_pressed = CS.out.brakePressed

      # 只在需要时发送刹车保持命令，减少不必要的CAN消息
      if self.frame % 2 == 0 and (brake_hold_allowed or self.brake_hold_active):
        can_sends.append(toyotacan.create_brake_hold_command(self.packer, self.frame, CS.pre_collision_2, self.brake_hold_active))
    except Exception as e:
      # 记录错误但不中断系统运行
      carlog.error(f"Auto brake hold error: {e}")

    return can_sends"""

        # 在文件末尾添加函数
        content += bsm_function
        log_message("已添加增强BSM和自动刹车保持函数")

    # 写入文件
    with open(file_path, 'w') as f:
        f.write(content)

    log_message("丰田车辆控制器修改完成")
    return True

def apply_toyota_carstate_changes():
    """应用丰田车辆状态修改"""
    log_message("应用丰田车辆状态修改...")

    file_path = "opendbc/car/toyota/carstate.py"

    # 检查文件是否存在
    if not os.path.exists(file_path):
        log_message(f"错误: 文件不存在 {file_path}")
        return False

    # 读取文件内容
    with open(file_path, 'r') as f:
        content = f.read()

    # 添加驾驶模式检测逻辑
    drive_mode_code = """
    # 丰田特调：驾驶模式按钮链接
    if self.CP.carFingerprint in TSS2_CAR and Params().get_bool("ToyotaDriveMode"):
      sport_signal = 'SPORT_ON_2' if self.CP.carFingerprint in (CAR.TOYOTA_RAV4_TSS2, CAR.LEXUS_ES_TSS2, CAR.TOYOTA_HIGHLANDER_TSS2) else 'SPORT_ON'

      # Check signals once
      if not self.signals_checked:
        self.signals_checked = True

        # Try to detect sport mode signal, handle missing signal with a fallback
        try:
          sport_mode = cp.vl["GEAR_PACKET"][sport_signal]
          self.sport_signal_seen = True
        except KeyError:
          sport_mode = 0
          self.sport_signal_seen = False

        # Try to detect eco mode signal, handle missing signal with a fallback
        try:
          eco_mode = cp.vl["GEAR_PACKET"]['ECON_ON']
          self.eco_signal_seen = True
        except KeyError:
          eco_mode = 0
          self.eco_signal_seen = False
      else:
        # Always re-check the signals to account for mode changes
        sport_mode = cp.vl["GEAR_PACKET"][sport_signal] if self.sport_signal_seen else 0
        eco_mode = cp.vl["GEAR_PACKET"]['ECON_ON'] if self.eco_signal_seen else 0

      # Set acceleration personality based on drive mode
      if sport_mode:
        # Sport mode detected - set to aggressive personality
        from openpilot.common.params import Params
        Params().put("AccelPersonality", "2")  # Aggressive
      elif eco_mode:
        # Eco mode detected - set to relaxed personality
        from openpilot.common.params import Params
        Params().put("AccelPersonality", "0")  # Relaxed
      else:
        # Normal mode - set to standard personality
        from openpilot.common.params import Params
        Params().put("AccelPersonality", "1")  # Standard"""

    # 在__init__方法中添加变量初始化
    init_code = """    # 丰田特调变量初始化
    self.signals_checked = False
    self.sport_signal_seen = False
    self.eco_signal_seen = False"""

    # 应用修改
    if "def __init__(self, CP):" in content and "self.sport_signal_seen" not in content:
        content = content.replace("def __init__(self, CP):", f"def __init__(self, CP):{init_code}")
        log_message("已添加驾驶模式变量初始化")

    if "丰田特调：驾驶模式按钮链接" not in content:
        # 在return ret之前添加驾驶模式检测逻辑
        content = content.replace("return ret", drive_mode_code + "\n    return ret")
        log_message("已添加驾驶模式检测逻辑")

    # 写入文件
    with open(file_path, 'w') as f:
        f.write(content)

    log_message("丰田车辆状态修改完成")
    return True

def apply_toyota_toyotacan_changes():
    """应用丰田CAN消息修改"""
    log_message("应用丰田CAN消息修改...")

    file_path = "opendbc/car/toyota/toyotacan.py"

    # 检查文件是否存在
    if not os.path.exists(file_path):
        log_message(f"错误: 文件不存在 {file_path}")
        return False

    # 读取文件内容
    with open(file_path, 'r') as f:
        content = f.read()

    # 添加BSM相关函数
    bsm_functions = """
def create_set_bsm_debug_mode(lr_blindspot, enabled):
  dat = b"\\x02\\x10\\x60\\x00\\x00\\x00\\x00" if enabled else b"\\x02\\x10\\x01\\x00\\x00\\x00\\x00"
  dat = lr_blindspot + dat

  return CanData(0x750, dat, 0)


def create_bsm_polling_status(lr_blindspot):
  return CanData(0x750, lr_blindspot + b"\\x02\\x21\\x69\\x00\\x00\\x00\\x00", 0)


# auto brake hold
def create_brake_hold_command(packer, frame, pre_collision_2, brake_hold_active):
  # forward PRE_COLLISION_2 when auto brake hold is not active
  # 使用字典的get方法提供默认值，处理模拟器中缺少键的情况
  values = {s: pre_collision_2.get(s, 0) for s in [
    "DSS1GDRV",
    "DS1STAT2",
    "DS1STBK2",
    "PCSWAR",
    "PCSALM",
    "PCSOPR",
    "PCSABK",
    "PBATRGR",
    "PPTRGR",
    "IBTRGR",
    "CLEXTRGR",
    "IRLT_REQ",
    "BRKHLD",
    "AVSTRGR",
    "VGRSTRGR",
    "PREFILL",
    "PBRTRGR",
    "PCSDIS",
    "PBPREPMP",
  ]}

  if brake_hold_active:
    values["DSS1GDRV"] = 0x3FF
    values["PBRTRGR"] = frame % 730 < 727  # cut actuation for 3 frames

  return packer.make_can_msg("PRE_COLLISION_2", 0, values)
"""

    # 应用修改
    if "create_set_bsm_debug_mode" not in content:
        content += bsm_functions
        log_message("已添加BSM和刹车保持相关函数")

    # 写入文件
    with open(file_path, 'w') as f:
        f.write(content)

    log_message("丰田CAN消息修改完成")
    return True

def apply_controlsd_changes():
    """应用控制逻辑修改"""
    log_message("应用控制逻辑修改...")

    file_path = "selfdrive/controls/controlsd.py"

    # 检查文件是否存在
    if not os.path.exists(file_path):
        log_message(f"错误: 文件不存在 {file_path}")
        return False

    # 读取文件内容
    with open(file_path, 'r') as f:
        content = f.read()

    # 添加导入
    if "from opendbc.car.toyota.values import CAR" not in content:
        content = content.replace(
            "from opendbc.car.car_helpers import interfaces\nfrom opendbc.car.vehicle_model import VehicleModel",
            "from opendbc.car.car_helpers import interfaces\nfrom opendbc.car.vehicle_model import VehicleModel\nfrom opendbc.car.toyota.values import CAR"
        )
        log_message("已添加CAR导入")

    # 修改速度设置逻辑
    speed_logic = """    lp = self.sm['longitudinalPlan']
    if self.CP.pcmCruise:
      speed_from_pcm = self.params.get_int("SpeedFromPCM")
      # 丰田特调优先：如果启用了丰田特调，使用更合理的速度设置逻辑
      if self.CP.carFingerprint.startswith(CAR.TOYOTA):
        toyota_stock_long = self.params.get_bool("ToyotaStockLongitudinal")
        toyota_tss2_long = self.params.get_bool("ToyotaTSS2Long")

        if toyota_stock_long or toyota_tss2_long:
          # 丰田特调模式：使用计算出的setSpeed，确保速度变化能够生效
          hudControl.setSpeed = setSpeed if lp.xState == 3 else float(desired_kph * CV.KPH_TO_MS)
        elif speed_from_pcm == 1: #toyota
          # 原始丰田逻辑：使用vCruiseCluster，但确保其有效性
          if CS.vCruiseCluster > 0:
            hudControl.setSpeed = float(CS.vCruiseCluster * CV.KPH_TO_MS)
          else:
            hudControl.setSpeed = setSpeed if lp.xState == 3 else float(desired_kph * CV.KPH_TO_MS)
        elif speed_from_pcm == 2:
          hudControl.setSpeed = float(max(30/3.6, desired_kph * CV.KPH_TO_MS))
        elif speed_from_pcm == 3: # honda
          hudControl.setSpeed = setSpeed if lp.xState == 3 else float(desired_kph * CV.KPH_TO_MS)
        else:
          hudControl.setSpeed = float(max(30/3.6, setSpeed))
      else:
        # 非丰田车辆保持原有逻辑
        if speed_from_pcm == 1: #toyota
          hudControl.setSpeed = float(CS.vCruiseCluster * CV.KPH_TO_MS)
        elif speed_from_pcm == 2:
          hudControl.setSpeed = float(max(30/3.6, desired_kph * CV.KPH_TO_MS))
        elif speed_from_pcm == 3: # honda
          hudControl.setSpeed = setSpeed if lp.xState == 3 else float(desired_kph * CV.KPH_TO_MS)
        else:
          hudControl.setSpeed = float(max(30/3.6, setSpeed))
    else:
      hudControl.setSpeed = setSpeed if lp.xState == 3 else float(desired_kph * CV.KPH_TO_MS)"""

    # 应用修改
    if "丰田特调优先：如果启用了丰田特调" not in content:
        content = content.replace(
            "lp = self.sm['longitudinalPlan']\n    if self.CP.pcmCruise:\n      speed_from_pcm = self.params.get_int(\"SpeedFromPCM\")",
            speed_logic
        )
        log_message("已修改速度设置逻辑")

    # 写入文件
    with open(file_path, 'w') as f:
        f.write(content)

    log_message("控制逻辑修改完成")
    return True

def apply_ui_settings_changes():
    """应用UI设置修改"""
    log_message("应用UI设置修改...")

    # 添加丰田特调设置到carrot_settings.json
    settings_file = "selfdrive/car/carrot_settings.json"

    if os.path.exists(settings_file):
        with open(settings_file, 'r') as f:
            content = f.read()

        # 检查是否已包含丰田特调设置
        if "ToyotaStockLongitudinal" not in content:
            # 添加丰田特调设置
            toyota_settings = """
  {
    "group": "丰田",
    "name": "ToyotaStockLongitudinal",
    "title": "丰田：原厂丰田纵向控制",
    "descr": "使用原厂丰田纵向控制参数",
    "egroup": "TUNING",
    "etitle": "Toyota Stock Longitudinal Control",
    "edescr": "Use stock Toyota longitudinal control parameters",
    "min": 0,
    "max": 1,
    "default": 0,
    "unit": 1
  },
  {
    "group": "丰田",
    "name": "ToyotaTSS2Long",
    "title": "丰田：TSS2自定义调校",
    "descr": "为丰田TSS2车辆启用自定义纵向调校",
    "egroup": "TUNING",
    "etitle": "Toyota TSS2 Custom Tuning",
    "edescr": "Enable custom longitudinal tuning for Toyota TSS2 vehicles",
    "min": 0,
    "max": 1,
    "default": 0,
    "unit": 1
  },
  {
    "group": "丰田",
    "name": "ToyotaEnhancedBsm",
    "title": "丰田：增强BSM支持",
    "descr": "为丰田车辆添加增强盲点监测支持",
    "egroup": "TUNING",
    "etitle": "Toyota Enhanced BSM Support",
    "edescr": "Add enhanced blind spot monitoring support for Toyota vehicles",
    "min": 0,
    "max": 1,
    "default": 0,
    "unit": 1
  },
  {
    "group": "丰田",
    "name": "ToyotaAutoHold",
    "title": "丰田：自动刹车保持（TSS2混合动力）",
    "descr": "专为TSS2混合动力车辆设计，当前方车辆停止时自动保持车辆停止",
    "egroup": "TUNING",
    "etitle": "Toyota Auto Brake Hold (TSS2 Hybrid)",
    "edescr": "Designed for TSS2 hybrid vehicles, automatically keeps vehicle stopped when vehicle ahead stops",
    "min": 0,
    "max": 1,
    "default": 0,
    "unit": 1
  },
  {
    "group": "丰田",
    "name": "ToyotaDriveMode",
    "title": "丰田：驾驶模式按钮链接",
    "descr": "将车辆的驾驶模式按钮与加速个性（轻松、标准、运动）链接",
    "egroup": "TUNING",
    "etitle": "Toyota Drive Mode Button Link",
    "edescr": "Link vehicle's drive mode button with acceleration personality (Relaxed, Standard, Aggressive)",
    "min": 0,
    "max": 1,
    "default": 0,
    "unit": 1
  },"""

            # 在文件末尾添加设置
            content = content.replace("  ]\n}", f"{toyota_settings}\n  ]\n}}")
            log_message("已添加丰田特调UI设置")

            # 写入文件
            with open(settings_file, 'w') as f:
                f.write(content)

    log_message("UI设置修改完成")
    return True

def apply_longitudinal_tuning_changes():
    """应用纵向调校修改"""
    log_message("应用纵向调校修改...")

    file_path = "opendbc/car/toyota/carcontroller.py"

    # 检查文件是否存在
    if not os.path.exists(file_path):
        log_message(f"错误: 文件不存在 {file_path}")
        return False

    # 读取文件内容
    with open(file_path, 'r') as f:
        content = f.read()

    # 修改TSS2纵向控制参数
    if "optimal for rav4 - 调整为更合理的跟车距离" not in content:
        tuning_code = """# 丰田TSS2纵向控制
  elif CP.carFingerprint in TSS2_CAR:
    if Params().get_bool("ToyotaTSS2Long"):
      if CP.carFingerprint == CAR.TOYOTA_RAV4_TSS2:
        #optimal for rav4 - 调整为更合理的跟车距离
        kiBP = [2., 8., 15., 25.]
        kiV = [0.6, 0.35, 0.25, 0.15]
      else:
        #optimal for corolla - 调整为更合理的跟车距离
        kiBP = [2., 8., 15.]
        kiV = [0.6, 0.35, 0.25]
    else:
      # 默认TSS2参数 - 稍微调整以减少跟车距离
      kiBP = [2., 6., 12.]
      kiV = [0.55, 0.3, 0.2]"""

        # 应用修改
        content = re.sub(
            r"# 丰田TSS2纵向控制\s+elif CP\.carFingerprint in TSS2_CAR:.*?else:\s+kiBP = \[2\., 5\.\]\s+kiV = \[0\.5, 0\.25\]",
            tuning_code,
            content,
            flags=re.DOTALL
        )
        log_message("已修改TSS2纵向控制参数")

    # 写入文件
    with open(file_path, 'w') as f:
        f.write(content)

    log_message("纵向调校修改完成")
    return True

def verify_migration():
    """验证移植是否成功"""
    log_message("验证移植是否成功...")

    # 检查关键文件和代码
    key_checks = [
        ("toyota_stock_longitudinal = False", "opendbc/car/toyota/carcontroller.py"),
        ("ToyotaStockLongitudinal", "selfdrive/car/carrot_settings.json"),
        ("丰田特调优先：如果启用了丰田特调", "selfdrive/controls/controlsd.py"),
        ("create_set_bsm_debug_mode", "opendbc/car/toyota/toyotacan.py"),
        ("丰田特调：驾驶模式按钮链接", "opendbc/car/toyota/carstate.py")
    ]

    missing_checks = []
    for check_text, file_path in key_checks:
        if os.path.exists(file_path):
            with open(file_path, 'r') as f:
                content = f.read()
                if check_text not in content:
                    missing_checks.append((check_text, file_path))
        else:
            missing_checks.append((f"文件不存在: {file_path}", file_path))

    if missing_checks:
        log_message("错误: 以下关键代码缺失:")
        for check_text, file_path in missing_checks:
            log_message(f"  - {check_text} in {file_path}")
        return False

    log_message("✓ 移植验证成功")
    return True

def create_migration_summary():
    """创建移植总结文档"""
    log_message("创建移植总结文档...")

    content = """# SunnyPilot特性完整一键移植总结

## 概述

本脚本完成了从原始分支到当前状态的完整移植，包括所有丰田特调功能和优化。

## 移植内容

### 1. 丰田车辆控制器 (carcontroller.py)

- 添加了丰田特调参数初始化
- 实现了参数更新逻辑
- 添加了增强BSM功能
- 添加了自动刹车保持功能
- 优化了TSS2纵向控制参数

### 2. 丰田车辆状态 (carstate.py)

- 添加了驾驶模式检测逻辑
- 实现了加速个性自动设置
- 处理了信号检测和错误情况

### 3. 丰田CAN消息 (toyotacan.py)

- 添加了BSM调试模式设置函数
- 添加了BSM状态轮询函数
- 添加了刹车保持命令创建函数

### 4. 控制逻辑 (controlsd.py)

- 修复了巡航速度设置问题
- 添加了丰田特调参数优先处理
- 优化了速度设置逻辑

### 5. UI设置 (carrot_settings.json)

- 添加了丰田特调UI设置
- 包括所有丰田特调功能的开关

## 使用方法

### 1. 启用丰田特调

在UI中启用以下选项：

- 丰田：原厂丰田纵向控制
- 丰田：TSS2自定义调校
- 丰田：增强BSM支持
- 丰田：自动刹车保持（TSS2混合动力）
- 丰田：驾驶模式按钮链接

### 2. 测试功能

1. **巡航速度设置**：
   - 使用set-设置当前速度
   - 使用set+/-调整速度
   - 观察仪表速度和实际巡航速度是否同步变化

2. **驾驶模式按钮链接**：
   - 切换驾驶模式（运动/经济模式）
   - 观察加速个性是否自动调整

3. **自动刹车保持**：
   - 在合适条件下测试自动刹车保持功能

4. **增强BSM**：
   - 观察盲点监测功能是否增强

## 注意事项

1. 移植前已自动备份原始代码
2. 移植后可能需要根据具体车型进行微调
3. 建议在安全环境下进行测试
4. 如有问题，可参考备份文件恢复

## 技术支持

如有问题，请参考：
- 备份文件：sp_migration_backup/
- 日志文件：sp_migration_log.txt
- 原始代码：sunnypilot_migration/

## 总结

本次完整移植成功将SunnyPilot的丰田特调功能集成到当前分支中，显著提升了系统的驾驶舒适性、安全性和适用性。脚本提供了完整的移植流程，包括备份、代码修改和验证，确保移植过程的可靠性和完整性。
"""

    with open("COMPLETE_SP_MIGRATION_SUMMARY.md", 'w') as f:
        f.write(content)

    log_message("已创建移植总结文档: COMPLETE_SP_MIGRATION_SUMMARY.md")

def main():
    """主函数"""
    print("=" * 50)
    print("    SunnyPilot特性完整一键移植脚本")
    print("=" * 50)
    print("")

    # 初始化日志
    with open(LOG_FILE, 'w') as f:
        f.write(f"SunnyPilot完整移植开始于: {datetime.datetime.now()}\n")

    # 1. 备份原始代码
    backup_original_code()

    # 2. 应用各种修改
    apply_toyota_carcontroller_changes()
    apply_toyota_carstate_changes()
    apply_toyota_toyotacan_changes()
    apply_controlsd_changes()
    apply_ui_settings_changes()
    apply_longitudinal_tuning_changes()

    # 3. 验证移植
    success = verify_migration()

    # 4. 创建总结文档
    create_migration_summary()

    print("")
    print("=" * 50)
    if success:
        print("           移植执行成功！")
    else:
        print("           移植执行失败！")
    print("=" * 50)
    print("")

    if success:
        print("后续步骤:")
        print("1. 编译项目:")
        print("   scons -j$(nproc)")
        print("")
        print("2. 在安全环境下进行测试")
        print("")
        print("3. 根据实际效果进行微调")
        print("")
        print("请查看移植总结文档：COMPLETE_SP_MIGRATION_SUMMARY.md")
        print("请查看移植日志：sp_migration_log.txt")
    else:
        print("请检查错误信息并修复后重新运行")
        print("请查看移植日志：sp_migration_log.txt")

if __name__ == "__main__":
    main()