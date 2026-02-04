#!/usr/bin/env python3
"""
VAAPI 优化的摄像头类
使用硬件加速解码 MJPEG 并直接输出 NV12
适用于 AMD Ryzen 8845HS + Radeon 780M

完全兼容原 camera.py 的 Camera 和 CameraMJPG 类
推荐使用 CameraVAAPI 替代 CameraMJPG
"""
import av
import threading
import queue
import numpy as np
import os


def bgr_to_nv12_bytes(bgr_frame):
    """
    BGR 到 NV12 转换函数（模块级，用于进程池）

    Args:
        bgr_frame: BGR 格式的 numpy 数组

    Returns:
        NV12 格式的字节数据
    """
    frame = av.VideoFrame.from_ndarray(bgr_frame, format='bgr24')
    return frame.reformat(format='nv12').to_ndarray().data.tobytes()


class Camera:
    """
    简单的同步摄像头类（无多线程）
    功能与原 camera.py 的 Camera 完全一致

    流程: V4L2 -> PyAV 解码 -> BGR -> NV12 -> 输出
    """

    def __init__(self, cam_type_state, stream_type, camera_id):
        """
        初始化简单摄像头

        Args:
            cam_type_state: 摄像头类型状态
            stream_type: 流类型
            camera_id: 摄像头设备 ID
        """
        try:
            camera_id = int(camera_id)
        except ValueError:  # 允许字符串，例如: /dev/video0
            pass

        self.cam_type_state = cam_type_state
        self.stream_type = stream_type
        self.cur_frame_id = 0

        self.container = av.open(camera_id)
        assert self.container.streams.video, f"无法打开摄像头视频流 {camera_id}"
        self.video_stream = self.container.streams.video[0]
        self.W = self.video_stream.codec_context.width
        self.H = self.video_stream.codec_context.height

    @classmethod
    def bgr2nv12(cls, bgr):
        """
        BGR 到 NV12 转换（类方法）

        Args:
            bgr: BGR 格式的 numpy 数组

        Returns:
            NV12 格式的 numpy 数组
        """
        frame = av.VideoFrame.from_ndarray(bgr, format='bgr24')
        return frame.reformat(format='nv12').to_ndarray()

    def read_frames(self):
        """
        生成器：读取帧并转换为 NV12

        Yields:
            NV12 格式的字节数据
        """
        for frame in self.container.decode(self.video_stream):
            img = frame.to_rgb().to_ndarray()[:, :, ::-1]  # 转换为 BGR24
            yuv = Camera.bgr2nv12(img)
            yield yuv.data.tobytes()
        self.container.close()


