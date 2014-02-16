#include "pebble.h"
	
#define DEBUG
#ifdef DEBUG
#define SECONDS_IN_HOUR 20.0
#else
#define SECONDS_IN_HOUR 3600.0
#endif

// These #defines should be replaced by inputs from the js app.
#define BODY_WATER 0.58
#define METABOLISM 0.017
#define WEIGHT_KGS 72.0

// This is a custom defined key for saving our count field
#define NUM_DRINKS_PKEY 1
#define START_TIME_PKEY 2

#define NUM_DRINKS_DEFAULT 0
#define START_TIME_DEFAULT 0
#define TIME_ELAPSED_DEFAULT 0

static Window *window;

static GBitmap *action_icon_plus;
static GBitmap *action_icon_minus;
static GBitmap *action_icon_car;

static ActionBarLayer *action_bar;

static TextLayer *header_text_layer;
static TextLayer *body_text_layer;
static TextLayer *label_text_layer;

static Layer *bottom_bar;

static BitmapLayer *car_layer;

// We'll save the count in memory from persistent storage
static int num_drinks = NUM_DRINKS_DEFAULT;
static time_t start_time = START_TIME_DEFAULT;
static uint32_t time_elapsed = TIME_ELAPSED_DEFAULT;

static char ebac_str[10];
static char body_text[50];

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
  start_time = time(NULL);
  tick_timer_service_subscribe(SECOND_UNIT, timer_handler);
}

static void stop_counting() {
  tick_timer_service_unsubscribe();
  num_drinks = 0;
  start_time = 0;
  time_elapsed = 0;
}

static float get_ebac(const float body_water,
                      const float metabolism,
                      const float weight_kgs,
                      const float standard_drinks,
                      const double drinking_secs) {
  if (standard_drinks <= 0) { return 0.0; }
  if (drinking_secs <= 0) { return 0.0; }
  // Pebble y u no have maximum?!
  float ebac = ((0.806 * standard_drinks * 1.2) / (body_water * weight_kgs)) - (metabolism * (drinking_secs / SECONDS_IN_HOUR));
  if (ebac <= 0.0) {
	stop_counting();
    return 0.0;
  }
  return ebac;
}

static void update_text() {
  const float ebac = get_ebac(BODY_WATER, METABOLISM, WEIGHT_KGS, num_drinks, time_elapsed);
  floatToString(ebac_str, sizeof(ebac_str), ebac);
  snprintf(body_text, sizeof(body_text), "%s EBAC", ebac_str);
  
  text_layer_set_text(body_text_layer, body_text);
}

static void timer_handler(struct tm *tick_time, TimeUnits units_changed) {
  if (start_time > 0) {
    time_elapsed = (uint32_t)(time(NULL) - start_time);
	  //uint32_t timediff = (uint32_t)(time(NULL) - start_time);
    //APP_LOG(APP_LOG_LEVEL_DEBUG, "Elapsed time: %lu", timediff);
	//time_elapsed = timediff;
  }
  update_text();
}

static void increment_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (num_drinks++ == 0) {
    start_counting();
  }
  update_text();
}

static void decrement_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (--num_drinks <= 0) {
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
  const int16_t width = layer_get_frame(layer).size.w - ACTION_BAR_WIDTH - 3;

  header_text_layer = text_layer_create(GRect(4, 0, width, 60));
  text_layer_set_font(header_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24));
  text_layer_set_background_color(header_text_layer, GColorClear);
  text_layer_set_text(header_text_layer, "EBAC Calculator");
  layer_add_child(layer, text_layer_get_layer(header_text_layer));

  body_text_layer = text_layer_create(GRect(4, 44, width, 60));
  text_layer_set_font(body_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_background_color(body_text_layer, GColorClear);
  layer_add_child(layer, text_layer_get_layer(body_text_layer));

  label_text_layer = text_layer_create(GRect(4, 44 + 28, width, 60));
  text_layer_set_font(label_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_background_color(label_text_layer, GColorClear);
  text_layer_set_text(label_text_layer, "* Estimate *");
  layer_add_child(layer, text_layer_get_layer(label_text_layer));
	
  update_text();
	
  // draw bottom bar (car)
  GRect bounds = layer_get_bounds(layer);
  bottom_bar = layer_create(GRect(0, bounds.size.h - 26, bounds.size.w, bounds.size.h));
  layer_add_child(layer, bottom_bar);
							
  car_layer = bitmap_layer_create(GRect(4, 0, 26, 22));
  bitmap_layer_set_bitmap(car_layer, action_icon_car);
  bitmap_layer_set_alignment(car_layer, GAlignCenter);
  layer_add_child(bottom_bar, bitmap_layer_get_layer(car_layer));
}

static void window_unload(Window *window) {
  text_layer_destroy(header_text_layer);
  text_layer_destroy(body_text_layer);
  text_layer_destroy(label_text_layer);

  action_bar_layer_destroy(action_bar);
  bitmap_layer_destroy(car_layer);
  layer_destroy(bottom_bar);
}

static void init(void) {
  action_icon_plus = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_ACTION_ICON_PLUS);
  action_icon_minus = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_ACTION_ICON_MINUS);
  action_icon_car = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_ACTION_ICON_CAR);
	
  window = window_create();
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
	
  //tick_timer_service_subscribe(SECOND_UNIT, timer_handler);

  window_stack_push(window, true /* Animated */);

  // Get the count from persistent storage for use if it exists, otherwise use the default
  // TODO(ebensh): Uncomment this.
  stop_counting();  // Reset our counters to a start state.
  num_drinks = persist_exists(NUM_DRINKS_PKEY) ? persist_read_int(NUM_DRINKS_PKEY) : NUM_DRINKS_DEFAULT;
  start_time = persist_exists(START_TIME_PKEY) ? persist_read_int(START_TIME_PKEY) : START_TIME_DEFAULT;
  time_t raw_time = time(NULL);
  timer_handler(gmtime(&raw_time), SECOND_UNIT);
}

static void deinit(void) {
  // Save the count into persistent storage on app exit
  persist_write_int(NUM_DRINKS_PKEY, num_drinks);
  persist_write_int(START_TIME_PKEY, start_time);

  window_destroy(window);

  gbitmap_destroy(action_icon_plus);
  gbitmap_destroy(action_icon_minus);
  gbitmap_destroy(action_icon_car);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
