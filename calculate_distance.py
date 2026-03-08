#!/usr/bin/env python3

from openpilot.selfdrive.controls.lib.longitudinal_mpc_lib.long_mpc import desired_follow_distance, get_T_FOLLOW
from cereal import log

# Test parameters
v_lead = 0.0  # Lead car speed (from TestFollowingDistance_0)
personality = log.LongitudinalPersonality.relaxed  # Default personality

# Calculate expected following distance
t_follow = get_T_FOLLOW(personality)
correct_steady_state = desired_follow_distance(v_lead, v_lead, 1.0, 0.5, t_follow)

print(f"Lead car speed: {v_lead} m/s")
print(f"T_FOLLOW: {t_follow}")
print(f"Expected following distance: {correct_steady_state} m")

# Also test with other speeds
for speed in [0, 10, 35]:
    t_follow = get_T_FOLLOW(personality)
    distance = desired_follow_distance(speed, speed, 1.0, 0.5, t_follow)
    print(f"Speed {speed} m/s: expected distance = {distance:.3f} m")