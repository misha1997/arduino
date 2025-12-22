#include "storage.h"
#include <Preferences.h>
#include "pet_state.h"
#include "pet_ai.h"

static Preferences prefs;

void storageInit() {
  prefs.begin("pet", false);
}

void storageSave() {
  prefs.putBytes("pet", &pet, sizeof(pet));
  prefs.putBytes("brain", &brain, sizeof(brain));
  prefs.putBytes("habits", &habits, sizeof(habits));
  prefs.putBytes("ai", &ai, sizeof(ai));
}

void storageLoad() {
  if (prefs.isKey("pet")) {
    prefs.getBytes("pet", &pet, sizeof(pet));
    prefs.getBytes("brain", &brain, sizeof(brain));
    prefs.getBytes("habits", &habits, sizeof(habits));
    prefs.getBytes("ai", &ai, sizeof(ai));
  }
}
