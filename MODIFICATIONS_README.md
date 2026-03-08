# SunnyPilot PC 模型修改说明

本文档详细描述了对 SunnyPilot PC 模型项目所做的主要修改及其原理。

## 修改概述

本次修改涉及多个核心模块，主要包括：
- 横向控制系统优化
- 神经网络横向控制增强
- UI界面改进
- 模型管理系统优化
- 新增摄像头模块
- 巡航控制扩展功能

## 详细修改内容

### 1. 横向控制系统优化

#### 1.1 智能饱和检测机制
**文件**: `selfdrive/controls/lib/latcontrol.py`

**修改内容**:
- 实现了基于速度的自适应饱和检测
- 高速场景(>15m/s): 降低饱和累积速率(0.3倍)，增加饱和容忍度(2.5倍)
- 城市场景: 保持标准敏感度

**原理**:
```python
# 原代码：简单的饱和检测逻辑
# 修改后：
def _check_saturation(self, saturated, CS, steer_limited_by_safety, curvature_limited):
  # Smart saturation detection: longer tolerance for sustained curves, stricter for city driving
  # Allow longer sustained control for highway curves and spiral ramps
  if CS.vEgo > 15:  # Highway/spiral ramp speeds
    # More permissive for sustained curves: slower accumulation, longer limit
    sat_rate = self.sat_count_rate * 0.3  # 3x slower accumulation
    sat_limit = self.sat_limit * 2.5    # 2.5x longer tolerance (20s at 100km/h)
  else:  # City driving
    # Normal sensitivity for city driving
    sat_rate = self.sat_count_rate
    sat_limit = self.sat_limit
```

**优势**:
- 在高速弯道和螺旋坡道上提供更平滑的控制体验
- 在城市驾驶中保持标准灵敏度，确保安全性
- 减少不必要的控制干预，提高驾驶舒适性

### 2. 神经网络横向控制增强

#### 2.1 神经网络横向控制扩展
**文件**: `sunnypilot/selfdrive/controls/lib/latcontrol_torque_ext.py`

**修改内容**:
- 添加了CI参数到构造函数，支持更灵活的配置
- 优化了神经网络前馈控制的集成

**原理**:
```python
def __init__(self, lac_torque, CP, CP_SP, CI):
  super().__init__(lac_torque, CP, CP_SP, CI)
```

#### 2.2 神经网络模型优化
**文件**: `sunnypilot/selfdrive/controls/lib/nnlc/nnlc.py`

**修改内容**:
- 添加了摩擦系数计算和基于扭矩空间的摩擦系数计算
- 优化了低速度处理逻辑
- 改进了未来时间偏移计算
- 添加了重力补偿和滚动/俯仰调整

**原理**:
```python
LOW_SPEED_X = [0, 10, 20, 30]
LOW_SPEED_Y = [12, 3, 1, 0]

# 改进的时间偏移计算
self.future_times = [0.3, 0.6, 1.0, 1.5] # seconds in the future
self.nn_future_times = [i + self.desired_lat_jerk_time for i in self.future_times]
```

**优势**:
- 提高了低速度下的控制精度
- 增强了模型对各种路况的适应性
- 改进了预测精度，特别是在复杂路况下

### 3. UI界面改进

#### 3.1 软件面板重构
**文件**: `selfdrive/ui/sunnypilot/qt/offroad/settings/software_panel.cc`

**修改内容**:
- 简化了软件面板界面，移除了复杂的模型选择功能
- 添加了分支搜索功能
- 添加了"禁用更新"切换选项
- 优化了用户交互体验

**原理**:
```cpp
// branch selector
QObject::disconnect(targetBranchBtn, nullptr, nullptr, nullptr);
connect(targetBranchBtn, &ButtonControlSP::clicked, [=]() {
  InputDialog d(tr("Search Branch"), this, tr("Enter search keywords, or leave blank to list all branches."), false);
    d.setMinLength(0);
    const int ret = d.exec();
    if (ret) {
      searchBranches(d.text());
    }
});
```

#### 3.2 新增模型管理面板
**文件**: `selfdrive/ui/sunnypilot/qt/offroad/settings/models_panel.cc` 和 `models_panel.h`

**新增功能**:
- 专门的模型管理界面
- 模型下载进度显示
- 模型缓存管理
- 模型类型分类显示

