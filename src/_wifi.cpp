#include "_wifi.h"

void WiFiHandler::initWiFi() {
  // Set WiFi to station mode and disconnect from any previous network.
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true);
  delay(100);
  
  // Initialize LittleFS for serving files if needed.
  if (!LittleFS.begin()) {
    Serial.println("LittleFS Mount Failed");
  } else {
    Serial.println("LittleFS Mounted Successfully");
  }
  
  Serial.println("WiFi initialized.");
}

void WiFiHandler::startScan() {
  if (!scanInProgress) {
    Serial.println("Starting WiFi scan...");
    WiFi.scanNetworks(true); // asynchronous scan
    scanInProgress = true;
  }
}

void WiFiHandler::updateScan() {
  int networks = WiFi.scanComplete();
  if (networks >= 0) {
    scanInProgress = false;
    Serial.print("Found ");
    Serial.print(networks);
    Serial.println(" networks:");
    for (int i = 0; i < networks; i++) {
      Serial.print(i);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.println(" dBm)");
    }
    WiFi.scanDelete(); // free memory used by scan results
  }
}

void WiFiHandler::connectToWiFi(const char* ssid, const char* password) {
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  
  unsigned long startAttemptTime = millis();
  // Wait for connection for up to 10 seconds.
  while (WiFi.status() != WL_CONNECTED && (millis() - startAttemptTime) < 10000) {
    delay(100);
    Serial.print(".");
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nFailed to connect.");
  }
}

void WiFiHandler::disconnectWiFi() {
  WiFi.disconnect();
  Serial.println("WiFi disconnected.");
}

String WiFiHandler::getSSID() {
  return WiFi.SSID();
}

IPAddress WiFiHandler::getLocalIP() {
  return WiFi.localIP();
}

int WiFiHandler::getRSSI() {
  return WiFi.RSSI();
}

bool WiFiHandler::isConnected() {
  return (WiFi.status() == WL_CONNECTED);
}
