#include <pebble.h>
#include "alcohol_effects.h"
#include "gui.h"
#include "utils.h"

//#define DEBUG

#ifdef DEBUG
    #define SECONDS_IN_HOUR 60.0
#else
    #define SECONDS_IN_HOUR 3600.0
#endif

// Keys for loading from appmessage
enum {
    USER_DATA_KEY_GENDER = 0x0,
    USER_DATA_KEY_WEIGHT = 0x1,
    USER_DATA_KEY_SIGNED_EULA = 0x2
};

// This is a custom defined key for saving our state structs
#define DRINKING_STATE_PKEY 3
#define EBAC_PARAMS_PKEY 4
#define SIGNED_EULA_PKEY 5

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

// We'll save the count in memory from persistent storage
static DrinkingState drinking_state;
static EBACParams ebac_params;
static bool signed_eula = false;
static uint32_t time_elapsed = 0;

static char ebac_str[10];
static char countdown_text[20];
static char body_text[50];
static char drink_counter_text[4];

static void timer_handler(struct tm *tick_time, TimeUnits units_changed);

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

    if (ebac > 1.0) {
        return 1.0;
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
    //update_ebac
    const float ebac = get_ebac(ebac_params.body_water, ebac_params.metabolism, ebac_params.weight_kgs, drinking_state.num_drinks, time_elapsed);
    floatToString(ebac_str, sizeof(ebac_str), ebac);
    snprintf(body_text, sizeof(body_text), "%s eBAC", ebac_str);

    //update_countdown
    const float dp = get_dp(ebac);
    if (dp == 0.0) {
        snprintf(countdown_text, sizeof(countdown_text), "OO H 00 M");
    } else {
        int dp_h = (int) dp;
        snprintf(countdown_text, sizeof(countdown_text), "%02d H %02d M", dp_h, (int)((dp - dp_h)*60));
    }

    snprintf(drink_counter_text, sizeof(drink_counter_text), "%d", (int)drinking_state.num_drinks);

    gui_update_ebac(body_text);
    gui_update_countdown(countdown_text);
    gui_update_drink_counter(drink_counter_text);
    gui_update_alcohol_effects(get_effect_message(ebac, time_elapsed));
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

    if (drinking_state.num_drinks > 99) {
        drinking_state.num_drinks = 99;
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

static void in_received_handler(DictionaryIterator *iter, void *context) {
    Tuple *user_data_gender_tuple = dict_find(iter, USER_DATA_KEY_GENDER);
    Tuple *user_data_weight_tuple = dict_find(iter, USER_DATA_KEY_WEIGHT);
    Tuple *user_data_signed_eula_tuple = dict_find(iter, USER_DATA_KEY_SIGNED_EULA);

    if (user_data_signed_eula_tuple) {
        signed_eula = (bool) user_data_signed_eula_tuple->value->uint8;
        persist_write_data(SIGNED_EULA_PKEY, &signed_eula, sizeof(signed_eula));
        if (signed_eula) {
            gui_hide_alert();
            gui_setup_buttons(click_config_provider);
        } else {
            gui_show_alert();
            gui_disable_buttons();
        }
    }

    if (user_data_gender_tuple) {
        uint8_t gender = (strncmp(user_data_gender_tuple->value->cstring, "female", user_data_gender_tuple->length) == 0) ? 1 : 0;
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
    gui_init();

    if (persist_exists(SIGNED_EULA_PKEY)) {
        persist_read_data(SIGNED_EULA_PKEY, &signed_eula, sizeof(signed_eula));
    } else {
        signed_eula = false;
    }

    if (!signed_eula) {
        gui_show_alert();
    } else {
        gui_setup_buttons(click_config_provider);
    }

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

    update_text();
}

static void deinit(void) {
    // Save data into persistent storage on exit.
    persist_write_data(DRINKING_STATE_PKEY, &drinking_state, sizeof(DrinkingState));
    persist_write_data(EBAC_PARAMS_PKEY, &ebac_params, sizeof(EBACParams));
    persist_write_data(SIGNED_EULA_PKEY, &signed_eula, sizeof(signed_eula));

    gui_destroy();
}

int main(void) {
    init();
    app_event_loop();
    deinit();
}
