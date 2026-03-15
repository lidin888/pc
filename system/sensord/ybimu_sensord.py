#!/usr/bin/env python3
"""
Yahboom 9-axis IMU sensor daemon for sunnypilot
将Yahboom九轴IMU数据发布到sunnypilot消息系统
"""

import time
import os
import sys
from typing import Optional

from cereal import log
import cereal.messaging as messaging
from openpilot.common.swaglog import cloudlog
from openpilot.common.realtime import config_realtime_process, Ratekeeper

try:
    from YbImuLib import YbImuSerial
except ImportError:
    cloudlog.error("YbImuLib not installed. Please install it first.")
    sys.exit(1)

import glob

def find_imu_port():
    """自动查找可用的 USB IMU 设备"""
    # 优先使用环境变量
    if os.getenv("YBIMU_PORT"):
        port = os.getenv("YBIMU_PORT")
        cloudlog.info(f"Using IMU port from env: {port}")
        return port

    # 查找所有 USB 串口设备
    usb_devices = glob.glob("/dev/ttyUSB*") + glob.glob("/dev/ttyACM*")
    if usb_devices:
        port = sorted(usb_devices)[0]
        cloudlog.info(f"Auto-detected IMU port: {port}")
        return port

    cloudlog.warning("No USB serial device found, using default /dev/ttyUSB0")
    return "/dev/ttyUSB0"

# IMU串口设备路径配置
SERIAL_PORT = find_imu_port()

# 传感器采样频率 (Hz)
SENSOR_FREQUENCY = 100  # Yahboom IMU typical rate


