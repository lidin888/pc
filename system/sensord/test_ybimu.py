#!/usr/bin/env python3
"""
测试Yahboom IMU并显示实时数据
"""

import time
import sys
import os
import glob
from YbImuLib import YbImuSerial

def find_imu_port():
    """自动查找可用的 USB IMU 设备"""
    # 优先使用环境变量
    if os.getenv("YBIMU_PORT"):
        return os.getenv("YBIMU_PORT")

    # 查找所有 USB 串口设备
    usb_devices = glob.glob("/dev/ttyUSB*") + glob.glob("/dev/ttyACM*")
    if usb_devices:
        return sorted(usb_devices)[0]  # 返回第一个

    return "/dev/ttyUSB0"  # 默认值

SERIAL_PORT = find_imu_port()

def main():
    print(f"正在连接Yahboom IMU ({SERIAL_PORT})...")

    try:
        imu = YbImuSerial(SERIAL_PORT, debug=False)
        imu.create_receive_threading()
        print("✓ IMU连接成功!")
    except Exception as e:
        print(f"✗ 连接失败: {e}")
        return 1

    time.sleep(0.5)

    # 尝试读取版本
    try:
        version = imu.get_version()
        print(f"固件版本: {version}")
    except:
        print("固件版本: 无法读取（串口模式正常）")

    print("\n开始读取数据 (按Ctrl+C退出)...\n")

    try:
        while True:
            # 读取所有传感器数据
            ax, ay, az = imu.get_accelerometer_data()
            gx, gy, gz = imu.get_gyroscope_data()
            mx, my, mz = imu.get_magnetometer_data()
            qw, qx, qy, qz = imu.get_imu_quaternion_data()
            roll, pitch, yaw = imu.get_imu_attitude_data(ToAngle=True)

            # 清屏并显示
            print("\033[2J\033[H", end="")  # 清屏
            print("=" * 70)
            print("                  Yahboom 9轴IMU 实时数据")
            print("=" * 70)
            print()
            print(f"加速度 [g]:       X={ax:7.3f}  Y={ay:7.3f}  Z={az:7.3f}")
            print(f"加速度 [m/s²]:    X={ax*9.81:7.3f}  Y={ay*9.81:7.3f}  Z={az*9.81:7.3f}")
            print()
            print(f"角速度 [rad/s]:   X={gx:7.3f}  Y={gy:7.3f}  Z={gz:7.3f}")
            print(f"角速度 [deg/s]:   X={gx*57.3:7.1f}  Y={gy*57.3:7.1f}  Z={gz*57.3:7.1f}")
            print()
            print(f"磁场 [uT]:        X={mx:7.1f}  Y={my:7.1f}  Z={mz:7.1f}")
            print()
            print(f"四元数:           W={qw:7.4f}  X={qx:7.4f}  Y={qy:7.4f}  Z={qz:7.4f}")
            print()
            print(f"姿态角 [deg]:     Roll={roll:7.2f}°  Pitch={pitch:7.2f}°  Yaw={yaw:7.2f}°")
            print()
            print("=" * 70)
            print("提示: 移动IMU查看数据变化")
            print("     按 Ctrl+C 退出程序")
            print("=" * 70)

            time.sleep(0.1)

    except KeyboardInterrupt:
        print("\n\n程序已退出")
        return 0

if __name__ == "__main__":
    sys.exit(main())
