#pragma once

#include <pebble.h>

#include "soberup_units.h"

typedef struct DrinkingState {
  unsigned int num_drinks;
  time_t start_time;
} DrinkingState;

typedef struct SoberUpConfiguration {
  bool signed_eula;
  Gender gender;
  Weight weight;
  WeightUnits weight_units;
  Grams grams_ethanol_in_standard_drink;
  DrinkingState drinking_state;
} SoberUpConfiguration;

_Static_assert(sizeof(SoberUpConfiguration) < PERSIST_DATA_MAX_LENGTH, "");

void soberup_configuration_init(void);

SoberUpConfiguration *soberup_configuration_get_configuration(void);

void soberup_configuration_set_write_protection_enabled(bool enabled);

void soberup_configuration_deinit(void);
