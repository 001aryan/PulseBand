# PulseBand — ESP32 Wearable Heart Rate Monitor

![PulseBand prototype on wrist](arm_monitor.jpg)

PulseBand is a wearable IoT heart-rate monitoring system built using an ESP32, a pulse sensor, and an SSD1306 OLED display. The device measures pulse activity, calculates BPM (beats per minute), displays live statistics on-device, and streams real-time data to a modern web dashboard.

The project includes a frontend deployed on Vercel, a backend API hosted on Render, and Supabase integration for cloud-based storage and management of heart-rate readings.

---

# Features

* ❤️ Real-time BPM monitoring

* 📈 Live pulse waveform visualization

* 📺 OLED display integration

* 🌐 Real-time web dashboard

* ☁️ Supabase cloud database integration

* 📡 ESP32 Wi‑Fi connectivity

* 💾 CSV export support

* 🧪 Backend simulator for testing

* 🚀 Production-ready deployment with Vercel and Render

* 📈 Real-time heart-rate monitoring

* ❤️ BPM calculation and pulse waveform detection

* 📺 OLED display output using SSD1306

* 🌐 Live web dashboard with SSE streaming

* 💾 CSV log downloads

* ☁️ Optional Supabase integration

* 🧪 Backend simulator for testing without hardware

* 🔌 ESP32 + Wi‑Fi connectivity

---

# Project Structure

```text
PulseBand/
│
├── firmware/
│   ├── heart_rate_monitor_improved.ino
│   └── PageIndex.h
│
├── backend/
│   ├── server.js
│   ├── package.json
│   └── ...
│
├── website/
│   ├── index.html
│   ├── styles.css
│   └── script.js
│
├── images/
│   └── arm_monitor.jpg
│
├── supabase.sql
├── SUPABASE_SETUP.md
└── README.md
```

---

# Hardware Requirements

## Components

* ESP32 Development Board
* Pulse Sensor (analog output)
* SSD1306 OLED Display (I2C)
* Breadboard / PCB
* Jumper wires
* Power source (USB or Li‑ion battery)

## Wiring Example

| Component           | ESP32 Pin |
| ------------------- | --------- |
| Pulse Sensor Signal | GPIO 34   |
| OLED SDA            | GPIO 21   |
| OLED SCL            | GPIO 22   |
| VCC                 | 3.3V      |
| GND                 | GND       |

---

# Software Stack

## Firmware

* Arduino IDE
* ESP32 Arduino Core
* Adafruit SSD1306 Library
* Adafruit GFX Library

## Backend

* Node.js
* Express.js
* Server-Sent Events (SSE)

## Frontend

* HTML
* CSS
* JavaScript

## Database (Optional)

* Supabase
* PostgreSQL

---

# How It Works

1. The pulse sensor reads analog heartbeat signals.
2. ESP32 filters the signal and calculates BPM.
3. BPM and status are shown on the OLED display.
4. ESP32 hosts or streams data to the dashboard.
5. Backend optionally stores readings in Supabase.
6. Users can monitor readings live and export logs.

---

# Firmware Setup

## Install Arduino IDE

Download:

* [https://www.arduino.cc/en/software](https://www.arduino.cc/en/software)

## Install ESP32 Board Support

In Arduino IDE:

1. Open:

   ```
   File → Preferences
   ```

2. Add this URL to **Additional Boards Manager URLs**:

```text
https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
```

3. Open:

```text
Tools → Board → Boards Manager
```

4. Search for:

```text
ESP32
```

5. Install the ESP32 package.

---

# Required Arduino Libraries

Install these libraries using Library Manager:

* Adafruit GFX
* Adafruit SSD1306
* WiFi
* WebServer

---

# Upload Firmware

Open:

```text
firmware/heart_rate_monitor_improved.ino
```

Update Wi‑Fi credentials:

```cpp
const char* ssid = "YOUR_WIFI";
const char* password = "YOUR_PASSWORD";
```

Then:

1. Select ESP32 board
2. Select COM port
3. Upload firmware

---

# Running the Backend

## Install Dependencies

```bash
cd backend
npm install
```

## Start Server

```bash
npm start
```

Default server:

```text
http://localhost:3000
```

---

# Frontend Dashboard

Open:

```text
website/index.html
```

Features:

* Live BPM display
* Waveform visualization
* Device status
* CSV export
* Connection monitoring

---

# Supabase Integration

PulseBand uses Supabase as its cloud database for storing readings, users, alerts, and emergency contact data.

## Setup Steps

1. Create a Supabase project
2. Open SQL Editor
3. Run contents of:

```text
supabase.sql
```

4. Add environment variables:

```env
SUPABASE_URL=your_project_url
SUPABASE_ANON_KEY=your_anon_key
SUPABASE_SERVICE_KEY=your_service_key
```

## Tables

The schema creates:

* users
* readings
* emergency_contacts
* emergency_alerts

---

# Deployment

## Frontend Deployment (Vercel)

The frontend dashboard is deployed on Vercel for fast performance, automatic deployments, and global CDN delivery.

Deploy command:

```bash
vercel
```

## Backend Deployment (Render)

The backend API is deployed on Render and manages:

* Live sensor streaming
* SSE connections
* CSV logging
* Supabase integration
* REST API endpoints

Start command:

```bash
npm start
```

---

# API Overview

## SSE Stream

```http
GET /events
```

Streams live BPM and waveform data.

## Readings Endpoint

```http
GET /api/readings
```

Returns stored readings.

## CSV Export

```http
GET /api/export
```

Downloads CSV logs.

---

# Example Data

```json
{
  "bpm": 78,
  "signal": 2140,
  "status": "Connected"
}
```

---

# Future Improvements

* 📱 Mobile application
* 🔔 Emergency alert notifications
* 🧠 AI-based anomaly detection
* 📊 Historical analytics dashboard
* 🔋 Battery optimization
* ⌚ Smartwatch-style enclosure

---

# Screenshots

Add screenshots here:

```text
dashboard1.png
dashboard2.png
dashboard3.png
dashboard4.png
arm_monitor.jpg
```

---

# License

MIT License

---

# Author

Made by Aryan using ESP32, IoT, and modern web technologies.

---

# Acknowledgements

* Espressif ESP32
* Adafruit Libraries
* Supabase
* Node.js
* Open-source IoT community
