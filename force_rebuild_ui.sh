#!/bin/bash

# 强制重新编译UI的脚本
echo "强制重新编译UI..."

# 清除编译缓存
echo "清除编译缓存..."
rm -rf selfdrive/ui/.sconsign.dblite
rm -rf selfdrive/ui/*.o
rm -rf selfdrive/ui/sunnypilot/*.o
rm -rf selfdrive/ui/sunnypilot/qt/*.o
rm -rf selfdrive/ui/sunnypilot/qt/offroad/*.o
rm -rf selfdrive/ui/sunnypilot/qt/offroad/settings/*.o
rm -rf selfdrive/ui/sunnypilot/qt/offroad/settings/vehicle/*.o
rm -rf selfdrive/ui/sunnypilot/qt/offroad/settings/lateral/*.o
rm -rf selfdrive/ui/sunnypilot/qt/widgets/*.o

# 清除主UI二进制文件
echo "清除主UI二进制文件..."
rm -f selfdrive/ui/ui

# 清除scons缓存
echo "清除scons缓存..."
scons --clean

echo "完成！现在可以运行 'tools/op.sh build' 重新编译项目"