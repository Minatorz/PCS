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
                menu1Index = i;  // Assuming currentMenuItem is global or accessible through ui.
                switch (menu1Index) {
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
                    break;
                    case 3:
                    ui->setScreenState(ScreenState::SELECT_SETLIST);
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
                selectedSongs[touchedIndex] = !selectedSongs[touchedIndex];
                selectedTrackCount += selectedSongs[touchedIndex] ? 1 : -1;
                ui->drawSongList();
                ui->updateSelectedSongCount();
                }
            }
            break;
            case ScreenState::EDIT_SETLIST:
                if (ui->isTouch(tx, ty, BACK_BUTTON_X, BACK_BUTTON_Y, BACK_BUTTON_WIDTH, BACK_BUTTON_HEIGHT)) {
                    if (menu1Index == 2) {
                        ui->setScreenState(ScreenState::MENU1);
                    } else {
                        ui->setScreenState(ScreenState::NEW_SETLIST);
                    }
                }
                else if (ui->isTouch(tx, ty, SAVE_BUTTON_X, SAVE_BUTTON_Y, SAVE_BUTTON_WIDTH, SAVE_BUTTON_HEIGHT)) {
                    // Serial.println(currentMenuItem);
                    if (menu1Index == 2) {
                        ui->setScreenState(ScreenState::SAVE_SETLIST);
                    } else {
                        ui->setScreenState(ScreenState::SELECT_SETLIST);
                    }
                }
                else {
                    // Assume the song list area starts at y = 60 and each item has height = 30.
                    int touchedIndex = (ty - 60) / 30 + scrollOffset;
                    if (touchedIndex >= 0 && touchedIndex < selectedProject.songCount) {
                        // When a song is touched, set it as the reorder target and enable reordering mode.
                        reorderTarget = touchedIndex;
                        isReordering = true;
                        // Redraw the song list to show the newly selected item as highlighted (green).
                        ui->drawEditedSongList();
                        return;
                    }
                }
                break;
            case ScreenState::SELECT_SETLIST:
            if (ui->isTouch(tx, ty, BACK_BUTTON_X, BACK_BUTTON_Y, BACK_BUTTON_WIDTH, BACK_BUTTON_HEIGHT))
                if (menu1Index == 1 || menu1Index == 3) {
                    ui->setScreenState(ScreenState::MENU1);
                } else {
                    ui->setScreenState(ScreenState::EDIT_SETLIST);
                }
                    
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
                    if (menu1Index == 1) {
                    loadedPreset = ps::loadPresetFromDevice(selectedPresetSlot);
                        ui->setScreenState(ScreenState::HOME);
                    } else if (menu1Index == 3) {
                        ps::deletePresetFromDevice(selectedPresetSlot);
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
                  ps::savePresetToDevice(selectedPresetSlot, loadedPreset);
                  Serial.print(F("✅ Setlist Saved in: "));
                  Serial.println(selectedPresetSlot);
                  isReorderedSongsInitialized = false;
                  ui->setScreenState(ScreenState::HOME);
              }
              else if (ui->isTouch(tx, ty, CLEAR_BUTTON_X, CLEAR_BUTTON_Y, CLEAR_BUTTON_WIDTH, CLEAR_BUTTON_HEIGHT)) {
                  setlistName[0] = '\0';
                  ui->drawSetlistName();
                  return;
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
                              ui->drawSetlistName();
                              return;
                          }
                      }
                  }
                  // Handle special keys: SHIFT, SPACE, BACKSPACE
                  if (ui->isTouch(tx, ty, 0, keyStartY + 4 * keyHeight, 2 * keyWidth, keyHeight)) {
                      isShifted = !isShifted;
                      ui->drawSetlistName();
                      return;
                  }
                  if (ui->isTouch(tx, ty, 2 * keyWidth, keyStartY + 4 * keyHeight, 6 * keyWidth, keyHeight)) {
                      size_t curLen = strlen(setlistName);
                      if (curLen < MAX_SONG_NAME_LEN) {
                          setlistName[curLen] = ' ';
                          setlistName[curLen + 1] = '\0';
                      }
                      ui->drawSetlistName();
                      return;
                  }
                  if (ui->isTouch(tx, ty, 8 * keyWidth, keyStartY + 4 * keyHeight, 2 * keyWidth, keyHeight)) {
                      size_t curLen = strlen(setlistName);
                      if (curLen > 0) {
                          setlistName[curLen - 1] = '\0';
                      }
                      ui->drawSetlistName();
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
                ui->WifiScan();
                return;
            }
            break;
            case ScreenState::MENU2_WIFIPASS: {
                int keyWidth = tft.width() / 10;
                int keyHeight = 30;
                int keyStartY = 88;
            
                if (ui->isTouch(tx, ty, BACK_BUTTON_X, BACK_BUTTON_Y, BACK_BUTTON_WIDTH, BACK_BUTTON_HEIGHT)) {
                    ui->setScreenState(ScreenState::MENU2_WIFICONNECT);
                    break;
                } 
                else if (ui->isTouch(tx, ty, SAVE_BUTTON_X, SAVE_BUTTON_Y, SAVE_BUTTON_WIDTH, SAVE_BUTTON_HEIGHT)) {
                    Serial.println("User clicked OK to confirm Wi-Fi password: ");
                    Serial.println(wifiPassword);
                    ui->setScreenState(ScreenState::MENU2_WIFICONFIRM);
                    break;
                } 
                // If a "clear" button exists, handle it here.
                else if (ui->isTouch(tx, ty, CLEAR_BUTTON_X, CLEAR_BUTTON_Y, CLEAR_BUTTON_WIDTH, CLEAR_BUTTON_HEIGHT)) {
                    wifiPassword[0] = '\0';
                    ui->drawWiFiPassword();
                    break;
                } 
                else {
                    bool keyHandled = false;
                    // Process regular alphanumeric keys.
                    for (int row = 0; row < 4 && !keyHandled; row++) {
                        for (int col = 0; col < 10; col++) {
                            int keyX = col * keyWidth;
                            int keyY = keyStartY + row * keyHeight;
                            if (ui->isTouch(tx, ty, keyX, keyY, keyWidth, keyHeight)) {
                                char keyChar = (isShifted && row > 0) ? toupper(keys[row][col]) : keys[row][col];
                                size_t curLen = strlen(wifiPassword);
                                if (curLen < MAX_WIFI_PASS_LEN) {
                                    wifiPassword[curLen] = keyChar;
                                    wifiPassword[curLen + 1] = '\0';
                                }
                                ui->drawWiFiPassword();
                                keyHandled = true;
                                break;
                            }
                        }
                    }
                    if (keyHandled) break;
            
                    // Handle special keys: SHIFT, SPACE, BACKSPACE.
                    if (ui->isTouch(tx, ty, 0, keyStartY + 4 * keyHeight, 2 * keyWidth, keyHeight)) {
                        isShifted = !isShifted;
                        // Redraw keyboard and input area.
                        ui->menu2WifiPasswordScreen(); 
                        break;
                    }
                    if (ui->isTouch(tx, ty, 2 * keyWidth, keyStartY + 4 * keyHeight, 6 * keyWidth, keyHeight)) {
                        size_t curLen = strlen(wifiPassword);
                        if (curLen < MAX_WIFI_PASS_LEN) {
                            wifiPassword[curLen] = ' ';
                            wifiPassword[curLen + 1] = '\0';
                        }
                        ui->drawWiFiPassword();
                        break;
                    }
                    if (ui->isTouch(tx, ty, 8 * keyWidth, keyStartY + 4 * keyHeight, 2 * keyWidth, keyHeight)) {
                        size_t curLen = strlen(wifiPassword);
                        if (curLen > 0) {
                            wifiPassword[curLen - 1] = '\0';
                        }
                        ui->drawWiFiPassword();
                        break;
                    }
                }
                break;
            }            
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
        // ui->updateScreen(); // Refresh the screen after processing the touch.
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
              case ScreenState::HOME:
              if (dtState == clkState)
                ui->updateHomeMenuSelection(1);   // move selection forward
              else
                ui->updateHomeMenuSelection(-1);  // move selection backward
              break;
                break;
              case ScreenState::MENU1:
              {
                int prevIndex = currentMenuItem;
                // Update currentMenuItem based on encoder input:
                if (dtState == clkState) {
                    currentMenuItem = (currentMenuItem + 1) % NUM_MENU_ITEMS;
                    Serial.println(currentMenuItem);
                } else {
                    currentMenuItem = (currentMenuItem - 1 + NUM_MENU_ITEMS) % NUM_MENU_ITEMS;
                    Serial.println(currentMenuItem);
                } 
                // Update only the two items that changed.
                ui->updateMenuSelection(prevIndex, currentMenuItem);
              }
                break;
                case ScreenState::NEW_SETLIST:
                if (currentProject.songCount > 0) {
                    int itemsPerPage = 5;  // Fixed visible count
                    int previousItem = currentSongItem;
                    int previousScrollOffset = scrollOffset;
                    
                    // Update currentMenuItem with wrap-around.
                    if (dtState == clkState) { // Clockwise rotation.
                        if (currentSongItem == currentProject.songCount - 1)
                            currentSongItem = 0;
                        else
                            currentSongItem++;
                    } else { // Counter-clockwise rotation.
                        if (currentSongItem == 0)
                            currentSongItem = currentProject.songCount - 1;
                        else
                            currentSongItem--;
                    }
                    
                    // Adjust scrollOffset to ensure the currentMenuItem is visible.
                    if (currentSongItem < scrollOffset)
                        scrollOffset = currentSongItem;
                    else if (currentSongItem >= scrollOffset + itemsPerPage)
                        scrollOffset = currentSongItem - itemsPerPage + 1;
                    
                    // If the scroll window has changed (or if we wrapped), redraw the entire list.
                    if (scrollOffset != previousScrollOffset) {
                        ui->drawSongList();
                    } else {
                        // Otherwise, update only the affected items.
                        ui->drawSongListItem(previousItem, false);
                        ui->drawSongListItem(currentSongItem, true);
                    }
                    // currentMenuItem = 0;
                }
                break;              
                case ScreenState::EDIT_SETLIST: {
                  int itemsPerPage = 5;  // Fixed to show 5 items.
                  
                  if (isReordering) {
                      int previousReorderTarget = reorderTarget;
                      int previousScrollOffset = scrollOffset;
                      int total = selectedProject.songCount;
                      
                      // Handle reordering with wrap-around.
                      if (dtState == clkState) {  // Clockwise rotation.
                          if (reorderTarget == total - 1) {
                              // Wrap-around: swap the last song with the first.
                              SongInfo temp = selectedProject.songs[total - 1];
                              selectedProject.songs[total - 1] = selectedProject.songs[0];
                              selectedProject.songs[0] = temp;
                              reorderTarget = 0;
                          } else {
                              // Normal swap downward.
                              SongInfo temp = selectedProject.songs[reorderTarget];
                              selectedProject.songs[reorderTarget] = selectedProject.songs[reorderTarget + 1];
                              selectedProject.songs[reorderTarget + 1] = temp;
                              reorderTarget++;
                          }
                      } else {  // Counter-clockwise rotation.
                          if (reorderTarget == 0) {
                              // Wrap-around: swap the first song with the last.
                              SongInfo temp = selectedProject.songs[0];
                              selectedProject.songs[0] = selectedProject.songs[total - 1];
                              selectedProject.songs[total - 1] = temp;
                              reorderTarget = total - 1;
                          } else {
                              // Normal swap upward.
                              SongInfo temp = selectedProject.songs[reorderTarget];
                              selectedProject.songs[reorderTarget] = selectedProject.songs[reorderTarget - 1];
                              selectedProject.songs[reorderTarget - 1] = temp;
                              reorderTarget--;
                          }
                      }
                      
                      // Adjust scrollOffset with wrap-around.
                      if (reorderTarget == 0) {
                          scrollOffset = 0;
                      } else if (reorderTarget == total - 1) {
                          scrollOffset = max(total - itemsPerPage, 0);
                      } else {
                          if (reorderTarget < scrollOffset)
                              scrollOffset = reorderTarget;
                          else if (reorderTarget >= scrollOffset + itemsPerPage)
                              scrollOffset = reorderTarget - itemsPerPage + 1;
                      }
                      
                      // If the scroll window changed, redraw the entire list.
                      if (scrollOffset != previousScrollOffset) {
                          ui->drawEditedSongList();
                      } else {
                          // Otherwise, update the affected items.
                          // Always draw the new reorderTarget as highlighted (green in reordering mode).
                          ui->drawEditedSongListItem(reorderTarget, true);
                          // Update the neighbor: if rotating clockwise, update the item before reorderTarget;
                          // if counter-clockwise, update the item after.
                          if (dtState == clkState) {
                              int neighbor = (reorderTarget == 0) ? total - 1 : reorderTarget - 1;
                              ui->drawEditedSongListItem(neighbor, false);
                          } else {
                              int neighbor = (reorderTarget == total - 1) ? 0 : reorderTarget + 1;
                              ui->drawEditedSongListItem(neighbor, false);
                          }
                      }
                  } else {
                      // Non-reordering branch with wrap-around (as previously implemented).
                      int previousItem = currentSongItem;
                      int previousScrollOffset = scrollOffset;
                      int total = selectedProject.songCount;
                      
                      if (dtState == clkState) {  // Clockwise: move down.
                          if (currentSongItem == total - 1)
                              currentSongItem = 0;
                          else
                              currentSongItem++;
                      } else {  // Counter-clockwise: move up.
                          if (currentSongItem == 0)
                              currentSongItem = total - 1;
                          else
                              currentSongItem--;
                      }
                      
                      // Adjust scrollOffset.
                      if (currentSongItem == 0)
                          scrollOffset = 0;
                      else if (currentSongItem == total - 1)
                          scrollOffset = max(total - itemsPerPage, 0);
                      else {
                          if (currentSongItem < scrollOffset)
                              scrollOffset = currentSongItem;
                          else if (currentSongItem >= scrollOffset + itemsPerPage)
                              scrollOffset = currentSongItem - itemsPerPage + 1;
                      }
                      
                      if (scrollOffset != previousScrollOffset) {
                          ui->drawEditedSongList();
                      } else {
                          ui->drawEditedSongListItem(previousItem, false);
                          ui->drawEditedSongListItem(currentSongItem, true);
                      }
                  }
              } break;                     
                // Rotary encoder event for preset selection
              case ScreenState::SELECT_SETLIST: {
                int itemsPerPage = 5;
                int previousIndex = ui->currentPresetIndex;
                int previousScrollOffset = ui->presetScrollOffset;
                int total = MAX_PRESETS;

                // Update currentPresetIndex with wrap-around
                if (dtState == clkState) {  // Clockwise rotation
                    if (ui->currentPresetIndex == total - 1)
                      ui->currentPresetIndex = 0;
                    else
                    ui->currentPresetIndex++;
                } else {  // Counter-clockwise rotation
                    if (ui->currentPresetIndex == 0)
                    ui->currentPresetIndex = total - 1;
                    else
                    ui->currentPresetIndex--;
                }

                // Adjust presetScrollOffset to keep currentPresetIndex visible.
                if (ui->currentPresetIndex < ui->presetScrollOffset)
                ui->presetScrollOffset = ui->currentPresetIndex;
                else if (ui->currentPresetIndex >= ui->presetScrollOffset + itemsPerPage)
                ui->presetScrollOffset = ui->currentPresetIndex - itemsPerPage + 1;

                // Instead of calling selectSetlistScreen() which does a full-screen clear,
                // only update the dynamic region:
                if (ui->presetScrollOffset != previousScrollOffset) {
                    // If scroll changed, redraw entire list area.
                    ui->drawPresetList();
                } else {
                    // Otherwise, update only the affected items.
                    ui->drawPresetListItem(previousIndex, false);
                    ui->drawPresetListItem(ui->currentPresetIndex, true);
                }
              } break;          
              case ScreenState::MENU2:
              {
                int prevIndex = currentMenu2Item;
                if (dtState == clkState)
                  currentMenu2Item = (currentMenu2Item + 1) % NUM_MENU2_ITEMS;
                else
                  currentMenu2Item = (currentMenu2Item - 1 + NUM_MENU2_ITEMS) % NUM_MENU2_ITEMS;
                  ui->updateMenu2Selection(prevIndex, currentMenu2Item);
              }
                break;
              case ScreenState::MENU2_WIFISETTINGS:
              {
                int prevIndex = currentWifiMenuItem;
                if (dtState == clkState)
                  currentWifiMenuItem = (currentWifiMenuItem + 1) % NUM_WIFI_MENU_ITEMS;
                else
                  currentWifiMenuItem = (currentWifiMenuItem - 1 + NUM_WIFI_MENU_ITEMS) % NUM_WIFI_MENU_ITEMS;
                  ui->updateWifiMenuSelection(prevIndex,  currentWifiMenuItem);
              }
                break;
                case ScreenState::MENU2_WIFICONNECT: {
                    const int itemsPerPage = 5;
                    int previousWifiItem = wifiCurrentItem;
                    int previousScrollOffset = wifiScrollOffset;
                
                    // Update wifiCurrentItem based on rotary encoder state.
                    if (dtState == clkState)
                        wifiCurrentItem++;
                    else
                        wifiCurrentItem--;
                
                    // Wrap-around.
                    if (wifiCurrentItem >= wifiCount)
                        wifiCurrentItem = 0;
                    else if (wifiCurrentItem < 0)
                        wifiCurrentItem = wifiCount - 1;
                
                    // Adjust scroll offset to keep the current item visible.
                    if (wifiCurrentItem < wifiScrollOffset)
                        wifiScrollOffset = wifiCurrentItem;
                    else if (wifiCurrentItem >= wifiScrollOffset + itemsPerPage)
                        wifiScrollOffset = wifiCurrentItem - (itemsPerPage - 1);
                    wifiScrollOffset = constrain(wifiScrollOffset, 0, max(0, wifiCount - itemsPerPage));
                
                    // Redraw only if scroll window changed; otherwise, update only the changed items.
                    if (wifiScrollOffset != previousScrollOffset) {
                        ui->drawWiFiList();
                    } else {
                        ui->drawWiFiListItem(previousWifiItem, false);
                        ui->drawWiFiListItem(wifiCurrentItem, true);
                    }
                    break;
                }                
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
          if (ui->getCurrentHomeMenuSelection() == 0) {
              ui->setScreenState(ScreenState::MENU1);
          } else {
              ui->setScreenState(ScreenState::MENU2);
          }
          break;
          case ScreenState::MENU1:
            switch (ui->getCurrentMenuItem()) {
                case 0:
                    ui->setScreenState(ScreenState::NEW_SETLIST);
                    break;
                case 1:
                    ui->setScreenState(ScreenState::SELECT_SETLIST);
                    break;
                case 2:
                    ui->setScreenState(ScreenState::EDIT_SETLIST);
                    break;
                case 3:
                    ui->setScreenState(ScreenState::HOME);
                    break;
                case 4:
                    ui->setScreenState(ScreenState::HOME);
                    break;
                default:
                    break;
            }
            break;
          case ScreenState::NEW_SETLIST:
            if (currentProject.songCount > 0) {
                // Toggle the selection of the current song.
                selectedSongs[currentSongItem] = !selectedSongs[currentSongItem];
                selectedTrackCount += selectedSongs[currentSongItem] ? 1 : -1;
                ui->drawSongListItem(currentSongItem, true);
                
                // Optionally, update the status text (e.g., "3 / 10")
                ui->updateSelectedSongCount();
            }
            break;
        
        case ScreenState::EDIT_SETLIST:
            if (isReordering) {
            isReordering = false;
            currentSongItem = reorderTarget;
            ui->drawEditedSongList();
            } else {
            reorderTarget = currentSongItem;
            isReordering = true;
            ui->drawEditedSongListItem(currentSongItem, true);
            }
            break;
        case ScreenState::SELECT_SETLIST:
            selectedPresetSlot = currentSongItem + 1;
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
                ui->setScreenState(ScreenState::WIFI_PROPERTIES);
            } else if (currentMenu2Item == 2) {
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
          if (debouncedState) { // Button pressed
              isPlaying = true;
              Midi::Play(loadedPreset.data.songs[currentTrack].songIndex);
              // Update only the track display region so that the text turns green.
              ui->drawLoadedPreset();
          }

      }
  }
}

