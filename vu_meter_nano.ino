#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ==========================
// VU Meter para Arduino Nano
// LCD 16x2 via I2C
// ==========================

// Endereço I2C mais comum: 0x27 (alguns módulos usam 0x3F)
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Pino analógico do módulo de microfone
const uint8_t MIC_PIN = A0;

// Controle do VU
const uint8_t BAR_WIDTH = 16;
const uint16_t SAMPLE_WINDOW_MS = 40;
const uint16_t REFRESH_MS = 60;
const uint16_t PEAK_HOLD_MS = 1000;

uint8_t levelFromAmplitude(int amplitude) {
  long mapped = map(amplitude, 0, 350, 0, BAR_WIDTH);
  if (mapped < 0) mapped = 0;
  if (mapped > BAR_WIDTH) mapped = BAR_WIDTH;
  if (mapped < 1) mapped = 1; // mantém a 1ª coluna sempre acesa
  return (uint8_t)mapped;
}

void drawBar(uint8_t row, uint8_t filled) {
  lcd.setCursor(0, row);
  for (uint8_t i = 0; i < BAR_WIDTH; i++) {
    lcd.print(i < filled ? (char)255 : ' ');
  }
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

  for (uint8_t i = 0; i < 3; i++) {
    lcd.clear();
    delay(90);
    for (uint8_t r = 0; r < 2; r++) {
      lcd.setCursor(0, r);
      for (uint8_t c = 0; c < 16; c++) {
        lcd.print((char)255);
      }
    }
    delay(90);
  }

  lcd.clear();
}


void drawPeakMarker(uint8_t row, uint8_t level, uint8_t peakLevel, bool showPeak) {
  if (!showPeak) return;
  if (peakLevel <= level) return;

  uint8_t peakCol = peakLevel - 1;
  if (peakCol >= BAR_WIDTH) peakCol = BAR_WIDTH - 1;
  lcd.setCursor(peakCol, row);
  lcd.print('|');
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

void setup() {
  lcd.init();
  lcd.backlight();

  lcdSelfTestAnimation();

  lcd.setCursor(0, 0);
  lcd.print("VU Meter I2C");
  lcd.setCursor(0, 1);
  lcd.print("Iniciando...");
  delay(700);
  lcd.clear();
}

void loop() {
  int p2p = peakToPeakAmplitude(SAMPLE_WINDOW_MS);

  static float smoothed = 0;
  static uint8_t peakLevel = 1;
  static unsigned long peakUntilMs = 0;

  smoothed = 0.75f * smoothed + 0.25f * p2p;

  uint8_t level = levelFromAmplitude((int)smoothed);

  unsigned long now = millis();
  if (level > peakLevel) {
    peakLevel = level;
    peakUntilMs = now + PEAK_HOLD_MS;
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
