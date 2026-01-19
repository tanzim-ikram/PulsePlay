#include <WiFi.h>
#include <WiFiManager.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include <time.h>
#include <Adafruit_NeoPixel.h>

// ---------------- PIN DEFINITIONS ----------------
#define OLED_SDA 8
#define OLED_SCL 9

#define NEOPIXEL_PIN 3       // your LED pin now drives the NeoPixel data
#define NEOPIXEL_COUNT 1

#define CLK 2
#define DT 1
#define SW 0

#define BUZZER_PIN 4         // CHANGE this to the pin you wired the piezo to

// ---------------- OLED ----------------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ---------------- NeoPixel ----------------
Adafruit_NeoPixel pixel(NEOPIXEL_COUNT, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

// ---------------- Mood Data ----------------
const int MOOD_COUNT = 10;
const char* moods[MOOD_COUNT] = {
  "Energetic","Calm","Focus","Happy","Sad",
  "Romantic","Angry","Nostalgic","Adventurous","Sleepy"
};

// Simple preview tags (edit as you like)
const char* moodHints[MOOD_COUNT] = {
  "Upbeat - Fast",
  "Soft - Slow",
  "Minimal - Deep",
  "Bright - Fun",
  "Low - Warm",
  "Sweet - Close",
  "Loud - Punchy",
  "Old - Gold",
  "New - Wild",
  "Chill - Drift"
};

int currentIndex = 1;

// ---------------- Backend ----------------
const char* BACKEND_URL = "https://pulseplay.up.railway.app/api/recommend";

// ---------------- Encoder State ----------------
int lastCLK;
unsigned long lastEncoderChange = 0;
const unsigned long ENCODER_DEBOUNCE = 2;  // tighter, we do accel ourselves

// ---------------- Button Gesture State ----------------
bool lastBtnReading = HIGH;
bool btnStableState = HIGH;
unsigned long lastBtnDebounceTime = 0;
const unsigned long BTN_DEBOUNCE_MS = 25;

unsigned long pressStartTime = 0;
bool longPressFired = false;

unsigned long lastReleaseTime = 0;
int clickCount = 0;

const unsigned long LONG_PRESS_MS = 700;
const unsigned long DOUBLE_CLICK_GAP_MS = 300;

// ---------------- Interaction / UI State ----------------
enum UiState { UI_MENU, UI_PREVIEW, UI_SENDING };
UiState uiState = UI_MENU;

unsigned long previewStart = 0;
const unsigned long PREVIEW_TIMEOUT_MS = 1400;

// ---------------- WiFi Manager ----------------
WiFiManager wm;

// ---------------- Buzzer (simple + reliable) ----------------
void buzzerInit() {
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
}

void beep(int freqHz, int ms) {
  tone(BUZZER_PIN, freqHz);
  delay(ms);
  noTone(BUZZER_PIN);
}

void clickTick() {
  tone(BUZZER_PIN, 3000);
  delay(10);
  noTone(BUZZER_PIN);
}

void confirmTone() {
  beep(1200, 80);
  delay(20);
  beep(1600, 60);
}

void errorTone() {
  for (int i = 0; i < 3; i++) {
    beep(220, 90);
    delay(60);
  }
}

// ---------------- NeoPixel helpers ----------------
uint8_t clamp8(int v) { return (v < 0) ? 0 : (v > 255 ? 255 : (uint8_t)v); }

void pixelSetRGB(uint8_t r, uint8_t g, uint8_t b) {
  pixel.setPixelColor(0, pixel.Color(r, g, b));
  pixel.show();
}

void pixelOff() {
  pixelSetRGB(0, 0, 0);
}

// breathing green when idle (connected)
void pixelBreathGreen(unsigned long t) {
  // triangle wave 0..255..0 over ~2.2s
  const unsigned long period = 2200;
  unsigned long x = t % period;
  int v = (x < period / 2) ? (int)(x * 255 / (period / 2)) : (int)((period - x) * 255 / (period / 2));
  // keep it soft
  v = 20 + (v * 80 / 255); // 20..100
  pixelSetRGB(0, (uint8_t)v, 0);
}

// blink red when not connected
void pixelBlinkRed(unsigned long t) {
  bool on = ((t / 350) % 2) == 0;
  pixelSetRGB(on ? 80 : 0, 0, 0);
}

// fade during sending (green pulse)
void pixelSendingFade(unsigned long t) {
  const unsigned long period = 900;
  unsigned long x = t % period;
  int v = (x < period / 2) ? (int)(x * 255 / (period / 2)) : (int)((period - x) * 255 / (period / 2));
  v = 10 + (v * 120 / 255); // 10..130
  pixelSetRGB(0, (uint8_t)v, 0);
}

// rapid triple blink red for error
void pixelErrorTripleBlink() {
  for (int i = 0; i < 3; i++) {
    pixelSetRGB(120, 0, 0);
    delay(80);
    pixelOff();
    delay(80);
  }
}

// ---------------- UI Functions ----------------
void drawBootScreen() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 22);
  display.println("PulsePlay");
  display.display();
}

