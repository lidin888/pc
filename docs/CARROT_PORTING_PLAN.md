# CarrotPilot → Sunnypilot 功能移植计划

## 一、功能概述

从 CarrotPilot（`/home/lanyi/视频/openpilot`）向 Sunnypilot 移植两大核心功能：

| 功能 | 描述 |
|------|------|
| **语音播报系统** | 基于预录 WAV 的事件驱动音频播放，覆盖交通灯、变道、测速、倒计时等 40+ 种场景 |
| **手机App局域网投射** | 通过 UDP/TCP 协议将手机端导航信息（地图、限速、测速、转弯引导）实时投射到设备 |

---

## 二、架构分析

### 2.1 语音播报系统架构

```
事件源                    消息总线              音频引擎
┌──────────────┐         ┌─────────────┐      ┌──────────────┐
│ selfdrived   │──event──►│ cereal      │──────►│ soundd.py    │
│ desire_helper│         │ (PubSub)    │      │  sounddevice │
│ carrot_serv  │──alert──►│             │──────►│  48kHz mono  │
└──────────────┘         └─────────────┘      └──────────────┘
```

- **无 TTS 引擎**，全部使用预录 `.wav` 文件（双语：韩语 + 英语）
- **事件驱动**：驾驶事件 → EventName → AudibleAlert → WAV 播放
- **自适应音量**：环境噪声 + 用户参数联合调节
- **倒计时播报**：`audio_1.wav` ~ `audio_10.wav`，用于测速/转弯提前提示

### 2.2 手机投射系统架构

```
┌──────────────────┐     UDP 7705/7706      ┌──────────────────────────┐
│  CarrotMan APK   │ ◄═══════════════════► │  设备端                    │
│  (手机端)         │     TCP 7709          │  ├─ carrot_man.py (主控)   │
│  ├─ 导航SDK      │ ════════════════════► │  ├─ carrot_serv.py (导航)  │
│  ├─ GPS          │                       │  ├─ amap_navi.py (外挂)   │
│  └─ 路况/限速    │     外挂设备           │  ├─ web_interface.py (Web) │
│                  │     UDP 4210/4211     │  └─ config.py (配置)      │
└──────────────────┘ ◄═══════════════════► └──────────────────────────┘
```

**通信协议矩阵**：

| 端口 | 协议 | 方向 | 用途 |
|------|------|------|------|
| 7705 | UDP 广播 | 设备→手机 | 设备状态广播（车速/巡航/导航状态）|
| 7706 | UDP 单播 | 手机→设备 | 导航数据（SDI测速/TBT转弯/GPS/道路信息）|
| 7709 | TCP | 手机→设备 | 导航路线坐标（二进制 float 数组）|
| 7710 | ZMQ REP | 手机→设备 | 远程命令执行（调试用）|
| 4210/4211 | UDP | 双向 | 外挂设备（激光雷达/摄像头盲区检测）|
| 8088 | HTTP | 浏览器→设备 | Web 配置界面 |
| 12345 | UDP 广播 | Kisa→设备 | 韩国交通信息源 |

---

## 三、关键冲突与解决方案

### 3.1 ⚠️ Cereal Custom.capnp Struct ID 冲突（最高优先级）

两个代码库在 `custom.capnp` 中使用**相同的 Struct ID** 定义了**完全不同的结构体**：

| Struct ID | Sunnypilot 占用 | CarrotPilot 占用 |
|-----------|-----------------|-------------------|
| `@0x81c2f05a394cf4af` | `SelfdriveStateSP` | `CarrotMan` |
| `@0xaedffd8f31e7b55d` | `ModelManagerSP` | `AmapNavi` |

**解决方案**：将 CarrotPilot 的结构体迁移到空闲的 Reserved 槽位：

| 新 Struct ID | 用途 |
|-------------|------|
| `@0xcb9fd56c7057593a` (CustomReserved10) | → **CarrotMan** (39 字段) |
| `@0xc2243c65e0340384` (CustomReserved11) | → **AmapNavi** (2 字段) |

> ✅ Sunnypilot 的 CustomReserved10–19 全部空闲，可安全使用。

### 3.2 AudibleAlert 枚举冲突

| 代码库 | 策略 | 枚举范围 |
|--------|------|---------|
| CarrotPilot | 直接修改 `car.capnp` AudibleAlert | @9–@43 (35个新值) |
| Sunnypilot | 在 `custom.capnp` 内独立定义 `SelfdriveStateSP.AudibleAlert` | @9–@30 reserved, @31–@32 自用 |

**解决方案**：遵循 Sunnypilot 的惯例，在 `SelfdriveStateSP.AudibleAlert` 中利用 reserved9–reserved30 对应 CarrotPilot 的音效（恰好可覆盖），并从 @33 起追加剩余枚举值。不修改 `car.capnp`。

### 3.3 UI 渲染架构冲突

| 代码库 | UI 扩展方式 |
|--------|-----------|
| CarrotPilot | `carrot.cc` + `carrot_gpu.cc` (~6700 行 NanoVG C++) |
| Sunnypilot | `selfdrive/ui/sunnypilot/` 目录体系 (Qt C++) |

**解决方案**：分阶段处理，第一阶段跳过 UI 渲染移植，先实现数据层；第二阶段将 CarrotPilot 的 HUD 信息按 Sunnypilot UI 架构重新实现。

---

## 四、移植计划（分阶段）

### 阶段一：基础设施层（预计 2-3 天）

> 目标：建立消息定义和服务注册，打通数据通路

#### 步骤 1.1 — 扩展 cereal 消息定义

**文件**: `cereal/custom.capnp`

