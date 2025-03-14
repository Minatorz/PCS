// ----------------------------------------------------------------
//  Include Files and Configurations
// ----------------------------------------------------------------
#include <Arduino.h>
#include "config.h"

// ----------------------------------------------------------------
//  Hardware Objects and Global Variables
// ----------------------------------------------------------------
Adafruit_ILI9341 tft(TFT_CS, TFT_DC, TFT_RST);
XPT2046_Touchscreen ts(T_CS, T_IRQ);
ESPNATIVEUSBMIDI usbmidi;     // For ESP32-S3 or boards supporting native USB
MIDI_CREATE_INSTANCE(ESPNATIVEUSBMIDI, usbmidi, MIDI);
Preferences preferences;

// ----------------------------------------------------------------
//  Data Structures
// ----------------------------------------------------------------
static const uint8_t MAX_SONG_NAME_LEN = 48;
static const uint8_t MAX_SONGS         = 50;
static const uint8_t MAX_PRESETS       = 10;  // e.g., 10 setlist slots

// Structure holding information for a single song
struct SongInfo {
  byte  songIndex;        // Original song index from SysEx
  int   changedIndex;     // Index after reordering
  char  songName[MAX_SONG_NAME_LEN + 1]; // Song name (null terminated)
  float locatorTime;      // Locator time in seconds

  // Parse SysEx data for a single song.
  bool getInfo(const byte *data, unsigned length) {
    if (data[1] != 0x00 || data[2] != 0x01 || data[3] != 0x61) {
      Serial.println(F("Invalid Manufacturer ID"));
      return false;
    }
    if (data[4] != 0x00) {
      Serial.println(F("Unknown message type"));
      return false;
    }
    // Extract song index (ASCII '0'..'9' or fallback)
    if (data[5] >= '0' && data[5] <= '9')
      songIndex = data[5] - '0';
    else
      songIndex = data[5];
      
    // Extract song name
    memset(songName, 0, sizeof(songName));
    int i = 6, j = 0;
    while (i < (int)length - 1 && data[i] != 0x00 && j < MAX_SONG_NAME_LEN) {
      songName[j++] = (char)data[i++];
    }
    songName[j] = '\0';

    // Check for valid separator and time bytes
    if (data[i] != 0x00 || i + 1 >= (int)length - 1) {
      Serial.println(F("Invalid format: Missing separator or time bytes"));
      return false;
    }
    
    // Parse the locator time string
    i++;
    char timeBuf[16];
    memset(timeBuf, 0, sizeof(timeBuf));
    int k = 0;
    while (i < (int)length - 1 && data[i] != 0xF7 && k < 15) {
      timeBuf[k++] = (char)data[i++];
    }
    timeBuf[k] = '\0';
    locatorTime = atof(timeBuf);

    Serial.print(F("Parsed Song: "));
    Serial.print(songName);
    Serial.print(F(" / Index: "));
    Serial.print(songIndex);
    Serial.print(F(" / Time: "));
    Serial.println(locatorTime);
    return true;
  }
};

// Structure holding project information (project name plus list of songs)
struct ProjectInfo {
  char      projectName[MAX_SONG_NAME_LEN + 1];
  SongInfo  songs[MAX_SONGS];
  int       songCount;
};

// Structure representing a preset (setlist) with its project data
struct Preset {
  char        name[MAX_SONG_NAME_LEN + 1]; // User-chosen setlist name
  ProjectInfo data;                        // Underlying project and songs
};

// ----------------------------------------------------------------
//  Global Application State
// ----------------------------------------------------------------
Preset   loadedPreset;          // The currently loaded preset
ProjectInfo currentProject;     // For scanning new songs from Ableton
ProjectInfo selectedProject;    // User-selected subset of songs for editing
int selectedPresetSlot = -1;    // Preset slot currently in use
bool presetChanged = false;
bool newSetlistScanned = false;

// User selection / reordering state
bool selectedSongs[MAX_SONGS] = {false};
int  selectedTrackCount = 0;
bool isReorderedSongsInitialized = false;
bool isReordering = false;
int  reorderTarget = -1;

// Scanning state for songs received via SysEx
bool songsReady = false;  // Signals that the end-of-song-list SysEx has been received

// UI Navigation and Menu variables
static const int NUM_MENU_ITEMS = 5;
static const char menuItems[NUM_MENU_ITEMS][16] PROGMEM = {
  "NEW SETLIST", "SELECT SETLIST", "EDIT SETLIST", "DELETE SETLIST", "EXIT"
};

static const int NUM_MENU2_ITEMS = 3;
static const char menu2Items[NUM_MENU2_ITEMS][16] PROGMEM = {
  "WiFi Settings", "Device Settings", "Back"
};
int currentMenu2Item = 0;  // Highlight index for Menu2

static const int NUM_WIFI_MENU_ITEMS = 3;
static const char wifiMenuItems[NUM_WIFI_MENU_ITEMS][16] PROGMEM = {
  "WiFi Connect", "WiFi Properties", "Back"
};
int currentWifiMenuItem = 0;  // Highlight index for Wi-Fi sub-menu

int  currentMenuItem = 0;
int  scrollOffset = 0;       // For scrolling lists on the display
int  totalTracks = 0;        // Total songs in loaded preset
int  currentTrack = 0;       // Currently active track
bool needsRedraw = true;
int selectedHomeSongIndex = 0;

enum MenuSelection { MENU1_SELECTED, MENU2_SELECTED };
MenuSelection currentMenuSelection = MENU1_SELECTED;

byte currentVolume = 0;      // Current volume level
bool isPlaying = false;


// On-screen keyboard variables
static const char *specialKeys[] = { "^", "Space", "<-" };
static const char keys[4][10] = {
  {'1','2','3','4','5','6','7','8','9','0'},
  {'q','w','e','r','t','y','u','i','o','p'},
  {'a','s','d','f','g','h','j','k','l',';'},
  {'z','x','c','v','b','n','m',',','.','/'}
};
bool isShifted = false;
char setlistName[MAX_SONG_NAME_LEN + 1] = {0};

// Button and encoder state tracking
bool BtnStartState = false, BtnStopState = false;
bool BtnStartLastState = false, BtnStopLastState = false;
volatile int encoderPosition = 0;
bool encoderLastState = true;
bool encoderButtonState = true;

// WiFi variables
static const int MAX_WIFI_NETWORKS = 20; // Maximum number of networks to display
String wifiSSIDs[MAX_WIFI_NETWORKS];
int wifiRSSI[MAX_WIFI_NETWORKS];
int wifiCount = 0;  // Number of networks found
bool wifiScanInProgress = false;
bool wifiScanDone = false;
static const uint8_t MAX_WIFI_PASS_LEN = 32;
char wifiPassword[MAX_WIFI_PASS_LEN + 1] = {0};
String selectedSSID = "";
int wifiScrollOffset = 0;
int wifiCurrentItem = 0;
bool wifiFullyConnected = false;

// WebSevrer Variables
AsyncWebServer server(80);         // Create a webserver on port 80
AsyncWebSocket ws("/ws");
bool webServerStarted = false;       // Flag to start the server only once

unsigned long lastPingReplyTime = 0;

// ----------------------------------------------------------------
//  Screen State Machine
// ----------------------------------------------------------------
enum class ScreenState {
  LOADING,
  HOME,
  MENU1,
  NEW_SETLIST,
  EDIT_SETLIST,
  SELECT_SETLIST,
  SAVE_SETLIST,
  MENU2,
  MENU2_WIFISETTINGS,
  MENU2_WIFICONNECT,
  MENU2_WIFIPASS,
  MENU2_WIFICONFIRM,
  MENU2_WIFICONNECTING,
  DEVICE_PROPERTIES,
  WIFI_PROPERTIES      // <-- New state for Wi‑Fi properties
};

