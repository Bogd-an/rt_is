#if !defined(CAMERA_MODEL_AI_THINKER)
  #error "❌ не та платформа"
#endif

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>

// --- Налаштування пінів та мережі ---
const int RXD1      = 15; // Пін для зв'язку з іншим пристроєм (наприклад, Arduino)
const int TXD1      = 14;
const int ledPin    = 4;  // Вбудований світлодіод для індикації

const char *ssid      = "BogWiFi";
const char *password  = "123456789";

const char *hostname = "esp32cam1"; //  mDNS (Bonjour) http://esp32cam1.local

// --- Глобальні змінні ---
char buffer[64];
int bufferIndex = 0;
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

void sendLog(const String& msg) {
    Serial.println(msg);
    ws.textAll(msg);
}

void sendToArduino(const char *msg) {
    Serial1.println(msg);
    sendLog(String("Sent to Arduino -> ") + msg);
}

void onWebSocketMessage(AsyncWebSocket *server, AsyncWebSocketClient *client, 
                        AwsFrameInfo *info, const uint8_t *data, size_t len) {
    // Перевіряємо, що це текстове повідомлення
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
        // Створюємо рядок з отриманих даних
        String msg = "";
        for (size_t i = 0; i < len; i++) {
            msg += (char)data[i];
        }
        msg.trim(); // Очищуємо від зайвих пробілів

        sendLog(String("Received from WebSocket -> ") + msg);

        if (msg == "on" || msg == "off") {
            sendToArduino(msg.c_str());
        }
    }
}

void init_mdns() {
    if (!MDNS.begin(hostname)) {
        Serial.println("Error setting up MDNS responder!");
        return;
    }
    Serial.printf("mDNS responder started. Hostname: http://%s.local\n", hostname);
    MDNS.addService("http", "tcp", 80);
}

void setup() {
    pinMode(ledPin, OUTPUT);
    digitalWrite(ledPin, LOW);

    Serial.begin(115200);
    Serial1.begin(9600, SERIAL_8N1, RXD1, TXD1);

    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
        digitalWrite(ledPin, !digitalRead(ledPin));
    }
    digitalWrite(ledPin, HIGH);

    Serial.printf("\nConnected to WiFi! ESP32 IP: http://%s\n", WiFi.localIP().toString().c_str());

    init_mdns();

    ws.onEvent([](AsyncWebSocket *server, AsyncWebSocketClient *client, 
                  AwsEventType type, void *arg, uint8_t *data, size_t len){
        if (type == WS_EVT_CONNECT) {
            Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
        } else if (type == WS_EVT_DISCONNECT) {
            Serial.printf("WebSocket client #%u disconnected\n", client->id());
        } else if (type == WS_EVT_DATA) {
            onWebSocketMessage(server, client, (AwsFrameInfo*)arg, data, len);
        }
    });

    server.addHandler(&ws);

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        String response = "ESP32-CAM is online!\n";
        response += "IP: " + WiFi.localIP().toString() + "\n";
        response += "mDNS Hostname: " + String(hostname) + ".local";
        request->send(200, "text/plain", response);
    });

    server.begin();
    sendLog("Server started. Ready to receive commands.");
}

void loop() {
    while (Serial1.available()) {
        char inChar = (char)Serial1.read();
        if (inChar == '\n') {
            buffer[bufferIndex] = '\0';
            if (bufferIndex > 0) {
                sendLog(String("Received from Arduino -> ") + buffer);
            }
            bufferIndex = 0;
        } else if (inChar >= 32) {
            if (bufferIndex < sizeof(buffer) - 1) {
                buffer[bufferIndex++] = inChar;
            }
        }
    }
}