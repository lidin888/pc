#!/usr/bin/env bash

# 专门解决模拟器车辆不动的问题

echo "=== 解决车辆不动问题 ==="

# 检查当前运行的模拟器进程
echo "检查当前模拟器状态..."

BRIDGE_PID=$(pgrep -f "run_bridge.py")
MANAGER_PID=$(pgrep -f "manager.py")

if [ -n "$BRIDGE_PID" ] && [ -n "$MANAGER_PID" ]; then
    echo "✓ 模拟器正在运行"
    echo "桥接PID: $BRIDGE_PID"
    echo "管理器PID: $MANAGER_PID"
else
    echo "✗ 模拟器未运行，请先启动模拟器"
    exit 1
fi

# 设置强制控制参数
echo "设置强制控制参数..."
python3 -c "
from openpilot.common.params import Params
params = Params()

# 强制启用所有控制
params.put_bool('IsOpenpilotEnabled', True)
params.put_bool('LongitudinalControl', True)
params.put_bool('ExperimentalMode', True)
params.put('CruiseSpeed1', '40')

# 禁用安全检查
params.put_bool('DisengageOnGas', False)
params.put_bool('DisengageOnBrake', False)

print('强制控制参数已设置')
"

echo ""
echo "=== 解决方案 ==="
echo "请按以下步骤操作："
echo ""
echo "1. 切换到模拟器窗口（确保窗口可见）"
echo "2. 按以下键盘命令："
echo "   i → 启动点火"
echo "   1 → 启用巡航控制"
echo "   w → 手动加速（如果巡航不工作）"
echo "   r → 重置模拟器（如果车辆卡住）"
echo ""
echo "3. 观察车辆是否开始移动"
echo ""

# 监控日志
echo "开始监控车辆状态..."
for i in {1..10}; do
    if tail -n 5 manager.log 2>/dev/null | grep -q "vEgo.*[1-9]"; then
        echo "✓ 车辆开始移动！"
        exit 0
    fi
    echo "等待车辆移动... ($i/10)"
    sleep 2
done

echo ""
echo "⚠ 车辆仍未移动，请手动操作键盘控制"
echo "确保模拟器窗口处于活动状态，然后按："
echo "i → 1 → w"
echo ""
echo "如果仍然不行，可能需要重启模拟器："
echo "1. 按 'q' 键退出当前模拟器"
echo "2. 重新运行: tools/sim/auto_start.sh"