ScreenState currentScreen = ScreenState::LOADING;
ScreenState previousScreen = ScreenState::LOADING;

// Forward declarations for screen functions (handlers)
void loadingScreen();
void homeScreen();
void menuScreen();
void newSetlistScreen();
void editSetlistScreen();
void selectSetlistScreen();
void saveSetlistScreen();
void menu2Screen();
void menu2WifiSettingsScreen();
void menu2WifiConnectScreen();
void menu2WifiPasswordScreen();
void menu2WifiConfirmScreen();
void menu2WifiConnectingScreen();
void devicePropertiesScreen();
void wifiPropertiesScreen();

// Function pointer array for state handlers
typedef void (*ScreenHandler)();
ScreenHandler screenHandlers[] = {
  loadingScreen,
  homeScreen,
  menuScreen,
  newSetlistScreen,
  editSetlistScreen,
  selectSetlistScreen,
  saveSetlistScreen,
  menu2Screen,
  menu2WifiSettingsScreen,
  menu2WifiConnectScreen,
  menu2WifiPasswordScreen,
  menu2WifiConfirmScreen,
  menu2WifiConnectingScreen,
  devicePropertiesScreen,
  wifiPropertiesScreen    // <-- New handler
};


// Helper functions to update and change screen states
void updateScreen() {
  screenHandlers[static_cast<int>(currentScreen)]();
}

void setScreenState(ScreenState newState) {
  previousScreen = currentScreen;
  currentScreen = newState;
  updateScreen();
}

// ----------------------------------------------------------------
//  MIDI Handling Class
// ----------------------------------------------------------------
class MIDIHandler {
public:
  static void begin() {
    MIDI.begin(MIDI_CHANNEL_OMNI);
    MIDI.setHandleSystemExclusive(handleSysEx);
    usbmidi.begin();
  }

  // Handle incoming SysEx messages from Ableton
  static void handleSysEx(byte *data, unsigned length) {
    if (length < 6) {
      Serial.println(F("⚠️ SysEx message too short. Ignoring."));
      return;
    }
    if (data[1] != 0x00 || data[2] != 0x01 || data[3] != 0x61) {
      Serial.println(F("Invalid Manufacturer ID."));
      return;
    }
    // Project name message (0x02)
    if (data[4] == 0x02) {
      memset(currentProject.projectName, 0, sizeof(currentProject.projectName));
      int i = 5, j = 0;
      while (i < (int)length - 1 && data[i] != 0x00 && j < MAX_SONG_NAME_LEN) {
        currentProject.projectName[j++] = (char)data[i++];
      }
      currentProject.projectName[j] = '\0';
      Serial.print(F("Project Name Received: "));
      Serial.println(currentProject.projectName);
      return;
    }
    // End-of-song list message: 0x00 followed by 0x7F
    if (data[4] == 0x00 && data[5] == 0x7F) {
      Serial.println(F("End signal received. Song list complete."));
      Serial.print(F("Final total songs received: "));
      Serial.println(currentProject.songCount);
      songsReady = true;
      return;
    }

    if (data[4] == 0x31) {
      // Flash the LED to indicate a ping was received
      lastPingReplyTime = millis();
      Serial.println(F("Ping reply received"));
      return;
    }
    // Otherwise, parse the song data
    if (currentProject.songCount == 0) {
      memset(&currentProject.songs[0], 0, sizeof(currentProject.songs));
      currentProject.songCount = 0;
    }
    if (currentProject.songCount < MAX_SONGS) {
      Serial.printf("Processing Song %d...\n", currentProject.songCount + 1);
      if (currentProject.songs[currentProject.songCount].getInfo(data, length)) {
        Serial.printf("Song %d added: %s\n",
                      currentProject.songCount + 1,
                      currentProject.songs[currentProject.songCount].songName);
        currentProject.songCount++;
      } else {
        Serial.println(F("Failed to parse song info."));
      }
    } else {
      Serial.println(F("Song list is full. Cannot add more songs."));
    }
  }

  // Send a SysEx message for the selected song
  static void sendSysExForSong(int selectedIndex) {
    if (selectedIndex < 0 || selectedIndex >= loadedPreset.data.songCount) {
      Serial.println(F("⚠️ Invalid song selection."));
      return;
    }
    byte originalIndex = loadedPreset.data.songs[selectedIndex].songIndex;
    Serial.println(originalIndex);
    Serial.println(F("🎵 Sending SysEx for Song:"));
    Serial.print(F("🎶 Index: "));
    Serial.println(originalIndex);
    byte sysexMessage[7] = {0xF0, 0x00, 0x01, 0x61, 0x01, originalIndex, 0xF7};
    MIDI.sendSysEx(sizeof(sysexMessage), sysexMessage, true);
  }

  static void sendSysExForStop() {
    // Construct a SysEx message for stopping playback.
    // The bytes: F0 00 01 61 11 F7
    // where 0x11 is our chosen command code for "stop".
    byte sysexStopMessage[] = {0xF0, 0x00, 0x01, 0x61, 0x11, 0xF7};
    MIDI.sendSysEx(sizeof(sysexStopMessage), sysexStopMessage, true);
    Serial.println(F("Stop SysEx Command Sent"));
  }  
};

// Function to send a ping SysEx message
void sendPing() {
  byte sysexPingMessage[] = {0xF0, 0x00, 0x01, 0x61, 0x30, 0xF7};
  MIDI.sendSysEx(sizeof(sysexPingMessage), sysexPingMessage, true);
  Serial.println(F("Ping SysEx sent"));
}

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

// ----------------------------------------------------------------
//  UI Helper Functions (Drawing routines)
// ----------------------------------------------------------------
namespace UI {
  static const uint16_t COLOR_WHITE = ILI9341_WHITE;
  static const uint16_t COLOR_BLACK = ILI9341_BLACK;
  static const uint16_t COLOR_GREEN = ILI9341_GREEN;
  static const uint16_t COLOR_BLUE  = ILI9341_BLUE;
  static const uint16_t COLOR_RED   = ILI9341_RED;
  static const uint16_t COLOR_YELLOW= ILI9341_YELLOW;
  static const uint16_t COLOR_CYAN  = ILI9341_CYAN;

  // Draw plain text at given coordinates.
  void drawText(const char* text, int16_t x, int16_t y, uint16_t color = COLOR_WHITE, uint8_t size = 2) {
    tft.setTextColor(color);
    tft.setTextSize(size);
    tft.setCursor(x, y);
    tft.print(text);
  }

  // Overload to accept Arduino Strings.
  void drawText(const String &text, int16_t x, int16_t y, uint16_t color = COLOR_WHITE, uint8_t size = 2) {
    drawText(text.c_str(), x, y, color, size);
  }

  // Draw centered text within a defined rectangle.
  void drawTextCenter(const String &text, int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    int16_t tw = text.length() * 6 * 2;
    int16_t th = 16;  // Text height for size 2
    int16_t xc = x + (w - tw) / 2;
    int16_t yc = y + (h - th) / 2;
    tft.setCursor(xc, yc);
    tft.setTextColor(color);
    tft.setTextSize(2);
    tft.print(text);
  }

  // Draw a rectangular button with centered label.
  void drawRectButton(int16_t x, int16_t y, int16_t w, int16_t h, const char* label, uint16_t color = COLOR_WHITE) {
    tft.drawRect(x, y, w, h, color);
    drawTextCenter(String(label), x, y, w, h, color);
  }

