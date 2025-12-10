#include <SoftwareSerial.h> // Використовуйте стандартну бібліотеку, якщо SoftwareSerial1 це просто перейменування
SoftwareSerial mySerial(2, 3); // Rename to avoid confusion regarding standard libs
#include <Servo.h>

// Motor control pins (L298P Motor Shield)
const int ENA = 10;
const int ENB = 11;
const int IN1 = 12;
const int IN3 = 13;
const int SPEED = 255;

// Servo pins
const int PAN_PIN  = 6;
const int TILT_PIN = 9;

int panAngle = 90;
int tiltAngle = 90;
int lastPanAngle = 90;   // Для перевірки змін
int lastTiltAngle = 90;  // Для перевірки змін

// Таймери для вимкнення серво
unsigned long lastServoMoveTime = 0;
const int SERVO_TIMEOUT = 500; // Час у мс, через який серво вимкнеться після руху
bool servosAttached = false;

// Servo objects
Servo panServo;
Servo tiltServo;

// Serial buffer
char buffer[30];
int bufferIndex = 0;

void setup() {
  Serial.begin(9600);
  mySerial.begin(9600);

  // Motor pins
  pinMode(ENA, OUTPUT);
  pinMode(ENB, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN3, OUTPUT);
  stopMotors();

  // Attach servos initially
  attachServos();
  panServo.write(panAngle);
  tiltServo.write(tiltAngle);

  mySerial.println("Ready to receive");
  Serial.println("Ready to receive");
  mySerial.flush();
}

void loop() {
  // Читання Serial
  if (mySerial.available()) {
    char inChar = mySerial.read();
    if (inChar == '\n') {
      buffer[bufferIndex] = '\0';
      if (bufferIndex > 0 && buffer[bufferIndex - 1] == '\r') {
        buffer[--bufferIndex] = '\0';
      }

      handleCommand(buffer);
      // mySerial.print(buffer); // Краще прибрати зайвий вивід назад, це завантажує канал
      // mySerial.println();
      bufferIndex = 0;
      
    } else if (bufferIndex < sizeof(buffer) - 1) {
      buffer[bufferIndex++] = inChar;
    }
  }

  // Логіка вимкнення серво для тиші
  if (servosAttached && (millis() - lastServoMoveTime > SERVO_TIMEOUT)) {
    detachServos();
  }
}

void attachServos() {
  if (!servosAttached) {
    panServo.attach(PAN_PIN);
    tiltServo.attach(TILT_PIN);
    servosAttached = true;
  }
}

void detachServos() {
  if (servosAttached) {
    panServo.detach();
    tiltServo.detach();
    servosAttached = false;
  }
}

void stopMotors() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN3, LOW);
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);
}

void handleCommand(const char* cmd) {
  // --- Motor Commands ---
  if (strcmp(cmd, "w") == 0) {
    digitalWrite(IN1, HIGH); digitalWrite(IN3, HIGH);
    analogWrite(ENA, SPEED); analogWrite(ENB, SPEED);
  } else if (strcmp(cmd, "s") == 0) {
    digitalWrite(IN1, LOW); digitalWrite(IN3, LOW);
    analogWrite(ENA, SPEED); analogWrite(ENB, SPEED);
  } else if (strcmp(cmd, "a") == 0) {
    digitalWrite(IN1, LOW); digitalWrite(IN3, HIGH);
    analogWrite(ENA, 0); analogWrite(ENB, SPEED);
  } else if (strcmp(cmd, "d") == 0) {
    digitalWrite(IN1, HIGH); digitalWrite(IN3, LOW);
    analogWrite(ENA, SPEED); analogWrite(ENB, 0);
  } else if (strcmp(cmd, "halt") == 0) {
    stopMotors();
  } 
  
  // --- Servo Commands ---
  else if (strncmp(cmd, "pan:", 4) == 0) {
    int val = atoi(cmd + 4);
    val = constrain(val, 0, 180);

    // Фільтр: рухаємося тільки якщо зміна > 2 градусів
    if (abs(val - lastPanAngle) > 2) {
      attachServos(); // Прокидаємося
      panServo.write(val);
      panAngle = val;
      lastPanAngle = val;
      lastServoMoveTime = millis(); // Скидаємо таймер сну
    }

  } else if (strncmp(cmd, "tilt:", 5) == 0) {
    int val = atoi(cmd + 5);
    val = constrain(val, 0, 180);

    // Фільтр: рухаємося тільки якщо зміна > 2 градусів
    if (abs(val - lastTiltAngle) > 2) {
      attachServos(); // Прокидаємося
      tiltServo.write(val);
      tiltAngle = val;
      lastTiltAngle = val;
      lastServoMoveTime = millis(); // Скидаємо таймер сну
    }
  }
}