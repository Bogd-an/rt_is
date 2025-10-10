#if !defined(__AVR_ATmega328P__)
  #error "❌ не та платформа"
#endif

#include <SoftwareSerial.h>

SoftwareSerial Serial1(2, 3); 

const int ledPin = 13;

char buffer[64];
int bufferIndex = 0;

void parseCommand() {
    if (strcmp(buffer, "on") == 0) {
        digitalWrite(ledPin, 1);
        sendLog("OK: LED is ON");
    } 
    else if (strcmp(buffer, "off") == 0) {
        digitalWrite(ledPin, 0);
        sendLog("OK: LED is OFF");
    } 
    else {
        sendLog("Error: Unknown command '"+String(buffer)+"'");
    }
}

void sendLog(const String &message) {
    Serial .println(message);
    Serial1.println(message);
}

void setup() {
    pinMode(ledPin, OUTPUT);
    digitalWrite(ledPin, 0);

    Serial .begin(9600);
    Serial1.begin(9600);

    Serial.println("Uno waiting for commands from ESP32...");
}

void loop() {
    while (Serial1.available()) {
        char inChar = (char)Serial1.read();
        if (inChar == '\n') {
            buffer[bufferIndex] = '\0';
            if (bufferIndex > 0) {
                Serial.print("Received from ESP32 -> ");
                Serial.println(buffer);
                parseCommand();
            }
            bufferIndex = 0;
        } 
        else if (inChar >= 32) { 
            if (bufferIndex < sizeof(buffer) - 1) {
                buffer[bufferIndex++] = inChar;
            }
        }
    }
}