#pragma once
#include <Arduino.h>

// ------------------- EMOTIONS -------------------
enum Emotion : uint8_t {
  HAPPY,
  EXCITED,
  PLAYFUL,
  CONTENT,
  NEUTRAL,
  TIRED,
  SLEEPY,
  HUNGRY,
  THIRSTY,
  SAD,
  SICK,
  ANGRY
};

// ------------------- GROWTH -------------------
enum Stage : uint8_t {
  ST_EGG,
  ST_BABY,
  ST_CHILD,
  ST_TEEN,
  ST_ADULT
};

// ------------------- AI -------------------
enum Mode : uint8_t {
  M_IDLE,
  M_PLAY,
  M_SEEK_FOOD,
  M_SEEK_WATER,
  M_WASH,
  M_SLEEP_DROWSY,  // хочет спать
  M_SLEEP_LIGHT,   // чуткий сон
  M_SLEEP_DEEP,    // глубокий сон
  M_WAKE_UP,
  M_SICK_MILD,
  M_SICK_HEAVY,
  M_SICK_RECOVERY,
  M_SAD
};

enum ActionType : uint8_t {
  ACT_FEED,
  ACT_DRINK,
  ACT_PLAY,
  ACT_PET,
  ACT_WASH,
  ACT_MED,
  ACT_SLEEP_TOGGLE,
  ACT__COUNT
};

enum SoundEvent : uint8_t {
  SND_PET,
  SND_FEED,
  SND_DRINK,
  SND_PLAY,
  SND_WASH,
  SND_SLEEP,
  SND_WAKE,
  SND_HAPPY,
  SND_SAD,
  SND_ANGRY,
  SND_SICK,
  SND_RANDOM
};

// ------------------- PERSONALITY -------------------
enum PersonalityType : uint8_t {
  P_CHILL,      // спокойный
  P_ENERGETIC,  // активный
  P_CURIOUS,    // любопытный
  P_SOCIAL,     // дружелюбный
  P_GRUMPY,     // ворчливый
  P_SHY         // стеснительный
};


// ------------------- STRUCTS -------------------
struct Stats {
  int hunger, thirst, energy, happiness, cleanliness, health, bonding;
  int playfulness, energeticness, friendliness;
  int mood;
  int sleepDebt;
  bool sleeping;
  int boredom;
  uint32_t happyStreak, bestHappyStreak;
  uint32_t cntFeed, cntDrink, cntPlay, cntWash, cntPet, cntMed;
};

struct PetState {
  uint32_t bornMillis;
  uint32_t lastInteractionMillis;
  Stage stage;
  Emotion emotion;
  Stats st;

  PersonalityType personality;
  uint32_t xp;
};

struct Brain {
  int trust, irritation, curiosity, stubbornness, attentionNeed;
  uint32_t lastDecisionMs;
};

struct Habits {
  uint8_t slot[ACT__COUNT][24];
  int expect[ACT__COUNT];
  uint32_t total[ACT__COUNT];
};

struct AIState {
  Mode mode;
  uint32_t modeSince;
  int lastChosenUtility;
};
