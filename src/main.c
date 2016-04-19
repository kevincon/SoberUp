#include "soberup_main_window.h"

#include <pebble.h>

#include "alcohol_effects.h"
#include "soberup_configuration.h"
#include "util.h"

static void init(void) {
  soberup_configuration_init();

  // TODO Push the EULA window if the EULA hasn't been signed
  soberup_main_window_push();
}

static void deinit(void) {
  soberup_configuration_deinit();
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