class YbImuDriver:
    """Yahboom IMU驱动，负责读取数据并发布消息"""

    def __init__(self, port: str = SERIAL_PORT):
        self.port = port
        self.imu: Optional[YbImuSerial] = None
        self.pm = messaging.PubMaster(['accelerometer', 'gyroscope', 'magnetometer'])

        # 统计信息
        self.error_count = 0
        self.last_good_read = time.monotonic()

    def connect(self) -> bool:
        """连接IMU设备"""
        try:
            cloudlog.info(f"Connecting to Yahboom IMU on {self.port}...")
            self.imu = YbImuSerial(self.port, debug=False)
            # 启动后台线程持续解析数据帧
            self.imu.create_receive_threading()

            # 等待一小段时间让设备初始化
            time.sleep(0.5)

            # 尝试读取版本信息（可能失败，串口模式不一定支持）
            try:
                version = self.imu.get_version()
                cloudlog.info(f"Yahboom IMU firmware version: {version}")
            except Exception:
                cloudlog.warning("Cannot read firmware version (normal in serial mode)")

            cloudlog.info("Yahboom IMU connected successfully")
            return True

        except Exception as e:
            cloudlog.error(f"Failed to connect to Yahboom IMU: {e}")
            return False

    def read_and_publish(self) -> bool:
        """读取IMU数据并发布消息"""
        if self.imu is None:
            return False

        try:
            # 读取加速度计数据 (单位: g)
            ax, ay, az = self.imu.get_accelerometer_data()

            # 读取陀螺仪数据 (单位: rad/s)
            gx, gy, gz = self.imu.get_gyroscope_data()

            # 读取磁力计数据 (单位: uT)
            mx, my, mz = self.imu.get_magnetometer_data()

            # 发布加速度计消息
            # sunnypilot期望的加速度单位是 m/s²，所以需要乘以重力常数
            # 坐标系转换: Yahboom IMU -> sunnypilot (NED坐标系)
            acc_msg = messaging.new_message('accelerometer', valid=True)
            acc_msg.accelerometer.sensor = log.SensorEventData.SensorSource.lsm6ds3trc
            acc_msg.accelerometer.type = 1  # SENSOR_TYPE_ACCELEROMETER
            acc_msg.accelerometer.timestamp = acc_msg.logMonoTime  # 使用消息自带的单调时钟时间
            acc_msg.accelerometer.init('acceleration')
            # 转换到m/s²并调整坐标系 (根据实际IMU安装方向可能需要调整)
            acc_msg.accelerometer.acceleration.v = [ax * 9.81, ay * 9.81, az * 9.81]
            self.pm.send('accelerometer', acc_msg)

            # 发布陀螺仪消息
            # 坐标系转换: Yahboom IMU -> sunnypilot
            gyro_msg = messaging.new_message('gyroscope', valid=True)
            gyro_msg.gyroscope.sensor = log.SensorEventData.SensorSource.lsm6ds3trc
            gyro_msg.gyroscope.type = 16  # SENSOR_TYPE_GYROSCOPE_UNCALIBRATED
            gyro_msg.gyroscope.timestamp = gyro_msg.logMonoTime  # 使用消息自带的单调时钟时间
            gyro_msg.gyroscope.init('gyroUncalibrated')
            gyro_msg.gyroscope.gyroUncalibrated.v = [gx, gy, gz]
            self.pm.send('gyroscope', gyro_msg)

            # 发布磁力计消息
            mag_msg = messaging.new_message('magnetometer', valid=True)
            mag_msg.magnetometer.sensor = log.SensorEventData.SensorSource.mmc5603nj
            mag_msg.magnetometer.type = 14  # SENSOR_TYPE_MAGNETIC_FIELD_UNCALIBRATED
            mag_msg.magnetometer.timestamp = mag_msg.logMonoTime  # 使用消息自带的单调时钟时间
            mag_msg.magnetometer.init('magneticUncalibrated')
            mag_msg.magnetometer.magneticUncalibrated.v = [mx, my, mz]
            self.pm.send('magnetometer', mag_msg)

            # 更新统计
            self.last_good_read = time.monotonic()
            self.error_count = 0

            return True

        except Exception as e:
            self.error_count += 1
            if self.error_count % 100 == 1:  # 每100次错误打印一次
                cloudlog.error(f"Error reading IMU data: {e}")

            # 如果错误太多，尝试重连
            if self.error_count > 1000:
                cloudlog.warning("Too many errors, attempting to reconnect...")
                self.reconnect()

            return False

    def reconnect(self):
        """重新连接IMU"""
        try:
            if self.imu:
                del self.imu
                self.imu = None
        except:
            pass

        time.sleep(1.0)
        self.connect()
        self.error_count = 0

    def check_health(self) -> bool:
        """检查IMU健康状态"""
        if self.imu is None:
            return False

        # 如果超过1秒没有成功读取，认为不健康
        time_since_good_read = time.monotonic() - self.last_good_read
        if time_since_good_read > 1.0:
            cloudlog.warning(f"No good IMU reads for {time_since_good_read:.1f}s")
            return False

        return True


def main():
    """主循环"""
    # 配置实时进程
    config_realtime_process([1, 2, 3], 5)

    cloudlog.info("Starting Yahboom IMU sensor daemon...")

    # 创建驱动实例
    driver = YbImuDriver(SERIAL_PORT)

    # 连接IMU
    if not driver.connect():
        cloudlog.error("Failed to connect to IMU, exiting...")
        return 1

    # 创建速率控制器
    rk = Ratekeeper(SENSOR_FREQUENCY, print_delay_threshold=0.05)

    # 主循环
    last_health_check = time.monotonic()

    try:
        while True:
            # 读取并发布数据
            driver.read_and_publish()

            # 定期健康检查
            if time.monotonic() - last_health_check > 5.0:
                if not driver.check_health():
                    cloudlog.warning("IMU health check failed, attempting reconnect...")
                    driver.reconnect()
                last_health_check = time.monotonic()

            # 保持固定频率
            rk.keep_time()

    except KeyboardInterrupt:
        cloudlog.info("Shutting down Yahboom IMU sensor daemon...")
        return 0
    except Exception as e:
        cloudlog.error(f"Unexpected error in main loop: {e}")
        return 1


if __name__ == "__main__":
    sys.exit(main())
