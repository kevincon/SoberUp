#include "soberup_main_window.h"

#include "alcohol_effects.h"
#include "ebac.h"
#include "soberup_configuration.h"
#include "util.h"

#define MAX_DRINKS_COUNTED (99)
#define TEXT_LAYER_STRING_BUFFER_SIZE (50)

typedef struct SoberUpMainWindow {
  Window *window;

  struct {
    ActionBarLayer *layer;
    GBitmap *plus_icon;
    GBitmap *configuration_icon;
    GBitmap *minus_icon;
  } action_bar;

  struct {
    StatusBarLayer *status_bar_layer;
    TextLayer *drink_counter_text_layer;
    char drink_counter_text_buffer[TEXT_LAYER_STRING_BUFFER_SIZE];
    BitmapLayer *beer_icon_layer;
    GBitmap *beer_icon;
  } top_area;

  struct {
    TextLayer *ebac_text_layer;
    char ebac_text_buffer[TEXT_LAYER_STRING_BUFFER_SIZE];
    TextLayer *ebac_subtitle_label_text_layer;
    TextLayer *alcohol_effects_text_layer;
  } body;

  struct {
    BitmapLayer *stopwatch_icon_layer;
    GBitmap *stopwatch_icon;
    TextLayer *countdown_label_text_layer;
    TextLayer *countdown_text_layer;
    char countdown_text_buffer[TEXT_LAYER_STRING_BUFFER_SIZE];
  } bottom_area;
} SoberUpMainWindow;

static void prv_update_text_layers(SoberUpMainWindow *data) {
  SoberUpConfiguration *configuration = soberup_configuration_get_configuration();

  const time_t current_time = time(NULL);

  // Update the drink counter text
  char *drink_counter_text_buffer = data->top_area.drink_counter_text_buffer;
  snprintf(drink_counter_text_buffer, TEXT_LAYER_STRING_BUFFER_SIZE,
           "Std. drinks:   %02d", configuration->drinking_state.num_drinks);
  text_layer_set_text(data->top_area.drink_counter_text_layer,
                      drink_counter_text_buffer);

  // Update the eBAC text
  const bool started_drinking =
    ((configuration->drinking_state.num_drinks > 0) &&
     (configuration->drinking_state.start_time != 0) &&
     (current_time >= configuration->drinking_state.start_time));
  float ebac = 0.0;
  if (started_drinking) {
    const time_t drinking_period_seconds = current_time - configuration->drinking_state.start_time;
    const Grams grams_ethanol_consumed =
      configuration->grams_ethanol_in_standard_drink * configuration->drinking_state.num_drinks;
    ebac = ebac_calculate_using_gender_and_weight(configuration->gender, configuration->weight,
                                                  configuration->weight_units,
                                                  grams_ethanol_consumed,
                                                  drinking_period_seconds);
  }
  char ebac_string[TEXT_LAYER_STRING_BUFFER_SIZE] = {0};
  floatToString(ebac_string, TEXT_LAYER_STRING_BUFFER_SIZE, ebac);
  char *ebac_text_buffer = data->body.ebac_text_buffer;
  snprintf(ebac_text_buffer, TEXT_LAYER_STRING_BUFFER_SIZE, "%s eBAC", ebac_string);
  text_layer_set_text(data->body.ebac_text_layer, ebac_text_buffer);

  // Update the alcohol effects text
  const time_t seconds_per_variation = 3;
  const size_t alcohol_effects_variation_index =
    (size_t)((current_time % (ALCOHOL_EFFECTS_STRING_VARIATIONS_COUNT * seconds_per_variation))
             / seconds_per_variation);
  const char *alcohol_effects_string =
    alcohol_effects_get_effect_string_for_ebac(ebac, alcohol_effects_variation_index);
  text_layer_set_text(data->body.alcohol_effects_text_layer, alcohol_effects_string);

  // Update the countdown text
  const time_t seconds_to_zero_ebac =
    ebac_drinking_period_seconds_to_zero_ebac_from_ebac_and_gender(ebac, configuration->gender);
  char *countdown_text_buffer = data->bottom_area.countdown_text_buffer;
  if (started_drinking) {
    const time_t hours_to_zero_ebac = seconds_to_zero_ebac / SECONDS_PER_HOUR;
    const time_t minutes_on_the_hour_to_zero_ebac =
      (seconds_to_zero_ebac / SECONDS_PER_MINUTE) - (hours_to_zero_ebac * MINUTES_PER_HOUR);
    snprintf(countdown_text_buffer, TEXT_LAYER_STRING_BUFFER_SIZE,
             "%02i H %02i M", (unsigned int)hours_to_zero_ebac,
             (unsigned int)minutes_on_the_hour_to_zero_ebac);
  } else {
    snprintf(countdown_text_buffer, TEXT_LAYER_STRING_BUFFER_SIZE, "00 H 00 M");
  }
  text_layer_set_text(data->bottom_area.countdown_text_layer, countdown_text_buffer);
}

