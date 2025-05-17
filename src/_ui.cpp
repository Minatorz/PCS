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
    // Define overall dimensions of the volume bar (same as used in homeScreen).
    int barX = 50;
    int barY = 10;
    int barWidth = 80;
    int barHeight = 25;
    
    // Define the interior (offset by 1 pixel on all sides to avoid redrawing the border).
    int interiorX = barX + 1;
    int interiorY = barY + 1;
    int interiorWidth = barWidth - 2;
    int interiorHeight = barHeight - 2;
    
    // Map the current and previous volume values to interior width.
    static byte lastVolume = currentVolume;  // Initialize with currentVolume.
    int filledWidth = map(currentVolume, 0, 127, 0, interiorWidth);
    int lastFilledWidth = map(lastVolume, 0, 127, 0, interiorWidth);

    // Update only the interior based on volume changes.
    if (filledWidth > lastFilledWidth) {
        // Volume increased: fill the extra portion in green.
        tft.fillRect(interiorX + lastFilledWidth, interiorY, filledWidth - lastFilledWidth, interiorHeight, ILI9341_GREEN);
    } else if (filledWidth < lastFilledWidth) {
        // Volume decreased: clear the portion that is no longer filled.
        tft.fillRect(interiorX + filledWidth, interiorY, lastFilledWidth - filledWidth, interiorHeight, ILI9341_BLACK);
    } else {
        // On the first call or if no change, ensure the interior is drawn correctly.
        tft.fillRect(interiorX, interiorY, filledWidth, interiorHeight, ILI9341_GREEN);
        if (filledWidth < interiorWidth) {
            tft.fillRect(interiorX + filledWidth, interiorY, interiorWidth - filledWidth, interiorHeight, ILI9341_BLACK);
        }
    }
    
    // Update lastVolume for future comparisons.
    lastVolume = currentVolume;
}

void UI::drawHomeMenuBox() {
    // Draw Box 1
    if (currentHomeMenuSelection == 0) {
        tft.drawRect(BOX1_X, BOX1_Y, BOX_WIDTH, BOX_HEIGHT, ILI9341_YELLOW);
        drawTextCenter("1", BOX1_X, BOX1_Y, BOX_WIDTH, BOX_HEIGHT, ILI9341_YELLOW);
    } else {
        tft.drawRect(BOX1_X, BOX1_Y, BOX_WIDTH, BOX_HEIGHT, ILI9341_WHITE);
        drawTextCenter("1", BOX1_X, BOX1_Y, BOX_WIDTH, BOX_HEIGHT, ILI9341_WHITE);
    }

    // Draw Box 2
    if (currentHomeMenuSelection == 1) {
        tft.drawRect(BOX2_X, BOX2_Y, BOX_WIDTH, BOX_HEIGHT, ILI9341_YELLOW);
        drawTextCenter("2", BOX2_X, BOX2_Y, BOX_WIDTH, BOX_HEIGHT, ILI9341_YELLOW);
    } else {
        tft.drawRect(BOX2_X, BOX2_Y, BOX_WIDTH, BOX_HEIGHT, ILI9341_WHITE);
        drawTextCenter("2", BOX2_X, BOX2_Y, BOX_WIDTH, BOX_HEIGHT, ILI9341_WHITE);
    }
}

void UI::updateHomeMenuSelection(int delta) {
    int previousSelection = currentHomeMenuSelection;
    // Update selection with wrapping (assuming two boxes)
    currentHomeMenuSelection += delta;
    if (currentHomeMenuSelection < 0)
        currentHomeMenuSelection = 1;
    else if (currentHomeMenuSelection > 1)
        currentHomeMenuSelection = 0;

    if (previousSelection != currentHomeMenuSelection) {
        // Redraw the box that lost selection.
        if (previousSelection == 0) {
            // Clear and redraw Box 1 un-highlighted.
            tft.fillRect(BOX1_X - 1, BOX1_Y - 1, BOX_WIDTH + 2, BOX_HEIGHT + 2, ILI9341_BLACK);
            tft.drawRect(BOX1_X, BOX1_Y, BOX_WIDTH, BOX_HEIGHT, ILI9341_WHITE);
            drawTextCenter("1", BOX1_X, BOX1_Y, BOX_WIDTH, BOX_HEIGHT, ILI9341_WHITE);
        } else if (previousSelection == 1) {
            // Clear and redraw Box 2 un-highlighted.
            tft.fillRect(BOX2_X - 1, BOX2_Y - 1, BOX_WIDTH + 2, BOX_HEIGHT + 2, ILI9341_BLACK);
            tft.drawRect(BOX2_X, BOX2_Y, BOX_WIDTH, BOX_HEIGHT, ILI9341_WHITE);
            drawTextCenter("2", BOX2_X, BOX2_Y, BOX_WIDTH, BOX_HEIGHT, ILI9341_WHITE);
        }

        // Redraw the box that gained selection.
        if (currentHomeMenuSelection == 0) {
            tft.fillRect(BOX1_X - 1, BOX1_Y - 1, BOX_WIDTH + 2, BOX_HEIGHT + 2, ILI9341_BLACK);
            tft.drawRect(BOX1_X, BOX1_Y, BOX_WIDTH, BOX_HEIGHT, ILI9341_YELLOW);
            drawTextCenter("1", BOX1_X, BOX1_Y, BOX_WIDTH, BOX_HEIGHT, ILI9341_YELLOW);
        } else if (currentHomeMenuSelection == 1) {
            tft.fillRect(BOX2_X - 1, BOX2_Y - 1, BOX_WIDTH + 2, BOX_HEIGHT + 2, ILI9341_BLACK);
            tft.drawRect(BOX2_X, BOX2_Y, BOX_WIDTH, BOX_HEIGHT, ILI9341_YELLOW);
            drawTextCenter("2", BOX2_X, BOX2_Y, BOX_WIDTH, BOX_HEIGHT, ILI9341_YELLOW);
        }
    }
}

