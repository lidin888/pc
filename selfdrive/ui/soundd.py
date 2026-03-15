import math
import numpy as np
import time
import wave


from cereal import car, messaging, custom
from openpilot.common.basedir import BASEDIR
from openpilot.common.filter_simple import FirstOrderFilter
from openpilot.common.params import Params
from openpilot.common.realtime import Ratekeeper
from openpilot.common.retry import retry
from openpilot.common.swaglog import cloudlog

from openpilot.system import micd

from openpilot.selfdrive.ui.sunnypilot.quiet_mode import QuietMode

SAMPLE_RATE = 48000
SAMPLE_BUFFER = 4096 # (approx 100ms)
MAX_VOLUME = 1.0
MIN_VOLUME = 0.1
SELFDRIVE_STATE_TIMEOUT = 5 # 5 seconds
FILTER_DT = 1. / (micd.SAMPLE_RATE / micd.FFT_SAMPLES)

AMBIENT_DB = 30 # DB where MIN_VOLUME is applied
DB_SCALE = 30 # AMBIENT_DB + DB_SCALE is where MAX_VOLUME is applied

AudibleAlert = car.CarControl.HUDControl.AudibleAlert
AudibleAlertSP = custom.SelfdriveStateSP.AudibleAlert


sound_list_sp: dict[int, tuple[str, int | None, float]] = {
  # AudibleAlertSP, file name, play count (none for infinite)
  AudibleAlertSP.promptSingleLow: ("prompt_single_low.wav", 1, MAX_VOLUME),
  AudibleAlertSP.promptSingleHigh: ("prompt_single_high.wav", 1, MAX_VOLUME),

  # CarrotPilot audio alerts
  AudibleAlertSP.audioTurn: ("audio_turn.wav", None, MAX_VOLUME),
  AudibleAlertSP.longEngaged: ("tici_engaged.wav", None, MAX_VOLUME),
  AudibleAlertSP.longDisengaged: ("tici_disengaged.wav", None, MAX_VOLUME),
  AudibleAlertSP.trafficSignGreen: ("traffic_sign_green.wav", None, MAX_VOLUME),
  AudibleAlertSP.trafficSignChanged: ("traffic_sign_changed.wav", None, MAX_VOLUME),
  AudibleAlertSP.laneChangeCarrot: ("audio_lane_change.wav", None, MAX_VOLUME),
  AudibleAlertSP.stopping: ("audio_stopping.wav", None, MAX_VOLUME),
  AudibleAlertSP.autoHold: ("audio_auto_hold.wav", None, MAX_VOLUME),
  AudibleAlertSP.engage2: ("audio_engage.wav", None, MAX_VOLUME),
  AudibleAlertSP.disengage2: ("audio_disengage.wav", None, MAX_VOLUME),
  AudibleAlertSP.trafficError: ("audio_traffic_error.wav", None, MAX_VOLUME),
  AudibleAlertSP.bsdWarning: ("audio_car_watchout.wav", None, MAX_VOLUME),
  AudibleAlertSP.speedDown: ("audio_speed_down.wav", None, MAX_VOLUME),
  AudibleAlertSP.stopStop: ("audio_stopstop.wav", None, MAX_VOLUME),
  AudibleAlertSP.reverseGear2: ("reverse_gear.wav", 1, MAX_VOLUME),
  AudibleAlertSP.audio1: ("audio_1.wav", None, MAX_VOLUME),
  AudibleAlertSP.audio2: ("audio_2.wav", None, MAX_VOLUME),
  AudibleAlertSP.audio3: ("audio_3.wav", None, MAX_VOLUME),
  AudibleAlertSP.audio4: ("audio_4.wav", None, MAX_VOLUME),
  AudibleAlertSP.audio5: ("audio_5.wav", None, MAX_VOLUME),
  AudibleAlertSP.audio6: ("audio_6.wav", None, MAX_VOLUME),
  AudibleAlertSP.audio7: ("audio_7.wav", None, MAX_VOLUME),
  AudibleAlertSP.audio8: ("audio_8.wav", None, MAX_VOLUME),
  AudibleAlertSP.audio9: ("audio_9.wav", None, MAX_VOLUME),
  AudibleAlertSP.audio10: ("audio_10.wav", None, MAX_VOLUME),
  AudibleAlertSP.nnff: ("nnff.wav", None, MAX_VOLUME),
  AudibleAlertSP.preLaneChangeCarrot: ("audio_pre_lane_change.wav", None, MAX_VOLUME),
  AudibleAlertSP.atcCancel: ("audio_atc_cancel.wav", None, MAX_VOLUME),
  AudibleAlertSP.atcResume: ("audio_atc_resume.wav", None, MAX_VOLUME),
  AudibleAlertSP.preLaneChangeLeft2: ("audio_pre_lane_left.wav", None, MAX_VOLUME),
  AudibleAlertSP.preLaneChangeRight2: ("audio_pre_lane_right.wav", None, MAX_VOLUME),
  AudibleAlertSP.laneChangeOk: ("audio_lane_change_ok.wav", None, MAX_VOLUME),
  AudibleAlertSP.lastLane: ("audio_last_lane.wav", None, MAX_VOLUME),
  AudibleAlertSP.newLane: ("audio_new_lane.wav", None, MAX_VOLUME),
  AudibleAlertSP.laneChangeEnd: ("audio_lane_change_end.wav", None, MAX_VOLUME),
}

