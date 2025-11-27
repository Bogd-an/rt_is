const int ENA = 10; // PWM керування швидкістю двигуна A
const int ENB = 11; // PWM керування швидкістю двигуна B
const int IN1 = 12; // напрямок двигуна A
const int IN3 = 13; // напрямок двигуна B
const int SPEED = 255; // Швидкість

char buffer[20];
int bufferIndex = 0;

void setup() {
  Serial.begin(9600);

  pinMode(ENA, OUTPUT);
  pinMode(ENB, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN3, OUTPUT);

  motor(0, 0, 0, 0); // Зупинити двигуни на початку

  Serial.println("Ready to receive");
  Serial.flush();
}

void loop() {
  if (Serial.available()) {
    char inChar = Serial.read();
    if (inChar == '\n') {
      buffer[bufferIndex] = '\0';

      // Видалити \r в кінці рядка
      if (bufferIndex > 0 && buffer[bufferIndex - 1] == '\r') {
        buffer[--bufferIndex] = '\0';
      }

      handleCommand(buffer);
      Serial.print(buffer);
      Serial.println();
      Serial.flush();
      bufferIndex = 0;
    } else if (bufferIndex < sizeof(buffer) - 1) {
      buffer[bufferIndex++] = inChar;
    }
  }
}

void handleCommand(const char* cmd) {
  if (strcmp(cmd, "w") == 0) {
    // Вперед (обидва двигуни вперед)
    motor(1, 1, SPEED, SPEED);
  } else if (strcmp(cmd, "s") == 0) {
    // Назад (обидва двигуни назад)
    motor(0, 0, SPEED, SPEED);
  } else if (strcmp(cmd, "a") == 0) {
    // Поворот вліво: праве колесо вперед
    // ліве колесо зупинене (або назад)
    // праве вперед
    motor(0, 1, 0, SPEED);
  } else if (strcmp(cmd, "d") == 0) {
    // Поворот вправо: ліве колесо вперед
    // ліве вперед
    // праве зупинене (або назад)
    motor(1, 0, SPEED, 0);
  } else if (strcmp(cmd, "halt") == 0) {
    // ліве колесо зупинене (або назад)
    // праве колесо зупинене (або назад)
    motor(0, 0, 0, 0);
  }
}

void motor(bool leftForward, bool rightForward, int leftSpeed, int rightSpeed) {
  digitalWrite(IN1, leftForward);
  digitalWrite(IN3, rightForward);
  analogWrite(ENA, leftSpeed);
  analogWrite(ENB, rightSpeed);
}