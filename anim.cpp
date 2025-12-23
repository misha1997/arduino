#include "anim.h"
#include "config.h"
#include "pet_state.h"

#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

static Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
static uint8_t frameBuf[1024];  // 128x64 / 8 = 1024 bytes
static uint16_t frame = 0;
static uint32_t lastAnim = 0;
static bool displayOK = false;
static bool sdOK = false;

// Кэш для избежания повторного чтения того же кадра
static Stage cachedStage = ST_EGG;
static Emotion cachedEmotion = NEUTRAL;
static uint16_t cachedFrame = 0;
static bool cacheValid = false;

static int clampi(int v,int a,int b){return v<a?a:v>b?b:v;}

void animInit() {
  Wire.begin(OLED_SDA, OLED_SCL);

  if (display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    displayOK = true;
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("Loading...");
    display.display();
    Serial.println("[ANIM] Display OK");
  } else {
    Serial.println("[ANIM] Display FAIL!");
  }

  SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
  if (SD.begin(SD_CS)) {
    sdOK = true;
    Serial.println("[ANIM] SD OK");
    
    // Проверка структуры папок
    if (SD.exists("/anim")) {
      Serial.println("[ANIM] /anim found");
      
      // Проверяем наличие хотя бы одной анимации
      File root = SD.open("/anim");
      if (root) {
        File entry = root.openNextFile();
        if (entry) {
          Serial.print("[ANIM] Found: ");
          Serial.println(entry.name());
          entry.close();
        }
        root.close();
      }
    } else {
      Serial.println("[ANIM] WARNING: /anim folder not found!");
      Serial.println("[ANIM] Expected structure: /anim/<stage>/<emotion>/f000.bin");
    }
  } else {
    Serial.println("[ANIM] SD FAIL!");
  }
}

static bool loadFrame(Stage s, Emotion e, uint16_t idx) {
  if (!sdOK) return false;
  
  // Проверка кэша
  if (cacheValid && cachedStage == s && cachedEmotion == e && cachedFrame == idx) {
    return true; // Данные уже в буфере
  }
  
  char path[80];
  snprintf(path, sizeof(path),
           "/anim/%s/%s/f%03u.bin",
           stageToStr(s),
           emotionToStr(e),
           idx);

  File f = SD.open(path, FILE_READ);
  if (!f) {
    // Попробуем альтернативное имя в нижнем регистре
    snprintf(path, sizeof(path),
             "/anim/%s/%s/f%03u.bin",
             stageToStr(s),
             emotionToStr(e),
             idx);
    // Конвертируем в lowercase
    for (char* p = path; *p; p++) {
      if (*p >= 'A' && *p <= 'Z') *p += 32;
    }
    f = SD.open(path, FILE_READ);
    
    if (!f) {
      // Если первый кадр не найден, выводим в лог
      if (idx == 0) {
        Serial.print("[ANIM] Not found: ");
        Serial.println(path);
      }
      cacheValid = false;
      return false;
    }
  }

  size_t fileSize = f.size();
  if (fileSize != sizeof(frameBuf)) {
    Serial.print("[ANIM] Wrong size: ");
    Serial.print(path);
    Serial.print(" (");
    Serial.print(fileSize);
    Serial.print(" bytes, expected ");
    Serial.print(sizeof(frameBuf));
    Serial.println(")");
    f.close();
    cacheValid = false;
    return false;
  }

  size_t r = f.read(frameBuf, sizeof(frameBuf));
  f.close();
  
  if (r == sizeof(frameBuf)) {
    // Обновляем кэш
    cachedStage = s;
    cachedEmotion = e;
    cachedFrame = idx;
    cacheValid = true;
    return true;
  }
  
  cacheValid = false;
  return false;
}

