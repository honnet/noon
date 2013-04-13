import re
import sys
import time
import rtmidi

def mapping(group, effect, value):
    return 3 * (group * 4 + effect) + value

class Midi:
    midiout = rtmidi.MidiOut()

    def __init__(self):
      available_ports = self.midiout.get_ports()
      if available_ports:
          if len(available_ports) > 1:
              print "Available ports:", available_ports
              portNum = 1
          else:
              portNum = 0
          self.midiout.open_port(portNum)
          print "Using the port '", available_ports[portNum], "'"
      else:
          print "Using VirtualRtMidiPort."
          self.midiout.open_virtual_port("VirtualRtMidiPort")

    def note_on(self, channel, note, velocity):
        channelON = 0x90 | channel
        note_on = [channelON, note, velocity]
        self.midiout.send_message(note_on)

    def note_off(self, channel, note):
        channelOFF = 0x80 | channel
        note_off = [channelOFF, note, 0]
        self.midiout.send_message(note_off)

    def cc(self, channel, number, value):
        print 'control_change<{}:{}:{}>'.format(channel, number, value)
        assert(channel < 16)
        channelCC = 0xB0 | channel
        cc = [channelCC, number, value]
        self.midiout.send_message(cc)

class Controller (object):
    def __init__(self, midi):
        self.midi = midi

class Transport (Controller):
    def __init__(self, midi):
        super(Transport, self).__init__(midi)

    def play(self):
        self.midi.cc(15, 15, 127)

    def stop(self):
        self.midi.cc(15, 14, 127)

    def record(self):
        self.midi.cc(15, 13, 127)

class Group (Controller):
    def __init__(self, id_, midi):
        super(Group, self).__init__(midi)
        self.id = id_
        self.effects = []

    def effect(self, effect):
        return self.effects[effect]

class Effect (Controller):
    def __init__(self, id_, midi, group):
        super(Effect, self).__init__(midi)
        self.group = group
        self.id = id_

    def control(self, offset):
        return mapping(self.group.id, self.id, offset)

    def enable(self, active=True):
        control = self.control(0)
        self.midi.cc(1, control, 127 if active else 0)

    def disable(self):
        self.enable(False)

class Effect1D (Effect):
    def __init__(self, id_, midi, group):
        super(Effect1D, self).__init__(id_, midi, group)

    def set_value(self, value):
        control = self.control(1)
        self.midi.cc(1, control, value)

    def set_value_from_accelerometer(self, x, y, z):
        self.set_value(z)

class Effect2D (Effect):
    def __init__(self, id_, midi, group):
        super(Effect2D, self).__init__(id_, midi, group)

    def set_value(self, x, y):
        control = self.control(1)
        self.midi.cc(1, control, x)
        control = self.control(2)
        self.midi.cc(1, control, y)

    def set_value_from_accelerometer(self, x, y, z):
        self.set_value(x, y)

class Parser:
    def __init__(self):
        self.midi = Midi()
        self.groups = []
        for i in range(0, 5):
            group = Group(i, self.midi)
            for j in range(0, 4):
                group.effects.append(Effect2D(j, self.midi, group))
            self.groups.append(group)

        self.transport = Transport(self.midi)

    def parse(self):
        while True:
            line = sys.stdin.readline()

            if not line:
                break

            cmd = line[0]

            if cmd == 'E':      # Enable effect
                group = int(line[1])
                effect = int(line[2])
                self.groups[group].effect(effect).enable()
            elif cmd == 'M':    # Modulate effect
                group = int(line[1])
                effect = int(line[2])
                x, y, z = [int(v) for v in line[3:].split(':')]
                self.groups[group].effect(effect).set_value_from_accelerometer(x, y, z)
            elif cmd == 'D':    # Disable effect
                group = int(line[1])
                effect = int(line[2])
                self.groups[group].effect(effect).disable()
            elif cmd == 'P':    # Play
                self.transport.play()
            elif cmd == 'S':    # Stop
                self.transport.stop()
            elif cmd == 'R':    # Record
                self.transport.record()
            elif cmd == 'B':    # Begin note
                note = int(line[1:])
                self.midi.note_on(0, note, 127)
                print 'note on', note
            elif cmd == 'F':    # Finish note
                note = int(line[1:])
                self.midi.note_off(0, note)
                print 'note off', note


def main():
    parser = Parser()
    parser.parse()

if __name__ == '__main__':
    main()

#   # Midi Learn code:
#   # comment the main call on top!

#   midi = Midi()
#   for group in range(0, 5):
#       for effect in range(0, 4):
#           for i, value in zip(range(0, 3), ['active', 'x', 'y']):
#               cc = mapping(group, effect, i)
#               print 'group:{} effect:{} {}'.format(group, effect, value)
#               raw_input()
#               midi.cc(1, cc, 127)
#               print '\n'

#   # next and previous scene commands:

#   transport = Transport(self.midi)
#   print 'prev'
#   raw_input()
#   transport.prev_scene()
#   print 'next'
#   raw_input()
#   transport.next_scene()
#   print '\n'

