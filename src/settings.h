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

TomatoSettings get_default_settings();

TomatoSettings read_settings();

void save_settings(TomatoSettings settings);