void UI::drawMenuItem(int index, bool selected) {
    // Calculate the y-coordinate for this menu item.
    int y = 60 + index * 30;
    
    // Clear the menu item region (we assume a width of 220 and height of 30).
    tft.fillRect(10, y, 220, 30, ILI9341_BLACK);
    
    // Set the cursor position (adjust as needed).
    tft.setCursor(15, y + 5);
    
    // If using an external font, set it here:
    // tft.setFont(&FreeSans9pt7b);
    
    // Set the text color based on selection.
    tft.setTextColor(selected ? ILI9341_YELLOW : ILI9341_WHITE);
    
    // Set text size.
    tft.setTextSize(2);
    
    // Copy the menu item text (assuming menuItems is stored in PROGMEM).
    char buffer[16];
    strncpy_P(buffer, menuItems[index], sizeof(buffer));
    buffer[sizeof(buffer) - 1] = '\0'; // ensure null-termination
    tft.print(buffer);
}

void UI::updateMenuSelection(int previousIndex, int currentIndex) {
    // Redraw the previous menu item without highlighting.
    drawMenuItem(previousIndex, false);
    
    // Redraw the new menu item with highlighting.
    drawMenuItem(currentIndex, true);
}

void UI::drawMenu2Item(int index, bool selected) {
    // Calculate the y-coordinate for this menu item.
    int y = 60 + index * 30;
    
    // Clear the menu item region (we assume a width of 220 and height of 30).
    tft.fillRect(10, y, 220, 30, ILI9341_BLACK);
    
    // Set the cursor position (adjust as needed).
    tft.setCursor(15, y + 5);
    
    // If using an external font, set it here:
    // tft.setFont(&FreeSans9pt7b);
    
    // Set the text color based on selection.
    tft.setTextColor(selected ? ILI9341_YELLOW : ILI9341_WHITE);
    
    // Set text size.
    tft.setTextSize(2);
    
    // Copy the menu item text (assuming menuItems is stored in PROGMEM).
    char buffer[16];
    strncpy_P(buffer, menu2Items[index], sizeof(buffer));
    buffer[sizeof(buffer) - 1] = '\0'; // ensure null-termination
    tft.print(buffer);
}

void UI::updateMenu2Selection(int previousIndex, int currentIndex) {
    // Redraw the previous menu item without highlighting.
    drawMenu2Item(previousIndex, false);
    
    // Redraw the new menu item with highlighting.
    drawMenu2Item(currentIndex, true);
}

void UI::drawWifiMenuItem(int index, bool selected) {
    // Calculate the y-coordinate for this menu item.
    int y = 60 + index * 30;
    
    // Clear the menu item region (we assume a width of 220 and height of 30).
    tft.fillRect(10, y, 220, 30, ILI9341_BLACK);
    
    // Set the cursor position (adjust as needed).
    tft.setCursor(15, y + 5);
    
    // If using an external font, set it here:
    // tft.setFont(&FreeSans9pt7b);
    
    // Set the text color based on selection.
    tft.setTextColor(selected ? ILI9341_YELLOW : ILI9341_WHITE);
    
    // Set text size.
    tft.setTextSize(2);
    
    // Copy the menu item text (assuming menuItems is stored in PROGMEM).
    char buffer[16];
    strncpy_P(buffer, wifiMenuItems[index], sizeof(buffer));
    buffer[sizeof(buffer) - 1] = '\0'; // ensure null-termination
    tft.print(buffer);
}

