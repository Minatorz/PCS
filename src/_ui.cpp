#include "_ui.h"

AsyncWebServerManager webServerManager(80);

// Constructor: Initialize with the provided display.
UI::UI() :  tft(TFT_CS, TFT_DC, TFT_RST) , 
            ts(T_CS, T_IRQ) ,
            currentState(ScreenState::LOADING),
            previousState(ScreenState::LOADING) {}

// Set a new screen state and update the display.
void UI::setScreenState(ScreenState newState) {
    previousState = currentState;
    currentState = newState;
    updateScreen();
}

void UI::init() {
    if (!ts.begin()) {
        Serial.println(F("Touchscreen Initialization Failed!"));
        while (1);
    }
    tft.begin();
    tft.setRotation(1);
}

ScreenState UI::getScreenState() const {
  return currentState;
}

ScreenState UI::getPreviousScreenState() const {
    return previousState;
}

void UI::restorePreviousScreenState() {
    currentState = previousState;
    updateScreen();
}

// Call the appropriate screen handler based on currentState.
void UI::updateScreen() {
  switch (currentState) {
    case ScreenState::LOADING:
      loadingScreen();
      break;
    case ScreenState::HOME:
      homeScreen();
      break;
    case ScreenState::MENU1:
      menuScreen();
      break;
    case ScreenState::NEW_SETLIST:
      newSetlistScreen();
      break;
    case ScreenState::EDIT_SETLIST:
      editSetlistScreen();
      break;
    case ScreenState::SELECT_SETLIST:
      selectSetlistScreen();
      break;
    case ScreenState::SAVE_SETLIST:
      saveSetlistScreen();
      break;
    case ScreenState::MENU2:
      menu2Screen();
      break;
    case ScreenState::MENU2_WIFISETTINGS:
      menu2WifiSettingsScreen();
      break;
    case ScreenState::MENU2_WIFICONNECT:
      menu2WifiConnectScreen();
      break;
    case ScreenState::MENU2_WIFIPASS:
      menu2WifiPasswordScreen();
      break;
    case ScreenState::MENU2_WIFICONFIRM:
      menu2WifiConfirmScreen();
      break;
    case ScreenState::MENU2_WIFICONNECTING:
      menu2WifiConnectingScreen();
      break;
    case ScreenState::DEVICE_PROPERTIES:
      devicePropertiesScreen();
      break;
    case ScreenState::WIFI_PROPERTIES:
      wifiPropertiesScreen();
      break;
    default:
      homeScreen(); // Default fallback
      break;
  }
}

// --- Basic UI Drawing Functions ---

void UI::drawText(const char* text, int16_t x, int16_t y, uint16_t color, uint8_t size) {
  tft.setTextColor(color);
  tft.setTextSize(size);
  tft.setCursor(x, y);
  tft.print(text);
}

void UI::drawTextCenter(const String &text, int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    int16_t tw = text.length() * 6 * 2; // rough width for size 2 text
    int16_t th = 16;                   // approximate text height for size 2
    int16_t xc = x + (w - tw) / 2;
    int16_t yc = y + (h - th) / 2;
    tft.setCursor(xc, yc);
    tft.setTextColor(color);
    tft.setTextSize(2);
    tft.print(text);
}

