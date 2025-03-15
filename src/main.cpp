#include <Arduino.h>
#include "config.h"
#include "_midi.h"
#include "_ui.h"
#include "_input.h"

Preferences preferences;

AsyncWebServer server(80);         // Create a webserver on port 80
AsyncWebSocket ws("/ws");

UI ui;

Input input(ui.getTouchscreen(), ui.getDisplay(), &ui);

ScreenState currentScreen = ScreenState::LOADING;
ScreenState previousScreen = ScreenState::LOADING;

// ----------------------------------------------------------------
//  Preset Manager Class (Handles saving/loading to NVS)
// ----------------------------------------------------------------
void notifyPresetUpdate();

class PresetManager {
public:
  static void savePresetToDevice(int presetNumber, const Preset &preset) {
    preferences.begin("Setlists", false);
    preferences.putString(("p" + String(presetNumber) + "_name").c_str(), preset.name);
    preferences.putString(("p" + String(presetNumber) + "_proj").c_str(), preset.data.projectName);
    preferences.putInt(("p" + String(presetNumber) + "_count").c_str(), preset.data.songCount);

    Serial.print(F("✅ Saving Preset: "));
    Serial.println(preset.name);
    Serial.print(F("✅ Using Project: "));
    Serial.println(preset.data.projectName);
    Serial.println("🎵 Total Songs in Preset: " + String(preset.data.songCount));

    for (int i = 0; i < preset.data.songCount; i++) {
      String songKey    = "p" + String(presetNumber) + "_song" + String(i);
      String indexKey   = "p" + String(presetNumber) + "_index" + String(i);
      String timeKey    = "p" + String(presetNumber) + "_time" + String(i);
      String cindexKey  = "p" + String(presetNumber) + "_cindex" + String(i);

      preferences.putString(songKey.c_str(), preset.data.songs[i].songName);
      preferences.putInt(indexKey.c_str(), preset.data.songs[i].songIndex);
      preferences.putInt(cindexKey.c_str(), preset.data.songs[i].changedIndex);
      preferences.putFloat(timeKey.c_str(), preset.data.songs[i].locatorTime);

      Serial.print(F("✔️ Saved: "));
      Serial.print(preset.data.songs[i].songName);
      Serial.print(F(" (Index: "));
      Serial.print(preset.data.songs[i].songIndex);
      Serial.print(F(" (Changed: "));
      Serial.print(preset.data.songs[i].changedIndex);
      Serial.print(F(", Locator: "));
      Serial.print(preset.data.songs[i].locatorTime, 6);
      Serial.println(F(")"));
    }
    preferences.end();
  }

  static Preset loadPresetFromDevice(int presetNumber) {
    preferences.begin("Setlists", true);
    Preset preset;

    String loadedName = preferences.getString(("p" + String(presetNumber) + "_name").c_str(), "No Preset");
    String projName   = preferences.getString(("p" + String(presetNumber) + "_proj").c_str(), "");
    int    songCount  = preferences.getInt(("p" + String(presetNumber) + "_count").c_str(), 0);

    strlcpy(preset.name, loadedName.c_str(), sizeof(preset.name));
    strlcpy(preset.data.projectName, projName.c_str(), sizeof(preset.data.projectName));
    preset.data.songCount = songCount;

    Serial.println("🔄 Loading Preset: " + loadedName);
    Serial.println("Project Used: " + projName);
    Serial.println("🎵 Songs in Preset: " + String(songCount));

    for (int i = 0; i < songCount; i++) {
      String songKey   = "p" + String(presetNumber) + "_song" + String(i);
      String indexKey  = "p" + String(presetNumber) + "_index" + String(i);
      String timeKey   = "p" + String(presetNumber) + "_time" + String(i);
      String cindexKey = "p" + String(presetNumber) + "_cindex" + String(i);

      String sName  = preferences.getString(songKey.c_str(), "Unknown Song");
      int    sIndex = preferences.getInt(indexKey.c_str(), i);
      int    cIndex = preferences.getInt(cindexKey.c_str(), i);
      float  sTime  = preferences.getFloat(timeKey.c_str(), 0.0);

      strlcpy(preset.data.songs[i].songName, sName.c_str(), sizeof(preset.data.songs[i].songName));
      preset.data.songs[i].songIndex    = sIndex;
      preset.data.songs[i].changedIndex = cIndex;
      preset.data.songs[i].locatorTime  = sTime;

      Serial.print(F("✔️ Loaded Song: "));
      Serial.print(preset.data.songs[i].songName);
      Serial.print(F(" (Index: "));
      Serial.print(preset.data.songs[i].songIndex);
      Serial.print(F(" (Changed Index: "));
      Serial.print(preset.data.songs[i].changedIndex);
      Serial.print(F(", Locator Time: "));
      Serial.print(preset.data.songs[i].locatorTime, 6);
      Serial.println(F(")"));

    }
    preferences.end();
    return preset;
  }
};