void UI::updateWifiMenuSelection(int previousIndex, int currentIndex) {
    // Redraw the previous menu item without highlighting.
    drawWifiMenuItem(previousIndex, false);
    
    // Redraw the new menu item with highlighting.
    drawWifiMenuItem(currentIndex, true);
}

void UI::drawSongList() {
    int listX = 10;
    int listY = 60;
    int listWidth = tft.width() - 20;
    int itemsPerPage = 5;
    int listHeight = itemsPerPage * 30; // 5 items, 30 px each

    // Clear the list area.
    tft.fillRect(listX, listY, listWidth, listHeight, ILI9341_BLACK);

    // Draw each visible song.
    for (int i = 0; i < itemsPerPage && (i + scrollOffset) < currentProject.songCount; i++) {
        int absoluteIndex = i + scrollOffset;
        drawSongListItem(absoluteIndex, (absoluteIndex == currentSongItem));
    }

    // Optionally, update status text (like "x / total")
    // Optionally, update status text showing (selected songs)/(total songs found)
    char buff[32];
    snprintf(buff, sizeof(buff), "%d / %d", selectedTrackCount, currentProject.songCount);
    drawTextTopCenter(buff, listY - 25, false, ILI9341_WHITE);
}

void UI::drawSongListItem(int absoluteIndex, bool highlighted) {
    // Calculate the relative position within the visible list.
    int relativeIndex = absoluteIndex - scrollOffset;
    int y = 60 + relativeIndex * 30;
    
    // Clear the region for this song item.
    tft.fillRect(10, y, 220, 30, ILI9341_BLACK);
    
    // Set the cursor and text size.
    tft.setCursor(10, y + 5);
    tft.setTextSize(2);
    
    // Determine the text color based on selection state.
    // If the song is toggled selected (via button press), color it green.
    // Otherwise, if it's currently highlighted by the rotary encoder, color it yellow.
    // If neither, use white.
    uint16_t textColor;
    if (selectedSongs[absoluteIndex]) {
        textColor = highlighted ? ILI9341_CYAN : ILI9341_GREEN;
    } else {
        textColor = highlighted ? ILI9341_YELLOW : ILI9341_WHITE;
    }
    
    tft.setTextColor(textColor, ILI9341_BLACK);
    
    // Print the song number and name.
    tft.print(absoluteIndex + 1);
    tft.print(". ");
    tft.print(currentProject.songs[absoluteIndex].songName);
}

void UI::updateSelectedSongCount() {
    // Assume listY is the y-position where the song list starts.
    int listY = 60;
    int statusHeight = 20;  // Height of the status text area.

    // Clear only the region where the status text is drawn.
    tft.fillRect(120, listY -27, 30, statusHeight, ILI9341_BLACK);

    // Update the status text with the number of selected songs.
    char buff[32];
    snprintf(buff, sizeof(buff), "%d / %d", selectedTrackCount, currentProject.songCount);
    drawTextTopCenter(buff, listY - 25, false, ILI9341_WHITE);
}

void UI::drawEditedSongList() {
    int listX = 10;
    int listY = 60;
    int itemsPerPage = 5;  // Always show 5 items if available.
    int listWidth = tft.width() - 20;
    int listHeight = itemsPerPage * 30; // 30 pixels per item.

    // Clear the entire list area.
    tft.fillRect(listX, listY, listWidth, listHeight, ILI9341_BLACK);

    // Redraw each visible song item.
    for (int i = 0; i < itemsPerPage && (i + scrollOffset) < selectedProject.songCount; i++) {
        int absoluteIndex = i + scrollOffset;
        // When reordering is active, use reorderTarget for highlighting;
        // otherwise, use currentMenuItem.
        bool highlighted = isReordering ? (absoluteIndex == reorderTarget)
                                        : (absoluteIndex == currentSongItem);
        drawEditedSongListItem(absoluteIndex, highlighted);
    }
}

void UI::drawEditedSongListItem(int absoluteIndex, bool highlighted) {
    // Calculate the vertical position based on the current scroll offset.
    int relativeIndex = absoluteIndex - scrollOffset;
    int y = 60 + relativeIndex * 30;
    
    // Clear the area for this song item.
    tft.fillRect(10, y, 270, 30, ILI9341_BLACK);
    
    // Set the cursor and text size.
    tft.setCursor(10, y + 5);
    tft.setTextSize(2);
    
    // When reordering mode is active and the item is highlighted, use green.
    // Otherwise, highlighted items are yellow; non-highlighted items are white.
    uint16_t textColor;
    // if (isReordering && highlighted) {
    //     textColor = ILI9341_GREEN;
    // } else {
    //     textColor = highlighted ? ILI9341_YELLOW : ILI9341_WHITE;
    // }

    if (isReordering) {
        // Green highlight for reordering target
        textColor = (absoluteIndex == reorderTarget) ? ILI9341_GREEN 
                   : ILI9341_WHITE;
    } else {
        // Yellow highlight for normal selection
        textColor = (absoluteIndex == currentSongItem) ? ILI9341_YELLOW 
                   : ILI9341_WHITE;
    }
    
    tft.setTextColor(textColor, ILI9341_BLACK);
    
    // Print the song number and name.
    tft.print(absoluteIndex + 1);
    tft.print(". ");
    tft.print(selectedProject.songs[absoluteIndex].songName);
}

