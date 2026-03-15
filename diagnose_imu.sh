#!/bin/bash
# 完整IMU诊断脚本

echo "🔧 Yahboom USB IMU 完整诊断"
echo "════════════════════════════════════════════════════════"

# 1. USB设备检查
echo -e "\n1️⃣  USB 设备检查"
echo "─────────────────────────────────────────────────────────"
if command -v lsusb &> /dev/null; then
    echo "USB设备列表:"
    lsusb | grep -i "serial\|usb\|prolific\|cp210" || lsusb | head -5
else
    echo "lsusb命令未装"
fi

echo -e "\n串口设备:"
ls -la /dev/ttyUSB* 2>/dev/null || echo "❌ 没有找到 /dev/ttyUSB*"
ls -la /dev/ttyACM* 2>/dev/null || echo "❌ 没有找到 /dev/ttyACM*"

# 2. Python环境检查
echo -e "\n2️⃣  Python 环境检查"
echo "─────────────────────────────────────────────────────────"
python3 << 'PYEOF'
import sys
print(f"Python版本: {sys.version}")

# 检查cereal库
try:
    import cereal.messaging as messaging
    print("✅ cereal库已安装")
except ImportError:
    print("❌ cereal库未安装")

# 检查YbImuLib
try:
    from YbImuLib import YbImuSerial
    print("✅ YbImuLib已安装")
except ImportError:
    print("❌ YbImuLib未安装")
    print("   安装: pip3 install YbImuLib")

PYEOF

# 3. 进程检查
echo -e "\n3️⃣  进程检查"
echo "─────────────────────────────────────────────────────────"
echo "运行中的sensord进程:"
ps aux | grep sensord | grep -v grep || echo "❌ 没有运行中的sensord进程"

echo -e "\nOP系统进程:"
ps aux | grep "manager.py\|systemd" | grep -v grep | head -3 || echo "❌ OP系统未运行"

# 4. 文件检查
echo -e "\n4️⃣  文件检查"
echo "─────────────────────────────────────────────────────────"
if [ -f "$HOME/sunnypilot/system/sensord/ybimu_sensord.py" ]; then
    echo "✅ ybimu_sensord.py 存在"
else
    echo "❌ ybimu_sensord.py 不存在"
fi

if [ -f "$HOME/sunnypilot/system/sensord/sensord.py" ]; then
    echo "✅ sensord.py 存在"
else
    echo "❌ sensord.py 不存在"
fi

# 5. 串口权限检查
echo -e "\n5️⃣  串口权限检查"
echo "─────────────────────────────────────────────────────────"
if [ -c /dev/ttyUSB0 ]; then
    echo "串口权限:"
    ls -la /dev/ttyUSB0
else
    echo "❌ /dev/ttyUSB0 不存在"
fi

# 6. 尝试连接IMU
echo -e "\n6️⃣  尝试连接IMU"
echo "─────────────────────────────────────────────────────────"
python3 << 'PYEOF'
import os
import sys

sys.path.insert(0, os.path.expanduser('~/sunnypilot'))

try:
    from YbImuLib import YbImuSerial
    print("尝试连接 /dev/ttyUSB0...")

    imu = YbImuSerial('/dev/ttyUSB0', debug=False)
    imu.create_receive_threading()

    import time
    time.sleep(0.5)

    try:
        version = imu.get_version()
        print(f"✅ 连接成功! 固件版本: {version}")
    except:
        print("✅ 连接成功!")

        # 尝试读取数据
        ax, ay, az = imu.get_accelerometer_data()
        print(f"   加速度: [{ax:.2f}, {ay:.2f}, {az:.2f}]")

except Exception as e:
    print(f"❌ 连接失败: {e}")
    print("   原因可能:")
    print("   1. USB设备未连接")
    print("   2. YbImuLib库未安装")
    print("   3. 串口权限不足")

PYEOF

# 7. 消息系统检查
echo -e "\n7️⃣  消息系统检查"
echo "─────────────────────────────────────────────────────────"
timeout 3 python3 << 'PYEOF'
import cereal.messaging as messaging
import time

print("尝试连接到消息系统...")
try:
    sm = messaging.SubMaster(['accelerometer'])
    print("✅ 消息系统可用")

    # 等待数据
    for i in range(10):
        sm.update(0)
        if sm.updated['accelerometer']:
            accel = sm['accelerometer'].acceleration.v
            print(f"✅ 正在接收IMU数据: {accel}")
            break
        time.sleep(0.1)
    else:
        print("❌ 没有接收到数据 (驱动可能未启动)")

except Exception as e:
    print(f"❌ 错误: {e}")

PYEOF

echo -e "\n════════════════════════════════════════════════════════"
echo "✅ 诊断完成"
echo "════════════════════════════════════════════════════════"