static void prv_tick_timer_service_handler(struct tm *tick_time, TimeUnits units_changed) {
  prv_update_text_layers(window_get_user_data(window_stack_get_top_window()));
}

static void prv_start_drinking(SoberUpConfiguration *configuration) {
  configuration->drinking_state.start_time = time(NULL);
  // TODO could this be triggered less frequently than every second?
  tick_timer_service_subscribe(SECOND_UNIT, prv_tick_timer_service_handler);
}

static void prv_stop_drinking(SoberUpConfiguration *configuration) {
  tick_timer_service_unsubscribe();
  configuration->drinking_state = (DrinkingState) {0};
}

static void prv_up_click_handler(ClickRecognizerRef recognizer, void *context) {
  SoberUpMainWindow *data = context;

  SoberUpConfiguration *configuration = soberup_configuration_get_configuration();

  if (configuration->drinking_state.num_drinks >= MAX_DRINKS_COUNTED) {
    return;
  }

  if (configuration->drinking_state.num_drinks == 0) {
    prv_start_drinking(configuration);
  }

  configuration->drinking_state.num_drinks++;

  prv_update_text_layers(data);
}

static void prv_down_click_handler(ClickRecognizerRef recognizer, void *context) {
  SoberUpMainWindow *data = context;

  SoberUpConfiguration *configuration = soberup_configuration_get_configuration();

  if (configuration->drinking_state.num_drinks <= 0) {
    return;
  }

  configuration->drinking_state.num_drinks--;

  if (configuration->drinking_state.num_drinks == 0) {
    prv_stop_drinking(configuration);
  }

  prv_update_text_layers(data);
}

static void prv_click_config_provider(void *context) {
  const uint16_t repeat_interval_ms = 50;
  window_single_repeating_click_subscribe(BUTTON_ID_UP, repeat_interval_ms,
                                          prv_up_click_handler);
  window_set_click_context(BUTTON_ID_UP, context);
  window_single_repeating_click_subscribe(BUTTON_ID_DOWN, repeat_interval_ms,
                                          prv_down_click_handler);
  window_set_click_context(BUTTON_ID_DOWN, context);
}

