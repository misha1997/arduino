#pragma once

#include <Arduino.h>
#include <WebServer.h>

// Инициализация WiFi + Web
void webInit();

// Обрабатывать клиента (вызывать в loop)
void webTick();
