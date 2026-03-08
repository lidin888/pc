#!/usr/bin/env python3
"""
测试参数保存功能
"""

import os
import sys

try:
    from openpilot.common.params import Params

    # 创建Params实例
    params = Params()

    # 测试端到端控制参数
    print("测试端到端控制参数保存...")
    params.put("EndToEndToggle", "1")
    value = params.get("EndToEndToggle")
    print(f"EndToEndToggle 设置为: {value}")

    # 测试实时转向自主学习参数
    print("测试实时转向自主学习参数保存...")
    params.put("LagdToggle", "1")
    value = params.get("LagdToggle")
    print(f"LagdToggle 设置为: {value}")

    print("\n参数保存测试完成！")
    print("如果值正确显示，说明参数可以正确保存。")
    print("请重启openpilot并检查这些设置是否保持。")

except Exception as e:
    print(f"测试失败: {e}")
    import traceback
    traceback.print_exc()