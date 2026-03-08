# SunnyPilot Car Interfaces

from collections import namedtuple

LatControlInputs = namedtuple('LatControlInputs', ['setpoint', 'roll_compensation', 'v_ego', 'a_ego'])
