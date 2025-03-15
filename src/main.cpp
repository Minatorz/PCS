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
  
  pinMode(ENC_CLK, INPUT);
  pinMode(ENC_DT, INPUT);
  pinMode(ENC_SW, INPUT_PULLUP);

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
  input.handleRotary();
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