  // Draw a top-center title with optional underline.
  void drawTextTopCenter(const char *text, int16_t yOffset = 7, bool underline = true, uint16_t textColor = COLOR_WHITE) {
    int16_t tw = strlen(text) * 6 * 2;
    int16_t xc = (tft.width() - tw) / 2;
    tft.setCursor(xc, yOffset);
    tft.setTextColor(textColor);
    tft.setTextSize(2);
    tft.print(text);
    if (underline) {
      tft.drawLine(xc, yOffset + 16, xc + tw, yOffset + 16, textColor);
    }
  }

  // Draw an on-screen keyboard.
  void drawKeyboard(bool shifted) {
    int keyStartY = 88;
    int keyHeight = 30;
    int keyWidth  = tft.width() / 10;
    // Clear keyboard area
    tft.fillRect(0, keyStartY, tft.width(), 5 * keyHeight, COLOR_BLACK);
    // Draw 4 rows of keys
    for (int row = 0; row < 4; row++) {
      for (int col = 0; col < 10; col++) {
        int x = col * keyWidth;
        int y = keyStartY + row * keyHeight;
        tft.drawRect(x, y, keyWidth, keyHeight, COLOR_WHITE);
        char displayKey = (shifted && row > 0) ? toupper(keys[row][col]) : keys[row][col];
        drawTextCenter(String(displayKey), x, y, keyWidth, keyHeight, COLOR_WHITE);
      }
    }
    // Draw special keys in the 5th row
    drawRectButton(0, keyStartY + 4 * keyHeight, 2 * keyWidth, keyHeight, specialKeys[0]);
    drawRectButton(2 * keyWidth, keyStartY + 4 * keyHeight, 6 * keyWidth, keyHeight, specialKeys[1]);
    drawRectButton(8 * keyWidth, keyStartY + 4 * keyHeight, 2 * keyWidth, keyHeight, specialKeys[2]);
  }
} // namespace UI

// ----------------------------------------------------------------
//  Touch and Input Helpers
// ----------------------------------------------------------------
bool getTouchCoordinates(int16_t &x, int16_t &y) {
  if (ts.touched()) {
    TS_Point point = ts.getPoint();
    // Remap touch coordinates to display coordinates
    x = map(point.x, 300, 3800, tft.width(), 0);
    y = map(point.y, 300, 3800, tft.height(), 0);
    return true;
  }
  return false;
}

bool isTouch(int16_t tx, int16_t ty, int16_t areaX, int16_t areaY, int16_t w, int16_t h) {
  return (tx > areaX && tx < (areaX + w) && ty > areaY && ty < (areaY + h));
}

// ----------------------------------------------------------------
//  Screen Functions (State Handlers)
// ----------------------------------------------------------------

// WebSocket event handler (optional for logging)
void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, 
  AwsEventType type, void *arg, uint8_t *data, size_t len) {
if (type == WS_EVT_CONNECT) {
Serial.println("WebSocket client connected");
} else if (type == WS_EVT_DISCONNECT) {
Serial.println("WebSocket client disconnected");
}
}

// 1) LOADING SCREEN
void loadingScreen() {
  tft.fillScreen(UI::COLOR_BLACK);
  UI::drawText("AbletonThesis", 20, 60, UI::COLOR_GREEN, 3);
  UI::drawText("Loading...", 40, 120, UI::COLOR_WHITE, 2);
  for (int i = 0; i <= 260; i += 20) {
    tft.fillRect(20, 160, i, 10, UI::COLOR_BLUE);
    delay(200);
  }
  Serial.println(F("Loading finish"));
  setScreenState(ScreenState::HOME);
}

// 2) HOME SCREEN & Volume Display
void displayVolume() {
  tft.fillRect(10, 50, 220, 30, UI::COLOR_BLACK);
  UI::drawText((String("Volume: ") + String(map(currentVolume, 0, 127, 0, 100))).c_str(),
               10, 50, UI::COLOR_WHITE, 2);
}

void homeScreen() {
  tft.fillScreen(UI::COLOR_BLACK);
  UI::drawText("Home Page", 10, 20, UI::COLOR_WHITE, 2);
  // Draw two boxes for navigation
  tft.drawRect(BOX1_X, BOX1_Y, BOX_WIDTH, BOX_HEIGHT, UI::COLOR_WHITE);
  UI::drawTextCenter("1", BOX1_X, BOX1_Y, BOX_WIDTH, BOX_HEIGHT, UI::COLOR_WHITE);
  tft.drawRect(BOX2_X, BOX2_Y, BOX_WIDTH, BOX_HEIGHT, UI::COLOR_WHITE);
  UI::drawTextCenter("2", BOX2_X, BOX2_Y, BOX_WIDTH, BOX_HEIGHT, UI::COLOR_WHITE);

  // Diagnostic marker: draw a colored border to indicate home screen is active
  // tft.drawRect(0, 0, tft.width(), tft.height(), UI::COLOR_GREEN);

  // Log state change
  Serial.println("Home Screen Displayed");

  displayVolume();

  if (selectedPresetSlot != -1) {
    if (presetChanged) {
      totalTracks = loadedPreset.data.songCount;
      presetChanged = false;
    }
    if (strcmp(loadedPreset.name, "No Preset") == 0 ||
        strlen(loadedPreset.data.projectName) == 0 ||
        totalTracks == 0) {
      UI::drawText("No tracks available.", 10, 80, UI::COLOR_RED, 3);
    } else {
      UI::drawText((String("Preset: ") + loadedPreset.name).c_str(), 10, 80, UI::COLOR_YELLOW, 2);
      uint16_t songColor = isPlaying ? UI::COLOR_GREEN : UI::COLOR_WHITE;
      UI::drawText(String(loadedPreset.data.songs[currentTrack].songName).c_str(),
                    10, 100, songColor, 3);
      UI::drawText((String(currentTrack + 1) + "/" + String(totalTracks)).c_str(),
                   tft.width() - 60, 100, UI::COLOR_WHITE, 2);
    }
    notifyPresetUpdate();
  } else {
    UI::drawText("No Preset Selected", 10, 80, UI::COLOR_RED, 2);
  }
}

// 3) MENU1 SCREEN
void menuScreen() {
  tft.fillScreen(UI::COLOR_BLACK);
  UI::drawTextTopCenter("SETLIST SETTINGS", 7, true, UI::COLOR_WHITE);
  for (int i = 0; i < NUM_MENU_ITEMS; i++) {
    int y = 60 + i * 30;
    tft.fillRect(10, y, 220, 30, UI::COLOR_BLACK);
    tft.setCursor(15, y + 5);
    tft.setTextColor((i == currentMenuItem) ? UI::COLOR_YELLOW : UI::COLOR_WHITE);
    tft.setTextSize(2);
    char buffer[16];
    strncpy_P(buffer, menuItems[i], sizeof(buffer));
    tft.print(buffer);
  }
}