void drawIntroScreen() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 8);
  display.println("Select mood,");
  display.println("Get music from AI");
  display.println("");
  display.println("Open web app: pulse");
  display.println("play.up.railway.app");
  display.display();
}

void drawMoodMenu() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  int start;
  int arrowRow;

  if (currentIndex == 0) { start = 0; arrowRow = 0; }
  else if (currentIndex == MOOD_COUNT - 1) {
    start = MOOD_COUNT - 3;
    if (start < 0) start = 0;
    arrowRow = min(2, MOOD_COUNT - 1);
  } else {
    start = currentIndex - 1;
    arrowRow = 1;
  }

  for (int i = 0; i < 3; i++) {
    int idx = start + i;
    if (idx >= 0 && idx < MOOD_COUNT) {
      display.setCursor(14, 16 + i * 16);
      display.print(moods[idx]);
    }
  }

  display.setCursor(0, 16 + arrowRow * 16);
  display.print("->");

  display.display();
}

void drawPreviewScreen() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(0, 10);
  display.print("Mood:");
  display.setCursor(40, 10);
  display.print(moods[currentIndex]);

  display.setCursor(0, 30);
  display.print(moodHints[currentIndex]);

  display.setCursor(0, 50);
  display.print("Hold=preview  Press=send");
  display.display();
}

void drawSendingScreen(const char* mood) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 24);
  display.print("Sending:");
  display.setCursor(10, 40);
  display.print(mood);
  display.display();
}

void drawResultScreen(const char* msg) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(20, 30);
  display.print(msg);
  display.display();
}

// ---------------- Time Sync (IMPORTANT for HTTPS) ----------------
bool syncTime() {
  configTime(6 * 3600, 0, "pool.ntp.org", "time.nist.gov", "time.google.com");

  time_t now = time(nullptr);
  unsigned long start = millis();
  while (now < 1700000000 && (millis() - start) < 10000) {
    delay(200);
    now = time(nullptr);
  }
  return (now >= 1700000000);
}

// ---------------- Backend Send (HTTPS) ----------------
bool sendMood(const char* mood) {
  if (WiFi.status() != WL_CONNECTED) return false;

  WiFiClientSecure client;
  client.setInsecure();
  client.setTimeout(15000);

  HTTPClient http;
  http.setTimeout(15000);
  http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);

  if (!http.begin(client, BACKEND_URL)) {
    http.end();
    return false;
  }

  http.addHeader("Content-Type", "application/json");
  String payload = "{\"mood\":\"" + String(mood) + "\"}";
  int httpCode = http.POST(payload);

  // Debug
  Serial.print("HTTP code: ");
  Serial.println(httpCode);
  if (httpCode >= 200 && httpCode < 300) {
    Serial.print("Response: ");
    Serial.println(http.getString());
    http.end();
    return true;
  } else {
    if (httpCode < 0) {
      Serial.print("HTTP error: ");
      Serial.println(http.errorToString(httpCode));
    } else {
      Serial.print("Response: ");
      Serial.println(http.getString());
    }
    http.end();
    return false;
  }
}

// ---------------- Encoder accel / inertia ----------------
int accelStepFromDelta(unsigned long dtMs) {
  // Fast rotation = bigger jump
  if (dtMs <= 60)  return 3;
  if (dtMs <= 120) return 2;
  return 1;
}

void applyIndexDelta(int delta) {
  currentIndex += delta;
  if (currentIndex < 0) currentIndex = 0;
  if (currentIndex >= MOOD_COUNT) currentIndex = MOOD_COUNT - 1;
}

// ---------------- Button gesture handling ----------------
void handleShortPress() {
  // Short press = send mood
  uiState = UI_SENDING;
  drawSendingScreen(moods[currentIndex]);

  // sending LED animation for ~300ms before network call (so user sees it)
  unsigned long start = millis();
  while (millis() - start < 300) {
    pixelSendingFade(millis());
    delay(10);
  }

  bool ok = sendMood(moods[currentIndex]);

  if (ok) {
    confirmTone();
    drawResultScreen("Sent!");
    delay(700);
  } else {
    errorTone();
    pixelErrorTripleBlink();
    drawResultScreen("Error!");
    delay(900);
  }

  uiState = UI_MENU;
  drawMoodMenu();
}

void handleLongPress() {
  // Long press = preview
  uiState = UI_PREVIEW;
  previewStart = millis();
  drawPreviewScreen();
}