- 将 `CustomReserved10` 替换为 `CarrotMan` 结构体定义（39 个字段）
- 将 `CustomReserved11` 替换为 `AmapNavi` 结构体定义（2 个字段）
- 在 `SelfdriveStateSP.AudibleAlert` 枚举中补充 CarrotPilot 的音效枚举：
  - reserved9 → `audioTurn @9`
  - reserved10 → `longEngaged @10`
  - reserved11 → `longDisengaged @11`
  - reserved12 → `trafficSignGreen @12`
  - reserved13 → `trafficSignChanged @13`
  - reserved14 → `laneChange @14`
  - reserved15 → `stopping @15`
  - reserved16 → `autoHold @16`
  - reserved17 → `engage2 @17`
  - reserved18 → `disengage2 @18`
  - reserved19 → `trafficError @19`
  - reserved20 → `bsdWarning @20`
  - reserved21 → `speedDown @21`
  - reserved22 → `stopStop @22`
  - reserved23 → `reverseGear2 @23`
  - reserved24–reserved30 → `audio1 @24` ~ `audio7 @30`
  - 追加 `audio8 @33` ~ `audio10 @35`
  - 追加 `nnff @36`, `preLaneChange @37` ... `laneChangeEnd @43`

**验证**: `scons cereal/` 编译通过

#### 步骤 1.2 — 注册新服务

**文件**: `cereal/services.py`

```python
# 新增 CarrotPilot 服务
"carrotMan":            (True, 0.),
"amapNavi":             (True, 0.),
"navInstructionCarrot": (True, 1., 10),
```

#### 步骤 1.3 — 重编译 cereal 生成代码

```bash
cd /home/lanyi/sunnypilot && scons cereal/
```

---

### 阶段二：核心逻辑移植（预计 3-5 天）

> 目标：移植 carrot 模块全部 Python 代码

#### 步骤 2.1 — 复制 carrot 模块

```bash
# 复制 carrot 目录
cp -r /home/lanyi/视频/openpilot/selfdrive/carrot/ /home/lanyi/sunnypilot/selfdrive/carrot/
```

**需要移植的文件（~8400 行 Python）**：

| 文件 | 行数 | 功能 |
|------|------|------|
| `carrot_man.py` | ~1090 | 主控管理器：UDP广播、路线接收、弯道限速、ZMQ命令 |
| `carrot_serv.py` | ~1388 | 导航服务：手机数据解析、SDI/TBT处理、速度控制、自动转弯 |
| `carrot_functions.py` | ~525 | 辅助函数库 |
| `amap_navi.py` | ~1917 | 外挂通信：激光雷达/摄像头盲区检测 |
| `config.py` | ~200 | 统一参数管理 |
| `web_interface.py` | ~300 | HTTP 配置界面 |
| `nav_params.json` | — | 导航参数默认值 |
| `nav_params.html` | — | Web 参数页面 |
| `radar.html` | — | Web 雷达页面 |

#### 步骤 2.2 — 修改模块引用路径

CarrotPilot 使用 `openpilot.selfdrive.carrot.*` 导入路径，需全部改为 sunnypilot 的正确路径（通常相同或需检查）。

关键修改：
- `from cereal import log` — CarrotPilot 直接引用 `log.CarrotMan`，需改为 `log.CustomReserved10`（或在 capnp 生成后确认实际名称）
- `messaging.new_message('carrotMan')` — 确认新服务名与注册一致
- 所有 `Params` 键名检查 — CarrotPilot 使用的自定义 Params 键可能不在 sunnypilot 的 `params_keys.h` 中

#### 步骤 2.3 — 注册 Params 键

**文件**: `common/params_keys.h`

添加 CarrotPilot 使用的 Params 键：
```
SoundVolumeAdjust, SoundVolumeAdjustEngage,
AutoNaviCountDownMode, AutoNaviSpeedCtrlMode,
AutoTurnControl, AutoNaviSpeedBumpTime, AutoNaviSpeedCtrlEnd,
（等，从 config.py 中提取完整列表）
```

#### 步骤 2.4 — 注册进程

**文件**: `system/manager/process_config.py`

```python
PythonProcess("carrot_man", "selfdrive.carrot.carrot_man", always_run),
```

---

### 阶段三：语音播报移植（预计 1-2 天）

> 目标：全部音效文件和扩展的 soundd.py

#### 步骤 3.1 — 复制音效文件

```bash
# 复制新增的 WAV 文件（约 27 个新音效）
cp /home/lanyi/视频/openpilot/selfdrive/assets/sounds/audio_*.wav \
   /home/lanyi/sunnypilot/selfdrive/assets/sounds/

cp /home/lanyi/视频/openpilot/selfdrive/assets/sounds/traffic_*.wav \
   /home/lanyi/sunnypilot/selfdrive/assets/sounds/

cp /home/lanyi/视频/openpilot/selfdrive/assets/sounds/tici_*.wav \
   /home/lanyi/sunnypilot/selfdrive/assets/sounds/

# 英语音效包
cp -r /home/lanyi/视频/openpilot/selfdrive/assets/sounds_eng/ \
   /home/lanyi/sunnypilot/selfdrive/assets/sounds_eng/
```

#### 步骤 3.2 — 扩展 soundd.py

**文件**: `selfdrive/ui/soundd.py`

在 sunnypilot 现有 soundd.py 上增量修改：

1. 在 `SOUND_MAP` 字典中添加新的 AudibleAlert → WAV 映射
2. 添加 `update_carrot_alert()` 方法处理倒计时逻辑
3. 添加 `carrotMan` 消息订阅
4. 保留 sunnypilot 现有的 `QuietMode` 和 `SelfdriveStateSP` 逻辑
5. 整合 CarrotPilot 的语言切换和分离音量控制

**注意**：不要直接覆盖 sunnypilot 的 soundd.py，需要**合并**两边的逻辑。

#### 步骤 3.3 — 事件映射扩展

**文件**: `selfdrive/selfdrived/events.py`

