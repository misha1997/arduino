#pragma once
#include "pet_types.h"
#include "bt.h"
#include "personality.h"

extern Brain brain;
extern Habits habits;
extern AIState ai;

void aiInit();
void aiTick();

void habitsOnAction(ActionType act);
int  habitsAffinity(ActionType act);