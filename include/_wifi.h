#ifndef _WIFI_H
#define _WIFI_H

#include <WiFi.h>
#include "config.h"

class WiFiHandler {
public:
  // Initialize WiFi and file system (LittleFS) if needed.
  void initWiFi();

  // Start an asynchronous WiFi scan.
  void startScan();

  // Check if the scan is complete and print results.
  void updateScan();

  // Connect to a WiFi network given SSID and password.
  void connectToWiFi(const char* ssid, const char* password);

  // Disconnect from WiFi.
  void disconnectWiFi();

  // Utility functions.
  String getSSID();
  IPAddress getLocalIP();
  int getRSSI();
  bool isConnected();

private:
  bool scanInProgress;
};

#endif // WIFI_HANDLER_H
