#include "pebble.h"
#include "alcohol_effects.h"
	
#define DEBUG
#ifdef DEBUG
#define SECONDS_IN_HOUR 60.0
#else
#define SECONDS_IN_HOUR 3600.0
#endif

// Keys for loading from appmessage
enum {
  USER_DATA_KEY_GENDER = 0x0,
  USER_DATA_KEY_WEIGHT = 0x1,
};

// This is a custom defined key for saving our state structs
#define DRINKING_STATE_PKEY 3
#define EBAC_PARAMS_PKEY 4

typedef struct DrinkingState {
  int32_t num_drinks;
  time_t start_time;
} DrinkingState;

typedef struct EBACParams {
  float body_water;
  float metabolism;
  float weight_kgs;
} EBACParams;

DrinkingState get_default_drinking_state() {
  DrinkingState state;
  state.num_drinks = 0;
  state.start_time = 0;
  return state;
}

EBACParams get_default_ebac_params() {
  EBACParams params;
  params.body_water = 0.58;
  params.metabolism = 0.017;
  params.weight_kgs = 72.0;
  return params;
}

static Window *window;

static GBitmap *action_icon_plus;
static GBitmap *action_icon_minus;
static GBitmap *action_icon_car;
static GBitmap *action_icon_beer;

static ActionBarLayer *action_bar;

static Layer *top_bar;
static TextLayer *header_text_layer;
static TextLayer *body_text_layer;
static TextLayer *label_text_layer;
static TextLayer *countdown_text_layer;
static TextLayer *drink_counter_text_layer;
static TextLayer *effects_text_layer;

static Layer *bottom_bar;

static BitmapLayer *car_layer;
static BitmapLayer *beer_layer;

// We'll save the count in memory from persistent storage
static DrinkingState drinking_state;
static EBACParams ebac_params;
static uint32_t time_elapsed = 0;

static char ebac_str[10];
static char countdown_text[20];
static char body_text[50];
static char drink_counter_text[4];

static void timer_handler(struct tm *tick_time, TimeUnits units_changed);

// This is from http://forums.getpebble.com/discussion/8280/displaying-the-value-of-a-floating-point
// because Pebble doesn't support %f in snprintf.
static char* floatToString(char* buffer, int bufferSize, double number) {
  char decimalBuffer[7];

  snprintf(buffer, bufferSize, "%d", (int)number);
  strcat(buffer, ".");

  snprintf(decimalBuffer, 7, "%04d", (int)((double)(number - (int)number) * (double)10000));
  strcat(buffer, decimalBuffer);

  return buffer;
}

static void start_counting() {
  time_elapsed = 1;
  drinking_state.start_time = time(NULL);
  tick_timer_service_subscribe(SECOND_UNIT, timer_handler);
}

static void stop_counting() {
  tick_timer_service_unsubscribe();
  drinking_state = get_default_drinking_state();
  time_elapsed = 0;
}

static float get_ebac(const float body_water,
					  const float metabolism,
					  const float weight_kgs,
                      const float standard_drinks,
                      const double drinking_secs) {
  // Pebble y u no have maximum?!
  // TODO remove ebac_params
  float ebac = ((0.806 * standard_drinks * 1.2) / (ebac_params.body_water * ebac_params.weight_kgs)) -
	  (ebac_params.metabolism * (drinking_secs / SECONDS_IN_HOUR));
  if (ebac <= 0.0) {
	stop_counting();
    return 0.0;
  }
  return ebac;
}

static float get_dp(const float current_ebac) {
  float dp = (current_ebac) / ebac_params.metabolism;
  if (dp <= 0.0) {
    return 0.0;
  }
  return dp;
}

static void update_text() {
  const float ebac = get_ebac(ebac_params.body_water, ebac_params.metabolism, ebac_params.weight_kgs, drinking_state.num_drinks, time_elapsed);
  floatToString(ebac_str, sizeof(ebac_str), ebac);
  snprintf(body_text, sizeof(body_text), "%s eBAC", ebac_str);
	
  const float dp = get_dp(ebac);
  if (dp == 0.0) {
	snprintf(countdown_text, sizeof(countdown_text), "OK TO DRIVE");
  } else {
	int dp_h = (int) dp;
    snprintf(countdown_text, sizeof(countdown_text), "%02d H %02d M", dp_h, (int)((dp - dp_h)*60));
  }
  
  snprintf(drink_counter_text, sizeof(drink_counter_text), "%d", (int)drinking_state.num_drinks);
/*
  #ifdef DEBUG
  char body_water_str[10];
  char metabolism_str[10];
  char weight_kgs_str[10];
  floatToString(body_water_str, sizeof(body_water_str), ebac_params.body_water);
  floatToString(metabolism_str, sizeof(metabolism_str), ebac_params.metabolism);
  floatToString(weight_kgs_str, sizeof(weight_kgs_str), ebac_params.weight_kgs);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "ebac: %s, num_drinks: %ld, start_time: %ld",
		  ebac_str, drinking_state.num_drinks, drinking_state.start_time);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "body_water: %s, metabolism: %s, weight_kgs: %s",
		  body_water_str, metabolism_str, weight_kgs_str);
  #endif
*/
  text_layer_set_text(body_text_layer, body_text);
  text_layer_set_text(countdown_text_layer, countdown_text);
  text_layer_set_text(drink_counter_text_layer, drink_counter_text);
  text_layer_set_text(effects_text_layer, get_effect_message(ebac, time_elapsed));
}