static void drawProceduralPet() {
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
      eyesClosed = (frame % 20) < 15;
      if (!eyesClosed) {
        display.fillCircle(centerX - size/3, eyeY, eyeSize, SSD1306_BLACK);
        display.fillCircle(centerX + size/3, eyeY, eyeSize, SSD1306_BLACK);
      } else {
        display.drawLine(centerX - size/3 - 2, eyeY, centerX - size/3 + 2, eyeY, SSD1306_BLACK);
        display.drawLine(centerX + size/3 - 2, eyeY, centerX + size/3 + 2, eyeY, SSD1306_BLACK);
      }
      break;
    case HAPPY:
    case EXCITED:
    case PLAYFUL:
      display.fillCircle(centerX - size/3, eyeY, eyeSize, SSD1306_BLACK);
      display.fillCircle(centerX + size/3, eyeY, eyeSize, SSD1306_BLACK);
      for (int i = -size/3; i <= size/3; i++) {
        int y = petY + size/4 + (i*i) / (size*2);
        if (y >= 0 && y < SCREEN_HEIGHT) {
          display.drawPixel(centerX + i, y, SSD1306_BLACK);
        }
      }
      break;
    case SAD:
    case SICK:
      display.fillCircle(centerX - size/3, eyeY, eyeSize, SSD1306_BLACK);
      display.fillCircle(centerX + size/3, eyeY, eyeSize, SSD1306_BLACK);
      for (int i = -size/3; i <= size/3; i++) {
        int y = petY + size/2 - (i*i) / (size*2);
        if (y >= 0 && y < SCREEN_HEIGHT) {
          display.drawPixel(centerX + i, y, SSD1306_BLACK);
        }
      }
      break;
    case HUNGRY:
    case THIRSTY:
      display.fillCircle(centerX - size/3, eyeY, eyeSize, SSD1306_BLACK);
      display.fillCircle(centerX + size/3, eyeY, eyeSize, SSD1306_BLACK);
      display.fillRect(centerX - 3, petY + size/4, 6, 4, SSD1306_BLACK);
      break;
    case ANGRY:
      display.fillCircle(centerX - size/3, eyeY, eyeSize, SSD1306_BLACK);
      display.fillCircle(centerX + size/3, eyeY, eyeSize, SSD1306_BLACK);
      display.drawLine(centerX - size/3 - 3, eyeY - 3, centerX - size/3 + 1, eyeY - 1, SSD1306_BLACK);
      display.drawLine(centerX + size/3 - 1, eyeY - 1, centerX + size/3 + 3, eyeY - 3, SSD1306_BLACK);
      break;
    default:
      display.fillCircle(centerX - size/3, eyeY, eyeSize, SSD1306_BLACK);
      display.fillCircle(centerX + size/3, eyeY, eyeSize, SSD1306_BLACK);
      display.drawLine(centerX - size/4, petY + size/3, centerX + size/4, petY + size/3, SSD1306_BLACK);
      break;
  }
  
  // Status bars at bottom
  int barY = SCREEN_HEIGHT - 16;
  int barW = SCREEN_WIDTH - 4;
  int barH = 3;
  int barSpacing = 4;
  
  int hungerW = (pet.st.hunger * barW) / 100;
  display.drawRect(2, barY, barW, barH, SSD1306_WHITE);
  display.fillRect(2, barY, hungerW, barH, SSD1306_WHITE);
  
  barY += barSpacing;
  int energyW = (pet.st.energy * barW) / 100;
  display.drawRect(2, barY, barW, barH, SSD1306_WHITE);
  display.fillRect(2, barY, energyW, barH, SSD1306_WHITE);
  
  barY += barSpacing;
  int happyW = (pet.st.happiness * barW) / 100;
  display.drawRect(2, barY, barW, barH, SSD1306_WHITE);
  display.fillRect(2, barY, happyW, barH, SSD1306_WHITE);
  
  // Top status
  display.setCursor(2, 2);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.print(stageToStr(pet.stage));
  
  display.setCursor(2, 12);
  display.print(emotionToStr(pet.emotion));
  
  if (pet.st.happiness > 70) {
    int x = SCREEN_WIDTH - 8;
    int y = 2;
    display.fillRect(x, y+1, 2, 1, SSD1306_WHITE);
    display.fillRect(x+3, y+1, 2, 1, SSD1306_WHITE);
    display.fillRect(x-1, y+2, 7, 2, SSD1306_WHITE);
    display.fillRect(x, y+4, 5, 1, SSD1306_WHITE);
    display.fillRect(x+1, y+5, 3, 1, SSD1306_WHITE);
    display.drawPixel(x+2, y+6, SSD1306_WHITE);
  }
  
  if (pet.st.sleeping) {
    display.setCursor(SCREEN_WIDTH - 20, 12);
    display.print("zzZ");
  }
  
  if (pet.st.health < 40) {
    display.setCursor(SCREEN_WIDTH - 10, 22);
    display.print("!");
  }
}

