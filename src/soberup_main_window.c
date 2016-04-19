#include "soberup_main_window.h"

#include "ebac.h"
#include "soberup_configuration.h"
#include "util.h"

#define MAX_DRINKS_COUNTED (99)

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
    char drink_counter_text_buffer[50];
    BitmapLayer *beer_icon_layer;
    GBitmap *beer_icon;
  } top_area;

  struct {
    TextLayer *ebac_text_layer;
    TextLayer *ebac_subtitle_label_text_layer;
    TextLayer *effects_text_layer;
  } body;

  struct {
    BitmapLayer *stopwatch_icon_layer;
    GBitmap *stopwatch_icon;
    TextLayer *countdown_label_text_layer;
    TextLayer *countdown_text_layer;
  } bottom_area;
} SoberUpMainWindow;

static void prv_update_text_layers(SoberUpMainWindow *data) {
  SoberUpConfiguration *configuration = soberup_configuration_get_configuration();
  // TODO extract the 50
  snprintf(data->top_area.drink_counter_text_buffer, 50, "Std. drinks:   %02d",
           configuration->drinking_state.num_drinks);
  text_layer_set_text(data->top_area.drink_counter_text_layer,
                      data->top_area.drink_counter_text_buffer);
//  const float ebac;
//  floatToString(ebac_str, sizeof(ebac_str), ebac);
//  snprintf(body_text, sizeof(body_text), "%s eBAC", ebac_str);
//
//  //update_countdown
//  const float dp = get_dp(ebac);
//  if (dp == 0.0) {
//    snprintf(countdown_text, sizeof(countdown_text), "OO H 00 M");
//  } else {
//    int dp_h = (int) dp;
//    snprintf(countdown_text, sizeof(countdown_text), "%02d H %02d M", dp_h, (int)((dp - dp_h)*60));
//  }
//
//  snprintf(drink_counter_text, sizeof(drink_counter_text), "%d", (int)drinking_state.num_drinks);
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
//
//  BitmapLayer *beer_icon_layer;
//  GBitmap *beer_icon;

  prv_update_text_layers(data);
}

static void prv_window_unload(Window *window) {
  SoberUpMainWindow *data = window_get_user_data(window);

  tick_timer_service_unsubscribe();

  if (data) {
    status_bar_layer_destroy(data->top_area.status_bar_layer);
    text_layer_destroy(data->top_area.drink_counter_text_layer);

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