void UI::drawTextTopCenter(const char *text, int16_t yOffset, bool underline , uint16_t textColor) {
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

void UI::drawRectButton(int16_t x, int16_t y, int16_t w, int16_t h, const char* label, uint16_t color) {
  tft.drawRect(x, y, w, h, color);
  drawTextCenter(String(label), x, y, w, h, color);
}

void UI::drawKeyboard(bool shifted) {
    int keyStartY = 88;              // Starting Y position for the keyboard
    int keyHeight = 30;              // Height of each key
    int keyWidth  = tft.width() / 10; // Divide the display width evenly for 10 keys

    // Clear the keyboard area
    tft.fillRect(0, keyStartY, tft.width(), 5 * keyHeight, ILI9341_BLACK);

    // Draw 4 rows of keys (using the keys array defined in config)
    for (int row = 0; row < 4; row++) {
        for (int col = 0; col < 10; col++) {
            int x = col * keyWidth;
            int y = keyStartY + row * keyHeight;
            // Draw key border
            tft.drawRect(x, y, keyWidth, keyHeight, ILI9341_WHITE);
            // Select the character: for rows after the first, if shifted then uppercase.
            char displayKey = keys[row][col];
            if (shifted && row > 0) {
                displayKey = toupper(displayKey);
            }
            // Draw the character centered in the key.
            drawTextCenter(String(displayKey), x, y, keyWidth, keyHeight, ILI9341_WHITE);
        }
    }

    // Draw the 5th row for special keys:
    // We assume specialKeys[0] is for Shift, specialKeys[1] for Space, and specialKeys[2] for Backspace.
    drawRectButton(0, keyStartY + 4 * keyHeight, 2 * keyWidth, keyHeight, specialKeys[0]);
    drawRectButton(2 * keyWidth, keyStartY + 4 * keyHeight, 6 * keyWidth, keyHeight, specialKeys[1]);
    drawRectButton(8 * keyWidth, keyStartY + 4 * keyHeight, 2 * keyWidth, keyHeight, specialKeys[2]);

    Serial.println("Keyboard drawn.");
}

void UI::displayVolume() {
    static bool firstCall = true;
    static byte lastVolume = 0;
    int barX = 100;
    int barY = 45;
    int barWidth = 80;
    int barHeight = 25;

    // On the very first call, perform a full draw.
    if (firstCall) {
        lastVolume = currentVolume;
        firstCall = false;
        int filledWidth = map(currentVolume, 0, 127, 0, barWidth);
        // Draw the filled portion.
        tft.fillRect(barX, barY, filledWidth, barHeight, ILI9341_GREEN);
        // Clear the remaining area.
        tft.fillRect(barX + filledWidth, barY, barWidth - filledWidth, barHeight, ILI9341_BLACK);
        // Draw the border.
        tft.drawRect(barX, barY, barWidth, barHeight, ILI9341_WHITE);
        return;
    }

    // Calculate the filled widths for current and previous volumes.
    int filledWidth = map(currentVolume, 0, 127, 0, barWidth);
    int lastFilledWidth = map(lastVolume, 0, 127, 0, barWidth);

    if (filledWidth > lastFilledWidth) {
        // Volume increased: fill only the extra portion.
        tft.fillRect(barX + lastFilledWidth, barY, filledWidth - lastFilledWidth, barHeight, ILI9341_GREEN);
    } else if (filledWidth < lastFilledWidth) {
        // Volume decreased: clear only the removed portion.
        tft.fillRect(barX + filledWidth, barY, lastFilledWidth - filledWidth, barHeight, ILI9341_BLACK);
    }
    
    // Always redraw the border.
    tft.drawRect(barX, barY, barWidth, barHeight, ILI9341_WHITE);
  
    lastVolume = currentVolume;
}

void UI::drawWiFiList() {
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

  void UI::checkWifiConnection() {
    if (currentState == ScreenState::MENU2_WIFICONNECTING) {
        if (WiFi.status() == WL_CONNECTED && !wifiFullyConnected) {
            wifiFullyConnected = true;
            digitalWrite(BUILTIN_LED, HIGH);
            tft.fillScreen(ILI9341_BLACK);
            drawTextTopCenter("Connected!", 7, true, ILI9341_GREEN);
            drawText("Successfully connected to: ", 10, 60, ILI9341_WHITE, 2);
            drawText(selectedSSID.c_str(), 10, 80, ILI9341_YELLOW, 2);

            static bool webServerSetupDone = false;
            if (!webServerSetupDone) {
                webServerManager.setup();
                webServerSetupDone = true;
            }
            // webServerManager.update(); // This call is optional here.

            delay(2000);
            setScreenState(ScreenState::HOME);
        }
    }
}

// --- INTERFACE ---

void UI::loadingScreen() {
    tft.fillScreen(ILI9341_BLACK);
    drawText("AbletonThesis", 20, 60, ILI9341_GREEN, 3);
    drawText("Loading...", 40, 120, ILI9341_WHITE, 2);
    for (int i = 0; i <= 260; i += 20) {
        tft.fillRect(20, 160, i, 10, ILI9341_BLUE);
        delay(200);
    }
    setScreenState(ScreenState::HOME);
}

void UI::homeScreen() {
    Serial.println("HomePage");
    tft.fillScreen(ILI9341_BLACK);
    drawText("Home Page", 10, 20, ILI9341_WHITE, 2);
    tft.drawRect(BOX1_X, BOX1_Y, BOX_WIDTH, BOX_HEIGHT, ILI9341_WHITE);
    drawTextCenter("1", BOX1_X, BOX1_Y, BOX_WIDTH, BOX_HEIGHT, ILI9341_WHITE);
    tft.drawRect(BOX2_X, BOX2_Y, BOX_WIDTH, BOX_HEIGHT, ILI9341_WHITE);
    drawTextCenter("2", BOX2_X, BOX2_Y, BOX_WIDTH, BOX_HEIGHT, ILI9341_WHITE);

    UI::drawText("Volume: " ,10, 50, ILI9341_WHITE, 2);

    if (selectedPresetSlot != -1) {
        if (presetChanged) {
            totalTracks = loadedPreset.data.songCount;
            presetChanged = false;
        }
        if (strcmp(loadedPreset.name, "No Preset") == 0 ||
            strlen(loadedPreset.data.projectName) == 0 ||
            totalTracks == 0) {
            drawText("No tracks available.", 10, 80, ILI9341_RED, 3);
        } else {
            drawText((String("Preset: ") + loadedPreset.name).c_str(), 10, 80, ILI9341_YELLOW, 2);
            uint16_t songColor = isPlaying ? ILI9341_GREEN : ILI9341_WHITE;
            drawText(String(loadedPreset.data.songs[currentTrack].songName).c_str(),
                        10, 100, songColor, 3);
            drawText((String(currentTrack + 1) + "/" + String(totalTracks)).c_str(),
                       tft.width() - 60, 100, ILI9341_WHITE, 2);
        }
        // notifyPresetUpdate();
    } else {
        drawText("No Preset Selected", 10, 80, ILI9341_RED, 2);
    }
}

void UI::menuScreen() {
    tft.fillScreen(ILI9341_BLACK);
    drawTextTopCenter("SETLIST SETTINGS", 7, true, ILI9341_WHITE);
    for (int i = 0; i < NUM_MENU_ITEMS; i++) {
        int y = 60 + i * 30;
        tft.fillRect(10, y, 220, 30, ILI9341_BLACK);
        tft.setCursor(15, y + 5);
        tft.setTextColor((i == currentMenuItem) ? ILI9341_YELLOW : ILI9341_WHITE);
        tft.setTextSize(2);
        char buffer[16];
        strncpy_P(buffer, menuItems[i], sizeof(buffer));
        tft.print(buffer);
    }
}

void UI::newSetlistScreen() {
    tft.fillScreen(ILI9341_BLACK);
    drawRectButton(BACK_BUTTON_X, BACK_BUTTON_Y, BACK_BUTTON_WIDTH, BACK_BUTTON_HEIGHT, "Back");
    drawRectButton(NEXT_BUTTON_X, NEXT_BUTTON_Y, NEXT_BUTTON_WIDTH, NEXT_BUTTON_HEIGHT, "Next");
    drawTextTopCenter("New Setlist", 7, true, ILI9341_WHITE);

    // Send SysEx and wait for song list if not already scanned
    if (!newSetlistScanned) {
        unsigned long startTime = millis();
        Midi::Scan();
        Serial.println(F("Sent SysEx to Notify Ableton"));

        memset(&currentProject, 0, sizeof(currentProject));
        currentProject.songCount = 0;

        while ((currentProject.songCount == 0 || !songsReady) &&
            (millis() - startTime < 7000)) {
        delay(50);  // Wait for up to 7 seconds
        }
        if (!songsReady) {
        Serial.println(F("⏳ Timeout! Songs not fully received."));
        drawText("Error: Incomplete setlist", 10, 60, ILI9341_RED, 2);
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
        tft.setTextColor(ILI9341_CYAN, ILI9341_BLACK);
        } else if (selectedSongs[idx]) {
        tft.setTextColor(ILI9341_GREEN, ILI9341_BLACK);
        } else if (idx == currentMenuItem) {
        tft.setTextColor(ILI9341_YELLOW, ILI9341_BLACK);
        } else {
        tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
        }
        tft.print(idx + 1);
        tft.print(". ");
        tft.print(currentProject.songs[idx].songName);
    }
    char buff[32];
    snprintf(buff, sizeof(buff), "%d / %d", selectedTrackCount, currentProject.songCount);
    UI::drawTextTopCenter(buff, 35, false, ILI9341_WHITE);
}

void UI::editSetlistScreen() {
    tft.fillScreen(ILI9341_BLACK);
    drawRectButton(BACK_BUTTON_X, BACK_BUTTON_Y, BACK_BUTTON_WIDTH, BACK_BUTTON_HEIGHT, "Back");
    drawRectButton(SAVE_BUTTON_X, SAVE_BUTTON_Y, SAVE_BUTTON_WIDTH, SAVE_BUTTON_HEIGHT, "Save");
    drawTextTopCenter("Edit Setlist", 7, true, ILI9341_WHITE);
    
        if (selectedTrackCount == 0) {
            tft.setCursor(10, 60);
            tft.setTextColor(ILI9341_WHITE);
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
            tft.setTextColor(ILI9341_GREEN, ILI9341_BLACK);
            } else if (!isReordering && trackIndex == currentMenuItem) {
            tft.setTextColor(ILI9341_YELLOW, ILI9341_BLACK);
            } else {
            tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
            }
            tft.setCursor(10, y);
            tft.setTextSize(2);
            tft.print(trackIndex + 1);
            tft.print(". ");
            tft.print(selectedProject.songs[trackIndex].songName);
        }
        scrollOffset = constrain(scrollOffset, 0, max(0, selectedProject.songCount - itemsPerPage));
}

void UI::selectSetlistScreen() {
    tft.fillScreen(ILI9341_BLACK);
    drawTextTopCenter("Select Preset", 7, true, ILI9341_WHITE);
    drawRectButton(BACK_BUTTON_X, BACK_BUTTON_Y, BACK_BUTTON_WIDTH, BACK_BUTTON_HEIGHT, "Back");
    drawRectButton(SAVE_BUTTON_X, SAVE_BUTTON_Y, SAVE_BUTTON_WIDTH, SAVE_BUTTON_HEIGHT, "Save");

    preferences.begin("Setlists", true);
    for (int i = 1; i <= MAX_PRESETS; i++) {
        int y = 50 + (i - 1) * 30;
        tft.setCursor(10, y);
        tft.setTextColor((i - 1) == currentMenuItem ? ILI9341_YELLOW : ILI9341_WHITE);
        tft.setTextSize(2);
        String presetName = preferences.getString(("p" + String(i) + "_name").c_str(), "No Preset");
        tft.print(i);
        tft.print(". ");
        tft.print(presetName);
    }
    preferences.end();
}

void UI::saveSetlistScreen() {
    tft.fillScreen(ILI9341_BLACK);
    drawRectButton(BACK_BUTTON_X, BACK_BUTTON_Y, BACK_BUTTON_WIDTH, BACK_BUTTON_HEIGHT, "Back");
    drawRectButton(SAVE_BUTTON_X, SAVE_BUTTON_Y, SAVE_BUTTON_WIDTH, SAVE_BUTTON_HEIGHT, "Save");
    drawTextTopCenter("Save Setlist", 7, true, ILI9341_WHITE);

    if (selectedPresetSlot == -1) {
        UI::drawText("No Slot Selected", 10, 50, ILI9341_RED, 2);
        return;
    }

    // Draw setlist name input box and show typed name
    tft.drawRect(10, 50, tft.width() - 20, 30, ILI9341_WHITE);
    tft.setCursor(15, 57);
    tft.setTextColor(ILI9341_WHITE);
    tft.setTextSize(2);
    tft.print(setlistName);

    // Draw on-screen keyboard
    drawKeyboard(isShifted);
}

void UI::menu2Screen() {
    tft.fillScreen(ILI9341_BLACK);
    drawTextTopCenter("System Settings", 7, true, ILI9341_WHITE);
    for (int i = 0; i < NUM_MENU2_ITEMS; i++) {
        int y = 60 + i * 30;
        tft.setCursor(15, y + 5);
        tft.setTextSize(2);
        tft.setTextColor((i == currentMenu2Item) ? ILI9341_YELLOW : ILI9341_WHITE);
        char buffer[16];
        strncpy_P(buffer, menu2Items[i], sizeof(buffer));
        tft.print(buffer);
    }
}

void UI::menu2WifiSettingsScreen() {
    tft.fillScreen(ILI9341_BLACK);
    drawTextTopCenter("Wi-Fi Settings", 7, true, ILI9341_WHITE);
    for (int i = 0; i < NUM_WIFI_MENU_ITEMS; i++) {
        int y = 60 + i * 30;
        tft.setCursor(15, y + 5);
        tft.setTextSize(2);
        tft.setTextColor((i == currentWifiMenuItem) ? ILI9341_YELLOW : ILI9341_WHITE);
        char buffer[16];
        strncpy_P(buffer, wifiMenuItems[i], sizeof(buffer));
        tft.print(buffer);
    }
}

void UI::menu2WifiConnectScreen() {
    // Draw the static portion (header and navigation buttons) only once.
    static bool staticDrawn = false;
    if (!staticDrawn) {
        tft.fillScreen(ILI9341_BLACK);
        drawTextTopCenter("Wi-Fi Connect", 7, true, ILI9341_WHITE);
        drawRectButton(BACK_BUTTON_X, BACK_BUTTON_Y, BACK_BUTTON_WIDTH, BACK_BUTTON_HEIGHT, "Back");
        drawRectButton(NEXT_BUTTON_X, NEXT_BUTTON_Y, NEXT_BUTTON_WIDTH, NEXT_BUTTON_HEIGHT, "Rescan");
        staticDrawn = true;
    }

    // Start a new Wi‑Fi scan if one hasn't been started.
    if (!wifiScanInProgress && !wifiScanDone) {
        Serial.println(F("Starting Async Wi-Fi Scan..."));
        WiFi.mode(WIFI_STA);
        WiFi.disconnect(true);
        delay(100);
        WiFi.scanNetworks(true);
        wifiScanInProgress = true;
    }

    // Poll for scan results while in progress.
    if (wifiScanInProgress) {
        int scanResult = WiFi.scanComplete();
        Serial.printf("Scan result: %d\n", scanResult);  // Debug print
        if (scanResult >= 0) {  // Scan complete
            wifiCount = (scanResult < MAX_WIFI_NETWORKS) ? scanResult : MAX_WIFI_NETWORKS;
            for (int i = 0; i < wifiCount; i++) {
                wifiSSIDs[i] = WiFi.SSID(i);
                wifiRSSI[i] = WiFi.RSSI(i);
            }
            wifiScanDone = true;
            wifiScanInProgress = false;
        } else {
            // Clear only the dynamic region and show a "Scanning..." message.
            tft.fillRect(10, 60, tft.width() - 20, 160, ILI9341_BLACK);
            drawText("Scanning...", 10, 60, ILI9341_YELLOW, 2);
            return;  // Exit until the scan completes.
        }
    }

    // Once the scan is complete, update only the dynamic region (Wi‑Fi list).
    if (wifiScanDone) {
        tft.fillRect(10, 60, tft.width() - 20, 160, ILI9341_BLACK);
        drawWiFiList();
    }
}

void UI::menu2WifiPasswordScreen() {
    tft.fillScreen(ILI9341_BLACK);
    drawTextTopCenter("Enter Password", 7, true, ILI9341_WHITE);
    drawTextTopCenter(selectedSSID.c_str(), 27, false, ILI9341_YELLOW);
    drawRectButton(BACK_BUTTON_X, BACK_BUTTON_Y, BACK_BUTTON_WIDTH, BACK_BUTTON_HEIGHT, "Back");
    drawRectButton(SAVE_BUTTON_X, SAVE_BUTTON_Y, SAVE_BUTTON_WIDTH, SAVE_BUTTON_HEIGHT, "OK");

    tft.drawRect(10, 50, tft.width() - 20, 30, ILI9341_WHITE);
    tft.setCursor(15, 57);
    tft.setTextSize(2);
    tft.setTextColor(ILI9341_WHITE);
    tft.print(wifiPassword);

    drawKeyboard(isShifted);
}

void UI::menu2WifiConfirmScreen() {
    tft.fillScreen(ILI9341_BLACK);
    drawTextTopCenter("Confirmation", 7, true, ILI9341_WHITE);
    drawText("SSID: ", 10, 50, ILI9341_WHITE, 2);
    drawText(selectedSSID.c_str(), 85, 50, ILI9341_YELLOW, 2);
    drawText("Password: ", 10, 80, ILI9341_WHITE, 2);
    drawText(wifiPassword, 130, 80, ILI9341_CYAN, 2);
    drawRectButton(BACK_BUTTON_X, BACK_BUTTON_Y, BACK_BUTTON_WIDTH, BACK_BUTTON_HEIGHT, "Back");
    drawRectButton(SAVE_BUTTON_X, SAVE_BUTTON_Y, SAVE_BUTTON_WIDTH, SAVE_BUTTON_HEIGHT, "OK");
}

void UI::menu2WifiConnectingScreen() {
    tft.fillScreen(ILI9341_BLACK);
    drawTextTopCenter("Connecting to Wi-Fi", 7, true, ILI9341_WHITE);
    drawText("SSID: ", 10, 60, ILI9341_WHITE, 2);
    drawText(selectedSSID.c_str(), 85, 60, ILI9341_YELLOW, 2);
    drawText("Please wait...", 10, 100, ILI9341_WHITE, 2);
}

void UI::devicePropertiesScreen() {
tft.fillScreen(ILI9341_BLACK);
    drawTextTopCenter("Properties", 7, true, ILI9341_WHITE);

    // Example device properties (customize as needed)
    drawText("Device: AbletonThesis", 10, 50, ILI9341_WHITE, 2);
    drawText("Firmware: v1.0", 10, 80, ILI9341_WHITE, 2);
    // drawText("Chip Model: " + String(ESP.getChipModel()), 10, 110, ILI9341_WHITE, 2);
    // drawText("Chip Revision: " + String(ESP.getChipRevision()), 10, 140, ILI9341_WHITE, 2);
    // drawText("Free Heap: " + String(ESP.getFreeHeap()), 10, 170, ILI9341_WHITE, 2);

    // Add more information as needed

    // Draw a Back button to return to the previous menu
    drawRectButton(BACK_BUTTON_X, BACK_BUTTON_Y, BACK_BUTTON_WIDTH, BACK_BUTTON_HEIGHT, "Back");
}

void UI::wifiPropertiesScreen() {
    tft.fillScreen(ILI9341_BLACK);
    drawTextTopCenter("Wi-Fi Properties", 7, true, ILI9341_WHITE);

    // Gather Wi-Fi details
    String ssid = WiFi.SSID();
    String ip = WiFi.localIP().toString();
    int rssi = WiFi.RSSI();
    String status = (WiFi.status() == WL_CONNECTED) ? "Connected" : "Disconnected";

    // Display the details
    drawText(("SSID: " + ssid).c_str(), 10, 50, ILI9341_WHITE, 2);
    drawText(("IP: " + ip).c_str(), 10, 80, ILI9341_WHITE, 2);
    drawText(("RSSI: " + String(rssi) + " dBm").c_str(), 10, 110, ILI9341_WHITE, 2);
    drawText(("Status: " + status).c_str(), 10, 140, ILI9341_WHITE, 2);

    // Back button to return to Wi-Fi settings menu
    drawRectButton(BACK_BUTTON_X, BACK_BUTTON_Y, BACK_BUTTON_WIDTH, BACK_BUTTON_HEIGHT, "Back");
}

bool UI::getTouchCoordinates(int16_t &x, int16_t &y) {
    if (ts.touched()) {
        TS_Point point = ts.getPoint();
        // Adjust calibration values as needed.
        x = map(point.x, 3800, 300, 0, tft.width());
        y = map(point.y, 3800, 300, 0, tft.height());
        return true;
    }
    return false;
}

// isTouch() checks if (x, y) is inside the given rectangle.
bool UI::isTouch(int16_t x, int16_t y, int16_t areaX, int16_t areaY, int16_t width, int16_t height) {
    return (x >= areaX && x < (areaX + width) && y >= areaY && y < (areaY + height));
}