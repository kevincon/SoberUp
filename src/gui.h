#pragma once
#include "pebble.h"

void gui_update_ebac(char *ebac_text);
void gui_update_countdown(char *countdown_text);
void gui_update_drink_counter(char *drink_counter_text);
void gui_update_alcohol_effects(const char *alcohol_effects_text);

void gui_setup_buttons(ClickConfigProvider click_config);

void gui_show_alert();
void gui_hide_alert();

void gui_init();
void gui_destroy();
