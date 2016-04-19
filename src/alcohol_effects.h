#pragma once

#include <pebble.h>

#define ALCOHOL_EFFECTS_STRING_VARIATIONS_COUNT (3)

const char *alcohol_effects_get_effect_string_for_ebac(float ebac, size_t variation_index);
