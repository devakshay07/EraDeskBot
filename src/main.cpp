#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#ifdef DEFAULT
#undef DEFAULT
#endif
#include <FluxGarage_RoboEyes.h>
#include <ESP32Servo.h>
#include "esp_task_wdt.h"
#include <WiFi.h>
#include <ArduinoOTA.h>
#include <NimBLEDevice.h>

/* WIFI & OTA CREDENTIALS */
const char* ssid = "lalit kumar";
const char* password = "10101980";

/* OTA TRIGGER BUTTON */
#define OTA_BUTTON_PIN 9 // The BOOT button on the ESP32-C3 Supermini
bool otaActive = false;
bool otaInitialized = false;
unsigned long otaStartTime = 0;
const unsigned long OTA_TIMEOUT_MS = 300000; // 5 minutes

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

/* POMODORO SYSTEM */
#define TOUCH_PIN 10
enum PomoState { POMO_INACTIVE, POMO_CONFIG, POMO_RUNNING, POMO_PAUSED };
PomoState pomoState = POMO_INACTIVE;
int pomoMinutes = 0;
unsigned long pomoStartTime = 0;
unsigned long pomoRemainingMs = 0;

bool lastTouch = false;
unsigned long touchStartTime = 0;
bool touchHandled = false;

void onPomoTap() {
  if (pomoState == POMO_INACTIVE) {
    pomoState = POMO_CONFIG;
    pomoMinutes = 25; // Default starting minutes
    targetPan = 90; targetTilt = 90; // Center the head
  } else if (pomoState == POMO_CONFIG) {
    pomoMinutes += 5;
    if (pomoMinutes > 120) pomoMinutes = 5;
  } else if (pomoState == POMO_RUNNING) {
    pomoState = POMO_PAUSED;
    pomoRemainingMs = pomoRemainingMs - (millis() - pomoStartTime);
  } else if (pomoState == POMO_PAUSED) {
    pomoState = POMO_RUNNING;
    pomoStartTime = millis();
  }
}

void onPomoHold() {
  if (pomoState == POMO_CONFIG) {
    pomoState = POMO_RUNNING;
    pomoRemainingMs = pomoMinutes * 60000UL;
    pomoStartTime = millis();
  } else if (pomoState == POMO_RUNNING || pomoState == POMO_PAUSED) {
    pomoState = POMO_INACTIVE;
  }
}

void handleTouch() {
  bool currentTouch = (digitalRead(TOUCH_PIN) == HIGH); // Assumes active HIGH like TTP223
  
  if (currentTouch && !lastTouch) {
    touchStartTime = millis();
    touchHandled = false;
  }

  if (currentTouch && !touchHandled) {
    if (millis() - touchStartTime > 3000) { // 3-second hold
      touchHandled = true;
      onPomoHold();
    }
  }

  if (!currentTouch && lastTouch) {
    if (!touchHandled) {
      if (millis() - touchStartTime > 50) { // Debounce tap
        onPomoTap();
      }
    }
  }
  lastTouch = currentTouch;
}

void drawPomodoroUI() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  
  if (pomoState == POMO_CONFIG) {
    display.setTextSize(1);
    display.setCursor(35, 5);
    display.print("POMO SETUP");
    display.setTextSize(3);
    
    // Center text roughly
    if (pomoMinutes < 10) display.setCursor(45, 25);
    else if (pomoMinutes < 100) display.setCursor(35, 25);
    else display.setCursor(25, 25);
    
    display.print(pomoMinutes);
    display.print("m");
  } else {
    unsigned long remaining = pomoRemainingMs;
    if (pomoState == POMO_RUNNING) {
      if (millis() - pomoStartTime >= pomoRemainingMs) {
        remaining = 0;
      } else {
        remaining = pomoRemainingMs - (millis() - pomoStartTime);
      }
    }
    
    int mins = remaining / 60000;
    int secs = (remaining % 60000) / 1000;
    
    display.setTextSize(1);
    
    if (pomoState == POMO_PAUSED) {
      display.setCursor(45, 5);
      display.print("PAUSED");
    } else if (remaining == 0) {
      display.setCursor(45, 5);
      display.print("DONE!");
    } else {
      display.setCursor(45, 5);
      display.print("FOCUS");
    }
    
    display.setTextSize(3);
    display.setCursor(20, 25);
    if (mins < 10) display.print("0");
    display.print(mins);
    display.print(":");
    if (secs < 10) display.print("0");
    display.print(secs);
    
    // Shake head when done
    if (remaining == 0) {
      if (millis() - lastMove > 500) {
        targetPan = (targetPan == 70) ? 110 : 70;
        lastMove = millis();
      }
    } else {
      targetPan = 90; targetTilt = 90;
    }
  }
  display.display();
}

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
  // If OTA is active, artificially keep the bot fully awake so it continues to animate normally
  if (otaActive) {
    lastHighRssiTime = millis();
  }

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
      if (random(0, 10) > 8) roboEyes.anim_confused();
      lastEyeMove = millis();
    }
  }
}

