#include <WiFi.h>  // Correct WiFi library for ESP32
#include <WebSocketsClient.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>

// Replace with your network credentials
const char* ssid = "GlobeAtHome_f6ba8";
const char* password = "StS3fcy2";

// WebSocket server settings
const char* websocket_host = "192.168.254.102";
uint16_t websocket_port = 8080;
const char* websocket_url = "/";

// DHT22 Sensor Setup
#define DHTPIN 4
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// WebSocket client object
WebSocketsClient webSocket;

// Change deviceId to an integer
int deviceId = 1;  // Example: 1 for Device 1, change to 2 for Device 2
unsigned long previousMillis = 0;
const long interval = 30000;  // Send data every 5 seconds

// Custom function to handle errors and provide suggestions
void handleError(const String& errorMsg, const String& suggestion) {
  Serial.println("Error: " + errorMsg);
  Serial.println("Suggested fix: " + suggestion);
}

void webSocketEvent(WStype_t type, uint8_t *payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:
      Serial.println("Disconnected from WebSocket server.");
      handleError("WebSocket Disconnected", "Check server connection or WebSocket server URL.");
      break;
    case WStype_CONNECTED:
      Serial.println("Connected to WebSocket server.");
      break;
    case WStype_TEXT:
      Serial.printf("Message from server: %s\n", payload);
      break;
    case WStype_ERROR:
      Serial.println("WebSocket error occurred.");
      handleError("WebSocket Error", "Check network connectivity or server health.");
      break;
  }
}

void setup() {
  Serial.begin(115200);
  dht.begin();

  // Connect to Wi-Fi
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);

  int retries = 0;
  const int maxRetries = 10;

  while (WiFi.status() != WL_CONNECTED && retries < maxRetries) {
    delay(1000);
    Serial.print(".");
    retries++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected to WiFi");
  } else {
    handleError("Failed to connect to WiFi", "Check WiFi credentials or router availability.");
    return;  // Exit if WiFi connection fails
  }

  // Setup WebSocket
  Serial.println("Connecting to WebSocket server...");
  webSocket.begin(websocket_host, websocket_port, websocket_url);
  webSocket.onEvent(webSocketEvent);
}

void loop() {
  webSocket.loop();

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    float t = dht.readTemperature();
    float h = dht.readHumidity();
    float hi = dht.computeHeatIndex(t, h, false);

    if (isnan(t) || isnan(h) || isnan(hi)) {
      handleError("Failed to read from DHT sensor", "Ensure the sensor is connected properly and try resetting the device.");
      return;
    }

    // Check if WebSocket is connected before sending data
    if (webSocket.isConnected()) {
      // Convert int deviceId to a string when sending the JSON data
      String jsonData = "{\"deviceId\":" + String(deviceId) + ",\"temperature\":" + String(t) +
                        ",\"humidity\":" + String(h) + ",\"heat_index\":" + String(hi) + "}";

      webSocket.sendTXT(jsonData);
      Serial.println("Sent data: " + jsonData);
    } else {
      handleError("WebSocket not connected", "Check if WebSocket server is online and reachable.");
    }
  }
}
