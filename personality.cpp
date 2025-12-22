#include "personality.h"
#include "pet_state.h"
#include "pet_ai.h"
#include <Arduino.h>

static int clampi(int v,int a,int b){ return v<a?a:v>b?b:v; }

const char* personalityToStr(PersonalityType p) {
  static const char* n[] = {"CHILL","ENERGETIC","CURIOUS","SOCIAL","GRUMPY","SHY"};
  return n[(uint8_t)p];
}

// “Тихая” эволюция раз в 30 секунд
void personalityTick() {
  static uint32_t last = 0;
  if (millis() - last < 30000) return;
  last = millis();
  personalityRecompute();
}

static int scoreChill() {
  return (pet.st.health + pet.st.cleanliness + pet.st.bonding) / 3 - pet.st.boredom/2;
}
static int scoreEnergetic() {
  return pet.st.energeticness + pet.st.energy - pet.st.sleepDebt/2;
}
static int scoreCurious() {
  return brain.curiosity + pet.st.playfulness - pet.st.sleepDebt/3;
}
static int scoreSocial() {
  return pet.st.friendliness + pet.st.bonding - brain.irritation/2;
}
static int scoreGrumpy() {
  return brain.irritation + brain.stubbornness + (50 - pet.st.happiness);
}
static int scoreShy() {
  // shy: мало доверия + высокая потребность во внимании (просит, но стесняется)
  return (100 - brain.trust) + brain.attentionNeed + (50 - pet.st.happiness)/2;
}

void personalityRecompute() {
  int sc[6];
  sc[P_CHILL]     = scoreChill();
  sc[P_ENERGETIC] = scoreEnergetic();
  sc[P_CURIOUS]   = scoreCurious();
  sc[P_SOCIAL]    = scoreSocial();
  sc[P_GRUMPY]    = scoreGrumpy();
  sc[P_SHY]       = scoreShy();

  // Небольшая инерция (не “скачет” каждую проверку)
  PersonalityType best = pet.personality;
  int bestV = -9999;
  for (int i=0;i<6;i++){
    int v = sc[i];
    if ((PersonalityType)i == pet.personality) v += 10; // бонус текущему
    if (v > bestV) { bestV = v; best = (PersonalityType)i; }
  }

  if (best != pet.personality) {
    pet.personality = best;
    // немного XP за смену характера
    pet.xp += 10;
  }

  // Мягко подстраиваем “врождённые” параметры мозга под личность
  switch (pet.personality) {
    case P_CHILL:
      brain.irritation = clampi(brain.irritation - 2, 0, 100);
      brain.stubbornness = clampi(brain.stubbornness - 1, 0, 100);
      break;
    case P_ENERGETIC:
      brain.curiosity = clampi(brain.curiosity + 2, 0, 100);
      brain.attentionNeed = clampi(brain.attentionNeed + 1, 0, 100);
      break;
    case P_CURIOUS:
      brain.curiosity = clampi(brain.curiosity + 3, 0, 100);
      break;
    case P_SOCIAL:
      brain.trust = clampi(brain.trust + 2, 0, 100);
      brain.irritation = clampi(brain.irritation - 1, 0, 100);
      break;
    case P_GRUMPY:
      brain.irritation = clampi(brain.irritation + 2, 0, 100);
      brain.stubbornness = clampi(brain.stubbornness + 2, 0, 100);
      break;
    case P_SHY:
      brain.trust = clampi(brain.trust + 1, 0, 100); // постепенно адаптируется
      break;
  }
}
