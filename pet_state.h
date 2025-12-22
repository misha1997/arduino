#pragma once
#include "pet_types.h"

extern PetState pet;

void petInit();
void simTick1s();

void actionFeed();
void actionDrink();
void actionPlay();
void actionWash();
void actionPet();
void actionMedicine();
void actionSleepToggle();

Emotion deriveEmotion();
const char* emotionToStr(Emotion e);
const char* stageToStr(Stage s);
