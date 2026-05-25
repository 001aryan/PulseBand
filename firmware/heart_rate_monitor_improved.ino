#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "PageIndex.h"

// -----------------------------
// User configuration
// -----------------------------
const char* WIFI_SSID = "YOUR_WIFI_NAME";
const char* WIFI_PASS = "YOUR_WIFI_PASSWORD";

constexpr int PULSE_PIN = 34;
constexpr int LED_PIN = 2;
constexpr int BUTTON_PIN = 32;

constexpr int SCREEN_WIDTH = 128;
constexpr int SCREEN_HEIGHT = 64;
constexpr int OLED_RESET = -1;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Sampling and detection tuning.
constexpr uint32_t SAMPLE_INTERVAL_MS = 4;       // ~250 Hz
constexpr uint32_t MIN_IBI_MS = 280;             // ~214 BPM upper limit
constexpr uint32_t MAX_IBI_MS = 1800;            // ~33 BPM lower limit
constexpr int RELEASE_MARGIN = 24;               // Hysteresis to avoid bounce
constexpr int MIN_DYNAMIC_AMPLITUDE = 45;        // Adaptive threshold floor

constexpr uint16_t LOG_CAPACITY = 900;

struct LogEntry {
  uint32_t elapsedMs;
  uint16_t bpm;
  uint16_t filtered;
};

LogEntry bpmLog[LOG_CAPACITY];
uint16_t logCount = 0;

AsyncWebServer server(80);
AsyncEventSource events("/events");

bool measuring = false;
bool pulseHigh = false;
bool buttonPrev = HIGH;

uint32_t bootMs = 0;
uint32_t lastSampleMs = 0;
uint32_t lastBeatMs = 0;
uint32_t lastPushMs = 0;
uint32_t lastLogMs = 0;

int rawSignal = 0;
int filteredSignal = 0;
int threshold = 1800;
int dynamicMin = 4095;
int dynamicMax = 0;

uint16_t currentBpm = 0;
uint16_t avgBpm = 0;

constexpr uint8_t BPM_SMOOTH_WINDOW = 8;
uint16_t bpmWindow[BPM_SMOOTH_WINDOW] = {0};
uint8_t bpmWindowIndex = 0;
uint8_t bpmWindowCount = 0;

String csvCache;

void updateOled() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print("PulseBand ESP32");

  display.setCursor(0, 14);
  display.print("WiFi: ");
  display.print(WiFi.isConnected() ? "OK" : "OFF");

  display.setCursor(0, 26);
  display.print("State: ");
  display.print(measuring ? "MEASURING" : "IDLE");

  display.setTextSize(2);
  display.setCursor(0, 42);
  display.print(currentBpm);
  display.setTextSize(1);
  display.print(" BPM");

  display.display();
}

void pushBpmToWindow(uint16_t value) {
  bpmWindow[bpmWindowIndex] = value;
  bpmWindowIndex = (bpmWindowIndex + 1) % BPM_SMOOTH_WINDOW;
  if (bpmWindowCount < BPM_SMOOTH_WINDOW) {
    bpmWindowCount++;
  }

  uint32_t sum = 0;
  for (uint8_t i = 0; i < bpmWindowCount; i++) {
    sum += bpmWindow[i];
  }
  avgBpm = bpmWindowCount > 0 ? static_cast<uint16_t>(sum / bpmWindowCount) : 0;
}

void clearBpmWindow() {
  memset(bpmWindow, 0, sizeof(bpmWindow));
  bpmWindowIndex = 0;
  bpmWindowCount = 0;
  avgBpm = 0;
}

void appendLog() {
  if (logCount >= LOG_CAPACITY) {
    return;
  }

  bpmLog[logCount].elapsedMs = millis() - bootMs;
  bpmLog[logCount].bpm = currentBpm;
  bpmLog[logCount].filtered = static_cast<uint16_t>(filteredSignal);
  logCount++;
}

String buildCsv() {
  String out = "ElapsedMs,BPM,FilteredSignal\n";
  out.reserve(26UL * (logCount + 1));

  for (uint16_t i = 0; i < logCount; i++) {
    out += String(bpmLog[i].elapsedMs);
    out += ",";
    out += String(bpmLog[i].bpm);
    out += ",";
    out += String(bpmLog[i].filtered);
    out += "\n";
  }
  return out;
}

String buildStateJson() {
  StaticJsonDocument<512> doc;

  doc["uptimeMs"] = millis() - bootMs;

  JsonObject wifi = doc.createNestedObject("wifi");
  wifi["ssid"] = WiFi.SSID();
  wifi["ip"] = WiFi.localIP().toString();
  wifi["rssi"] = WiFi.RSSI();

  JsonObject sensor = doc.createNestedObject("sensor");
  sensor["raw"] = rawSignal;
  sensor["filtered"] = filteredSignal;
  sensor["threshold"] = threshold;

  JsonObject bpm = doc.createNestedObject("bpm");
  bpm["current"] = currentBpm;
  bpm["average"] = avgBpm;

  JsonObject state = doc.createNestedObject("state");
  state["measuring"] = measuring;
  state["samplesLogged"] = logCount;

  String payload;
  serializeJson(doc, payload);
  return payload;
}

