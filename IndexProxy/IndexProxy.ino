#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <Preferences.h>
#include "SplashLogo.h"
#include "Prescripts.h"
#include "WebUi.h"

const int PASS_BUTTON_PIN = 6;
const int FAIL_BUTTON_PIN = 8;

const unsigned long DEFAULT_MESSAGE_MS = 5000;
const unsigned long RESULT_MESSAGE_MS = 2500;
const unsigned long BUTTON_HOLD_MS = 700;
const uint32_t MIN_DURATION_MS = 500;
const uint32_t MAX_DURATION_MS = 60000;
const int MAX_CUSTOM_PRESCRIPTS = 20;

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
WebServer server(80);
HardwareSerial dfSerial(1);
Preferences prefs;

unsigned long oledTimeout = 0;
bool oledActive = false;
bool presetAwaitingDecision = false;
int activePresetIndex = -1;
unsigned long presetDecisionDeadline = 0;
unsigned long bothButtonsStart = 0;
bool comboTriggered = false;
bool lastPassPressed = false;
bool lastFailPressed = false;

String customPrescriptTexts[MAX_CUSTOM_PRESCRIPTS];
uint32_t customPrescriptDurations[MAX_CUSTOM_PRESCRIPTS];
int customPrescriptCount = 0;

const char scrambleChars[] =
"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
"0123456789"
"!@#$%^&*";

char randomChar() {
  int len = strlen(scrambleChars);
  return scrambleChars[random(len)];
}

String normalizeText(String text) {
  text.replace("\r", " ");
  text.replace("\n", " ");
  text.trim();

  while (text.indexOf("  ") >= 0) {
    text.replace("  ", " ");
  }

  return text;
}

uint32_t clampDurationMs(uint32_t durationMs) {
  if (durationMs < MIN_DURATION_MS) return MIN_DURATION_MS;
  if (durationMs > MAX_DURATION_MS) return MAX_DURATION_MS;
  return durationMs;
}

uint32_t parseDurationMsArg(const String &value, uint32_t fallbackMs) {
  if (value.length() == 0) return fallbackMs;

  float seconds = value.toFloat();

  if (seconds <= 0.0f) return fallbackMs;

  return clampDurationMs(static_cast<uint32_t>(seconds * 1000.0f));
}

String makePrefKey(const char *prefix, int index) {
  return String(prefix) + String(index);
}

void saveCustomPrescripts() {
  prefs.putUChar("count", static_cast<uint8_t>(customPrescriptCount));

  for (int i = 0; i < MAX_CUSTOM_PRESCRIPTS; i++) {
    String textKey = makePrefKey("t", i);
    String durationKey = makePrefKey("d", i);

    if (i < customPrescriptCount) {
      prefs.putString(textKey.c_str(), customPrescriptTexts[i]);
      prefs.putUInt(durationKey.c_str(), customPrescriptDurations[i]);
    } else {
      prefs.remove(textKey.c_str());
      prefs.remove(durationKey.c_str());
    }
  }
}

void loadCustomPrescripts() {
  customPrescriptCount = 0;

  int savedCount = prefs.getUChar("count", 0);
  if (savedCount > MAX_CUSTOM_PRESCRIPTS) savedCount = MAX_CUSTOM_PRESCRIPTS;

  for (int i = 0; i < savedCount; i++) {
    String text = normalizeText(prefs.getString(makePrefKey("t", i).c_str(), ""));
    uint32_t durationMs = clampDurationMs(prefs.getUInt(makePrefKey("d", i).c_str(), 3000));

    if (text.length() == 0) continue;

    customPrescriptTexts[customPrescriptCount] = text;
    customPrescriptDurations[customPrescriptCount] = durationMs;
    customPrescriptCount++;
  }

  if (customPrescriptCount != savedCount) {
    saveCustomPrescripts();
  }
}

int getPrescriptCount() {
  return DEFAULT_PRESCRIPT_COUNT + customPrescriptCount;
}

bool getPrescriptByIndex(int index, String &text, uint32_t &durationMs, bool &builtIn) {
  if (index < 0 || index >= getPrescriptCount()) return false;

  if (index < static_cast<int>(DEFAULT_PRESCRIPT_COUNT)) {
    text = DEFAULT_PRESCRIPTS[index].text;
    durationMs = DEFAULT_PRESCRIPTS[index].durationMs;
    builtIn = true;
    return true;
  }

  int customIndex = index - DEFAULT_PRESCRIPT_COUNT;
  text = customPrescriptTexts[customIndex];
  durationMs = customPrescriptDurations[customIndex];
  builtIn = false;
  return true;
}

