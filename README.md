# 🖧 ESP32 WebSocket LED Control Web Server

## 📌 Overview
This project demonstrates how to use the **ESP32** with **ESP-IDF** to host a **WebSocket-based web server** that allows real-time control of an onboard (or external) LED from a web browser interface. The communication is bi-directional — any state change is reflected back to the client.

---

## 🚀 Features

- 📡 **WiFi SoftAP Mode** — ESP32 acts as a WiFi Access Point.
- 🌐 **Web Server with WebSocket Support** — serves HTML UI and handles WebSocket messages.
- 💡 **Real-Time LED Control** — LED ON/OFF commands sent from web interface.
- 🔁 **LED Blink Function** — Added blink animation for visual feedback.
- 📁 **SPIFFS File System** — Used to store and serve `index.html`.

---

## 📂 Project Structure

ESP32_WebSocket_LED/
├── main/
│ ├── main.c # Main application logic
│ ├── CMakeLists.txt # CMake build file
├── spiffs_image/
│ └── index.html # Web interface HTML file
├── sdkconfig # ESP-IDF configuration file
├── partition.csv # Custom partition table with SPIFFS
└── README.md # Project documentation (this file)


---

## 🛠️ Requirements

- **ESP32 Dev Board**
- **ESP-IDF v5.1.x**
- Python (with `idf.py`)
- USB to UART cable (for flashing and monitoring)

---

## 🔧 Build & Flash Instructions

1. **Set up ESP-IDF**  
   Follow the official guide: [https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/)

2. **Clone this repository**  
   ```bash
   git clone https://github.com/daniyal-944/Webserver_Project
   cd Webserver_Project
