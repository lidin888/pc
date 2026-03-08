#!/usr/bin/env python3

import os
import shutil

def copy_file(src, dst):
    """复制文件，如果目标目录不存在则创建"""
    os.makedirs(os.path.dirname(dst), exist_ok=True)
    shutil.copy2(src, dst)
    print(f"Copied: {src} -> {dst}")

def main():
    """复制 TN 转向学习功能的核心文件"""

    # 定义源文件和目标文件的映射
    file_mappings = [
        # 转向学习核心文件
        ("/home/dengjian/openpilot/tn/sunnypilot/selfdrive/controls/lib/lane_turn_desire.py",
         "/home/dengjian/openpilot/selfdrive/controls/lib/lane_turn_desire.py"),

        # 横向控制扩展文件
        ("/home/dengjian/openpilot/tn/opendbc/sunnypilot/car/lateral_ext.py",
         "/home/dengjian/openpilot/opendbc/sunnypilot/car/lateral_ext.py"),

        # 参数键文件
        ("/home/dengjian/openpilot/tn/commom/params_keys.h",
         "/home/dengjian/openpilot/common/params_keys.h"),
    ]

    # 复制文件
    for src, dst in file_mappings:
        if os.path.exists(src):
            copy_file(src, dst)
        else:
            print(f"Warning: Source file not found: {src}")

    print("\nTN 转向学习功能文件复制完成!")
    print("\n已复制以下功能:")
    print("1. 车道转向意图学习 (Lane Turn Desire)")
    print("2. 实时转向延迟学习 (Live Learning Steer Delay)")
    print("3. 横向控制扩展 (Lateral Extension)")
    print("4. 相关参数定义")

if __name__ == "__main__":
    main()