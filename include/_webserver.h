#ifndef _WEBSERVER_H
#define _WEBSERVER_H

#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <LittleFS.h>

class AsyncWebServerManager {
private:
    AsyncWebServer server;    // HTTP server instance.
    AsyncWebSocket ws;        // WebSocket instance on the given endpoint.
    bool serverStarted;
    uint16_t port;            // Store the port number.

    // Mount LittleFS and return true if successful.
    bool initFileSystem() {
        if (!LittleFS.begin()) {
            Serial.println("LittleFS mount failed");
            return false;
        }
        Serial.println("LittleFS mounted successfully");
        return true;
    }

    // Set up the WebSocket endpoint and its event handler.
    void setupWebSocket() {
        ws.onEvent([this](AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
                            void *arg, uint8_t *data, size_t len) {
            if (type == WS_EVT_CONNECT) {
                Serial.printf("WebSocket client connected, id: %u\n", client->id());
                client->text("Welcome to the Async WebSocket Server");
            } else if (type == WS_EVT_DISCONNECT) {
                Serial.printf("WebSocket client disconnected, id: %u\n", client->id());
            } else if (type == WS_EVT_DATA) {
                String msg = "";
                for (size_t i = 0; i < len; i++) {
                    msg += (char)data[i];
                }
                Serial.printf("WebSocket received: %s\n", msg.c_str());
                // If the message starts with "GET_FILE:", read and return file content.
                if (msg.startsWith("GET_FILE:")) {
                    String filename = msg.substring(9); // Extract filename after "GET_FILE:"
                    Serial.printf("Client requested file: %s\n", filename.c_str());
                    if (!LittleFS.exists(filename)) {
                        client->text("Error: File not found");
                    } else {
                        File file = LittleFS.open(filename, "r");
                        if (!file) {
                            client->text("Error: Unable to open file");
                        } else {
                            String fileContent = "";
                            while (file.available()) {
                                fileContent += char(file.read());
                            }
                            file.close();
                            client->text(fileContent);
                        }
                    }
                } else {
                    // Echo back the received message.
                    client->text(msg);
                }
            }
        });
        // Add the WebSocket handler to the HTTP server.
        server.addHandler(&ws);
    }

    // Set up HTTP routes to serve static files from LittleFS.
    void setupHTTPRoutes() {
        server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");
    }

public:
    // Constructor: initialize the server and WebSocket with the given port.
    AsyncWebServerManager(uint16_t port)
        : server(port), ws("/ws"), serverStarted(false), port(port) {}

    // Set up LittleFS, WebSocket, and HTTP routes, then start the server.
    void setup() {
        if (!initFileSystem()) {
            Serial.println("Failed to mount LittleFS. Aborting web server setup.");
            return;
        }
        setupWebSocket();
        setupHTTPRoutes();
        server.begin();
        serverStarted = true;
        Serial.printf("Async Web Server started on port %d\n", port);
    }

    // For an asynchronous server, no periodic update is required.
    void update() {
        // Not needed for AsyncWebServer.
    }

    // Check if the server is running.
    bool isRunning() const {
        return serverStarted;
    }
};

#endif // ASYNCWEBSERVERMANAGER_H
