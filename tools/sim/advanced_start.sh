#!/usr/bin/env bash

# 高级模拟器启动脚本 - 解决车辆不动的问题

echo "=== 高级模拟器启动 ==="

# 设置环境变量
export PASSIVE="0"
export NOBOARD="1"
export SIMULATION="1"
export SKIP_FW_QUERY="1"
export FINGERPRINT="HONDA_CIVIC_2022"
export BLOCK="camerad,loggerd,encoderd,micd,logmessaged"

# 启用openpilot参数
python3 -c "from openpilot.selfdrive.test.helpers import set_params_enabled; set_params_enabled()"

# 设置高级控制参数
echo "设置高级控制参数..."
python3 -c "
from openpilot.common.params import Params
params = Params()

# 启用所有必要的控制功能
params.put_bool('AlphaLongitudinalEnabled', True)
params.put_bool('ExperimentalMode', True)
params.put_bool('IsOpenpilotEnabled', True)
params.put_bool('LongitudinalControl', True)

# 设置巡航速度
params.put('CruiseSpeed1', '40')  # 40 km/h

# 禁用可能阻止移动的安全检查
params.put_bool('DisengageOnGas', False)
params.put_bool('DisengageOnBrake', False)

print('高级控制参数设置完成')
"

# 启动模拟器桥接
echo "启动模拟器桥接..."
python3 tools/sim/run_bridge.py --high_quality > sim_bridge.log 2>&1 &
BRIDGE_PID=$!

# 等待桥接启动
sleep 5

# 启动openpilot manager
echo "启动openpilot manager..."
cd system/manager
python3 manager.py > manager.log 2>&1 &
MANAGER_PID=$!

# 等待系统启动
sleep 10

echo "模拟器启动完成!"
echo "桥接PID: $BRIDGE_PID"
echo "管理器PID: $MANAGER_PID"
echo ""

# 监控车辆状态并自动解决问题
monitor_vehicle() {
    echo "开始监控车辆状态..."

    for i in {1..30}; do  # 监控30秒
        # 检查速度数据
        if tail -n 10 manager.log | grep -q "vEgo.*[1-9]"; then
            echo "✓ 车辆开始移动!"
            return 0
        fi

        echo "等待车辆移动... ($i/30)"
        sleep 1
    done

    echo "⚠ 车辆未移动，尝试自动解决方案..."

    # 尝试重置模拟器
    echo "尝试重置模拟器..."
    python3 -c "
from openpilot.common.params import Params
params = Params()
# 发送重置命令
params.put('SimulatorReset', '1')
print('重置命令已发送')
"

    sleep 2

    # 再次检查
    if tail -n 10 manager.log | grep -q "vEgo.*[1-9]"; then
        echo "✓ 重置后车辆开始移动!"
        return 0
    else
        echo "✗ 自动解决方案失败，需要手动干预"
        echo ""
        echo "手动解决方案:"
        echo "1. 确保模拟器窗口可见"
        echo "2. 按 'i' 键启动点火"
        echo "3. 按 '1' 键启用巡航"
        echo "4. 按 'w' 键手动加速"
        echo "5. 按 'r' 键重置模拟器"
        return 1
    fi
}

# 开始监控
monitor_vehicle

echo ""
echo "模拟器正在运行中..."
echo "查看日志: tail -f manager.log"
echo "停止模拟器: kill $BRIDGE_PID $MANAGER_PID"

# 等待进程结束
wait $BRIDGE_PID $MANAGER_PID