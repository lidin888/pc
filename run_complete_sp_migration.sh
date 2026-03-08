#!/bin/bash

# SunnyPilot特性完整一键移植执行脚本
# 使用方法: ./run_complete_sp_migration.sh

echo "========================================="
echo "    SunnyPilot特性完整一键移植脚本"
echo "========================================="
echo ""

# 检查Python是否安装
if ! command -v python3 &> /dev/null; then
    echo "错误: 未找到Python3，请先安装Python3"
    exit 1
fi

# 检查脚本文件是否存在
if [ ! -f "complete_sp_migration.py" ]; then
    echo "错误: 未找到complete_sp_migration.py脚本文件"
    exit 1
fi

# 设置执行权限
chmod +x complete_sp_migration.py

# 执行一键移植脚本
echo "开始执行SunnyPilot特性完整一键移植..."
python3 complete_sp_migration.py

# 检查执行结果
if [ $? -eq 0 ]; then
    echo ""
    echo "========================================="
    echo "           移植执行完成！"
    echo "========================================="
    echo ""
    echo "后续步骤:"
    echo "1. 编译项目:"
    echo "   scons -j\$(nproc)"
    echo ""
    echo "2. 在安全环境下进行测试"
    echo ""
    echo "3. 根据实际效果进行微调"
    echo ""
    echo "请查看移植总结文档：COMPLETE_SP_MIGRATION_SUMMARY.md"
    echo "请查看移植日志：sp_migration_log.txt"
else
    echo ""
    echo "========================================="
    echo "           移植执行失败！"
    echo "========================================="
    echo ""
    echo "请检查错误信息并修复后重新运行"
    echo "请查看移植日志：sp_migration_log.txt"
fi