void sendEventState() {
  String payload = buildStateJson();
  events.send(payload.c_str(), "message", millis());
}

void resetMeasurement() {
  currentBpm = 0;
  filteredSignal = 0;
  rawSignal = 0;
  dynamicMin = 4095;
  dynamicMax = 0;
  threshold = 1800;
  pulseHigh = false;
  lastBeatMs = 0;
  clearBpmWindow();
  logCount = 0;
}

void processButton() {
  const bool buttonNow = digitalRead(BUTTON_PIN);
  if (buttonPrev == HIGH && buttonNow == LOW) {
    measuring = !measuring;
    digitalWrite(LED_PIN, measuring ? HIGH : LOW);
    if (!measuring) {
      pulseHigh = false;
    }
  }
  buttonPrev = buttonNow;
}

void sampleSensor() {
  rawSignal = analogRead(PULSE_PIN);

  // Single-pole IIR low-pass filter for stable peak extraction.
  filteredSignal = static_cast<int>(0.8f * filteredSignal + 0.2f * rawSignal);

  // Track dynamic range with slow decay so threshold adapts with motion/noise.
  if (filteredSignal < dynamicMin) {
    dynamicMin = filteredSignal;
  } else {
    dynamicMin = min(dynamicMin + 1, 4095);
  }

  if (filteredSignal > dynamicMax) {
    dynamicMax = filteredSignal;
  } else {
    dynamicMax = max(dynamicMax - 1, 0);
  }

  const int amplitude = max(MIN_DYNAMIC_AMPLITUDE, dynamicMax - dynamicMin);
  threshold = dynamicMin + static_cast<int>(amplitude * 0.58f);

  if (!measuring) {
    return;
  }

  const uint32_t now = millis();
  const bool crossing = filteredSignal > threshold;

  if (!pulseHigh && crossing && (now - lastBeatMs > MIN_IBI_MS)) {
    pulseHigh = true;

    if (lastBeatMs != 0) {
      const uint32_t ibi = now - lastBeatMs;
      if (ibi >= MIN_IBI_MS && ibi <= MAX_IBI_MS) {
        currentBpm = static_cast<uint16_t>(60000UL / ibi);
        pushBpmToWindow(currentBpm);
      }
    }

    lastBeatMs = now;
  }

  if (pulseHigh && filteredSignal < (threshold - RELEASE_MARGIN)) {
    pulseHigh = false;
  }
}

void setupRoutes() {
  server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send_P(200, "text/html", PAGE_INDEX);
  });

  server.on("/api/state", HTTP_GET, [](AsyncWebServerRequest* request) {
    String payload = buildStateJson();
    request->send(200, "application/json", payload);
  });

  server.on("/api/measure", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (request->hasParam("enabled")) {
      const String value = request->getParam("enabled")->value();
      measuring = (value == "1" || value == "true" || value == "on");
      digitalWrite(LED_PIN, measuring ? HIGH : LOW);
    }

    if (!measuring) {
      pulseHigh = false;
    }

    request->send(200, "application/json", "{\"ok\":true}");
  });

  server.on("/api/reset", HTTP_GET, [](AsyncWebServerRequest* request) {
    resetMeasurement();
    request->send(200, "application/json", "{\"ok\":true}");
  });

  server.on("/api/log.csv", HTTP_GET, [](AsyncWebServerRequest* request) {
    csvCache = buildCsv();

    AsyncWebServerResponse* response = request->beginResponse(200, "text/csv", csvCache);
    response->addHeader("Content-Disposition", "attachment; filename=PulseBandLog.csv");
    request->send(response);
  });

  events.onConnect([](AsyncEventSourceClient* client) {
    if (client->lastId()) {
      Serial.printf("Client reconnected. Last event ID: %u\n", client->lastId());
    }
    const String payload = buildStateJson();
    client->send(payload.c_str(), "message", millis());
  });

  server.addHandler(&events);
  server.begin();
}

void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  Serial.print("Connecting to WiFi");
  uint8_t retries = 0;

  while (WiFi.status() != WL_CONNECTED && retries < 50) {
    delay(250);
    Serial.print(".");
    retries++;
  }

  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("Connected. IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("WiFi not connected. Dashboard will be unavailable.");
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(PULSE_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  digitalWrite(LED_PIN, LOW);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("SSD1306 init failed.");
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("PulseBand Booting...");
  display.display();

  bootMs = millis();

  connectWiFi();
  setupRoutes();

  updateOled();
}

void loop() {
  processButton();

  const uint32_t now = millis();

  if (now - lastSampleMs >= SAMPLE_INTERVAL_MS) {
    lastSampleMs = now;
    sampleSensor();
  }

  if (measuring && now - lastLogMs >= 1000) {
    lastLogMs = now;
    appendLog();
  }

  if (now - lastPushMs >= 1000) {
    lastPushMs = now;
    sendEventState();
    updateOled();
  }
}
