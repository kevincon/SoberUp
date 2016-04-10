#include "gui.h"
#include "alert.h"

static Window *window;

/* Action bar */
static ActionBarLayer *action_bar;
static GBitmap *action_icon_plus;
static GBitmap *action_icon_minus;

/* Top bar */
static Layer *top_bar;
static TextLayer *header_text_layer;
static BitmapLayer *beer_layer;
static GBitmap *menu_icon_beer;
static TextLayer *drink_counter_text_layer;

/* Body */
static TextLayer *body_text_layer;
static TextLayer *label_text_layer;
static TextLayer *effects_text_layer;

/* Bottom bar */
static Layer *bottom_bar;
static BitmapLayer *stopwatch_layer;
static GBitmap *bottom_icon_stopwatch;
static TextLayer *countdown_label_layer;
static TextLayer *countdown_text_layer;

static void load_action_bar() {
  action_bar = action_bar_layer_create();
  action_bar_layer_add_to_window(action_bar, window);

  action_icon_plus = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_ACTION_ICON_PLUS);
  action_icon_minus = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_ACTION_ICON_MINUS);
}

static void unload_action_bar() {
  gbitmap_destroy(action_icon_plus);
  gbitmap_destroy(action_icon_minus);

  action_bar_layer_destroy(action_bar);
}

static void load_top_bar() {
  Layer *root_layer = window_get_root_layer(window);
  const int16_t width = layer_get_frame(root_layer).size.w - ACTION_BAR_WIDTH - 6;

  top_bar = layer_create(GRect(4, 10, width, 28));
  layer_add_child(root_layer, top_bar);

  header_text_layer = text_layer_create(GRect(0, 0, 80, 56));
  text_layer_set_font(header_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24));
  text_layer_set_background_color(header_text_layer, GColorClear);
  text_layer_set_text(header_text_layer, "Std. drinks:");
  layer_add_child(top_bar, text_layer_get_layer(header_text_layer));

  menu_icon_beer = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_MENU_ICON_BEER);
  const GRect menu_icon_beer_bounds = gbitmap_get_bounds(menu_icon_beer);
  const int16_t beer_width = menu_icon_beer_bounds.size.w;
  const int16_t beer_height = menu_icon_beer_bounds.size.h;
  beer_layer = bitmap_layer_create(GRect(width - beer_width, 0, beer_width, beer_height));
  bitmap_layer_set_bitmap(beer_layer, menu_icon_beer);
  bitmap_layer_set_alignment(beer_layer, GAlignCenter);

  drink_counter_text_layer = text_layer_create(GRect(width - beer_width - 30, 0, 28, 38));
  text_layer_set_font(drink_counter_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24));
  text_layer_set_background_color(drink_counter_text_layer, GColorClear);
  text_layer_set_text(drink_counter_text_layer, "0");
  text_layer_set_text_alignment(drink_counter_text_layer, GTextAlignmentRight);
  layer_add_child(top_bar, text_layer_get_layer(drink_counter_text_layer));

  layer_add_child(top_bar, bitmap_layer_get_layer(beer_layer));
}

static void unload_top_bar() {
  gbitmap_destroy(menu_icon_beer);
  bitmap_layer_destroy(beer_layer);
  text_layer_destroy(header_text_layer);
  text_layer_destroy(drink_counter_text_layer);
  layer_destroy(top_bar);
}

