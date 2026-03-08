#pragma once

#include <string>
#include <unordered_map>

#include "cereal/gen/cpp/log.capnp.h"
#include "common/params.h"

// 定义参数类型
enum ParamValueType {
  STRING = 0,
  INT = 1,
  FLOAT = 2,
  BOOL = 3,
  BYTES = 4,
  JSON = 5
};

// 定义参数属性
struct ParamKeyAttributes {
  uint32_t type;
  ParamValueType value_type;
  std::string default_value;

  ParamKeyAttributes() = default;
  ParamKeyAttributes(uint32_t t, ParamValueType vt, const std::string& def = "")
    : type(t), value_type(vt), default_value(def) {}
};

// 定义参数键值映射
inline std::unordered_map<std::string, ParamKeyAttributes> get_keys() {
  std::unordered_map<std::string, ParamKeyAttributes> keys;

  keys["AccessToken"] = {CLEAR_ON_MANAGER_START | DONT_LOG, STRING};
  keys["AdbEnabled"] = {PERSISTENT | BACKUP, BOOL};
  keys["AlwaysOnDM"] = {PERSISTENT | BACKUP, BOOL};
  keys["ApiCache_Device"] = {PERSISTENT, STRING};
  keys["ApiCache_FirehoseStats"] = {PERSISTENT, JSON};
  keys["AssistNowToken"] = {PERSISTENT, STRING};
  keys["AthenadPid"] = {PERSISTENT, INT};
  keys["AthenadUploadQueue"] = {PERSISTENT, JSON};
  keys["AthenadRecentlyViewedRoutes"] = {PERSISTENT, STRING};
  keys["BootCount"] = {PERSISTENT, INT};
  keys["CalibrationParams"] = {PERSISTENT, BYTES};
  keys["CameraDebugExpGain"] = {CLEAR_ON_MANAGER_START, STRING};
  keys["CameraDebugExpTime"] = {CLEAR_ON_MANAGER_START, STRING};
  keys["CarBatteryCapacity"] = {PERSISTENT, INT};
  keys["CarParams"] = {CLEAR_ON_MANAGER_START | CLEAR_ON_ONROAD_TRANSITION, BYTES};
  keys["CarParamsCache"] = {CLEAR_ON_MANAGER_START, BYTES};
  keys["CarParamsPersistent"] = {PERSISTENT, BYTES};
  keys["CarParamsPrevRoute"] = {PERSISTENT, BYTES};
  keys["LongPitch"] = {PERSISTENT, BOOL, "1"};
  keys["EVTable"] = {PERSISTENT, BOOL, "1"};
  keys["CompletedTrainingVersion"] = {PERSISTENT, STRING, "0.2.0"};
  keys["ControlsReady"] = {CLEAR_ON_MANAGER_START | CLEAR_ON_ONROAD_TRANSITION, BOOL};
  keys["CurrentBootlog"] = {PERSISTENT, STRING};
  keys["CurrentRoute"] = {CLEAR_ON_MANAGER_START | CLEAR_ON_ONROAD_TRANSITION, STRING};
  keys["DisableLogging"] = {CLEAR_ON_MANAGER_START | CLEAR_ON_ONROAD_TRANSITION, BOOL};
  keys["DisablePowerDown"] = {PERSISTENT | BACKUP, BOOL};
  keys["DisableUpdates"] = {PERSISTENT | BACKUP, BOOL, "0"};
  keys["DisengageOnAccelerator"] = {PERSISTENT | BACKUP, BOOL, "0"};
  keys["DongleId"] = {PERSISTENT, STRING};
  keys["DoReboot"] = {CLEAR_ON_MANAGER_START, BOOL};
  keys["DoShutdown"] = {CLEAR_ON_MANAGER_START, BOOL};
  keys["DoUninstall"] = {CLEAR_ON_MANAGER_START, BOOL};
  keys["DriverTooDistracted"] = {CLEAR_ON_MANAGER_START | CLEAR_ON_IGNITION_ON, BOOL};
  keys["AlphaLongitudinalEnabled"] = {PERSISTENT | DEVELOPMENT_ONLY | BACKUP, BOOL};
  keys["ExperimentalMode"] = {PERSISTENT | BACKUP, BOOL};
  keys["ExperimentalModeConfirmed"] = {PERSISTENT | BACKUP, BOOL};
  keys["FirmwareQueryDone"] = {CLEAR_ON_MANAGER_START | CLEAR_ON_ONROAD_TRANSITION, BOOL};
  keys["ForcePowerDown"] = {PERSISTENT, BOOL};
  keys["GitBranch"] = {PERSISTENT, STRING};
  keys["GitCommit"] = {PERSISTENT, STRING};
  keys["GitCommitDate"] = {PERSISTENT, STRING};
  keys["GitDiff"] = {PERSISTENT, STRING};
  keys["GithubSshKeys"] = {PERSISTENT | BACKUP, STRING};
  keys["GithubUsername"] = {PERSISTENT | BACKUP, STRING};
  keys["GitRemote"] = {PERSISTENT, STRING};
  keys["GsmApn"] = {PERSISTENT | BACKUP, STRING};
  keys["GsmMetered"] = {PERSISTENT | BACKUP, BOOL, "1"};
  keys["GsmRoaming"] = {PERSISTENT | BACKUP, BOOL};
  keys["HardwareSerial"] = {PERSISTENT, STRING};
  keys["HasAcceptedTerms"] = {PERSISTENT, STRING, "0"};
  keys["InstallDate"] = {PERSISTENT | TIME, STRING};
  keys["IsDriverViewEnabled"] = {CLEAR_ON_MANAGER_START, BOOL};
  keys["IsEngaged"] = {PERSISTENT, BOOL};
  keys["IsLdwEnabled"] = {PERSISTENT | BACKUP, BOOL};
  keys["IsMetric"] = {PERSISTENT | BACKUP, BOOL};
  keys["IsOffroad"] = {CLEAR_ON_MANAGER_START, BOOL};
  keys["IsOnroad"] = {PERSISTENT, BOOL};
  keys["IsRhdDetected"] = {PERSISTENT, BOOL};
  keys["IsReleaseBranch"] = {CLEAR_ON_MANAGER_START, BOOL};
  keys["IsTakingSnapshot"] = {CLEAR_ON_MANAGER_START, BOOL};
  keys["IsTestedBranch"] = {CLEAR_ON_MANAGER_START, BOOL};
  keys["JoystickDebugMode"] = {CLEAR_ON_MANAGER_START | CLEAR_ON_OFFROAD_TRANSITION, BOOL};
  keys["LanguageSetting"] = {PERSISTENT | BACKUP, STRING, "main_en"};
  keys["LastAthenaPingTime"] = {CLEAR_ON_MANAGER_START, INT};
  keys["LastGPSPosition"] = {PERSISTENT, STRING};
  keys["LastManagerExitReason"] = {CLEAR_ON_MANAGER_START, STRING};
  keys["LastOffroadStatusPacket"] = {CLEAR_ON_MANAGER_START | CLEAR_ON_OFFROAD_TRANSITION, JSON};
  keys["LastPowerDropDetected"] = {CLEAR_ON_MANAGER_START, STRING};
  keys["LastUpdateException"] = {CLEAR_ON_MANAGER_START, STRING};
  keys["LastUpdateRouteCount"] = {PERSISTENT, INT, "0"};
  keys["LastUpdateTime"] = {PERSISTENT | TIME, STRING};
  keys["LastUpdateUptimeOnroad"] = {PERSISTENT, FLOAT, "0.0"};
  keys["LiveDelay"] = {PERSISTENT | BACKUP, BYTES};
  keys["LiveParameters"] = {PERSISTENT, JSON};
  keys["LiveParametersV2"] = {PERSISTENT, BYTES};
  keys["LiveTorqueParameters"] = {PERSISTENT | DONT_LOG, BYTES};
  keys["LocationFilterInitialState"] = {PERSISTENT, BYTES};
  keys["LongitudinalManeuverMode"] = {CLEAR_ON_MANAGER_START | CLEAR_ON_OFFROAD_TRANSITION, BOOL};
  keys["LongitudinalPersonality"] = {PERSISTENT | BACKUP, INT, "1"};
  keys["LongitudinalPersonalityMax"] = {PERSISTENT | BACKUP, INT, "3"};
  keys["ShowTpms"] = {PERSISTENT | BACKUP, BOOL, "1"};
  keys["ShowDateTime"] = {PERSISTENT | BACKUP, BOOL, "1"};
  keys["ShowPathEnd"] = {PERSISTENT | BACKUP, BOOL, "1"};
  keys["ShowCustomBrightness"] = {PERSISTENT | BACKUP, INT, "100"};
  keys["ShowLaneInfo"] = {PERSISTENT | BACKUP, BOOL, "1"};
  keys["ShowRadarInfo"] = {PERSISTENT | BACKUP, BOOL, "0"};
  keys["ShowDeviceState"] = {PERSISTENT | BACKUP, BOOL, "1"};
  keys["ShowRouteInfo"] = {PERSISTENT | BACKUP, BOOL, "1"};
  keys["ShowDebugLog"] = {PERSISTENT | BACKUP, BOOL, "0"};
  keys["ShowDebugUI"] = {PERSISTENT | BACKUP, BOOL, "0"};
  keys["ShowPathMode"] = {PERSISTENT | BACKUP, INT, "0"};
  keys["ShowPathColor"] = {PERSISTENT | BACKUP, INT, "12"};
  keys["ShowPathColorCruiseOff"] = {PERSISTENT | BACKUP, INT, "19"};
  keys["ShowPathModeLane"] = {PERSISTENT | BACKUP, INT, "0"};
  keys["ShowPathColorLane"] = {PERSISTENT | BACKUP, INT, "13"};
  keys["ShowPlotMode"] = {PERSISTENT | BACKUP, INT, "0"};
  keys["AutoCruiseControl"] = {PERSISTENT | BACKUP, BOOL, "0"};
  keys["CruiseEcoControl"] = {PERSISTENT | BACKUP, BOOL, "0"};
  keys["CarrotCruiseDecel"] = {PERSISTENT | BACKUP, INT, "-1"};
  keys["CarrotCruiseAtcDecel"] = {PERSISTENT | BACKUP, INT, "-1"};
  keys["CommaLongAcc"] = {PERSISTENT | BACKUP, BOOL, "0"};
  keys["AutoGasTokSpeed"] = {PERSISTENT | BACKUP, INT, "30"};
  keys["AutoGasSyncSpeed"] = {PERSISTENT | BACKUP, INT, "1"};
  keys["AutoEngage"] = {PERSISTENT | BACKUP, BOOL, "0"};
  keys["DisableMinSteerSpeed"] = {PERSISTENT | BACKUP, BOOL, "0"};
  keys["SoftHoldMode"] = {PERSISTENT | BACKUP, BOOL, "0"};
  keys["AutoSpeedUptoRoadSpeedLimit"] = {PERSISTENT | BACKUP, BOOL, "0"};
  keys["AutoRoadSpeedAdjust"] = {PERSISTENT | BACKUP, INT, "-1"};
  keys["AutoCurveSpeedLowerLimit"] = {PERSISTENT | BACKUP, INT, "30"};
  keys["AutoCurveSpeedFactor"] = {PERSISTENT | BACKUP, INT, "100"};
  keys["AutoCurveSpeedAggressiveness"] = {PERSISTENT | BACKUP, INT, "100"};
  keys["AutoTurnControl"] = {PERSISTENT | BACKUP, INT, "2"};
  keys["AutoTurnControlSpeedTurn"] = {PERSISTENT | BACKUP, INT, "20"};
  keys["AutoTurnControlTurnEnd"] = {PERSISTENT | BACKUP, INT, "6"};
  keys["AutoTurnMapChange"] = {PERSISTENT | BACKUP, BOOL, "0"};
  keys["AutoNaviSpeedCtrlEnd"] = {PERSISTENT | BACKUP, INT, "7"};
  keys["AutoNaviSpeedCtrlMode"] = {PERSISTENT | BACKUP, INT, "3"};
  keys["AutoNaviSpeedBumpTime"] = {PERSISTENT | BACKUP, INT, "1"};
  keys["AutoNaviSpeedBumpSpeed"] = {PERSISTENT | BACKUP, INT, "35"};
  keys["AutoNaviSpeedSafetyFactor"] = {PERSISTENT | BACKUP, INT, "100"};
  keys["AutoNaviSpeedDecelRate"] = {PERSISTENT | BACKUP, INT, "80"};
  keys["AutoRoadSpeedLimitOffset"] = {PERSISTENT | BACKUP, INT, "0"};
  keys["AutoNaviCountDownMode"] = {PERSISTENT | BACKUP, BOOL, "0"};
  keys["TurnSpeedControlMode"] = {PERSISTENT | BACKUP, INT, "2"};
  keys["MapTurnSpeedFactor"] = {PERSISTENT | BACKUP, INT, "100"};
  keys["StoppingAccel"] = {PERSISTENT | BACKUP, INT, "0"};
  keys["StopDistanceCarrot"] = {PERSISTENT | BACKUP, INT, "550"};
  keys["JLeadFactor3"] = {PERSISTENT | BACKUP, INT, "0"};
  keys["CruiseButtonMode"] = {PERSISTENT | BACKUP, INT, "0"};
  keys["CancelButtonMode"] = {PERSISTENT | BACKUP, INT, "0"};
  keys["LfaButtonMode"] = {PERSISTENT | BACKUP, INT, "0"};
  keys["CruiseButtonTest1"] = {PERSISTENT | BACKUP, INT, "8"};
  keys["CruiseButtonTest2"] = {PERSISTENT | BACKUP, INT, "30"};
  keys["CruiseButtonTest3"] = {PERSISTENT | BACKUP, INT, "1"};
  keys["CruiseSpeedUnit"] = {PERSISTENT | BACKUP, INT, "10"};
  keys["CruiseSpeedUnitBasic"] = {PERSISTENT | BACKUP, INT, "5"};
  keys["CruiseSpeed1"] = {PERSISTENT | BACKUP, INT, "30"};
  keys["CruiseSpeed2"] = {PERSISTENT | BACKUP, INT, "50"};
  keys["CruiseSpeed3"] = {PERSISTENT | BACKUP, INT, "80"};
  keys["CruiseSpeed4"] = {PERSISTENT | BACKUP, INT, "110"};
  keys["CruiseSpeed5"] = {PERSISTENT | BACKUP, INT, "130"};
  keys["PaddleMode"] = {PERSISTENT | BACKUP, INT, "0"};
  keys["MyDrivingMode"] = {PERSISTENT | BACKUP, INT, "1"};
  keys["MyDrivingModeAuto"] = {PERSISTENT | BACKUP, BOOL, "0"};
  keys["TrafficLightDetectMode"] = {PERSISTENT | BACKUP, INT, "2"};
  keys["CruiseMaxVals0"] = {PERSISTENT | BACKUP, INT, "80"};
  keys["CruiseMaxVals1"] = {PERSISTENT | BACKUP, INT, "100"};
  keys["CruiseMaxVals2"] = {PERSISTENT | BACKUP, INT, "120"};
  keys["CruiseMaxVals3"] = {PERSISTENT | BACKUP, INT, "100"};
  keys["CruiseMaxVals4"] = {PERSISTENT | BACKUP, INT, "70"};
  keys["CruiseMaxVals5"] = {PERSISTENT | BACKUP, INT, "50"};
  keys["CruiseMaxVals6"] = {PERSISTENT | BACKUP, INT, "40"};
  keys["LongTuningKpV"] = {PERSISTENT | BACKUP, INT, "100"};
  keys["LongTuningKiV"] = {PERSISTENT | BACKUP, INT, "0"};
  keys["LongTuningKf"] = {PERSISTENT | BACKUP, INT, "100"};
  keys["LongActuatorDelay"] = {PERSISTENT | BACKUP, INT, "20"};
  keys["VEgoStopping"] = {PERSISTENT | BACKUP, INT, "50"};
  keys["RadarReactionFactor"] = {PERSISTENT | BACKUP, INT, "100"};
  keys["EnableRadarTracks"] = {PERSISTENT | BACKUP, BOOL, "1"};
  keys["EnableEscc"] = {PERSISTENT | BACKUP, BOOL, "1"};
  keys["RadarLatFactor"] = {PERSISTENT | BACKUP, INT, "0"};
  keys["EnableCornerRadar"] = {PERSISTENT | BACKUP, BOOL, "0"};
  keys["HyundaiCameraSCC"] = {PERSISTENT | BACKUP, BOOL, "0"};
  keys["IsLdwsCar"] = {PERSISTENT | BACKUP, BOOL, "0"};
  keys["CanfdHDA2"] = {PERSISTENT | BACKUP, BOOL, "0"};
  keys["CanfdDebug"] = {PERSISTENT | BACKUP, BOOL, "0"};
  keys["SoundVolumeAdjust"] = {PERSISTENT | BACKUP, INT, "100"};
  keys["SoundVolumeAdjustEngage"] = {PERSISTENT | BACKUP, INT, "100"};
  keys["TFollowGap1"] = {PERSISTENT | BACKUP, INT, "110"};
  keys["TFollowGap2"] = {PERSISTENT | BACKUP, INT, "140"};
  keys["TFollowGap3"] = {PERSISTENT | BACKUP, INT, "160"};
  keys["TFollowGap4"] = {PERSISTENT | BACKUP, INT, "200"};
  keys["DynamicTFollow"] = {PERSISTENT | BACKUP, BOOL, "0"};
  keys["DynamicTFollowLC"] = {PERSISTENT | BACKUP, INT, "100"};
  keys["HapticFeedbackWhenSpeedCamera"] = {PERSISTENT | BACKUP, BOOL, "0"};
  keys["UseLaneLineSpeed"] = {PERSISTENT | BACKUP, BOOL, "0"};
  keys["PathOffset"] = {PERSISTENT | BACKUP, INT, "0"};
  keys["UseLaneLineCurveSpeed"] = {PERSISTENT | BACKUP, BOOL, "0"};
  keys["AdjustLaneOffset"] = {PERSISTENT | BACKUP, BOOL, "0"};
  keys["AdjustCurveOffset"] = {PERSISTENT | BACKUP, BOOL, "0"};
  keys["LaneChangeNeedTorque"] = {PERSISTENT | BACKUP, BOOL, "0"};
  keys["AutoLaneChangeMinSpeed"] = {PERSISTENT | BACKUP, INT, "0"};
  keys["LaneChangeDelay"] = {PERSISTENT | BACKUP, INT, "0"};
  keys["LaneChangeBsd"] = {PERSISTENT | BACKUP, BOOL, "0"};
  keys["MaxAngleFrames"] = {PERSISTENT | BACKUP, INT, "85"};
  keys["LateralTorqueCustom"] = {PERSISTENT | BACKUP, BOOL, "1"};
  keys["LateralTorqueAccelFactor"] = {PERSISTENT | BACKUP, INT, "3000"};
  keys["LateralTorqueFriction"] = {PERSISTENT | BACKUP, INT, "100"};
  keys["LateralTorqueKpV"] = {PERSISTENT | BACKUP, INT, "100"};
  keys["LateralTorqueKiV"] = {PERSISTENT | BACKUP, INT, "10"};
  keys["LateralTorqueKf"] = {PERSISTENT | BACKUP, INT, "100"};
  keys["LateralTorqueKd"] = {PERSISTENT | BACKUP, INT, "0"};
  keys["LatMpcPathCost"] = {PERSISTENT | BACKUP, INT, "200"};
  keys["LatMpcMotionCost"] = {PERSISTENT | BACKUP, INT, "7"};
  keys["LatMpcAccelCost"] = {PERSISTENT | BACKUP, INT, "120"};
  keys["LatMpcJerkCost"] = {PERSISTENT | BACKUP, INT, "4"};
  keys["LatMpcSteeringRateCost"] = {PERSISTENT | BACKUP, INT, "7"};
  keys["LatMpcInputOffset"] = {PERSISTENT | BACKUP, INT, "4"};
  keys["CustomSteerMax"] = {PERSISTENT | BACKUP, INT, "0"};
  keys["CustomSteerDeltaUp"] = {PERSISTENT | BACKUP, INT, "0"};
  keys["CustomSteerDeltaDown"] = {PERSISTENT | BACKUP, INT, "0"};
  keys["CustomSteerDeltaUpLC"] = {PERSISTENT | BACKUP, INT, "0"};
  keys["CustomSteerDeltaDownLC"] = {PERSISTENT | BACKUP, INT, "0"};
  keys["SpeedFromPCM"] = {PERSISTENT | BACKUP, INT, "2"};
  keys["SteerActuatorDelay"] = {PERSISTENT | BACKUP, INT, "0"};  // 强制使用SP的延迟学习功能
  keys["MaxTimeOffroadMin"] = {PERSISTENT | BACKUP, INT, "60"};
  keys["DisableDM"] = {PERSISTENT | BACKUP, BOOL, "1"};
  keys["EnableConnect"] = {PERSISTENT | BACKUP, BOOL, "0"};
  keys["MuteDoor"] = {PERSISTENT | BACKUP, BOOL, "0"};
  keys["MuteSeatbelt"] = {PERSISTENT | BACKUP, BOOL, "0"};
  keys["RecordRoadCam"] = {PERSISTENT | BACKUP, BOOL, "0"};
  keys["HDPuse"] = {PERSISTENT | BACKUP, BOOL, "0"};
  keys["CruiseOnDist"] = {PERSISTENT | BACKUP, INT, "400"};
  keys["HotspotOnBoot"] = {PERSISTENT | BACKUP, BOOL, "0"};
  keys["SoftwareMenu"] = {PERSISTENT | BACKUP, BOOL, "1"};
  keys["CustomSteerOffset"] = {PERSISTENT | BACKUP, INT, "0"};
  keys["CameraOffset"] = {PERSISTENT | BACKUP, INT, "0"};  // 摄像头Y轴偏移，单位0.01米
  keys["SteerAngleOffset"] = {PERSISTENT | BACKUP, INT, "0"};
  keys["CustomSR"] = {PERSISTENT | BACKUP, INT, "145"};
  keys["SteerRatioRate"] = {PERSISTENT | BACKUP, INT, "100"};
  keys["NNFF"] = {PERSISTENT | BACKUP, BOOL, "0"};
  keys["NNFFLite"] = {PERSISTENT | BACKUP, BOOL, "0"};
  keys["SameSpiCamFilter"] = {PERSISTENT | BACKUP, BOOL, "1"};
  keys["StockBlinkerCtrl"] = {PERSISTENT | BACKUP, BOOL, "0"};
  keys["ExtBlinkerCtrlTest"] = {PERSISTENT | BACKUP, BOOL, "0"};
  keys["BlinkerMode"] = {PERSISTENT | BACKUP, INT, "1"};
  keys["LaneStabTime"] = {PERSISTENT | BACKUP, INT, "30"};
  keys["BsdDelayTime"] = {PERSISTENT | BACKUP, INT, "10"};
  keys["SideBsdDelayTime"] = {PERSISTENT | BACKUP, INT, "10"};
  keys["SideRelDistTime"] = {PERSISTENT | BACKUP, INT, "10"};
  keys["SidevRelDistTime"] = {PERSISTENT | BACKUP, INT, "10"};
  keys["SideRadarMinDist"] = {PERSISTENT | BACKUP, INT, "0"};
  keys["AutoTurnDistOffset"] = {PERSISTENT | BACKUP, INT, "0"};
  keys["AutoTurnInNotRoadEdge"] = {PERSISTENT | BACKUP, BOOL, "1"};
  keys["ContinuousLaneChange"] = {PERSISTENT | BACKUP, BOOL, "1"};
  keys["ContinuousLaneChangeCnt"] = {PERSISTENT | BACKUP, INT, "4"};
  keys["ContinuousLaneChangeInterval"] = {PERSISTENT | BACKUP, INT, "2"};
  keys["AutoTurnLeft"] = {PERSISTENT | BACKUP, BOOL, "1"};
  keys["AutoUpRoadLimit"] = {PERSISTENT | BACKUP, BOOL, "0"};
  keys["AutoUpHighwayRoadLimit"] = {PERSISTENT | BACKUP, BOOL, "0"};
  keys["AutoForkDistOffsetH"] = {PERSISTENT | BACKUP, INT, "1500"};
  keys["AutoEnTurnNewLaneTimeH"] = {PERSISTENT | BACKUP, INT, "3"};
  keys["AutoDoForkDecalDistH"] = {PERSISTENT | BACKUP, INT, "100"};
  keys["AutoDoForkDecalDist"] = {PERSISTENT | BACKUP, INT, "20"};
  keys["AutoDoForkCheckDistH"] = {PERSISTENT | BACKUP, INT, "20"};
  keys["AutoDoForkBlinkerDistH"] = {PERSISTENT | BACKUP, INT, "30"};
  keys["AutoDoForkNavDistH"] = {PERSISTENT | BACKUP, INT, "80"};
  keys["AutoForkDecalRateH"] = {PERSISTENT | BACKUP, INT, "75"};
  keys["AutoForkSpeedMinH"] = {PERSISTENT | BACKUP, INT, "60"};
  keys["AutoKeepForkSpeedH"] = {PERSISTENT | BACKUP, INT, "5"};
  keys["AutoForkDecalRate"] = {PERSISTENT | BACKUP, INT, "80"};
  keys["AutoForkSpeedMin"] = {PERSISTENT | BACKUP, INT, "45"};
  keys["AutoKeepForkSpeed"] = {PERSISTENT | BACKUP, INT, "5"};
  keys["AutoForkDistOffset"] = {PERSISTENT | BACKUP, INT, "30"};
  keys["AutoEnTurnNewLaneTime"] = {PERSISTENT | BACKUP, INT, "1"};
  keys["AutoDoForkCheckDist"] = {PERSISTENT | BACKUP, INT, "10"};
  keys["AutoDoForkBlinkerDist"] = {PERSISTENT | BACKUP, INT, "15"};
  keys["AutoDoForkNavDist"] = {PERSISTENT | BACKUP, INT, "15"};
  keys["AutoUpRoadLimit40KMH"] = {PERSISTENT | BACKUP, BOOL, "0"};
  keys["AutoUpHighwayRoadLimit40KMH"] = {PERSISTENT | BACKUP, BOOL, "0"};
  keys["RoadType"] = {PERSISTENT | BACKUP, INT, "-2"};
  keys["AutoCurveSpeedFactorH"] = {PERSISTENT | BACKUP, INT, "90"};
  keys["AutoCurveSpeedAggressivenessH"] = {PERSISTENT | BACKUP, INT, "110"};
  keys["NewLaneWidthDiff"] = {PERSISTENT | BACKUP, INT, "8"};
  keys["ComputerType"] = {PERSISTENT | BACKUP, INT, "1"};
  keys["NetworkMetered"] = {PERSISTENT | BACKUP, BOOL};
  keys["NetworkAddress"] = {PERSISTENT, STRING};
  keys["SoftRestartTriggered"] = {CLEAR_ON_MANAGER_START, INT, "0"};
  keys["device_go_off_road"] = {PERSISTENT, BOOL, "0"};
  keys["ObdMultiplexingChanged"] = {CLEAR_ON_MANAGER_START | CLEAR_ON_ONROAD_TRANSITION, BOOL};
  keys["ObdMultiplexingEnabled"] = {CLEAR_ON_MANAGER_START | CLEAR_ON_ONROAD_TRANSITION, BOOL};
  keys["Offroad_CarUnrecognized"] = {CLEAR_ON_MANAGER_START | CLEAR_ON_ONROAD_TRANSITION, JSON};
  keys["Offroad_ConnectivityNeeded"] = {CLEAR_ON_MANAGER_START, JSON};
  keys["Offroad_ConnectivityNeededPrompt"] = {CLEAR_ON_MANAGER_START, JSON};
  keys["Offroad_ExcessiveActuation"] = {PERSISTENT, JSON};
  keys["Offroad_IsTakingSnapshot"] = {CLEAR_ON_MANAGER_START, JSON};
  keys["Offroad_NeosUpdate"] = {CLEAR_ON_MANAGER_START, JSON};
  keys["Offroad_NoFirmware"] = {CLEAR_ON_MANAGER_START | CLEAR_ON_ONROAD_TRANSITION, JSON};
  keys["Offroad_Recalibration"] = {CLEAR_ON_MANAGER_START | CLEAR_ON_ONROAD_TRANSITION, JSON};
  keys["Offroad_StorageMissing"] = {CLEAR_ON_MANAGER_START, JSON};
  keys["Offroad_TemperatureTooHigh"] = {CLEAR_ON_MANAGER_START, JSON};
  keys["Offroad_UnregisteredHardware"] = {CLEAR_ON_MANAGER_START, JSON};
  keys["Offroad_UpdateFailed"] = {CLEAR_ON_MANAGER_START, JSON};
  keys["OnroadCycleRequested"] = {CLEAR_ON_MANAGER_START, BOOL};
  keys["OpenpilotEnabledToggle"] = {PERSISTENT | BACKUP, BOOL, "1"};
  keys["SearchInput"] = {PERSISTENT, STRING, "0"};
  keys["GMapKey"] = {PERSISTENT, STRING, "0"};
  keys["MapboxStyle"] = {PERSISTENT, STRING, "0"};
  keys["PandaHeartbeatLost"] = {CLEAR_ON_MANAGER_START | CLEAR_ON_OFFROAD_TRANSITION, BOOL};
  keys["PandaSomResetTriggered"] = {CLEAR_ON_MANAGER_START | CLEAR_ON_OFFROAD_TRANSITION, BOOL};
  keys["PandaSignatures"] = {CLEAR_ON_MANAGER_START, BYTES};
  keys["PrimeType"] = {PERSISTENT, INT};
  keys["RecordAudio"] = {PERSISTENT | BACKUP, BOOL};
  keys["RecordAudioFeedback"] = {PERSISTENT | BACKUP, BOOL, "0"};
  keys["RecordFront"] = {PERSISTENT | BACKUP, BOOL};
  keys["RecordFrontLock"] = {PERSISTENT, BOOL};  // for the internal fleet
  keys["SecOCKey"] = {PERSISTENT | DONT_LOG | BACKUP, STRING};
  keys["RouteCount"] = {PERSISTENT, INT, "0"};
  keys["SnoozeUpdate"] = {CLEAR_ON_MANAGER_START | CLEAR_ON_OFFROAD_TRANSITION, BOOL};
  keys["SshEnabled"] = {PERSISTENT | BACKUP, BOOL};
  keys["TermsVersion"] = {PERSISTENT, STRING};
  keys["TrainingVersion"] = {PERSISTENT, STRING};
  keys["UbloxAvailable"] = {PERSISTENT, BOOL};
  keys["UpdateAvailable"] = {CLEAR_ON_MANAGER_START | CLEAR_ON_ONROAD_TRANSITION, BOOL};
  keys["UpdateFailedCount"] = {CLEAR_ON_MANAGER_START, INT};
  keys["UpdaterAvailableBranches"] = {PERSISTENT, STRING};
  keys["UpdaterCurrentDescription"] = {CLEAR_ON_MANAGER_START, STRING};
  keys["UpdaterCurrentReleaseNotes"] = {CLEAR_ON_MANAGER_START, BYTES};
  keys["UpdaterFetchAvailable"] = {CLEAR_ON_MANAGER_START, BOOL};
  keys["UpdaterNewDescription"] = {CLEAR_ON_MANAGER_START, STRING};
  keys["UpdaterNewReleaseNotes"] = {CLEAR_ON_MANAGER_START, BYTES};
  keys["UpdaterState"] = {CLEAR_ON_MANAGER_START, STRING};
  keys["UpdaterTargetBranch"] = {CLEAR_ON_MANAGER_START, STRING};
  keys["UpdaterLastFetchTime"] = {PERSISTENT | TIME, STRING};
  keys["UptimeOffroad"] = {PERSISTENT, FLOAT, "0.0"};
  keys["UptimeOnroad"] = {PERSISTENT, FLOAT, "0.0"};
  keys["Version"] = {PERSISTENT, STRING};

  // --- sunnypilot params --- //
  keys["AccelPersonality"] = {PERSISTENT | BACKUP, INT, "1"};
  keys["ApiCache_DriveStats"] = {PERSISTENT, JSON};
  keys["AutoLaneChangeBsmDelay"] = {PERSISTENT | BACKUP, BOOL, "0"};
  keys["AutoLaneChangeTimer"] = {PERSISTENT | BACKUP, INT, "0"};
  keys["BlinkerMinLateralControlSpeed"] = {PERSISTENT | BACKUP, INT, "20"};  // MPH or km/h
  keys["BlinkerPauseLateralControl"] = {PERSISTENT | BACKUP, INT, "0"};
  keys["Brightness"] = {PERSISTENT | BACKUP, INT, "0"};
  keys["CarParamsSP"] = {CLEAR_ON_MANAGER_START | CLEAR_ON_ONROAD_TRANSITION, BYTES};
  keys["CarParamsSPCache"] = {CLEAR_ON_MANAGER_START, BYTES};
  keys["CarParamsSPPersistent"] = {PERSISTENT, BYTES};
  keys["CarPlatformBundle"] = {PERSISTENT | BACKUP, JSON};
  keys["ChevronInfo"] = {PERSISTENT | BACKUP, INT, "4"};
  keys["CustomAccIncrementsEnabled"] = {PERSISTENT | BACKUP, BOOL, "0"};
  keys["CustomAccLongPressIncrement"] = {PERSISTENT | BACKUP, INT, "5"};
  keys["CustomAccShortPressIncrement"] = {PERSISTENT | BACKUP, INT, "1"};
  keys["DeviceBootMode"] = {PERSISTENT | BACKUP, INT, "0"};
  keys["EnableGithubRunner"] = {PERSISTENT | BACKUP, BOOL};
  keys["GithubRunnerSufficientVoltage"] = {CLEAR_ON_MANAGER_START , BOOL};
  keys["InteractivityTimeout"] = {PERSISTENT | BACKUP, INT, "0"};
  keys["IsDevelopmentBranch"] = {CLEAR_ON_MANAGER_START, BOOL};
  keys["MaxTimeOffroad"] = {PERSISTENT | BACKUP, INT, "1800"};
  keys["ModelRunnerTypeCache"] = {CLEAR_ON_ONROAD_TRANSITION, INT};
  keys["OffroadMode"] = {CLEAR_ON_MANAGER_START, BOOL};
  keys["QuickBootToggle"] = {PERSISTENT | BACKUP, BOOL, "0"};
  keys["QuietMode"] = {PERSISTENT | BACKUP, BOOL, "0"};
  keys["RainbowMode"] = {PERSISTENT | BACKUP, BOOL, "0"};
  keys["ShowAdvancedControls"] = {PERSISTENT | BACKUP, BOOL, "0"};
  keys["VibePersonalityEnabled"] = {PERSISTENT | BACKUP, BOOL, "0"};
  keys["VibeAccelPersonalityEnabled"] = {PERSISTENT | BACKUP, BOOL, "0"};
  keys["VibeFollowPersonalityEnabled"] = {PERSISTENT | BACKUP, BOOL, "0"};
  keys["VisionTurnSpeedControl"] = {PERSISTENT | BACKUP, BOOL, "0"};

  // toyota specific params
  keys["ToyotaAutoHold"] = {PERSISTENT | BACKUP, BOOL, "0"};
  keys["ToyotaEnhancedBsm"] = {PERSISTENT | BACKUP, BOOL, "0"};
  keys["ToyotaTSS2Long"] = {PERSISTENT | BACKUP, BOOL, "0"};
  keys["ToyotaStockLongitudinal"] = {PERSISTENT | BACKUP, BOOL, "0"};
  keys["ToyotaDriveMode"] = {PERSISTENT | BACKUP, BOOL, "0"};
  keys["RoadEdgeLaneChangeEnabled"] = {PERSISTENT | BACKUP, BOOL, "0"};

  // MADS params
  keys["Mads"] = {PERSISTENT | BACKUP, BOOL, "1"};
  keys["MadsMainCruiseAllowed"] = {PERSISTENT | BACKUP, BOOL, "1"};
  keys["MadsSteeringMode"] = {PERSISTENT | BACKUP, INT, "0"};
  keys["MadsUnifiedEngagementMode"] = {PERSISTENT | BACKUP, BOOL, "1"};

  // Model Manager params
  keys["ModelManager_ActiveBundle"] = {PERSISTENT, JSON};
  keys["ModelManager_ClearCache"] = {CLEAR_ON_MANAGER_START, BOOL};
  keys["ModelManager_DownloadIndex"] = {CLEAR_ON_MANAGER_START | CLEAR_ON_ONROAD_TRANSITION, INT, "0"};
  keys["ModelManager_Favs"] = {PERSISTENT | BACKUP, STRING, ""};
  keys["ModelManager_LastSyncTime"] = {CLEAR_ON_MANAGER_START | CLEAR_ON_OFFROAD_TRANSITION, INT, "0"};
  keys["ModelManager_ModelsCache"] = {PERSISTENT | BACKUP, JSON};

  // Neural Network Lateral Control
  keys["NeuralNetworkLateralControl"] = {PERSISTENT | BACKUP, BOOL, "0"};

  // sunnylink params
  keys["EnableSunnylinkUploader"] = {PERSISTENT | BACKUP, BOOL};
  keys["LastSunnylinkPingTime"] = {CLEAR_ON_MANAGER_START, INT};
  keys["SunnylinkCache_Roles"] = {PERSISTENT, STRING};
  keys["SunnylinkCache_Users"] = {PERSISTENT, STRING};
  keys["SunnylinkDongleId"] = {PERSISTENT, STRING};
  keys["SunnylinkdPid"] = {PERSISTENT, INT};
  keys["SunnylinkEnabled"] = {PERSISTENT, BOOL};

  // Backup Manager params
  keys["BackupManager_CreateBackup"] = {PERSISTENT, BOOL};
  keys["BackupManager_RestoreVersion"] = {PERSISTENT, STRING};

  // sunnypilot car specific params
  keys["HyundaiLongitudinalTuning"] = {PERSISTENT | BACKUP, INT, "0"};

  keys["DynamicExperimentalControl"] = {PERSISTENT | BACKUP, BOOL, "0"};
  keys["BlindSpot"] = {PERSISTENT | BACKUP, BOOL, "0"};

  // sunnypilot model params
  keys["LagdToggle"] = {PERSISTENT | BACKUP, BOOL, "1"};
  keys["LagdToggleDelay"] = {PERSISTENT | BACKUP, FLOAT, "0.2"};
  keys["LagdValueCache"] = {PERSISTENT, FLOAT, "0.2"};
  keys["LaneTurnDesire"] = {PERSISTENT | BACKUP, BOOL, "0"};
  keys["LaneTurnValue"] = {PERSISTENT | BACKUP, FLOAT, "19.0"};

  // 端到端控制参数
  keys["EndToEndToggle"] = {PERSISTENT | BACKUP, BOOL, "0"};
  keys["EndToEndForceLaneChange"] = {PERSISTENT | BACKUP, BOOL, "0"};

  // mapd
  keys["MapAdvisorySpeedLimit"] = {CLEAR_ON_ONROAD_TRANSITION, FLOAT};
  keys["MapdVersion"] = {PERSISTENT, STRING};
  keys["MapSpeedLimit"] = {CLEAR_ON_ONROAD_TRANSITION, FLOAT, "0.0"};
  keys["NextMapSpeedLimit"] = {CLEAR_ON_ONROAD_TRANSITION, JSON};
  keys["Offroad_OSMUpdateRequired"] = {CLEAR_ON_MANAGER_START, JSON};
  keys["OsmDbUpdatesCheck"] = {CLEAR_ON_MANAGER_START, BOOL};  // mapd database update happens with device ON, reset on boot
  keys["OSMDownloadBounds"] = {PERSISTENT, STRING};
  keys["OsmDownloadedDate"] = {PERSISTENT, STRING, "0.0"};
  keys["OSMDownloadLocations"] = {PERSISTENT, JSON};
  keys["OSMDownloadProgress"] = {CLEAR_ON_MANAGER_START, JSON};
  keys["OsmLocal"] = {PERSISTENT, BOOL};
  keys["OsmLocationName"] = {PERSISTENT, STRING};
  keys["OsmLocationTitle"] = {PERSISTENT, STRING};
  keys["OsmLocationUrl"] = {PERSISTENT, STRING};
  keys["OsmStateName"] = {PERSISTENT, STRING, "All"};
  keys["OsmStateTitle"] = {PERSISTENT, STRING};
  keys["OsmWayTest"] = {PERSISTENT, STRING};
  keys["RoadName"] = {CLEAR_ON_ONROAD_TRANSITION, STRING};

  // Speed Limit Control
  keys["SpeedLimitControl"] = {PERSISTENT | BACKUP, BOOL, "0"};
  keys["SpeedLimitControlPolicy"] = {PERSISTENT | BACKUP, INT, "3"};
  keys["SpeedLimitEngageType"] = {PERSISTENT | BACKUP, INT, "0"};
  keys["SpeedLimitOffsetType"] = {PERSISTENT | BACKUP, INT, "0"};
  keys["SpeedLimitValueOffset"] = {PERSISTENT | BACKUP, INT, "0"};
  keys["SpeedLimitWarningType"] = {PERSISTENT | BACKUP, INT, "0"};
  keys["SpeedLimitWarningOffsetType"] = {PERSISTENT | BACKUP, INT, "0"};
  keys["SpeedLimitWarningValueOffset"] = {PERSISTENT | BACKUP, INT, "0"};

  // Mapbox and Carrot parameters
  keys["MapboxPublicKey"] = {PERSISTENT, STRING, ""};
  keys["CarrotException"] = {PERSISTENT, BOOL, "0"};

  // 从params_keys_cp.h恢复的参数
  keys["CarrotException"] = {CLEAR_ON_MANAGER_START, STRING};
  keys["CarName"] = {PERSISTENT, STRING};
  keys["EVTable"] = {PERSISTENT, BOOL, "1"};
  keys["LongPitch"] = {PERSISTENT, BOOL, "1"};
  keys["ActivateCruiseAfterBrake"] = {CLEAR_ON_MANAGER_START, BOOL};
  keys["ComfortBrake"] = {PERSISTENT, BOOL};
  keys["AutoOvertakeConfig"] = {PERSISTENT, BOOL};
  keys["DevicePosition"] = {CLEAR_ON_MANAGER_START, BOOL};
  keys["NNFFModelName"] = {CLEAR_ON_OFFROAD_TRANSITION, STRING};
  keys["EnableRadarTracksResult"] = {PERSISTENT | CLEAR_ON_MANAGER_START, BOOL};
  keys["CanParserResult"] = {CLEAR_ON_MANAGER_START | CLEAR_ON_OFFROAD_TRANSITION, BOOL};
  keys["FingerPrints"] = {PERSISTENT | CLEAR_ON_MANAGER_START, STRING};

  // 车辆选择相关的参数
  keys["CarSelected3"] = {PERSISTENT, STRING};
  keys["SupportedCars"] = {PERSISTENT, STRING};
  keys["SupportedCars_gm"] = {PERSISTENT, STRING};
  keys["SupportedCars_honda"] = {PERSISTENT, STRING};
  keys["SupportedCars_hyundai"] = {PERSISTENT, STRING};
  keys["SupportedCars_toyota"] = {PERSISTENT, STRING};
  keys["SupportedCars_mazda"] = {PERSISTENT, STRING};
  keys["SupportedCars_tesla"] = {PERSISTENT, STRING};
  keys["SupportedCars_volkswagen"] = {PERSISTENT, STRING};

  return keys;
}

// 为了兼容性，保留原来的 keys 变量
inline static std::unordered_map<std::string, ParamKeyAttributes> keys = get_keys();