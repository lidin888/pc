#!/usr/bin/env bash

# 自动启动模拟器并设置初始速度的脚本

echo "启动自动模拟器模式..."

# 设置环境变量
export PASSIVE="0"
export NOBOARD="1"
export SIMULATION="1"
export SKIP_FW_QUERY="1"
export FINGERPRINT="HONDA_CIVIC_2022"
export BLOCK="camerad,loggerd,encoderd,micd,logmessaged"

# 启用openpilot参数
python3 -c "from openpilot.selfdrive.test.helpers import set_params_enabled; set_params_enabled()"

# 设置自动控制参数
python3 -c "
from openpilot.common.params import Params
params = Params()
# 启用自动控制
params.put_bool('AlphaLongitudinalEnabled', True)
params.put_bool('ExperimentalMode', True)
# 设置初始巡航速度 (40 km/h)
params.put('CruiseSpeed1', '40')
print('自动控制参数设置完成')
"

# 启动模拟器桥接（无键盘控制）
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
echo "系统将在10秒后自动启用控制..."

# 等待系统稳定
sleep 10

# 尝试自动启用控制
echo "尝试自动启用openpilot控制..."
python3 -c "
import time
from openpilot.common.params import Params

# 等待系统准备就绪
time.sleep(5)

# 设置巡航速度
params = Params()
params.put('CruiseSpeed1', '40')  # 40 km/h

print('自动控制已设置，车辆应该开始移动...')
"

echo "模拟器正在运行中..."
echo "查看日志: tail -f manager.log"
echo "停止模拟器: kill $BRIDGE_PID $MANAGER_PID"

# 等待进程结束
wait $BRIDGE_PID $MANAGER_PID