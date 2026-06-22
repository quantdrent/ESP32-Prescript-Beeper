#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <esp_system.h>
#include <math.h>

namespace {

constexpr uint8_t kScreenWidth = 128;
constexpr uint8_t kScreenHeight = 64;
constexpr int8_t kOledReset = -1;
constexpr uint8_t kOledAddress = 0x3C;

constexpr uint8_t kSdaPin = 21;
constexpr uint8_t kSclPin = 22;
constexpr uint8_t kPassButtonPin = 18;
constexpr uint8_t kFailButtonPin = 19;

constexpr uint32_t kScrambleSpeedMs = 50;
constexpr uint32_t kScrambleDurationMs = 500;
constexpr uint32_t kRevealDurationMs = 1500;
constexpr uint32_t kShortScrambleDurationMs = 300;
constexpr uint32_t kShortRevealDurationMs = 800;
constexpr uint32_t kDebounceMs = 25;

constexpr size_t kRecentMessagesSize = 5;
constexpr size_t kBodyLineWidth = 21;
constexpr size_t kBodyMaxLines = 5;

constexpr char kScrambleChars[] =
    "ABCDEF@HIJ_LM%OPQR^WX#YZa#b+cdefgh*iqrxyz0123456789";

struct PrescriptMessage {
  const char* text;
  uint16_t weight;
  uint8_t minCount;
  int8_t remaining;
};

PrescriptMessage gMessages[] = {
    {"HAVE ALL OF YOUR SINNERS TARGET AN ENEMY WITH THE LOWEST HEALTH", 40, 0, 2},
    {"SINNERS WITH THE HIGHEST SPEED, TARGET THE SLOWEST ENEMY SLOT", 45, 0, 2},
    {"SINNERS WHO ROLLED EVEN-NUMBERED SPEED,CAN ONLY DEAL DAMAGE TO ODD-NUMBERED SLOTS", 35, 0, 2},
    {"SINNERS WHO ROLLED ODD-NUMBERED SPEED,CANNOT DEAL DAMAGE TO EVEN-NUMBERED SLOTS", 35, 0, 2},
    {"DON'T LET A SINNER TAKE MORE THAN 20 DAMAGE THIS TURN", 40, 0, 1},
    {"HAVE YOUR FASTEST SINNERS CLASH WITH THE SLOWEST ENEMY SLOT", 40, 0, 2},
    {"HAVE ALL OF YOUR SINNERS TARGET A SINGLE SLOT", 45, 0, 3},
    {"USE WINRATE COMMAND THIS TURN", 45, 0, 2},
    {"USE DAMAGE COMMAND THIS TURN", 45, 0, 2},
    {"ACTIVATE A ABSOLUTE RESONANCE THIS TURN", 40, 0, 3},
    {"DON'T USE ANY SKILL_3 THIS TURN", 45, 0, 3},
    {"HAVE A SINNER TAKE DAMAGE THIS TURN", 20, 0, 3},
    {"USE AN AOE EGO AND HAVE IT ONLY DAMAGE ONE ENEMY OR ONE PART", 10, 4, 2},
    {"INFLICT FIVE DIFFERENT DEBUFFS NEXT TURN", 20, 3, 3},
    {"HAVE ALL OF YOUR SINNERS USE A DEFENSE SKILL", 20, 1, 4},
    {"ACTIVATE A RESONANCE PASSIVE THIS TURN", 10, 1, 3},
    {"DEAL AN OVERALL 100 DAMAGE THIS TURN", 30, 2, 2},
    {"USE AN E.G.O SKILL THIS TURN", 10, 2, 5},
    {"DON'T CLASH WITH ANY OF YOUR SINNERS", 10, 2, 5},
    {"DONT'T KILL/STAGGER ANY ENEMIES THIS TURN", 10, 2, 5},
    {"REPEAT THE LAST PRESCRIPT", 20, 3, 5},
    {"ATTACK WITH SINNERS WITH SPEED TOTALING NO GREATER THAN 14", 20, 3, 5},
    {"STAGGER, PART_BREAK, OR DEAL 500 DAMAGE TO AN ENEMY THIS TURN", 10, 4, 5},
    {"SINNERS WITH SPEED LOWER OR EQUAL TO \"E\", USE AN EGO SKILL", 20, 5, 5},
    {"CHAIN ATLEAST 3 SKILL SLOTS WITH THE SAME SIN-AFFINITY", 10, 5, 5},
    {"USE AN EGO THAT BUFFS YOUR ENTIRE TEAM THIS OR NEXT TURN", 5, 7, 1},
    {"GENERATE FOUR UNIQUE SIN RESOURCES THIS TURN", 20, 6, 3},
    {"CHANGE 1 SINNERS RESISTANCE TYPE TO BE FATAL WITH ONE OF THE ENEMIES SKILLS", 5, 8, 3},
    {"HAVE A SINNER CORRODE NEXT TURN", 10, 6, 3},
    {"HAVE A SINNER DIE NEXT TURN", 5, 9, 3},
    {"DON'T USE MORE THAN 2 SKILL_1 THIS TURN", 20, 6, 3},
    {"USE AN OVERCLOCK E.G.O THIS TURN AND HAVE THAT SINNER CORRODE THIS OR NEXT TURN", 5, 6, 3},
    {"DEAL AN OVERALL 300 DAMAGE THIS TURN", 30, 5, 3},
    {"HAVE NO MORE THAN 200 SANITY IN-TOTAL NEXT TURN", 10, 7, 3},
    {"HAVE A TOTAL OF 20 STATUS EFFECT [BUFF/DEBUFFS] ON YOUR SINNERS NEXT TURN", 10, 7, 3},
    {"ACTIVATE AN EGO THAT WOULD DIRECTLY/INDIRECTLY KILL AN ALLY", 5, 8, 10},
    {"USE TEN OR MORE SIN RESOURCES THIS TURN", 20, 8, 10},
    {"USE AN OVERCLOCK E.G.O THIS TURN", 5, 3, 10},
    {"PAT YOUR SLOWEST SINNERS ON THE HEAD FOR 3 SECOND EACH", 5, 3, 1},
    {"Uhm... Can you buy me Ice Cream? ~ Sora", 5, 50, 1},
};

constexpr const char* kIntroMessages[] = {
    "Uhm.. Hello, are you here to Play? ~Sora",
    "Welcome back ~ R13n",
    "Testing.... Ahh wrong prescript. ~ Moirai",
};

enum class PagerState : uint8_t {
  BootIntro,
  ReadyForNext,
  Scrambling,
  AwaitingResolution,
};

struct Button {
  uint8_t pin = 0;
  bool stableState = HIGH;
  bool lastReading = HIGH;
  uint32_t lastChangeMs = 0;

