#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ==========================
// VU Meter para Arduino Nano
// LCD 16x2 via I2C
// ==========================

LiquidCrystal_I2C lcd(0x27, 16, 2); // ajuste para 0x3F se necessário

const uint8_t MIC_PIN = A0;
const uint8_t BTN_PROFILE_PIN = 6;      // botão para trocar perfil
const uint8_t BTN_SENSITIVITY_PIN = 7;  // botão para trocar sensibilidade

const uint8_t BAR_WIDTH = 16;
const uint16_t REFRESH_MS = 60;
const uint16_t UI_MESSAGE_MS = 700;
const uint16_t DEBOUNCE_MS = 40;

struct VuProfile {
  const char *name;
  uint16_t sampleWindowMs;
  uint16_t peakHoldMs;
  float lowBandAlpha;
  float attackAlpha;
  float releaseAlpha;
};

const VuProfile PROFILES[] = {
  {"SUAVE", 60, 500, 0.12f, 0.35f, 0.15f},
  {"EQUIL", 40, 350, 0.18f, 0.45f, 0.20f},
  {"AGRES", 25, 220, 0.28f, 0.65f, 0.35f},
};
const uint8_t PROFILE_COUNT = sizeof(PROFILES) / sizeof(PROFILES[0]);

const float SENS_LEVELS[] = {0.70f, 0.90f, 1.10f, 1.35f, 1.65f};
const uint8_t SENS_COUNT = sizeof(SENS_LEVELS) / sizeof(SENS_LEVELS[0]);

uint8_t profileIndex = 1;     // inicia no EQUIL
uint8_t sensitivityIndex = 2; // inicia em 1.10x

uint8_t peakLevel = 1;
unsigned long peakUntilMs = 0;
float lowBand = 0;
float smoothed = 0;

unsigned long profileLastPress = 0;
unsigned long sensLastPress = 0;
unsigned long uiMessageUntil = 0;

uint8_t levelFromAmplitude(float amplitude) {
  long mapped = map((long)amplitude, 0, 350, 0, BAR_WIDTH);
  if (mapped < 1) mapped = 1; // mantém a 1ª coluna sempre acesa
  if (mapped > BAR_WIDTH) mapped = BAR_WIDTH;
  return (uint8_t)mapped;
}

void drawBar(uint8_t row, uint8_t filled) {
  lcd.setCursor(0, row);
  for (uint8_t i = 0; i < BAR_WIDTH; i++) {
    lcd.print(i < filled ? (char)255 : ' ');
  }
}

void drawPeakMarker(uint8_t row, uint8_t level, uint8_t peak, bool showPeak) {
  if (!showPeak || peak <= level) return;
  uint8_t peakCol = peak - 1;
  if (peakCol >= BAR_WIDTH) peakCol = BAR_WIDTH - 1;
  lcd.setCursor(peakCol, row);
  lcd.print('|');
}

void showModeMessage(const char *line0, const char *line1) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(line0);
  lcd.setCursor(0, 1);
  lcd.print(line1);
  uiMessageUntil = millis() + UI_MESSAGE_MS;
}

void showProfileMessage() {
  char line1[17];
  snprintf(line1, sizeof(line1), "%u/3 %s", profileIndex + 1, PROFILES[profileIndex].name);
  showModeMessage("Perfil:", line1);
}

void showSensitivityMessage() {
  char line1[17];
  int sensPct = (int)(SENS_LEVELS[sensitivityIndex] * 100.0f + 0.5f);
  snprintf(line1, sizeof(line1), "%u/5 %d%%", sensitivityIndex + 1, sensPct);
  showModeMessage("Sensibilidade:", line1);
}

void lcdSelfTestAnimation() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Teste LCD I2C");
  lcd.setCursor(0, 1);
  lcd.print("Aguarde...");
  delay(700);

  randomSeed(analogRead(A7));
  for (uint8_t frame = 0; frame < 18; frame++) {
    for (uint8_t r = 0; r < 2; r++) {
      lcd.setCursor(0, r);
      for (uint8_t c = 0; c < 16; c++) {
        lcd.print(random(0, 100) > 40 ? (char)255 : ' ');
      }
    }
    delay(110);
  }

  lcd.clear();
}

int peakToPeakAmplitude(uint16_t windowMs) {
  int minV = 1023;
  int maxV = 0;
  unsigned long start = millis();

  while (millis() - start < windowMs) {
    int v = analogRead(MIC_PIN);
    if (v < minV) minV = v;
    if (v > maxV) maxV = v;
  }
  return maxV - minV;
}

void handleButtons() {
  unsigned long now = millis();

  if (digitalRead(BTN_PROFILE_PIN) == LOW && (now - profileLastPress) > DEBOUNCE_MS) {
    profileLastPress = now;
    profileIndex = (profileIndex + 1) % PROFILE_COUNT;
    peakLevel = 1;
    peakUntilMs = 0;
    lowBand = 0;
    smoothed = 0;
    showProfileMessage();
  }

  if (digitalRead(BTN_SENSITIVITY_PIN) == LOW && (now - sensLastPress) > DEBOUNCE_MS) {
    sensLastPress = now;
    sensitivityIndex = (sensitivityIndex + 1) % SENS_COUNT;
    showSensitivityMessage();
  }
}

void setup() {
  pinMode(BTN_PROFILE_PIN, INPUT_PULLUP);
  pinMode(BTN_SENSITIVITY_PIN, INPUT_PULLUP);

  lcd.init();
  lcd.backlight();

  lcdSelfTestAnimation();
  showModeMessage("VU Meter I2C", "Iniciando...");
  delay(700);
  lcd.clear();
}

void loop() {
  handleButtons();

  if ((long)(uiMessageUntil - millis()) > 0) {
    delay(REFRESH_MS);
    return;
  }

  const VuProfile &p = PROFILES[profileIndex];

  int p2p = peakToPeakAmplitude(p.sampleWindowMs);
  float adjusted = p2p * SENS_LEVELS[sensitivityIndex];

  lowBand += p.lowBandAlpha * (adjusted - lowBand);
  float alpha = (lowBand >= smoothed) ? p.attackAlpha : p.releaseAlpha;
  smoothed += alpha * (lowBand - smoothed);

  uint8_t level = levelFromAmplitude(smoothed);

  unsigned long now = millis();
  if (level > peakLevel) {
    peakLevel = level;
    peakUntilMs = now + p.peakHoldMs;
  } else if ((long)(now - peakUntilMs) > 0) {
    peakLevel = level;
  }

  bool showPeak = ((long)(peakUntilMs - now) > 0);

  drawBar(0, level);
  drawBar(1, level);
  drawPeakMarker(0, level, peakLevel, showPeak);
  drawPeakMarker(1, level, peakLevel, showPeak);

  delay(REFRESH_MS);
}