// WebSocket event handler (optional for logging)
void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, 
  AwsEventType type, void *arg, uint8_t *data, size_t len) {
if (type == WS_EVT_CONNECT) {
Serial.println("WebSocket client connected");
} else if (type == WS_EVT_DISCONNECT) {
Serial.println("WebSocket client disconnected");
}
}

// // 2) HOME SCREEN & Volume Display
// void displayVolume() {
//   tft.fillRect(10, 50, 220, 30, UI::COLOR_BLACK);
//   UI::drawText((String("Volume: ") + String(map(currentVolume, 0, 127, 0, 100))).c_str(),
//                10, 50, UI::COLOR_WHITE, 2);
// }

// void wifiPropertiesScreen() {
//   tft.fillScreen(UI::COLOR_BLACK);
//   UI::drawTextTopCenter("Wi-Fi Properties", 7, true, UI::COLOR_WHITE);

//   // Gather Wi-Fi details
//   String ssid = WiFi.SSID();
//   String ip = WiFi.localIP().toString();
//   int rssi = WiFi.RSSI();
//   String status = (WiFi.status() == WL_CONNECTED) ? "Connected" : "Disconnected";

//   // Display the details
//   UI::drawText("SSID: " + ssid, 10, 50, UI::COLOR_WHITE, 2);
//   UI::drawText("IP: " + ip, 10, 80, UI::COLOR_WHITE, 2);
//   UI::drawText("RSSI: " + String(rssi), 10, 110, UI::COLOR_WHITE, 2);
//   UI::drawText("Status: " + status, 10, 140, UI::COLOR_WHITE, 2);

//   // Back button to return to Wi-Fi settings menu
//   UI::drawRectButton(BACK_BUTTON_X, BACK_BUTTON_Y, BACK_BUTTON_WIDTH, BACK_BUTTON_HEIGHT, "Back");
// }

// void checkWifiScan() {
//   if (wifiScanInProgress) {
//     int done = WiFi.scanComplete();
//     if (done >= 0) {
//       wifiScanInProgress = false;
//       wifiScanDone = true;
//       wifiCount = min(done, MAX_WIFI_NETWORKS);
//       for (int i = 0; i < wifiCount; i++) {
//         wifiSSIDs[i] = WiFi.SSID(i);
//         wifiRSSI[i] = WiFi.RSSI(i);
//       }
//       Serial.print("Async Wi-Fi scan complete. Found: ");
//       Serial.println(done);
//       if (currentScreen == ScreenState::MENU2_WIFICONNECT)
//         menu2WifiConnectScreen();
//     }
//   }
// }

// void checkWifiConnection() {
//   if (currentScreen == ScreenState::MENU2_WIFICONNECTING) {
//     if (WiFi.status() == WL_CONNECTED && !wifiFullyConnected) {
//       wifiFullyConnected = true;
//       digitalWrite(BUILTIN_LED, HIGH);
//       tft.fillScreen(UI::COLOR_BLACK);
//       UI::drawTextTopCenter("Connected!", 7, true, UI::COLOR_GREEN);
//       UI::drawText("Successfully connected to: ", 10, 60, UI::COLOR_WHITE, 2);
//       UI::drawText(selectedSSID.c_str(), 10, 80, UI::COLOR_YELLOW, 2);

//       ws.onEvent(onWsEvent);
//       server.addHandler(&ws);

//       if (!webServerStarted) {
//         if (!LittleFS.begin()) {
//           Serial.println("LittleFS mount failed, formatting...");
//           if (!LittleFS.format()) {
//             Serial.println("LittleFS format failed!");
//             return;
//           }
//           if (!LittleFS.begin()) {
//             Serial.println("LittleFS mount failed after formatting!");
//             return;
//           }
//         }
//         Serial.println("LittleFS mounted successfully");
//         server.serveStatic("/", LittleFS, "/");
//         server.onNotFound([](AsyncWebServerRequest *request){
//           request->send(LittleFS, "/index.html", "text/html");
//         });
//       }
//       server.begin();
//       webServerStarted = true;
//       Serial.println(F("WebServer Started"));

//       delay(2000);
//       setScreenState(ScreenState::HOME);
//     }
//   }
// }

// ----------------------------------------------------------------
//  Input Handling (Touch, Encoder, and Buttons)
// // ----------------------------------------------------------------
// void handleTouch() {
//   static bool wasTouched = false;
//   static unsigned long lastTouchTime = 0;
//   const unsigned long DEBOUNCE_MS = 200;

