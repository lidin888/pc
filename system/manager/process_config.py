import os
import operator
import platform

from cereal import car, custom
from openpilot.common.params import Params
from openpilot.system.hardware import PC, TICI
from openpilot.system.manager.process import PythonProcess, NativeProcess, DaemonProcess
from openpilot.system.hardware.hw import Paths

from openpilot.sunnypilot.mapd.mapd_manager import MAPD_PATH

from sunnypilot.models.helpers import get_active_model_runner
from sunnypilot.sunnylink.utils import sunnylink_need_register, sunnylink_ready, use_sunnylink_uploader
import glob

WEBCAM = os.getenv("USE_WEBCAM") is not None
USBCAM = os.getenv("USE_USBCAM") is not None
NO_DM = os.getenv("NO_DM") is not None

# def usb_imu_available() -> bool:
#   """检测 USB IMU 设备是否连接"""
#   usb_devices = glob.glob("/dev/ttyUSB*") + glob.glob("/dev/ttyACM*")
#   return len(usb_devices) > 0

def driverview(started: bool, params: Params, CP: car.CarParams) -> bool:
  return started or params.get_bool("IsDriverViewEnabled")

def notcar(started: bool, params: Params, CP: car.CarParams) -> bool:
  return started and CP.notCar

def iscar(started: bool, params: Params, CP: car.CarParams) -> bool:
  return started and not CP.notCar

def logging(started: bool, params: Params, CP: car.CarParams) -> bool:
  run = (not CP.notCar) or not params.get_bool("DisableLogging")
  return started and run

def ublox_available() -> bool:
  return os.path.exists('/dev/ttyHS0') and not os.path.exists('/persist/comma/use-quectel-gps')

def ublox(started: bool, params: Params, CP: car.CarParams) -> bool:
  use_ublox = ublox_available()
  if use_ublox != params.get_bool("UbloxAvailable"):
    params.put_bool("UbloxAvailable", use_ublox)
  return started and use_ublox

def joystick(started: bool, params: Params, CP: car.CarParams) -> bool:
  return started and params.get_bool("JoystickDebugMode")

def not_joystick(started: bool, params: Params, CP: car.CarParams) -> bool:
  return started and not params.get_bool("JoystickDebugMode")

def long_maneuver(started: bool, params: Params, CP: car.CarParams) -> bool:
  return started and params.get_bool("LongitudinalManeuverMode")

def not_long_maneuver(started: bool, params: Params, CP: car.CarParams) -> bool:
  return started and not params.get_bool("LongitudinalManeuverMode")

def qcomgps(started: bool, params: Params, CP: car.CarParams) -> bool:
  return started and not ublox_available()

def always_run(started: bool, params: Params, CP: car.CarParams) -> bool:
  return True

def only_onroad(started: bool, params: Params, CP: car.CarParams) -> bool:
  return started

def only_offroad(started: bool, params: Params, CP: car.CarParams) -> bool:
  return not started

def use_github_runner(started, params, CP: car.CarParams) -> bool:
  return not PC and params.get_bool("EnableGithubRunner") and (
    not params.get_bool("NetworkMetered") and not params.get_bool("GithubRunnerSufficientVoltage"))

def use_copyparty(started, params, CP: car.CarParams) -> bool:
  return bool(params.get_bool("EnableCopyparty"))

def sunnylink_ready_shim(started, params, CP: car.CarParams) -> bool:
  """Shim for sunnylink_ready to match the process manager signature."""
  return sunnylink_ready(params)

def sunnylink_need_register_shim(started, params, CP: car.CarParams) -> bool:
  """Shim for sunnylink_need_register to match the process manager signature."""
  return sunnylink_need_register(params)

def use_sunnylink_uploader_shim(started, params, CP: car.CarParams) -> bool:
  """Shim for use_sunnylink_uploader to match the process manager signature."""
  return use_sunnylink_uploader(params)

def is_snpe_model(started, params, CP: car.CarParams) -> bool:
  """Check if the active model runner is SNPE."""
  return bool(get_active_model_runner(params, not started) == custom.ModelManagerSP.Runner.snpe)

def is_tinygrad_model(started, params, CP: car.CarParams) -> bool:
  """Check if the active model runner is SNPE."""
  return bool(get_active_model_runner(params, not started) == custom.ModelManagerSP.Runner.tinygrad)