// 4) NEW SETLIST SCREEN
void newSetlistScreen() {
  tft.fillScreen(UI::COLOR_BLACK);
  UI::drawRectButton(BACK_BUTTON_X, BACK_BUTTON_Y, BACK_BUTTON_WIDTH, BACK_BUTTON_HEIGHT, "Back");
  UI::drawRectButton(NEXT_BUTTON_X, NEXT_BUTTON_Y, NEXT_BUTTON_WIDTH, NEXT_BUTTON_HEIGHT, "Next");
  UI::drawTextTopCenter("New Setlist", 7, true, UI::COLOR_WHITE);

  // Send SysEx and wait for song list if not already scanned
  if (!newSetlistScanned) {
    unsigned long startTime = millis();
    byte sysexMessage[] = {0xF0, 0x00, 0x01, 0x61, 0x10, 0xF7};
    MIDI.sendSysEx(sizeof(sysexMessage), sysexMessage, true);
    Serial.println(F("Sent SysEx to Notify Ableton"));

    memset(&currentProject, 0, sizeof(currentProject));
    currentProject.songCount = 0;

    while ((currentProject.songCount == 0 || !songsReady) &&
           (millis() - startTime < 7000)) {
      delay(50);  // Wait for up to 7 seconds
    }
    if (!songsReady) {
      Serial.println(F("⏳ Timeout! Songs not fully received."));
      UI::drawText("Error: Incomplete setlist", 10, 60, UI::COLOR_RED, 2);
      return;
    }
    newSetlistScanned = true;
  }
  isReorderedSongsInitialized = false;
  delay(50);

  // Display scanned songs list
  int itemsPerPage = (tft.height() - 60) / 30 - 1;
  for (int i = 0; i < itemsPerPage && (i + scrollOffset) < currentProject.songCount; i++) {
    int idx = i + scrollOffset;
    int y = 60 + i * 30;
    tft.setCursor(10, y);
    if (selectedSongs[idx] && idx == currentMenuItem) {
      tft.setTextColor(UI::COLOR_CYAN, UI::COLOR_BLACK);
    } else if (selectedSongs[idx]) {
      tft.setTextColor(UI::COLOR_GREEN, UI::COLOR_BLACK);
    } else if (idx == currentMenuItem) {
      tft.setTextColor(UI::COLOR_YELLOW, UI::COLOR_BLACK);
    } else {
      tft.setTextColor(UI::COLOR_WHITE, UI::COLOR_BLACK);
    }
    tft.print(idx + 1);
    tft.print(". ");
    tft.print(currentProject.songs[idx].songName);
  }
  char buff[32];
  snprintf(buff, sizeof(buff), "%d / %d", selectedTrackCount, currentProject.songCount);
  UI::drawTextTopCenter(buff, 35, false, UI::COLOR_WHITE);
}

// 5) EDIT SETLIST SCREEN
void editSetlistScreen() {
  tft.fillScreen(UI::COLOR_BLACK);
  UI::drawRectButton(BACK_BUTTON_X, BACK_BUTTON_Y, BACK_BUTTON_WIDTH, BACK_BUTTON_HEIGHT, "Back");
  UI::drawRectButton(SAVE_BUTTON_X, SAVE_BUTTON_Y, SAVE_BUTTON_WIDTH, SAVE_BUTTON_HEIGHT, "Save");
  UI::drawTextTopCenter("Edit Setlist", 7, true, UI::COLOR_WHITE);

  if (selectedTrackCount == 0) {
    tft.setCursor(10, 60);
    tft.setTextColor(UI::COLOR_WHITE);
    tft.setTextSize(2);
    tft.print("No songs selected.");
    return;
  }
  
  // Initialize reordering if needed
  if (!isReorderedSongsInitialized) {
    memset(&selectedProject, 0, sizeof(selectedProject));
    strcpy(selectedProject.projectName, currentProject.projectName);
    int idx = 0;
    for (int i = 0; i < currentProject.songCount; i++) {
      if (selectedSongs[i]) {
        selectedProject.songs[idx] = currentProject.songs[i];
        selectedProject.songs[idx].songIndex  = i;
        selectedProject.songs[idx].changedIndex = idx;
        idx++;
      }
    }
    selectedProject.songCount = idx;
    
    Serial.println(F("Selected Project Info:"));
    for (int i = 0; i < selectedProject.songCount; i++) {
      Serial.print("Song ");
      Serial.print(selectedProject.songs[i].changedIndex + 1);
      Serial.print(" (OrigIdx ");
      Serial.print(selectedProject.songs[i].songIndex);
      Serial.print(") - ");
      Serial.println(selectedProject.songs[i].songName);
    }
    isReorderedSongsInitialized = true;
  }

  // Display songs in selectedProject
  int itemsPerPage = min((tft.height() - 60) / 30 - 1, selectedProject.songCount);
  for (int i = 0; i < itemsPerPage; i++) {
    int trackIndex = scrollOffset + i;
    if (trackIndex >= selectedProject.songCount) break;
    selectedProject.songs[trackIndex].changedIndex = trackIndex;
    int y = 60 + i * 30;
    if (isReordering && reorderTarget == trackIndex) {
      tft.setTextColor(UI::COLOR_GREEN, UI::COLOR_BLACK);
    } else if (!isReordering && trackIndex == currentMenuItem) {
      tft.setTextColor(UI::COLOR_YELLOW, UI::COLOR_BLACK);
    } else {
      tft.setTextColor(UI::COLOR_WHITE, UI::COLOR_BLACK);
    }
    tft.setCursor(10, y);
    tft.setTextSize(2);
    tft.print(trackIndex + 1);
    tft.print(". ");
    tft.print(selectedProject.songs[trackIndex].songName);
  }
  scrollOffset = constrain(scrollOffset, 0, max(0, selectedProject.songCount - itemsPerPage));
}

// 6) SELECT SETLIST SCREEN
void selectSetlistScreen() {
  tft.fillScreen(UI::COLOR_BLACK);
  UI::drawTextTopCenter("Select Preset", 7, true, UI::COLOR_WHITE);
  UI::drawRectButton(BACK_BUTTON_X, BACK_BUTTON_Y, BACK_BUTTON_WIDTH, BACK_BUTTON_HEIGHT, "Back");
  UI::drawRectButton(SAVE_BUTTON_X, SAVE_BUTTON_Y, SAVE_BUTTON_WIDTH, SAVE_BUTTON_HEIGHT, "Save");

  preferences.begin("Setlists", true);
  for (int i = 1; i <= MAX_PRESETS; i++) {
    int y = 50 + (i - 1) * 30;
    tft.setCursor(10, y);
    tft.setTextColor((i - 1) == currentMenuItem ? UI::COLOR_YELLOW : UI::COLOR_WHITE);
    tft.setTextSize(2);
    String presetName = preferences.getString(("p" + String(i) + "_name").c_str(), "No Preset");
    tft.print(i);
    tft.print(". ");
    tft.print(presetName);
  }
  preferences.end();
}

// 7) SAVE SETLIST SCREEN
void saveSetlistScreen() {
  tft.fillScreen(UI::COLOR_BLACK);
  UI::drawRectButton(BACK_BUTTON_X, BACK_BUTTON_Y, BACK_BUTTON_WIDTH, BACK_BUTTON_HEIGHT, "Back");
  UI::drawRectButton(SAVE_BUTTON_X, SAVE_BUTTON_Y, SAVE_BUTTON_WIDTH, SAVE_BUTTON_HEIGHT, "Save");
  UI::drawTextTopCenter("Save Setlist", 7, true, UI::COLOR_WHITE);

  if (selectedPresetSlot == -1) {
    UI::drawText("No Slot Selected", 10, 50, UI::COLOR_RED, 2);
    return;
  }
  
  // Draw setlist name input box and show typed name
  tft.drawRect(10, 50, tft.width() - 20, 30, UI::COLOR_WHITE);
  tft.setCursor(15, 57);
  tft.setTextColor(UI::COLOR_WHITE);
  tft.setTextSize(2);
  tft.print(setlistName);

  // Draw on-screen keyboard
  UI::drawKeyboard(isShifted);
}

