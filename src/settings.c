#define WORK_STATE 0
#define RELAX_STATE 1
  
#define RUNNING_EXEC_STATE 0
#define PAUSED_EXEC_STATE 1
  
#define MAX_APP_IDLE (30 * 60)
#define MAX_ITERATION_IDLE (8 * 60 * 60)
#define WORK_DURATION (25 * 60)
#define RELAX_DURATION (5 * 60)
  
#define INCREMENT_TIME 60


#define LAST_TIME_KEY 0
#define STATE_KEY 1
#define CURRENT_DURATION_KEY 2
#define ITERATION_KEY 3

#define LAST_TIME_DEFAULT 0
#define STATE_DEFAULT 0
#define ITERATION_DEFAULT 0

#include "pebble.h"

typedef struct TomatoSettings {
  int last_time;
  int state;
  int current_duration;
  int iteration;
} TomatoSettings;

static int32_t read_int(const uint32_t key, int32_t def_value) {
  if (persist_exists(key)) {
    return persist_read_int(key);
  } else {
    return def_value;
  }
}

static TomatoSettings get_default_settings() {
  TomatoSettings default_settings = {
    time(NULL),
    STATE_DEFAULT,
    WORK_DURATION,
    ITERATION_DEFAULT
  };
  
  return default_settings;
}

static TomatoSettings read_settings() {
  TomatoSettings default_settings = get_default_settings();
  
  TomatoSettings settings = {
    read_int(LAST_TIME_KEY, default_settings.last_time),
    read_int(STATE_KEY, default_settings.state),
    read_int(CURRENT_DURATION_KEY, default_settings.current_duration),
    read_int(ITERATION_KEY, default_settings.iteration)
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

static void save_settings(TomatoSettings settings) {
  persist_write_int(LAST_TIME_KEY, settings.last_time);
  persist_write_int(STATE_KEY, settings.state);
  persist_write_int(CURRENT_DURATION_KEY, settings.current_duration);
  persist_write_int(ITERATION_KEY, settings.iteration);
}