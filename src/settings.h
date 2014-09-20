#define WORK_STATE 0
#define RELAX_STATE 1
  
#define RUNNING_EXEC_STATE 0
#define PAUSED_EXEC_STATE 1
  
#define MAX_APP_IDLE (30 * 60)
#define MAX_ITERATION_IDLE (8 * 60 * 60)

#define INCREMENT_TIME 60


#define LAST_TIME_KEY 0
#define STATE_KEY 1
#define CURRENT_DURATION_KEY 2
#define ITERATION_KEY 3
#define WORK_DURATION_KEY 4
#define RELAX_DURATION_KEY 5

#define LAST_TIME_DEFAULT 0
#define STATE_DEFAULT 0
#define ITERATION_DEFAULT 0
  
#include "pebble.h"
  
#ifndef SETTINGS_H
#define SETTINGS_H

typedef struct SettingParams {
  int default_value;
  int min_value;
  int max_value;
  char* title;
  char* format;
} SettingParams;

extern const SettingParams work_duration_params;
extern const SettingParams relax_duration_params;

typedef struct TomatoSettings {
  int last_time;
  int state;
  int current_duration;
  int iteration;
  int work_duration;
  int relax_duration;
} TomatoSettings;

TomatoSettings get_default_settings();

TomatoSettings read_settings();

void save_settings(TomatoSettings settings);

#endif /* SETTINGS_H */