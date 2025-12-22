#include "config.h"
#include "pet_types.h"
#include "pet_state.h"
#include "pet_ai.h"
#include "sound.h"
#include "anim.h"
#include "web.h"
#include "storage.h"

void setup() {
  Serial.begin(115200);

  pinMode(BTN, INPUT);  // или INPUT_PULLDOWN для TTP223

  petInit();
  aiInit();
  soundInit();
  animInit();
  webInit();
  storageInit();
  storageLoad();

  Serial.println("Virtual Pet started");
}

void loop() {
  static uint32_t lastSim = 0;
  static bool lastBtn = false;
  static uint32_t lastSave = 0;
  bool b = digitalRead(BTN);

  webTick();    // WEB
  soundTick();  // AUDIO
  animTick();   // OLED

  if (b && !lastBtn) {
    actionPet();
  }
  lastBtn = b;

  if (millis() - lastSave > 60000) {
    lastSave = millis();
    storageSave();
  }

  if (millis() - lastSim >= SIM_TICK_MS) {
    lastSim = millis();
    simTick1s();
  }
}
