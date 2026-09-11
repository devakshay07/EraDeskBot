#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <FluxGarage_RoboEyes.h>
#include <ESP32Servo.h>
#include "esp_task_wdt.h"
#include <WiFi.h>
#include <ArduinoOTA.h>

/* WIFI & OTA CREDENTIALS */
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

/* OLED */
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define SDA_PIN 4
#define SCL_PIN 5

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
RoboEyes<Adafruit_SSD1306> roboEyes(display);

/* SERVOS */
Servo panServo;
Servo tiltServo;

#define PAN_PIN 7
#define TILT_PIN 6

int panPos = 90;
int tiltPos = 90;

int targetPan = 90;
int targetTilt = 90;

/* TIMING */
unsigned long lastMove = 0;
unsigned long lastServoUpdate = 0;

const int moveInterval = 2000;
const int servoSpeed = 15;

bool restMode = false;

unsigned long stateTimer = 0;

const unsigned long ACTIVE_TIME = 60000;   // 1 minute
const unsigned long REST_TIME   = 30000;   // 30 seconds

unsigned long lastEyeMove = 0;

/* ================= SERVO ================= */

void updateServo() {

  if (millis() - lastServoUpdate < servoSpeed)
    return;

  lastServoUpdate = millis();

  if (panPos < targetPan)
    panPos++;
  else if (panPos > targetPan)
    panPos--;

  if (tiltPos < targetTilt)
    tiltPos++;
  else if (tiltPos > targetTilt)
    tiltPos--;

  panPos = constrain(panPos, 0, 180);
  tiltPos = constrain(tiltPos, 0, 180);

  panServo.write(panPos);
  tiltServo.write(tiltPos);
}

/* ================= IDLE ================= */

void idleScan() {

  int positions[] = {N, NE, E, SE, S, SW, W, NW, DEFAULT};

  if (!restMode) {

    if (millis() - lastMove > moveInterval) {
      targetPan = random(20, 160);
      targetTilt = random(60, 120);
      // We rely on roboEyes native idleMode for the eye movements!
      lastMove = millis();
    }

    if (millis() - stateTimer >= ACTIVE_TIME) {
      restMode = true;
      stateTimer = millis();
      targetPan = 90;
      targetTilt = 90;
      
      roboEyes.setMood(TIRED); // Get sleepy
      roboEyes.setIdleMode(false); // Stop looking around actively
      roboEyes.setPosition(DEFAULT); // Center the eyes
    }
  }

  else {

    targetPan = 90;
    targetTilt = 90;

    if (millis() - lastEyeMove > 5000) {
      // Randomly do something while sleeping
      int r = random(0, 10);
      if (r > 8) roboEyes.animConfused(); // Have a weird dream
      lastEyeMove = millis();
    }

    if (millis() - stateTimer >= REST_TIME) {
      restMode = false;
      stateTimer = millis();
      
      roboEyes.setMood(DEFAULT); // Wake up
      roboEyes.setIdleMode(true, 4, 2); // Start actively looking around again
    }
  }
}


/* ================= SETUP ================= */

void setup() {

  Serial.begin(115200);

  randomSeed(esp_random());

  Wire.begin(SDA_PIN, SCL_PIN);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    while (true) {
      delay(100);
    }
  }

  roboEyes.begin(SCREEN_WIDTH, SCREEN_HEIGHT, 100);

  roboEyes.setAutoblinker(true, 3, 2);
  roboEyes.setIdleMode(true, 4, 2);
  roboEyes.setCuriosity(true);


  stateTimer = millis();

  /* Attach servos with pulse width */

  panServo.attach(PAN_PIN, 500, 2500);
  tiltServo.attach(TILT_PIN, 500, 2500);

  panServo.write(panPos);
  tiltServo.write(tiltPos);

  delay(300);

  /* Watchdog */

  esp_task_wdt_config_t wdt_config = {
      .timeout_ms = 5000,
      .idle_core_mask = (1 << 0),
      .trigger_panic = true
  };

  esp_task_wdt_init(&wdt_config);
  esp_task_wdt_add(NULL);

  /* WiFi & OTA (Non-Blocking) */
  if (String(ssid) != "YOUR_WIFI_SSID") {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    ArduinoOTA.setHostname("EraDeskBot");
    ArduinoOTA.begin();
  }
}

/* ================= LOOP ================= */

void loop() {

  esp_task_wdt_reset();

  if (WiFi.status() == WL_CONNECTED) {
    ArduinoOTA.handle();
  }

  idleScan();

  updateServo();

  roboEyes.update();
}