添加新的 EventName → AudibleAlert 映射（变道、交通灯、盲区等）。

---

### 阶段四：导航引擎移植（预计 2-3 天）

> 目标：移植 navd 模块（如需要 Mapbox 导航）

#### 步骤 4.1 — 移植 navd

```bash
cp -r /home/lanyi/视频/openpilot/selfdrive/navd/ /home/lanyi/sunnypilot/selfdrive/navd/
```

**包含**：
- `navd.py` (~391 行) — Mapbox 路线计算
- `helpers.py` — 坐标辅助
- `map_renderer.py` — C++ 地图渲染 FFI
- `SConscript` — 编译脚本

#### 步骤 4.2 — Mapbox 地图渲染依赖

检查并移植：
- `selfdrive/navd/map_renderer.cc` — C++ 原生库
- Mapbox GL Native 依赖（需要编译）
- Mapbox API Token 配置

---

### 阶段五：UI 集成（预计 5-7 天，可选/延后）

> 目标：在 sunnypilot UI 中显示导航 HUD 信息

#### 步骤 5.1 — 数据层对接

在 sunnypilot 的 `selfdrive/ui/sunnypilot/` 中添加 carrotMan 消息订阅：
- `ui_scene.h` — 添加 carrotMan 数据字段
- `ui.cc` — 添加消息订阅和数据更新

#### 步骤 5.2 — HUD 渲染（按 sunnypilot 架构重写）

**不直接移植 carrot.cc/carrot_gpu.cc**，改为在 sunnypilot 的 Qt 框架中实现：

- 限速标志显示
- 转弯引导箭头和距离
- 测速摄像头警告
- 倒计时数字
- 路名显示
- 盲区警告指示

---

## 五、源文件对照表

| CarrotPilot 源文件 | Sunnypilot 目标 | 操作 |
|---|---|---|
| `selfdrive/carrot/*` | `selfdrive/carrot/*` | 整目录复制 + 修改引用 |
| `cereal/custom.capnp` | `cereal/custom.capnp` | 合并（使用 Reserved10/11） |
| `cereal/services.py` | `cereal/services.py` | 增量添加服务 |
| `cereal/car.capnp` AudibleAlert | `cereal/custom.capnp` SelfdriveStateSP | 映射到 SP 枚举体系 |
| `selfdrive/ui/soundd.py` | `selfdrive/ui/soundd.py` | 合并扩展 |
| `selfdrive/assets/sounds/*.wav` | `selfdrive/assets/sounds/*.wav` | 复制新增文件 |
| `selfdrive/assets/sounds_eng/` | `selfdrive/assets/sounds_eng/` | 整目录复制 |
| `selfdrive/navd/*` | `selfdrive/navd/*` | 整目录复制 |
| `selfdrive/ui/carrot.cc/.h` | `selfdrive/ui/sunnypilot/` | 按 SP 架构重写 |
| `system/manager/process_config.py` | `system/manager/process_config.py` | 增量添加进程 |
| `common/params_keys.h` | `common/params_keys.h` | 增量添加键名 |

---

## 六、依赖清单

### Python 依赖（已有）
- `sounddevice` — 音频输出
- `numpy` — 音频处理
- `cereal` (msgq/capnp) — 消息通信
- `pyzmq` — 远程命令（端口 7710）

### Python 依赖（可能需新增）
- `shapely` — 导航路径几何运算（可选）

### 系统依赖
- PortAudio — sounddevice 底层
- Mapbox GL Native — 地图渲染（阶段四）

### 外部应用
- **CarrotMan APK** — 手机端 Android 应用（需单独获取安装包）

---

## 七、风险评估

| 风险 | 等级 | 影响 | 缓解措施 |
|------|------|------|---------|
| Cereal 兼容性 | 🔴 高 | 消息序列化不兼容导致进程崩溃 | 严格验证 capnp 编译；单元测试 |
| Params 键冲突 | 🟡 中 | 参数读写失败 | 统一命名空间前缀 `Carrot*` |
| soundd.py 合并冲突 | 🟡 中 | QuietMode 等 SP 功能失效 | 增量修改，保留 SP 原有逻辑 |
| UI 架构差异 | 🟠 较高 | 渲染代码无法直接复用 | 阶段五延后处理，使用 SP 架构重写 |
| CarrotMan APK 依赖 | 🟡 中 | 手机端无法匹配 | 需确保 APK 版本与协议版本兼容 |

---

## 八、推荐执行顺序

```
阶段一 (基础设施) ──→ 阶段三 (语音播报) ──→ 阶段二 (核心逻辑) ──→ 阶段四 (导航) ──→ 阶段五 (UI)
       2-3天                 1-2天                3-5天               2-3天            5-7天

                        ← 最小可用版本 →
```

**推荐先完成阶段一+三**：只需改 cereal + soundd.py + 音效文件，不引入新进程，风险最低、见效最快。

**阶段二是核心价值交付**：完成后手机投射即可工作（数据层），即使没有 UI 显示也能驱动语音播报和速度控制。

**阶段五可延后或简化**：先用日志/调试信息验证数据通路，UI 后续逐步完善。

---

## 九、验证检查点

| 阶段 | 验证项 |
|------|--------|
| 阶段一完成 | `scons cereal/` 编译通过；Python 可 import CarrotMan 消息 |
| 阶段三完成 | soundd 能播放所有新增 WAV；倒计时 1-10 正常发声 |
| 阶段二完成 | carrot_man 进程启动无报错；UDP 7705 广播可被手机接收 |
| 阶段四完成 | navd 进程启动；Mapbox 路线可计算 |
| 阶段五完成 | 导航 HUD 在画面上正确渲染 |

---

## 十、实际移植进度总结

> 以下为 CarrotPilot → Sunnypilot 实际移植工作的完整记录。

