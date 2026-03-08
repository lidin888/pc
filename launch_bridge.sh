#!/usr/bin/env bash

# 修改后的启动脚本
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null && pwd )"

echo "启动模拟器（修复键盘控制问题）..."

# 清理现有进程
pkill -f "run_bridge.py" 2>/dev/null || true
pkill -f "manager.py" 2>/dev/null || true
sleep 2

# 设置环境
source "$DIR/.venv/bin/activate"
unset PYTHONPATH
export PYTHONPATH="$DIR"
export LD_LIBRARY_PATH="$DIR/third_party/acados/x86_64/lib:$DIR/selfdrive/controls/lib/longitudinal_mpc_lib/c_generated_code:$DIR/selfdrive/controls/lib/lateral_mpc_lib/c_generated_code:$LD_LIBRARY_PATH"
export PASSIVE="0"
export NOBOARD="1"
export SIMULATION="1"
export SKIP_FW_QUERY="1"
export FINGERPRINT="HONDA_CIVIC_2022"

# 启用参数
python3 -c "from openpilot.selfdrive.test.helpers import set_params_enabled; set_params_enabled()"

# 启动桥接进程（不使用 --joystick 参数）
echo "启动桥接进程（启用键盘控制）..."
cd "$DIR"
python3 tools/sim/run_bridge.py > "$DIR/sim_bridge.log" 2>&1 &
BRIDGE_PID=$!
echo "桥接进程PID: $BRIDGE_PID"

# 等待桥接进程初始化
sleep 5

# 启动管理器进程
echo "启动管理器进程..."
cd "$DIR/system/manager"
python3 manager.py > "$DIR/manager.log" 2>&1 &
MANAGER_PID=$!
echo "管理器进程PID: $MANAGER_PID"

sleep 3

# 检查进程状态
if kill -0 $BRIDGE_PID 2>/dev/null && kill -0 $MANAGER_PID 2>/dev/null; then
    echo "✓ 模拟器启动成功!"
    echo "现在可以在终端中使用键盘控制车辆!"
    echo "按 i 启动车辆，按 w 加速前进"
    wait $BRIDGE_PID $MANAGER_PID
else
    echo "错误: 进程启动失败"
    exit 1
fi
