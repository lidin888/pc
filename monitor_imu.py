#!/usr/bin/env python3
"""
USB IMU 实时监控脚本
Monitor Yahboom USB IMU in real-time
"""

import sys
import time
import subprocess
from collections import deque

try:
    import cereal.messaging as messaging
except ImportError:
    print("Error: cereal not installed")
    sys.exit(1)

def check_process():
    """检查使用的IMU驱动"""
    try:
        result = subprocess.run(['ps', 'aux'], capture_output=True, text=True, timeout=5)

        # 检查各种可能的进程名
        lines = result.stdout.lower()

        if 'ybimu_sensord' in lines:
            return "✅ USB IMU (ybimu_sensord)"
        elif 'sensord' in lines and 'python' in lines:
            if 'sensord.py' in result.stdout:
                return "📍 标准驱动 (sensord.py - I2C IMU)"
            else:
                return "📍 IMU驱动 (sensord)"
        else:
            return "❓ IMU驱动未找到 (未启动或进程名不同)"
    except Exception as e:
        return f"⚠️ 检测失败: {e}"

def monitor_imu_data():
    """实时监控IMU数据"""
    try:
        sm = messaging.SubMaster(['accelerometer', 'gyroscope', 'magnetometer'])

        # 数据缓冲，用于计算频率
        accel_times = deque(maxlen=100)
        gyro_times = deque(maxlen=100)

        print("\n" + "="*70)
        print("🎯 USB IMU 实时监控")
        print("="*70)
        print(f"驱动: {check_process()}")
        print("="*70)
        print(f"{'时间':<12} {'加速度(m/s²)':<25} {'陀螺仪(rad/s)':<25} {'频率':<8}")
        print("="*70)

        start_time = time.time()
        updates = 0

        while True:
            sm.update()
            updates += 1

            # 获取加速度数据
            accel_valid = sm.updated['accelerometer']
            gyro_valid = sm.updated['gyroscope']
            mag_valid = sm.updated['magnetometer']

            if accel_valid or gyro_valid or mag_valid:
                elapsed = time.time() - start_time

                # 提取数据
                if accel_valid:
                    accel = sm['accelerometer'].acceleration.v
                    accel_times.append(time.time())
                    accel_str = f"[{accel[0]:6.2f}, {accel[1]:6.2f}, {accel[2]:6.2f}]"
                else:
                    accel_str = "[  --,   --,   --]"

                if gyro_valid:
                    gyro = sm['gyroscope'].gyroUncalibrated.v
                    gyro_times.append(time.time())
                    gyro_str = f"[{gyro[0]:6.3f}, {gyro[1]:6.3f}, {gyro[2]:6.3f}]"
                else:
                    gyro_str = "[  --,   --,   --]"

                # 计算频率
                if len(accel_times) > 1:
                    freq = len(accel_times) / (accel_times[-1] - accel_times[0]) if accel_times[-1] != accel_times[0] else 0
                    freq_str = f"{freq:.1f} Hz"
                else:
                    freq_str = "-- Hz"

                # 打印一行
                print(f"{elapsed:>10.2f}s  {accel_str}  {gyro_str}  {freq_str}")

                # 每100条数据显示一次状态
                if updates % 100 == 0:
                    indicator = "✅" if (accel_valid or gyro_valid) else "❌"
                    print(f"{indicator} 状态: 正在工作")

            time.sleep(0.01)

    except KeyboardInterrupt:
        print("\n" + "="*70)
        print("✋ 监控已停止")
        print("="*70)
    except Exception as e:
        print(f"❌ 错误: {e}")
        sys.exit(1)

def quick_check():
    """快速检查"""
    print("\n🔍 快速检查 IMU 状态...")
    print(f"驱动类型: {check_process()}")

    # 尝试连接一次
    try:
        print("尝试连接数据流...", end=" ", flush=True)
        sm = messaging.SubMaster(['accelerometer', 'gyroscope', 'magnetometer'])

        # 等待数据，最多尝试10次
        for i in range(10):
            sm.update(0)
            if sm.updated['accelerometer']:
                accel = sm['accelerometer'].acceleration.v
                print(f"✅ 成功!")
                print(f"当前加速度: {accel}")
                if sm.updated['gyroscope']:
                    gyro = sm['gyroscope'].gyroUncalibrated.v
                    print(f"当前陀螺仪: {gyro}")
                return
            time.sleep(0.1)

        print("❌ 无数据 (驱动可能未启动)")
    except Exception as e:
        print(f"❌ 错误: {type(e).__name__}: {e}")
        print("\n💡 提示: 确保以下其中任意一个条件满足:")
        print("   1. USB IMU已连接到 /dev/ttyUSB0")
        print("   2. OP系统已启动并加载了sensord驱动")
        print("   3. 尝试手动启动: python3 system/sensord/ybimu_sensord.py")
        sys.exit(1)

if __name__ == "__main__":
    if len(sys.argv) > 1 and sys.argv[1] == "--quick":
        quick_check()
    else:
        monitor_imu_data()
