#include "settings.h"

const SettingParams pomodoro_duration_params = {
    .default_value = 25,
    .min_value = 1,
    .max_value = 60,
    .title = "Pomodoro Duration",
    .format = "%u min."
};

const SettingParams break_duration_params = {
    .default_value = 5,
    .min_value = 1,
    .max_value = 30,
    .title = "Break Duration",
    .format = "%u min."
};

const SettingParams long_break_enabled_params = {
    .default_value = true,
    .title = "Long Break"
};

const SettingParams long_break_duration_params = {
    .default_value = 15,
    .min_value = 1,
    .max_value = 60,
    .title = "Long Break Duration",
    .format = "%u min."
};

const SettingParams long_break_delay_params = {
    .default_value = 5,
    .min_value = 1,
    .max_value = 10,
    .title = "Long Break Delay",
    .format = "%u"
};

static int32_t read_int(const uint32_t key, int32_t def_value) {
  if (persist_exists(key)) {
    return persist_read_int(key);
  } else {
    return def_value;
  }
}

TomatoSettings get_default_settings() {
  TomatoSettings default_settings = {
    .last_time = time(NULL),
    .state = STATE_DEFAULT,
    .current_duration = pomodoro_duration_params.default_value,
    .iteration = ITERATION_DEFAULT,
    .pomodoro_duration = pomodoro_duration_params.default_value,
    .break_duration = break_duration_params.default_value,
    .long_break_enabled = long_break_enabled_params.default_value,
    .long_break_duration = long_break_duration_params.default_value,
    .long_break_delay = long_break_delay_params.default_value
  };
  
  return default_settings;
}

TomatoSettings read_settings() {
  TomatoSettings default_settings = get_default_settings();
  
  TomatoSettings settings = {
    .last_time = read_int(LAST_TIME_KEY, default_settings.last_time),
    .state = read_int(STATE_KEY, default_settings.state),
    .current_duration = read_int(CURRENT_DURATION_KEY, default_settings.current_duration),
    .iteration = read_int(ITERATION_KEY, default_settings.iteration),
    .pomodoro_duration = read_int(POMODORO_DURATION_KEY, default_settings.pomodoro_duration),
    .break_duration = read_int(BREAK_DURATION_KEY, default_settings.break_duration),
    .long_break_enabled = read_bool(LONG_BREAK_ENABLED_KEY, default_settings.long_break_enabled),
    .long_break_duration = read_int(LONG_BREAK_DURATION_KEY, default_settings.long_break_duration),
    .long_break_delay = read_int(LONG_BREAK_DELAY_KEY, default_settings.long_break_delay),
  };
  
  int time_passed = default_settings.last_time - settings.last_time;

  if (time_passed > MAX_ITERATION_IDLE) {
    settings.iteration = default_settings.iteration;  
  }
  
  if (time_passed > MAX_APP_IDLE) {
    settings.last_time = default_settings.last_time;
    settings.state = default_settings.state;
    settings.current_duration = default_settings.current_duration;
  }
  
  return settings;
}

void save_settings(TomatoSettings settings) {
  persist_write_int(LAST_TIME_KEY, settings.last_time);
  persist_write_int(STATE_KEY, settings.state);
  persist_write_int(CURRENT_DURATION_KEY, settings.current_duration);
  persist_write_int(ITERATION_KEY, settings.iteration);
}