### 10.1 总体进度

| 阶段 | 状态 | 说明 |
|------|------|------|
| 阶段一：基础设施层 | ✅ 已完成 | cereal 消息定义、服务注册、Params 键 |
| 阶段二：核心逻辑移植 | ✅ 已完成 | carrot 模块复制、进程注册、引用修正 |
| 阶段三：语音播报移植 | ✅ 已完成 | WAV 文件复制、soundd.py 全面改写 |
| 阶段四：导航引擎移植 | ⏭️ 跳过 | SP 已有独立 mapd/导航系统 |
| 阶段五：UI 集成 | ✅ 已完成 | drawCarrotPanel + 6 项新 HUD 功能 |

### 10.2 阶段一：基础设施层 — 完成详情

#### cereal/custom.capnp

- **CarrotMan 结构体**：利用 `CustomReserved10`（`@0xcb9fd56c7057593a`）定义，39 个字段
  - 导航数据：`xSpdType`, `xSpdLimit`, `xSpdDist`, `xSpdCountDown`, `xTurnInfo`, `xDistToTurn`, `xDesireRoad`
  - 道路信息：`roadName`, `roadCate`, `roadLimitSpeed`, `atcType`
  - 交通状态：`trafficState`, `trafficStopDist`
  - UI 数据：`leftBlind`, `rightBlind`, `extBlinker`, `vTurnSpeed`
  - 倒计时：`leftSec`
  - 其余约 15 个控制/状态字段

- **AmapNavi 结构体**：利用 `CustomReserved11`（`@0xc2243c65e0340384`）定义，2 个字段

- **AudibleAlertSP 枚举**：从 @9 到 @45，共 37 个值
  - @9 `audioTurn` ~ @23 `reverseGear2`：基础导航与驾驶音效
  - @24 `audio1` ~ @35 `audio10`：倒计时播报 (注: @31/@32 跳过 SP 已用槽位)
  - @36 `nnff` ~ @45 `laneChangeEnd`：NNFF + 变道系列音效

- **OnroadEventSP.EventName**：新增 `trafficSignGreen@24`, `trafficSignChanged@25`, `trafficStopping@26`

#### cereal/services.py

```python
"carrotMan":            (True, 0.),
"amapNavi":             (True, 0.),
"navInstructionCarrot": (True, 1., 10),
```

#### common/params_keys.h

新增 CarrotPilot 用到的所有 Params 键（`SoundVolumeAdjust` 等）。

### 10.3 阶段二：核心逻辑移植 — 完成详情

#### selfdrive/carrot/ 目录

整目录复制并修正引用路径：`carrot_man.py`, `carrot_serv.py`, `carrot_functions.py`, `amap_navi.py`, `config.py` 等。

#### system/manager/process_config.py

```python
PythonProcess("carrot_man", "selfdrive.carrot.carrot_man", always_run),
```

### 10.4 阶段三：语音播报移植 — 完成详情

#### 设计决策：绕过事件系统，直接在 soundd.py 监听 carrotMan

**原因**：
1. `EngagementAlert`（events_base.py）只接受 `car.CarControl.HUDControl.AudibleAlert`（值 0-8），无法传递 `AudibleAlertSP`（值 9-45）
2. 如果走事件系统，需要在 `custom.capnp` 中新增 ~20 个 `OnroadEventSP.EventName`，再在 `events.py` 中建立映射，改动面大
3. SP 的 `modelV2.meta` 没有 CP 的 `eventType` 字段，无法通过模型管道触发音频事件

**解决方案**：在 `soundd.py` 中直接订阅 `carrotMan` 消息，通过状态变化检测触发音频，完全独立于事件系统。

#### 音频文件

从 CP `sounds_eng/` 目录复制了 8 个缺失的 WAV 文件到 `selfdrive/assets/sounds/`：

| 文件 | 用途 |
|------|------|
| `audio_atc_cancel.wav` | 领航已退出 |
| `audio_atc_resume.wav` | 领航已恢复 |
| `audio_pre_lane_left.wav` | 准备左变道 |
| `audio_pre_lane_right.wav` | 准备右变道 |
| `audio_lane_change_ok.wav` | 变道已完成 |
| `audio_last_lane.wav` | 车辆已靠边 |
| `audio_new_lane.wav` | 出现新车道 |
| `audio_lane_change_end.wav` | 变道已结束 |

其余 ~20 个 WAV 文件（audio_turn, traffic_sign_green, audio_1~10 等）已在之前阶段复制完成。

#### soundd.py — update_carrot_alert() 完整重写

`update_carrot_alert(self, sm, new_alert)` 方法通过监控 carrotMan 字段变化触发音频：

**1. atcType 状态机 → 变道/转弯/领航音效**

| 状态变化 | 触发音效 |
|---------|----------|
| `"prepare"` 消失 + 含 `"atc"` | `preLaneChangeCarrot` (准备变道) |
| `"prepare"` 消失 + 含 `"fork"` + `"now"` | `laneChangeCarrot` (立即变道) |
| `"prepare"` 消失 + 含 `"fork"` | `preLaneChangeCarrot` (准备变道) |
| `"atc"` 消失 + atcType 清空 | `atcCancel` (领航退出) |
| `"atc"` 消失 + 含 `"turn"` | `audioTurn` (转弯提示) |
| 新出现 `"turn"` | `audioTurn` (转弯提示) |
| 新出现 `"now"` + 含 `"fork"` | `laneChangeCarrot` (立即变道) |
| 从空变为含 `"atc"`/`"fork"` | `atcResume` (领航恢复) |

**2. trafficState 状态变化 → 交通灯音效**

| 状态变化 | 触发音效 |
|---------|----------|
| → `2` (从非 2) | `trafficSignGreen` (绿灯通行) |
| → `1` (从非 1) | `trafficSignChanged` (红灯变化) |

