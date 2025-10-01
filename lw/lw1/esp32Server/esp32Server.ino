#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>

#define RXD1 15
#define TXD1 14

char buffer[20];
int bufferIndex = 0;
bool ledState = false;
const char *ssid = "BogWiFi";
const char *password = "123456789";

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// mDNS hostname
const char *hostname = "ESP32cam1";

void sendLog(String msg)
{
    Serial.println(msg);
    ws.textAll(msg);
}

void sendToArduino(const char *msg)
{
    Serial1.println(msg);
    sendLog(String("Sent to Arduino -> ") + msg);
}

void onWebSocketMessage(AsyncWebSocket *server, AsyncWebSocketClient *client,
                        AwsFrameInfo *info, String data)
{
    data.trim();
    if (data == "on" || data == "off")
    {
        sendToArduino(data.c_str());
    }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
             AwsEventType type, void *arg, uint8_t *data, size_t len)
{
    if (type == WS_EVT_DATA)
    {
        AwsFrameInfo *info = (AwsFrameInfo *)arg;
        if (info->final && info->index == 0 &&
            info->len == len && info->opcode == WS_TEXT)
        {
            String msg = "";
            for (size_t i = 0; i < len; i++)
                msg += (char)data[i];
            onWebSocketMessage(server, client, info, msg);
        }
    }
}

// ---- Нова ініціалізація mDNS ----
bool init_mdns()
{
    if (!MDNS.begin(hostname))
    {
        Serial.println("Error setting up MDNS responder!");
        return false;
    }

    Serial.printf("mDNS started! Hostname: %s.local\n", hostname);

    // Додаємо сервіси
    MDNS.addService("http", "tcp", 80);
    MDNS.addService("arduino", "tcp", 3232);

    return true;
}

void setup()
{
    Serial.begin(115200);
    Serial1.begin(9600, SERIAL_8N1, RXD1, TXD1);

    WiFi.begin(ssid, password); // або WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi");

    const time_t endTime = millis() + 15 * 1000; // 15 секунд таймаут
    while (WiFi.status() != WL_CONNECTED && millis() < endTime)
    {
        delay(500);
        Serial.print(".");
    }

    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("\nFailed to connect to WiFi!");
        return;
    }

    Serial.println("\nConnected to WiFi");
    Serial.print("ESP32 IP: ");
    Serial.println(WiFi.localIP());

    // Ініціалізація mDNS
    if (init_mdns())
    {
        Serial.println("mDNS service initialized successfully");
        // Тут можна робити MDNS.queryService() або MDNS.queryHost()
        // наприклад:
        // IPAddress ip = MDNS.queryHost("vissen");
        // if (ip) Serial.printf("Found vissen.local at %s\n", ip.toString().c_str());
    }

    ws.onEvent(onEvent);
    server.addHandler(&ws);
    server.begin();

    Serial.println("Web server started!");
    // ==== HTTP відповідь з IP та mDNS ====
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        String response = "ESP32 IP: " + WiFi.localIP().toString() +
                          "\nMDNS: " + String(hostname) + ".local";
        request->send(200, "text/plain", response);
    });
}

void loop()
{
    while (Serial1.available())
    {
        char inChar = Serial1.read();
        if (inChar == '\n')
        {
            buffer[bufferIndex] = '\0';
            sendLog(String("Received from Arduino -> ") + buffer);
            bufferIndex = 0;
            break;
        }
        else if (bufferIndex < sizeof(buffer) - 1)
        {
            buffer[bufferIndex++] = inChar;
        }
    }
}