def is_stock_model(started, params, CP: car.CarParams) -> bool:
  """Check if the active model runner is stock."""
  return bool(get_active_model_runner(params, not started) == custom.ModelManagerSP.Runner.stock)

def mapd_ready(started: bool, params: Params, CP: car.CarParams) -> bool:
  return bool(os.path.exists(Paths.mapd_root()))

def uploader_ready(started: bool, params: Params, CP: car.CarParams) -> bool:
  if not params.get_bool("OnroadUploads"):
    return only_offroad(started, params, CP)

  return always_run(started, params, CP)

def or_(*fns):
  return lambda *args: operator.or_(*(fn(*args) for fn in fns))

def and_(*fns):
  return lambda *args: operator.and_(*(fn(*args) for fn in fns))

procs = [
  DaemonProcess("manage_athenad", "system.athena.manage_athenad", "AthenadPid"),

  #NativeProcess("loggerd", "system/loggerd", ["./loggerd"], logging, enabled=not PC),
  NativeProcess("encoderd", "system/loggerd", ["./encoderd"], only_onroad, enabled=not PC),
  NativeProcess("stream_encoderd", "system/loggerd", ["./encoderd", "--stream"], notcar, enabled=not PC),
  PythonProcess("logmessaged", "system.logmessaged", always_run, enabled=not PC),

  NativeProcess("usbcamerad", "tools/webcam", ["./camerad"], driverview, enabled=USBCAM),
  #NativeProcess("camerad", "system/camerad", ["./camerad"], driverview, enabled=not WEBCAM),
  PythonProcess("webcamerad", "tools.webcam.camerad", driverview, enabled=WEBCAM),
  PythonProcess("proclogd", "system.proclogd", only_onroad, enabled=not PC),
  PythonProcess("journald", "system.journald", only_onroad, enabled=not PC),
  PythonProcess("micd", "system.micd", iscar, enabled=not PC),
  PythonProcess("timed", "system.timed", always_run, enabled=not PC),

  PythonProcess("modeld", "selfdrive.modeld.modeld", and_(only_onroad, is_stock_model)),
  PythonProcess("dmonitoringmodeld", "selfdrive.modeld.dmonitoringmodeld", driverview, enabled=(not NO_DM)),  # 驾驶员监控摄像头

  # PythonProcess("sensord", "system.sensord.ybimu_sensord" if usb_imu_available() else "system.sensord.sensord", always_run, enabled=usb_imu_available() or not PC),  # 自动检测 USB IMU
  NativeProcess("sensord_jy62", "system/sensord", ["./sensord_jy62"], always_run, enabled=PC),
  PythonProcess("soundd", "selfdrive.ui.soundd", only_onroad),
  NativeProcess("ui", "selfdrive/ui", ["./ui"], always_run, watchdog_max_dt=(5 if not PC else None)),
  PythonProcess("raylib_ui", "selfdrive.ui.ui", always_run, enabled=False, watchdog_max_dt=(5 if not PC else None)),
  PythonProcess("soundd", "selfdrive.ui.soundd", only_onroad),
  PythonProcess("locationd", "selfdrive.locationd.locationd", always_run),
  NativeProcess("_pandad", "selfdrive/pandad", ["./pandad"], always_run, enabled=False),
  PythonProcess("calibrationd", "selfdrive.locationd.calibrationd", only_onroad),
  PythonProcess("torqued", "selfdrive.locationd.torqued", only_onroad),
  PythonProcess("controlsd", "selfdrive.controls.controlsd", and_(not_joystick, iscar)),
  PythonProcess("joystickd", "tools.joystick.joystickd", or_(joystick, notcar)),
  PythonProcess("selfdrived", "selfdrive.selfdrived.selfdrived", only_onroad),
  PythonProcess("card", "selfdrive.car.card", only_onroad),
  PythonProcess("deleter", "system.loggerd.deleter", always_run, enabled=not PC),
  PythonProcess("dmonitoringd", "selfdrive.monitoring.dmonitoringd", driverview, enabled=(not NO_DM)),  # 驾驶员监控
  PythonProcess("qcomgpsd", "system.qcomgpsd.qcomgpsd", qcomgps, enabled=TICI),
  PythonProcess("pandad", "selfdrive.pandad.pandad", always_run),
  PythonProcess("paramsd", "selfdrive.locationd.paramsd", only_onroad),
  PythonProcess("lagd", "selfdrive.locationd.lagd", only_onroad),
  PythonProcess("ubloxd", "system.ubloxd.ubloxd", ublox, enabled=TICI),
  PythonProcess("pigeond", "system.ubloxd.pigeond", ublox, enabled=TICI),
  PythonProcess("plannerd", "selfdrive.controls.plannerd", not_long_maneuver),
  PythonProcess("maneuversd", "tools.longitudinal_maneuvers.maneuversd", long_maneuver),
  PythonProcess("radard", "selfdrive.controls.radard", only_onroad),
  PythonProcess("hardwared", "system.hardware.hardwared", always_run),
  PythonProcess("tombstoned", "system.tombstoned", always_run, enabled=not PC),
  PythonProcess("updated", "system.updated.updated", only_offroad, enabled=not PC),
  PythonProcess("uploader", "system.loggerd.uploader", uploader_ready, enabled=not PC),
  PythonProcess("statsd", "system.statsd", always_run, enabled=not PC),
  PythonProcess("feedbackd", "selfdrive.ui.feedback.feedbackd", only_onroad, enabled=not PC),

  # debug procs
  NativeProcess("bridge", "cereal/messaging", ["./bridge"], notcar, enabled=not PC),
  PythonProcess("webrtcd", "system.webrtc.webrtcd", notcar, enabled=not PC),
  PythonProcess("webjoystick", "tools.bodyteleop.web", notcar, enabled=not PC),
  PythonProcess("joystick", "tools.joystick.joystick_control", and_(joystick, iscar)),

  # sunnylink <3 (disabled on PC)
  DaemonProcess("manage_sunnylinkd", "sunnypilot.sunnylink.athena.manage_sunnylinkd", "SunnylinkdPid", enabled=not PC),
  PythonProcess("sunnylink_registration_manager", "sunnypilot.sunnylink.registration_manager", sunnylink_need_register_shim, enabled=not PC),
  PythonProcess("statsd_sp", "sunnypilot.sunnylink.statsd", and_(always_run, sunnylink_ready_shim)),
]

