import re
import sys
import time
import rtmidi

################################################################################

def note_on(channel, note, velocity):
    assert (channel  >= 0) & (channel  <= 15)
    assert (note     >= 0) & (note     <= 127)
    assert (velocity >= 0) & (velocity <= 127)
    channelON  = 0x90 | channel
    note_on =  [channelON, note, velocity]
    midiout.send_message(note_on)

def note_off(channel, note):
    assert (channel  >= 0) & (channel  <= 15)
    assert (note     >= 0) & (note     <= 127)
    channelOFF  = 0x80 | channel
    note_off = [channelOFF, note, 0]
    midiout.send_message(note_off)

################################################################################

midiout = rtmidi.MidiOut()
available_ports = midiout.get_ports()

if available_ports:
    print "Using the 1st available ports in:", available_ports
    midiout.open_port(0)
else:
    print "Using VirtualRtMidiPort."
    midiout.open_virtual_port("VirtualRtMidiPort")

button_old = False
note = 42 # random

while True:
    line = sys.stdin.readline()
    if not line:
        break
    # we receive :
    #  1) a boolean for the touch (0 or 1)
    #  2) a signed char for the tilt (-128 to 127)
    match = re.match(r'([0-1]),(-?[0-9]{1,3})', line)
    if not match:
        continue

    # the accelerometer gives 64 for 1g but we want 127:
    velocity = min(127, abs(int(match.group(2))) * 2)
    button_new = int(match.group(1))

    if button_new and not button_old:           # button rising edge
        print "\nnote on, velocity:", velocity
        note_on(0, note, velocity)
    elif not button_new and button_old:         # button falling edge
        print "note off"
        note_off(0, note)

    button_old = button_new

del midiout