bool addCustomPrescript(String text, uint32_t durationMs) {
  if (customPrescriptCount >= MAX_CUSTOM_PRESCRIPTS) return false;

  customPrescriptTexts[customPrescriptCount] = text;
  customPrescriptDurations[customPrescriptCount] = durationMs;
  customPrescriptCount++;
  saveCustomPrescripts();
  return true;
}

bool deleteCustomPrescriptByGlobalIndex(int globalIndex) {
  if (globalIndex < static_cast<int>(DEFAULT_PRESCRIPT_COUNT)) return false;

  int customIndex = globalIndex - DEFAULT_PRESCRIPT_COUNT;
  if (customIndex < 0 || customIndex >= customPrescriptCount) return false;

  for (int i = customIndex; i < customPrescriptCount - 1; i++) {
    customPrescriptTexts[i] = customPrescriptTexts[i + 1];
    customPrescriptDurations[i] = customPrescriptDurations[i + 1];
  }

  customPrescriptCount--;
  customPrescriptTexts[customPrescriptCount] = "";
  customPrescriptDurations[customPrescriptCount] = 0;
  saveCustomPrescripts();
  return true;
}

String jsonEscape(const String &value) {
  String escaped;
  escaped.reserve(value.length() + 8);

  for (size_t i = 0; i < value.length(); i++) {
    char c = value[i];

    if (c == '\\' || c == '\"') {
      escaped += '\\';
      escaped += c;
    } else if (c == '\b') {
      escaped += "\\b";
    } else if (c == '\f') {
      escaped += "\\f";
    } else if (c == '\n') {
      escaped += "\\n";
    } else if (c == '\r') {
      escaped += "\\r";
    } else if (c == '\t') {
      escaped += "\\t";
    } else {
      escaped += c;
    }
  }

  return escaped;
}

void drawBitmapSlice(
  int16_t x,
  int16_t y,
  const uint8_t *bitmap,
  uint8_t width,
  uint8_t height,
  int16_t clipLeft,
  int16_t clipRight
) {
  if (clipRight < clipLeft) return;

  const uint8_t bytesPerRow = (width + 7) / 8;

  for (uint8_t row = 0; row < height; row++) {
    for (uint8_t col = 0; col < width; col++) {
      int16_t screenX = x + col;

      if (screenX < clipLeft || screenX > clipRight) continue;

      const uint16_t byteIndex = (row * bytesPerRow) + (col / 8);
      const uint8_t mask = 0x80 >> (col % 8);

      if (pgm_read_byte(bitmap + byteIndex) & mask) {
        display.drawPixel(screenX, y + row, SH110X_WHITE);
      }
    }
  }
}

void drawCenteredText(String text) {
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);
  display.setTextSize(1);

  const int maxCharsPerLine = 21;

  String lines[5];
  int lineCount = 0;

  while (text.length() > 0 && lineCount < 5) {
    if (text.length() <= maxCharsPerLine) {
      lines[lineCount++] = text;
      break;
    }

    int splitPos = maxCharsPerLine;

    while (splitPos > 0 && text[splitPos] != ' ') {
      splitPos--;
    }

    if (splitPos == 0) {
      splitPos = maxCharsPerLine;
    }

    lines[lineCount++] = text.substring(0, splitPos);
    text = text.substring(splitPos);
    text.trim();
  }

  int lineHeight = 10;
  int totalHeight = lineCount * lineHeight;
  int startY = (SCREEN_HEIGHT - totalHeight) / 2;

  for (int i = 0; i < lineCount; i++) {
    int16_t x1, y1;
    uint16_t w, h;

    display.getTextBounds(lines[i], 0, 0, &x1, &y1, &w, &h);

    int x = (SCREEN_WIDTH - w) / 2;
    display.setCursor(x, startY + (i * lineHeight));
    display.print(lines[i]);
  }

  display.display();
}

