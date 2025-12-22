#include "bt.h"
#include "pet_state.h"
#include "pet_ai.h"
#include "sound.h"
#include "personality.h"
#include "config.h"

// helpers
static int clampi(int v, int a, int b) {
  return v < a ? a : (v > b ? b : v);
}

static BTStatus actSetMode(Mode m) {
  if (ai.mode != m) {
    ai.mode = m;
    ai.modeSince = millis();
  }
  return BT_SUCCESS;
}

// ====== CONDITIONS (needs) ======
static bool cHungry() {
  return pet.st.hunger < 25;
}
static bool cThirsty() {
  return pet.st.thirst < 25;
}
static bool cDirty() {
  return pet.st.cleanliness < 25;
}
static bool cSleepy() {
  return (isNightNow() && pet.st.energy < 40);
}
static bool cBored() {
  return pet.st.boredom > 70;
}
static bool cWantsPet() {
  return (brain.attentionNeed > 60 && pet.st.bonding < 60);
}

// ====== ACTIONS (basic) ======
static BTStatus aFood() {
  return actSetMode(M_SEEK_FOOD);
}
static BTStatus aWater() {
  return actSetMode(M_SEEK_WATER);
}
static BTStatus aWash() {
  return actSetMode(M_WASH);
}
static BTStatus aPlay() {
  return actSetMode(M_PLAY);
}
static BTStatus aIdle() {
  return actSetMode(M_IDLE);
}
static BTStatus aSad() {
  return actSetMode(M_SAD);
}

// ============================================================================
// SLEEP SCENE (BT subtree)
// ============================================================================
static uint32_t sleepEnterMs = 0;

static void enterSleepMode(Mode m) {
  if (ai.mode != m) {
    ai.mode = m;
    ai.modeSince = millis();
    sleepEnterMs = millis();
    if (m == M_SLEEP_DROWSY) playSound(SND_SLEEP);  // короткий сигнал засыпания
  }
}

static bool isInSleepModes() {
  return (ai.mode == M_SLEEP_DROWSY || ai.mode == M_SLEEP_LIGHT || ai.mode == M_SLEEP_DEEP || ai.mode == M_WAKE_UP);
}

static BTStatus aSleepDrowsy() {
  enterSleepMode(M_SLEEP_DROWSY);
  return BT_RUNNING;
}

static BTStatus aSleepLight() {
  enterSleepMode(M_SLEEP_LIGHT);
  pet.st.energy = clampi(pet.st.energy + 1, 0, 100);
  pet.st.sleepDebt = clampi(pet.st.sleepDebt - 1, 0, 100);
  return BT_RUNNING;
}

static BTStatus aSleepDeep() {
  enterSleepMode(M_SLEEP_DEEP);
  pet.st.energy = clampi(pet.st.energy + 2, 0, 100);
  pet.st.sleepDebt = clampi(pet.st.sleepDebt - 2, 0, 100);
  return BT_RUNNING;
}

static BTStatus aWakeUp() {
  enterSleepMode(M_WAKE_UP);
  playSound(SND_WAKE);
  pet.emotion = HAPPY;  // <-- FIX: emotion в PetState, не в Stats
  // после короткой фазы пробуждения вернемся в idle
  if (millis() - sleepEnterMs > 2000) {
    ai.mode = M_IDLE;
    return BT_SUCCESS;
  }
  return BT_RUNNING;
}

static BTStatus btSleepTree() {
  // Если сон вообще не нужен И мы не в сцене сна — отдаем FAIL (не перехватываем root)
  if (!cSleepy() && !isInSleepModes()) return BT_FAIL;

  // Начало: ещё не спит — только хочет
  if (!isInSleepModes()) {
    return aSleepDrowsy();
  }

  // DROWSY -> LIGHT через 5 сек
  if (ai.mode == M_SLEEP_DROWSY && millis() - sleepEnterMs > 5000) {
    return aSleepLight();
  }

  // LIGHT -> DEEP через 15 сек, если нет сильных потребностей
  if (ai.mode == M_SLEEP_LIGHT && millis() - sleepEnterMs > 15000 && pet.st.hunger > 30 && pet.st.thirst > 30) {
    return aSleepDeep();
  }

  // LIGHT: может проснуться от голода/жажды
  if (ai.mode == M_SLEEP_LIGHT && (pet.st.hunger < 20 || pet.st.thirst < 20)) {
    return aWakeUp();
  }

  // DEEP: пробуждение при энергии > 80 или при крит. потребностях
  if (ai.mode == M_SLEEP_DEEP && (pet.st.energy > 80 || pet.st.hunger < 20 || pet.st.thirst < 20)) {
    return aWakeUp();
  }

  // WAKE_UP: короткая фаза
  if (ai.mode == M_WAKE_UP) {
    return aWakeUp();
  }

  // удержание текущей фазы сна
  if (ai.mode == M_SLEEP_LIGHT) return aSleepLight();
  if (ai.mode == M_SLEEP_DEEP) return aSleepDeep();

  return BT_RUNNING;
}