void handleDoublePress() {
  // Double press = surprise me
  int old = currentIndex;
  if (MOOD_COUNT > 1) {
    do { currentIndex = random(0, MOOD_COUNT); } while (currentIndex == old);
  }
  clickTick();
  uiState = UI_MENU;
  drawMoodMenu();
}

// Debounced read + gesture recognizer
void updateButtonGestures() {
  bool reading = digitalRead(SW);

  if (reading != lastBtnReading) {
    lastBtnDebounceTime = millis();
    lastBtnReading = reading;
  }

  if ((millis() - lastBtnDebounceTime) > BTN_DEBOUNCE_MS) {
    if (reading != btnStableState) {
      btnStableState = reading;

      // Button pressed
      if (btnStableState == LOW) {
        pressStartTime = millis();
        longPressFired = false;
      }

      // Button released
      if (btnStableState == HIGH) {
        unsigned long pressDur = millis() - pressStartTime;

        if (!longPressFired) {
          // Count clicks for single/double
          clickCount++;
          lastReleaseTime = millis();
        }
      }
    }
  }

  // Long press fire while holding
  if (btnStableState == LOW && !longPressFired) {
    if (millis() - pressStartTime >= LONG_PRESS_MS) {
      longPressFired = true;
      clickCount = 0; // cancel click counting
      handleLongPress();
    }
  }

  // Resolve single vs double click after gap
  if (clickCount > 0 && (millis() - lastReleaseTime) > DOUBLE_CLICK_GAP_MS) {
    if (clickCount == 1) handleShortPress();
    else handleDoublePress();
    clickCount = 0;
  }
}

// ---------------- SETUP ----------------
void setup() {
  Serial.begin(115200);

  pinMode(CLK, INPUT);
  pinMode(DT, INPUT);
  pinMode(SW, INPUT_PULLUP);

  pinMode(BUZZER_PIN, OUTPUT);
  buzzerInit();

  pixel.begin();
  pixel.setBrightness(255);
  pixelOff();

  Wire.begin(OLED_SDA, OLED_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    while (1) { pixelSetRGB(120, 0, 0); delay(100); pixelOff(); delay(100); }
  }

  randomSeed(esp_random());

  lastCLK = digitalRead(CLK);

  drawBootScreen();
  delay(1200);

  wm.setConfigPortalTimeout(180);

  // While connecting, blink red
  unsigned long connectAnimStart = millis();
  while (WiFi.status() != WL_CONNECTED) {
    // autoConnect blocks internally, so we call it once
    break;
  }

  if (!wm.autoConnect("PulsePlay")) {
    display.clearDisplay();
    display.setCursor(10, 30);
    display.print("WiFi Failed");
    display.display();
    errorTone();
    pixelErrorTripleBlink();
    delay(1500);
    ESP.restart();
  }

  // Connected: static green briefly
  pixelSetRGB(0, 120, 0);

  // Sync time
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 28);
  display.print("Syncing time...");
  display.display();

  bool ok = syncTime();
  Serial.print("Time sync: ");
  Serial.println(ok ? "OK" : "FAILED");

  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0, 28);
  display.print(ok ? "Time OK" : "Time FAIL");
  display.display();
  delay(600);

  drawIntroScreen();
  delay(2000);
  uiState = UI_MENU;
  drawMoodMenu();
}

// ---------------- LOOP ----------------
void loop() {
  unsigned long now = millis();

  // NeoPixel status behaviors
  if (uiState == UI_SENDING) {
    pixelSendingFade(now);
  } else if (WiFi.status() != WL_CONNECTED) {
    pixelBlinkRed(now);
  } else {
    // connected and not sending: breathing green
    pixelBreathGreen(now);
  }

  // Preview timeout
  if (uiState == UI_PREVIEW && (now - previewStart) > PREVIEW_TIMEOUT_MS) {
    uiState = UI_MENU;
    drawMoodMenu();
  }

  // Button gestures (works in all states)
  updateButtonGestures();

  // Encoder only active in menu (or preview — allow changing selection)
  if (uiState == UI_MENU || uiState == UI_PREVIEW) {
    int currentCLK = digitalRead(CLK);

    if (currentCLK != lastCLK && currentCLK == LOW) {
      unsigned long dt = now - lastEncoderChange;
      if (dt > ENCODER_DEBOUNCE) {
        int dir = (digitalRead(DT) != currentCLK) ? +1 : -1;
        int step = accelStepFromDelta(dt);
        applyIndexDelta(dir * step);

        clickTick(); // soft click per tick

        if (uiState == UI_PREVIEW) drawPreviewScreen();
        else drawMoodMenu();

        lastEncoderChange = now;
      }
    }
    lastCLK = currentCLK;
  }
}