void showScrambledText(String text, uint32_t holdMs, bool decorate) {
  text = normalizeText(text);
  if (text.length() == 0) return;

  if (decorate) {
    text = "_" + text + "_";
  }

  int len = text.length();

  for (int frame = 0; frame < 25; frame++) {
    String out = "";

    for (int i = 0; i < len; i++) {
      out += randomChar();
    }

    drawCenteredText(out);
    delay(40);
  }

  for (int reveal = 0; reveal <= len; reveal++) {
    String out = "";

    for (int i = 0; i < len; i++) {
      if (i < reveal) {
        out += text[i];
      } else {
        out += randomChar();
      }
    }

    drawCenteredText(out);
    delay(50);
  }

  drawCenteredText(text);

  if (holdMs > 0) {
    oledTimeout = millis() + holdMs;
    oledActive = true;
  } else {
    oledTimeout = 0;
    oledActive = false;
  }
}

void showStartupLogo() {
  const int16_t logoX = (SCREEN_WIDTH - SPLASH_LOGO_WIDTH) / 2;
  const int16_t logoY = (SCREEN_HEIGHT - SPLASH_LOGO_HEIGHT) / 2;
  const uint8_t frames = 42;
  const int16_t logoRight = logoX + SPLASH_LOGO_WIDTH - 1;

  for (uint8_t frame = 0; frame <= frames; frame++) {
    const float t = frame / static_cast<float>(frames);
    const float eased = t * t * t;
    const int16_t revealedWidth = constrain(
      round(eased * SPLASH_LOGO_WIDTH),
      0,
      SPLASH_LOGO_WIDTH
    );
    const int16_t revealLeft = logoX + SPLASH_LOGO_WIDTH - revealedWidth;
    const int16_t lineX = constrain(revealLeft - 1, logoX, logoRight);

    display.clearDisplay();

    drawBitmapSlice(
      logoX,
      logoY,
      splashLogoBitmap,
      SPLASH_LOGO_WIDTH,
      SPLASH_LOGO_HEIGHT,
      revealLeft,
      logoRight
    );

    if (revealedWidth > 0 && revealedWidth < SPLASH_LOGO_WIDTH) {
      display.fillRect(lineX, logoY - 3, 2, SPLASH_LOGO_HEIGHT + 6, SH110X_WHITE);
      display.drawFastVLine(lineX + 2, logoY + 1, SPLASH_LOGO_HEIGHT - 2, SH110X_WHITE);
      display.drawPixel(lineX + 3, logoY + 6, SH110X_WHITE);
      display.drawPixel(lineX + 3, logoY + SPLASH_LOGO_HEIGHT - 7, SH110X_WHITE);

      if (frame % 5 == 0) {
        display.drawPixel(lineX, logoY - 1, SH110X_WHITE);
        display.drawPixel(lineX + 1, logoY + SPLASH_LOGO_HEIGHT, SH110X_WHITE);
      }
    }

    display.display();
    delay(26);
  }

  display.clearDisplay();
  display.drawBitmap(logoX, logoY, splashLogoBitmap, SPLASH_LOGO_WIDTH, SPLASH_LOGO_HEIGHT, SH110X_WHITE);
  display.display();
  delay(950);
}

void showBootStandbyText() {
  showScrambledText("_STANDBY._", 0, false);
}

void showResultText(bool passed) {
  presetAwaitingDecision = false;
  activePresetIndex = -1;
  presetDecisionDeadline = 0;
  showScrambledText(passed ? "CLEAR." : "FAILED.", RESULT_MESSAGE_MS, false);
}

void startDecisionText(String text, uint32_t durationMs, bool decorate) {
  showScrambledText(text, 0, decorate);
  presetAwaitingDecision = true;
  presetDecisionDeadline = millis() + durationMs;
}

void triggerRandomPrescript() {
  int count = getPrescriptCount();
  if (count == 0) return;

  String text;
  uint32_t durationMs;
  bool builtIn = false;

  activePresetIndex = random(count);

  if (!getPrescriptByIndex(activePresetIndex, text, durationMs, builtIn)) {
    activePresetIndex = -1;
    return;
  }

  startDecisionText(text, durationMs, false);
}

void sendApiResponse(int statusCode, const String &body) {
  server.sendHeader("Cache-Control", "no-store");
  server.send(statusCode, "text/plain", body);
}

void handleRoot() {
  server.sendHeader("Cache-Control", "no-store");
  server.send_P(200, "text/html", index_html);
}

void handlePrescripts() {
  String json = "[";

  for (int i = 0; i < getPrescriptCount(); i++) {
    String text;
    uint32_t durationMs;
    bool builtIn = false;

    getPrescriptByIndex(i, text, durationMs, builtIn);

    if (i > 0) json += ",";

    json += "{";
    json += "\"id\":" + String(i) + ",";
    json += "\"text\":\"" + jsonEscape(text) + "\",";
    json += "\"durationMs\":" + String(durationMs) + ",";
    json += "\"builtIn\":" + String(builtIn ? "true" : "false");
    json += "}";
  }

  json += "]";

  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", json);
}

