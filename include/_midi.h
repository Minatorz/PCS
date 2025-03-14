#ifndef MIDI_H
#define MIDI_H

#include <USB.h>
#include <ESPNATIVEUSBMIDI.h>
#include <MIDI.h>
#include "config.h"


class Midi {
    public:
        // Initialize MIDI, set up SysEx callback.
        static void begin();
        // Callback for handling incoming SysEx messages.
        static void handleSysEx(byte *data, unsigned length);
        // Sends a SysEx message for the specified song index.
        static void Play(byte songIndex);

        static void Scan(); 
        // Sends a SysEx message to stop playback.
        static void Stop();  
        // Sends a SysEx ping message.
        static void Ping();

        static void read();

        static void volume(byte vol);
    };
    
#endif 