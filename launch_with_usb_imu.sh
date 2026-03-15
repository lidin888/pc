#!/bin/bash
# 启动 openpilot 并使用 USB IMU

echo "🚀 启动 openpilot with USB IMU"
echo "================================"

# 自动检测 USB 设备
found_port=""
for port in /dev/ttyUSB* /dev/ttyACM*; do
    if [ -e "$port" ]; then
        found_port="$port"
        break
    fi
done

if [ -n "$found_port" ]; then
    echo "✅ 检测到 USB IMU: $found_port"
    export YBIMU_PORT="$found_port"
else
    echo "❌ 未检测到 USB IMU 设备 (/dev/ttyUSB* 或 /dev/ttyACM*)"
    echo "   请检查 USB 连接"
    exit 1
fi

# 强制使用 USB IMU
export USE_USB_IMU=1

echo "   YBIMU_PORT=$YBIMU_PORT"
echo "   USE_USB_IMU=$USE_USB_IMU"
echo ""
echo "启动 openpilot..."
echo "================================"

cd /home/lanyi/sunnypilot
exec ./launch_openpilot.sh