static void drawStatusOverlay() {
  // Рисуем статус-бары поверх анимации с SD
  int barY = SCREEN_HEIGHT - 16;
  int barW = SCREEN_WIDTH - 4;
  int barH = 3;
  int barSpacing = 4;
  
  // Затемняем фон под барами
  display.fillRect(0, SCREEN_HEIGHT - 18, SCREEN_WIDTH, 18, SSD1306_BLACK);
  
  // Голод
  int hungerW = (pet.st.hunger * barW) / 100;
  display.drawRect(2, barY, barW, barH, SSD1306_WHITE);
  display.fillRect(2, barY, hungerW, barH, SSD1306_WHITE);
  
  // Энергия
  barY += barSpacing;
  int energyW = (pet.st.energy * barW) / 100;
  display.drawRect(2, barY, barW, barH, SSD1306_WHITE);
  display.fillRect(2, barY, energyW, barH, SSD1306_WHITE);
  
  // Счастье
  barY += barSpacing;
  int happyW = (pet.st.happiness * barW) / 100;
  display.drawRect(2, barY, barW, barH, SSD1306_WHITE);
  display.fillRect(2, barY, happyW, barH, SSD1306_WHITE);
  
  // Иконки сверху
  if (pet.st.sleeping) {
    display.fillRect(SCREEN_WIDTH - 22, 0, 22, 10, SSD1306_BLACK);
    display.setCursor(SCREEN_WIDTH - 20, 2);
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.print("zzZ");
  }
  
  if (pet.st.health < 40) {
    display.fillRect(SCREEN_WIDTH - 12, 10, 12, 10, SSD1306_BLACK);
    display.setCursor(SCREEN_WIDTH - 10, 12);
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.print("!");
  }
  
  if (pet.st.happiness > 70) {
    int x = SCREEN_WIDTH - 8;
    int y = 22;
    display.fillRect(x-2, y, 10, 8, SSD1306_BLACK);
    display.fillRect(x, y+1, 2, 1, SSD1306_WHITE);
    display.fillRect(x+3, y+1, 2, 1, SSD1306_WHITE);
    display.fillRect(x-1, y+2, 7, 2, SSD1306_WHITE);
    display.fillRect(x, y+4, 5, 1, SSD1306_WHITE);
    display.fillRect(x+1, y+5, 3, 1, SSD1306_WHITE);
    display.drawPixel(x+2, y+6, SSD1306_WHITE);
  }
}

void animTick() {
  if (!displayOK) return;
  
  if (millis() - lastAnim < ANIM_TICK_MS) return;
  lastAnim = millis();

  display.clearDisplay();

  // Попытка загрузить кадр с SD
  bool frameLoaded = loadFrame(pet.stage, pet.emotion, frame);
  
  if (frameLoaded) {
    // Отображаем кадр с SD-карты
    display.drawBitmap(0, 0, frameBuf, SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_WHITE);
    
    // Добавляем статус-бары поверх
    drawStatusOverlay();
    
    frame++;
    
    // Проверяем существование следующего кадра
    if (!loadFrame(pet.stage, pet.emotion, frame)) {
      // Следующий кадр не найден - цикл завершён
      frame = 0;
    }
  } else {
    // Fallback: процедурная анимация
    if (frame == 0) {
      Serial.println("[ANIM] Using procedural fallback");
    }
    drawProceduralPet();
    frame++;
    if (frame > 50) frame = 0;
  }
  
  display.display();
}

// Дополнительная функция для отладки
void animDebugSD() {
  if (!sdOK) {
    Serial.println("[DEBUG] SD not available");
    return;
  }
  
  Serial.println("[DEBUG] Checking animation files:");
  
  const char* stages[] = {"EGG", "BABY", "CHILD", "TEEN", "ADULT"};
  const char* emotions[] = {"HAPPY", "EXCITED", "PLAYFUL", "CONTENT", "NEUTRAL", 
                            "TIRED", "SLEEPY", "HUNGRY", "THIRSTY", "SAD", "SICK", "ANGRY"};
  
  for (int s = 0; s < 5; s++) {
    for (int e = 0; e < 12; e++) {
      char path[80];
      snprintf(path, sizeof(path), "/anim/%s/%s", stages[s], emotions[e]);
      
      if (SD.exists(path)) {
        File dir = SD.open(path);
        if (dir) {
          int count = 0;
          File entry = dir.openNextFile();
          while (entry) {
            count++;
            entry.close();
            entry = dir.openNextFile();
          }
          dir.close();
          Serial.printf("  %s/%s: %d frames\n", stages[s], emotions[e], count);
        }
      }
    }
  }
}