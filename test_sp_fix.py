#!/usr/bin/env python3

import os

def test_fix():
    """测试修复是否有效"""
    print("测试SunnyPilot特性修复...")

    # 运行修复脚本
    print("\n1. 运行修复脚本...")
    os.system("python3 fix_sp_migration.py")

    # 验证关键文件是否存在
    print("\n2. 验证关键文件是否存在...")
    key_files = [
        "sunnypilot/selfdrive/controls/lib/nnlc/nnlc.py",
        "sunnypilot/selfdrive/controls/lib/latcontrol_torque_ext_base.py",
        "sunnypilot/selfdrive/car/car_specific.py",
        "opendbc_repo/opendbc/sunnypilot/car/toyota/secoc_long.py",
        "opendbc_repo/opendbc/sunnypilot/car/toyota/values.py",
    ]

    all_exist = True
    for file_path in key_files:
        if os.path.exists(file_path):
            print(f"✓ {file_path}")
        else:
            print(f"✗ {file_path}")
            all_exist = False

    if all_exist:
        print("\n✓ 所有关键文件已创建！")
        print("\n现在可以运行完整的移植脚本:")
        print("python3 migrate_sp_features.py")
        return True
    else:
        print("\n✗ 部分文件创建失败")
        return False

if __name__ == "__main__":
    test_fix()