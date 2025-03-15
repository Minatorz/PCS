#include "config.h"

// -----------------------------------------------------
// Global Variable Definitions
// -----------------------------------------------------
int selectedPresetSlot = -1;
bool presetChanged = false;
bool newSetlistScanned = false;

bool selectedSongs[MAX_SONGS] = { false };
int  selectedTrackCount = 0;
bool isReorderedSongsInitialized = false;
bool isReordering = false;
int  reorderTarget = -1;

bool songsReady = false;

unsigned long lastPingReplyTime = 0;

bool webServerStarted = false;  

int currentMenuItem = 0;
int scrollOffset = 0;
int totalTracks = 0;
int currentTrack = 0;
bool needsRedraw = true;
int selectedHomeSongIndex = 0;

int currentMenu2Item = 0;
int currentWifiMenuItem = 0;

MenuSelection currentMenuSelection = MENU1_SELECTED;

byte currentVolume = 0;
bool isPlaying = false;

const char* specialKeys[] = { "^", "Space", "<-" };
const char keys[4][10] = {
  {'1','2','3','4','5','6','7','8','9','0'},
  {'q','w','e','r','t','y','u','i','o','p'},
  {'a','s','d','f','g','h','j','k','l',';'},
  {'z','x','c','v','b','n','m',',','.','/'}
};
bool isShifted = false;
char setlistName[MAX_SONG_NAME_LEN + 1] = {0};

bool BtnStartState = false, BtnStopState = false;
bool BtnStartLastState = false, BtnStopLastState = false;
volatile int encoderPosition = 0;
bool encoderLastState = true;
bool encoderButtonState = true;

String wifiSSIDs[MAX_WIFI_NETWORKS];
int wifiRSSI[MAX_WIFI_NETWORKS];
int wifiCount = 0;
bool wifiScanInProgress = false;
bool wifiScanDone = false;
char wifiPassword[MAX_WIFI_PASS_LEN + 1] = {0};
String selectedSSID = "";
int wifiScrollOffset = 0;
int wifiCurrentItem = 0;
bool wifiFullyConnected = false;

// -----------------------------------------------------
// Global Structure Instances
// -----------------------------------------------------
Preset loadedPreset;
ProjectInfo selectedProject;
ProjectInfo currentProject;

// -----------------------------------------------------
// Implementation of SongInfo::getInfo
// -----------------------------------------------------
bool SongInfo::getInfo(const byte *data, unsigned length) {
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