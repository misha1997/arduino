#include "anim.h"
#include "config.h"
#include "pet_state.h"

#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

static Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
static uint8_t frameBuf[1024];
static uint16_t frame = 0;
static uint32_t lastAnim = 0;

static int clampi(int v,int a,int b){return v<a?a:v>b?b:v;}

void animInit() {
  Wire.begin(OLED_SDA, OLED_SCL);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.display();

  SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
  SD.begin(SD_CS);
}

static bool loadFrame(Stage s, Emotion e, uint16_t idx) {
  char path[80];
  snprintf(path, sizeof(path),
           "/anim/%s/%s/f%03u.bin",
           stageToStr(s),
           emotionToStr(e),
           idx);

  File f = SD.open(path, FILE_READ);
  if (!f) return false;

  size_t r = f.read(frameBuf, sizeof(frameBuf));
  f.close();
  return r == sizeof(frameBuf);
}

void animTick() {
  if (millis() - lastAnim < ANIM_TICK_MS) return;
  lastAnim = millis();

  if (!loadFrame(pet.stage, pet.emotion, frame)) {
    frame = 0;
    loadFrame(pet.stage, pet.emotion, frame);
  }

  display.clearDisplay();
  display.drawBitmap(0, 0, frameBuf, SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_WHITE);
  display.display();

  frame++;
  if (frame > 20) frame = 0; // пока фикс
}
