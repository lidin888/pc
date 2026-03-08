# SunnyPilot Events

class EventsSP:
  def __init__(self):
    self.events = []

  def add(self, event):
    self.events.append(event)

  def has(self, event):
    return event in self.events

  def remove(self, event):
    if event in self.events:
      self.events.remove(event)
