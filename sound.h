#pragma once
#include "pet_types.h"

enum WaveType {
  WAVE_SINE,
  WAVE_SQUARE,
  WAVE_SAW
};

void soundInit();
void playSound(SoundEvent ev);
void soundTick();
