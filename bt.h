#pragma once
#include <Arduino.h>

enum BTStatus : uint8_t { BT_FAIL, BT_SUCCESS, BT_RUNNING };

// один тик дерева (вызывать раз в 1с)
BTStatus btTick();