void UI::drawPresetList() {
    int itemsPerPage = 5;      // Fixed to show 5 presets
    int listX = 10;
    int listY = 60;
    
    preferences.begin("Setlists", true);
    for (int i = 0; i < itemsPerPage; i++) {
        int absoluteIndex = i + presetScrollOffset;
        if (absoluteIndex >= MAX_PRESETS)
            break;
        int y = listY + i * 30;
        tft.fillRect(listX, y, tft.width() - 20, 30, ILI9341_BLACK); // Clear the region

        tft.setCursor(listX, y + 5);
        tft.setTextSize(2);
        uint16_t color = (absoluteIndex == currentPresetIndex) ? ILI9341_YELLOW : ILI9341_WHITE;
        tft.setTextColor(color, ILI9341_BLACK);

        String presetName = preferences.getString(("p" + String(absoluteIndex + 1) + "_name").c_str(), "No Data");
        tft.print(absoluteIndex + 1);
        tft.print(". ");
        tft.print(presetName);
    }
    preferences.end();
}

void UI::drawPresetListItem(int index, bool highlighted) {
    int itemsPerPage = 5;
    int listY = 60;
    int relativeIndex = index - presetScrollOffset;
    int y = listY + relativeIndex * 30;

    tft.fillRect(10, y, tft.width() - 20, 30, ILI9341_BLACK);
    tft.setCursor(10, y + 5);
    tft.setTextSize(2);
    uint16_t color = highlighted ? ILI9341_YELLOW : ILI9341_WHITE;
    tft.setTextColor(color, ILI9341_BLACK);
    preferences.begin("Setlists", true);
    String presetName = preferences.getString(("p" + String(index + 1) + "_name").c_str(), "No Data");
    preferences.end();
    tft.print(index + 1);
    tft.print(". ");
    tft.print(presetName);
}

void UI::drawSetlistName() {
    // Define the input box dimensions.
    int x = 10;
    int y = 50;
    int w = tft.width() - 20;
    int h = 30;
    int btnWidth = 30;  // Width of the clear "X" button.
    
    // Define the text area to be all of the input box except the clear button area.
    int textAreaWidth = w - btnWidth;  // only clear text area
    
    // Clear only the text area.
    tft.fillRect(x, y, textAreaWidth, h, ILI9341_BLACK);
    
    // Draw the input box border around the entire area.
    tft.drawRect(x, y, w, h, ILI9341_WHITE);
    
    // Update the text in the text area.
    tft.setCursor(x + 5, y + 5);
    tft.setTextSize(2);
    tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
    tft.print(setlistName);
    
    // Now, draw the clear button if it hasn't already been drawn.
    // Instead of redrawing it every time, you might want to draw it once
    // and then only redraw if the button area is not being cleared.
    int btnX = x + textAreaWidth;  // Position at the right edge of the text area.
    int btnY = y;  // align vertically with input box.
    
    // Option 1: Always draw the clear button (if the fillRect above doesn't affect it).
    // Option 2: Redraw the clear button only if necessary.
    // In this example, we assume the text area is cleared only up to btnX,
    // so the button area remains intact.
    tft.fillRect(btnX, btnY, btnWidth, h, ILI9341_RED);
    tft.drawRect(btnX, btnY, btnWidth, h, ILI9341_WHITE);
    tft.setCursor(btnX + 8, btnY + 5);  // adjust these offsets as needed.
    tft.setTextSize(2);
    tft.setTextColor(ILI9341_WHITE, ILI9341_RED);
    tft.print("X");
}

