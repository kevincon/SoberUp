#pragma once
#define MAX_MESSAGE_COLUMN 3
#define TICKS_PER_ROTATION 4

static const char const *kMessages[9][MAX_MESSAGE_COLUMN] = {
  {"",                 "",                 ""},
  {"Mild euphoria",    "Relaxation",       "Joyousness"},
  {"Blunted feelings", "Disinhibition",    "Extroversion"},
  {"Over-expression",  "Anger/sadness",    "Decreased libido"},
  {"Stupor",           "Motor impairment", "Memory blackout"},
  {"Unconsciousness",  "Possible death",   "CNS depression"},
  {"Lack of behavior", "Unconsciousness",  "Possible death"},
  {"Poisoning",        "Death",            "Call 911"},
  {"...",              "...",              "..."}
};

static int ebac_to_effect_tier(const float ebac) {
  if (ebac < 0.03) { return 0; }
  else if (ebac < 0.06) { return 1; }
  else if (ebac < 0.09) { return 2; }
  else if (ebac < 0.2) { return 3; }
  else if (ebac < 0.3) { return 4; }
  else if (ebac < 0.4) { return 5; }
  else if (ebac < 0.5) { return 6; }
  else if (ebac < 0.75) { return 7; }
  return 8;
}

static char const *get_effect_message(const float ebac, const uint32_t timer_tick) {
  const int row = ebac_to_effect_tier(ebac);
  const int col = (timer_tick % (MAX_MESSAGE_COLUMN * TICKS_PER_ROTATION)) / TICKS_PER_ROTATION;
  return kMessages[row][col];
}