**3. 测速摄像头接近 → 减速警告**

| 条件 | 触发音效 |
|------|----------|
| `xSpdLimit > 0` 首次出现 | `speedDown` (前方测速) |

**4. 倒计时播报（保留原有逻辑）**

| 条件 | 触发音效 |
|------|----------|
| `leftSec` = 1~10 | `audio1` ~ `audio10` |
| `leftSec` = 0 | `longDisengaged` |
| `leftSec` = 11 | `promptDistracted` |

#### soundd.py — get_audible_alert() 增强

新增独立的 `carrotMan` 更新分支：当 `selfdriveState` 没有更新但 `carrotMan` 更新时，也会检查并触发 carrot 音频事件，确保不遗漏。

#### events.py 中的交通灯事件

已有 `trafficSignChanged`, `trafficSignGreen`, `trafficStopping` 三个事件定义，但它们使用 `AudibleAlert.none`（不播放声音）。现在这些音效改由 soundd.py 直接处理，不再依赖事件系统。

### 10.5 阶段五：UI 集成 — 完成详情

#### drawCarrotPanel（左侧信息面板）

在 `selfdrive/ui/sunnypilot/qt/onroad/hud.cc` 中实现统一的 `drawCarrotPanel()` 函数：

- **位置**：左侧，距底部 44px
- **背景**：地中海蓝 (QColor(0, 100, 180, 200))，圆角 20px
- **布局**：竖向排列，从上到下

**面板内容**：

| 元素 | 数据来源 | 显示方式 |
|------|---------|---------|
| 转弯图标 | `xTurnInfo` (方向) | 16 种箭头 SVG (直行/左转/右转/掉头等) |
| 转弯距离 | `xDistToTurn` | 数字 + 单位 (m/km) |
| 路名 | `roadName` | 白色文字，最多 15 字符 |
| 限速牌 | `roadLimitSpeed` | 红圈白底限速标志 |
| 导航限速 | `xSpdLimit` | 黄色数字 |
| 测速距离 | `xSpdDist` | 伴随 xSpdLimit 显示 |
| 交通灯状态 | `trafficState` | 红/绿/灰三色圆点 |
| 交通灯距离 | `trafficStopDist` | 距停止线距离 |
| 曲率限速 | `vTurnSpeed` | 黄色数字，弯道图标下方 |

#### SP 限速牌 fallback

`drawSpeedSign()` 增加逻辑：当 SP 自身的 `navLimit` 为 0 时，fallback 读取 `carrotMan.roadLimitSpeed` 或 `carrotMan.xSpdLimit`，确保限速始终显示。

#### 6 项新增 HUD 功能

| 功能 | 字段 | 显示方式 |
|------|------|---------|
| **弯道限速** | `vTurnSpeed` | 黄色数字，CarrotPanel 内弯道图标下方 |
| **道路类别标签** | `roadCate` | 彩色圆角标签（高速=绿，城快=橙，国道=蓝，省道=紫，县道=红） |
| **左盲区警告** | `leftBlind` | 屏幕左边缘蓝色渐变条 |
| **右盲区警告** | `rightBlind` | 屏幕右边缘蓝色渐变条 |
| **转向灯指示** | `extBlinker` | 屏幕顶部绿色箭头 (1=左，2=右，3=双闪) |
| **倒计时显示** | `xSpdCountDown` | 面板内 countdown 数字 |

### 10.6 额外修复

#### Geely 油门踏板修复 (panda safety)

**文件**：`opendbc_repo/opendbc/safety/modes/geely.h`

**问题**：吉利车型在实验性纵向控制模式下，驾驶员踩油门后 OP 不恢复控制。

**根因**：吉利原厂 ACC 在踩油门时完全退出 → `pcm_cruise_check(false)` 清除 `controls_allowed` → 松开油门后 ACC 不自动恢复 → `controls_allowed` 保持 false。

**修复**：在 `gas_pressed_prev` 为 true 期间跳过 `pcm_cruise_check()`，只记录状态不清除权限：

```c
if (!gas_pressed_prev) {
  pcm_cruise_check(cruise_engaged);
} else {
  cruise_engaged_prev = cruise_engaged;
}
```

### 10.7 已知限制

#### SP 地图在 PC 上不工作

**原因**（不是 Google Maps 或 Mapbox token 问题）：
1. `mapd` + `mapd_manager` 在 `process_config.py` 中被 `enabled=not PC` 禁用
2. `mapd` 二进制是 ARM aarch64 架构，无法在 x86_64 PC 上运行
3. PC 上没有 GPS 硬件

**结论**：这是平台架构限制，非配置问题。如需 PC 调试需交叉编译或使用 QEMU。

#### modelV2.meta.eventType 不可用

SP 的 `log.capnp` 中 `modelV2.meta` 没有 CP 的 `eventType` 字段。CP 通过此字段从模型管道触发更精细的音频事件（如准备左/右变道、变道完成、新车道等 12 种事件类型）。当前 SP 的变道系列音效（`preLaneChangeLeft2`, `preLaneChangeRight2`, `laneChangeOk`, `lastLane`, `newLane`, `laneChangeEnd`）虽已有 WAV 文件和 AudibleAlertSP 枚举定义，但暂时无触发源。

---

## 十一、Controls/Planning 层 Carrot 集成差异分析

> 此章节记录 CarrotPilot (CP) 在控制/规划层的专有修改，以及 Sunnypilot (SP) 的当前移植状态。

### 11.1 总体对比