// 8) MENU2 and WiFi-Related Screens
void drawWifiList() {
  tft.fillRect(10, 60, tft.width() - 20, 160, ILI9341_BLACK);
  const int itemsPerPage = 5;
  const int lineSpacing = 30;
  for (int i = 0; i < itemsPerPage && (i + wifiScrollOffset) < wifiCount; i++) {
    int index = wifiScrollOffset + i;
    int y = 60 + i * lineSpacing;
    if (index == wifiCurrentItem)
      tft.setTextColor(ILI9341_YELLOW, ILI9341_BLACK);
    else
      tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
    tft.setTextSize(2);
    tft.setCursor(10, y);
    tft.print(String(index + 1) + ". " + wifiSSIDs[index]);

    int rssiVal = wifiRSSI[index];
    int bars = map(rssiVal, -90, -30, 0, 5);
    bars = constrain(bars, 0, 5);
    int barWidth = 5, barHeight = 10, barSpacing = 3;
    int barBaseX = tft.width() - 60, barBaseY = y;
    for (int b = 0; b < 5; b++) {
      int bx = barBaseX + b * (barWidth + barSpacing);
      if (b < bars)
        tft.fillRect(bx, barBaseY, barWidth, barHeight, ILI9341_GREEN);
      else
        tft.drawRect(bx, barBaseY, barWidth, barHeight, ILI9341_WHITE);
    }
  }
}

void menu2Screen() {
  tft.fillScreen(UI::COLOR_BLACK);
  UI::drawTextTopCenter("System Settings", 7, true, UI::COLOR_WHITE);
  for (int i = 0; i < NUM_MENU2_ITEMS; i++) {
    int y = 60 + i * 30;
    tft.setCursor(15, y + 5);
    tft.setTextSize(2);
    tft.setTextColor((i == currentMenu2Item) ? UI::COLOR_YELLOW : UI::COLOR_WHITE);
    char buffer[16];
    strncpy_P(buffer, menu2Items[i], sizeof(buffer));
    tft.print(buffer);
  }
}

void menu2WifiSettingsScreen() {
  tft.fillScreen(UI::COLOR_BLACK);
  UI::drawTextTopCenter("Wi-Fi Settings", 7, true, UI::COLOR_WHITE);
  for (int i = 0; i < NUM_WIFI_MENU_ITEMS; i++) {
    int y = 60 + i * 30;
    tft.setCursor(15, y + 5);
    tft.setTextSize(2);
    tft.setTextColor((i == currentWifiMenuItem) ? UI::COLOR_YELLOW : UI::COLOR_WHITE);
    char buffer[16];
    strncpy_P(buffer, wifiMenuItems[i], sizeof(buffer));
    tft.print(buffer);
  }
}

void menu2WifiConnectScreen() {
  tft.fillScreen(UI::COLOR_BLACK);
  UI::drawTextTopCenter("Wi-Fi Connect", 7, true, UI::COLOR_WHITE);
  UI::drawRectButton(BACK_BUTTON_X, BACK_BUTTON_Y, BACK_BUTTON_WIDTH, BACK_BUTTON_HEIGHT, "Back");
  UI::drawRectButton(NEXT_BUTTON_X, NEXT_BUTTON_Y, NEXT_BUTTON_WIDTH, NEXT_BUTTON_HEIGHT, "Re");
  
  if (!wifiScanInProgress && !wifiScanDone) {
    Serial.println(F("Starting Async Wi-Fi Scan..."));
    WiFi.mode(WIFI_STA);
    WiFi.disconnect(true);
    delay(100);
    WiFi.scanNetworks(true);
    wifiScanInProgress = true;
  }
  if (wifiScanInProgress) {
    UI::drawText("Scanning...", 10, 60, UI::COLOR_YELLOW, 2);
  } else {
    if (wifiCount == 0)
      UI::drawText("No Networks Found", 10, 60, UI::COLOR_RED, 2);
    else
      drawWifiList();
  }
}

void menu2WifiPasswordScreen() {
  tft.fillScreen(UI::COLOR_BLACK);
  UI::drawTextTopCenter("Enter Password", 7, true, UI::COLOR_WHITE);
  UI::drawTextTopCenter(selectedSSID.c_str(), 27, false, UI::COLOR_YELLOW);
  UI::drawRectButton(BACK_BUTTON_X, BACK_BUTTON_Y, BACK_BUTTON_WIDTH, BACK_BUTTON_HEIGHT, "Back");
  UI::drawRectButton(SAVE_BUTTON_X, SAVE_BUTTON_Y, SAVE_BUTTON_WIDTH, SAVE_BUTTON_HEIGHT, "OK");

  tft.drawRect(10, 50, tft.width() - 20, 30, ILI9341_WHITE);
  tft.setCursor(15, 57);
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_WHITE);
  tft.print(wifiPassword);

  UI::drawKeyboard(isShifted);
}

void menu2WifiConfirmScreen() {
  tft.fillScreen(UI::COLOR_BLACK);
  UI::drawTextTopCenter("Confirmation", 7, true, UI::COLOR_WHITE);
  UI::drawText("SSID: ", 10, 50, UI::COLOR_WHITE, 2);
  UI::drawText(selectedSSID.c_str(), 85, 50, UI::COLOR_YELLOW, 2);
  UI::drawText("Password: ", 10, 80, UI::COLOR_WHITE, 2);
  UI::drawText(wifiPassword, 130, 80, UI::COLOR_CYAN, 2);
  UI::drawRectButton(BACK_BUTTON_X, BACK_BUTTON_Y, BACK_BUTTON_WIDTH, BACK_BUTTON_HEIGHT, "Back");
  UI::drawRectButton(SAVE_BUTTON_X, SAVE_BUTTON_Y, SAVE_BUTTON_WIDTH, SAVE_BUTTON_HEIGHT, "OK");
}

void menu2WifiConnectingScreen() {
  tft.fillScreen(UI::COLOR_BLACK);
  UI::drawTextTopCenter("Connecting to Wi-Fi", 7, true, UI::COLOR_WHITE);
  UI::drawText("SSID: ", 10, 60, UI::COLOR_WHITE, 2);
  UI::drawText(selectedSSID.c_str(), 85, 60, UI::COLOR_YELLOW, 2);
  UI::drawText("Please wait...", 10, 100, UI::COLOR_WHITE, 2);
}

void devicePropertiesScreen() {
  tft.fillScreen(UI::COLOR_BLACK);
  UI::drawTextTopCenter("Properties", 7, true, UI::COLOR_WHITE);
  
  // Example device properties (customize as needed)
  UI::drawText("Device: AbletonThesis", 10, 50, UI::COLOR_WHITE, 2);
  UI::drawText("Firmware: v1.0", 10, 80, UI::COLOR_WHITE, 2);
  UI::drawText("Chip Model: " + String(ESP.getChipModel()), 10, 110, UI::COLOR_WHITE, 2);
  UI::drawText("Chip Revision: " + String(ESP.getChipRevision()), 10, 140, UI::COLOR_WHITE, 2);
  UI::drawText("Free Heap: " + String(ESP.getFreeHeap()), 10, 170, UI::COLOR_WHITE, 2);
  
  // Add more information as needed
  
  // Draw a Back button to return to the previous menu
  UI::drawRectButton(BACK_BUTTON_X, BACK_BUTTON_Y, BACK_BUTTON_WIDTH, BACK_BUTTON_HEIGHT, "Back");
}

void wifiPropertiesScreen() {
  tft.fillScreen(UI::COLOR_BLACK);
  UI::drawTextTopCenter("Wi-Fi Properties", 7, true, UI::COLOR_WHITE);

  // Gather Wi-Fi details
  String ssid = WiFi.SSID();
  String ip = WiFi.localIP().toString();
  int rssi = WiFi.RSSI();
  String status = (WiFi.status() == WL_CONNECTED) ? "Connected" : "Disconnected";

  // Display the details
  UI::drawText("SSID: " + ssid, 10, 50, UI::COLOR_WHITE, 2);
  UI::drawText("IP: " + ip, 10, 80, UI::COLOR_WHITE, 2);
  UI::drawText("RSSI: " + String(rssi), 10, 110, UI::COLOR_WHITE, 2);
  UI::drawText("Status: " + status, 10, 140, UI::COLOR_WHITE, 2);

  // Back button to return to Wi-Fi settings menu
  UI::drawRectButton(BACK_BUTTON_X, BACK_BUTTON_Y, BACK_BUTTON_WIDTH, BACK_BUTTON_HEIGHT, "Back");
}