class CameraVAAPI:
    """
    VAAPI 硬件加速摄像头
    功能完全兼容原 CameraMJPG
    
    流程: V4L2 (MJPEG) -> VAAPI 硬件解码 -> NV12 -> 输出
    """

    def __init__(self, cam_type_state, stream_type, camera_id, num_workers=None, max_queue_size=10, use_processes=False):
        """
        初始化 VAAPI 摄像头（完全兼容 CameraMJPG 接口）

        Args:
            cam_type_state: 摄像头类型状态
            stream_type: 流类型
            camera_id: 摄像头设备 ID
            num_workers: 工作线程数（默认 CPU 核心数）
            max_queue_size: 队列最大大小
            use_processes: 忽略此参数（VAAPI 不需要进程池）
        """
        try:
            camera_id = int(camera_id)
        except ValueError:
            pass

        self.cam_type_state = cam_type_state
        self.stream_type = stream_type
        self.camera_id = camera_id
        self.cur_frame_id = 0

        # 多线程配置
        if num_workers is None:
            num_workers = os.cpu_count() or 4
        self.num_workers = num_workers

        # VAAPI 配置
        options = {
            'hwaccel': 'vaapi',
            'hwaccel_device': '/dev/dri/renderD128',
            'hwaccel_output_format': 'vaapi',
            'framerate': '20',
            'video_size': '2592x1944',
            'input_format': 'mjpeg',
        }

        # 打开摄像头
        self.container = av.open(camera_id, options=options, format='v4l2')
        self.video_stream = self.container.streams.video[0]

        # 获取参数
        self.W = self.video_stream.codec_context.width
        self.H = self.video_stream.codec_context.height
        self.fps = self.video_stream.average_rate
        self.current_format = 'MJPEG'

        # 读取线程直接输出到 output_queue，无需中间队列
        self.read_thread = threading.Thread(target=self._frame_reader, daemon=True)
        self.read_thread.start()

        print(f"VAAPI 摄像头初始化:")
        print(f"  分辨率: {self.W}x{self.H}")
        print(f"  帧率: {self.fps}")
        print(f"  数据格式: {self.current_format}")
        print(f"  硬件加速: VAAPI (AMD Radeon 780M)")
        print(f"  队列大小: {max_queue_size}")

    def _frame_reader(self):
        """后台线程：直接输出到队列"""
        # 使用单个队列，避免多次拷贝
        self.frame_queue = queue.Queue(maxsize=10)
        try:
            for packet in self.container.demux(self.video_stream):
                if self.stop_event.is_set():
                    break

                for frame in packet.decode():
                    if self.stop_event.is_set():
                        break

                    # VAAPI 解码后直接转 NV12（避免不必要的 RGB 转换）
                    nv12_frame = frame.reformat(format='nv12')
                    nv12_bytes = nv12_frame.to_ndarray().data.tobytes()

                    # 检查 NV12 数据大小
                    expected_size = self.W * self.H * 3 // 2
                    if len(nv12_bytes) != expected_size:
                        self.stop_event.set()
                        print(f"NV12 数据大小不匹配: {len(nv12_bytes)} != {expected_size}")
                        break

                    self.cur_frame_id += 1

                    # 直接放入队列，队满时丢弃旧帧
                    try:
                        self.frame_queue.put(nv12_bytes, timeout=0.1)
                    except queue.Full:
                        # 队列满，丢弃最旧的帧
                        try:
                            _ = self.frame_queue.get_nowait()
                            self.frame_queue.put(nv12_bytes)
                        except queue.Empty:
                            pass

        except Exception as e:
            print(f"VAAPI 异步读取错误: {e}")
        finally:
            self.container.close()

    def read_frames(self):
        """
        生成器：从队列读取帧

        Yields:
            NV12 格式的字节数据
        """
        try:
            while not self.stop_event.is_set():
                try:
                    nv12_bytes = self.frame_queue.get(timeout=0.5)
                    yield nv12_bytes
                    self.frame_queue.task_done()
                except queue.Empty:
                    if self.stop_event.is_set():
                        break
        finally:
            self.stop_event.set()
            self.read_thread.join()
            if hasattr(self, 'container'):
                self.container.close()

    def __del__(self):
        self.stop_event.set()
        if hasattr(self, 'read_thread'):
            self.read_thread.join(timeout=1)

if __name__ == '__main__':
    import time

    print("=" * 60)
    print("VAAPI 摄像头测试")
    print("=" * 60)

    # 测试简单 Camera 类
    print("\n1. 测试简单 Camera 类:")
    try:
        cam = Camera('roadCameraState', 'VISION_STREAM_ROAD', '/dev/video0')

        frame_count = 0
        start_time = time.time()

        for nv12 in cam.read_frames():
            frame_count += 1
            print(f"帧 {frame_count}: 大小 {len(nv12)} 字节")

            if frame_count >= 10:
                break

        elapsed = time.time() - start_time
        fps = frame_count / elapsed
        print(f"平均 FPS: {fps:.2f}")

    except Exception as e:
        print(f"简单 Camera 测试失败: {e}")

    # 测试 CameraVAAPI 类
    print("\n2. 测试 CameraVAAPI 类:")
    try:
        cam = CameraVAAPI('roadCameraState', 'VISION_STREAM_ROAD', '/dev/video0')

        frame_count = 0
        start_time = time.time()

        for nv12 in cam.read_frames():
            frame_count += 1
            print(f"帧 {frame_count}: 大小 {len(nv12)} 字节")

            if frame_count >= 10:
                break

        elapsed = time.time() - start_time
        fps = frame_count / elapsed
        print(f"平均 FPS: {fps:.2f}")

    except Exception as e:
        print(f"CameraVAAPI 测试失败: {e}")

    print("\n" + "=" * 60)
    print("测试完成")
    print("=" * 60)
