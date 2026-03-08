using Cxx = import "./include/c++.capnp";
$Cxx.namespace("cereal");

@0xb526ba661d550a59;

# custom.capnp: a home for empty structs reserved for custom forks
# These structs are guaranteed to remain reserved and empty in mainline
# cereal, so use these if you want custom events in your fork.

# DO rename the structs
# DON'T change the identifier (e.g. @0x81c2f05a394cf4af)

# you can rename the struct, but don't change the identifier
struct CarrotMan @0x81c2f05a394cf4af {
	activeCarrot @0 : Int32;
	nRoadLimitSpeed @1 : Int32;
	remote @2 : Text;
	xSpdType @3 : Int32;
	xSpdLimit @4 : Int32;
	xSpdDist @5 : Int32;
	xSpdCountDown @6 : Int32;
	xTurnInfo @7 : Int32;
	xDistToTurn @8 : Int32;
	xTurnCountDown @9 : Int32;
	atcType @10 : Text;
	vTurnSpeed @11 : Int32;
	szPosRoadName @12 : Text;
	szTBTMainText @13 : Text;
	desiredSpeed @14 : Int32;
	desiredSource @15 : Text;
	carrotCmdIndex @16 : Int32;
	carrotCmd @17 : Text;
	carrotArg @18 : Text;
	xPosLat @19 : Float32;
	xPosLon @20 : Float32;
	xPosAngle @21 : Float32;
	xPosSpeed @22 : Float32;
	trafficState @23 : Int32;
	nGoPosDist @24 : Int32;
	nGoPosTime @25 : Int32;
	szSdiDescr @26 : Text;
	naviPaths @27 : Text;
	leftSec @28 : Int32;

	xDistToTurnNav @29 : Int32;
	xDistToTurnNavLast @30 : Int32;
	xDistToTurnMax @31 : Int32;
	xDistToTurnMaxCnt @32 : Int32;
	xLeftTurnSec @33 : Int32;
	roadCate @34 : Int32;
	extBlinker @35 : Int32;
	extState @36 : Int32;

	leftBlind @37 : Int32;
	rightBlind @38 : Int32;
}

struct AmapNavi @0xaedffd8f31e7b55d {
	leftBlind @0 : Int32;
	rightBlind @1 : Int32;
}

struct LongitudinalPlanSP @0xf35cc4560bbf6ec2 {
  dec @0 :DynamicExperimentalControl;
  accelPersonality @3 :AccelerationPersonality;
  visionTurnSpeedControl @4 :VisionTurnSpeedControl;

  events @1 :List(OnroadEventSP.Event);
  slc @2 :SpeedLimitControl;

  struct DynamicExperimentalControl {
    state @0 :DynamicExperimentalControlState;
    enabled @1 :Bool;
    active @2 :Bool;

    enum DynamicExperimentalControlState {
      acc @0;
      blended @1;
    }
  }

  enum AccelerationPersonality {
    sport @0;
    normal @1;
    eco @2;
  }

  struct VisionTurnSpeedControl {
    state @0 :VisionTurnSpeedControlState;
    velocity @1 :Float32;
    currentLateralAccel @2 :Float32;
    maxPredictedLateralAccel @3 :Float32;

    enum VisionTurnSpeedControlState {
      disabled @0; # No predicted substantial turn on vision range or feature disabled.
      entering @1; # A substantial turn is predicted ahead, adapting speed to turn comfort levels.
      turning @2; # Actively turning. Managing acceleration to provide a roll on turn feeling.
      leaving @3; # Road ahead straightens. Start to allow positive acceleration.
    }
  }

  struct SpeedLimitControl {
    state @0 :SpeedLimitControlState;
    enabled @1 :Bool;
    active @2 :Bool;
    speedLimit @3 :Float32;
    speedLimitOffset @4 :Float32;
    distToSpeedLimit @5 :Float32;
  }

  enum SpeedLimitControlState {
    inactive @0; # No speed limit set or not enabled by parameter.
    tempInactive @1; # User wants to ignore speed limit until it changes.
    preActive @2;
    adapting @3; # Reducing speed to match new speed limit.
    active @4; # Cruising at speed limit.
  }
}

struct OnroadEventSP @0xda96579883444c35 {
  events @0 :List(Event);

  struct Event {
    name @0 :EventName;

    # event types
    enable @1 :Bool;
    noEntry @2 :Bool;
    warning @3 :Bool;   # alerts presented only when  enabled or soft disabling
    userDisable @4 :Bool;
    softDisable @5 :Bool;
    immediateDisable @6 :Bool;
    preEnable @7 :Bool;
    permanent @8 :Bool; # alerts presented regardless of openpilot state
    overrideLateral @10 :Bool;
    overrideLongitudinal @9 :Bool;
  }

  enum EventName {
    lkasEnable @0;
    lkasDisable @1;
    manualSteeringRequired @2;
    manualLongitudinalRequired @3;
    silentLkasEnable @4;
    silentLkasDisable @5;
    silentBrakeHold @6;
    silentWrongGear @7;
    silentReverseGear @8;
    silentDoorOpen @9;
    silentSeatbeltNotLatched @10;
    silentParkBrake @11;
    controlsMismatchLateral @12;
    hyundaiRadarTracksConfirmed @13;
    experimentalModeSwitched @14;
    wrongCarModeAlertOnly @15;
    pedalPressedAlertOnly @16;
    speedLimitPreActive @17;
    speedLimitActive @18;
    speedLimitConfirmed @19;
    speedLimitValueChange @20;
    laneTurnLeft @21;
    laneTurnRight @22;
  }
}

struct CustomReserved4 @0x80ae746ee2596b11 {
}

struct CustomReserved5 @0xa5cd762cd951a455 {
}

struct CustomReserved6 @0xf98d843bfd7004a3 {
}

struct CustomReserved7 @0xb86e6369214c01c8 {
}

struct CustomReserved8 @0xf416ec09499d9d19 {
}

struct CustomReserved9 @0xa1680744031fdb2d {
}

struct CustomReserved10 @0xcb9fd56c7057593a {
}

struct CustomReserved11 @0xc2243c65e0340384 {
}

struct CustomReserved12 @0x9ccdc8676701b412 {
}

struct CustomReserved13 @0xcd96dafb67a082d0 {
}

struct CustomReserved14 @0xb057204d7deadf3f {
}

struct CustomReserved15 @0xbd443b539493bc68 {
}

struct CustomReserved16 @0xfc6241ed8877b611 {
}

struct CustomReserved17 @0xa30662f84033036c {
}

struct CustomReserved18 @0xc86a3d38d13eb3ef {
}

struct CustomReserved19 @0xa4f1eb3323f5f582 {
}