void checkWifiScan() {
  if (wifiScanInProgress) {
    int done = WiFi.scanComplete();
    if (done >= 0) {
      wifiScanInProgress = false;
      wifiScanDone = true;
      wifiCount = min(done, MAX_WIFI_NETWORKS);
      for (int i = 0; i < wifiCount; i++) {
        wifiSSIDs[i] = WiFi.SSID(i);
        wifiRSSI[i] = WiFi.RSSI(i);
      }
      Serial.print("Async Wi-Fi scan complete. Found: ");
      Serial.println(done);
      if (currentScreen == ScreenState::MENU2_WIFICONNECT)
        menu2WifiConnectScreen();
    }
  }
}

void checkWifiConnection() {
  if (currentScreen == ScreenState::MENU2_WIFICONNECTING) {
    if (WiFi.status() == WL_CONNECTED && !wifiFullyConnected) {
      wifiFullyConnected = true;
      digitalWrite(BUILTIN_LED, HIGH);
      tft.fillScreen(UI::COLOR_BLACK);
      UI::drawTextTopCenter("Connected!", 7, true, UI::COLOR_GREEN);
      UI::drawText("Successfully connected to: ", 10, 60, UI::COLOR_WHITE, 2);
      UI::drawText(selectedSSID.c_str(), 10, 80, UI::COLOR_YELLOW, 2);

      ws.onEvent(onWsEvent);
      server.addHandler(&ws);

      if (!webServerStarted) {
        if (!LittleFS.begin()) {
          Serial.println("LittleFS mount failed, formatting...");
          if (!LittleFS.format()) {
            Serial.println("LittleFS format failed!");
            return;
          }
          if (!LittleFS.begin()) {
            Serial.println("LittleFS mount failed after formatting!");
            return;
          }
        }
        Serial.println("LittleFS mounted successfully");
        server.serveStatic("/", LittleFS, "/");
        server.onNotFound([](AsyncWebServerRequest *request){
          request->send(LittleFS, "/index.html", "text/html");
        });
      }
      server.begin();
      webServerStarted = true;
      Serial.println(F("WebServer Started"));

      delay(2000);
      setScreenState(ScreenState::HOME);
    }
  }
}