static void load_body() {
  Layer *root_layer = window_get_root_layer(window);
  const int16_t width = layer_get_frame(root_layer).size.w - ACTION_BAR_WIDTH - 3;

  body_text_layer = text_layer_create(GRect(2, 13 + 35, width, 60));
  text_layer_set_font(body_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_background_color(body_text_layer, GColorClear);
  text_layer_set_text_alignment(body_text_layer, GTextAlignmentCenter);
  layer_add_child(root_layer, text_layer_get_layer(body_text_layer));

  label_text_layer = text_layer_create(GRect(2, 13 + 35 + 28, width, 60));
  text_layer_set_font(label_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_background_color(label_text_layer, GColorClear);
  text_layer_set_text_alignment(label_text_layer, GTextAlignmentCenter);
  text_layer_set_text(label_text_layer, "(Estimated BAC)");
  layer_add_child(root_layer, text_layer_get_layer(label_text_layer));

  effects_text_layer = text_layer_create(GRect(2, 13 + 35 + 28 + 12, width, 60));
  text_layer_set_font(effects_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_background_color(effects_text_layer, GColorClear);
  text_layer_set_text_alignment(effects_text_layer, GTextAlignmentCenter);
  layer_add_child(root_layer, text_layer_get_layer(effects_text_layer));
}

static void unload_body() {
  text_layer_destroy(body_text_layer);
  text_layer_destroy(label_text_layer);
  text_layer_destroy(effects_text_layer);
}

static void load_bottom_bar() {
  Layer *root_layer = window_get_root_layer(window);
  const int16_t width = layer_get_frame(root_layer).size.w - ACTION_BAR_WIDTH - 4;
  const int16_t height = layer_get_frame(root_layer).size.h;

  bottom_icon_stopwatch = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BOTTOM_ICON_STOPWATCH);
  const GRect bottom_icon_stopwatch_bounds = gbitmap_get_bounds(bottom_icon_stopwatch);
  const int16_t stopwatch_width = bottom_icon_stopwatch_bounds.size.w;
  const int16_t stopwatch_height = bottom_icon_stopwatch_bounds.size.h;

  bottom_bar = layer_create(GRect(0, height - stopwatch_height - 3, width, stopwatch_height));
  layer_add_child(root_layer, bottom_bar);

  stopwatch_layer = bitmap_layer_create(GRect(2, 0, stopwatch_width, stopwatch_height));
  bitmap_layer_set_bitmap(stopwatch_layer, bottom_icon_stopwatch);
  bitmap_layer_set_alignment(stopwatch_layer, GAlignCenter);
  layer_add_child(bottom_bar, bitmap_layer_get_layer(stopwatch_layer));

  countdown_label_layer = text_layer_create(GRect(27, 0, width - 27, stopwatch_height));
  text_layer_set_font(countdown_label_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_background_color(countdown_label_layer, GColorClear);
  text_layer_set_text(countdown_label_layer, "Est. time to 0 BAC:");
  text_layer_set_text_alignment(countdown_label_layer, GTextAlignmentCenter);
  layer_add_child(bottom_bar, text_layer_get_layer(countdown_label_layer));

  countdown_text_layer = text_layer_create(GRect(30, 9, width - 30, stopwatch_height));
  text_layer_set_font(countdown_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_background_color(countdown_text_layer, GColorClear);
  text_layer_set_text(countdown_text_layer, "OO H 00 M");
  text_layer_set_text_alignment(countdown_text_layer, GTextAlignmentCenter);
  layer_add_child(bottom_bar, text_layer_get_layer(countdown_text_layer));
}

static void unload_bottom_bar() {
  text_layer_destroy(countdown_label_layer);
  text_layer_destroy(countdown_text_layer);
  bitmap_layer_destroy(stopwatch_layer);
  layer_destroy(bottom_bar);
}

void window_load(Window *me) {
  load_action_bar();
  load_top_bar();
  load_body();
  load_bottom_bar();
}

void window_unload(Window *window) {
  unload_action_bar();
  unload_top_bar();
  unload_body();
  unload_bottom_bar();
  gui_hide_alert();
}

void gui_update_ebac(char *ebac_text) {
  text_layer_set_text(body_text_layer, ebac_text);
}

void gui_update_countdown(char *countdown_text) {
  text_layer_set_text(countdown_text_layer, countdown_text);
}

void gui_update_drink_counter(char *drink_counter_text) {
  text_layer_set_text(drink_counter_text_layer, drink_counter_text);
}

void gui_update_alcohol_effects(const char *alcohol_effects_text) {
  text_layer_set_text(effects_text_layer, alcohol_effects_text);
}

void gui_setup_buttons(ClickConfigProvider click_config) {
  action_bar_layer_set_icon(action_bar, BUTTON_ID_UP, action_icon_plus);
  action_bar_layer_set_icon(action_bar, BUTTON_ID_DOWN, action_icon_minus);
  action_bar_layer_set_click_config_provider(action_bar, click_config);
}

void gui_show_alert() {
  alert_show(window, "Alert", "Please configure SoberUp settings on your phone.", 0);
}

void gui_hide_alert() {
  alert_cancel();
}

void gui_init() {
  window = window_create();
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });

  window_stack_push(window, true /* Animated */);
}

void gui_destroy() {
  window_destroy(window);

  gbitmap_destroy(bottom_icon_stopwatch);
}