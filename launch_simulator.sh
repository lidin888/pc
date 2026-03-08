#!/usr/bin/env bash

# 模拟器独立模式启动脚本
# 确保使用当前目录的项目而不是其他目录

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null && pwd )"

echo "启动模拟器独立模式，使用项目目录: $DIR"

# 检查虚拟环境是否存在
if [ ! -f "$DIR/.venv/bin/activate" ]; then
    echo "错误: 在当前目录没有找到虚拟环境"
    echo "请先运行: tools/op.sh --standalone venv 或 tools/op.sh --standalone setup"
    exit 1
fi

# 激活当前目录的虚拟环境
source "$DIR/.venv/bin/activate"

# 强制设置Python路径和库路径，确保使用当前目录
unset PYTHONPATH
export PYTHONPATH="$DIR"
export LD_LIBRARY_PATH="$DIR/third_party/acados/x86_64/lib:$DIR/selfdrive/controls/lib/longitudinal_mpc_lib/c_generated_code:$DIR/selfdrive/controls/lib/lateral_mpc_lib/c_generated_code:$LD_LIBRARY_PATH"

# 设置模拟器环境变量
export PASSIVE="0"
export NOBOARD="1"
export SIMULATION="1"
export SKIP_FW_QUERY="1"
export FINGERPRINT="HONDA_CIVIC_2022"

# 禁用不需要的进程
export BLOCK="camerad,loggerd,encoderd,micd,logmessaged"
if [[ "$CI" ]]; then
  export BLOCK="${BLOCK},ui"
fi

echo "模拟器环境变量设置:"
echo "  虚拟环境: $DIR/.venv"
echo "  PYTHONPATH=$PYTHONPATH"
echo "  SIMULATION=$SIMULATION"
echo "  FINGERPRINT=$FINGERPRINT"

# 清理任何现有的进程
echo "清理现有进程..."
pkill -f "run_bridge.py" 2>/dev/null || true
pkill -f "manager.py" 2>/dev/null || true
sleep 2

# 启用参数
python3 -c "from openpilot.selfdrive.test.helpers import set_params_enabled; set_params_enabled()"

# 设置自动控制参数
echo "设置自动控制参数..."
cd "$DIR"
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

# 启动模拟器桥接进程（在后台）- 使用自动模式
echo "启动模拟器桥接进程（自动模式）..."
python3 tools/sim/run_bridge.py --high_quality > "$DIR/sim_bridge.log" 2>&1 &
BRIDGE_PID=$!
echo "桥接进程PID: $BRIDGE_PID"

# 等待桥接进程初始化
sleep 5

# 检查桥接进程是否正常运行
if ! kill -0 $BRIDGE_PID 2>/dev/null; then
    echo "错误: 桥接进程启动失败"
    echo "查看日志文件: $DIR/sim_bridge.log"
    cat "$DIR/sim_bridge.log"
    exit 1
fi

# 启动openpilot manager进程
echo "启动openpilot manager进程..."
cd "$DIR/system/manager"
python3 manager.py > "$DIR/manager.log" 2>&1 &
MANAGER_PID=$!
echo "Manager进程PID: $MANAGER_PID"

# 等待进程初始化
sleep 3

# 检查进程状态
echo "检查进程状态..."
if kill -0 $BRIDGE_PID 2>/dev/null && kill -0 $MANAGER_PID 2>/dev/null; then
    echo "✓ 模拟器启动成功!"
    echo "桥接进程PID: $BRIDGE_PID"
    echo "Manager进程PID: $MANAGER_PID"
    echo ""
    echo "要停止模拟器，运行:"
    echo "  kill $BRIDGE_PID $MANAGER_PID"
    echo "或运行:"
    echo "  $DIR/tools/sim/cleanup.sh"
    echo ""
    echo "查看桥接日志: tail -f $DIR/sim_bridge.log"
    echo "查看管理器日志: tail -f $DIR/manager.log"
    echo ""
    echo "模拟器正在运行..."

    # 等待进程结束
    wait $BRIDGE_PID $MANAGER_PID
else
    echo "错误: 进程启动失败"
    echo "桥接进程状态: $(kill -0 $BRIDGE_PID 2>/dev/null && echo "运行中" || echo "已停止")"
    echo "Manager进程状态: $(kill -0 $MANAGER_PID 2>/dev/null && echo "运行中" || echo "已停止")"
    echo ""
    echo "查看桥接日志:"
    cat "$DIR/sim_bridge.log"
    echo ""
    echo "查看管理器日志:"
    cat "$DIR/manager.log"
    exit 1
fi

echo "模拟器已停止"