//   int16_t tx, ty;
//   bool currentlyTouched = getTouchCoordinates(tx, ty);
//   if (!wasTouched && currentlyTouched) {
//     unsigned long now = millis();
//     if (now - lastTouchTime > DEBOUNCE_MS) {
//       lastTouchTime = now;
//       switch (currentScreen) {
//         case ScreenState::HOME:
//           if (isTouch(tx, ty, BOX1_X, BOX1_Y, BOX_WIDTH, BOX_HEIGHT))
//             setScreenState(ScreenState::MENU1);
//           else if (isTouch(tx, ty, BOX2_X, BOX2_Y, BOX_WIDTH, BOX_HEIGHT))
//             setScreenState(ScreenState::MENU2);
//           break;
//         case ScreenState::MENU1:
//           for (int i = 0; i < NUM_MENU_ITEMS; i++) {
//             int y = 60 + i * 30;
//             if (isTouch(tx, ty, 10, y, 220, 30)) {
//               currentMenuItem = i;
//               switch (currentMenuItem) {
//                 case 0:
//                   setScreenState(ScreenState::NEW_SETLIST);
//                   newSetlistScanned = false;
//                   songsReady = false;
//                   memset(selectedSongs, false, sizeof(selectedSongs));
//                   selectedTrackCount = 0;
//                   scrollOffset = 0;
//                   currentMenuItem = 0;
//                   break;
//                 case 1:
//                   setScreenState(ScreenState::SELECT_SETLIST);
//                   break;
//                 case 2:
//                   setScreenState(ScreenState::EDIT_SETLIST);
//                   scrollOffset = 0;
//                   break;
//                 case 3:
//                   setScreenState(ScreenState::HOME);
//                   break;
//                 case 4:
//                   setScreenState(ScreenState::HOME);
//                   break;
//               }
//             }
//           }
//           break;
//         case ScreenState::NEW_SETLIST:
//           if (isTouch(tx, ty, BACK_BUTTON_X, BACK_BUTTON_Y, BACK_BUTTON_WIDTH, BACK_BUTTON_HEIGHT))
//             setScreenState(ScreenState::MENU1);
//           else if (isTouch(tx, ty, NEXT_BUTTON_X, NEXT_BUTTON_Y, NEXT_BUTTON_WIDTH, NEXT_BUTTON_HEIGHT)) {
//             setScreenState(ScreenState::EDIT_SETLIST);
//             scrollOffset = 0;
//           }
//           else {
//             int touchedIndex = (ty - 60) / 30 + scrollOffset;
//             if (touchedIndex >= 0 && touchedIndex < currentProject.songCount) {
//               currentMenuItem = touchedIndex;
//               selectedSongs[touchedIndex] = !selectedSongs[touchedIndex];
//               selectedTrackCount += selectedSongs[touchedIndex] ? 1 : -1;
//               newSetlistScreen();
//             }
//           }
//           break;
//         case ScreenState::EDIT_SETLIST:
//           if (isTouch(tx, ty, BACK_BUTTON_X, BACK_BUTTON_Y, BACK_BUTTON_WIDTH, BACK_BUTTON_HEIGHT))
//             setScreenState(ScreenState::NEW_SETLIST);
//           else if (isTouch(tx, ty, SAVE_BUTTON_X, SAVE_BUTTON_Y, SAVE_BUTTON_WIDTH, SAVE_BUTTON_HEIGHT))
//             setScreenState(ScreenState::SELECT_SETLIST);
//           break;
//         case ScreenState::SELECT_SETLIST:
//           if (isTouch(tx, ty, BACK_BUTTON_X, BACK_BUTTON_Y, BACK_BUTTON_WIDTH, BACK_BUTTON_HEIGHT))
//             setScreenState(ScreenState::MENU1);
//           else if (isTouch(tx, ty, SAVE_BUTTON_X, SAVE_BUTTON_Y, SAVE_BUTTON_WIDTH, SAVE_BUTTON_HEIGHT))
//             setScreenState(ScreenState::SAVE_SETLIST);
//           else {
//             for (int i = 0; i < MAX_PRESETS; i++) {
//               int y = 50 + i * 30;
//               if (isTouch(tx, ty, 10, y, tft.width() - 20, 30)) {
//                 selectedPresetSlot = i + 1;
//                 presetChanged = true;
//                 Serial.print(F("✅ Selected Setlist Slot: "));
//                 Serial.println(selectedPresetSlot);
//                 if (previousScreen == ScreenState::MENU1) {
//                   loadedPreset = PresetManager::loadPresetFromDevice(selectedPresetSlot);
//                   setScreenState(ScreenState::HOME);
//                 } else {
//                   setScreenState(ScreenState::SAVE_SETLIST);
//                 }
//                 return;
//               }
//             }
//           }
//           break;
//         case ScreenState::SAVE_SETLIST:
//           if (isTouch(tx, ty, BACK_BUTTON_X, BACK_BUTTON_Y, BACK_BUTTON_WIDTH, BACK_BUTTON_HEIGHT))
//             setScreenState(ScreenState::EDIT_SETLIST);
//           else if (isTouch(tx, ty, SAVE_BUTTON_X, SAVE_BUTTON_Y, SAVE_BUTTON_WIDTH, SAVE_BUTTON_HEIGHT)) {
//             strcpy(loadedPreset.name, setlistName);
//             strcpy(loadedPreset.data.projectName, selectedProject.projectName);
//             loadedPreset.data.songCount = selectedTrackCount;
//             for (int i = 0; i < selectedTrackCount; i++) {
//               loadedPreset.data.songs[i] = selectedProject.songs[i];
//               loadedPreset.data.songs[i].songIndex = selectedProject.songs[i].songIndex;
//               loadedPreset.data.songs[i].changedIndex = selectedProject.songs[i].changedIndex;
//             }
//             PresetManager::savePresetToDevice(selectedPresetSlot, loadedPreset);
//             Serial.print(F("✅ Setlist Saved in: "));
//             Serial.println(selectedPresetSlot);
//             loadedPreset = PresetManager::loadPresetFromDevice(selectedPresetSlot);
//             isReorderedSongsInitialized = false;
//             setScreenState(ScreenState::HOME);
//           }
//           else {
//             int keyWidth = tft.width() / 10;
//             int keyHeight = 30;
//             int keyStartY = 88;
//             for (int row = 0; row < 4; row++) {
//               for (int col = 0; col < 10; col++) {
//                 int keyX = col * keyWidth, keyY = keyStartY + row * keyHeight;
//                 if (isTouch(tx, ty, keyX, keyY, keyWidth, keyHeight)) {
//                   char keyChar = (isShifted && row > 0) ? toupper(keys[row][col]) : keys[row][col];
//                   size_t curLen = strlen(setlistName);
//                   if (curLen < MAX_SONG_NAME_LEN) {
//                     setlistName[curLen] = keyChar;
//                     setlistName[curLen + 1] = '\0';
//                   }
//                   saveSetlistScreen();
//                   return;
//                 }
//               }
//             }
//             // Handle special keys: SHIFT, SPACE, BACKSPACE
//             if (isTouch(tx, ty, 0, keyStartY + 4 * keyHeight, 2 * keyWidth, keyHeight)) {
//               isShifted = !isShifted;
//               saveSetlistScreen();
//               return;
//             }
//             if (isTouch(tx, ty, 2 * keyWidth, keyStartY + 4 * keyHeight, 6 * keyWidth, keyHeight)) {
//               size_t curLen = strlen(setlistName);
//               if (curLen < MAX_SONG_NAME_LEN) {
//                 setlistName[curLen] = ' ';
//                 setlistName[curLen + 1] = '\0';
//               }
//               saveSetlistScreen();
//               return;
//             }
//             if (isTouch(tx, ty, 8 * keyWidth, keyStartY + 4 * keyHeight, 2 * keyWidth, keyHeight)) {
//               size_t curLen = strlen(setlistName);
//               if (curLen > 0) {
//                 setlistName[curLen - 1] = '\0';
//               }
//               saveSetlistScreen();
//               return;
//             }
//           }
//           break;
//         case ScreenState::MENU2:
//           for (int i = 0; i < NUM_MENU2_ITEMS; i++) {
//             int y = 60 + i * 30;
//             if (isTouch(tx, ty, 10, y, 220, 30)) {
//               currentMenu2Item = i;
//               switch (i) {
//                 case 0:
//                   setScreenState(ScreenState::MENU2_WIFISETTINGS);
//                   break;
//                 case 1:
//                   setScreenState(ScreenState::DEVICE_PROPERTIES);
//                   break;
//                 case 2:
//                   setScreenState(ScreenState::HOME);
//                   break;
//               }
//               return;
//             }
//           }
//           break;
//         case ScreenState::MENU2_WIFISETTINGS:
//           for (int i = 0; i < NUM_WIFI_MENU_ITEMS; i++) {
//             int y = 60 + i * 30;
//             if (isTouch(tx, ty, 10, y, 220, 30)) {
//               currentWifiMenuItem = i;
//               switch (i) {
//                 case 0:
//                   wifiScanInProgress = false;
//                   wifiScanDone = false;
//                   wifiCount = 0;
//                   setScreenState(ScreenState::MENU2_WIFICONNECT);
//                   break;
//                 case 1:
//                   setScreenState(ScreenState::WIFI_PROPERTIES);
//                   break;
//                 case 2:
//                   setScreenState(ScreenState::MENU2);
//               }
//               return;
//             }
//           }
//           break;
//         case ScreenState::MENU2_WIFICONNECT:
//           if (isTouch(tx, ty, BACK_BUTTON_X, BACK_BUTTON_Y, BACK_BUTTON_WIDTH, BACK_BUTTON_HEIGHT)) {
//             setScreenState(ScreenState::MENU2_WIFISETTINGS);
//             return;
//           }
//           else if (isTouch(tx, ty, NEXT_BUTTON_X, NEXT_BUTTON_Y, NEXT_BUTTON_WIDTH, NEXT_BUTTON_HEIGHT)) {
//             WiFi.scanDelete();
//             WiFi.disconnect(true);
//             delay(50);
//             WiFi.scanNetworks(true);
//             wifiScanInProgress = true;
//             wifiScanDone = false;
//             menu2WifiConnectScreen();
//             return;
//           }
//           break;
//         case ScreenState::MENU2_WIFIPASS:
//           if (isTouch(tx, ty, BACK_BUTTON_X, BACK_BUTTON_Y, BACK_BUTTON_WIDTH, BACK_BUTTON_HEIGHT)) {
//             setScreenState(ScreenState::MENU2_WIFICONNECT);
//           } else if (isTouch(tx, ty, SAVE_BUTTON_X, SAVE_BUTTON_Y, SAVE_BUTTON_WIDTH, SAVE_BUTTON_HEIGHT)) {
//             Serial.println("User clicked OK to confirm Wi-Fi password: ");
//             Serial.println(wifiPassword);
//             setScreenState(ScreenState::MENU2_WIFICONFIRM);
//           } else {
//             int keyWidth = tft.width() / 10;
//             int keyHeight = 30;
//             int keyStartY = 88;
//             for (int row = 0; row < 4; row++) {
//               for (int col = 0; col < 10; col++) {
//                 int keyX = col * keyWidth, keyY = keyStartY + row * keyHeight;
//                 if (isTouch(tx, ty, keyX, keyY, keyWidth, keyHeight)) {
//                   char keyChar = (isShifted && row > 0) ? toupper(keys[row][col]) : keys[row][col];
//                   size_t curLen = strlen(wifiPassword);
//                   if (curLen < MAX_WIFI_PASS_LEN) {
//                     wifiPassword[curLen] = keyChar;
//                     wifiPassword[curLen + 1] = '\0';
//                   }
//                   menu2WifiPasswordScreen();
//                   return;
//                 }
//               }
//             }
//             if (isTouch(tx, ty, 0, keyStartY + 4 * keyHeight, 2 * keyWidth, keyHeight)) {
//               isShifted = !isShifted;
//               menu2WifiPasswordScreen();
//               return;
//             }
//             if (isTouch(tx, ty, 2 * keyWidth, keyStartY + 4 * keyHeight, 6 * keyWidth, keyHeight)) {
//               size_t curLen = strlen(wifiPassword);
//               if (curLen < MAX_WIFI_PASS_LEN) {
//                 wifiPassword[curLen] = ' ';
//                 wifiPassword[curLen + 1] = '\0';
//               }
//               menu2WifiPasswordScreen();
//               return;
//             }
//             if (isTouch(tx, ty, 8 * keyWidth, keyStartY + 4 * keyHeight, 2 * keyWidth, keyHeight)) {
//               size_t curLen = strlen(wifiPassword);
//               if (curLen > 0) {
//                 wifiPassword[curLen - 1] = '\0';
//               }
//               menu2WifiPasswordScreen();
//               return;
//             }
//           }
//           break;
//         case ScreenState::MENU2_WIFICONFIRM:
//           if (isTouch(tx, ty, BACK_BUTTON_X, BACK_BUTTON_Y, BACK_BUTTON_WIDTH, BACK_BUTTON_HEIGHT)) {
//             setScreenState(ScreenState::MENU2_WIFIPASS);
//             return;
//           }
//           if (isTouch(tx, ty, SAVE_BUTTON_X, SAVE_BUTTON_Y, SAVE_BUTTON_WIDTH, SAVE_BUTTON_HEIGHT)) {
//             Serial.printf("Connecting to SSID: %s with password: %s\n",
//                           selectedSSID.c_str(), wifiPassword);
//             WiFi.begin(selectedSSID.c_str(), wifiPassword);
//             setScreenState(ScreenState::MENU2_WIFICONNECTING);
//             return;
//           }
//           updateScreen();
//           break;
//         case ScreenState::DEVICE_PROPERTIES:
//           if (isTouch(tx, ty, BACK_BUTTON_X, BACK_BUTTON_Y, BACK_BUTTON_WIDTH, BACK_BUTTON_HEIGHT)) {
//             setScreenState(ScreenState::MENU2);  // Return to the previous menu
//           }
//           break;
//         case ScreenState::WIFI_PROPERTIES:
//           if (isTouch(tx, ty, BACK_BUTTON_X, BACK_BUTTON_Y, BACK_BUTTON_WIDTH, BACK_BUTTON_HEIGHT)) {
//             setScreenState(ScreenState::MENU2_WIFISETTINGS);
//           }
//           break;      
//         default:
//           break;
//       }
//       updateScreen();
//     }
//   } else if (!currentlyTouched) {
//     wasTouched = false;
//   }

