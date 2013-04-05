#include "RtMidi.h"
#include <stdlib.h>
#include <iostream>
using namespace std;


int main(int argc, char* argv[])
{
    static char buf[1024];
    int sample;
    bool touch, touchOld = 0;

    RtMidiOut *midiout = new RtMidiOut();
    vector<unsigned char> message;
    message.push_back(0); message.push_back(0); message.push_back(0); // TODO improve

    // Check available ports.
    unsigned int nPorts = midiout->getPortCount();
    if (nPorts == 0) {
        midiout->openVirtualPort("NoonMIDI");
        cout << "No ports available, creating one: NoonMIDI" << endl;
    } else {
        midiout->openPort(0); // Open first available port.
        cout << "Using port:" << midiout->getPortName(0) << endl;
    }

    for(;;) {
        cin.getline(buf, 1024);

        if (*buf) {
            char *start=buf, *end;
            touch  = int(strtol(start, &end, 10));
            start  = end+1;
            sample = int(strtol(start, &end, 10));

            // cout << "T:" << touch << " Z:" << sample << endl;

            ///////////////////////////////////////////////////////////////////
            if (touch) {
                if (touchOld != touch) { // just started to touch
                    message[0] = 0x90 | 0x3;    // on: channel 3
                    message[1] = 42;            // random note
                    message[2] = sample;        // velocity
                    midiout->sendMessage(&message);
                    cout << " ! touch rising edge, velocity: " << sample << endl;
                }
                // log when press
//                cout << "Z:" << sample << endl;
            } else { ///////////////////////////////////////////////////////////
                if (touchOld != touch) { // just released the touch
                    cout << " ! touch falling edge !" << endl;
                    message[0] = 0x80 | 0x3;    // off: channel 3
                    message[1] = 42;            // random note
                    message[2] = 0;             // velocity 0
                    midiout->sendMessage(&message);
                }
                // ...
            }
            touchOld = touch;
            ///////////////////////////////////////////////////////////////////
        }
    }

    delete midiout;
    return 0;
}

