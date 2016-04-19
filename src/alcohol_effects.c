#include "alcohol_effects.h"

#include "util.h"

static const char *s_alochol_effects_strings[][ALCOHOL_EFFECTS_STRING_VARIATIONS_COUNT] = {
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

static size_t prv_effect_tier_index_from_ebac(const float ebac) {
  if (ebac < 0.03) {
    return 0;
  } else if (ebac < 0.06) {
    return 1;
  } else if (ebac < 0.09) {
    return 2;
  } else if (ebac < 0.2) {
    return 3;
  } else if (ebac < 0.3) {
    return 4;
  } else if (ebac < 0.4) {
    return 5;
  } else if (ebac < 0.5) {
    return 6;
  } else if (ebac < 0.75) {
    return 7;
  } else {
    return 8;
  }
}

const char *alcohol_effects_get_effect_string_for_ebac(float ebac, size_t variation_index) {
  variation_index = CLIP_MAX(variation_index, ALCOHOL_EFFECTS_STRING_VARIATIONS_COUNT - 1);

  const size_t effect_tier_index = prv_effect_tier_index_from_ebac(ebac);
  return s_alochol_effects_strings[effect_tier_index][variation_index];
}