# sunnypilot
procs += [
  # Models
  PythonProcess("models_manager", "sunnypilot.models.manager", only_offroad, enabled=not PC),
  NativeProcess("modeld_snpe", "sunnypilot/modeld", ["./modeld"], and_(only_onroad, is_snpe_model)),
  NativeProcess("modeld_tinygrad", "sunnypilot/modeld_v2", ["./modeld"], and_(only_onroad, is_tinygrad_model)),

  # Backup
  PythonProcess("backup_manager", "sunnypilot.sunnylink.backups.manager", and_(only_offroad, sunnylink_ready_shim), enabled=not PC),

  # mapd
  NativeProcess("mapd", Paths.mapd_root(), ["bash", "-c", f"{MAPD_PATH} > /dev/null 2>&1"], mapd_ready, enabled=not PC),
  PythonProcess("mapd_manager", "sunnypilot.mapd.mapd_manager", always_run, enabled=not PC),

  # locationd
  NativeProcess("locationd_llk", "sunnypilot/selfdrive/locationd", ["./locationd"], only_onroad, enabled=not PC),

  # carrot (phone projection & navigation)
  PythonProcess("carrot_man", "selfdrive.carrot.carrot_man", only_onroad),
]

if os.path.exists("./github_runner.sh"):
  procs += [NativeProcess("github_runner_start", "system/manager", ["./github_runner.sh", "start"], and_(only_offroad, use_github_runner), enabled=not PC, sigkill=False)]

if os.path.exists("../../sunnypilot/sunnylink/uploader.py"):
  procs += [PythonProcess("sunnylink_uploader", "sunnypilot.sunnylink.uploader", use_sunnylink_uploader_shim, enabled=not PC)]

if os.path.exists("../../third_party/copyparty/copyparty-sfx.py"):
  sunnypilot_root = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
  copyparty_args = [f"-v{Paths.crash_log_root()}:/swaglogs:r"]
  copyparty_args += [f"-v{Paths.log_root()}:/routes:r"]
  copyparty_args += [f"-v{Paths.model_root()}:/models:rw"]
  copyparty_args += [f"-v{sunnypilot_root}:/sunnypilot:rw"]
  copyparty_args += ["-p8080"]
  copyparty_args += ["-z"]
  copyparty_args += ["-q"]
  procs += [NativeProcess("copyparty-sfx", "third_party/copyparty", ["./copyparty-sfx.py", *copyparty_args], and_(only_offroad, use_copyparty))]

managed_processes = {p.name: p for p in procs}
