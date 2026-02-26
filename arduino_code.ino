/* ESP32 Smart Security + Radar + Blynk (FreeRTOS dual-core)
   Updated by  Lohith ❤️
   NEW:
   - V5 LED: Radar detection alert
   - Radar distance (V1) scaled from 0–100
*/

#define BLYNK_TEMPLATE_ID "TMPL3wt47Qyg5"
#define BLYNK_TEMPLATE_NAME "smart security"
#define BLYNK_AUTH_TOKEN "5e68AywnmTcnvYZeJS8S9pi2-oDeRkHY"

#include <Keypad.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>
#include <NewPing.h>
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// ------------------ USER CONFIG ------------------
char BLYNK_AUTH[] = "5e68AywnmTcnvYZeJS8S9pi2-oDeRkHY";
char WIFI_SSID[] = "realme P1 5G";
char WIFI_PASS[] = "12345678";
// -------------------------------------------------

// ---------------------- PIN MAP ----------------------
#define I2C_SDA 21
#define I2C_SCL 22

#define DOOR_SERVO_PIN 18
#define RADAR_SERVO_PIN 19

#define TRIG_PIN 23  
#define ECHO_PIN 34

#define PIR_PIN 35

const byte ROWS = 4;
const byte COLS = 3;
byte rowPins[ROWS] = {32, 33, 25, 26};
byte colPins[COLS] = {12, 13, 14};

#define DOOR_GREEN 2
#define DOOR_RED 15
#define RADAR_GREEN 16
#define RADAR_RED 17
#define PIR_GREEN 27
#define PIR_RED 5
#define BUZZER_PIN 4

// ---------------------- HARDWARE ----------------------
LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo doorServo;
Servo radarServo;
NewPing sonar(TRIG_PIN, ECHO_PIN, 200);

// ---------------------- KEYPAD ----------------------
char keys[ROWS][COLS] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// ---------------------- PASSWORD ----------------------
String enteredPassword = "";
String correctPassword = "1234";
bool keypadBlocked = false;

// ---------------------- RADAR ----------------------
volatile int radarAngle = 90;
volatile int radarDistance = 50;

const int radar_min = 15;
const int radar_max = 180;
const int radar_stepDelayMs = 10;

unsigned long lastBlynkUpdate = 0;
const unsigned long BLYNK_UPDATE_MS = 500;

TaskHandle_t radarTaskHandle = NULL;

// ---------------------- Smooth Door ----------------------
void smoothDoorOpenClose() {
  for (int p = 0; p <= 90; p++) { doorServo.write(p); delay(10); }
  delay(2000);
  for (int p = 90; p >= 0; p--) { doorServo.write(p); delay(10); }
}

// ---------------------- BLYNK CALLBACKS ----------------------
// V0: Manual Door Open
BLYNK_WRITE(V0) {
  int v = param.asInt();
  if (v == 1 && !keypadBlocked) {
    digitalWrite(DOOR_GREEN, HIGH);
    digitalWrite(DOOR_RED, LOW);
    lcd.clear();
    lcd.print("Manual Open");
    smoothDoorOpenClose();
    digitalWrite(DOOR_GREEN, LOW);
    lcd.clear();
    lcd.print("Enter Password:");
    lcd.setCursor(0,1);
  }
}

// V7: Block Keypad
BLYNK_WRITE(V7) {
  keypadBlocked = param.asInt();
  lcd.clear();
  if (keypadBlocked) {
    lcd.print("Keypad BLOCKED");
  } else {
    lcd.print("Enter Password:");
  }
  lcd.setCursor(0,1);
}

// ---------------------- RADAR TASK (Core 0) ----------------------
void radarTask(void *pv) {
  while (true) {

    // Sweep UP
    for (int a = radar_min; a <= radar_max; a++) {
      radarServo.write(a);
      radarAngle = a;

      int d = sonar.ping_cm();
      radarDistance = (d == 0 ? 50 : d);

      Serial.print(a);
      Serial.print(",");
      Serial.print(radarDistance);
      Serial.print(".\n");

      // Hardware radar LED
      if (radarDistance < 40) {
        digitalWrite(RADAR_GREEN, LOW);
        digitalWrite(RADAR_RED, HIGH);
      } else {
        digitalWrite(RADAR_GREEN, HIGH);
        digitalWrite(RADAR_RED, LOW);
      }

      vTaskDelay(pdMS_TO_TICKS(radar_stepDelayMs));
    }

    // Sweep DOWN
    for (int a = radar_max; a >= radar_min; a--) {
      radarServo.write(a);
      radarAngle = a;

      int d = sonar.ping_cm();
      radarDistance = (d == 0 ? 50 : d);

      Serial.print(a);
      Serial.print(",");
      Serial.print(radarDistance);
      Serial.print(".\n");

      if (radarDistance < 40) {
        digitalWrite(RADAR_GREEN, LOW);
        digitalWrite(RADAR_RED, HIGH);
      } else {
        digitalWrite(RADAR_GREEN, HIGH);
        digitalWrite(RADAR_RED, LOW);
      }

      vTaskDelay(pdMS_TO_TICKS(radar_stepDelayMs));
    }

  }
}