//   // Handle rotary encoder button press
//   bool btnState = digitalRead(ENC_SW);
//   if (btnState == LOW && encoderButtonState == HIGH) {
//     switch (currentScreen) {
//       case ScreenState::HOME:
//         // if (currentMenuSelection == MENU1_SELECTED)
//         //   setScreenState(ScreenState::MENU1);
//         // else
//         //   setScreenState(ScreenState::MENU2);
//         break;
//       case ScreenState::NEW_SETLIST:
//         if (currentProject.songCount > 0) {
//           selectedSongs[currentMenuItem] = !selectedSongs[currentMenuItem];
//           selectedTrackCount += selectedSongs[currentMenuItem] ? 1 : -1;
//           newSetlistScreen();
//         }
//         break;
//       case ScreenState::EDIT_SETLIST:
//         if (isReordering) {
//           isReordering = false;
//           currentMenuItem = reorderTarget;
//           editSetlistScreen();
//         } else {
//           reorderTarget = currentMenuItem;
//           isReordering = true;
//           editSetlistScreen();
//         }
//         break;
//       case ScreenState::SELECT_SETLIST:
//         selectedPresetSlot = currentMenuItem + 1;
//         Serial.print(F("✅ Selected Setlist Slot via Encoder: "));
//         Serial.println(selectedPresetSlot);
//         setScreenState(ScreenState::SAVE_SETLIST);
//         break;
//       case ScreenState::MENU2:
//         if (currentMenu2Item == 0) {
//           setScreenState(ScreenState::MENU2_WIFISETTINGS);
//         } else if (currentMenu2Item == 1) {
//           // Device Settings placeholder
//         } else if (currentMenu2Item == 2) {
//           setScreenState(ScreenState::HOME);
//         }
//         break;
//       case ScreenState::MENU2_WIFISETTINGS:
//         if (currentWifiMenuItem == 0) {
//           wifiScanInProgress = false;
//           wifiScanDone = false;
//           wifiCount = 0;
//           setScreenState(ScreenState::MENU2_WIFICONNECT);
//         } else if (currentWifiMenuItem == 1) {
//           setScreenState(ScreenState::MENU2);
//         }
//         break;
//       case ScreenState::MENU2_WIFICONNECT:
//         if (wifiCount > 0) {
//           selectedSSID = wifiSSIDs[wifiCurrentItem];
//           memset(wifiPassword, 0, sizeof(wifiPassword));
//           setScreenState(ScreenState::MENU2_WIFIPASS);
//         }
//         break;
//       default:
//         break;
//     }
//   }
//   encoderButtonState = btnState;
// }

