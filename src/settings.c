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

#define LAST_TIME_DEFAULT 0
#define STATE_DEFAULT 0


static int32_t read_int(const uint32_t key, int32_t def_value) {
  if (persist_exists(key)) {
    return persist_read_int(key);
  } else {
    return def_value;
  }
}

static void read_settings() {
  last_time = read_int(LAST_TIME_KEY, LAST_TIME_DEFAULT);
  state = read_int(STATE_KEY, STATE_DEFAULT);
}

static void save_settings() {
  persist_write_int(LAST_TIME_KEY, last_time);
  persist_write_int(STATE_KEY, state);
}