# TN 转向学习功能移植总结

## 概述
本文档总结了将 TN 分支中的转向学习功能移植到主分支的工作。移植的功能包括：
1. 车道转向意图学习 (Lane Turn Desire)
2. 实时转向延迟学习 (Live Learning Steer Delay)
3. 丰田特调选项

## 完成的工作

### 1. UI 界面创建
- 创建了 `SteeringSettings` 类，包含转向学习相关的 UI 控件
- 创建了 `ToyotaSettings` 类，包含丰田特调相关的 UI 控件
- 创建了 `RadishSteeringPanel` 类，将转向学习和丰田特调整合到"萝卜"标签下
- 修改了 `CarrotPanel` 类，添加了"转向学习"按钮

### 2. 控件实现
- 修复了 `controls.h` 文件，移除了不必要的头文件包含
- 实现了 `controls.cc` 文件，添加了所有 UI 控件的实现
- 实现了 `toggle.h` 和 `toggle.cc` 文件，添加了自定义开关控件

### 3. 核心功能移植
- 复制了 `lane_turn_desire.py` 文件，实现车道转向意图学习
- 复制了 `lateral_ext.py` 文件，实现横向控制扩展
- 更新了 `params_keys.h` 文件，添加了新的参数定义

### 4. 构建系统更新
- 更新了 `SConscript` 文件，确保新文件被包含在构建中
- 更新了 `params.h` 文件，添加了新参数的声明

## 文件结构

### 新创建的文件
```
selfdrive/ui/sunnypilot/qt/offroad/settings/
├── steering_settings.h
├── steering_settings.cc
├── radish_steering_panel.h
├── radish_steering_panel.cc
└── vehicle/
    ├── toyota_settings.h
    └── toyota_settings.cc

selfdrive/ui/sunnypilot/qt/widgets/
├── controls.h
└── controls.cc

selfdrive/ui/sunnypilot/qt/widgets/
├── toggle.h
└── toggle.cc

selfdrive/controls/lib/
├── lane_turn_desire.py
└── lane_turn_desire.h

opendbc/sunnypilot/car/
└── lateral_ext.py

common/
└── params_keys.h
```

### 修改的文件
```
selfdrive/ui/qt/offroad/
├── settings.h
└── settings.cc

selfdrive/ui/
└── SConscript

common/
└── params.h
```

## 功能说明

### 转向学习选项卡
包含以下设置：
- **Use Lane Turn Desires**: 启用车道转向意图学习
- **Adjust Lane Turn Speed**: 设置车道转向的最大速度
- **Live Learning Steer Delay**: 启用实时转向延迟学习
- **Adjust Software Delay**: 调整软件延迟值

### 丰田特调选项卡
包含以下设置：
- **Toyota: Drive Mode Button Link**: 链接驾驶模式按钮与加速个性
- **Toyota: Auto Brake Hold (TSS2 Hybrid)**: 自动刹车保持
- **Toyota: Enhanced BSM Support**: 增强盲点监测支持
- **Toyota: TSS2 Custom Tune**: TSS2 自定义调校
- **Toyota: Stock Toyota Longitudinal**: 使用丰田原厂纵向控制

## 使用方法

1. 在设置界面中点击"萝卜"标签
2. 点击"转向学习"按钮
3. 在转向学习选项卡中配置相关参数
4. 在丰田特调选项卡中配置丰田特定参数

## 注意事项

1. 所有参数都需要在离线状态下修改
2. 修改参数后需要重启 openpilot 才能生效
3. 转向学习功能需要一定的学习时间才能达到最佳效果

## 后续工作

1. 测试所有功能是否正常工作
2. 根据测试结果调整参数默认值
3. 添加更多车型的特调选项
4. 优化 UI 界面，提升用户体验