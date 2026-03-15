#!/bin/bash
# 手动启动Yahboom USB IMU驱动
# Manual start of Yahboom USB IMU sensor daemon
#
# 说明: 通常无需运行此脚本，OP系统会自动检测USB IMU并启动驱动
# Notes: Usually no need to run this, OP system will auto-detect and start
#
# 使用场景:
# 1. 单独测试USB IMU驱动
# 2. 快速重启IMU驱动（不重启整个OP系统）
# 3. 使用自定义串口: export YBIMU_PORT=/dev/ttyUSB1 && bash start_ybimu.sh

cd /home/lanyi/sunnypilot

# 自动查找 USB IMU 设备
if [ -z "$YBIMU_PORT" ]; then
    for port in /dev/ttyUSB* /dev/ttyACM*; do
        if [ -e "$port" ]; then
            export YBIMU_PORT="$port"
            break
        fi
    done
fi

# 确保串口权限
if [ -n "$YBIMU_PORT" ] && [ -e "$YBIMU_PORT" ]; then
    sudo chmod 666 "$YBIMU_PORT" 2>/dev/null || true
else
    echo "错误: 未找到 USB IMU 设备"
    exit 1
fi

echo "Starting Yahboom IMU sensor daemon on $YBIMU_PORT..."
echo "Note: 通常情况下 OP 系统会自动启动此驱动，无需手动运行"

# 启动传感器守护进程
python3 system/sensord/ybimu_sensord.py