// ---------------------- SETUP ----------------------
void setup() {
  Serial.begin(9600);

  WiFi.mode(WIFI_STA);
  Blynk.begin(BLYNK_AUTH, WIFI_SSID, WIFI_PASS);

  lcd.init();
  lcd.backlight();

  pinMode(PIR_PIN, INPUT);

  pinMode(DOOR_GREEN, OUTPUT);
  pinMode(DOOR_RED, OUTPUT);
  pinMode(RADAR_GREEN, OUTPUT);
  pinMode(RADAR_RED, OUTPUT);
  pinMode(PIR_GREEN, OUTPUT);
  pinMode(PIR_RED, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  digitalWrite(DOOR_GREEN, LOW);
  digitalWrite(DOOR_RED, LOW);
  digitalWrite(PIR_GREEN, HIGH);
  digitalWrite(PIR_RED, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  doorServo.attach(DOOR_SERVO_PIN, 500, 2400);
  radarServo.attach(RADAR_SERVO_PIN, 500, 2400);

  doorServo.write(0);
  radarServo.write(90);

  lcd.clear();
  lcd.print("Enter Password:");
  lcd.setCursor(0,1);

  xTaskCreatePinnedToCore(
    radarTask,
    "RadarTask",
    4096,
    NULL,
    1,
    &radarTaskHandle,
    0
  );

  delay(200);
}

// ---------------------- MAIN LOOP (Core 1) ----------------------
void loop() {

  Blynk.run();

  // ---------- PIR ----------
  if (digitalRead(PIR_PIN)) {
    digitalWrite(PIR_GREEN, LOW);
    digitalWrite(PIR_RED, HIGH);
    digitalWrite(BUZZER_PIN, HIGH);
    Blynk.virtualWrite(V2, 255);  // PIR LED
  } else {
    digitalWrite(PIR_GREEN, HIGH);
    digitalWrite(PIR_RED, LOW);
    digitalWrite(BUZZER_PIN, LOW);
    Blynk.virtualWrite(V2, 0);
  }

  // ---------- KEY PAD ----------
  if (!keypadBlocked) {
    char key = keypad.getKey();
    if (key) {
      if (key == '#') {
        lcd.clear();
        if (enteredPassword == correctPassword) {
          digitalWrite(DOOR_GREEN, HIGH);
          digitalWrite(DOOR_RED, LOW);
          lcd.print("Access Granted");

          Blynk.virtualWrite(V4, 255);  // door LED ON

          smoothDoorOpenClose();

          digitalWrite(DOOR_GREEN, LOW);
          Blynk.virtualWrite(V4, 0);
        } else {
          digitalWrite(DOOR_RED, HIGH);
          lcd.print("Wrong Pass");
          Blynk.virtualWrite(V6, 255);
          delay(1200);
          digitalWrite(DOOR_RED, LOW);
          Blynk.virtualWrite(V6, 0);
        }
        enteredPassword = "";
        lcd.clear();
        lcd.print("Enter Password:");
        lcd.setCursor(0,1);
      }
      else if (key == '*') {
        enteredPassword = "";
        lcd.clear();
        lcd.print("Enter Password:");
        lcd.setCursor(0,1);
      }
      else if (enteredPassword.length() < 4) {
        enteredPassword += key;
        lcd.print("*");
      }
    }
  }

  // ---------- BLYNK PERIODIC UPDATES ----------
  if (millis() - lastBlynkUpdate > BLYNK_UPDATE_MS) {
    lastBlynkUpdate = millis();

    int rd = radarDistance;
    int ra = radarAngle;

    // Scale V1 from 0–50 → 0–100
    int scaled = (rd * 100) / 50;
    if (scaled > 100) scaled = 100;

    Blynk.virtualWrite(V1, scaled);  // scaled radar distance
    Blynk.virtualWrite(V3, ra);      // radar angle

    // NEW: V5 LED → Radar object detection
    if (rd < 40) Blynk.virtualWrite(V5, 255);
    else Blynk.virtualWrite(V5, 0);

    // door status
    Blynk.virtualWrite(V4, digitalRead(DOOR_GREEN) ? 255 : 0);

    // mirror keypad lock state
    Blynk.virtualWrite(V7, keypadBlocked ? 1 : 0);
  }

  delay(10);
}