void UI::drawLoadedPreset() {
    int textY = 100;
    tft.fillRect(10, textY, tft.width()-20, 45, ILI9341_BLACK);
    tft.drawRect(10, textY, tft.width()-20, 45, ILI9341_WHITE);
    tft.fillRect(150, 13, 60, 20, ILI9341_BLACK);
    
    if (strcmp(loadedPreset.name, "No Preset") == 0 ||
        strlen(loadedPreset.data.projectName) == 0 ||
        totalTracks == 0) {
        drawText("No tracks available.", 10, textY+10, ILI9341_RED, 2);
    } else {
        // Draw the preset name
        constexpr int MAX_PRESET_NAME_CHARS = 16; // Adjust based on your display needs
        char presetDisplayName[MAX_PRESET_NAME_CHARS + 3] = {0}; // Buffer for preset name
        const char* presetName = loadedPreset.name;
        if (presetName != nullptr) {
            strncpy(presetDisplayName, presetName, MAX_PRESET_NAME_CHARS);
            presetDisplayName[MAX_PRESET_NAME_CHARS] = '\0'; // Force null-termination
            
            // Add ellipsis if needed
            if (strlen(presetName) > MAX_PRESET_NAME_CHARS) {
                if (MAX_PRESET_NAME_CHARS >= 2) {
                    presetDisplayName[MAX_PRESET_NAME_CHARS - 1] = '.';
                    presetDisplayName[MAX_PRESET_NAME_CHARS] = '.';
                    presetDisplayName[MAX_PRESET_NAME_CHARS + 1] = '\0';
                }
            }
        } else {
            strncpy(presetDisplayName, "Invalid Preset", sizeof(presetDisplayName));
        }
        drawText(presetDisplayName, 95, 80, ILI9341_YELLOW, 2);

        constexpr int MAX_PROJECT_NAME_CHARS = 15; // Adjust based on your display needs
        char projectDisplayName[MAX_PROJECT_NAME_CHARS + 3] = {0}; // Buffer for project name
        const char* projectName = loadedPreset.data.projectName;
        if (projectName != nullptr) {
            strncpy(projectDisplayName, projectName, MAX_PROJECT_NAME_CHARS);
            projectDisplayName[MAX_PROJECT_NAME_CHARS] = '\0'; // Force null-termination
            
            // Add ellipsis if needed
            if (strlen(projectName) > MAX_PROJECT_NAME_CHARS) {
                if (MAX_PROJECT_NAME_CHARS >= 2) {
                    projectDisplayName[MAX_PROJECT_NAME_CHARS - 1] = '.';
                    projectDisplayName[MAX_PROJECT_NAME_CHARS] = '.';
                    projectDisplayName[MAX_PROJECT_NAME_CHARS + 1] = '\0';
                }
            }
        } else {
            strncpy(projectDisplayName, "Invalid Project", sizeof(projectDisplayName));
        }
        drawText(projectDisplayName, 105, 150, ILI9341_ORANGE, 2);
        
        // Handle song color
        uint16_t songColor = presetChanged ? ILI9341_WHITE : 
                           (isPlaying ? ILI9341_GREEN : ILI9341_WHITE);

        // SAFE VERSION: Proper array indexing
        constexpr int MAX_DISPLAY_CHARS = 14;
        char displayName[MAX_DISPLAY_CHARS + 3] = {0};  // Initialize buffer
        
        if (currentTrack < totalTracks) {  // Ensure valid track index
            const char* songName = loadedPreset.data.songs[currentTrack].songName;
            
            if (songName != nullptr) {
                // Truncate with safe operations
                strncpy(displayName, songName, MAX_DISPLAY_CHARS);
                displayName[MAX_DISPLAY_CHARS] = '\0';  // Force null-termination
                
                // Add ellipsis only if needed
                if (strlen(songName) > MAX_DISPLAY_CHARS) {
                    if (MAX_DISPLAY_CHARS >= 2) {  // Ensure room for ellipsis
                        displayName[MAX_DISPLAY_CHARS-1] = '.';
                        displayName[MAX_DISPLAY_CHARS] = '.';
                        displayName[MAX_DISPLAY_CHARS+1] = '\0';
                    }
                }
            } else {
                strncpy(displayName, "Invalid Song", sizeof(displayName));
            }
        } else {
            strncpy(displayName, "Invalid Track", sizeof(displayName));
        }

        // Draw the song name
        drawText(displayName, 15, textY+10, songColor, 3);
        
        // Draw track counter
        if (totalTracks > 0) {
            drawText((String(currentTrack + 1) + "/" + String(totalTracks)).c_str(),
                     150, 16, ILI9341_WHITE, 2);
        }
    }
    
    webServerManager.notifyPresetUpdate();
    presetChanged = false;
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
      int barBaseX = tft.width() - 60, barBaseY = y + 2;
      for (int b = 0; b < 5; b++) {
        int bx = barBaseX + b * (barWidth + barSpacing) + 5;
        if (b < bars)
          tft.fillRect(bx, barBaseY, barWidth, barHeight, ILI9341_GREEN);
        else
          tft.drawRect(bx, barBaseY, barWidth, barHeight, ILI9341_WHITE);
      }
    }
}