// ============================================================================
// SICKNESS SCENE (BT subtree)
// ============================================================================
static bool cMildSick() {
  return pet.st.health < 60 && pet.st.health >= 35;
}
static bool cHeavySick() {
  return pet.st.health < 35;
}

static BTStatus aSickMild() {
  if (ai.mode != M_SICK_MILD) {
    ai.mode = M_SICK_MILD;
    ai.modeSince = millis();
    playSound(SND_SICK);
  }
  pet.st.energy = clampi(pet.st.energy - 1, 0, 100);
  pet.st.happiness = clampi(pet.st.happiness - 1, 0, 100);
  return BT_RUNNING;
}

static BTStatus aSickHeavy() {
  if (ai.mode != M_SICK_HEAVY) {
    ai.mode = M_SICK_HEAVY;
    ai.modeSince = millis();
    playSound(SND_SICK);
  }
  pet.st.energy = clampi(pet.st.energy - 2, 0, 100);
  pet.st.happiness = clampi(pet.st.happiness - 2, 0, 100);
  return BT_RUNNING;
}

static BTStatus aSickRecovery() {
  if (ai.mode != M_SICK_RECOVERY) {
    ai.mode = M_SICK_RECOVERY;
    ai.modeSince = millis();
  }
  pet.st.health = clampi(pet.st.health + 1, 0, 100);
  pet.st.energy = clampi(pet.st.energy + 1, 0, 100);
  return BT_RUNNING;
}

static BTStatus btSickTree() {
  // Если есть болезнь — сцена активна
  if (cHeavySick()) return aSickHeavy();
  if (cMildSick()) return aSickMild();

  // Если мы были в болезни — переходим в recovery
  if (ai.mode == M_SICK_MILD || ai.mode == M_SICK_HEAVY) {
    if (pet.st.health > 70) return aSickRecovery();
    return BT_RUNNING;
  }

  // Recovery завершается
  if (ai.mode == M_SICK_RECOVERY) {
    if (pet.st.health > 90) {
      ai.mode = M_IDLE;
      playSound(SND_HAPPY);
      return BT_SUCCESS;
    }
    return aSickRecovery();
  }

  return BT_FAIL;
}

// ============================================================================
// “PERSONALITY STYLE” (idle policy)
// ============================================================================
static BTStatus aIdleByPersonality() {
  switch (pet.personality) {
    case P_ENERGETIC:
      if (pet.st.energy > 35) return aPlay();
      return aIdle();
    case P_CURIOUS:
      if (pet.st.boredom > 40 && pet.st.energy > 30) return aPlay();
      return aIdle();
    case P_SOCIAL:
      // социальный не форсит режим, но легче уходит в idle (просит вниманием через requestIfNeeded в AI)
      return aIdle();
    case P_GRUMPY:
      if (pet.st.boredom > 80) return aSad();
      return aIdle();
    case P_SHY:
      return aIdle();
    case P_CHILL:
    default:
      return aIdle();
  }
}

// ============================================================================
// ROOT SELECTOR
// ============================================================================
BTStatus btTick() {
  personalityTick();

  // 1) БОЛЕЗНЬ — абсолютный приоритет
  BTStatus s = btSickTree();
  if (s != BT_FAIL) return s;

  // 2) СОН — сцена (перехватывает только если сон нужен или уже начался)
  s = btSleepTree();
  if (s != BT_FAIL) return s;

  // 3) Потребности
  if (cHungry()) return aFood();
  if (cThirsty()) return aWater();
  if (cDirty()) return aWash();

  // 4) Скука/внимание
  if (cBored()) {
    if (brain.stubbornness > 70 && brain.irritation > 50) return aSad();
    return aPlay();
  }

  // 5) Idle политика личности
  return aIdleByPersonality();
}