void handleSend() {
  String text = "";
  uint32_t durationMs = DEFAULT_MESSAGE_MS;
  bool fromPreset = false;
  int presetIndex = -1;

  if (server.hasArg("id")) {
    presetIndex = server.arg("id").toInt();
    bool builtIn = false;

    if (!getPrescriptByIndex(presetIndex, text, durationMs, builtIn)) {
      sendApiResponse(404, "Unknown prescript");
      return;
    }

    fromPreset = true;
  } else if (server.hasArg("val")) {
    text = normalizeText(server.arg("val"));
    if (text.length() == 0) {
      sendApiResponse(400, "Missing text");
      return;
    }
  } else {
    sendApiResponse(400, "Missing text");
    return;
  }

  durationMs = parseDurationMsArg(server.arg("duration"), durationMs);

  activePresetIndex = fromPreset ? presetIndex : -1;
  startDecisionText(text, durationMs, !fromPreset);

  sendApiResponse(200, "OK");
}

void handleAddPrescript() {
  if (!server.hasArg("val")) {
    sendApiResponse(400, "Missing text");
    return;
  }

  String text = normalizeText(server.arg("val"));
  if (text.length() == 0) {
    sendApiResponse(400, "Missing text");
    return;
  }

  uint32_t durationMs = parseDurationMsArg(server.arg("duration"), 3000);

  if (!addCustomPrescript(text, durationMs)) {
    sendApiResponse(409, "Prescript storage full");
    return;
  }

  sendApiResponse(200, "Saved");
}

void handleDeletePrescript() {
  if (!server.hasArg("id")) {
    sendApiResponse(400, "Missing id");
    return;
  }

  int index = server.arg("id").toInt();

  if (!deleteCustomPrescriptByGlobalIndex(index)) {
    sendApiResponse(400, "Cannot delete that prescript");
    return;
  }

  sendApiResponse(200, "Deleted");
}

void handleButtons() {
  bool passPressed = digitalRead(PASS_BUTTON_PIN) == LOW;
  bool failPressed = digitalRead(FAIL_BUTTON_PIN) == LOW;

  if (presetAwaitingDecision) {
    if (passPressed && !failPressed && !lastPassPressed) {
      showResultText(true);
    } else if (failPressed && !passPressed && !lastFailPressed) {
      showResultText(false);
    }
  } else if (passPressed && failPressed) {
    if (bothButtonsStart == 0) {
      bothButtonsStart = millis();
    } else if (!comboTriggered && (millis() - bothButtonsStart >= BUTTON_HOLD_MS)) {
      comboTriggered = true;
      triggerRandomPrescript();
    }
  } else {
    bothButtonsStart = 0;

    if (!passPressed && !failPressed) {
      comboTriggered = false;
    }
  }

  lastPassPressed = passPressed;
  lastFailPressed = failPressed;
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Booting...");

  randomSeed(micros());

  pinMode(PASS_BUTTON_PIN, INPUT_PULLUP);
  pinMode(FAIL_BUTTON_PIN, INPUT_PULLUP);

  prefs.begin("prescripts", false);
  loadCustomPrescripts();

  Wire.begin(3, 4);

  if (!display.begin(0x3C, true)) {
    Serial.println("OLED FAILED");
    while (1) {
      delay(100);
    }
  }

  display.setRotation(2);
  display.clearDisplay();
  display.display();

  showStartupLogo();
  showBootStandbyText();

  WiFi.mode(WIFI_AP);
  WiFi.softAP("LarpMachine");

  server.on("/", handleRoot);
  server.on("/send", handleSend);
  server.on("/prescripts", handlePrescripts);
  server.on("/add-prescript", handleAddPrescript);
  server.on("/delete-prescript", handleDeletePrescript);
  server.begin();

  Serial.println("Ready!");
}

void loop() {
  server.handleClient();
  handleButtons();

  if (presetAwaitingDecision && millis() > presetDecisionDeadline) {
    showResultText(false);
    return;
  }

  if (oledActive && millis() > oledTimeout) {
    display.clearDisplay();
    display.display();

    oledActive = false;
    presetAwaitingDecision = false;
    activePresetIndex = -1;
    presetDecisionDeadline = 0;
  }
}