/* ================= SETUP ================= */
void setup() {
  Serial.begin(115200);
  delay(2000); // Give USB CDC time to attach
  Serial.println("\n[SYSTEM] Booting EraDeskBot...");
  randomSeed(esp_random());

  /* HW Inputs */
  pinMode(OTA_BUTTON_PIN, INPUT_PULLUP);
  pinMode(TOUCH_PIN, INPUT);
  WiFi.mode(WIFI_OFF); // Start with WiFi completely off for max radio performance

  /* I2C & Display */
  Wire.begin(SDA_PIN, SCL_PIN);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("\n[ERROR] OLED not found! Check SDA (GPIO 4), SCL (GPIO 5), and power.");
    while (true) delay(100);
  }
  Serial.println("[SYSTEM] OLED initialized.");
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
  pBLEScan->setWindow(50); // Scan 50% of the time
  pBLEScan->start(0, nullptr, false); // 0 = scan forever, false = non-blocking

  /* Watchdog */
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
  esp_task_wdt_config_t wdt_config = {
      .timeout_ms = 5000,
      .idle_core_mask = (1 << 0),
      .trigger_panic = true
  };
  esp_task_wdt_init(&wdt_config);
#else
  esp_task_wdt_init(5, true);
#endif
  esp_task_wdt_add(NULL);
}

/* ================= LOOP ================= */
void loop() {
  esp_task_wdt_reset();

  handleTouch();

  /* OTA Button Logic */
  if (!otaActive && digitalRead(OTA_BUTTON_PIN) == LOW) {
    if (String(ssid) != "YOUR_WIFI_SSID") {
      otaActive = true;
      otaStartTime = millis();
      
      // Stop BLE scanning to give 100% of the antenna to WiFi
      NimBLEDevice::getScan()->stop();
      
      // Visual feedback that OTA is listening
      roboEyes.setMood(HAPPY); 
      
      WiFi.mode(WIFI_STA);
      WiFi.begin(ssid, password);
      
      if (!otaInitialized) {
        ArduinoOTA.setHostname("EraDeskBot");
        ArduinoOTA.begin();
        otaInitialized = true;
      }
    }
  }

  if (otaActive) {
    // Print IP address once it connects
    static bool ipPrinted = false;
    if (WiFi.status() == WL_CONNECTED) {
      if (!ipPrinted) {
        Serial.print("\n[OTA] Connected! IP Address: ");
        Serial.println(WiFi.localIP());
        ipPrinted = true;
      }
      ArduinoOTA.handle();
    } else {
      ipPrinted = false;
    }
    
    // Auto-disable WiFi after 5 minutes to restore performance
    if (millis() - otaStartTime > OTA_TIMEOUT_MS) {
      otaActive = false;
      WiFi.disconnect(true);
      WiFi.mode(WIFI_OFF);
      
      // Restart BLE scanning
      NimBLEDevice::getScan()->start(0, nullptr, false);
      
      // Force sleep to visually indicate OTA window closed
      lastHighRssiTime = millis() - SLEEP_TIMEOUT; 
    }
  }

  updateServo();

  if (pomoState == POMO_INACTIVE) {
    idleScan();
    roboEyes.update();
  } else {
    drawPomodoroUI();
  }
}
