# lora-phantom-trigger
A long-range wireless trigger system for high-speed industrial cameras (like the Phantom T4040). Built with ESP32-S3, LoRa (SX1262), and a PC817 optocoupler for isolated dry-contact switching.
# LoRa Wireless High-Speed Camera Trigger

A robust, two-way wireless triggering system designed specifically for industrial high-speed cameras like the **Phantom T4040**. 

Developed in Visual Studio Code using the Arduino framework and PlatformIO, this project uses a pair of Heltec ESP32-S3 LoRa V4 boards (SX1262) operating on the 915MHz band. The Camera Unit uses a PC817 optocoupler to create an electrically isolated, zero-voltage dry-contact closure, safely triggering the camera without risking damage to its internal electronics.

## 🚀 Features
* **High-Speed LoRa Link:** Configured for low latency (SF7, BW500.0) to ensure instant triggering across test ranges.
* **Two-Way Acknowledgment:** When the Camera Unit receives a fire command, it immediately transmits an acknowledgment ("K") back to the Remote Unit, triggering a 10-second confirmation flash sequence.
* **Connection Heartbeat:** The Remote Unit transmits a heartbeat ping ("H") every 10 seconds. If the Camera Unit misses pings for 25 seconds, the Connection LEDs on both units turn off to warn the operator.
* **Isolated Dry-Contact Switch:** Uses a PC817 optocoupler to ensure absolutely no voltage is passed to the camera. 
* **Industrial Pulse Timing:** Engineered with a strict 200ms trigger pulse to bypass aggressive hardware noise filters on heavy-duty cameras.
* **Internal Battery Monitoring:** Both units actively read the Heltec V4's internal battery voltage divider circuit and trigger dedicated Red LEDs when the LiPo voltage drops to critical levels.

## 🛠️ Hardware Requirements
* **2x** Heltec ESP32-S3 LoRa V3/V4 (One Remote Unit, One Camera Unit)
* **1x** PC817 Optocoupler
* **1x** 220Ω - 330Ω Resistor
* **4x** LEDs (Connection Status & Low Battery warnings for both boards)
* **1x** Push Button
* High-Speed Camera requiring a dry-contact trigger

## 🔌 Hardware Pinouts

### Camera Unit (Receiver)
| Component | Heltec V4 Pin | Logic/Note |
| :--- | :--- | :--- |
| **PC817 Optocoupler (Pin 1)** | GPIO 4 | Driven via 220Ω resistor. Outputs a 200ms HIGH pulse. |
| **PC817 Optocoupler (Pin 2)** | GND | |
| **PC817 Optocoupler (Pins 3/4)**| To Camera | **Directional!** Pin 3 to Cam GND, Pin 4 to Cam Positive. |
| **Connection Status LED** | GPIO 33 | Turns OFF if no signal for 25s. |
| **Low Battery LED (Red)** | GPIO 42 | Turns ON if battery drops below ~3.3V (ADC < 835). |

### Remote Unit (Transmitter)
| Component | Heltec V4 Pin | Logic/Note |
| :--- | :--- | :--- |
| **Trigger Push Button** | GPIO 4 | Uses `INPUT_PULLUP`. Connects to GND when pressed. |
| **Connection Status LED** | GPIO 33 | Turns OFF if no signal for 25s. |
| **Trigger Success LED** | GPIO 34 | Flashes for 10s upon receiving "K" acknowledgment. |
| **Low Battery LED (Red)** | GPIO 42 | Turns ON if battery drops below ~3.4V (ADC < 860). |

*Note: GPIO 1 (VBAT_PIN) and GPIO 37 (VBAT_CTRL) are reserved on both boards for the Heltec internal battery voltage divider circuit.*

## 📁 Project Structure & Compilation

This project is configured for **PlatformIO** in Visual Studio Code. It uses a single project workspace with a multi-environment `platformio.ini` file, allowing you to compile and flash both the Camera Unit and Remote Unit seamlessly.

### Directory Setup
Ensure your source files are named appropriately and placed inside the `src/` folder:
```text
esp32-lora-camera-trigger/
│
├── platformio.ini
└── src/
    ├── camera.cpp
    └── remote.cpp
