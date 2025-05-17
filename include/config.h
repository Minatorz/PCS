#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <WiFi.h>

// -----------------------------------------------------
// ESP32 PIN DEFINITIONS
// -----------------------------------------------------
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
#define T_IRQ     3

// -----------------------------------------------------
// MENU AND BUTTON MACROS
// -----------------------------------------------------
#define BOX_WIDTH  35
#define BOX_HEIGHT 35
#define BOX1_X     240
#define BOX1_Y     5
#define BOX2_X     280
#define BOX2_Y     5

#define BACK_BUTTON_WIDTH  60
#define BACK_BUTTON_HEIGHT 35
#define BACK_BUTTON_X      5
#define BACK_BUTTON_Y      8

// For buttons that depend on the display width, use a fixed value or compute at runtime.
#define DISPLAY_WIDTH 320
#define NEXT_BUTTON_WIDTH  60
#define NEXT_BUTTON_HEIGHT 35
#define NEXT_BUTTON_X      (DISPLAY_WIDTH - NEXT_BUTTON_WIDTH - 5)
#define NEXT_BUTTON_Y      8

#define SAVE_BUTTON_WIDTH  60
#define SAVE_BUTTON_HEIGHT 35
#define SAVE_BUTTON_X      (DISPLAY_WIDTH - SAVE_BUTTON_WIDTH - 5)
#define SAVE_BUTTON_Y      8

#define MENU_ITEM_HEIGHT 30
#define MENU_ITEM_X 20
#define MENU_ITEM_Y 50
#define MENU_ITEM_WIDTH 200

#define CLEAR_BUTTON_WIDTH  30
#define CLEAR_BUTTON_HEIGHT 30
// Position the clear button at the far right of the input area.
// Suppose your input text box starts at x = 10 and has width = (tft.width() - 20).
// We then place the button at the right edge of that box.
#define CLEAR_BUTTON_X      (tft.width() - 10 - CLEAR_BUTTON_WIDTH)
#define CLEAR_BUTTON_Y      50  // same y as input text area

#define LED_PING 4 
#define LED_WIFI 5

// -----------------------------------------------------
// Compile-Time Constants (defined as macros)
// -----------------------------------------------------
#define MAX_SONG_NAME_LEN 48
#define MAX_SONGS         50
#define MAX_PRESETS       10   // e.g., 10 setlist slots

#define MAX_WIFI_NETWORKS 20   // Maximum number of networks to display
#define MAX_WIFI_PASS_LEN 32

#define NUM_MENU_ITEMS    5
#define NUM_MENU2_ITEMS   3
#define NUM_WIFI_MENU_ITEMS 3

// -----------------------------------------------------
// Global Variable Declarations (using extern)
// -----------------------------------------------------
extern int selectedPresetSlot;    // Preset slot currently in use
extern bool presetChanged;
extern bool newSetlistScanned;

// User selection / reordering state
extern bool selectedSongs[MAX_SONGS];
extern int  selectedTrackCount;
extern bool isReorderedSongsInitialized;
extern bool isReordering;
extern int  reorderTarget;

// Scanning state for songs received via SysEx
extern bool songsReady;  // Signals that the end-of-song-list SysEx has been received

extern unsigned long lastPingReplyTime;

extern bool webServerStarted;  

// UI Navigation and Menu variables

extern int currentSongItem;
extern int scrollOffset;       // For scrolling lists on the display
extern int totalTracks;        // Total songs in loaded preset
extern int currentTrack;       // Currently active track
extern bool needsRedraw;
extern int selectedHomeSongIndex;

extern int currentMenuItem;
extern int currentMenu2Item;   // Highlight index for Menu2
extern int currentWifiMenuItem;   // Highlight index for Wi-Fi sub-menu

// Menu selection enumeration
enum MenuSelection { MENU1_SELECTED, MENU2_SELECTED };
extern MenuSelection currentMenuSelection;

extern int menu1Index;

extern byte currentVolume;      // Current volume level
extern bool isPlaying;

// On-screen keyboard variables
extern const char* specialKeys[];
extern const char keys[4][10];
extern bool isShifted;
extern char setlistName[MAX_SONG_NAME_LEN + 1];

// Button and encoder state tracking
extern bool BtnStartState, BtnStopState;
extern bool BtnStartLastState, BtnStopLastState;
extern volatile int encoderPosition;
extern bool encoderLastState;
extern bool encoderButtonState;

// WiFi variables
extern String wifiSSIDs[MAX_WIFI_NETWORKS];
extern int wifiRSSI[MAX_WIFI_NETWORKS];
extern int wifiCount;  // Number of networks found
extern bool wifiScanInProgress;
extern bool wifiScanDone;
extern char wifiPassword[MAX_WIFI_PASS_LEN + 1];
extern String selectedSSID;
extern int wifiScrollOffset;
extern int wifiCurrentItem;
extern bool wifiFullyConnected;

// -----------------------------------------------------
// String Arrays in PROGMEM for Menu Items
// -----------------------------------------------------
static const char menuItems[NUM_MENU_ITEMS][16] PROGMEM = {
  "NEW SETLIST", "SELECT SETLIST", "EDIT SETLIST", "DELETE SETLIST", "EXIT"
};

static const char menu2Items[NUM_MENU2_ITEMS][16] PROGMEM = {
  "WiFi Settings", "Device Settings", "Back"
};

static const char wifiMenuItems[NUM_WIFI_MENU_ITEMS][16] PROGMEM = {
  "WiFi Connect", "WiFi Properties", "Back"
};

// -----------------------------------------------------
// Structure Definitions
// -----------------------------------------------------
struct SongInfo {
    byte  songIndex;        // Original song index from SysEx
    int   changedIndex;     // Index after reordering
    char  songName[MAX_SONG_NAME_LEN + 1]; // Song name (null terminated)
    float locatorTime;      // Locator time in seconds

    // Parse SysEx data for a single song.
    bool getInfo(const byte *data, unsigned length);
};

struct ProjectInfo {
    char     projectName[MAX_SONG_NAME_LEN + 1];
    SongInfo songs[MAX_SONGS];
    int      songCount;
};

struct Preset {
    char        name[MAX_SONG_NAME_LEN + 1]; // User-chosen setlist name
    ProjectInfo data;                        // Underlying project and songs
};

// -----------------------------------------------------
// Global Structure Instances
// -----------------------------------------------------
extern Preset loadedPreset;          // The currently loaded preset
extern ProjectInfo selectedProject;    // User-selected subset of songs for editing
extern ProjectInfo currentProject;

#endif // CONFIG_H