void UI::drawWiFiListItem(int absoluteIndex, bool highlighted) {
    // Calculate the relative position in the visible list.
    int relativeIndex = absoluteIndex - wifiScrollOffset;
    int y = 60 + relativeIndex * 30;
    
    // Clear the region for this Wi-Fi item.
    tft.fillRect(10, y, tft.width() - 70, 30, ILI9341_BLACK);
    
    // Set text color based on selection state.
    uint16_t textColor = highlighted ? ILI9341_YELLOW : ILI9341_WHITE;
    tft.setTextColor(textColor, ILI9341_BLACK);
    tft.setTextSize(2);
    tft.setCursor(10, y);
    tft.print(String(absoluteIndex + 1) + ". " + wifiSSIDs[absoluteIndex]);
    
    // Draw signal strength bars.
    int rssiVal = wifiRSSI[absoluteIndex];
    int bars = map(rssiVal, -90, -30, 0, 5);
      bars = constrain(bars, 0, 5);
      int barWidth = 5, barHeight = 10, barSpacing = 3;
      int barBaseX = tft.width() - 60, barBaseY = y + 2;
      for (int b = 0; b < 5; b++) {
        int bx = barBaseX + b * (barWidth + barSpacing) + 5;
        if (b < bars)
          tft.fillRect(bx, barBaseY, barWidth, barHeight, ILI9341_GREEN);
        else
          tft.drawRect(bx, barBaseY, barWidth, barHeight, ILI9341_WHITE);
      }
}

void UI::checkWifiConnection() {
    if (currentState == ScreenState::MENU2_WIFICONNECTING) {
        unsigned long startAttemptTime = millis();
        // Check if the Wi-Fi connection is established.unsigned long startAttemptTime = millis();
        const unsigned long wifiTimeout = 10000; // 5 seconds

        while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < wifiTimeout) {
            delay(500); // avoid watchdog reset
            }
            
            // Check if connected or timeout occurred
            if (WiFi.status() == WL_CONNECTED) {
                Serial.println("Connected to WiFi!");
                // Proceed with connected logic (e..beg., ui->setScreenState(...))
            } else {
                Serial.println("Failed to connect to WiFi within 5 seconds.");
                WiFi.disconnect(); // optional
                setScreenState(ScreenState::HOME);
            }

        if (WiFi.status() == WL_CONNECTED && !wifiFullyConnected) {
            wifiFullyConnected = true;
            digitalWrite(LED_WIFI, HIGH);
            tft.fillScreen(ILI9341_BLACK);
            drawTextTopCenter("Connected!", 7, true, ILI9341_GREEN);
            drawText("Successfully connected to: ", 10, 60, ILI9341_WHITE, 2);
            drawText(selectedSSID.c_str(), 10, 80, ILI9341_YELLOW, 2);

            static bool webServerSetupDone = false;
            if (!webServerSetupDone) {
                webServerManager.setup();
                webServerManager.onPresetUpdate = [this]() {
                    if (selectedPresetSlot != -1) {
                        drawLoadedPreset(); // Redraw only if a preset is selected
                    }
                };
                webServerSetupDone = true;
            }
            // webServerManager.update(); // This call is optional here.

            delay(2000);
            setScreenState(ScreenState::HOME);
        }
    }
}

void UI::WifiScan() {
    // Start a new Wi-Fi scan if one hasn't been started yet
    if (!wifiScanInProgress && !wifiScanDone) {
        Serial.println(F("Starting Async Wi-Fi Scan..."));
        WiFi.mode(WIFI_STA);      // Set Wi-Fi mode to Station (STA)
        WiFi.setSleep(false);
        WiFi.disconnect(true);    // Disconnect from any previous Wi-Fi connection
        delay(100);               // Wait briefly before scanning
        WiFi.scanNetworks(true);  // Start scanning asynchronously
        wifiScanInProgress = true;
    }

    // Poll for scan results while in progress
    if (wifiScanInProgress) {
        int n = WiFi.scanComplete();  // Check if scan is completed
        Serial.println(n);  // Debug print to check scan result
        tft.fillRect(10, 60, tft.width() - 20, 160, ILI9341_BLACK);  // Clear previous content
        drawText("Scanning...", 10, 60, ILI9341_YELLOW, 2);  // Show scanning message

        // Keep scanning until we get a positive result (scan complete)
        while (n <= 0) {
            delay(100);  // Wait a bit before checking again
            n = WiFi.scanComplete();  // Check scan result again
        }
        Serial.println(n);  // Debug print to check scan result

        // If scan result is positive, proceed
        if (n > 0) {
            wifiCount = (n < MAX_WIFI_NETWORKS) ? n : MAX_WIFI_NETWORKS;
            for (int i = 0; i < wifiCount; i++) {
                wifiSSIDs[i] = WiFi.SSID(i);   // Store SSID
                wifiRSSI[i] = WiFi.RSSI(i);    // Store RSSI (signal strength)
            }
            wifiScanDone = true;            // Mark scan as complete
            wifiScanInProgress = false;     // Reset scan flag
            Serial.println(F("Wi-Fi scan completed"));
        }
    }

    if (wifiScanDone) {
        tft.fillRect(10, 60, tft.width() - 20, 160, ILI9341_BLACK);  // Clear previous content
        drawWiFiList();  // Draw the Wi-Fi list
    }
    wifiScanDone = false;
}