| 维度 | CP | SP | 移植状态 |
|------|----|----|---------|
| 控制层 carrot 引用数 | ~99 处 | ~52 处 | **部分移植** |
| `desire_helper.py` 行数 | 1256 行 | 188 行 | **大幅简化重写** |
| `lateral_planner.py` | 独立文件存在 | **不存在** | **未移植** |
| `lane_planner_2.py` | 存在(CP专有) | **不存在** | **未移植** |
| `controlsd.py` carrot 集成 | 有（carrotMan订阅+HUD） | **无** | **未移植** |
| `latcontrol_torque.py` | 304 行（含 carrot 调参） | 104 行 | **未移植** |
| `card.py` / `cruise.py` carrot 集成 | 有（46处引用） | **无** | **未移植** |
| `modeld.py` DesireHelper | 传入 carrotMan + radarState + amapNavi | 仅 carState + latActive | **部分移植** |

### 11.2 已移植到 SP 的 Controls 功能

#### ✅ plannerd.py — CarrotPlanner 纵向集成

CP 和 SP 均在 `plannerd.py` 中实例化 `CarrotPlanner` 并传递给纵向规划器。

**关键差异**：CP 同时传给 lateral_planner 和 longitudinal_planner，SP 只传给 longitudinal_planner（因 SP 无独立 lateral_planner）。

#### ✅ longitudinal_planner.py — 基本 Carrot 纵向控制

SP 已实现：
- `carrot.update(sm, v_cruise_kph, mode)` 速度覆盖
- `carrot.mode` MPC 模式切换
- `carrot.soft_hold_active` 重置状态检测
- `carrot.get_carrot_accel(v_ego)` 加速度上限
- `carrot.jerk_factor_apply` jerk 权重传递

**SP 特有架构**：通过 `LongitudinalPlannerSP` 叠加层同时集成了 SP 原生的 DEC / SCC / SLA 功能。

#### ✅ long_mpc.py — 基本 Carrot MPC 参数

SP 已实现：
- `carrot.get_T_FOLLOW()` 跟车距离
- `carrot.comfort_brake` / `carrot.stop_distance` 制动舒适度
- `carrot.dynamic_t_follow()` 动态跟车距离调整
- `carrot.xState` 交通灯停车障碍物注入（e2eStop 时）
- carrot 事件合并到 SP 事件通道

#### ✅ desire_helper.py — ATC 虚拟转向灯（简化版）

SP 重写了一个精简版本（188 行 vs CP 1256 行），实现了核心功能：
- `carrot_virtual_blinker` 虚拟转向灯（ATC 注入）
- `carrotMan.atcType` 响应（turn/atc/fork left/right）
- `carrotMan.carrotCmd` LANECHANGE/OVERTAKE 命令
- `_get_effective_direction()` 将虚拟转向灯合并到车道变更方向

### 11.3 CP 有但 SP 未移植的功能

#### ❌ 1. controlsd.py — carrotMan HUD/速度集成

CP 的 `controlsd.py` 有 6 处重要 carrot 集成：

```python
# CP controlsd.py 中的 carrot 集成（SP 中完全缺失）
sm = SubMaster([..., 'carrotMan', 'lateralPlan', 'radarState', ...])

# a. 曲率限速用于 lane-mode 切换
curve_speed_abs = abs(self.sm['carrotMan'].vTurnSpeed)
self.lanefull_mode_enabled = (lat_plan.useLaneLines and
                              curve_speed_abs > self.params.get_int("UseLaneLineCurveSpeed"))

# b. HUD desiredSpeed 覆盖
desired_kph = min(CS.vCruiseCluster, self.sm['carrotMan'].desiredSpeed)

# c. HUD activeCarrot / atcDistance 状态
hudControl.activeCarrot = self.sm['carrotMan'].activeCarrot
hudControl.atcDistance = self.sm['carrotMan'].xDistToTurn

# d. SpeedFromPCM 多模式 setSpeed 逻辑
if speed_from_pcm == 3:  # honda
    hudControl.setSpeed = setSpeed if lp.xState == 3 else float(desired_kph * CV.KPH_TO_MS)
```

**影响**：SP 的 controlsd 不感知 carrot 导航速度、ATC 距离、交通灯状态等，HUD 显示值无法动态适配。

#### ❌ 2. lateral_planner.py — 完整的横向路径规划

CP 有一个独立的 `lateral_planner.py`（~280 行），包含：
- **LanePlanner2 车道线规划**：`lane_planner_2.py` 用于车道线模式路径生成
- **Lane-mode / Laneless-mode 自动切换**：基于速度 + 模型减速预测
- **carrot ATC 车道线乘数归零**：ATC 激活时 `lane_change_multiplier = 0.0`
- **carrotMan.vTurnSpeed 曲率限速传递**
- **完整的 MPC 参数动态调整**：`LatMpcPathCost`, `LatMpcMotionCost`, `LatMpcAccelCost`, `LatMpcJerkCost`, `LatMpcSteeringRateCost` 全部从 Params 读取
- **pathOffset 路径偏移参数化**
- **radar 侧向车辆信息发布**：`leadLeft/leadRight` 距离和速度写入 lateralPlan debug

**SP 无此文件**，横向规划完全依赖 openpilot 默认的 `model_v2.action.desiredCurvature`。

#### ❌ 3. lane_planner_2.py — 车道线融合路径

CP 专有的 `lane_planner_2.py` 提供：
- 四车道线概率解析（`llll_prob`, `lll_prob`, `rll_prob`, `rrll_prob`）
- 车道宽度自适应计算
- lanefull_mode 路径与 model 路径的融合
- 曲率限速场景的偏移计算

**SP 无此文件**。

#### ❌ 4. long_mpc.py — CP 专有的高级纵向 MPC 特性

SP 缺失的 CP long_mpc 高级功能：