// ----------------------------------------------------------------
//  Input Handling (Touch, Encoder, and Buttons)
// ----------------------------------------------------------------
void handleTouch() {
  static bool wasTouched = false;
  static unsigned long lastTouchTime = 0;
  const unsigned long DEBOUNCE_MS = 200;

  int16_t tx, ty;
  bool currentlyTouched = getTouchCoordinates(tx, ty);
  if (!wasTouched && currentlyTouched) {
    unsigned long now = millis();
    if (now - lastTouchTime > DEBOUNCE_MS) {
      lastTouchTime = now;
      switch (currentScreen) {
        case ScreenState::HOME:
          if (isTouch(tx, ty, BOX1_X, BOX1_Y, BOX_WIDTH, BOX_HEIGHT))
            setScreenState(ScreenState::MENU1);
          else if (isTouch(tx, ty, BOX2_X, BOX2_Y, BOX_WIDTH, BOX_HEIGHT))
            setScreenState(ScreenState::MENU2);
          break;
        case ScreenState::MENU1:
          for (int i = 0; i < NUM_MENU_ITEMS; i++) {
            int y = 60 + i * 30;
            if (isTouch(tx, ty, 10, y, 220, 30)) {
              currentMenuItem = i;
              switch (currentMenuItem) {
                case 0:
                  setScreenState(ScreenState::NEW_SETLIST);
                  newSetlistScanned = false;
                  songsReady = false;
                  memset(selectedSongs, false, sizeof(selectedSongs));
                  selectedTrackCount = 0;
                  scrollOffset = 0;
                  currentMenuItem = 0;
                  break;
                case 1:
                  setScreenState(ScreenState::SELECT_SETLIST);
                  break;
                case 2:
                  setScreenState(ScreenState::EDIT_SETLIST);
                  scrollOffset = 0;
                  break;
                case 3:
                  setScreenState(ScreenState::HOME);
                  break;
                case 4:
                  setScreenState(ScreenState::HOME);
                  break;
              }
            }
          }
          break;
        case ScreenState::NEW_SETLIST:
          if (isTouch(tx, ty, BACK_BUTTON_X, BACK_BUTTON_Y, BACK_BUTTON_WIDTH, BACK_BUTTON_HEIGHT))
            setScreenState(ScreenState::MENU1);
          else if (isTouch(tx, ty, NEXT_BUTTON_X, NEXT_BUTTON_Y, NEXT_BUTTON_WIDTH, NEXT_BUTTON_HEIGHT)) {
            setScreenState(ScreenState::EDIT_SETLIST);
            scrollOffset = 0;
          }
          else {
            int touchedIndex = (ty - 60) / 30 + scrollOffset;
            if (touchedIndex >= 0 && touchedIndex < currentProject.songCount) {
              currentMenuItem = touchedIndex;
              selectedSongs[touchedIndex] = !selectedSongs[touchedIndex];
              selectedTrackCount += selectedSongs[touchedIndex] ? 1 : -1;
              newSetlistScreen();
            }
          }
          break;
        case ScreenState::EDIT_SETLIST:
          if (isTouch(tx, ty, BACK_BUTTON_X, BACK_BUTTON_Y, BACK_BUTTON_WIDTH, BACK_BUTTON_HEIGHT))
            setScreenState(ScreenState::NEW_SETLIST);
          else if (isTouch(tx, ty, SAVE_BUTTON_X, SAVE_BUTTON_Y, SAVE_BUTTON_WIDTH, SAVE_BUTTON_HEIGHT))
            setScreenState(ScreenState::SELECT_SETLIST);
          break;
        case ScreenState::SELECT_SETLIST:
          if (isTouch(tx, ty, BACK_BUTTON_X, BACK_BUTTON_Y, BACK_BUTTON_WIDTH, BACK_BUTTON_HEIGHT))
            setScreenState(ScreenState::MENU1);
          else if (isTouch(tx, ty, SAVE_BUTTON_X, SAVE_BUTTON_Y, SAVE_BUTTON_WIDTH, SAVE_BUTTON_HEIGHT))
            setScreenState(ScreenState::SAVE_SETLIST);
          else {
            for (int i = 0; i < MAX_PRESETS; i++) {
              int y = 50 + i * 30;
              if (isTouch(tx, ty, 10, y, tft.width() - 20, 30)) {
                selectedPresetSlot = i + 1;
                presetChanged = true;
                Serial.print(F("✅ Selected Setlist Slot: "));
                Serial.println(selectedPresetSlot);
                if (previousScreen == ScreenState::MENU1) {
                  loadedPreset = PresetManager::loadPresetFromDevice(selectedPresetSlot);
                  setScreenState(ScreenState::HOME);
                } else {
                  setScreenState(ScreenState::SAVE_SETLIST);
                }
                return;
              }
            }
          }
          break;
        case ScreenState::SAVE_SETLIST:
          if (isTouch(tx, ty, BACK_BUTTON_X, BACK_BUTTON_Y, BACK_BUTTON_WIDTH, BACK_BUTTON_HEIGHT))
            setScreenState(ScreenState::EDIT_SETLIST);
          else if (isTouch(tx, ty, SAVE_BUTTON_X, SAVE_BUTTON_Y, SAVE_BUTTON_WIDTH, SAVE_BUTTON_HEIGHT)) {
            strcpy(loadedPreset.name, setlistName);
            strcpy(loadedPreset.data.projectName, selectedProject.projectName);
            loadedPreset.data.songCount = selectedTrackCount;
            for (int i = 0; i < selectedTrackCount; i++) {
              loadedPreset.data.songs[i] = selectedProject.songs[i];
              loadedPreset.data.songs[i].songIndex = selectedProject.songs[i].songIndex;
              loadedPreset.data.songs[i].changedIndex = selectedProject.songs[i].changedIndex;
            }
            PresetManager::savePresetToDevice(selectedPresetSlot, loadedPreset);
            Serial.print(F("✅ Setlist Saved in: "));
            Serial.println(selectedPresetSlot);
            loadedPreset = PresetManager::loadPresetFromDevice(selectedPresetSlot);
            isReorderedSongsInitialized = false;
            setScreenState(ScreenState::HOME);
          }
          else {
            int keyWidth = tft.width() / 10;
            int keyHeight = 30;
            int keyStartY = 88;
            for (int row = 0; row < 4; row++) {
              for (int col = 0; col < 10; col++) {
                int keyX = col * keyWidth, keyY = keyStartY + row * keyHeight;
                if (isTouch(tx, ty, keyX, keyY, keyWidth, keyHeight)) {
                  char keyChar = (isShifted && row > 0) ? toupper(keys[row][col]) : keys[row][col];
                  size_t curLen = strlen(setlistName);
                  if (curLen < MAX_SONG_NAME_LEN) {
                    setlistName[curLen] = keyChar;
                    setlistName[curLen + 1] = '\0';
                  }
                  saveSetlistScreen();
                  return;
                }
              }
            }
            // Handle special keys: SHIFT, SPACE, BACKSPACE
            if (isTouch(tx, ty, 0, keyStartY + 4 * keyHeight, 2 * keyWidth, keyHeight)) {
              isShifted = !isShifted;
              saveSetlistScreen();
              return;
            }
            if (isTouch(tx, ty, 2 * keyWidth, keyStartY + 4 * keyHeight, 6 * keyWidth, keyHeight)) {
              size_t curLen = strlen(setlistName);
              if (curLen < MAX_SONG_NAME_LEN) {
                setlistName[curLen] = ' ';
                setlistName[curLen + 1] = '\0';
              }
              saveSetlistScreen();
              return;
            }
            if (isTouch(tx, ty, 8 * keyWidth, keyStartY + 4 * keyHeight, 2 * keyWidth, keyHeight)) {
              size_t curLen = strlen(setlistName);
              if (curLen > 0) {
                setlistName[curLen - 1] = '\0';
              }
              saveSetlistScreen();
              return;
            }
          }
          break;
        case ScreenState::MENU2:
          for (int i = 0; i < NUM_MENU2_ITEMS; i++) {
            int y = 60 + i * 30;
            if (isTouch(tx, ty, 10, y, 220, 30)) {
              currentMenu2Item = i;
              switch (i) {
                case 0:
                  setScreenState(ScreenState::MENU2_WIFISETTINGS);
                  break;
                case 1:
                  setScreenState(ScreenState::DEVICE_PROPERTIES);
                  break;
                case 2:
                  setScreenState(ScreenState::HOME);
                  break;
              }
              return;
            }
          }
          break;
        case ScreenState::MENU2_WIFISETTINGS:
          for (int i = 0; i < NUM_WIFI_MENU_ITEMS; i++) {
            int y = 60 + i * 30;
            if (isTouch(tx, ty, 10, y, 220, 30)) {
              currentWifiMenuItem = i;
              switch (i) {
                case 0:
                  wifiScanInProgress = false;
                  wifiScanDone = false;
                  wifiCount = 0;
                  setScreenState(ScreenState::MENU2_WIFICONNECT);
                  break;
                case 1:
                  setScreenState(ScreenState::WIFI_PROPERTIES);
                  break;
                case 2:
                  setScreenState(ScreenState::MENU2);
              }
              return;
            }
          }
          break;
        case ScreenState::MENU2_WIFICONNECT:
          if (isTouch(tx, ty, BACK_BUTTON_X, BACK_BUTTON_Y, BACK_BUTTON_WIDTH, BACK_BUTTON_HEIGHT)) {
            setScreenState(ScreenState::MENU2_WIFISETTINGS);
            return;
          }
          else if (isTouch(tx, ty, NEXT_BUTTON_X, NEXT_BUTTON_Y, NEXT_BUTTON_WIDTH, NEXT_BUTTON_HEIGHT)) {
            WiFi.scanDelete();
            WiFi.disconnect(true);
            delay(50);
            WiFi.scanNetworks(true);
            wifiScanInProgress = true;
            wifiScanDone = false;
            menu2WifiConnectScreen();
            return;
          }
          break;
        case ScreenState::MENU2_WIFIPASS:
          if (isTouch(tx, ty, BACK_BUTTON_X, BACK_BUTTON_Y, BACK_BUTTON_WIDTH, BACK_BUTTON_HEIGHT)) {
            setScreenState(ScreenState::MENU2_WIFICONNECT);
          } else if (isTouch(tx, ty, SAVE_BUTTON_X, SAVE_BUTTON_Y, SAVE_BUTTON_WIDTH, SAVE_BUTTON_HEIGHT)) {
            Serial.println("User clicked OK to confirm Wi-Fi password: ");
            Serial.println(wifiPassword);
            setScreenState(ScreenState::MENU2_WIFICONFIRM);
          } else {
            int keyWidth = tft.width() / 10;
            int keyHeight = 30;
            int keyStartY = 88;
            for (int row = 0; row < 4; row++) {
              for (int col = 0; col < 10; col++) {
                int keyX = col * keyWidth, keyY = keyStartY + row * keyHeight;
                if (isTouch(tx, ty, keyX, keyY, keyWidth, keyHeight)) {
                  char keyChar = (isShifted && row > 0) ? toupper(keys[row][col]) : keys[row][col];
                  size_t curLen = strlen(wifiPassword);
                  if (curLen < MAX_WIFI_PASS_LEN) {
                    wifiPassword[curLen] = keyChar;
                    wifiPassword[curLen + 1] = '\0';
                  }
                  menu2WifiPasswordScreen();
                  return;
                }
              }
            }
            if (isTouch(tx, ty, 0, keyStartY + 4 * keyHeight, 2 * keyWidth, keyHeight)) {
              isShifted = !isShifted;
              menu2WifiPasswordScreen();
              return;
            }
            if (isTouch(tx, ty, 2 * keyWidth, keyStartY + 4 * keyHeight, 6 * keyWidth, keyHeight)) {
              size_t curLen = strlen(wifiPassword);
              if (curLen < MAX_WIFI_PASS_LEN) {
                wifiPassword[curLen] = ' ';
                wifiPassword[curLen + 1] = '\0';
              }
              menu2WifiPasswordScreen();
              return;
            }
            if (isTouch(tx, ty, 8 * keyWidth, keyStartY + 4 * keyHeight, 2 * keyWidth, keyHeight)) {
              size_t curLen = strlen(wifiPassword);
              if (curLen > 0) {
                wifiPassword[curLen - 1] = '\0';
              }
              menu2WifiPasswordScreen();
              return;
            }
          }
          break;
        case ScreenState::MENU2_WIFICONFIRM:
          if (isTouch(tx, ty, BACK_BUTTON_X, BACK_BUTTON_Y, BACK_BUTTON_WIDTH, BACK_BUTTON_HEIGHT)) {
            setScreenState(ScreenState::MENU2_WIFIPASS);
            return;
          }
          if (isTouch(tx, ty, SAVE_BUTTON_X, SAVE_BUTTON_Y, SAVE_BUTTON_WIDTH, SAVE_BUTTON_HEIGHT)) {
            Serial.printf("Connecting to SSID: %s with password: %s\n",
                          selectedSSID.c_str(), wifiPassword);
            WiFi.begin(selectedSSID.c_str(), wifiPassword);
            setScreenState(ScreenState::MENU2_WIFICONNECTING);
            return;
          }
          updateScreen();
          break;
        case ScreenState::DEVICE_PROPERTIES:
          if (isTouch(tx, ty, BACK_BUTTON_X, BACK_BUTTON_Y, BACK_BUTTON_WIDTH, BACK_BUTTON_HEIGHT)) {
            setScreenState(ScreenState::MENU2);  // Return to the previous menu
          }
          break;
        case ScreenState::WIFI_PROPERTIES:
          if (isTouch(tx, ty, BACK_BUTTON_X, BACK_BUTTON_Y, BACK_BUTTON_WIDTH, BACK_BUTTON_HEIGHT)) {
            setScreenState(ScreenState::MENU2_WIFISETTINGS);
          }
          break;      
        default:
          break;
      }
      updateScreen();
    }
  } else if (!currentlyTouched) {
    wasTouched = false;
  }

  // Handle rotary encoder button press
  bool btnState = digitalRead(ENC_SW);
  if (btnState == LOW && encoderButtonState == HIGH) {
    switch (currentScreen) {
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
          newSetlistScreen();
        }
        break;
      case ScreenState::EDIT_SETLIST:
        if (isReordering) {
          isReordering = false;
          currentMenuItem = reorderTarget;
          editSetlistScreen();
        } else {
          reorderTarget = currentMenuItem;
          isReordering = true;
          editSetlistScreen();
        }
        break;
      case ScreenState::SELECT_SETLIST:
        selectedPresetSlot = currentMenuItem + 1;
        Serial.print(F("✅ Selected Setlist Slot via Encoder: "));
        Serial.println(selectedPresetSlot);
        setScreenState(ScreenState::SAVE_SETLIST);
        break;
      case ScreenState::MENU2:
        if (currentMenu2Item == 0) {
          setScreenState(ScreenState::MENU2_WIFISETTINGS);
        } else if (currentMenu2Item == 1) {
          // Device Settings placeholder
        } else if (currentMenu2Item == 2) {
          setScreenState(ScreenState::HOME);
        }
        break;
      case ScreenState::MENU2_WIFISETTINGS:
        if (currentWifiMenuItem == 0) {
          wifiScanInProgress = false;
          wifiScanDone = false;
          wifiCount = 0;
          setScreenState(ScreenState::MENU2_WIFICONNECT);
        } else if (currentWifiMenuItem == 1) {
          setScreenState(ScreenState::MENU2);
        }
        break;
      case ScreenState::MENU2_WIFICONNECT:
        if (wifiCount > 0) {
          selectedSSID = wifiSSIDs[wifiCurrentItem];
          memset(wifiPassword, 0, sizeof(wifiPassword));
          setScreenState(ScreenState::MENU2_WIFIPASS);
        }
        break;
      default:
        break;
    }
  }
  encoderButtonState = btnState;
}