void UI::drawWiFiPassword() {
    // Clear only the text area inside the input box.
    // (Assuming the input box was drawn at x=10, y=50, with width = tft.width()-20 and height = 30.)
    tft.fillRect(11, 51, tft.width()-22, 28, ILI9341_BLACK);
    tft.setCursor(15, 57);
    tft.setTextSize(2);
    tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
    tft.print(wifiPassword);
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

    // Draw volume display.
    drawText("Vol ", 10, 15, ILI9341_GREEN, 2);
    tft.drawRect(50, 10, 80, 25, ILI9341_WHITE);

    drawText("Preset:", 10, 80, ILI9341_YELLOW, 2);
    tft.drawRect(10, 100, tft.width()-20, 45, ILI9341_WHITE);
    drawText("Project:", 10, 150, ILI9341_ORANGE, 2);

    displayVolume();

    String ip = WiFi.localIP().toString();
    if (ip == "0.0.0.0")
        drawText("No WiFi Connection", 10, 225, ILI9341_RED, 2);
    else
        drawText(("WebServer:" + ip).c_str(), 10, 225, ILI9341_GREENYELLOW, 2);     
    // Adjust coordinates as needed.

    // Draw the home menu box.
    drawHomeMenuBox();

    // If a preset is selected, display the loaded preset info.
    if (selectedPresetSlot != -1) {
        if (presetChanged) {
            totalTracks = loadedPreset.data.songCount;
            presetChanged = false;
        }
        drawLoadedPreset();  // Draw only the preset info region.
    } else {
        // drawText("No Preset Selected", 10, 80, ILI9341_RED, 2);
    }
    webServerManager.notifyPresetUpdate();
}

