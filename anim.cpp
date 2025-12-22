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
static bool displayOK = false;
static bool sdOK = false;

static int clampi(int v,int a,int b){return v<a?a:v>b?b:v;}

void animInit() {
  Wire.begin(OLED_SDA, OLED_SCL);

  if (display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    displayOK = true;
    display.clearDisplay();
    display.display();
    Serial.println("[ANIM] Display OK");
  } else {
    Serial.println("[ANIM] Display FAIL!");
  }

  SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
  if (SD.begin(SD_CS)) {
    sdOK = true;
    Serial.println("[ANIM] SD OK");
  } else {
    Serial.println("[ANIM] SD FAIL!");
  }
}

static bool loadFrame(Stage s, Emotion e, uint16_t idx) {
  if (!sdOK) return false;
  
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

static void drawProceduralPet() {
  display.clearDisplay();
  
  int centerX = SCREEN_WIDTH / 2;
  int centerY = SCREEN_HEIGHT / 2;
  
  // Animation offset based on frame (bouncing animation)
  int bounce = (frame % 20) < 10 ? (frame % 10) : (10 - (frame % 10));
  int yOffset = bounce / 2;
  
  // Size based on stage
  int size = 10;
  switch (pet.stage) {
    case ST_EGG: size = 8; break;
    case ST_BABY: size = 10; break;
    case ST_CHILD: size = 12; break;
    case ST_TEEN: size = 14; break;
    case ST_ADULT: size = 16; break;
  }
  
  int petY = centerY + yOffset - 10;
  
  // Draw pet body (circle)
  display.fillCircle(centerX, petY, size, SSD1306_WHITE);
  
  // Draw eyes based on emotion
  int eyeY = petY - size/3;
  int eyeSize = 2;
  bool eyesClosed = false;
  
  switch (pet.emotion) {
    case SLEEPY:
    case TIRED:
      eyesClosed = (frame % 20) < 15; // blink
      if (!eyesClosed) {
        display.fillCircle(centerX - size/3, eyeY, eyeSize, SSD1306_BLACK);
        display.fillCircle(centerX + size/3, eyeY, eyeSize, SSD1306_BLACK);
      } else {
        display.drawLine(centerX - size/3 - 2, eyeY, centerX - size/3 + 2, eyeY, SSD1306_WHITE);
        display.drawLine(centerX + size/3 - 2, eyeY, centerX + size/3 + 2, eyeY, SSD1306_WHITE);
      }
      break;
    case HAPPY:
    case EXCITED:
      display.fillCircle(centerX - size/3, eyeY, eyeSize, SSD1306_BLACK);
      display.fillCircle(centerX + size/3, eyeY, eyeSize, SSD1306_BLACK);
      // Smile (simple curve using pixels)
      for (int i = -size/3; i <= size/3; i++) {
        int y = petY + size/4 + (i*i) / (size*2);
        display.drawPixel(centerX + i, y, SSD1306_BLACK);
      }
      break;
    case SAD:
      display.fillCircle(centerX - size/3, eyeY, eyeSize, SSD1306_BLACK);
      display.fillCircle(centerX + size/3, eyeY, eyeSize, SSD1306_BLACK);
      // Frown (inverted curve)
      for (int i = -size/3; i <= size/3; i++) {
        int y = petY + size/2 - (i*i) / (size*2);
        display.drawPixel(centerX + i, y, SSD1306_BLACK);
      }
      break;
    case HUNGRY:
    case THIRSTY:
      display.fillCircle(centerX - size/3, eyeY, eyeSize, SSD1306_BLACK);
      display.fillCircle(centerX + size/3, eyeY, eyeSize, SSD1306_BLACK);
      // Open mouth (oval)
      display.fillRect(centerX - 3, petY + size/4, 6, 4, SSD1306_BLACK);
      break;
    default:
      display.fillCircle(centerX - size/3, eyeY, eyeSize, SSD1306_BLACK);
      display.fillCircle(centerX + size/3, eyeY, eyeSize, SSD1306_BLACK);
      break;
  }
  
  // Draw status bar at bottom
  int barY = SCREEN_HEIGHT - 8;
  int barW = SCREEN_WIDTH - 4;
  int barH = 4;
  
  // Hunger bar
  int hungerW = (pet.st.hunger * barW) / 100;
  display.fillRect(2, barY, hungerW, barH, SSD1306_WHITE);
  
  // Happiness indicator (simple heart symbol)
  if (pet.st.happiness > 50) {
    display.drawPixel(SCREEN_WIDTH - 10, 2, SSD1306_WHITE);
    display.drawPixel(SCREEN_WIDTH - 8, 2, SSD1306_WHITE);
    display.drawPixel(SCREEN_WIDTH - 6, 2, SSD1306_WHITE);
    display.drawPixel(SCREEN_WIDTH - 9, 3, SSD1306_WHITE);
    display.drawPixel(SCREEN_WIDTH - 7, 3, SSD1306_WHITE);
    display.drawPixel(SCREEN_WIDTH - 8, 4, SSD1306_WHITE);
  }
  
  display.display();
}

void animTick() {
  if (!displayOK) return;
  
  if (millis() - lastAnim < ANIM_TICK_MS) return;
  lastAnim = millis();

  // Try to load frame from SD card
  bool frameLoaded = loadFrame(pet.stage, pet.emotion, frame);
  if (!frameLoaded) {
    frame = 0;
    frameLoaded = loadFrame(pet.stage, pet.emotion, frame);
  }

  // If SD frames available, draw bitmap; otherwise show procedural animation
  if (frameLoaded && sdOK) {
    display.clearDisplay();
    display.drawBitmap(0, 0, frameBuf, SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_WHITE);
    display.display();
    frame++;
    if (frame > 20) frame = 0;
  } else {
    // Fallback: show animated procedural pet
    drawProceduralPet();
    frame++;
    if (frame > 50) frame = 0; // Longer cycle for procedural animation
  }
}
