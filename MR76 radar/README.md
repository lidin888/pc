# MR76毫米波雷达接入指南

本文档介绍了如何在外接MR76毫米波雷达到ajouatom-pc系统中。

## 文件列表

需要替换以下三个文件：

1. `/opendbc/dbc/u_radar.dbc` - DBC定义文件
2. `/opendbc/car/radar_interface.py` - 雷达接口实现文件
3. `/selfdrive/car/card.py` - 车辆接口主控文件（需要备份原文件）

## 接入步骤

### 1. 备份原始文件

在替换任何文件之前，请务必备份原始文件：

```bash
# 备份card.py文件
cp /home/ajouatom-pc/selfdrive/car/card.py /home/ajouatom-pc/selfdrive/car/card.py.backup
```

### 2. 替换DBC文件

将MR76雷达的DBC定义文件复制到指定位置：

```bash
cp /home/ajouatom-pc/MR76\ radar/u_radar.dbc /home/ajouatom-pc/opendbc_repo/opendbc/dbc/u_radar.dbc
```

该文件定义了雷达的数据格式，包括：
- RadarState: 雷达状态消息
- Status: 雷达检测到的目标数量等状态信息
- ObjectData: 目标物体数据（ID、距离、速度等）
- ObjectData_0 至 ObjectData_11: 最多支持12个目标物体的数据

### 3. 替换雷达接口文件

将自定义的雷达接口实现文件复制到指定位置：

```bash
cp /home/ajouatom-pc/MR76\ radar/radar_interface.py /home/ajouatom-pc/opendbc_repo/opendbc/car/radar_interface.py
```

此文件实现了以下功能：
- 解析来自MR76雷达的CAN数据
- 过滤无效或不需要的目标物体
- 提供目标物体的距离、速度等信息给系统其他模块使用

#### 主要参数说明：

- `DREL_OFFSET`: 车头到雷达的距离偏移量
- `LAT_OFFSET`: 雷达左右位置修正值
- `MAX_OBJECTS`: 最大处理目标数（默认12个）
- `MAX_LAT_DIST`: 最大横向距离（默认3.7米）
- `MIN_DIST`: 最小检测距离（默认0.5米）
- `STATIONARY_OBJ_VREL`: 静止物体的相对速度阈值
- `CLOSED_OBJ_DREL`: 近距离物体的距离阈值
- `MIN_RCS`: 最小雷达截面值
- `IGNORE_OBJ_STATE`: 忽略的目标状态类型
- `NOT_SEEN_INIT`: 目标未被检测到的最大次数

### 4. 替换车辆接口主控文件

将修改后的车辆接口主控文件复制到指定位置：

```bash
cp /home/ajouatom-pc/MR76\ radar/card.py /home/ajouatom-pc/selfdrive/car/card.py
```

该文件主要修改了雷达数据处理的部分，确保能够正确接收和解析MR76雷达的数据。

### 5. 重启系统

完成上述文件替换后，重启系统使更改生效：

```bash
sudo reboot
```

## 验证雷达连接

系统启动后，可以通过以下方式验证雷达是否正常工作：

1. 检查是否有liveTracks消息输出：
   ```bash
   cereal_logger liveTracks
   selfdrive/debug/can_printer.py --bus 2 #这是op最底层显示can信号的命令
   ```

2. 查看雷达数据是否在系统界面中显示

## 故障排除

如果雷达数据没有正常显示，请检查：

1. 硬件连接是否正确
2. 雷达是否正常供电
3. CAN总线通信是否正常
4. 对应的DBC文件是否正确加载
5. radar_interface.py中的过滤参数是否合适

如有必要，可以调整[radar_interface.py](file:///home/ajouatom-pc/MR76%20radar/radar_interface.py)中的参数以适应具体的应用场景。