void UI::menuScreen() {
    tft.fillScreen(ILI9341_BLACK);
    drawTextTopCenter("SETLIST SETTINGS", 7, true, ILI9341_WHITE);
    for (int i = 0; i < NUM_MENU_ITEMS; i++) {
        drawMenuItem(i, i == currentMenuItem);
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
    drawSongList();
}

void UI::editSetlistScreen() {
    currentSongItem = 0;      // Start at first item
    scrollOffset = 0;         // Reset scroll position
    isReordering = false;     // Ensure reordering mode is off

    tft.fillScreen(ILI9341_BLACK);
    drawRectButton(BACK_BUTTON_X, BACK_BUTTON_Y, BACK_BUTTON_WIDTH, BACK_BUTTON_HEIGHT, "Back");
    drawRectButton(SAVE_BUTTON_X, SAVE_BUTTON_Y, SAVE_BUTTON_WIDTH, SAVE_BUTTON_HEIGHT, "Next");
    drawTextTopCenter("Edit Setlist", 7, true, ILI9341_WHITE);

    // If we're creating a NEW setlist (currentMenuItem == 0), require at least one song selected.
    // If we're editing an existing preset (currentMenuItem == 2), use the loaded preset data.
    if (selectedTrackCount == 0 && menu1Index == 0) {
        tft.setCursor(10, 60);
        tft.setTextColor(ILI9341_WHITE);
        tft.setTextSize(2);
        tft.print("No songs selected.");
        return;
    }
            
    // Initialize reordering if needed.
    if (!isReorderedSongsInitialized) { 
        memset(&selectedProject, 0, sizeof(selectedProject));
        int idx = 0;
        if (menu1Index == 0) {
            // New Setlist: copy songs from currentProject where selectedSongs is true.
            strcpy(selectedProject.projectName, currentProject.projectName);
            for (int i = 0; i < currentProject.songCount; i++) {
                if (selectedSongs[i]) {
                    selectedProject.songs[idx] = currentProject.songs[i];
                    selectedProject.songs[idx].songIndex  = i;
                    selectedProject.songs[idx].changedIndex = idx;
                    idx++;
                }
            }
            selectedProject.songCount = idx;
        } else if (menu1Index == 2) {
            // Edit Setlist: load songs from the loaded preset.
            strcpy(selectedProject.projectName, loadedPreset.data.projectName);
            for (int i = 0; i < loadedPreset.data.songCount; i++) {
                // For editing, copy all songs from loadedPreset.
                selectedProject.songs[idx] = loadedPreset.data.songs[i];
                selectedProject.songs[idx].songIndex  = loadedPreset.data.songs[i].songIndex;
                selectedProject.songs[idx].changedIndex = idx;
                idx++;
            }
            selectedProject.songCount = loadedPreset.data.songCount;
            // Update selectedTrackCount to reflect the loaded preset's song count.
            selectedTrackCount = loadedPreset.data.songCount;
        }
        
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
    
    // Now, display the song list (show up to 5 items).
    int itemsPerPage = min(5, selectedProject.songCount);
    for (int i = 0; i < itemsPerPage; i++) {
        int absoluteIndex = scrollOffset + i;
        if (absoluteIndex >= selectedProject.songCount) break;
        // Draw each item, highlighting the currentMenuItem (if not in reordering mode).
        bool highlighted = (!isReordering && (absoluteIndex == currentMenuItem));
        drawEditedSongListItem(absoluteIndex, highlighted);
    }
    scrollOffset = constrain(scrollOffset, 0, max(0, selectedProject.songCount - itemsPerPage));
}

void UI::selectSetlistScreen() {
    tft.fillScreen(ILI9341_BLACK);
    drawTextTopCenter("Select Preset", 7, true, ILI9341_WHITE);
    drawRectButton(BACK_BUTTON_X, BACK_BUTTON_Y, BACK_BUTTON_WIDTH, BACK_BUTTON_HEIGHT, "Back");
    drawRectButton(SAVE_BUTTON_X, SAVE_BUTTON_Y, SAVE_BUTTON_WIDTH, SAVE_BUTTON_HEIGHT, "Save");

    // Draw the preset list using our helper.
    drawPresetList();
}

void UI::saveSetlistScreen() {
    tft.fillScreen(ILI9341_BLACK);
    drawRectButton(BACK_BUTTON_X, BACK_BUTTON_Y, BACK_BUTTON_WIDTH, BACK_BUTTON_HEIGHT, "Back");
    drawRectButton(SAVE_BUTTON_X, SAVE_BUTTON_Y, SAVE_BUTTON_WIDTH, SAVE_BUTTON_HEIGHT, "Save");
    drawTextTopCenter("Save Setlist", 7, true, ILI9341_WHITE);

    if (selectedPresetSlot == -1) {
        drawText("No Slot Selected", 10, 50, ILI9341_RED, 2);
        return;
    }

    // Load the preset name from Preferences
    preferences.begin("Setlists", true);
    String presetName = preferences.getString(("p" + String(selectedPresetSlot) + "_name").c_str(), "No Preset");
    preferences.end();
    // If the preset already has a name, load it into setlistName
    if (presetName != "No Preset") {
        presetName.toCharArray(setlistName, MAX_SONG_NAME_LEN + 1);
    }
    // Otherwise, setlistName remains as is (or can be cleared if desired)

    // Draw the input box (only the input text area and clear button)
    drawSetlistName();

    // Draw the on-screen keyboard (which updates only that area)
    drawKeyboard(isShifted);
}

void UI::menu2Screen() {
    tft.fillScreen(ILI9341_BLACK);
    drawTextTopCenter("System Settings", 7, true, ILI9341_WHITE);
    for (int i = 0; i < NUM_MENU2_ITEMS; i++) {
        drawMenu2Item(i, i == currentMenu2Item);
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
    tft.fillScreen(ILI9341_BLACK);
    drawRectButton(BACK_BUTTON_X, BACK_BUTTON_Y, BACK_BUTTON_WIDTH, BACK_BUTTON_HEIGHT, "Back");
    drawRectButton(NEXT_BUTTON_X, NEXT_BUTTON_Y, NEXT_BUTTON_WIDTH, NEXT_BUTTON_HEIGHT, "Re");
    drawTextTopCenter("Wi-Fi Connect", 7, true, ILI9341_WHITE);

    WifiScan();
}

void UI::menu2WifiPasswordScreen() {
    tft.fillScreen(ILI9341_BLACK);
    drawTextTopCenter("Enter Password", 7, true, ILI9341_WHITE);
    drawTextTopCenter(selectedSSID.c_str(), 27, false, ILI9341_YELLOW);
    drawRectButton(BACK_BUTTON_X, BACK_BUTTON_Y, BACK_BUTTON_WIDTH, BACK_BUTTON_HEIGHT, "Back");
    drawRectButton(SAVE_BUTTON_X, SAVE_BUTTON_Y, SAVE_BUTTON_WIDTH, SAVE_BUTTON_HEIGHT, "OK");

    // Draw the input box border once.
    tft.drawRect(10, 50, tft.width() - 20, 30, ILI9341_WHITE);
    // Draw the initial password (if any) in the text area.
    drawWiFiPassword();

    // Draw the on-screen keyboard.
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