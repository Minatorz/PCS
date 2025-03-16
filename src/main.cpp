#include <Arduino.h>
#include "config.h"
#include "_midi.h"
#include "_ui.h"
#include "_input.h"
#include "_preset.h"
#include "_webserver.h"

UI ui;

Input input(ui.getTouchscreen(), ui.getDisplay(), &ui);

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
  pinMode(LED_WIFI, OUTPUT);
  digitalWrite(LED_WIFI, LOW);
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