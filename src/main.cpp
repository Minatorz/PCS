#include <Arduino.h>
#include "config.h"
#include "_midi.h"
#include "_ui.h"
#include "_input.h"
#include "_preset.h"

// AsyncWebServer server(80);         // Create a webserver on port 80
// AsyncWebSocket ws("/ws");

UI ui;

Input input(ui.getTouchscreen(), ui.getDisplay(), &ui);

// void notifyPresetUpdate();

// // WebSocket event handler (optional for logging)
// void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, 
//   AwsEventType type, void *arg, uint8_t *data, size_t len) {
// if (type == WS_EVT_CONNECT) {
// Serial.println("WebSocket client connected");
// } else if (type == WS_EVT_DISCONNECT) {
// Serial.println("WebSocket client disconnected");
// }
// }

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
  ps::initPreferences();


  // Start with LOADING screen
  ui.updateScreen();

  xTaskCreatePinnedToCore(midiTask, "MIDI Task", 2048, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(pingTask, "PingTask", 2048, NULL, 1, NULL, 1);
}

void loop() {
  // Continuously handle touch and encoder inputs and check WiFi state
  input.handleTouch();
  input.handleRotary();
  input.StartButton();
  input.StopButton();
  input.LeftButton();
  input.RightButton();
  input.handleVolume();
  
  ui.checkWifiConnection();

  delay(10);  // Yield time to other tasks
}

