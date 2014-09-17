#define TRUE 1
#define FALSE 0

#define WORK_STATE 0
#define RELAX_STATE 1
  
#define RUNNING_EXEC_STATE 0
#define PAUSED_EXEC_STATE 1
  
#define WORK_DURATION (25 * 60)
#define RELAX_DURATION (5 * 60)
  
#define INCREMENT_TIME 60


#define LAST_TIME_KEY 0
#define STATE_KEY 1
#define CURRENT_DURATION_KEY 2

#define LAST_TIME_DEFAULT 0
#define STATE_DEFAULT 0

#include "pebble.h"

typedef struct TomatoSettings {
  int last_time;
  int state;
  int current_duration;
} TomatoSettings;

static int32_t read_int(const uint32_t key, int32_t def_value) {
  if (persist_exists(key)) {
    return persist_read_int(key);
  } else {
    return def_value;
  }
}

static TomatoSettings read_settings() {
  TomatoSettings settings = {
    read_int(LAST_TIME_KEY, time(NULL)),
    read_int(STATE_KEY, STATE_DEFAULT),
    read_int(CURRENT_DURATION_KEY, WORK_DURATION)
  };
  return settings;
}

static void save_settings(TomatoSettings settings) {
  persist_write_int(LAST_TIME_KEY, settings.last_time);
  persist_write_int(STATE_KEY, settings.state);
  persist_write_int(CURRENT_DURATION_KEY, settings.current_duration);
}