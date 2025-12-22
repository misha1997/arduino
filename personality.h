#pragma once
#include "pet_types.h"

const char* personalityToStr(PersonalityType p);

// Вызывать раз в 1с/или раз в N секунд
void personalityTick();

// Ручной пересчёт (если нужно)
void personalityRecompute();
