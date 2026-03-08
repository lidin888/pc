#!/usr/bin/env python3
"""
手动启用模拟器控制的脚本
用于解决车辆不动的问题
"""

import time
import subprocess
from openpilot.common.params import Params

def enable_control():
    """启用openpilot控制"""
    params = Params()

    print("=== 启用模拟器控制 ===")

    # 1. 确保参数正确设置
    print("1. 设置控制参数...")
    params.put_bool('AlphaLongitudinalEnabled', True)
    params.put_bool('ExperimentalMode', True)
    params.put('CruiseSpeed1', '40')  # 40 km/h

    # 2. 启用openpilot
    print("2. 启用openpilot...")
    params.put_bool('IsOpenpilotEnabled', True)

    # 3. 设置巡航控制
    print("3. 设置巡航控制...")
    params.put_bool('CruiseControlEnabled', True)

    # 4. 发送控制命令
    print("4. 发送控制命令...")

    # 尝试通过CAN总线发送控制命令
    try:
        # 启用纵向控制
        params.put_bool('LongitudinalControl', True)

        # 设置初始速度
        params.put('vCruise', '40')  # 40 km/h

        print("✓ 控制参数设置完成")
    except Exception as e:
        print(f"⚠ 设置参数时出错: {e}")

    print("5. 等待系统启动...")
    time.sleep(5)

    print("=== 控制启用完成 ===")
    print("如果车辆仍然不动，请检查:")
    print("1. 模拟器窗口是否正常显示")
    print("2. 车辆前方是否有障碍物")
    print("3. 尝试按 'i' 键启动点火")
    print("4. 尝试按 '1' 键启用巡航")

def check_simulator_status():
    """检查模拟器状态"""
    print("=== 检查模拟器状态 ===")

    # 检查桥接进程
    try:
        result = subprocess.run(['pgrep', '-f', 'run_bridge.py'], capture_output=True, text=True)
        if result.returncode == 0:
            print("✓ 模拟器桥接进程正在运行")
        else:
            print("✗ 模拟器桥接进程未运行")
    except Exception as e:
        print(f"⚠ 检查桥接进程时出错: {e}")

    # 检查manager进程
    try:
        result = subprocess.run(['pgrep', '-f', 'manager.py'], capture_output=True, text=True)
        if result.returncode == 0:
            print("✓ openpilot manager进程正在运行")
        else:
            print("✗ openpilot manager进程未运行")
    except Exception as e:
        print(f"⚠ 检查manager进程时出错: {e}")

    # 检查参数状态
    params = Params()

    is_enabled = params.get_bool('IsOpenpilotEnabled')
    print(f"✓ openpilot启用状态: {is_enabled}")

    experimental_mode = params.get_bool('ExperimentalMode')
    print(f"✓ 实验模式状态: {experimental_mode}")

if __name__ == "__main__":
    print("模拟器控制启用脚本")
    print("=" * 50)

    # 检查状态
    check_simulator_status()

    # 询问是否启用控制
    response = input("\n是否启用控制? (y/n): ")
    if response.lower() in ['y', 'yes']:
        enable_control()
    else:
        print("已取消启用控制")