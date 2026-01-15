#!/bin/bash
cd /data/openpilot/camera/ && 
source op_yolo_venv/bin/activate && 
cd /data/openpilot/camera && 
export PYTHONPATH=/data/openpilot
python3 lane.py