| 特性 | CP 实现 | SP 当前状态 |
|------|--------|-----------|
| **j_lead（前车 jerk）追踪** | `self.j_lead` 平滑滤波 + `extrapolate_lead` 含 j_lead 项 | 不存在，`extrapolate_lead` 无 j_lead 参数 |
| **前车急减速修正** | `if a_lead < -2.0 and j_lead > 0.5: 修正 a_lead` | 不存在 |
| **j_lead_factor 参数** | `carrot.j_lead_factor` 控制前车 jerk 影响权重 | 不存在 |
| **PARAM_DIM = 8** | MPC 参数含 `comfort_brake` + `stop_distance` | PARAM_DIM = 6，无这两个参数 |
| **moreRelaxed 驾驶人格** | T_FOLLOW 和 jerk 有 moreRelaxed 选项 | 不存在 |
| **set_accel_limits()** | 动态设置 `cruise_min_a` / `max_a` | 不存在，使用固定 `ACCEL_MIN/ACCEL_MAX` |
| **a_change_cost 动态调整** | 基于 j_lead 大小：急减速时降低到 20 | 固定 `A_CHANGE_COST = 200` |
| **交通灯停车距离调整** | `carrot.trafficStopDistanceAdjust` + `stop_x` 融合 | 简单的 `carrot.stop_dist` < 900 判断 |
| **导航限速减速率** | `carrot.autoNaviSpeedDecelRate` 设置 `params[:,0]` | 不存在 |
| **vCluRatio 集群速度修正** | `v_cruise *= vCluRatio` | 不存在（longitudinal_planner 层也缺失） |

#### ❌ 5. longitudinal_planner.py — CP 专有发布字段

CP 的 `longitudinal_planner.publish()` 发布以下 SP 缺失的字段：

```python
# CP 发布到 longitudinalPlan 但 SP 不发布的字段
longitudinalPlan.xState = carrot.xState.value            # 交通灯/E2E 状态
longitudinalPlan.trafficState = carrot.trafficState.value # 交通灯颜色
longitudinalPlan.cruiseTarget = self.v_cruise_kph         # 巡航目标速度
longitudinalPlan.tFollow = float(self.mpc.t_follow)       # 跟车时间
longitudinalPlan.desiredDistance = float(self.mpc.desired_distance)  # 期望跟车距离
longitudinalPlan.events = carrot.events.to_msg()          # carrot 事件
longitudinalPlan.myDrivingMode = carrot.myDrivingMode.value  # 驾驶模式
longitudinalPlan.vTargetNow = float(self.output_v_target_now)
longitudinalPlan.jTargetNow = float(self.output_j_target_now)
```

**影响**：UI 层无法显示 xState、trafficState、跟车距离等信息，soundd 也无法触发基于这些状态的语音。

> **注意**：SP 通过独立的 `longitudinalPlanSP` 消息发布自己的 DEC/SCC/SLA 状态，carrot 事件通过 `EventsSP` 合并。但缺少 xState/trafficState/tFollow/desiredDistance 等实时调试信息。

#### ❌ 6. card.py — 雷达 update_carrot + softHold + carrotCruise

CP 的 `card.py` 有三个重要 carrot 集成：

```python
# a. 雷达接口使用 carrot 增强版
RD = self.RI.update_carrot(CS.vEgo, CS.aEgo, rcv_time, can_list)

# b. softHoldActive 从 cruise_helper 传入 CarState
CS.softHoldActive = self.v_cruise_helper._soft_hold_active
self.CI.CS.softHoldActive = CS.softHoldActive

# c. carrotCruise 标志位
CS.carrotCruise = 1 if self.v_cruise_helper.carrot_cruise_active else 0
```

SP 的 `card.py` 完全没有这些集成。

#### ❌ 7. cruise.py — carrot 远程巡航控制

CP 的 `cruise.py` 有 30+ 处 carrot 集成，核心功能：
- **carrot_cruise_active**: carrot 接管巡航激活状态
- **carrot_command()**: 处理来自 carrotMan 的远程巡航命令（CRUISE ON/OFF/GO/STOP, SPEED UP/DOWN/SET）
- **nRoadLimitSpeed / desiredSpeed**: 从 carrotMan 读取导航限速和目标速度
- **carrot_cruise_active** 影响待机状态恢复逻辑

SP 的 `cruise.py` 没有任何 carrot 集成。

#### ❌ 8. desire_helper.py — CP 完整版 1256 行 vs SP 简化版 188 行

SP 的简化版已包含核心 ATC 触发逻辑，但 CP 完整版额外包含大量特性：

| CP 完整版特性 | SP 简化版 |
|-------------|----------|
| 盲区检测整合（`carrotMan.leftBlind/rightBlind + amapNavi`） | 仅 `carstate.leftBlindspot/rightBlindspot` |
| 基于 `carrot_lane_change_count` 的计时器（0.2秒精度） | 无独立计数器 |
| `carrot_overtake_cmd` 超车模式标志 | 无 |
| `stockBlinkerCtrl` 原车转向灯策略 | 无 |
| `xroadcate` 道路类别（高速/城快等）影响变道策略 | 无 |
| `xDistToTurn` / `xLeftTurnSec` 转弯距离/时间感知 | 无 |
| atc fork/turn 区分不同车道变更策略 | 简单统一处理 |
| `desireLog` 调试日志字段 | 无 |
| `event_type` / `event_type_id` 精细事件编码 | 无 |
| `leftFrontBlind` / `rightFrontBlind` 前方盲区 | 无 |
| `lane_width_left/right` / `distance_to_road_edge_left/right` | 无 |
| radarState 侧向车辆检测整合 | 无 |

#### ❌ 9. modeld.py — DesireHelper 调用差异

