#include "_input.h"

Input::Input(XPT2046_Touchscreen &ts, Adafruit_ILI9341 &tft, UI *uiInstance)
  : ts(ts), tft(tft), ui(uiInstance)
{
}

// Private helper: if the screen is touched, map raw coordinates to display coordinates.
bool Input::touchHandler(int16_t &x, int16_t &y) {
  if (ts.touched()) {
    TS_Point point = ts.getPoint();
    // Map raw coordinates to display coordinates.
    // Adjust calibration values as needed.
    x = map(point.x, 300, 3800, 0, tft.width());
    y = map(point.y, 300, 3800, 0, tft.height());
    return true;
  }
  return false;
}

// Private helper: checks if (x,y) is inside the area defined by (areaX, areaY, width, height).
bool Input::isTouch(int16_t x, int16_t y, int16_t areaX, int16_t areaY, int16_t width, int16_t height) {
  return (x >= areaX && x < (areaX + width) && y >= areaY && y < (areaY + height));
}

// Public function: processes a touch event (with debouncing) and updates UI state accordingly.
void Input::handleTouch() {
  static bool wasTouched = false;
  static unsigned long lastTouchTime = 0;
  const unsigned long DEBOUNCE_MS = 200;

  int16_t tx, ty;
  bool currentlyTouched = touchHandler(tx, ty);
//   Serial.println(currentlyTouched);

  if (!wasTouched && currentlyTouched) {
    unsigned long now = millis();
    if (now - lastTouchTime > DEBOUNCE_MS) {
      lastTouchTime = now;
      ScreenState cs = ui->getScreenState();
      switch (cs) {
        case ScreenState::HOME:
          if (isTouch(tx, ty, BOX1_X, BOX1_Y, BOX_WIDTH, BOX_HEIGHT)) {
            ui->setScreenState(ScreenState::MENU1);
            Serial.println("menu1");
          }
          else if (isTouch(tx, ty, BOX2_X, BOX2_Y, BOX_WIDTH, BOX_HEIGHT))
            ui->setScreenState(ScreenState::MENU2);
          break;
        case ScreenState::MENU1:
          for (int i = 0; i < NUM_MENU_ITEMS; i++) {
            int y = 60 + i * 30;
            if (isTouch(tx, ty, 10, y, 220, 30)) {
              currentMenuItem = i;  // Assuming currentMenuItem is global or accessible through ui.
              switch (currentMenuItem) {
                case 0:
                  ui->setScreenState(ScreenState::NEW_SETLIST);
                  newSetlistScanned = false;
                  songsReady = false;
                  memset(selectedSongs, 0, sizeof(selectedSongs));
                  selectedTrackCount = 0;
                  scrollOffset = 0;
                  currentMenuItem = 0;
                  break;
                case 1:
                  ui->setScreenState(ScreenState::SELECT_SETLIST);
                  break;
                case 2:
                  ui->setScreenState(ScreenState::EDIT_SETLIST);
                  scrollOffset = 0;
                  break;
                case 3:
                  ui->setScreenState(ScreenState::HOME);
                  break;
                case 4:
                  ui->setScreenState(ScreenState::HOME);
                  break;
              }
            }
          }
          break;
        case ScreenState::NEW_SETLIST:
          if (isTouch(tx, ty, BACK_BUTTON_X, BACK_BUTTON_Y, BACK_BUTTON_WIDTH, BACK_BUTTON_HEIGHT))
            ui->setScreenState(ScreenState::MENU1);
          else if (isTouch(tx, ty, NEXT_BUTTON_X, NEXT_BUTTON_Y, NEXT_BUTTON_WIDTH, NEXT_BUTTON_HEIGHT)) {
            ui->setScreenState(ScreenState::EDIT_SETLIST);
            scrollOffset = 0;
          } else {
            int touchedIndex = (ty - 60) / 30 + scrollOffset;
            if (touchedIndex >= 0 && touchedIndex < currentProject.songCount) {
              currentMenuItem = touchedIndex;
              selectedSongs[touchedIndex] = !selectedSongs[touchedIndex];
              selectedTrackCount += selectedSongs[touchedIndex] ? 1 : -1;
              ui->newSetlistScreen();  // Call UI's newSetlistScreen method.
            }
          }
          break;
        // ... Add additional cases for other screen states as needed ...
        default:
          break;
      }
      ui->updateScreen(); // Refresh the screen after processing the touch.
    }
    wasTouched = true;
  } else if (!currentlyTouched) {
    wasTouched = false;
  }


  // Optionally, add additional input handling (e.g., rotary encoder) here.
}