**原理**:
```cpp
// 创建进度条用于下载显示
supercomboProgressBar = createProgressBar(this);
QString supercomboType = tr("Driving Model");
supercomboFrame = createModelDetailFrame(this, supercomboType, supercomboProgressBar);
```

**优势**:
- 提供了更直观的模型管理界面
- 支持多种模型类型的独立管理
- 改进了用户体验和操作便利性

### 4. 模型管理系统优化

#### 4.1 下载超时处理
**文件**: `sunnypilot/models/manager.py`

**修改内容**:
- 添加了下载超时处理(10分钟)
- 改进了错误处理机制
- 优化了下载进度报告

**原理**:
```python
try:
  timeout = aiohttp.ClientTimeout(total=600)  # 10 minutes timeout
  async with aiohttp.ClientSession(timeout=timeout) as session:
    # 下载逻辑...
```

**优势**:
- 防止下载过程无限期挂起
- 提供更好的错误恢复机制
- 改进了用户反馈

### 5. 新增摄像头模块

#### 5.1 摄像头实现
**文件**: `tools/webcam/camera.cc`, `tools/webcam/camerad.cc` 等

**新增功能**:
- 完整的摄像头模块实现
- 支持MJPEG解码
- 支持YUV到NV12格式转换
- 支持多路视频流处理

**原理**:
```cpp
// decode_jpeg output NV12
bool decode_jpeg(const void* mjpeg_data, size_t mjpeg_size, std::vector<uint8_t>& yuv, std::vector<uint8_t>& nv12, int& width, int& height, tjhandle handle) {
  // 解码逻辑...
}
```

**优势**:
- 提供了完整的摄像头支持
- 支持多种视频格式转换
- 为模拟器和其他工具提供了视频输入能力

### 6. 巡航控制扩展功能

#### 6.1 自定义巡航增量
**文件**: `sunnypilot/selfdrive/car/cruise_ext.py`

**新增功能**:
- 支持自定义巡航速度增量
- 区分短按和长按增量设置
- 提供更灵活的巡航控制

**原理**:
```python
def update_v_cruise_delta(self, long_press: bool, v_cruise_delta: float) -> tuple[bool, float]:
  if not self.custom_acc_enabled:
    v_cruise_delta = v_cruise_delta * (5 if long_press else 1)
    return long_press, v_cruise_delta

  # Apply user-specified multipliers to the base increment
  short_increment = np.clip(self.short_increment, 1, 10)
  long_increment = np.clip(self.long_increment, 1, 10)

  actual_increment = long_increment if long_press else short_increment
  round_to_nearest = actual_increment in (5, 10)
  v_cruise_delta = v_cruise_delta * actual_increment

  return round_to_nearest, v_cruise_delta
```

**优势**:
- 提供了更个性化的巡航控制体验
- 允许用户根据自己的偏好调整速度增量
- 增强了系统的可定制性

## 系统架构变化

### 1. 模块化改进
- 将模型管理从软件面板中分离出来，形成独立的模块
- 摄像头功能模块化，便于复用和维护
- 巡航控制功能扩展，提高可配置性

### 2. 性能优化
- 横向控制系统优化，减少不必要的控制干预
- 神经网络模型优化，提高预测精度
- 下载超时处理，防止系统挂起

### 3. 用户体验改进
- UI界面简化，提高操作便利性
- 进度显示优化，提供更好的反馈
- 自定义功能增强，满足不同用户需求

## 兼容性说明

本次修改保持了与原有系统的兼容性，同时增加了以下新功能：
1. 支持更灵活的横向控制配置
2. 增强的神经网络横向控制
3. 改进的用户界面
4. 新增的摄像头支持
5. 扩展的巡航控制功能

## 测试建议

1. **横向控制测试**:
   - 在高速弯道测试控制平滑性
   - 在城市道路测试控制响应性

2. **神经网络控制测试**:
   - 测试各种路况下的控制精度
   - 验证低速度下的控制性能

3. **UI功能测试**:
   - 测试模型管理功能
   - 验证软件设置功能

4. **摄像头功能测试**:
   - 测试视频流捕获
   - 验证格式转换功能

5. **巡航控制测试**:
   - 测试自定义增量功能
   - 验证短按和长按响应

## 总结

本次修改主要围绕提高系统性能、增强用户体验和扩展功能三个方面展开。通过智能化的横向控制、优化的神经网络模型、改进的用户界面以及新增的功能模块，使SunnyPilot PC模型项目在保持原有稳定性的同时，提供了更强大的功能和更好的用户体验。