#!/bin/bash
# 快速检查 USB IMU 状态

echo "════════════════════════════════════════════════════════"
echo "🔍 Yahboom USB IMU 状态检查"
echo "════════════════════════════════════════════════════════"

# 1. 检查USB设备
echo -e "\n1️⃣  USB 设备："
if [ -e /dev/ttyUSB0 ]; then
    echo "   ✅ 检测到: /dev/ttyUSB0"
    ls -la /dev/ttyUSB0
else
    echo "   ❌ 未检测到USB设备"
fi

# 2. 检查驱动进程
echo -e "\n2️⃣  IMU 驱动进程："
if pgrep -f "ybimu_sensord" > /dev/null; then
    echo "   ✅ ybimu_sensord 正在运行"
    ps aux | grep ybimu_sensord | grep -v grep
elif pgrep -f "sensord.py" > /dev/null; then
    echo "   📍 标准 sensord 驱动正在运行"
    ps aux | grep sensord.py | grep -v grep
else
    echo "   ❌ 未检测到任何IMU驱动"
fi

# 3. 检查是否有数据流
echo -e "\n3️⃣  数据流检查："
timeout 2 python3 << 'EOF' 2>/dev/null
import cereal.messaging as messaging
sm = messaging.SubMaster(['accelerometer'], timeout=1000)
for _ in range(5):
    sm.update(0)
    if sm.updated['accelerometer']:
        accel = sm['accelerometer'].acceleration.v
        print(f"   ✅ 正在接收数据: {accel}")
        break
else:
    print("   ⏳ 等待数据中...")
EOF

# 4. 检查日志
echo -e "\n4️⃣  最近的日志："
if [ -f /data/openpilot/tmp/cloudlog.txt ]; then
    grep -i "yahboom\|imu" /data/openpilot/tmp/cloudlog.txt 2>/dev/null | tail -3 || echo "   (无IMU相关日志)"
else
    echo "   📂 日志文件未找到: /data/openpilot/tmp/cloudlog.txt"
fi

echo -e "\n════════════════════════════════════════════════════════"
echo "✅ 检查完成"
echo "════════════════════════════════════════════════════════"
