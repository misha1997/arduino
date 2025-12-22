#include "pet_ai.h"
#include "pet_state.h"
#include "config.h"
#include "sound.h"

// ====== ГЛОБАЛЬНЫЕ ОБЪЕКТЫ (ОБЯЗАТЕЛЬНО) ======
Brain brain;
Habits habits;
AIState ai;

// ====== helpers ======
static int clampi(int v, int a, int b) {
  return v < a ? a : (v > b ? b : v);
}

// ====== INIT ======
void aiInit() {
  brain.trust = 50;
  brain.irritation = 0;
  brain.curiosity = 50;
  brain.stubbornness = 30;
  brain.attentionNeed = 20;
  brain.lastDecisionMs = 0;

  memset(&habits, 0, sizeof(habits));

  ai.mode = M_IDLE;
  ai.modeSince = millis();
  ai.lastChosenUtility = 0;
}

// ====== HABITS ======
void habitsOnAction(ActionType act) {
  uint8_t hour = (millis() % VDAY_MS) / 60000UL;

  habits.slot[act][hour] =
    clampi(habits.slot[act][hour] + 20, 0, 255);

  habits.total[act]++;
  habits.expect[act] = clampi(habits.expect[act] + 5, 0, 100);
}

int habitsAffinity(ActionType act) {
  uint8_t hour = (millis() % VDAY_MS) / 60000UL;
  return habits.slot[act][hour] + habits.expect[act];
}

bool refuseAction(ActionType act) {
  if (brain.stubbornness > 70 && brain.irritation > 60)
    return true;

  if (act == ACT_PLAY && pet.st.energy < 30)
    return true;

  return false;
}

static uint32_t lastRequestMs = 0;

static void requestIfNeeded() {
  if (millis() - lastRequestMs < 15000) return;
  lastRequestMs = millis();

  if (pet.st.hunger < 30 && habitsAffinity(ACT_FEED) > 120) {
    playSound(SND_SAD);
  }
  if (pet.st.boredom > 70 && habitsAffinity(ACT_PLAY) > 120) {
    playSound(SND_RANDOM);
  }
}

// ====== AI TICK ======
void aiTick() {
  btTick();
  requestIfNeeded();
}