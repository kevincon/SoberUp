#pragma once

#include <pebble.h>

#include "soberup_units.h"

//! Uses the Widmark formula: https://en.wikipedia.org/wiki/Blood_alcohol_content
float ebac_calculate_using_gender_and_weight(Gender gender, Weight weight, WeightUnits weight_units,
                                             Grams grams_ethanol, time_t drinking_period_seconds);

time_t ebac_drinking_period_seconds_from_ebac_and_gender(float ebac, Gender gender);
