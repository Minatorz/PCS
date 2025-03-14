#ifndef CONFIG_H
#define CONFIG_H

// *** LIBRARY DEFINITION ***
#include <ESPNATIVEUSBMIDI.h>
#include <USB.h>
#include <MIDI.h>
#include <Adafruit_ILI9341.h>
#include <XPT2046_Touchscreen.h>
#include <SPI.h>
#include <Preferences.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <LittleFS.h>

// *** ESP32 PIN DEFINITIONS ***
#define BTN_START 36
#define BTN_STOP  37
#define BTN_LEFT  35
#define BTN_RIGHT 38
#define POT_VOL   2
#define ENC_CLK   42
#define ENC_DT    41
#define ENC_SW    40
#define TFT_CS    10
#define TFT_DC    8
#define TFT_RST   9
#define T_CS      15
#define T_IRQ     21

// *** MENU BOX ***
#define BOX_WIDTH  35
#define BOX_HEIGHT 35
#define BOX1_X     240
#define BOX1_Y     5
#define BOX2_X     280
#define BOX2_Y     5

// *** BACK BUTTON ***
#define BACK_BUTTON_WIDTH  60
#define BACK_BUTTON_HEIGHT 35
#define BACK_BUTTON_X      5
#define BACK_BUTTON_Y      8

// *** NEXT BUTTON ***
#define NEXT_BUTTON_WIDTH  60
#define NEXT_BUTTON_HEIGHT 35
#define NEXT_BUTTON_X      (tft.width() - NEXT_BUTTON_WIDTH - 5)
#define NEXT_BUTTON_Y      8

// *** NEXT BUTTON ***
#define SAVE_BUTTON_WIDTH  60
#define SAVE_BUTTON_HEIGHT 35
#define SAVE_BUTTON_X      (tft.width() - SAVE_BUTTON_WIDTH - 5)
#define SAVE_BUTTON_Y      8

// *** SETLIST MENU ***
#define MENU_ITEM_HEIGHT 30
#define MENU_ITEM_X 20
#define MENU_ITEM_Y 50
#define MENU_ITEM_WIDTH 200

// *** 
#define LED_PING 0

// #define BUILTIN_LED

#endif
