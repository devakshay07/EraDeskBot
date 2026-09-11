# 🤖 Era DeskBot

![Era DeskBot Banner](https://img.shields.io/badge/Era_DeskBot-Active-brightgreen?style=for-the-badge&logo=robot)
![License](https://img.shields.io/badge/license-MIT-blue.svg)

Meet **Era DeskBot**, a tiny, expressive, and slightly dramatic desk companion built around the ESP32-C3 Supermini. Era watches you work, gets bored when you ignore it, and judges your code silently. 

Powered by an OLED display and dual servos, it adds a literal spark of life to your workstation.

---

## ⚡ Features
- **Expressive OLED Eyes:** Uses the FluxGarage RoboEyes library for smooth, animated expressions (blinking, looking around, acting curious).
- **Smooth 2-Axis Motion:** Pan and tilt tracking using dual servos with built-in software easing for fluid, non-jerky movements.
- **State Machine Architecture:** Non-blocking `millis()` based loops. The bot seamlessly transitions between active tracking and a dormant "rest mode".
- **Hardware Watchdog:** Bulletproof ESP-IDF watchdog implementation. If the main loop ever hangs, the bot forcefully recovers itself.

---

## 🛠️ Hardware Requirements
- **Microcontroller:** ESP32-C3 Supermini (or equivalent ESP32 board).
- **Display:** 0.96" I2C OLED Display (SSD1306).
- **Actuators:** 2x Micro Servos (Pan and Tilt). *Upgrading to MG90S metal-gear servos is highly recommended over standard SG90s!*
- **Power (CRITICAL):** Do NOT power the servos directly from the ESP32 3.3V/5V rails. Use a dedicated 5V buck converter or external power supply capable of at least 2 Amps, and tie the grounds together.

## 📌 Pinout & Wiring

| Component | ESP32-C3 Supermini Pin | Notes |
| :--- | :--- | :--- |
| **OLED SDA** | GPIO 4 | Custom routed I2C |
| **OLED SCL** | GPIO 5 | Custom routed I2C |
| **Pan Servo** | GPIO 7 | Safe GPIO for PWM |
| **Tilt Servo** | GPIO 6 | Safe GPIO for PWM |

*(Note: TILT_PIN is safely mapped to GPIO 6 to avoid boot collisions with the onboard LED / bootstrapping).*

---

## 🚀 Getting Started

This project is natively configured for **PlatformIO**.

1. Clone the repository:
   ```bash
   git clone https://github.com/devakshay07/Era-DeskBot.git
   cd Era-DeskBot
   ```
2. Open the folder in VSCode with the PlatformIO extension installed.
3. Build and Upload to your ESP32-C3 Supermini. All library dependencies are automatically downloaded via `platformio.ini`.

---

## 🙏 Credits & Open Source
This project stands on the shoulders of giants. Massive thanks to the developers of these open-source libraries:
- [FluxGarage/RoboEyes](https://github.com/FluxGarage/RoboEyes) - For the incredible expressive OLED eye animations.
- [Adafruit GFX & SSD1306](https://github.com/adafruit/Adafruit-GFX-Library) - For the core display rendering engines.
- [madhephaestus/ESP32Servo](https://github.com/madhephaestus/ESP32Servo) - For rock-solid hardware timer PWM generation on the ESP32.

---

## 📜 License
This project is open-source and licensed under the **MIT License**. See the `LICENSE` file for more details.