// // ----------------------------------------------------------------
// //  Rotary Encoder Handler
// // ----------------------------------------------------------------
// void handleEncoder() {
//   pinMode(ENC_CLK, INPUT);
//   pinMode(ENC_DT, INPUT);
//   pinMode(ENC_SW, INPUT_PULLUP);

//   bool clkState = digitalRead(ENC_CLK);
//   bool dtState  = digitalRead(ENC_DT);

//   if (clkState != encoderLastState && clkState == LOW) {
//     switch (currentScreen) {
//       case ScreenState::MENU1:
//         if (dtState == clkState)
//           currentMenuItem = (currentMenuItem + 1) % NUM_MENU_ITEMS;
//         else
//           currentMenuItem = (currentMenuItem - 1 + NUM_MENU_ITEMS) % NUM_MENU_ITEMS;
//         menuScreen();
//         break;
//       case ScreenState::NEW_SETLIST:
//         if (currentProject.songCount > 0) {
//           int itemsPerPage = (tft.height() - 60) / 30 - 1;
//           if (dtState == clkState)
//             currentMenuItem = min(currentMenuItem + 1, currentProject.songCount - 1);
//           else
//             currentMenuItem = max(currentMenuItem - 1, 0);
//           if (currentMenuItem < scrollOffset)
//             scrollOffset = currentMenuItem;
//           else if (currentMenuItem >= scrollOffset + itemsPerPage)
//             scrollOffset = currentMenuItem - itemsPerPage + 1;
//           newSetlistScreen();
//         }
//         break;
//       case ScreenState::EDIT_SETLIST: {
//         int itemsPerPage = (tft.height() - 60) / 30 - 1;
//         if (isReordering) {
//           if (dtState == clkState && reorderTarget < selectedProject.songCount - 1) {
//             SongInfo temp = selectedProject.songs[reorderTarget];
//             selectedProject.songs[reorderTarget] = selectedProject.songs[reorderTarget + 1];
//             selectedProject.songs[reorderTarget + 1] = temp;
//             reorderTarget++;
//           } else if (dtState != clkState && reorderTarget > 0) {
//             SongInfo temp = selectedProject.songs[reorderTarget];
//             selectedProject.songs[reorderTarget] = selectedProject.songs[reorderTarget - 1];
//             selectedProject.songs[reorderTarget - 1] = temp;
//             reorderTarget--;
//           }
//           if (reorderTarget < scrollOffset)
//             scrollOffset = reorderTarget;
//           else if (reorderTarget >= scrollOffset + itemsPerPage)
//             scrollOffset = reorderTarget - itemsPerPage + 1;
//           editSetlistScreen();
//         } else {
//           if (selectedProject.songCount > 0) {
//             if (dtState == clkState)
//               currentMenuItem = (currentMenuItem + 1) % selectedProject.songCount;
//             else
//               currentMenuItem = (currentMenuItem - 1 + selectedProject.songCount) % selectedProject.songCount;
//             if (currentMenuItem < scrollOffset)
//               scrollOffset = currentMenuItem;
//             else if (currentMenuItem >= scrollOffset + itemsPerPage)
//               scrollOffset = currentMenuItem - itemsPerPage + 1;
//           }
//           editSetlistScreen();
//         }
//       } break;
//       case ScreenState::SELECT_SETLIST:
//         if (dtState == clkState)
//           currentMenuItem = (currentMenuItem + 1) % MAX_PRESETS;
//         else
//           currentMenuItem = (currentMenuItem - 1 + MAX_PRESETS) % MAX_PRESETS;
//         selectSetlistScreen();
//         break;
//       case ScreenState::HOME:
//         if (dtState == clkState)
//           currentMenuSelection = (currentMenuSelection == MENU1_SELECTED) ? MENU2_SELECTED : MENU1_SELECTED;
//         needsRedraw = true;
//         break;
//       case ScreenState::MENU2:
//         if (dtState == clkState)
//           currentMenu2Item = (currentMenu2Item + 1) % NUM_MENU2_ITEMS;
//         else
//           currentMenu2Item = (currentMenu2Item - 1 + NUM_MENU2_ITEMS) % NUM_MENU2_ITEMS;
//         menu2Screen();
//         break;
//       case ScreenState::MENU2_WIFISETTINGS:
//         if (dtState == clkState)
//           currentWifiMenuItem = (currentWifiMenuItem + 1) % NUM_WIFI_MENU_ITEMS;
//         else
//           currentWifiMenuItem = (currentWifiMenuItem - 1 + NUM_WIFI_MENU_ITEMS) % NUM_WIFI_MENU_ITEMS;
//         menu2WifiSettingsScreen();
//         break;
//       case ScreenState::MENU2_WIFICONNECT: {
//         const int itemsPerPage = 5;
//         if (dtState == clkState)
//           wifiCurrentItem++;
//         else
//           wifiCurrentItem--;
//         if (wifiCurrentItem >= wifiCount)
//           wifiCurrentItem = 0;
//         else if (wifiCurrentItem < 0)
//           wifiCurrentItem = wifiCount - 1;
//         if (wifiCurrentItem < wifiScrollOffset)
//           wifiScrollOffset = wifiCurrentItem;
//         else if (wifiCurrentItem >= wifiScrollOffset + itemsPerPage)
//           wifiScrollOffset = wifiCurrentItem - (itemsPerPage - 1);
//         wifiScrollOffset = constrain(wifiScrollOffset, 0, max(0, wifiCount - itemsPerPage));
//         drawWifiList();
//       } break;
//       default:
//         break;
//     }
//   }
//   encoderLastState = clkState;
// }

