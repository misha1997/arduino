#pragma once
#include <Arduino.h>

// OLED
#define OLED_SDA 5
#define OLED_SCL 4

// SD
#define SD_CS   10
#define SD_SCK  6
#define SD_MOSI 7
#define SD_MISO 8

// I2S
#define I2S_DOUT 9
#define I2S_BCLK 20
#define I2S_LRC  21

// BUTTON
#define BTN 2

// Display
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// Timing
#define SIM_TICK_MS   1000
#define ANIM_TICK_MS  80
#define AUTOSAVE_MS  60000

// Virtual day (24 minutes)
#define VDAY_MS (24UL * 60UL * 1000UL)

inline bool isNightNow() {
  // виртуальные сутки: VDAY_MS = 24 * 60 * 60 * 1000 (или твой масштаб)
  uint8_t h = (millis() % VDAY_MS) / 60000UL; // 0..23
  return (h >= 20 || h <= 6);
}
