#ifndef INPUT_H
#define INPUT_H

#include "_ui.h"      // Contains declaration of your UI class and ScreenState enum.
#include "config.h"   // For pin definitions and global variables like BOX1_X, NUM_MENU_ITEMS, etc.
#include "_midi.h"
#include "_webserver.h"

class Input {
public:
  // Constructor: requires references to the touchscreen, display, and a pointer to the UI instance.
  Input(XPT2046_Touchscreen &ts, Adafruit_ILI9341 &tft, UI *uiInstance);

  // Public interface to process touch input.
  void handleTouch();

  void handleRotary();

  void StartButton();

  void StopButton();

  void LeftButton();

  void RightButton();

  void handleVolume();

private:
  // References to the touchscreen and display.
  XPT2046_Touchscreen &ts;
  Adafruit_ILI9341 &tft;
  UI *ui;
};

#endif // INPUT_H
