#!/bin/bash
cd /data/openpilot/camera/ && 
source op_yolo_venv/bin/activate && 
cd /data/openpilot/camera && 
python3 lane.py