sound_list: dict[int, tuple[str, int | None, float]] = {
  # AudibleAlert, file name, play count (none for infinite)
  AudibleAlert.engage: ("engage.wav", 1, MAX_VOLUME),
  AudibleAlert.disengage: ("disengage.wav", 1, MAX_VOLUME),
  AudibleAlert.refuse: ("refuse.wav", 1, MAX_VOLUME),

  AudibleAlert.prompt: ("prompt.wav", 1, MAX_VOLUME),
  AudibleAlert.promptRepeat: ("prompt.wav", None, MAX_VOLUME),
  AudibleAlert.promptDistracted: ("prompt_distracted.wav", None, MAX_VOLUME),

  AudibleAlert.warningSoft: ("warning_soft.wav", None, MAX_VOLUME),
  AudibleAlert.warningImmediate: ("warning_immediate.wav", None, MAX_VOLUME),

  **sound_list_sp,
}

def check_selfdrive_timeout_alert(sm):
  ss_missing = time.monotonic() - sm.recv_time['selfdriveState']

  if ss_missing > SELFDRIVE_STATE_TIMEOUT:
    if sm['selfdriveState'].enabled and (ss_missing - SELFDRIVE_STATE_TIMEOUT) < 10:
      return True

  return False


def linear_resample(samples, original_rate, new_rate):
    if original_rate == new_rate:
        return samples
    resampling_factor = float(new_rate) / original_rate
    num_resampled_samples = int(len(samples) * resampling_factor)
    resampled = np.zeros(num_resampled_samples, dtype=np.float32)
    for i in range(num_resampled_samples):
        orig_index = i / resampling_factor
        lower_index = int(orig_index)
        upper_index = min(lower_index + 1, len(samples) - 1)
        resampled[i] = (samples[lower_index] * (upper_index - orig_index) +
                        samples[upper_index] * (orig_index - lower_index))
    return resampled


