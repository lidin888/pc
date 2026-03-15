#!/usr/bin/env python3
"""检查 openpilot 是否在使用 USB IMU"""

import os
import sys
import time

# 添加 openpilot 路径
sys.path.insert(0, "/home/lanyi/sunnypilot")

def check_device():
    """检查 USB 设备是否存在"""
    if os.path.exists("/dev/ttyUSB0"):
        print("✅ USB IMU 设备: /dev/ttyUSB0 存在")
        return True
    else:
        print("❌ USB IMU 设备: /dev/ttyUSB0 不存在")
        return False

def check_process():
    """检查是否有 ybimu_sensord 进程在运行"""
    import subprocess
    result = subprocess.run(['pgrep', '-f', 'ybimu_sensord'], 
                          capture_output=True, text=True)
    if result.returncode == 0:
        print(f"✅ ybimu_sensord 进程运行中 (PID: {result.stdout.strip()})")
        return True
    else:
        print("❌ ybimu_sensord 进程未运行")
        # 检查标准 sensord
        result2 = subprocess.run(['pgrep', '-f', 'sensord.py'], 
                               capture_output=True, text=True)
        if result2.returncode == 0:
            print(f"   (正在使用标准 I2C sensord, PID: {result2.stdout.strip()})")
        return False

def check_data_stream():
    """检查是否在接收 IMU 数据"""
    try:
        import cereal.messaging as messaging
        
        print("\n检查数据流 (等待 2 秒)...")
        sm = messaging.SubMaster(['accelerometer', 'gyroscope'], timeout=100)
        
        acc_count = 0
        gyro_count = 0
        
        for _ in range(20):  # 2 秒
            sm.update(100)
            if sm.updated['accelerometer']:
                acc_count += 1
            if sm.updated['gyroscope']:
                gyro_count += 1
            time.sleep(0.1)
        
        if acc_count > 0:
            print(f"✅ 加速度计数据: 接收到 {acc_count} 个消息")
            acc = sm['accelerometer'].acceleration.v
            print(f"   最新数据: [{acc[0]:.2f}, {acc[1]:.2f}, {acc[2]:.2f}] m/s²")
        else:
            print("❌ 加速度计数据: 未接收到数据")
            
        if gyro_count > 0:
            print(f"✅ 陀螺仪数据: 接收到 {gyro_count} 个消息")
            gyro = sm['gyroscope'].gyroUncalibrated.v
            print(f"   最新数据: [{gyro[0]:.3f}, {gyro[1]:.3f}, {gyro[2]:.3f}] rad/s")
        else:
            print("❌ 陀螺仪数据: 未接收到数据")
            
        return acc_count > 0 and gyro_count > 0
        
    except Exception as e:
        print(f"❌ 检查数据流时出错: {e}")
        return False

def check_manager_config():
    """检查 manager 配置"""
    is_usb_imu = os.getenv("USE_USB_IMU") or os.path.exists(os.getenv("YBIMU_PORT", "/dev/ttyUSB0"))
    if is_usb_imu:
        print("✅ Manager 配置: 将使用 ybimu_sensord (USB IMU)")
    else:
        print("❌ Manager 配置: 将使用标准 sensord (I2C IMU)")
    return is_usb_imu

if __name__ == "__main__":
    print("=" * 60)
    print("检查 openpilot 是否使用 USB IMU")
    print("=" * 60)
    
    print("\n1️⃣  设备检查:")
    dev_ok = check_device()
    
    print("\n2️⃣  配置检查:")
    cfg_ok = check_manager_config()
    
    print("\n3️⃣  进程检查:")
    proc_ok = check_process()
    
    print("\n4️⃣  数据流检查:")
    data_ok = check_data_stream()
    
    print("\n" + "=" * 60)
    if dev_ok and proc_ok and data_ok:
        print("✅ 结论: openpilot 正在使用 USB IMU")
    elif dev_ok and cfg_ok:
        print("⚠️  结论: USB IMU 已配置，但需要启动 openpilot")
    else:
        print("❌ 结论: openpilot 未使用 USB IMU")
    print("=" * 60)
