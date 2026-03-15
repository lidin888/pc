#!/bin/bash
# 一键安装YbImuLib库

echo "🔧 YbImuLib 安装工具"
echo "════════════════════════════════════════════════════════"

# 检查Python
echo "1️⃣  检查Python环境..."
if ! command -v python3 &> /dev/null; then
    echo "❌ Python3未安装"
    exit 1
fi
echo "✅ Python3: $(python3 --version)"

# 检查pip
echo -e "\n2️⃣  检查pip..."
if ! command -v pip3 &> /dev/null; then
    echo "❌ pip3未安装，尝试安装..."
    python3 -m pip install --upgrade pip
fi
echo "✅ pip3: $(pip3 --version)"

# 检查是否已安装
echo -e "\n3️⃣  检查YbImuLib..."
if python3 -c "from YbImuLib import YbImuSerial" 2>/dev/null; then
    echo "✅ YbImuLib已安装"
    python3 -c "from YbImuLib import YbImuSerial; print('   版本和位置:', YbImuSerial.__file__ if hasattr(YbImuSerial, '__file__') else 'Unknown')"
else
    echo "❌ YbImuLib未安装，正在安装..."

    # 尝试多个源
    echo -e "\n   尝试官方源..."
    if pip3 install YbImuLib 2>/dev/null; then
        echo "✅ 安装成功 (官方源)"
    else
        echo "   官方源失败，尝试阿里云镜像..."
        if pip3 install YbImuLib -i https://mirrors.aliyun.com/pypi/simple 2>/dev/null; then
            echo "✅ 安装成功 (阿里云镜像)"
        else
            echo "   阿里云镜像失败，尝试清华镜像..."
            if pip3 install YbImuLib -i https://pypi.tsinghua.edu.cn/simple 2>/dev/null; then
                echo "✅ 安装成功 (清华镜像)"
            else
                echo "❌ 所有源都失败"
                exit 1
            fi
        fi
    fi
fi

# 安装依赖
echo -e "\n4️⃣  检查依赖..."
for pkg in pyserial; do
    if python3 -c "import $pkg" 2>/dev/null; then
        echo "   ✅ $pkg"
    else
        echo "   📥 安装 $pkg..."
        pip3 install $pkg
    fi
done

# 最终验证
echo -e "\n5️⃣  最终验证..."
python3 << 'PYEOF'
try:
    from YbImuLib import YbImuSerial
    print("   ✅ YbImuLib可以导入")
    print(f"   ✅ 类: YbImuSerial")
except ImportError as e:
    print(f"   ❌ 导入失败: {e}")
    exit(1)
PYEOF

echo -e "\n════════════════════════════════════════════════════════"
echo "✅ 安装完成!"
echo "════════════════════════════════════════════════════════"
echo -e "\n现在可以运行:"
echo "  python3 ~/sunnypilot/system/sensord/ybimu_sensord.py"
