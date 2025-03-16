#include "_input.h"

Input::Input(XPT2046_Touchscreen &ts, Adafruit_ILI9341 &tft, UI *uiInstance)
  : ts(ts), tft(tft), ui(uiInstance)
{
}



void Input::handleTouch() {
    static bool wasTouched = false;
    static unsigned long lastTouchTime = 0;
    const unsigned long DEBOUNCE_MS = 200;

    int16_t tx, ty;
    bool currentlyTouched = ui->getTouchCoordinates(tx, ty);

        // if (currentlyTouched) {
        //     ui->getDisplay().fillCircle(tx, ty, 3, ILI9341_RED);
        // }

    if (!wasTouched && currentlyTouched) {
        unsigned long now = millis();
        if (now - lastTouchTime > DEBOUNCE_MS) {
        lastTouchTime = now;
        ScreenState cs = ui->getScreenState();
        switch (cs) {
            case ScreenState::HOME:
            if (ui->isTouch(tx, ty, BOX1_X, BOX1_Y, BOX_WIDTH, BOX_HEIGHT)) {
                ui->setScreenState(ScreenState::MENU1);
            }
            else if (ui->isTouch(tx, ty, BOX2_X, BOX2_Y, BOX_WIDTH, BOX_HEIGHT))
                ui->setScreenState(ScreenState::MENU2);
            break;
            case ScreenState::MENU1:
            for (int i = 0; i < NUM_MENU_ITEMS; i++) {
                int y = 60 + i * 30;
                if (ui->isTouch(tx, ty, 10, y, 220, 30)) {
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
            if (ui->isTouch(tx, ty, BACK_BUTTON_X, BACK_BUTTON_Y, BACK_BUTTON_WIDTH, BACK_BUTTON_HEIGHT))
                ui->setScreenState(ScreenState::MENU1);
            else if (ui->isTouch(tx, ty, NEXT_BUTTON_X, NEXT_BUTTON_Y, NEXT_BUTTON_WIDTH, NEXT_BUTTON_HEIGHT)) {
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
            case ScreenState::EDIT_SETLIST:
            if (ui->isTouch(tx, ty, BACK_BUTTON_X, BACK_BUTTON_Y, BACK_BUTTON_WIDTH, BACK_BUTTON_HEIGHT))
                ui->setScreenState(ScreenState::NEW_SETLIST);
            else if (ui->isTouch(tx, ty, SAVE_BUTTON_X, SAVE_BUTTON_Y, SAVE_BUTTON_WIDTH, SAVE_BUTTON_HEIGHT))
                ui->setScreenState(ScreenState::SELECT_SETLIST);
            break;
            case ScreenState::SELECT_SETLIST:
            if (ui->isTouch(tx, ty, BACK_BUTTON_X, BACK_BUTTON_Y, BACK_BUTTON_WIDTH, BACK_BUTTON_HEIGHT))
                ui->setScreenState(ScreenState::MENU1);
            else if (ui->isTouch(tx, ty, SAVE_BUTTON_X, SAVE_BUTTON_Y, SAVE_BUTTON_WIDTH, SAVE_BUTTON_HEIGHT))
                ui->setScreenState(ScreenState::SAVE_SETLIST);
            else {
                for (int i = 0; i < MAX_PRESETS; i++) {
                int y = 50 + i * 30;
                if (ui->isTouch(tx, ty, 10, y, tft.width() - 20, 30)) {
                    selectedPresetSlot = i + 1;
                    presetChanged = true;
                    Serial.print(F("✅ Selected Setlist Slot: "));
                    Serial.println(selectedPresetSlot);
                    if (ui->getPreviousScreenState() == ScreenState::MENU1) {
                    // loadedPreset = PresetManager::loadPresetFromDevice(selectedPresetSlot);
                    ui->setScreenState(ScreenState::HOME);
                    } else {
                    ui->setScreenState(ScreenState::SAVE_SETLIST);
                    }
                    return;
                }
                }
            }
            break;
            case ScreenState::SAVE_SETLIST:
            if (ui->isTouch(tx, ty, BACK_BUTTON_X, BACK_BUTTON_Y, BACK_BUTTON_WIDTH, BACK_BUTTON_HEIGHT))
                ui->setScreenState(ScreenState::EDIT_SETLIST);
            else if (ui->isTouch(tx, ty, SAVE_BUTTON_X, SAVE_BUTTON_Y, SAVE_BUTTON_WIDTH, SAVE_BUTTON_HEIGHT)) {
                strcpy(loadedPreset.name, setlistName);
                strcpy(loadedPreset.data.projectName, selectedProject.projectName);
                loadedPreset.data.songCount = selectedTrackCount;
                for (int i = 0; i < selectedTrackCount; i++) {
                loadedPreset.data.songs[i] = selectedProject.songs[i];
                loadedPreset.data.songs[i].songIndex = selectedProject.songs[i].songIndex;
                loadedPreset.data.songs[i].changedIndex = selectedProject.songs[i].changedIndex;
                }
                // PresetManager::savePresetToDevice(selectedPresetSlot, loadedPreset);
                Serial.print(F("✅ Setlist Saved in: "));
                Serial.println(selectedPresetSlot);
                // loadedPreset = PresetManager::loadPresetFromDevice(selectedPresetSlot);
                isReorderedSongsInitialized = false;
                ui->setScreenState(ScreenState::HOME);
            }
            else {
                int keyWidth = tft.width() / 10;
                int keyHeight = 30;
                int keyStartY = 88;
                for (int row = 0; row < 4; row++) {
                for (int col = 0; col < 10; col++) {
                    int keyX = col * keyWidth, keyY = keyStartY + row * keyHeight;
                    if (ui->isTouch(tx, ty, keyX, keyY, keyWidth, keyHeight)) {
                    char keyChar = (isShifted && row > 0) ? toupper(keys[row][col]) : keys[row][col];
                    size_t curLen = strlen(setlistName);
                    if (curLen < MAX_SONG_NAME_LEN) {
                        setlistName[curLen] = keyChar;
                        setlistName[curLen + 1] = '\0';
                    }
                    ui->updateScreen();
                    return;
                    }
                }
                }
                // Handle special keys: SHIFT, SPACE, BACKSPACE
                if (ui->isTouch(tx, ty, 0, keyStartY + 4 * keyHeight, 2 * keyWidth, keyHeight)) {
                isShifted = !isShifted;
                ui->updateScreen();
                return;
                }
                if (ui->isTouch(tx, ty, 2 * keyWidth, keyStartY + 4 * keyHeight, 6 * keyWidth, keyHeight)) {
                size_t curLen = strlen(setlistName);
                if (curLen < MAX_SONG_NAME_LEN) {
                    setlistName[curLen] = ' ';
                    setlistName[curLen + 1] = '\0';
                }
                ui->updateScreen();
                return;
                }
                if (ui->isTouch(tx, ty, 8 * keyWidth, keyStartY + 4 * keyHeight, 2 * keyWidth, keyHeight)) {
                size_t curLen = strlen(setlistName);
                if (curLen > 0) {
                    setlistName[curLen - 1] = '\0';
                }
                ui->updateScreen();
                return;
                }
            }
            break;
            case ScreenState::MENU2:
            for (int i = 0; i < NUM_MENU2_ITEMS; i++) {
                int y = 60 + i * 30;
                if (ui->isTouch(tx, ty, 10, y, 220, 30)) {
                currentMenu2Item = i;
                switch (i) {
                    case 0:
                    ui->setScreenState(ScreenState::MENU2_WIFISETTINGS);
                    break;
                    case 1:
                    ui->setScreenState(ScreenState::DEVICE_PROPERTIES);
                    break;
                    case 2:
                    ui->setScreenState(ScreenState::HOME);
                    break;
                }
                return;
                }
            }
            break;
            case ScreenState::MENU2_WIFISETTINGS:
            for (int i = 0; i < NUM_WIFI_MENU_ITEMS; i++) {
                int y = 60 + i * 30;
                if (ui->isTouch(tx, ty, 10, y, 220, 30)) {
                currentWifiMenuItem = i;
                switch (i) {
                    case 0:
                    wifiScanInProgress = false;
                    wifiScanDone = false;
                    wifiCount = 0;
                    ui->setScreenState(ScreenState::MENU2_WIFICONNECT);
                    break;
                    case 1:
                    ui->setScreenState(ScreenState::WIFI_PROPERTIES);
                    break;
                    case 2:
                    ui->setScreenState(ScreenState::MENU2);
                }
                return;
                }
            }
            break;
            case ScreenState::MENU2_WIFICONNECT:
            if (ui->isTouch(tx, ty, BACK_BUTTON_X, BACK_BUTTON_Y, BACK_BUTTON_WIDTH, BACK_BUTTON_HEIGHT)) {
                ui->setScreenState(ScreenState::MENU2_WIFISETTINGS);
                return;
            }
            else if (ui->isTouch(tx, ty, NEXT_BUTTON_X, NEXT_BUTTON_Y, NEXT_BUTTON_WIDTH, NEXT_BUTTON_HEIGHT)) {
                WiFi.scanDelete();
                WiFi.disconnect(true);
                delay(50);
                WiFi.scanNetworks(true);
                wifiScanInProgress = true;
                wifiScanDone = false;
                ui->updateScreen();
                return;
            }
            break;
            case ScreenState::MENU2_WIFIPASS:
            if (ui->isTouch(tx, ty, BACK_BUTTON_X, BACK_BUTTON_Y, BACK_BUTTON_WIDTH, BACK_BUTTON_HEIGHT)) {
                ui->setScreenState(ScreenState::MENU2_WIFICONNECT);
            } else if (ui->isTouch(tx, ty, SAVE_BUTTON_X, SAVE_BUTTON_Y, SAVE_BUTTON_WIDTH, SAVE_BUTTON_HEIGHT)) {
                Serial.println("User clicked OK to confirm Wi-Fi password: ");
                Serial.println(wifiPassword);
                ui->setScreenState(ScreenState::MENU2_WIFICONFIRM);
            } else {
                int keyWidth = tft.width() / 10;
                int keyHeight = 30;
                int keyStartY = 88;
                for (int row = 0; row < 4; row++) {
                for (int col = 0; col < 10; col++) {
                    int keyX = col * keyWidth, keyY = keyStartY + row * keyHeight;
                    if (ui->isTouch(tx, ty, keyX, keyY, keyWidth, keyHeight)) {
                    char keyChar = (isShifted && row > 0) ? toupper(keys[row][col]) : keys[row][col];
                    size_t curLen = strlen(wifiPassword);
                    if (curLen < MAX_WIFI_PASS_LEN) {
                        wifiPassword[curLen] = keyChar;
                        wifiPassword[curLen + 1] = '\0';
                    }
                    ui->updateScreen();
                    return;
                    }
                }
                }
                if (ui->isTouch(tx, ty, 0, keyStartY + 4 * keyHeight, 2 * keyWidth, keyHeight)) {
                isShifted = !isShifted;
                ui->updateScreen();
                return;
                }
                if (ui->isTouch(tx, ty, 2 * keyWidth, keyStartY + 4 * keyHeight, 6 * keyWidth, keyHeight)) {
                size_t curLen = strlen(wifiPassword);
                if (curLen < MAX_WIFI_PASS_LEN) {
                    wifiPassword[curLen] = ' ';
                    wifiPassword[curLen + 1] = '\0';
                }
                ui->updateScreen();
                return;
                }
                if (ui->isTouch(tx, ty, 8 * keyWidth, keyStartY + 4 * keyHeight, 2 * keyWidth, keyHeight)) {
                size_t curLen = strlen(wifiPassword);
                if (curLen > 0) {
                    wifiPassword[curLen - 1] = '\0';
                }
                ui->updateScreen();
                return;
                }
            }
            break;
            case ScreenState::MENU2_WIFICONFIRM:
            if (ui->isTouch(tx, ty, BACK_BUTTON_X, BACK_BUTTON_Y, BACK_BUTTON_WIDTH, BACK_BUTTON_HEIGHT)) {
                ui->setScreenState(ScreenState::MENU2_WIFIPASS);
                return;
            }
            if (ui->isTouch(tx, ty, SAVE_BUTTON_X, SAVE_BUTTON_Y, SAVE_BUTTON_WIDTH, SAVE_BUTTON_HEIGHT)) {
                Serial.printf("Connecting to SSID: %s with password: %s\n",
                            selectedSSID.c_str(), wifiPassword);
                WiFi.begin(selectedSSID.c_str(), wifiPassword);
                ui->setScreenState(ScreenState::MENU2_WIFICONNECTING);
                return;
            }
            ui->updateScreen();
            break;
            case ScreenState::DEVICE_PROPERTIES:
            if (ui->isTouch(tx, ty, BACK_BUTTON_X, BACK_BUTTON_Y, BACK_BUTTON_WIDTH, BACK_BUTTON_HEIGHT)) {
                ui->setScreenState(ScreenState::MENU2);  // Return to the previous menu
            }
            break;
            case ScreenState::WIFI_PROPERTIES:
            if (ui->isTouch(tx, ty, BACK_BUTTON_X, BACK_BUTTON_Y, BACK_BUTTON_WIDTH, BACK_BUTTON_HEIGHT)) {
                ui->setScreenState(ScreenState::MENU2_WIFISETTINGS);
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
}

void Input::handleRotary() {
    // Read the rotary encoder pins.
    bool clkState = digitalRead(ENC_CLK);
    bool dtState  = digitalRead(ENC_DT);
    static bool lastClkState = clkState;
    ScreenState cs = ui->getScreenState();
    
    if (clkState != encoderLastState && clkState == LOW) {
            switch (cs) {
              case ScreenState::MENU1:
                if (dtState == clkState)
                  currentMenuItem = (currentMenuItem + 1) % NUM_MENU_ITEMS;
                else
                  currentMenuItem = (currentMenuItem - 1 + NUM_MENU_ITEMS) % NUM_MENU_ITEMS;
                ui->updateScreen();
                break;
              case ScreenState::NEW_SETLIST:
                if (currentProject.songCount > 0) {
                  int itemsPerPage = (tft.height() - 60) / 30 - 1;
                  if (dtState == clkState)
                    currentMenuItem = min(currentMenuItem + 1, currentProject.songCount - 1);
                  else
                    currentMenuItem = max(currentMenuItem - 1, 0);
                  if (currentMenuItem < scrollOffset)
                    scrollOffset = currentMenuItem;
                  else if (currentMenuItem >= scrollOffset + itemsPerPage)
                    scrollOffset = currentMenuItem - itemsPerPage + 1;
                ui->updateScreen();
                }
                break;
              case ScreenState::EDIT_SETLIST: {
                int itemsPerPage = (tft.height() - 60) / 30 - 1;
                if (isReordering) {
                  if (dtState == clkState && reorderTarget < selectedProject.songCount - 1) {
                    SongInfo temp = selectedProject.songs[reorderTarget];
                    selectedProject.songs[reorderTarget] = selectedProject.songs[reorderTarget + 1];
                    selectedProject.songs[reorderTarget + 1] = temp;
                    reorderTarget++;
                  } else if (dtState != clkState && reorderTarget > 0) {
                    SongInfo temp = selectedProject.songs[reorderTarget];
                    selectedProject.songs[reorderTarget] = selectedProject.songs[reorderTarget - 1];
                    selectedProject.songs[reorderTarget - 1] = temp;
                    reorderTarget--;
                  }
                  if (reorderTarget < scrollOffset)
                    scrollOffset = reorderTarget;
                  else if (reorderTarget >= scrollOffset + itemsPerPage)
                    scrollOffset = reorderTarget - itemsPerPage + 1;
                    ui->updateScreen();;
                } else {
                  if (selectedProject.songCount > 0) {
                    if (dtState == clkState)
                      currentMenuItem = (currentMenuItem + 1) % selectedProject.songCount;
                    else
                      currentMenuItem = (currentMenuItem - 1 + selectedProject.songCount) % selectedProject.songCount;
                    if (currentMenuItem < scrollOffset)
                      scrollOffset = currentMenuItem;
                    else if (currentMenuItem >= scrollOffset + itemsPerPage)
                      scrollOffset = currentMenuItem - itemsPerPage + 1;
                  }
                  ui->updateScreen();;
                }
              } break;
              case ScreenState::SELECT_SETLIST:
                if (dtState == clkState)
                  currentMenuItem = (currentMenuItem + 1) % MAX_PRESETS;
                else
                  currentMenuItem = (currentMenuItem - 1 + MAX_PRESETS) % MAX_PRESETS;
                  ui->updateScreen();;
                break;
              case ScreenState::HOME:
                if (dtState == clkState)
                  currentMenuSelection = (currentMenuSelection == MENU1_SELECTED) ? MENU2_SELECTED : MENU1_SELECTED;
                needsRedraw = true;
                break;
              case ScreenState::MENU2:
                if (dtState == clkState)
                  currentMenu2Item = (currentMenu2Item + 1) % NUM_MENU2_ITEMS;
                else
                  currentMenu2Item = (currentMenu2Item - 1 + NUM_MENU2_ITEMS) % NUM_MENU2_ITEMS;
                  ui->updateScreen();;
                break;
              case ScreenState::MENU2_WIFISETTINGS:
                if (dtState == clkState)
                  currentWifiMenuItem = (currentWifiMenuItem + 1) % NUM_WIFI_MENU_ITEMS;
                else
                  currentWifiMenuItem = (currentWifiMenuItem - 1 + NUM_WIFI_MENU_ITEMS) % NUM_WIFI_MENU_ITEMS;
                  ui->updateScreen();;
                break;
              case ScreenState::MENU2_WIFICONNECT: {
                const int itemsPerPage = 5;
                if (dtState == clkState)
                  wifiCurrentItem++;
                else
                  wifiCurrentItem--;
                if (wifiCurrentItem >= wifiCount)
                  wifiCurrentItem = 0;
                else if (wifiCurrentItem < 0)
                  wifiCurrentItem = wifiCount - 1;
                if (wifiCurrentItem < wifiScrollOffset)
                  wifiScrollOffset = wifiCurrentItem;
                else if (wifiCurrentItem >= wifiScrollOffset + itemsPerPage)
                  wifiScrollOffset = wifiCurrentItem - (itemsPerPage - 1);
                wifiScrollOffset = constrain(wifiScrollOffset, 0, max(0, wifiCount - itemsPerPage));
                ui->updateScreen();;
              } break;
              default:
                break;
            }
          }
          encoderLastState = clkState;

    // Handle rotary encoder button.
    bool btnState = digitalRead(ENC_SW);
    if (btnState == LOW && encoderButtonState == HIGH) {
        switch (cs) {
        case ScreenState::HOME:
            // if (currentMenuSelection == MENU1_SELECTED)
            //   setScreenState(ScreenState::MENU1);
            // else
            //   setScreenState(ScreenState::MENU2);
            break;
        case ScreenState::NEW_SETLIST:
            if (currentProject.songCount > 0) {
            selectedSongs[currentMenuItem] = !selectedSongs[currentMenuItem];
            selectedTrackCount += selectedSongs[currentMenuItem] ? 1 : -1;
            ui->updateScreen();
            }
            break;
        case ScreenState::EDIT_SETLIST:
            if (isReordering) {
            isReordering = false;
            currentMenuItem = reorderTarget;
            ui->updateScreen();
            } else {
            reorderTarget = currentMenuItem;
            isReordering = true;
            ui->updateScreen();
            }
            break;
        case ScreenState::SELECT_SETLIST:
            selectedPresetSlot = currentMenuItem + 1;
            Serial.print(F("✅ Selected Setlist Slot via Encoder: "));
            Serial.println(selectedPresetSlot);
            ui->setScreenState(ScreenState::SAVE_SETLIST);
            break;
        case ScreenState::MENU2:
            if (currentMenu2Item == 0) {
                ui->setScreenState(ScreenState::MENU2_WIFISETTINGS);
            } else if (currentMenu2Item == 1) {
            // Device Settings placeholder
            } else if (currentMenu2Item == 2) {
                ui->setScreenState(ScreenState::HOME);
            }
            break;
        case ScreenState::MENU2_WIFISETTINGS:
            if (currentWifiMenuItem == 0) {
            wifiScanInProgress = false;
            wifiScanDone = false;
            wifiCount = 0;
            ui->setScreenState(ScreenState::MENU2_WIFICONNECT);
            } else if (currentWifiMenuItem == 1) {
                ui->setScreenState(ScreenState::MENU2);
            }
            break;
        case ScreenState::MENU2_WIFICONNECT:
            if (wifiCount > 0) {
            selectedSSID = wifiSSIDs[wifiCurrentItem];
            memset(wifiPassword, 0, sizeof(wifiPassword));
            ui->setScreenState(ScreenState::MENU2_WIFIPASS);
            }
            break;
        default:
            break;
        }
    }
    encoderButtonState = btnState;
}

void Input::StartButton() {
    static bool debouncedState = false;
    static bool lastRawState = HIGH; // Assuming buttons are INPUT_PULLUP.
    static unsigned long lastDebounceTime = 0;
    const unsigned long debounceDelay = 20;

    bool currentRawState = (digitalRead(BTN_START) == LOW);
    if (currentRawState != lastRawState) {
        lastDebounceTime = millis();
        lastRawState = currentRawState;
    }
    if ((millis() - lastDebounceTime) > debounceDelay) {
        if (currentRawState != debouncedState) {
            debouncedState = currentRawState;
            if (debouncedState) {
                isPlaying = true;
                Midi::Play(loadedPreset.data.songs[currentTrack].songIndex);
                needsRedraw = true;
            }
        }
    }
}

void Input::StopButton() {
    static bool debouncedState = false;
    static bool lastRawState = false;
    static unsigned long lastDebounceTime = 0;
    const unsigned long debounceDelay = 20;

    bool currentRawState = (digitalRead(BTN_STOP) == LOW);
    if (currentRawState != lastRawState) {
        lastDebounceTime = millis();
        lastRawState = currentRawState;
    }
    if ((millis() - lastDebounceTime) > debounceDelay) {
        if (currentRawState != debouncedState) {
            debouncedState = currentRawState;
            if (debouncedState) {
                Midi::Stop();
                isPlaying = false;
                needsRedraw = true;
            }
        }
    }
}

void Input::LeftButton() {
    static bool lastLeftState = HIGH;
    static unsigned long lastPressTime = 0;
    const unsigned long debounceDelay = 200; // milliseconds

    bool btnLeftState = digitalRead(BTN_LEFT);
    
    // Check for a transition from not-pressed (HIGH) to pressed (LOW)
    if (lastLeftState == HIGH && btnLeftState == LOW) {
        if (millis() - lastPressTime > debounceDelay) {
            // Trigger the event only once per press.
            if (selectedPresetSlot != -1 && totalTracks > 0) {
                currentTrack = (currentTrack - 1 + totalTracks) % totalTracks;
                Serial.println(F("Moved to previous track"));
            } else {
                Serial.println(F("No tracks available to navigate (left)."));
            }
            lastPressTime = millis();
        }
    }
    lastLeftState = btnLeftState;
}

void Input::RightButton() {
    static bool lastRightState = HIGH;
    static unsigned long lastPressTime = 0;
    const unsigned long debounceDelay = 200; // milliseconds

    bool btnRightState = digitalRead(BTN_RIGHT);
    
    // Check for a transition from HIGH to LOW.
    if (lastRightState == HIGH && btnRightState == LOW) {
        if (millis() - lastPressTime > debounceDelay) {
            // Trigger the event only once per press.
            if (selectedPresetSlot != -1 && totalTracks > 0) {
                currentTrack = (currentTrack + 1) % totalTracks;
                Serial.println(F("Moved to next track"));
            } else {
                Serial.println(F("No tracks available to navigate (right)."));
            }
            lastPressTime = millis();
        }
    }
    lastRightState = btnRightState;
}

void Input::handleVolume() {
    static byte lastMidiVolume = 255;
    int potValue = analogRead(POT_VOL);
    byte midiVol = map(potValue, 0, 4095, 0, 127);
    
    if (midiVol != lastMidiVolume) {
      lastMidiVolume = midiVol;
      currentVolume = midiVol;
      // Call your MIDI volume function here; adjust as needed.
      Midi::volume(midiVol);
      if (ui->getScreenState() == ScreenState::HOME)
        ui->displayVolume(); // Ensure displayVolume() exists in your UI class.
    }
  }