class Soundd(QuietMode):
  def __init__(self):
    super().__init__()

    self.params = Params()
    self.soundVolumeAdjust = 1.0
    self.carrot_count_down = 0

    # CarrotPilot audio state tracking
    self.carrot_atc_type_last = ""
    self.carrot_traffic_state_last = 0
    self.carrot_spd_dist_last = 0
    self.carrot_spd_limit_last = 0
    self.carrot_enabled_last = False

    self.lang = self.params.get('LanguageSetting') or 'main_en'
    self.load_sounds()

    self.current_alert = AudibleAlert.none
    self.current_volume = MIN_VOLUME
    self.current_sound_frame = 0

    self.selfdrive_timeout_alert = False
    self.sound_pressure_received = False
    self.sound_pressure_timeout_frames = 0

    self.spl_filter_weighted = FirstOrderFilter(0, 2.5, FILTER_DT, initialized=False)

  def load_sounds(self):
    self.loaded_sounds: dict[int, np.ndarray] = {}

    # Load all sounds
    for sound in sound_list:
      filename, play_count, volume = sound_list[sound]

      # Language-aware sound loading: Korean (main_ko) uses sounds/, others use sounds_eng/
      if self.lang == "main_ko":
        sound_path = BASEDIR + "/selfdrive/assets/sounds/" + filename
      else:
        eng_path = BASEDIR + "/selfdrive/assets/sounds_eng/" + filename
        default_path = BASEDIR + "/selfdrive/assets/sounds/" + filename
        import os
        sound_path = eng_path if os.path.exists(eng_path) else default_path

      try:
        with wave.open(sound_path, 'r') as wavefile:
          assert wavefile.getsampwidth() == 2
          actual_sample_rate = wavefile.getframerate()
          nchannels = wavefile.getnchannels()
          assert nchannels in [1, 2]

          length = wavefile.getnframes()
          frames = wavefile.readframes(length)
          samples = np.frombuffer(frames, dtype=np.int16).astype(np.float32)

          if nchannels == 2:
            samples = samples[0::2] / 2 + samples[1::2] / 2

          resampled_samples = linear_resample(samples, actual_sample_rate, SAMPLE_RATE) * volume
          self.loaded_sounds[sound] = resampled_samples / (2**16/2)
      except Exception as e:
        cloudlog.warning(f"Failed to load sound {filename}: {e}")
        # Create a short silence as fallback
        self.loaded_sounds[sound] = np.zeros(SAMPLE_RATE // 10, dtype=np.float32)

  def get_sound_data(self, frames): # get "frames" worth of data from the current alert sound, looping when required

    ret = np.zeros(frames, dtype=np.float32)

    if self.should_play_sound(self.current_alert):
      num_loops = sound_list[self.current_alert][1]
      sound_data = self.loaded_sounds[self.current_alert]
      written_frames = 0

      current_sound_frame = self.current_sound_frame % len(sound_data)
      loops = self.current_sound_frame // len(sound_data)

      while written_frames < frames and (num_loops is None or loops < num_loops):
        available_frames = sound_data.shape[0] - current_sound_frame
        frames_to_write = min(available_frames, frames - written_frames)
        ret[written_frames:written_frames+frames_to_write] = sound_data[current_sound_frame:current_sound_frame+frames_to_write]
        written_frames += frames_to_write
        self.current_sound_frame += frames_to_write

    return ret * self.current_volume

  def callback(self, data_out: np.ndarray, frames: int, time, status) -> None:
    if status:
      cloudlog.warning(f"soundd stream over/underflow: {status}")
    data_out[:frames, 0] = self.get_sound_data(frames)

  def update_alert(self, new_alert):
    current_alert_played_once = self.current_alert == AudibleAlert.none or self.current_sound_frame > len(self.loaded_sounds[self.current_alert])
    if self.current_alert != new_alert and (new_alert != AudibleAlert.none or current_alert_played_once):
      self.current_alert = new_alert
      self.current_sound_frame = 0

  def update_carrot_alert(self, sm, new_alert):
    """Handle CarrotPilot audio alerts by watching carrotMan field changes.

    Also handles engage/disengage sounds since MADS may remove standard events.

    Audio triggers:
    - atcType changes: lane change, turn, ATC cancel/resume sounds
    - trafficState changes: traffic light green/changed sounds
    - leftSec countdown: audio_1 ~ audio_10 countdown sounds
    - xSpdDist approaching: speed camera warning sounds
    """
    # === 0. Engage/disengage sounds (MADS may suppress standard events) ===
    if sm.updated.get('selfdriveState', False):
      enabled_now = sm['selfdriveState'].enabled
      if enabled_now and not self.carrot_enabled_last:
        self.carrot_enabled_last = True
        if new_alert == AudibleAlert.none:
          return AudibleAlert.engage
      elif not enabled_now and self.carrot_enabled_last:
        self.carrot_enabled_last = False
        if new_alert == AudibleAlert.none:
          return AudibleAlert.disengage

    if new_alert != AudibleAlert.none:
      return new_alert

    if not sm.alive.get('carrotMan', False):
      return new_alert

    cm = sm['carrotMan']

    # === 1. atcType state machine → lane change / turn / ATC sounds ===
    atc_type = cm.atcType
    if atc_type != self.carrot_atc_type_last:
      old_atc = self.carrot_atc_type_last
      self.carrot_atc_type_last = atc_type

      # "prepare" disappears → action is starting
      if "prepare" not in atc_type and "prepare" in old_atc:
        if "atc" in atc_type:
          new_alert = AudibleAlertSP.preLaneChangeCarrot  # 准备变道
        elif "fork" in atc_type:
          if "now" in atc_type:
            new_alert = AudibleAlertSP.laneChangeCarrot  # 立即变道
          else:
            new_alert = AudibleAlertSP.preLaneChangeCarrot
      elif "prepare" in atc_type:
        pass  # 进入准备阶段，不播报
      # "atc" disappears
      elif "atc" not in atc_type and "atc" in old_atc:
        if not atc_type:  # atcType cleared → ATC cancel
          new_alert = AudibleAlertSP.atcCancel
        elif "fork" in atc_type:
          if "now" in atc_type:
            new_alert = AudibleAlertSP.laneChangeCarrot
          else:
            new_alert = AudibleAlertSP.preLaneChangeCarrot
        elif "turn" in atc_type:
          new_alert = AudibleAlertSP.audioTurn
      # new "turn" appears
      elif "turn" in atc_type and "turn" not in old_atc:
        new_alert = AudibleAlertSP.audioTurn
      # "now" appears (fork left/right → fork left/right now)
      elif "now" in atc_type and "now" not in old_atc:
        if "fork" in atc_type:
          new_alert = AudibleAlertSP.laneChangeCarrot
      # ATC resume (from empty to active)
      elif atc_type and not old_atc:
        if "atc" in atc_type or "fork" in atc_type:
          new_alert = AudibleAlertSP.atcResume

    if new_alert != AudibleAlert.none:
      return new_alert

    # === 2. trafficState changes → traffic light sounds ===
    traffic_state = cm.trafficState
    if traffic_state != self.carrot_traffic_state_last:
      old_traffic = self.carrot_traffic_state_last
      self.carrot_traffic_state_last = traffic_state
      if traffic_state == 2 and old_traffic != 2:  # → green
        new_alert = AudibleAlertSP.trafficSignGreen
      elif traffic_state == 1 and old_traffic != 1:  # → red
        new_alert = AudibleAlertSP.trafficSignChanged

    if new_alert != AudibleAlert.none:
      return new_alert

    # === 3. Speed camera approaching → speed down sound ===
    spd_limit = cm.xSpdLimit
    spd_dist = cm.xSpdDist
    if spd_limit > 0 and spd_dist > 0:
      # Trigger speed down alert when camera first detected or distance drops below threshold
      if self.carrot_spd_limit_last == 0 and spd_limit > 0:
        new_alert = AudibleAlertSP.speedDown
      self.carrot_spd_limit_last = spd_limit
      self.carrot_spd_dist_last = spd_dist
    else:
      self.carrot_spd_limit_last = 0
      self.carrot_spd_dist_last = 0

    if new_alert != AudibleAlert.none:
      return new_alert

    # === 4. leftSec countdown → audio_1 ~ audio_10 ===
    count_down = cm.leftSec
    if self.carrot_count_down != count_down:
      self.carrot_count_down = count_down
      if count_down == 0:
        new_alert = AudibleAlertSP.longDisengaged
      elif 0 < count_down <= 10:
        alert_name = f'audio{count_down}'
        new_alert = getattr(AudibleAlertSP, alert_name, AudibleAlert.none)
      elif count_down == 11:
        new_alert = AudibleAlert.promptDistracted

    return new_alert

  def get_audible_alert(self, sm):
    if sm.updated['selfdriveState']:
      new_alert = sm['selfdriveState'].alertSound.raw
      new_alert = self.update_carrot_alert(sm, new_alert)
      self.update_alert(new_alert)
    elif sm.updated.get('carrotMan', False):
      # Process carrotMan updates independently of selfdriveState
      new_alert = self.update_carrot_alert(sm, AudibleAlert.none)
      if new_alert != AudibleAlert.none:
        self.update_alert(new_alert)
    elif check_selfdrive_timeout_alert(sm):
      self.update_alert(AudibleAlert.warningImmediate)
      self.selfdrive_timeout_alert = True
    elif self.selfdrive_timeout_alert:
      self.update_alert(AudibleAlert.none)
      self.selfdrive_timeout_alert = False

  def calculate_volume(self, weighted_db):
    volume = ((weighted_db - AMBIENT_DB) / DB_SCALE) * (MAX_VOLUME - MIN_VOLUME) + MIN_VOLUME
    return math.pow(10, (np.clip(volume, MIN_VOLUME, MAX_VOLUME) - 1))

  @retry(attempts=7, delay=3)
  def get_stream(self, sd):
    # reload sounddevice to reinitialize portaudio
    sd._terminate()
    sd._initialize()
    return sd.OutputStream(channels=1, samplerate=SAMPLE_RATE, callback=self.callback, blocksize=SAMPLE_BUFFER)

  def soundd_thread(self):
    # sounddevice must be imported after forking processes
    import sounddevice as sd

    sm = messaging.SubMaster(['selfdriveState', 'soundPressure', 'carrotMan'])

    with self.get_stream(sd) as stream:
      rk = Ratekeeper(20)

      cloudlog.info(f"soundd stream started: {stream.samplerate=} {stream.channels=} {stream.dtype=} {stream.device=}, {stream.blocksize=}")
      while True:
        sm.update(0)

        self.load_param()

        if sm.updated['soundPressure'] and self.current_alert == AudibleAlert.none: # only update volume filter when not playing alert
          self.sound_pressure_received = True
          self.spl_filter_weighted.update(sm["soundPressure"].soundPressureWeightedDb)
          self.current_volume = self.calculate_volume(float(self.spl_filter_weighted.x)) * self.soundVolumeAdjust
        elif not self.sound_pressure_received:
          # PC mode or micd not running: use default volume after 3 seconds
          self.sound_pressure_timeout_frames += 1
          if self.sound_pressure_timeout_frames > 60:  # 3 seconds at 20Hz
            self.current_volume = MAX_VOLUME * self.soundVolumeAdjust

        self.get_audible_alert(sm)

        rk.keep_time()

        assert stream.active

        try:
          self.soundVolumeAdjust = float(self.params.get_int("SoundVolumeAdjust")) / 100.
        except Exception:
          self.soundVolumeAdjust = 1.0


def main():
  s = Soundd()
  s.soundd_thread()


if __name__ == "__main__":
  main()
