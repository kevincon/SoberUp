#define MAX_MESSAGE_COLUMN 3
#define TICKS_PER_ROTATION 10

static const char const* kMessages[8][3] = {
  {"0,0", "0,1", "0,2"},
  {"1,0", "1,1", "1,2"},
  {"2,0", "2,1", "2,2"},
  {"3,0", "3,1", "3,2"},
  {"4,0", "4,1", "4,2"},
  {"5,0", "5,1", "5,2"},
  {"6,0", "6,1", "6,2"},
  {"Wearing a watch! :)", "Wearing a watch! :)", "Wearing a watch! :)"}
};

static int ebac_to_effect_tier(const float ebac) {
  // TODO(ebensh): This is a lot of float comparisons and we could drop
  // the first value in each half-open interval to save processing time.
  if (ebac < 0.03) { return 0; }
  else if (ebac < 0.06) { return 1; }
  else if (ebac < 0.09) { return 2; }
  else if (ebac < 0.2) { return 3; }
  else if (ebac < 0.3) { return 4; }
  else if (ebac < 0.4) { return 5; }
  else if (ebac < 0.5) { return 6; }
  return 7;
}

static char const* get_effect_message(const float ebac, const uint32_t timer_tick) {
  const int row = ebac_to_effect_tier(ebac);
  const int col = (timer_tick % (MAX_MESSAGE_COLUMN * TICKS_PER_ROTATION)) / TICKS_PER_ROTATION;
  return kMessages[row][col];
}
