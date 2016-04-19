#include "soberup_configuration.h"

#define SOBERUP_CONFIGURATION_CURRENT_VERSION 1

typedef enum {
  SoberUpConfigurationPersistedDataKeys_Version,
  SoberUpConfigurationPersistedDataKeys_Data,

  SoberUpConfigurationPersistedDataKeys_Count
} SoberUpConfigurationPersistedDataKeys;

static SoberUpConfiguration s_configuration;

static bool s_write_protection_enabled = true;

static void prv_clear_all_persisted_data(void) {
  for (uint32_t key = 0; key < SoberUpConfigurationPersistedDataKeys_Count; key++) {
    persist_delete(key);
  }
}

static void prv_write_configuration(void) {
  persist_write_int(SoberUpConfigurationPersistedDataKeys_Version,
                    SOBERUP_CONFIGURATION_CURRENT_VERSION);
  persist_write_data(SoberUpConfigurationPersistedDataKeys_Data, &s_configuration,
                     sizeof(s_configuration));
}

//! Returns true if data was migrated successfully and loaded into the static configuration
static bool prv_migrate_configuration_if_necessary(void) {
  if (!persist_exists(SoberUpConfigurationPersistedDataKeys_Version)) {
    return false;
  }

  const int32_t persisted_version = persist_read_int(SoberUpConfigurationPersistedDataKeys_Version);
  switch (persisted_version) {
    case SOBERUP_CONFIGURATION_CURRENT_VERSION:
      // Valid version
      return false;
      // Add more cases here as versions increment to handle data migration; return true if migrated
    default:
      // Unexpected newer version, just clear all data and start over
      prv_clear_all_persisted_data();
      return false;
  }
}

static void prv_set_default_configuration(void) {
  s_configuration = (SoberUpConfiguration) {
    .gender = Gender_Male,
    .grams_ethanol_in_standard_drink = 14,
    .weight = 180,
    .weight_units = WeightUnits_Pounds,
  };
}

// Public functions
/////////////////////

void soberup_configuration_init(void) {
  if (prv_migrate_configuration_if_necessary()) {
    // Data was migrated and loaded into the static configuration, so we're good to go
    return;
  }

  if (persist_exists(SoberUpConfigurationPersistedDataKeys_Data)) {
    persist_read_data(SoberUpConfigurationPersistedDataKeys_Data, &s_configuration,
                      sizeof(s_configuration));
  } else {
    prv_set_default_configuration();
  }
}

void soberup_configuration_set_write_protection_enabled(bool enabled) {
  s_write_protection_enabled = enabled;
}

SoberUpConfiguration *soberup_configuration_get_configuration(void) {
  return &s_configuration;
}

void soberup_configuration_deinit(void) {
  // Only write back the configuration on deinit if we've disabled write protection
  // We do this so that we can show the user a tooltip on the main window saying that they should
  // configure the app, even if the user already exited the app a previous time without doing so
  if (!s_write_protection_enabled) {
    prv_write_configuration();
  }
}
