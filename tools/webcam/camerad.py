#!/usr/bin/env python3
import threading
import os
from collections import namedtuple

from msgq.visionipc import VisionIpcServer, VisionStreamType
from cereal import messaging

from openpilot.tools.webcam.camera import CameraMJPG
from openpilot.common.realtime import Ratekeeper

WIDE_CAM = os.getenv("WIDE_CAM")
NO_DM = os.getenv("NO_DM") is not None
CameraType = namedtuple("CameraType", ["msg_name", "stream_type", "cam_id"])
CAMERAS = [
  CameraType("roadCameraState", VisionStreamType.VISION_STREAM_ROAD, os.getenv("ROAD_CAM", "0")),
  # CameraType("driverCameraState", VisionStreamType.VISION_STREAM_DRIVER, os.getenv("DRIVER_CAM", "2")),
]
if not NO_DM:
  CAMERAS.append(CameraType("driverCameraState", VisionStreamType.VISION_STREAM_DRIVER, os.getenv("DRIVER_CAM", "2")))
if WIDE_CAM:
  CAMERAS.append(CameraType("wideRoadCameraState", VisionStreamType.VISION_STREAM_WIDE_ROAD, WIDE_CAM))

class Camerad:
  def __init__(self):
    self.pm = messaging.PubMaster([c.msg_name for c in CAMERAS])
    self.vipc_server = VisionIpcServer("camerad")

    self.cameras = []
    for c in CAMERAS:
      cam_device = f"/dev/video{c.cam_id}"
      print(f"opening {c.msg_name} at {cam_device}")
      cam = CameraMJPG(c.msg_name, c.stream_type, cam_device)

      # Attach hardcoded calibration based on camera type
      if c.stream_type == VisionStreamType.VISION_STREAM_WIDE_ROAD:
        # Wide-angle camera calibration (from 123/123/photon/广角, 9x9 corners, 2cm squares)
        # Latest calibration: updated with 10 photos, 10 valid (recalibrated)
        cam.intrinsics = [[1234.63840469, 0.0, 1006.0986037],
                          [0.0, 1240.26283227, 586.29319731],
                          [0.0, 0.0, 1.0]]
        cam.dist_coeffs = [-0.43760033, 0.23128919, -0.00500692, 0.00146263, -0.08478902]
        print("Using latest wide-angle camera calibration (recalibrated)")
      else:
        # Road camera calibration (from 123/123/photon/1, 9x9 corners, 2cm squares)
        # 19 photos, 18 valid, RMS=0.66px (2026-01-07)
        cam.intrinsics = [[1657.98064, 0.0, 1920.0/2],
                          [0.0, 1664.95430, 1080.0/2],
                          [0.0, 0.0, 1.0]]
        cam.dist_coeffs = [0.32457375, -2.02586754, -0.00388917, -0.01334650, 3.90317772]
        print("Using road camera calibration")

      self.cameras.append(cam)
      self.vipc_server.create_buffers(c.stream_type, 20, cam.W, cam.H)

    self.vipc_server.start_listener()

  def _send_yuv(self, yuv, frame_id, pub_type, yuv_type, cam=None):
    eof = int(frame_id * 0.05 * 1e9)
    self.vipc_server.send(yuv_type, yuv, frame_id, eof, eof)
    dat = messaging.new_message(pub_type, valid=True)
    msg = {
      "frameId": frame_id,
      "transform": [1.0, 0.0, 0.0,
                    0.0, 1.0, 0.0,
                    0.0, 0.0, 1.0]
    }
    # Note: FrameData struct doesn't have intrinsics/distortionCoefficients fields
    # Camera intrinsics are stored on cam object for internal use only
    setattr(dat, pub_type, msg)
    self.pm.send(pub_type, dat)

  def camera_runner(self, cam):
    rk = Ratekeeper(20, None)
    for yuv in cam.read_frames():
      self._send_yuv(yuv, cam.cur_frame_id, cam.cam_type_state, cam.stream_type, cam)
      cam.cur_frame_id += 1
      rk.keep_time()

  def run(self):
    threads = []
    for cam in self.cameras:
      cam_thread = threading.Thread(target=self.camera_runner, args=(cam,))
      cam_thread.start()
      threads.append(cam_thread)

    for t in threads:
      t.join()


def main():
  camerad = Camerad()
  camerad.run()


if __name__ == "__main__":
  main()