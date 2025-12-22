#include "web.h"

#include <WiFi.h>
#include <ArduinoJson.h>

#include "config.h"
#include "pet_state.h"
#include "pet_ai.h"
#include "sound.h"

// ================= WiFi =================

static const char* AP_SSID = "PET-ESP32C3";
static const char* AP_PASS = "12345678";

static WebServer server(80);

// ================= HTML =================

static const char HTML_INDEX[] PROGMEM = R"HTML(
<!doctype html>
<html>
<head>
<meta charset="utf-8"/>
<meta name="viewport" content="width=device-width, initial-scale=1"/>
<title>ESP32 Virtual Pet</title>
<style>
body{font-family:system-ui;background:#fafafa;max-width:720px;margin:16px auto;padding:0 12px;}
.grid{display:grid;grid-template-columns:repeat(2,1fr);gap:10px;}
button{padding:14px;border-radius:14px;border:1px solid #ccc;font-size:16px;background:#fff}
pre{background:#eee;padding:12px;border-radius:12px;overflow:auto}
h2{margin-top:0}
.small{opacity:.7;font-size:12px}
</style>
</head>
<body>

<h2>🐣 ESP32 Virtual Pet</h2>

<div class="grid">
  <button onclick="act('feed')">🍕 Feed</button>
  <button onclick="act('drink')">💧 Drink</button>
  <button onclick="act('play')">🎮 Play</button>
  <button onclick="act('wash')">🧼 Wash</button>
  <button onclick="act('pet')">🤍 Pet</button>
  <button onclick="act('med')">💊 Medicine</button>
  <button onclick="act('sleep')">🌙 Sleep</button>
</div>

<p class="small">
OLED: animation only · Sound: synthesized · AI: FSM + habits
</p>

<h3>Status</h3>
<pre id="st">loading...</pre>

<script>
async function act(name){
  await fetch('/action?name='+encodeURIComponent(name));
  await update();
}

async function update(){
  const r = await fetch('/status');
  document.getElementById('st').textContent = await r.text();
}

setInterval(update, 800);
update();
</script>

</body>
</html>
)HTML";

// ================= JSON =================

static String buildStatusJson() {
  StaticJsonDocument<768> doc;

  doc["stage"] = stageToStr(pet.stage);
  doc["emotion"] = emotionToStr(pet.emotion);
  doc["mode"] = (int)ai.mode;

  doc["virtualHour"] = (millis() % VDAY_MS) / 60000UL;

  JsonObject needs = doc.createNestedObject("needs");
  needs["hunger"] = pet.st.hunger;
  needs["thirst"] = pet.st.thirst;
  needs["energy"] = pet.st.energy;
  needs["happiness"] = pet.st.happiness;
  needs["cleanliness"] = pet.st.cleanliness;
  needs["health"] = pet.st.health;
  needs["bonding"] = pet.st.bonding;

  JsonObject brainJson = doc.createNestedObject("brain");
  brainJson["mood"] = pet.st.mood;
  brainJson["boredom"] = pet.st.boredom;
  brainJson["sleepDebt"] = pet.st.sleepDebt;
  brainJson["sleeping"] = pet.st.sleeping;
  brainJson["trust"] = brain.trust;
  brainJson["irritation"] = brain.irritation;
  brainJson["attentionNeed"] = brain.attentionNeed;

  JsonObject habitsNow = doc.createNestedObject("habits_now");
  habitsNow["feed"] = habitsAffinity(ACT_FEED);
  habitsNow["pet"] = habitsAffinity(ACT_PET);
  habitsNow["play"] = habitsAffinity(ACT_PLAY);

  JsonObject stats = doc.createNestedObject("stats");
  stats["happyStreak"] = pet.st.happyStreak;
  stats["bestHappyStreak"] = pet.st.bestHappyStreak;
  stats["feed"] = pet.st.cntFeed;
  stats["drink"] = pet.st.cntDrink;
  stats["play"] = pet.st.cntPlay;
  stats["wash"] = pet.st.cntWash;
  stats["pet"] = pet.st.cntPet;
  stats["med"] = pet.st.cntMed;

  String out;
  serializeJson(doc, out);
  return out;
}

// ================= Handlers =================

static void handleIndex() {
  server.send(200, "text/html; charset=utf-8", HTML_INDEX);
}

static void handleStatus() {
  server.send(200, "application/json", buildStatusJson());
}

static void handleAction() {
  if (!server.hasArg("name")) {
    server.send(400, "text/plain", "missing action");
    return;
  }

  String name = server.arg("name");

  if (name == "feed") actionFeed();
  else if (name == "drink") actionDrink();
  else if (name == "play") actionPlay();
  else if (name == "wash") actionWash();
  else if (name == "pet") actionPet();
  else if (name == "med") actionMedicine();
  else if (name == "sleep") actionSleepToggle();
  else {
    server.send(400, "text/plain", "unknown action");
    return;
  }

  server.send(200, "application/json", buildStatusJson());
}

// ================= Init / Tick =================

void webInit() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);

  Serial.print("[WEB] AP IP: ");
  Serial.println(WiFi.softAPIP());

  server.on("/", handleIndex);
  server.on("/status", handleStatus);
  server.on("/action", handleAction);

  server.begin();
  Serial.println("[WEB] Server started");
}

void webTick() {
  server.handleClient();
}