// // ----------------------------------------------------------------
//  FreeRTOS Tasks
// ----------------------------------------------------------------
void buttonStartTask(void *pvParameters) {
  bool debouncedState = false;
  bool lastRawState = false;
  unsigned long lastDebounceTime = 0;
  const unsigned long debounceDelay = 20;
  for (;;) {
    bool rawState = (digitalRead(BTN_START) == LOW);
    if (rawState != lastRawState) {
      lastDebounceTime = millis();
      lastRawState = rawState;
    }
    if ((millis() - lastDebounceTime) > debounceDelay) {
      if (rawState != debouncedState) {
        debouncedState = rawState;
        if (debouncedState) {
          isPlaying = true;
          Midi::Play(loadedPreset.data.songs[currentTrack].songIndex);
          needsRedraw = true;
        } else {
        }
      }
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void buttonStopTask(void *pvParameters) {
  bool debouncedState = false;
  bool lastRawState = false;
  unsigned long lastDebounceTime = 0;
  const unsigned long debounceDelay = 20;
  for (;;) {
    bool rawState = (digitalRead(BTN_STOP) == LOW);
    if (rawState != lastRawState) {
      lastDebounceTime = millis();
      lastRawState = rawState;
    }
    if ((millis() - lastDebounceTime) > debounceDelay) {
      if (rawState != debouncedState) {
        debouncedState = rawState;
        if (debouncedState) {
          Midi::Stop();
          isPlaying = false;
          Serial.println("Stop Button Pressed: SysEx Stop Sent");
          needsRedraw = true;
        }
      }
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void buttonTask(void *pvParameters) {
  while (true) {
    bool btnLeftState = (digitalRead(BTN_LEFT) == LOW);
    bool btnRightState = (digitalRead(BTN_RIGHT) == LOW);
    if (selectedPresetSlot != -1 && totalTracks > 0) {
      if (btnLeftState && !BtnStartLastState) {
        currentTrack = (currentTrack - 1 + totalTracks) % totalTracks;
        // homeScreen();
        Serial.println(F("Moved to previous track"));
        vTaskDelay(pdMS_TO_TICKS(200));
      }
      if (btnRightState && !BtnStopLastState) {
        currentTrack = (currentTrack + 1) % totalTracks;
        // homeScreen();
        Serial.println(F("Moved to next track"));
        vTaskDelay(pdMS_TO_TICKS(200));
      }
    } else if (btnLeftState || btnRightState) {
      Serial.println(F("No tracks available to navigate."));
      vTaskDelay(pdMS_TO_TICKS(200));
    }
    BtnStartLastState = btnLeftState;
    BtnStopLastState = btnRightState;
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

void volumeControlTask(void *pvParameters) {
  byte lastMidiVolume = 255;
  for (;;) {
    int potValue = analogRead(POT_VOL);
    byte midiVol = map(potValue, 0, 4095, 0, 127);
    if (midiVol != lastMidiVolume) {
      lastMidiVolume = midiVol;
      currentVolume = midiVol;
      Midi::volume(midiVol);
      // if (currentScreen == ScreenState::HOME)
        // displayVolume();
    }
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

void midiTask(void *pvParameters) {
  for (;;) {
    Midi::read();
    vTaskDelay(pdMS_TO_TICKS(1));
  }
}

void pingTask(void *pvParameters) {
  for (;;) {
    Midi::Ping();
    
    // Check if a ping reply was received in the last 2 seconds
    if (millis() - lastPingReplyTime < 2000) {
      digitalWrite(LED_PING, HIGH);  // Connection is active: LED on
    } else {
      digitalWrite(LED_PING, LOW);   // No recent reply: LED off
    }
    
    vTaskDelay(pdMS_TO_TICKS(1000));  // Wait 1 second before sending the next ping
  }
}


// ----------------------------------------------------------------
//  Setup and Main Loop
// ----------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  // Initialize MIDI
  Midi::begin();

  // Configure pins
  pinMode(BTN_START, INPUT_PULLUP);
  pinMode(BTN_STOP, INPUT_PULLUP);
  pinMode(BTN_LEFT, INPUT_PULLUP);
  pinMode(BTN_RIGHT, INPUT_PULLUP);
  pinMode(POT_VOL, INPUT);
  pinMode(BUILTIN_LED, OUTPUT);
  digitalWrite(BUILTIN_LED, LOW);
  pinMode(LED_PING, OUTPUT);
  digitalWrite(LED_PING, LOW);

  ui.init();
  // Initialize preferences
  preferences.begin("Setlists", false);
  preferences.clear(); // Uncomment to clear saved presets on boot if desired
  preferences.end();
  Serial.println(F("Ready to run."));

  // tft.fillScreen(UI::COLOR_BLACK);

  // Start with LOADING screen
  ui.updateScreen();

  // Create FreeRTOS tasks
  xTaskCreatePinnedToCore(buttonStartTask, "BtnStartTask", 2048, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(buttonStopTask, "BtnStopTask", 2048, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(buttonTask, "BtnNavTask", 4096, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(volumeControlTask, "VolumeCtrlTask", 2048, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(midiTask, "MIDI Task", 2048, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(pingTask, "PingTask", 2048, NULL, 1, NULL, 1);
}

void loop() {
  // Continuously handle touch and encoder inputs and check WiFi state
  input.handleTouch();
  // handleEncoder();
  // checkWifiScan();
  // checkWifiConnection();

  // if (needsRedraw) {
  //   updateScreen();
  //   needsRedraw = false;
  // }

  delay(10);  // Yield time to other tasks
}

// void notifyPresetUpdate() {
//   // Build a JSON string with preset data
//   String json = "{";
//   json += "\"name\":\"" + String(loadedPreset.name) + "\",";
//   json += "\"projectName\":\"" + String(loadedPreset.data.projectName) + "\",";
//   json += "\"songCount\":" + String(loadedPreset.data.songCount) + ",";
//   json += "\"songs\":[";
//   for (int i = 0; i < loadedPreset.data.songCount; i++) {
//     json += "{";
//     json += "\"songName\":\"" + String(loadedPreset.data.songs[i].songName) + "\",";
//     json += "\"songIndex\":" + String(loadedPreset.data.songs[i].songIndex);
//     json += "}";
//     if (i < loadedPreset.data.songCount - 1)
//       json += ",";
//   }
//   json += "]}";
  
//   // Broadcast the JSON message to all connected WebSocket clients
//   ws.textAll(json);
//   Serial.println("Preset updated and broadcasted: " + json);
// }