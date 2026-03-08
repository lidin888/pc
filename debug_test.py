#!/usr/bin/env python3

import sys
import os
sys.path.append(os.path.dirname(os.path.abspath(__file__)))

from openpilot.selfdrive.test.longitudinal_maneuvers.maneuver import Maneuver

# Create a simple maneuver with lead car
man = Maneuver(
    'test',
    duration=20.0,  # Run for 20 seconds
    initial_speed=0.0,
    lead_relevancy=True,
    initial_distance_lead=100,
    speed_lead_values=[10.0],  # Lead car at 10 m/s
    breakpoints=[0.0],
    e2e=False,
    personality=0,
)

print("Starting maneuver evaluation...")
valid, output = man.evaluate()
print(f"Maneuver valid: {valid}")
print(f"Final distance between cars: {output[-1,2] - output[-1,1]}")
print(f"Final ego car speed: {output[-1,3]}")
print(f"Final lead car speed: {output[-1,4]}")
print(f"Final acceleration: {output[-1,5]}")