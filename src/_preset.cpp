#include "_preset.h"
#include <Arduino.h>

Preferences preferences;

void ps::savePresetToDevice(int presetNumber, const Preset &preset) {
  preferences.begin("Setlists", false);
  
  preferences.putString(("p" + String(presetNumber) + "_name").c_str(), preset.name);
  preferences.putString(("p" + String(presetNumber) + "_proj").c_str(), preset.data.projectName);
  preferences.putInt(("p" + String(presetNumber) + "_count").c_str(), preset.data.songCount);

  for (int i = 0; i < preset.data.songCount; i++) {
    String songKey    = "p" + String(presetNumber) + "_song"  + String(i);
    String indexKey   = "p" + String(presetNumber) + "_index" + String(i);
    String timeKey    = "p" + String(presetNumber) + "_time"  + String(i);
    String cindexKey  = "p" + String(presetNumber) + "_cindex"+ String(i);

    preferences.putString(songKey.c_str(), preset.data.songs[i].songName);
    preferences.putInt(indexKey.c_str(), preset.data.songs[i].songIndex);
    preferences.putInt(cindexKey.c_str(), preset.data.songs[i].changedIndex);
    preferences.putFloat(timeKey.c_str(), preset.data.songs[i].locatorTime);
  }
  
  preferences.end();
}

Preset ps::loadPresetFromDevice(int presetNumber) {
  preferences.begin("Setlists", true);
  
  Preset preset;
  String loadedName = preferences.getString(("p" + String(presetNumber) + "_name").c_str(), "No Preset");
  String projName   = preferences.getString(("p" + String(presetNumber) + "_proj").c_str(), "");
  int songCount     = preferences.getInt(("p" + String(presetNumber) + "_count").c_str(), 0);
  
  strlcpy(preset.name, loadedName.c_str(), sizeof(preset.name));
  strlcpy(preset.data.projectName, projName.c_str(), sizeof(preset.data.projectName));
  preset.data.songCount = songCount;

  for (int i = 0; i < songCount; i++) {
    String songKey   = "p" + String(presetNumber) + "_song"  + String(i);
    String indexKey  = "p" + String(presetNumber) + "_index" + String(i);
    String timeKey   = "p" + String(presetNumber) + "_time"  + String(i);
    String cindexKey = "p" + String(presetNumber) + "_cindex"+ String(i);

    String sName  = preferences.getString(songKey.c_str(), "Unknown Song");
    int sIndex    = preferences.getInt(indexKey.c_str(), i);
    int cIndex    = preferences.getInt(cindexKey.c_str(), i);
    float sTime   = preferences.getFloat(timeKey.c_str(), 0.0);

    strlcpy(preset.data.songs[i].songName, sName.c_str(), sizeof(preset.data.songs[i].songName));
    preset.data.songs[i].songIndex    = sIndex;
    preset.data.songs[i].changedIndex = cIndex;
    preset.data.songs[i].locatorTime  = sTime;
  }
  
  preferences.end();
  return preset;
}

void ps::initPreferences() {
  if (!preferences.begin("Setlists", false)) {
      Serial.println("Error initializing preferences");
  } else {
      Serial.println("Preferences initialized successfully.");
      // Optionally, clear preferences if you need to reset defaults.
      // preferences.clear();
  }
  preferences.end();
}