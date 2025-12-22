#include "pet_state.h"
#include "config.h"
#include "pet_ai.h"
#include "sound.h"
#include "bt.h"

PetState pet;

static int clampi(int v, int a, int b) {
  return v < a ? a : v > b ? b
                           : v;
}

void actionFeed() {
  if (ai.mode >= M_SLEEP_DROWSY && ai.mode <= M_WAKE_UP) return;
  if (ai.mode == M_SICK_HEAVY) return;
  habitsOnAction(ACT_FEED);
  pet.st.hunger = clampi(pet.st.hunger + 30, 0, 100);
  pet.st.happiness = clampi(pet.st.happiness + 3, 0, 100);
  pet.st.cntFeed++;
  pet.xp += 1;
  playSound(SND_FEED);
}
void actionDrink() {
  if (ai.mode >= M_SLEEP_DROWSY && ai.mode <= M_WAKE_UP) return;
  if (ai.mode == M_SICK_HEAVY) return;
  habitsOnAction(ACT_DRINK);
  pet.st.thirst = clampi(pet.st.thirst + 30, 0, 100);
  pet.st.happiness = clampi(pet.st.happiness + 2, 0, 100);
  pet.st.cntDrink++;
  pet.xp += 1;
  playSound(SND_DRINK);
}
void actionPlay() {
  if (ai.mode >= M_SLEEP_DROWSY && ai.mode <= M_WAKE_UP) return;
  if (ai.mode == M_SICK_HEAVY) return;
  habitsOnAction(ACT_PLAY);
  if (pet.st.energy < 20) {
    playSound(SND_ANGRY);
    return;
  }
  pet.st.energy = clampi(pet.st.energy - 6, 0, 100);
  pet.st.happiness = clampi(pet.st.happiness + 8, 0, 100);
  pet.st.playfulness = clampi(pet.st.playfulness + 1, 0, 100);
  pet.st.boredom = clampi(pet.st.boredom - 20, 0, 100);
  pet.st.cntPlay++;
  pet.xp += 3;
  playSound(SND_PLAY);
}
void actionWash() {
  if (ai.mode >= M_SLEEP_DROWSY && ai.mode <= M_WAKE_UP) return;
  if (ai.mode == M_SICK_HEAVY) return;
  habitsOnAction(ACT_WASH);
  pet.st.cleanliness = clampi(pet.st.cleanliness + 25, 0, 100);
  pet.st.happiness = clampi(pet.st.happiness + 4, 0, 100);
  pet.st.cntWash++;
  pet.xp += 1;
  playSound(SND_WASH);
}
void actionPet() {
  if (ai.mode >= M_SLEEP_DROWSY && ai.mode <= M_WAKE_UP) return;
  if (ai.mode == M_SICK_HEAVY) return;
  habitsOnAction(ACT_PET);
  pet.st.happiness = clampi(pet.st.happiness + 6, 0, 100);
  pet.st.bonding = clampi(pet.st.bonding + 4, 0, 100);
  pet.st.friendliness = clampi(pet.st.friendliness + 1, 0, 100);
  pet.st.cntPet++;
  pet.xp += 2;
  playSound(SND_PET);
  if (!pet.st.sleeping) {
    pet.st.sleepDebt = clampi(pet.st.sleepDebt + (isNightNow() ? 2 : 1), 0, 100);
  }
}
void actionMedicine() {
  habitsOnAction(ACT_MED);
  pet.st.health = clampi(pet.st.health + 15, 0, 100);
  pet.st.cntMed++;
  playSound(SND_SICK);  // Using SND_SICK as medicine sound
}
void actionSleepToggle() {
  habitsOnAction(ACT_SLEEP_TOGGLE);
  // Toggle sleep mode - if not sleeping, force sleep; if sleeping, wake up
  if (ai.mode >= M_SLEEP_DROWSY && ai.mode <= M_SLEEP_DEEP) {
    // Wake up
    ai.mode = M_WAKE_UP;
    ai.modeSince = millis();
    playSound(SND_WAKE);
  } else if (ai.mode != M_SICK_HEAVY) {
    // Force sleep
    ai.mode = M_SLEEP_DROWSY;
    ai.modeSince = millis();
    playSound(SND_SLEEP);
  }
}


void petInit() {
  pet.bornMillis = millis();
  pet.lastInteractionMillis = millis();
  pet.stage = ST_EGG;
  pet.emotion = NEUTRAL;
  pet.st = {};
  pet.st.hunger = pet.st.thirst = pet.st.energy = 80;
  pet.st.cleanliness = pet.st.health = 80;
  pet.st.happiness = 60;

  pet.personality = P_CHILL;
  pet.xp = 0;
}

Emotion deriveEmotion() {
  if (pet.st.health < 35) return SICK;
  if (pet.st.hunger < 25) return HUNGRY;
  if (pet.st.thirst < 25) return THIRSTY;
  if (pet.st.sleeping) return SLEEPY;
  if (pet.st.energy < 25) return TIRED;
  if (pet.st.mood < -50) return SAD;
  if (pet.st.mood > 40) return HAPPY;
  return NEUTRAL;
}

void simTick1s() {
  pet.st.hunger = clampi(pet.st.hunger - 1, 0, 100);
  pet.st.thirst = clampi(pet.st.thirst - 1, 0, 100);
  pet.st.boredom = clampi(pet.st.boredom + 1, 0, 100);

  // Synchronize sleeping flag with AI modes
  pet.st.sleeping = (ai.mode >= M_SLEEP_DROWSY && ai.mode <= M_SLEEP_DEEP);

  aiTick();  // FSM / habits

  Emotion e = deriveEmotion();
  if (e != pet.emotion) {
    pet.emotion = e;
    if (e == HAPPY) playSound(SND_HAPPY);
    if (e == SAD) playSound(SND_SAD);
    if (e == ANGRY) playSound(SND_ANGRY);
  }
}

const char* emotionToStr(Emotion e) {
  static const char* n[] = { "HAPPY", "EXCITED", "PLAYFUL", "CONTENT", "NEUTRAL", "TIRED", "SLEEPY", "HUNGRY", "THIRSTY", "SAD", "SICK", "ANGRY" };
  return n[e];
}

const char* stageToStr(Stage s) {
  static const char* n[] = { "EGG", "BABY", "CHILD", "TEEN", "ADULT" };
  return n[s];
}
