#ifndef _UI_H
#define _UI_H

#include "config.h"
#include <Adafruit_ILI9341.h>
#include <XPT2046_Touchscreen.h>
#include <SPI.h>
#include "_midi.h"
#include "_preset.h"
#include "_webserver.h"

// Define an enumeration for your screen states.
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
    WIFI_PROPERTIES
};

class UI {
public:

    Adafruit_ILI9341 &getDisplay() { return tft; }
    XPT2046_Touchscreen &getTouchscreen() { return ts; }
    int getCurrentHomeMenuSelection() const { return currentHomeMenuSelection; }
    int getCurrentMenuItem() const { return currentMenuItem; }


    int currentPresetIndex = 0;      // The currently selected preset index (0-indexed)
    int presetScrollOffset = 0;      // Scroll offset for the preset list
    // You can change the initial values as needed.
    // Constructor: Provide a reference to the display object.
    UI();

    void init();

    // Change and update the current screen state.
    void setScreenState(ScreenState newState);
    ScreenState getScreenState() const;
    ScreenState getPreviousScreenState() const;
    void restorePreviousScreenState();
    void updateScreen();

    void displayVolume();
    void drawHomeMenuBox();
    void updateHomeMenuSelection(int delta);
    void drawMenuItem(int index, bool selected);
    void updateMenuSelection(int previousIndex, int currentIndex);
    void drawMenu2Item(int index, bool selected);
    void updateMenu2Selection(int previousIndex, int currentIndex);
    void drawWifiMenuItem(int index, bool selected);
    void updateWifiMenuSelection(int previousIndex, int currentIndex);
    void drawSongList();
    void drawSongListItem(int absoluteIndex, bool selected);
    void updateSelectedSongCount();
    void drawEditedSongList();
    void drawEditedSongListItem(int absoluteIndex, bool highlighted);
    void drawPresetList();
    void drawPresetListItem(int index, bool highlighted);
    void drawSetlistName();
    void drawLoadedPreset();

    void drawWiFiList();
    void drawWiFiListItem(int absoluteIndex, bool highlighted);
    void checkWifiConnection();
    void WifiScan();
    void drawWiFiPassword();

    // Screen state handlers—each function draws a particular screen.
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

    bool getTouchCoordinates(int16_t &x, int16_t &y);

    bool isTouch(int16_t x, int16_t y, int16_t areaX, int16_t areaY, int16_t width, int16_t height);

private:
    Adafruit_ILI9341 tft;  // Reference to the display.
    XPT2046_Touchscreen ts;
    ScreenState currentState;  // Current screen state.
    ScreenState previousState;

    int currentHomeMenuSelection = 0;

        // Basic UI drawing functions.
    void drawText(const char* text, int16_t x, int16_t y, uint16_t color = ILI9341_WHITE, uint8_t size = 2);
    void drawTextCenter(const String &text, int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
    void drawTextTopCenter(const char *text, int16_t yOffset = 7, bool underline = true, uint16_t textColor = ILI9341_WHITE);
    void drawRectButton(int16_t x, int16_t y, int16_t w, int16_t h, const char* label, uint16_t color = ILI9341_WHITE);
    void drawKeyboard(bool shifted);
};

#endif
