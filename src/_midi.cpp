#include "_midi.h"

ESPNATIVEUSBMIDI usbmidi;
MIDI_CREATE_INSTANCE(ESPNATIVEUSBMIDI, usbmidi, MIDI)

void Midi::begin() {
    USB.productName("AbletonThesis");
    if (!USB.begin()) {
    Serial.println(F("USB Initialization Failed!"));
    while (1);
    }
    MIDI.begin(MIDI_CHANNEL_OMNI);
    MIDI.setHandleSystemExclusive(handleSysEx);
    Serial.println(F("MIDI initialized."));
}

void Midi::handleSysEx(byte *data, unsigned length) {
    if (length < 6) {
        Serial.println(F("⚠️ SysEx message too short. Ignoring."));
        return;
    }
    // Validate Manufacturer ID: expecting 0x00, 0x01, 0x61 (example).
    if (data[1] != 0x00 || data[2] != 0x01 || data[3] != 0x61) {
        Serial.println(F("Invalid Manufacturer ID."));
        return;
    }

    if (data[4] == 0x00 && data[5] == 0x7F) {
        Serial.println(F("End signal received. Song list complete."));
        Serial.print(F("Final total songs received: "));
        Serial.println(currentProject.songCount);
        songsReady = true;
        return;
    }

    if (data[4] == 0x02) {
        memset(currentProject.projectName, 0, sizeof(currentProject.projectName));
        int i = 5, j = 0;
        while (i < (int)length - 1 && data[i] != 0x00 && j < MAX_SONG_NAME_LEN) {
          currentProject.projectName[j++] = (char)data[i++];
        }
        currentProject.projectName[j] = '\0';
        Serial.print(F("Project Name Received: "));
        Serial.println(currentProject.projectName);
        return;
    }
    
    // Example: handle a ping reply (if data[4] == 0x31).
    if (data[4] == 0x31) {
        // Flash the LED to indicate a ping was received
        lastPingReplyTime = millis();
        // Serial.println(lastPingReplyTime);
        return;
    }

    if (currentProject.songCount == 0) {
        memset(&currentProject.songs[0], 0, sizeof(currentProject.songs));
        currentProject.songCount = 0;
      }
      if (currentProject.songCount < MAX_SONGS) {
        Serial.printf("Processing Song %d...\n", currentProject.songCount + 1);
        if (currentProject.songs[currentProject.songCount].getInfo(data, length)) {
          Serial.printf("Song %d added: %s\n",
                        currentProject.songCount + 1,
                        currentProject.songs[currentProject.songCount].songName);
          currentProject.songCount++;
        } else {
          Serial.println(F("Failed to parse song info."));
        }
      } else {
        Serial.println(F("Song list is full. Cannot add more songs."));
      }
    
    // Process other SysEx commands (project name, song data, etc.)
    Serial.println(F("Processing SysEx message..."));
    // (Insert additional parsing logic here.)
}

void Midi::Play(byte songIndex) {
    byte sysexMessage[7] = { 0xF0, 0x00, 0x01, 0x61, 0x01, songIndex, 0xF7 };
    MIDI.sendSysEx(sizeof(sysexMessage), sysexMessage, true);
    Serial.print(F("Sent SysEx for Song Index: "));
    Serial.println(songIndex);
}

void Midi::Scan() {
    byte sysexMessage[] = {0xF0, 0x00, 0x01, 0x61, 0x10, 0xF7};
    MIDI.sendSysEx(sizeof(sysexMessage), sysexMessage, true);
    Serial.println(F("Sent SysEx to Notify Ableton"));
}

void Midi::Stop() {
    byte sysexStopMessage[] = { 0xF0, 0x00, 0x01, 0x61, 0x11, 0xF7 };
    MIDI.sendSysEx(sizeof(sysexStopMessage), sysexStopMessage, true);
    Serial.println(F("Stop SysEx Command Sent"));
}

void Midi::Ping() {
    byte sysexPingMessage[] = { 0xF0, 0x00, 0x01, 0x61, 0x30, 0xF7 };
    MIDI.sendSysEx(sizeof(sysexPingMessage), sysexPingMessage, true);
}

void Midi::read() {
    MIDI.read();
}

void Midi::volume(byte vol) {
    MIDI.sendControlChange(7, vol, 1);
}