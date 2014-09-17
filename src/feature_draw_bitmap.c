#include "pebble.h"
#include "arc.c"
  
#define TRUE 1
#define FALSE 0

#define WORK_STATE 0
#define RELAX_STATE 1
  
#define RUNNING_STATE 0
#define PAUSED_STATE 1
  
#define WORK_DURATION (25 * 60)
#define RELAX_DURATION (5 * 60)
  
#define INCREMENT_TIME 60
  
static Window *window;

static Layer *layer;
static TextLayer *timeLayer;
static Layer *arcLayer;
static TextLayer *iterationLayer;

static GBitmap *workImage;
static GBitmap *relaxImage;

static int state = WORK_STATE;

static int runningState = RUNNING_STATE;
static int lastElapsed;

static time_t lastTime;
static time_t curTime;

static int iteration = 1;

static int workDuration = WORK_DURATION;
static int relaxDuration = RELAX_DURATION;
static int curDuration = WORK_DURATION;
static int incrementTime = INCREMENT_TIME;

static void layer_draw_image(Layer *me, GContext* ctx) {
  GBitmap *image = (state == WORK_STATE) ? workImage : relaxImage;
  GRect bounds = image->bounds;
  graphics_draw_bitmap_in_rect(ctx, image, (GRect) { .origin = { 0, 0 }, .size = bounds.size });
}

static void layer_draw_arc(Layer *me, GContext* ctx) {
  int end_angle = TRIG_MAX_ANGLE * (curTime - lastTime) / curDuration;
  if (!end_angle) {
    return;
  }
  graphics_draw_arc(ctx, GPoint(25, 25), 25, 6, -angle_90, end_angle - angle_90, GColorBlack);
}


void print_iteration() {
    static char buffer[] = "99";
    snprintf(buffer, sizeof("99"), "%d", iteration);
    text_layer_set_text(iterationLayer, buffer);  
}

void toggle_work_relax(int skip) {
  if (state == WORK_STATE) {
    state = RELAX_STATE;
    curDuration = relaxDuration;
    layer_mark_dirty(layer);
    vibes_short_pulse();
  } else {
    if (!skip) {
      iteration++;
    }
    state = WORK_STATE;
    curDuration = workDuration;
    layer_mark_dirty(layer);
    vibes_double_pulse();

    print_iteration();
  }
}

void update_time() {
  static char buffer[] = "00:00";
  time_t diff = lastTime + curDuration - curTime;
  strftime(buffer, sizeof("00:00"), "%M:%S", localtime(&diff));
  text_layer_set_text(timeLayer, buffer);
  layer_mark_dirty(arcLayer);
}

void toggle_pause() {
  curTime = time(NULL);
  if (runningState == RUNNING_STATE) {
    lastElapsed = curTime - lastTime;
    runningState = PAUSED_STATE;
  } else {
    lastTime = curTime - lastElapsed;
    runningState = RUNNING_STATE;
  }
}

void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  curTime = time(NULL);
  lastTime = curTime;
  toggle_work_relax(TRUE);
  
  update_time();
}

void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  curDuration = curDuration + incrementTime;
  
  update_time();
}

void down_doubleclick_handler(ClickRecognizerRef recognizer, void *context) {
  curDuration = curDuration - incrementTime;
  
  update_time();
}

void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  toggle_pause();
}

static void handle_tick(struct tm *tick_time, TimeUnits units_changed) {
  if (runningState != RUNNING_STATE) {
    return;
  }

  curTime = time(NULL);
  
  if(curTime - lastTime > curDuration) {
    lastTime = curTime;
    toggle_work_relax(FALSE);
  }
  
  update_time();
}

void config_provider(void *context) {
  //window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
  window_multi_click_subscribe(BUTTON_ID_DOWN, 2, 2, 0, true, down_doubleclick_handler);
}

int main(void) {
  window = window_create();
  window_stack_push(window, true /* Animated */);
  
  lastTime = time(NULL);
  
  window_set_click_config_provider(window, config_provider);
  
  tick_timer_service_subscribe(SECOND_UNIT, handle_tick);

  // Init the layer for display the image
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_frame(window_layer);
  layer = layer_create(bounds);
  layer_set_update_proc(layer, layer_draw_image);
  layer_add_child(window_layer, layer);

  bounds =  (GRect) { .origin = { 10, 10 }, .size = { 70, 24 } };
  timeLayer = text_layer_create(bounds);
  text_layer_set_background_color(timeLayer, GColorClear);
  text_layer_set_text_color(timeLayer, GColorBlack);
  text_layer_set_font(timeLayer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  layer_add_child(window_layer, text_layer_get_layer(timeLayer));

  bounds =  (GRect) { .origin = { 7, 2 }, .size = { 50, 50 } };
  arcLayer = layer_create(bounds);
  layer_set_update_proc(arcLayer, layer_draw_arc);
  layer_add_child(window_layer, arcLayer);

  bounds =  (GRect) { .origin = { 144 - 30, 148 - 30 }, .size = { 25, 25 } };
  iterationLayer = text_layer_create(bounds);
  text_layer_set_background_color(iterationLayer, GColorWhite);
  text_layer_set_text_color(iterationLayer, GColorBlack);
  text_layer_set_font(iterationLayer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(iterationLayer, GTextAlignmentRight);
  layer_add_child(window_layer, text_layer_get_layer(iterationLayer));
  
  print_iteration();

  workImage = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_WORK);
  relaxImage = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_READ);

  app_event_loop();

  gbitmap_destroy(workImage);
  gbitmap_destroy(relaxImage);
  
  window_destroy(window);
  layer_destroy(layer);
  layer_destroy(arcLayer);
  text_layer_destroy(timeLayer);
  text_layer_destroy(iterationLayer);
}
