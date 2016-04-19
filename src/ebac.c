#include "ebac.h"

#include "util.h"

#define POUNDS_PER_KILOGRAM (2.2)

static float prv_ebac_calculate(float body_water, float metabolism, Weight weight_kgs,
                                Grams grams_ethanol, time_t drinking_period_hours) {
  const float mean_body_water_in_blood = 0.806;

  // This formula uses an assumption that 1 standard drink is 17 grams of ethanol
  const float standard_drinks = (float)(grams_ethanol / 17.0);
  const float ethanol_amount_in_swedish_units = (float)(standard_drinks * 1.2);

  const float ebac =
    ((mean_body_water_in_blood * ethanol_amount_in_swedish_units) / (body_water * weight_kgs)) -
      (metabolism * drinking_period_hours);

  return (float)CLIP(ebac, 0.0, 1.0);
}

static float prv_metabolism_from_gender(Gender gender) {
  return (float)((gender == Gender_Male) ? 0.015 : 0.017);
}

float ebac_calculate_using_gender_and_weight(Gender gender, Weight weight, WeightUnits weight_units,
                                             Grams grams_ethanol, time_t drinking_period_seconds) {
  const Weight weight_kgs = (Weight)((weight_units == WeightUnits_Pounds) ?
                              (weight / POUNDS_PER_KILOGRAM) : weight);
  // These constants are from https://en.wikipedia.org/wiki/Blood_alcohol_content
  const float body_water = (float)((gender == Gender_Male) ? 0.58 : 0.49);
  const float metabolism = prv_metabolism_from_gender(gender);

  const time_t drinking_period_hours = drinking_period_seconds / SECONDS_PER_HOUR;

  return prv_ebac_calculate(body_water, metabolism, weight_kgs, grams_ethanol,
                            drinking_period_hours);
}

time_t ebac_drinking_period_seconds_from_ebac_and_gender(float ebac, Gender gender) {
  const float metabolism = prv_metabolism_from_gender(gender);
  float drinking_period_hours = ebac / metabolism;
  drinking_period_hours = (float)CLIP_MIN(drinking_period_hours, 0.0);

  return (time_t)(drinking_period_hours * SECONDS_PER_HOUR);
}
