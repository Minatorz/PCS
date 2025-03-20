#ifndef _PRESET_H
#define _PRESET_H

#include "config.h"
#include <Preferences.h>

extern Preferences preferences;

// The PresetManager class handles saving and loading presets to NVS.
class ps {
public:
  // Save a preset to the device storage.
  static void savePresetToDevice(int presetNumber, const Preset &preset);
  
  // Load a preset from the device storage.
  static Preset loadPresetFromDevice(int presetNumber);

  static void deletePresetFromDevice(int presetNumber);

  static void initPreferences();
};

#endif // _PRESET_H