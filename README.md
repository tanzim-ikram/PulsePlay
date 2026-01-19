# PulsePlay – ESP32 Firmware

PulsePlay is a mood-based music recommendation device built on ESP32.
This repository contains the **firmware** responsible for hardware interaction, UI rendering, network communication, and micro-interactions.

The device allows users to select a mood using a rotary encoder and sends that mood to an AI backend, which recommends music via a web application.


## ✨ Features

### 🎛️ Tangible Interaction

* Rotary encoder with acceleration (fast scroll = faster navigation)
* Short press → select mood
* Long press → mood preview
* Double press → random “Surprise me” mood

### 🖥️ OLED UI (128×64)

* Boot screen
* Instruction screen
* Scrollable mood menu
* Mood preview screen
* Sending & result screens
* Smooth transitions and feedback

### 💡 NeoPixel Status LED (1× RGB)

| State               | Behavior               |
| ------------------- | ---------------------- |
| Device Off          | LED Off                |
| Wi-Fi Not Connected | Blinking Red           |
| Wi-Fi Connected     | Solid Green            |
| Idle                | Breathing Green        |
| Sending Mood        | Fade In / Out          |
| Error               | Rapid Triple Red Blink |

### 🔊 Audio Feedback (Piezo Buzzer)

* Soft click on encoder rotation
* Confirmation tone on successful send
* Error tone on failure

### 🌐 Networking

* Wi-Fi setup using **WiFiManager**
* Secure HTTPS communication
* Sends mood as JSON to backend API
* Handles network errors gracefully


## 🧠 System Overview

```
[ Rotary Encoder ] ---> Mood Selection
        |
        v
[ OLED Display ] <--- UI Feedback
        |
        v
[ ESP32 ] ---> HTTPS Request ---> AI Backend
        |
        v
[ Web App ] ---> Music Recommendation
```


## 🔧 Hardware Requirements

| Component                    | Quantity  |
| ---------------------------- | --------- |
| ESP32                        | 1         |
| OLED Display (128×64, I²C)   | 1         |
| Rotary Encoder (with button) | 1         |
| NeoPixel RGB LED             | 1         |
| Piezo Buzzer                 | 1         |
| Jumper Wires                 | As needed |


## 🔌 Pin Configuration

### OLED (I²C)

| OLED | ESP32  |
| ---- | ------ |
| SDA  | GPIO 8 |
| SCL  | GPIO 9 |
| VCC  | 3.3V   |
| GND  | GND    |

### Rotary Encoder

| Encoder | ESP32  |
| ------- | ------ |
| CLK     | GPIO 2 |
| DT      | GPIO 1 |
| SW      | GPIO 0 |
| VCC     | 3.3V   |
| GND     | GND    |

### NeoPixel LED

| NeoPixel | ESP32     |
| -------- | --------- |
| DIN      | GPIO 3    |
| VCC      | 3.3V / 5V |
| GND      | GND       |

### Piezo Buzzer

| Buzzer | ESP32  |
| ------ | ------ |
| +      | GPIO 4 |
| −      | GND    |


## 📦 Required Libraries

Install the following libraries via **Arduino Library Manager**:

* `WiFiManager`
* `Adafruit SSD1306`
* `Adafruit GFX`
* `Adafruit NeoPixel`
* `HTTPClient` (built-in)
* `WiFiClientSecure` (built-in)


## ⚙️ Setup Instructions

1. **Clone the repository**

   ```bash
   git clone https://github.com/your-username/pulseplay-esp32.git
   ```

2. **Open the project**

   * Open `PulsePlay.ino` in Arduino IDE

3. **Select Board**

   * Board: **ESP32 Dev Module**
   * Port: Your ESP32 serial port

4. **Install required libraries**

5. **Upload the code**


## 📶 Wi-Fi Configuration

* On first boot, the ESP32 opens a WiFiManager portal
* Connect to the hotspot **“PulsePlay”**
* Enter your Wi-Fi credentials
* Device auto-reconnects on future boots


## 🔐 Backend Configuration

The firmware sends mood data to:

```
https://pulseplay-server-production.up.railway.app/api/recommend
```

Payload example:

```json
{
  "mood": "Calm"
}
```

The backend must respond with HTTP `200 OK` for success.


## 🧪 Interaction Summary

| Action                | Result                |
| --------------------- | --------------------- |
| Rotate encoder slowly | Precise scrolling     |
| Rotate encoder fast   | Accelerated scrolling |
| Short press           | Send selected mood    |
| Long press            | Mood preview          |
| Double press          | Random mood           |
| Wi-Fi error           | Red LED + error tone  |


## 🚀 Future Improvements

* RGB LED mood-specific colors
* Haptic feedback (vibration motor)
* BLE support for direct music control
* Mood learning & history
* Companion mobile app

## 🧩 Project Context

PulsePlay explores **Tangible User Interfaces (TUI)** by blending physical interaction with digital intelligence.
The firmware prioritizes **expressiveness, feedback richness, and emotional clarity** over raw functionality.


## 📄 License

MIT License
Free to use, modify, and distribute.