| 方面 | CP | SP |
|------|----|----|
| SubMaster 订阅 | + `carrotMan`, `radarState`, `amapNavi` | 无这三个 |
| DH.update() 签名 | `(carState, modelV2, latActive, prob, carrotMan, radarState)` | `(carState, latActive, prob)` |
| DH.update() 传入 modelV2 | ✅ 用于 lane width/road edge 分析 | ❌ |
| 发布 meta.desire | ✅ `DH.desire` | ❌ 通过 DH 而非模型覆盖 |
| 发布 meta.desireLog | ✅ | ❌ |
| 发布 meta.eventType | ✅ 12 种精细事件 | ❌ |
| 发布 meta.blinker | ✅ | ❌ |
| 发布 meta.laneWidthLeft/Right | ✅ | ❌ |
| 发布 meta.leftFrontBlind/rightFrontBlind | ✅ | ❌ |

#### ❌ 10. latcontrol_torque.py — 横向控制调参

CP 的 `latcontrol_torque.py` 有 304 行，SP 只有 104 行。CP 扩展包含：
- `LateralTorqueCustom` 参数化 latAccelFactor / latAccelOffset / friction 覆盖
- NNFF (Neural Network Feedforward) 集成（`use_nnff` / `use_nnff_lite`）
- 动态调参从 Params 实时读取

### 11.4 移植优先级建议

| 优先级 | 功能 | 影响 |
|--------|------|------|
| **P0 - 阻塞** | controlsd.py carrotMan 订阅 + HUD 字段 | 无此不显示 ATC 距离、导航速度、activeCarrot |
| **P0 - 阻塞** | longitudinalPlan xState/trafficState 发布 | soundd 交通灯语音和 HUD 交通灯状态依赖 |
| **P1 - 高** | cruise.py carrot_command 远程巡航 | app 无法遥控巡航开关/速度调节 |
| **P1 - 高** | card.py softHoldActive + carrotCruise | 停车保持和 carrot 巡航激活依赖 |
| **P1 - 高** | long_mpc j_lead 追踪 + 动态 a_change_cost | 跟车舒适度和前车急刹反应 |
| **P2 - 中** | lateral_planner.py + lane_planner_2.py | 车道线模式路径规划 |
| **P2 - 中** | modeld.py DH 增强（carrotMan/radarState 传入） | 变道精细事件 + 前方盲区检测 |
| **P2 - 中** | desire_helper 完整版特性 | 盲区整合/计时器/超车模式 |
| **P3 - 低** | latcontrol_torque 调参扩展 | 横向控制参数化调优 |
| **P3 - 低** | vCluRatio 集群速度修正 | 特定车型仪表盘速度修正 |
| **P3 - 低** | moreRelaxed 驾驶人格 | 额外一级驾驶风格选项 |

---

## 十二、文件修改清单

### 新增文件

| 文件路径 | 说明 |
|---------|------|
| `selfdrive/carrot/` (整目录) | CarrotPilot 核心逻辑模块 |
| `selfdrive/assets/sounds/audio_atc_cancel.wav` | 领航退出音效 |
| `selfdrive/assets/sounds/audio_atc_resume.wav` | 领航恢复音效 |
| `selfdrive/assets/sounds/audio_pre_lane_left.wav` | 准备左变道音效 |
| `selfdrive/assets/sounds/audio_pre_lane_right.wav` | 准备右变道音效 |
| `selfdrive/assets/sounds/audio_lane_change_ok.wav` | 变道完成音效 |
| `selfdrive/assets/sounds/audio_last_lane.wav` | 车辆靠边音效 |
| `selfdrive/assets/sounds/audio_new_lane.wav` | 新车道音效 |
| `selfdrive/assets/sounds/audio_lane_change_end.wav` | 变道结束音效 |
| (以及其余 ~20 个 audio_*.wav / traffic_*.wav / tici_*.wav) | 基础导航音效 |

### 修改文件

| 文件路径 | 修改内容 |
|---------|---------|
| `cereal/custom.capnp` | CarrotMan/AmapNavi 结构体；AudibleAlertSP 枚举扩展；OnroadEventSP 新增 |
| `cereal/services.py` | 注册 carrotMan/amapNavi/navInstructionCarrot 服务 |
| `common/params_keys.h` | 添加 Carrot 相关 Params 键 |
| `system/manager/process_config.py` | 注册 carrot_man 进程 |
| `selfdrive/ui/soundd.py` | update_carrot_alert() 重写；get_audible_alert() 增强；状态追踪变量 |
| `selfdrive/ui/sunnypilot/qt/onroad/hud.cc` | drawCarrotPanel()；盲区/转向灯/弯道限速/道路类别/倒计时 |
| `selfdrive/ui/sunnypilot/qt/onroad/hud.h` | 新增 carrot UI 字段声明 |
| `selfdrive/selfdrived/events.py` | trafficSignGreen/Changed/Stopping + softHold + audioPrompt 事件定义 |
| `opendbc_repo/opendbc/safety/modes/geely.h` | 油门踏板 controls_allowed 修复 |
| `cereal/car.capnp` | CarState @61-68 carrot 字段；HUDControl @11-25 carrot 字段；ButtonEvent.Type 新增 lfaButton/paddleLeft/paddleRight |
| `cereal/log.capnp` | LongitudinalPlan @40-48 carrot 字段；ControlsState @67 activeLaneLine；OnroadEvent softHold/audioPrompt |
| `selfdrive/car/cruise.py` | VCruiseCarrot 类（~600 行）— carrot 巡航控制核心 |
| `selfdrive/car/card.py` | VCruiseCarrot 替换 VCruiseHelper；SubMaster 增加 carrotMan/longitudinalPlan/radarState/modelV2；CS carrot 字段赋值 |
| `selfdrive/controls/controlsd.py` | SubMaster 增加 carrotMan/radarState；hudControl carrot 字段；desiredSpeed/setSpeed 逻辑；_update_side() 侧方车辆检测；activeLaneLine |
| `selfdrive/car/car_specific.py` | activateCruise/softHold 事件；carrotCruise/vCruise audioPrompt 追踪；geely 品牌支持 |
