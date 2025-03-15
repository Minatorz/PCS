#ifndef INPUT_H
#define INPUT_H

#include "_ui.h"      // Contains declaration of your UI class and ScreenState enum.
#include "config.h"   // For pin definitions and global variables like BOX1_X, NUM_MENU_ITEMS, etc.

class Input {
public:
  // Constructor: requires references to the touchscreen, display, and a pointer to the UI instance.
  Input(XPT2046_Touchscreen &ts, Adafruit_ILI9341 &tft, UI *uiInstance);

  // Public interface to process touch input.
  void handleTouch();

private:
  // Private helper: reads raw touch coordinates and maps them to display coordinates.
  bool touchHandler(int16_t &x, int16_t &y);
  
  // Private helper: returns true if (x, y) is within the specified rectangle.
  bool isTouch(int16_t x, int16_t y, int16_t areaX, int16_t areaY, int16_t width, int16_t height);

  // References to the touchscreen and display.
  XPT2046_Touchscreen &ts;
  Adafruit_ILI9341 &tft;
  
  // Pointer to the UI instance (to get and set screen state, call UI functions, etc.)
  UI *ui;
};

#endif // INPUT_H