  void begin(uint8_t buttonPin) {
    pin = buttonPin;
    pinMode(pin, INPUT_PULLUP);
    stableState = digitalRead(pin);
    lastReading = stableState;
    lastChangeMs = millis();
  }

  bool pressed() {
    const uint32_t now = millis();
    const bool reading = digitalRead(pin);

    if (reading != lastReading) {
      lastReading = reading;
      lastChangeMs = now;
    }

    if ((now - lastChangeMs) >= kDebounceMs && reading != stableState) {
      stableState = reading;
      return stableState == LOW;
    }

    return false;
  }
};

struct ScrambleAnimation {
  String targetText;
  uint32_t scrambleDurationMs = 0;
  uint32_t revealDurationMs = 0;
  uint32_t startMs = 0;
  uint32_t lastTickMs = 0;
  bool active = false;
  PagerState nextState = PagerState::ReadyForNext;
};

Adafruit_SSD1306 gDisplay(kScreenWidth, kScreenHeight, &Wire, kOledReset);
Button gPassButton;
Button gFailButton;
ScrambleAnimation gAnimation;

PagerState gState = PagerState::BootIntro;
PagerState gLastRenderedState = PagerState::BootIntro;
String gDisplayText;
String gLastRenderedText;
int gAchieved = 0;
int gFailed = 0;
int gTotal = 0;
int gLastRenderedAchieved = -1;
int gLastRenderedFailed = -1;
int gLastRenderedTotal = -1;
int gClickCount = 0;
int gLastMessageIndex = -1;
int gRecentMessages[kRecentMessagesSize] = {-1, -1, -1, -1, -1};
size_t gRecentCount = 0;
bool gDisplayDirty = true;

char randomChar() {
  const size_t poolSize = strlen(kScrambleChars);
  return kScrambleChars[random(poolSize)];
}

String framedText(const String& text) {
  return "_" + text + "_";
}

void markDisplayDirty() {
  gDisplayDirty = true;
}

bool isRecentMessage(int index) {
  for (size_t i = 0; i < gRecentCount; ++i) {
    if (gRecentMessages[i] == index) {
      return true;
    }
  }
  return false;
}

void pushRecentMessage(int index) {
  if (gRecentCount < kRecentMessagesSize) {
    gRecentMessages[gRecentCount++] = index;
    return;
  }

  for (size_t i = 1; i < kRecentMessagesSize; ++i) {
    gRecentMessages[i - 1] = gRecentMessages[i];
  }
  gRecentMessages[kRecentMessagesSize - 1] = index;
}

int pickMessageIndex() {
  const int messageCount = static_cast<int>(sizeof(gMessages) / sizeof(gMessages[0]));
  const int maxAttempts = 25;

  bool hasEligible = false;
  for (int i = 0; i < messageCount; ++i) {
    if (gClickCount >= gMessages[i].minCount && gMessages[i].remaining > 0) {
      hasEligible = true;
      break;
    }
  }

  if (!hasEligible) {
    return -1;
  }

  for (int attempt = 0; attempt < maxAttempts; ++attempt) {
    int totalWeight = 0;
    for (int i = 0; i < messageCount; ++i) {
      if (gClickCount >= gMessages[i].minCount && gMessages[i].remaining > 0) {
        totalWeight += gMessages[i].weight;
      }
    }

    if (totalWeight <= 0) {
      return -1;
    }

    int roll = random(totalWeight);
    int pickedIndex = -1;

    for (int i = 0; i < messageCount; ++i) {
      if (gClickCount < gMessages[i].minCount || gMessages[i].remaining <= 0) {
        continue;
      }

      roll -= gMessages[i].weight;
      if (roll < 0) {
        pickedIndex = i;
        break;
      }
    }

    if (pickedIndex == -1) {
      continue;
    }

    if (pickedIndex != gLastMessageIndex && !isRecentMessage(pickedIndex)) {
      gMessages[pickedIndex].remaining--;
      pushRecentMessage(pickedIndex);
      gLastMessageIndex = pickedIndex;
      return pickedIndex;
    }
  }

  return -1;
}

void startScramble(const String& text, uint32_t scrambleDurationMs,
                   uint32_t revealDurationMs, PagerState nextState) {
  gAnimation.targetText = text;
  gAnimation.scrambleDurationMs = scrambleDurationMs;
  gAnimation.revealDurationMs = revealDurationMs;
  gAnimation.startMs = millis();
  gAnimation.lastTickMs = 0;
  gAnimation.active = true;
  gAnimation.nextState = nextState;
  gState = PagerState::Scrambling;
}

String makeScrambleFrame(const String& target, size_t revealCount, bool revealPhase) {
  String out;
  out.reserve(target.length() + 2);

  for (size_t i = 0; i < target.length(); ++i) {
    if (revealPhase && i < revealCount) {
      out += target[i];
    } else {
      out += randomChar();
    }
  }

  return framedText(out);
}

void updateScrambleAnimation() {
  if (!gAnimation.active) {
    return;
  }

  const uint32_t now = millis();
  if (gAnimation.lastTickMs != 0 && (now - gAnimation.lastTickMs) < kScrambleSpeedMs) {
    return;
  }
  gAnimation.lastTickMs = now;

  const uint32_t elapsed = now - gAnimation.startMs;

  if (elapsed < gAnimation.scrambleDurationMs) {
    gDisplayText = makeScrambleFrame(gAnimation.targetText, 0, false);
    markDisplayDirty();
    return;
  }

  const uint32_t revealElapsed = elapsed - gAnimation.scrambleDurationMs;
  const float progress =
      gAnimation.revealDurationMs == 0
          ? 1.0f
          : min(1.0f, static_cast<float>(revealElapsed) /
                           static_cast<float>(gAnimation.revealDurationMs));
  const size_t revealCount =
      static_cast<size_t>(floor(progress * static_cast<float>(gAnimation.targetText.length())));

  gDisplayText = makeScrambleFrame(gAnimation.targetText, revealCount, true);
  markDisplayDirty();

  if (progress >= 1.0f) {
    gDisplayText = framedText(gAnimation.targetText);
    gAnimation.active = false;
    gState = gAnimation.nextState;
    markDisplayDirty();
  }
}

void drawWrappedBody(const String& text) {
  size_t start = 0;
  for (size_t line = 0; line < kBodyMaxLines; ++line) {
    if (start >= text.length()) {
      break;
    }

    const size_t end = min(start + kBodyLineWidth, text.length());
    gDisplay.setCursor(0, 12 + (line * 9));
    gDisplay.print(text.substring(start, end));
    start = end;
  }
}

void renderDisplay() {
  const bool needsRender =
      gDisplayDirty ||
      gState != gLastRenderedState ||
      gDisplayText != gLastRenderedText ||
      gAchieved != gLastRenderedAchieved ||
      gFailed != gLastRenderedFailed ||
      gTotal != gLastRenderedTotal;

  if (!needsRender) {
    return;
  }

  gDisplay.clearDisplay();
  gDisplay.setTextColor(SSD1306_WHITE);
  gDisplay.setTextSize(1);

  char header[20];
  snprintf(header, sizeof(header), "P:%d F:%d T:%d", gAchieved, gFailed, gTotal);
  gDisplay.setCursor(0, 0);
  gDisplay.print(header);
  gDisplay.drawLine(0, 9, 127, 9, SSD1306_WHITE);

  drawWrappedBody(gDisplayText);

  gDisplay.setCursor(0, 56);
  switch (gState) {
    case PagerState::ReadyForNext:
      gDisplay.print("PASS=NEXT");
      break;
    case PagerState::AwaitingResolution:
      gDisplay.print("PASS/FAIL");
      break;
    case PagerState::BootIntro:
    case PagerState::Scrambling:
      gDisplay.print("SCRAMBLING");
      break;
  }

  gDisplay.display();

  gLastRenderedState = gState;
  gLastRenderedText = gDisplayText;
  gLastRenderedAchieved = gAchieved;
  gLastRenderedFailed = gFailed;
  gLastRenderedTotal = gTotal;
  gDisplayDirty = false;
}

void beginIntro() {
  const size_t introCount = sizeof(kIntroMessages) / sizeof(kIntroMessages[0]);
  const String intro = kIntroMessages[random(introCount)];
  startScramble(intro, kShortScrambleDurationMs, kShortRevealDurationMs,
                PagerState::ReadyForNext);
}

void beginPrescript() {
  gClickCount++;
  const int pickedIndex = pickMessageIndex();
  if (pickedIndex < 0) {
    gDisplayText = "NO PRESCRIPTS LEFT";
    gState = PagerState::ReadyForNext;
    markDisplayDirty();
    return;
  }

  startScramble(gMessages[pickedIndex].text, kScrambleDurationMs, kRevealDurationMs,
                PagerState::AwaitingResolution);
}

void markPass() {
  gAchieved++;
  gTotal++;
  startScramble("CleAr", kShortScrambleDurationMs, kShortRevealDurationMs,
                PagerState::ReadyForNext);
}

void markFail() {
  gFailed++;
  gTotal++;
  startScramble("FaIL", kShortScrambleDurationMs, kShortRevealDurationMs,
                PagerState::ReadyForNext);
}

}  // namespace

void setup() {
  Serial.begin(115200);
  randomSeed(static_cast<uint32_t>(esp_random()));

  gPassButton.begin(kPassButtonPin);
  gFailButton.begin(kFailButtonPin);

  Wire.begin(kSdaPin, kSclPin);
  if (!gDisplay.begin(SSD1306_SWITCHCAPVCC, kOledAddress)) {
    for (;;) {
      delay(100);
    }
  }

  gDisplay.clearDisplay();
  gDisplay.display();
  markDisplayDirty();

  beginIntro();
}

void loop() {
  const bool passPressed = gPassButton.pressed();
  const bool failPressed = gFailButton.pressed();

  if (gState == PagerState::ReadyForNext && passPressed) {
    beginPrescript();
  } else if (gState == PagerState::AwaitingResolution) {
    if (passPressed) {
      markPass();
    } else if (failPressed) {
      markFail();
    }
  }

  updateScrambleAnimation();
  renderDisplay();
}