void Input::StopButton() {
  static bool debouncedState = false;
  static bool lastRawState = HIGH; // Assuming buttons are INPUT_PULLUP.
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
          if (debouncedState) { // Button pressed.
              Midi::Stop();
              isPlaying = false;
              // Update only the track display region so that the text returns to normal color.
              ui->drawLoadedPreset();
          }
      }
  }
}

void Input::LeftButton() {
  static bool lastLeftState = HIGH;
  static unsigned long lastPressTime = 0;
  const unsigned long debounceDelay = 200; // milliseconds

  bool btnLeftState = digitalRead(BTN_LEFT);
  
  // Check for transition from not-pressed to pressed.
  if (lastLeftState == HIGH && btnLeftState == LOW) {
    if (!isPlaying) {
        if (millis() - lastPressTime > debounceDelay) {
            // Trigger event only once per press.
            if (selectedPresetSlot != -1 && totalTracks > 0) {
                currentTrack = (currentTrack - 1 + totalTracks) % totalTracks;
                Serial.println(F("Moved to previous track"));
                presetChanged = true;  // Mark that the track has changed.
                // Update the homepage preset display partially.
                ui->drawLoadedPreset();
            } else {
                Serial.println(F("No tracks available to navigate (left)."));
            }
            lastPressTime = millis();
        }
    }
  }
  lastLeftState = btnLeftState;
}

void Input::RightButton() {
  static bool lastRightState = HIGH;
  static unsigned long lastPressTime = 0;
  const unsigned long debounceDelay = 200; // milliseconds

  bool btnRightState = digitalRead(BTN_RIGHT);
  
  if (lastRightState == HIGH && btnRightState == LOW) {
    if (!isPlaying) {
        if (millis() - lastPressTime > debounceDelay) {
            if (selectedPresetSlot != -1 && totalTracks > 0) {
                currentTrack = (currentTrack + 1) % totalTracks;
                Serial.println(F("Moved to next track"));
                presetChanged = true;  // Mark that the track has changed.
                ui->drawLoadedPreset();
            } else {
                Serial.println(F("No tracks available to navigate (right)."));
            }
            lastPressTime = millis();
        }
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