static void timer_handler(struct tm *tick_time, TimeUnits units_changed) {
  if (drinking_state.start_time > 0) {
    time_elapsed = (uint32_t)(time(NULL) - drinking_state.start_time);
  }
  update_text();
}

static void increment_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (drinking_state.num_drinks++ == 0) {
    start_counting();
  }
  update_text();
}

static void decrement_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (--drinking_state.num_drinks <= 0) {
	stop_counting();
  }
  update_text();
}

static void click_config_provider(void *context) {
  const uint16_t repeat_interval_ms = 50;
  window_single_repeating_click_subscribe(BUTTON_ID_UP, repeat_interval_ms, (ClickHandler) increment_click_handler);
  window_single_repeating_click_subscribe(BUTTON_ID_DOWN, repeat_interval_ms, (ClickHandler) decrement_click_handler);
}

static void window_load(Window *me) {
  action_bar = action_bar_layer_create();
  action_bar_layer_add_to_window(action_bar, me);
  action_bar_layer_set_click_config_provider(action_bar, click_config_provider);

  action_bar_layer_set_icon(action_bar, BUTTON_ID_UP, action_icon_plus);
  action_bar_layer_set_icon(action_bar, BUTTON_ID_DOWN, action_icon_minus);

  Layer *layer = window_get_root_layer(me);
  GRect bounds = layer_get_bounds(layer);
  const int16_t width = layer_get_frame(layer).size.w - ACTION_BAR_WIDTH - 3;
	
  top_bar = layer_create(GRect(4, 10, width, 28 + 10));
  layer_add_child(layer, top_bar);
  header_text_layer = text_layer_create(GRect(0, 0, 65, 56));
  text_layer_set_font(header_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24));
  text_layer_set_background_color(header_text_layer, GColorClear);
  text_layer_set_text(header_text_layer, "SoberUp");
  layer_add_child(top_bar, text_layer_get_layer(header_text_layer));
  beer_layer = bitmap_layer_create(GRect(width - 52, 0, width - 48, 28));
  bitmap_layer_set_bitmap(beer_layer, action_icon_beer);
  bitmap_layer_set_alignment(beer_layer, GAlignCenter);
	
  drink_counter_text_layer = text_layer_create(GRect(width - 52 - 38, 0, width - 52 - 10, 38));
  text_layer_set_font(drink_counter_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24));
  text_layer_set_background_color(drink_counter_text_layer, GColorClear);
  text_layer_set_text(drink_counter_text_layer, "0");
  text_layer_set_text_alignment(drink_counter_text_layer, GTextAlignmentRight);
  layer_add_child(top_bar, text_layer_get_layer(drink_counter_text_layer));	
	
  layer_add_child(top_bar, bitmap_layer_get_layer(beer_layer));
	
  body_text_layer = text_layer_create(GRect(2, 44, width, 60));
  text_layer_set_font(body_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_background_color(body_text_layer, GColorClear);
  text_layer_set_text_alignment(body_text_layer, GTextAlignmentCenter);
  layer_add_child(layer, text_layer_get_layer(body_text_layer));

  label_text_layer = text_layer_create(GRect(2, 44 + 28, width, 60));
  text_layer_set_font(label_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_background_color(label_text_layer, GColorClear);
  text_layer_set_text_alignment(label_text_layer, GTextAlignmentCenter);
  text_layer_set_text(label_text_layer, "(Estimated BAC)");
  layer_add_child(layer, text_layer_get_layer(label_text_layer));
	
  effects_text_layer = text_layer_create(GRect(2, 44 + 28 + 20, width, 60));
  text_layer_set_font(effects_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_background_color(effects_text_layer, GColorClear);
  text_layer_set_text_alignment(effects_text_layer, GTextAlignmentCenter);
  //text_layer_set_text(effects_text_layer, "12345678901234567890");
  layer_add_child(layer, text_layer_get_layer(effects_text_layer));
	
  // draw bottom bar (car)
  bottom_bar = layer_create(GRect(0, bounds.size.h - 22, bounds.size.w - ACTION_BAR_WIDTH - 4, bounds.size.h));
  layer_add_child(layer, bottom_bar);
							
  car_layer = bitmap_layer_create(GRect(4, 0, 26, 22));
  bitmap_layer_set_bitmap(car_layer, action_icon_car);
  bitmap_layer_set_alignment(car_layer, GAlignCenter);
  layer_add_child(bottom_bar, bitmap_layer_get_layer(car_layer));
	
  countdown_text_layer = text_layer_create(GRect(30, 0, bounds.size.w - ACTION_BAR_WIDTH - 30 - 4, 22));
  text_layer_set_font(countdown_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_background_color(countdown_text_layer, GColorClear);
  text_layer_set_text(countdown_text_layer, "OK TO DRIVE");
  text_layer_set_text_alignment(countdown_text_layer, GTextAlignmentCenter);
  layer_add_child(bottom_bar, text_layer_get_layer(countdown_text_layer));	
	
  update_text();
}

static void window_unload(Window *window) {
  text_layer_destroy(header_text_layer);
  text_layer_destroy(body_text_layer);
  text_layer_destroy(label_text_layer);
  text_layer_destroy(countdown_text_layer);
  text_layer_destroy(drink_counter_text_layer);

  action_bar_layer_destroy(action_bar);
  bitmap_layer_destroy(car_layer);
  bitmap_layer_destroy(beer_layer);
  layer_destroy(bottom_bar);
  layer_destroy(top_bar);
}

static void in_received_handler(DictionaryIterator *iter, void *context) {
  Tuple *user_data_gender_tuple = dict_find(iter, USER_DATA_KEY_GENDER);
  Tuple *user_data_weight_tuple = dict_find(iter, USER_DATA_KEY_WEIGHT);

  if (user_data_gender_tuple) {
	const uint8_t gender = user_data_gender_tuple->value->uint8;
	if (gender == 0) {
	  // Female
	  ebac_params.body_water = 0.49;
	  ebac_params.metabolism = 0.017;
	} else if (gender == 1) {
	  // Male
	  ebac_params.body_water = 0.58;
	  ebac_params.metabolism = 0.015;
	}
  }
  if (user_data_weight_tuple) {
	// Convert user's weight to kg before writing.
	const uint16_t weight_lbs = user_data_weight_tuple->value->uint16;
	ebac_params.weight_kgs = weight_lbs / 2.2;
  }
}

static void app_message_init(void) {
  // Register message handlers
  app_message_register_inbox_received(in_received_handler);
  // Init buffers
  app_message_open(64, 64);
}

static void init(void) {
  action_icon_plus = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_ACTION_ICON_PLUS);
  action_icon_minus = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_ACTION_ICON_MINUS);
  action_icon_car = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_ACTION_ICON_CAR);
  action_icon_beer = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_ACTION_ICON_BEER);
	
  window = window_create();
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
	
  window_stack_push(window, true /* Animated */);

  // Get the count from persistent storage for use if it exists, otherwise use the default
  stop_counting();  // Reset our drinking counters to their default state.
  if (persist_exists(DRINKING_STATE_PKEY)) {
    persist_read_data(DRINKING_STATE_PKEY, &drinking_state, sizeof(DrinkingState));
  } else {
	drinking_state = get_default_drinking_state();
  }
  if (drinking_state.num_drinks <= 0) { stop_counting(); }
  
  if (persist_exists(EBAC_PARAMS_PKEY)) {
    persist_read_data(EBAC_PARAMS_PKEY, &ebac_params, sizeof(EBACParams));
  } else {
	ebac_params = get_default_ebac_params();
  }
  time_t raw_time = time(NULL);
  timer_handler(gmtime(&raw_time), SECOND_UNIT);

  if (drinking_state.num_drinks > 0 && drinking_state.start_time > 0) {
    tick_timer_service_subscribe(SECOND_UNIT, timer_handler);
  }

  app_message_init();
}

static void deinit(void) {
  // Save data into persistent storage on exit.
  persist_write_data(DRINKING_STATE_PKEY, &drinking_state, sizeof(DrinkingState));
  persist_write_data(EBAC_PARAMS_PKEY, &ebac_params, sizeof(EBACParams));

  window_destroy(window);

  gbitmap_destroy(action_icon_plus);
  gbitmap_destroy(action_icon_minus);
  gbitmap_destroy(action_icon_car);
  gbitmap_destroy(action_icon_beer);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
