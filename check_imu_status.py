#!/usr/bin/env python3
"""
检查 openpilot 是否在使用 USB IMU
Check if openpilot is using USB IMU
"""

import os
import sys
import time
import subprocess

# 添加 openpilot 路径
sys.path.insert(0, "/home/lanyi/sunnypilot")

def check_device():
    """检查 USB 设备是否存在"""
    import glob
    devices = glob.glob("/dev/ttyUSB*") + glob.glob("/dev/ttyACM*")
    if devices:
        print(f"✅ 检测到 USB 串口设备: {', '.join(devices)}")
        return True
    else:
        print("❌ 未检测到 USB IMU 设备")
        return False

def check_process():
    """检查是否有 ybimu_sensord 进程在运行"""
    result = subprocess.run(['pgrep', '-f', 'ybimu_sensord'],
                          capture_output=True, text=True)
    if result.returncode == 0:
        pid = result.stdout.strip()
        print(f"✅ USB IMU 驱动正在运行 (ybimu_sensord, PID: {pid})")
        return True
    else:
        # 检查标准 sensord
        result2 = subprocess.run(['pgrep', '-f', 'sensord.py'],
                               capture_output=True, text=True)
        if result2.returncode == 0:
            print(f"📍 正在使用板载 I2C IMU (sensord.py, PID: {result2.stdout.strip()})")
        else:
            print("❌ 未检测到任何 IMU 驱动进程")
        return False

def check_sensor_messages():
    """检查传感器消息来源"""
    try:
        import cereal.messaging as messaging
        from cereal import log

        print("\n检查传感器数据源 (等待 3 秒)...")
        sm = messaging.SubMaster(['accelerometer', 'gyroscope'])

        sensor_sources = set()
        for _ in range(30):  # 3 秒
            sm.update(100)

            if sm.updated['accelerometer']:
                source = sm['accelerometer'].sensor
                sensor_sources.add(source)

            if sm.updated['gyroscope']:
                source = sm['gyroscope'].sensor
                sensor_sources.add(source)

            time.sleep(0.1)

        if sensor_sources:
            print("\n传感器来源:")
            for source in sensor_sources:
                source_name = log.SensorEventData.SensorSource.schema.enumerants.get(source, "未知")

                if source == log.SensorEventData.SensorSource.lsm6ds3trc:
                    print(f"  ✅ {source_name} (USB IMU - Yahboom)")
                elif source == log.SensorEventData.SensorSource.lsm6ds3:
                    print(f"  📍 {source_name} (板载 IMU - comma three)")
                elif source == log.SensorEventData.SensorSource.bmx055:
                    print(f"  📍 {source_name} (板载 IMU - comma two)")
                else:
                    print(f"  ❓ {source_name}")
            return True
        else:
            print("❌ 未接收到传感器数据")
            return False

    except Exception as e:
        print(f"⚠️  无法检查传感器消息: {e}")
        return False

def check_env():
    """检查环境变量配置"""
    use_usb_imu = os.getenv("USE_USB_IMU")
    ybimu_port = os.getenv("YBIMU_PORT")

    if use_usb_imu or ybimu_port:
        print("\n环境变量配置:")
        if use_usb_imu:
            print(f"  USE_USB_IMU = {use_usb_imu}")
        if ybimu_port:
            print(f"  YBIMU_PORT = {ybimu_port}")
        return True
    return False

def main():
    print("=" * 70)
    print("         检查 openpilot 是否使用 USB IMU")
    print("=" * 70)

    print("\n【1】设备检查")
    print("─" * 70)
    dev_ok = check_device()

    print("\n【2】进程检查")
    print("─" * 70)
    proc_ok = check_process()

    print("\n【3】环境变量检查")
    print("─" * 70)
    env_set = check_env()
    if not env_set:
        print("  (未设置 USB IMU 相关环境变量)")

    print("\n【4】传感器数据检查")
    print("─" * 70)
    sensor_ok = check_sensor_messages()

    print("\n" + "=" * 70)
    print("【结论】")
    print("=" * 70)

    if proc_ok and sensor_ok:
        print("✅ openpilot 正在使用 USB IMU")
    elif dev_ok and (env_set or os.path.exists("/dev/ttyUSB0")):
        print("⚠️  USB IMU 已配置但未运行")
        print("   提示: 需要启动 openpilot 或手动运行 ybimu_sensord")
    else:
        print("❌ openpilot 未使用 USB IMU")
        if not dev_ok:
            print("   提示: 请先连接 USB IMU 设备")

    print("=" * 70)

if __name__ == "__main__":
    main()
