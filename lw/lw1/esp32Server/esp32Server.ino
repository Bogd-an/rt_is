#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>

#define RXD1 15
#define TXD1 14

char buffer[20];
int bufferIndex = 0;
bool ledState = false;
const char *ssid = "Mystery";
const char *password = "BZ343MJIPH78T01SL";

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// mDNS змінні
const char *hostname = "ESP32cam1";
mdns_server_t *mdns = NULL;

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

// Функція для пошуку mDNS хостів
void resolve_mdns_host(const char *hostname)
{
    Serial.printf("mDNS Host Lookup: %s.local\n", hostname);
    if (mdns_query(mdns, hostname, NULL, 1000)) {
        const mdns_result_t *results = mdns_result_get(mdns, 0);
        size_t i = 1;
        while(results) {
            Serial.printf("  %u: IP:" IPSTR ", IPv6:" IPV6STR "\n", i++, 
                         IP2STR(&results->addr), IPV62STR(results->addrv6));
            results = results->next;
        }
        mdns_result_free(mdns);
    } else {
        Serial.println("  Host Not Found");
    }
}

// Функція для ініціалізації mDNS
bool init_mdns() {
    const char *arduTxtData[4] = {
        "board=esp32",
        "tcp_check=no",
        "ssh_upload=no",
        "auth_upload=no"
    };

    esp_err_t err = mdns_init(TCPIP_ADAPTER_IF_STA, &mdns);
    if (err) {
        Serial.printf("Failed starting MDNS: %u\n", err);
        return false;
    }

    // Встановлюємо ім'я хоста та екземпляр
    if (mdns_set_hostname(mdns, hostname) != ESP_OK) {
        Serial.println("Failed to set MDNS hostname");
        return false;
    }
    
    if (mdns_set_instance(mdns, hostname) != ESP_OK) {
        Serial.println("Failed to set MDNS instance");
        return false;
    }
    
    // Додаємо сервіс для Arduino
    if (mdns_service_add(mdns, "_arduino", "_tcp", 3232) != ESP_OK) {
        Serial.println("Failed to add Arduino service");
    } else {
        mdns_service_txt_set(mdns, "_arduino", "_tcp", 4, arduTxtData);
    }
    
    // Додаємо сервіс для HTTP
    if (mdns_service_add(mdns, "_http", "_tcp", 80) != ESP_OK) {
        Serial.println("Failed to add HTTP service");
    } else {
        mdns_service_txt_set(mdns, "_http", "_tcp", 4, arduTxtData);
        mdns_service_instance_set(mdns, "_http", "_tcp", "Aquacontrol32 WebServer");
    }

    Serial.printf("mDNS started! Hostname: %s.local\n", hostname);
    return true;
}

void setup()
{
    Serial.begin(115200);
    Serial1.begin(9600, SERIAL_8N1, RXD1, TXD1);

    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi");
    
    const time_t endTime = millis() + 15 * 1000; // 15 секунд таймаут
    while (WiFi.status() != WL_CONNECTED && millis() < endTime)
    {
        delay(500);
        Serial.print(".");
    }

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("\nFailed to connect to WiFi!");
        return;
    }

    Serial.println("\nConnected to WiFi");
    Serial.print("ESP32 IP: ");
    Serial.println(WiFi.localIP());

    // Ініціалізація mDNS
    if (init_mdns()) {
        Serial.println("mDNS service initialized successfully");
        
        // Тестуємо пошук інших пристроїв (опціонально)
        Serial.println("Testing mDNS lookup...");
        resolve_mdns_host("vissen");
    }

    ws.onEvent(onEvent);
    server.addHandler(&ws);
    server.begin();
    
    Serial.println("Web server started!");
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