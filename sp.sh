
#!/bin/bash
sudo chmod 777 /dev/bus/usb/*
sudo udevadm control --reload-rules && sudo udevadm trigger
cd /home/radxa/pc
source .venv/bin/activate
export FINGERPRINT="TOYOTA_COROLLA_TSS2"
# USB摄像头配置（主摄像头+广角摄像头）
USE_WEBCAM=1 \
ROAD_CAM_PATH=/dev/video0 \
ROAD_CAM_WIDTH=1920 \
ROAD_CAM_HEIGHT=1080 \
ROAD_CAM_FRAMERATE=20 \
WIDE_ROAD_CAM_PATH=/dev/video2 \
WIDE_ROAD_CAM_WIDTH=1920 \
WIDE_ROAD_CAM_HEIGHT=1080 \
WIDE_ROAD_CAM_FRAMERATE=20 \
system/manager/manager.py
