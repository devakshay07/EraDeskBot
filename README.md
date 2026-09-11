# 🤖 Era DeskBot - The ESP32-C3 Smart Desk Companion

![Era DeskBot Banner](https://img.shields.io/badge/Era_DeskBot-Active-brightgreen?style=for-the-badge&logo=robot)
![Platform](https://img.shields.io/badge/Platform-ESP32--C3-orange)
![License](https://img.shields.io/badge/license-MIT-blue.svg)

Meet **Era DeskBot**, a tiny, expressive, and zero-setup smart desk companion built on the **ESP32-C3 Supermini**. Era watches you work, gets bored when you ignore it, tracks your physical presence using invisible BLE fields, and judges your code silently.

> **Keywords:** `ESP32-C3 Supermini`, `Desk Companion Robot`, `Desktop Robot`, `Arduino`, `PlatformIO`, `BLE Proximity Detection`, `Servo Robot Eyes`, `OLED SSD1306 Robot`, `Open Source Robot`, `DIY AI Assistant Hardware`

---

## ⚡ Key Features

- **Zero-Setup BLE Proximity Detection:** Era automatically detects when you sit at your desk by passively scanning for the raw BLE signals emitted by your iPhone, Apple Watch, or AirPods. No apps, no pairing, no hardcoded MAC addresses. Just walk up, and it wakes up.
- **Physical Pomodoro Timer (Touch Sensor):** Tap a capacitive touch sensor (GPIO 10) to instantly transform Era into a focus timer. The robot halts its animations, stares straight ahead, and displays a crisp Pomodoro UI on its OLED. Tap to add 5 minutes, hold to start/pause. When the timer hits zero, the bot physically shakes its head to alert you.
- **Expressive OLED Eyes:** Uses the `FluxGarage RoboEyes` library for smooth, animated expressions (blinking, tiredness, confusion).
- **On-Demand OTA Updates (No Stutter):** To prevent radio bottlenecks, Wi-Fi stays physically powered off. Pressing the physical BOOT button (GPIO 9) kills the BLE scanner to give 100% of the radio to Wi-Fi for exactly 5 minutes. The bot continues to animate and look around normally while listening for an Over-The-Air firmware flash. After 5 minutes, it kills the Wi-Fi and reboots the BLE scanner to restore maximum performance.
- **Smooth 2-Axis Motion:** Pan and tilt tracking using dual servos with built-in software easing for fluid movements.
- **State Machine Architecture:** Non-blocking `millis()` based loops and hardware Watchdog protection.

---

## 🛠️ Hardware Requirements

- **Microcontroller:** ESP32-C3 Supermini
- **Display:** 0.96" I2C OLED Display (SSD1306)
- **Actuators:** 2x Micro Servos (Pan and Tilt). *Upgrading to MG90S metal-gear servos is highly recommended over standard SG90s!*
- **Power (CRITICAL):** Do NOT power the servos directly from the ESP32 3.3V/5V rails. Use a dedicated 5V buck converter (>= 2A) and tie the grounds together.

---

## 📌 Circuit Diagram & Wiring

```mermaid
graph LR
    PWR[5V 2A Power Supply] -->|5V| VCC_SERVO[Servo VCC]
    PWR -->|GND| GND_SERVO[Servo GND]
    PWR -->|5V| VCC_ESP[ESP32 5V In]
    PWR -->|GND| GND_ESP[ESP32 GND]
    
    ESP[ESP32-C3 Supermini] -->|GPIO 4 - SDA| OLED[SSD1306 OLED]
    ESP -->|GPIO 5 - SCL| OLED
    ESP -->|3.3V| OLED
    ESP -->|GND| OLED
    
    ESP -->|GPIO 7 - PWM| PAN[Pan Servo]
    ESP -->|GPIO 6 - PWM| TILT[Tilt Servo]
    
    classDef esp fill:#2b2b2b,stroke:#ff8800,stroke-width:2px,color:#fff;
    classDef pwr fill:#d43f3a,stroke:#333,stroke-width:2px,color:#fff;
    class ESP esp;
    class PWR pwr;
```

---

## 🧠 Logic Flowchart

```mermaid
stateDiagram-v2
    [*] --> Setup
    Setup --> BLE_Passive_Scan : Init Radio (WiFi OFF)
    Setup --> Asleep : Default State
    
    state Asleep {
        direction LR
        Sleeping --> Twitch : Random Interval
        Twitch --> Sleeping : Confused Dream
    }
    
    state Awake {
        direction LR
        Active --> LookAround : Native IdleMode
        LookAround --> Active : Servo Pan/Tilt
    }
    
    state OTA_Mode {
        direction LR
        WiFi_ON --> Listening : ArduinoOTA
    }
    
    BLE_Passive_Scan --> Detect_Signal : Any BLE Device > -55 dBm
    
    Detect_Signal --> Awake : Wake up Trigger
    Awake --> Asleep : No signal for 60s
    
    Asleep --> OTA_Mode : Press BOOT Button (GPIO 9)
    Awake --> OTA_Mode : Press BOOT Button
    OTA_Mode --> Asleep : 5 Minute Timeout (WiFi OFF)
```

---

## 📦 Required Libraries
If you're using PlatformIO, these are automatically installed via `platformio.ini`. If you're using the standard **Arduino IDE**, install these via the Library Manager:
- `Adafruit GFX Library` by Adafruit
- `Adafruit SSD1306` by Adafruit
- `ESP32Servo` by Kevin Harrington
- `FluxGarage RoboEyes` by FluxGarage
- `NimBLE-Arduino` by h2zero

---

## 🚀 Getting Started

This project is natively configured for **PlatformIO**.

1. Clone the repository:
   ```bash
   git clone https://github.com/devakshay07/EraDeskBot.git
   cd EraDeskBot
   ```
2. *(Optional)* Update `ssid` and `password` in `src/main.cpp` to enable OTA updates. Leave default to run offline perfectly.
3. Open the folder in VSCode with the PlatformIO extension installed.
4. Build and Upload to your ESP32-C3 Supermini.

---

## 🙏 Credits & Open Source
Massive thanks to:
- [FluxGarage/RoboEyes](https://github.com/FluxGarage/RoboEyes) 
- [Adafruit GFX](https://github.com/adafruit/Adafruit-GFX-Library) 
- [NimBLE-Arduino](https://github.com/h2zero/NimBLE-Arduino)
- [ESP32Servo](https://github.com/madhephaestus/ESP32Servo) 

## 📜 License
Licensed under the **MIT License**. See `LICENSE` for details.