static void prv_window_load(Window *window) {
  SoberUpMainWindow *data = window_get_user_data(window);
  if (!data) {
    return;
  }

  Layer *window_root_layer = window_get_root_layer(window);
  const GRect window_root_layer_bounds = layer_get_bounds(window_root_layer);

  data->action_bar.layer = action_bar_layer_create();
  ActionBarLayer *action_bar_layer = data->action_bar.layer;
  action_bar_layer_set_click_config_provider(action_bar_layer, prv_click_config_provider);
  action_bar_layer_set_context(action_bar_layer, data);
  data->action_bar.plus_icon = gbitmap_create_with_resource(RESOURCE_ID_ACTION_BAR_PLUS_ICON);
  action_bar_layer_set_icon_animated(action_bar_layer, BUTTON_ID_UP, data->action_bar.plus_icon,
                                     true);
  data->action_bar.minus_icon = gbitmap_create_with_resource(RESOURCE_ID_ACTION_BAR_MINUS_ICON);
  action_bar_layer_set_icon_animated(action_bar_layer, BUTTON_ID_DOWN, data->action_bar.minus_icon,
                                     true);
  action_bar_layer_add_to_window(action_bar_layer, window);

  data->top_area.status_bar_layer = status_bar_layer_create();
  StatusBarLayer *status_bar_layer = data->top_area.status_bar_layer;
  status_bar_layer_set_colors(status_bar_layer, GColorBlack, GColorWhite);
  layer_add_child(window_root_layer, status_bar_layer_get_layer(status_bar_layer));

  const GRect content_bounds =
    grect_inset(window_root_layer_bounds,
                GEdgeInsets(STATUS_BAR_LAYER_HEIGHT, ACTION_BAR_WIDTH, 0, 0));

  // TODO make this more relative to the overall layout
  const GRect drink_counter_text_layer_frame = (GRect) {
    .origin = content_bounds.origin,
    .size = GSize(content_bounds.size.w, 18),
  };
  data->top_area.drink_counter_text_layer = text_layer_create(drink_counter_text_layer_frame);
  TextLayer *drink_counter_text_layer = data->top_area.drink_counter_text_layer;
  text_layer_set_font(drink_counter_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_background_color(drink_counter_text_layer, GColorClear);
  text_layer_set_text_alignment(drink_counter_text_layer,
                                PBL_IF_RECT_ELSE(GTextAlignmentLeft, GTextAlignmentCenter));
  layer_add_child(window_root_layer, text_layer_get_layer(drink_counter_text_layer));

  data->top_area.beer_icon = gbitmap_create_with_resource(RESOURCE_ID_BEER_ICON);
  GBitmap *beer_icon = data->top_area.beer_icon;
  const GRect beer_icon_bounds = gbitmap_get_bounds(beer_icon);
  const GRect beer_icon_layer_frame = (GRect) {
    .origin = GPoint(content_bounds.size.w - beer_icon_bounds.size.w, content_bounds.origin.y),
    .size = beer_icon_bounds.size,
  };
  data->top_area.beer_icon_layer = bitmap_layer_create(beer_icon_layer_frame);
  BitmapLayer *beer_icon_layer = data->top_area.beer_icon_layer;
  bitmap_layer_set_bitmap(beer_icon_layer, beer_icon);
  layer_add_child(window_root_layer, bitmap_layer_get_layer(beer_icon_layer));

  // TODO make work on both rect and round
  data->body.ebac_text_layer = text_layer_create(GRect(0, 13 + 35, content_bounds.size.w, 60));
  TextLayer *ebac_text_layer = data->body.ebac_text_layer;
  text_layer_set_font(ebac_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_background_color(ebac_text_layer, GColorClear);
  text_layer_set_text_alignment(ebac_text_layer, GTextAlignmentCenter);
  layer_add_child(window_root_layer, text_layer_get_layer(ebac_text_layer));

  // TODO make work on both rect and round
  data->body.ebac_subtitle_label_text_layer = text_layer_create(GRect(0, 13 + 35 + 28,
                                                                      content_bounds.size.w, 60));
  TextLayer *ebac_subtitle_label_text_layer = data->body.ebac_subtitle_label_text_layer;
  text_layer_set_font(ebac_subtitle_label_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_background_color(ebac_subtitle_label_text_layer, GColorClear);
  text_layer_set_text_alignment(ebac_subtitle_label_text_layer, GTextAlignmentCenter);
  text_layer_set_text(ebac_subtitle_label_text_layer, "(Estimated BAC)");
  layer_add_child(window_root_layer, text_layer_get_layer(ebac_subtitle_label_text_layer));

  // TODO make work on both rect and round
  data->body.alcohol_effects_text_layer = text_layer_create(GRect(0, 13 + 35 + 28 + 12,
                                                                  content_bounds.size.w, 60));
  TextLayer *alcohol_effects_text_layer = data->body.alcohol_effects_text_layer;
  text_layer_set_font(alcohol_effects_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_background_color(alcohol_effects_text_layer, GColorClear);
  text_layer_set_text_alignment(alcohol_effects_text_layer, GTextAlignmentCenter);
  layer_add_child(window_root_layer, text_layer_get_layer(alcohol_effects_text_layer));

  // TODO make work on both rect and round
  data->bottom_area.countdown_label_text_layer = text_layer_create(
    GRect(0, content_bounds.size.h - 27 - 3, content_bounds.size.w, 27));
  TextLayer *countdown_label_text_layer = data->bottom_area.countdown_label_text_layer;
  text_layer_set_font(countdown_label_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_background_color(countdown_label_text_layer, GColorClear);
  text_layer_set_text(countdown_label_text_layer, "Est. time to 0 BAC:");
  text_layer_set_text_alignment(countdown_label_text_layer, GTextAlignmentCenter);
  layer_add_child(window_root_layer, text_layer_get_layer(countdown_label_text_layer));

  // TODO make work on both rect and round
  data->bottom_area.countdown_text_layer = text_layer_create(
    GRect(0, content_bounds.size.h - 27 - 3 + 9, content_bounds.size.w, 27));
  TextLayer *countdown_text_layer = data->bottom_area.countdown_text_layer;
  text_layer_set_font(countdown_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_background_color(countdown_text_layer, GColorClear);
  text_layer_set_text(countdown_text_layer, "OO H 00 M");
  text_layer_set_text_alignment(countdown_text_layer, GTextAlignmentCenter);
  layer_add_child(window_root_layer, text_layer_get_layer(countdown_text_layer));

  prv_update_text_layers(data);
}

static void prv_window_unload(Window *window) {
  SoberUpMainWindow *data = window_get_user_data(window);

  tick_timer_service_unsubscribe();

  if (data) {
    status_bar_layer_destroy(data->top_area.status_bar_layer);
    text_layer_destroy(data->top_area.drink_counter_text_layer);
    // TODO destroy remaining text layers, bitmap layers, and bitmaps

    gbitmap_destroy(data->action_bar.plus_icon);
    gbitmap_destroy(data->action_bar.minus_icon);
    action_bar_layer_destroy(data->action_bar.layer);

    window_destroy(data->window);
  }

  free(data);
}

void soberup_main_window_push(void) {
  SoberUpMainWindow *data = calloc(1, sizeof(*data));
  if (!data) {
    return;
  }

  data->window = window_create();
  Window *window = data->window;

  window_set_window_handlers(window, (WindowHandlers) {
    .load = prv_window_load,
    .unload = prv_window_unload,
  });
  window_set_user_data(window, data);

  const bool animated = true;
  window_stack_push(window, animated);
}
