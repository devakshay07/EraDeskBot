#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <FluxGarage_RoboEyes.h>
#include <ESP32Servo.h>
#include "esp_task_wdt.h"
#include <WiFi.h>
#include <ArduinoOTA.h>
#include <NimBLEDevice.h>

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
int panPos = 90, tiltPos = 90;
int targetPan = 90, targetTilt = 90;

/* TIMING & STATE */
unsigned long lastMove = 0;
unsigned long lastServoUpdate = 0;
const int moveInterval = 2000;
const int servoSpeed = 15;
bool restMode = true; // Start asleep until we see someone
unsigned long lastEyeMove = 0;

/* BLE PRESENCE DETECTION */
unsigned long lastHighRssiTime = 0;
const int RSSI_THRESHOLD = -55; // Adjust this threshold based on room size
const unsigned long SLEEP_TIMEOUT = 60000; // 1 minute without seeing you = sleep

class MyAdvertisedDeviceCallbacks: public NimBLEAdvertisedDeviceCallbacks {
    void onResult(NimBLEAdvertisedDevice* advertisedDevice) {
        if (advertisedDevice->getRSSI() > RSSI_THRESHOLD) {
            lastHighRssiTime = millis();
        }
    }
};

/* ================= SERVO ================= */
void updateServo() {
  if (millis() - lastServoUpdate < servoSpeed) return;
  lastServoUpdate = millis();

  if (panPos < targetPan) panPos++;
  else if (panPos > targetPan) panPos--;
  if (tiltPos < targetTilt) tiltPos++;
  else if (tiltPos > targetTilt) tiltPos--;

  panPos = constrain(panPos, 0, 180);
  tiltPos = constrain(tiltPos, 0, 180);
  panServo.write(panPos);
  tiltServo.write(tiltPos);
}

/* ================= IDLE SCAN & PRESENCE ================= */
void idleScan() {
  // If we saw a strong BLE signal within the last minute, stay awake!
  if (millis() - lastHighRssiTime < SLEEP_TIMEOUT) {
    if (restMode) {
      // Waking up sequence
      restMode = false;
      roboEyes.setMood(DEFAULT);
      roboEyes.setIdleMode(true, 4, 2);
    }
    // Occasionally move head while awake
    if (millis() - lastMove > moveInterval) {
      targetPan = random(20, 160);
      targetTilt = random(60, 120);
      lastMove = millis();
    }
  } else {
    // Timeout reached, no strong BLE signals. Go to sleep.
    if (!restMode) {
      restMode = true;
      targetPan = 90;
      targetTilt = 90;
      roboEyes.setMood(TIRED);
      roboEyes.setIdleMode(false);
      roboEyes.setPosition(DEFAULT);
    }
    // Occasional twitch/dream while sleeping
    if (millis() - lastEyeMove > 5000) {
      if (random(0, 10) > 8) roboEyes.animConfused();
      lastEyeMove = millis();
    }
  }
}

/* ================= SETUP ================= */
void setup() {
  Serial.begin(115200);
  randomSeed(esp_random());

  /* I2C & Display */
  Wire.begin(SDA_PIN, SCL_PIN);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    while (true) delay(100);
  }
  roboEyes.begin(SCREEN_WIDTH, SCREEN_HEIGHT, 100);
  roboEyes.setAutoblinker(true, 3, 2);
  roboEyes.setIdleMode(false); // We start asleep
  roboEyes.setMood(TIRED);

  /* Servos */
  panServo.attach(PAN_PIN, 500, 2500);
  tiltServo.attach(TILT_PIN, 500, 2500);
  panServo.write(panPos);
  tiltServo.write(tiltPos);
  delay(300);

  /* BLE Passive Scanning */
  NimBLEDevice::init("");
  NimBLEScan* pBLEScan = NimBLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(false); // Passive scan uses less power & CPU
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(50); // Scan 50% of the time to leave radio room for WiFi
  pBLEScan->start(0, nullptr, false); // 0 = scan forever, false = non-blocking

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