// ----------------------------------------------------------------
//  Rotary Encoder Handler
// ----------------------------------------------------------------
void handleEncoder() {
  pinMode(ENC_CLK, INPUT);
  pinMode(ENC_DT, INPUT);
  pinMode(ENC_SW, INPUT_PULLUP);

  bool clkState = digitalRead(ENC_CLK);
  bool dtState  = digitalRead(ENC_DT);

  if (clkState != encoderLastState && clkState == LOW) {
    switch (currentScreen) {
      case ScreenState::MENU1:
        if (dtState == clkState)
          currentMenuItem = (currentMenuItem + 1) % NUM_MENU_ITEMS;
        else
          currentMenuItem = (currentMenuItem - 1 + NUM_MENU_ITEMS) % NUM_MENU_ITEMS;
        menuScreen();
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
          newSetlistScreen();
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
          editSetlistScreen();
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
          editSetlistScreen();
        }
      } break;
      case ScreenState::SELECT_SETLIST:
        if (dtState == clkState)
          currentMenuItem = (currentMenuItem + 1) % MAX_PRESETS;
        else
          currentMenuItem = (currentMenuItem - 1 + MAX_PRESETS) % MAX_PRESETS;
        selectSetlistScreen();
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
        menu2Screen();
        break;
      case ScreenState::MENU2_WIFISETTINGS:
        if (dtState == clkState)
          currentWifiMenuItem = (currentWifiMenuItem + 1) % NUM_WIFI_MENU_ITEMS;
        else
          currentWifiMenuItem = (currentWifiMenuItem - 1 + NUM_WIFI_MENU_ITEMS) % NUM_WIFI_MENU_ITEMS;
        menu2WifiSettingsScreen();
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
        drawWifiList();
      } break;
      default:
        break;
    }
  }
  encoderLastState = clkState;
}

// ----------------------------------------------------------------
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
          MIDIHandler::sendSysExForSong(currentTrack);
          Serial.println("Start Button Pressed");
          needsRedraw = true;
        } else {
          Serial.println("Start Button Released");
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
          MIDIHandler::sendSysExForStop();
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
        homeScreen();
        Serial.println(F("Moved to previous track"));
        vTaskDelay(pdMS_TO_TICKS(200));
      }
      if (btnRightState && !BtnStopLastState) {
        currentTrack = (currentTrack + 1) % totalTracks;
        homeScreen();
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
      MIDI.sendControlChange(7, midiVol, 1);
      if (currentScreen == ScreenState::HOME)
        displayVolume();
    }
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

void midiTask(void *pvParameters) {
  for (;;) {
    MIDI.read();
    vTaskDelay(pdMS_TO_TICKS(1));
  }
}

void pingTask(void *pvParameters) {
  for (;;) {
    sendPing();
    
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

  USB.productName("AbletonThesis");
  if (!USB.begin()) {
    Serial.println(F("USB Initialization Failed!"));
    while (1);
  }

  // Initialize MIDI
  MIDIHandler::begin();

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

  // Initialize touch
  if (!ts.begin()) {
    Serial.println(F("Touchscreen Initialization Failed!"));
    while (1);
  }

  // Initialize TFT display
  tft.begin();
  tft.setRotation(1);

  // Initialize preferences
  preferences.begin("Setlists", false);
  // preferences.clear(); // Uncomment to clear saved presets on boot if desired
  preferences.end();
  Serial.println(F("Ready to run."));

  tft.fillScreen(UI::COLOR_BLACK);

  // Start with LOADING screen
  updateScreen();

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
  handleTouch();
  handleEncoder();
  checkWifiScan();
  checkWifiConnection();

  if (needsRedraw) {
    updateScreen();
    needsRedraw = false;
  }

  delay(10);  // Yield time to other tasks
}

void notifyPresetUpdate() {
  // Build a JSON string with preset data
  String json = "{";
  json += "\"name\":\"" + String(loadedPreset.name) + "\",";
  json += "\"projectName\":\"" + String(loadedPreset.data.projectName) + "\",";
  json += "\"songCount\":" + String(loadedPreset.data.songCount) + ",";
  json += "\"songs\":[";
  for (int i = 0; i < loadedPreset.data.songCount; i++) {
    json += "{";
    json += "\"songName\":\"" + String(loadedPreset.data.songs[i].songName) + "\",";
    json += "\"songIndex\":" + String(loadedPreset.data.songs[i].songIndex);
    json += "}";
    if (i < loadedPreset.data.songCount - 1)
      json += ",";
  }
  json += "]}";
  
  // Broadcast the JSON message to all connected WebSocket clients
  ws.textAll(json);
  Serial.println("Preset updated and broadcasted: " + json);
}