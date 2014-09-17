#include "pebble.h"
#include "arc.c"
#include "settings.c"
  
static Window *window;

static Layer *layer;
static TextLayer *time_layer;
static Layer *arc_layer;
static TextLayer *iteration_layer;

static GBitmap *work_image;
static GBitmap *relax_image;

static int state = WORK_STATE;

static int exec_state = RUNNING_EXEC_STATE;
static int last_elapsed;

static time_t last_time;
static time_t cur_time;

static int iteration = 1;

static int work_duration = WORK_DURATION;
static int relax_duration = RELAX_DURATION;
static int current_duration = WORK_DURATION;
static int increment_time = INCREMENT_TIME;

static void layer_draw_image(Layer *me, GContext* ctx) {
  GBitmap *image = (state == WORK_STATE) ? work_image : relax_image;
  GRect bounds = image->bounds;
  graphics_draw_bitmap_in_rect(ctx, image, (GRect) { .origin = { 0, 0 }, .size = bounds.size });
}

static void layer_draw_arc(Layer *me, GContext* ctx) {
  int end_angle = TRIG_MAX_ANGLE * (cur_time - last_time) / current_duration;
  if (!end_angle) {
    return;
  }
  graphics_draw_arc(ctx, GPoint(25, 25), 25, 6, -angle_90, end_angle - angle_90, GColorBlack);
}


void print_iteration() {
    static char buffer[] = "99";
    snprintf(buffer, sizeof("99"), "%d", iteration);
    text_layer_set_text(iteration_layer, buffer);  
}

void toggle_work_relax(int skip) {
  if (state == WORK_STATE) {
    state = RELAX_STATE;
    current_duration = relax_duration;
    layer_mark_dirty(layer);
    vibes_short_pulse();
  } else {
    if (!skip) {
      iteration++;
    }
    state = WORK_STATE;
    current_duration = work_duration;
    layer_mark_dirty(layer);
    vibes_double_pulse();

    print_iteration();
  }
}

void update_time() {
  static char buffer[] = "00:00";
  time_t diff = last_time + current_duration - cur_time;
  strftime(buffer, sizeof("00:00"), "%M:%S", localtime(&diff));
  text_layer_set_text(time_layer, buffer);
  layer_mark_dirty(arc_layer);
}

void toggle_pause() {
  cur_time = time(NULL);
  if (exec_state == RUNNING_EXEC_STATE) {
    last_elapsed = cur_time - last_time;
    exec_state = PAUSED_EXEC_STATE;
  } else {
    last_time = cur_time - last_elapsed;
    exec_state = RUNNING_EXEC_STATE;
  }
}

void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  cur_time = time(NULL);
  last_time = cur_time;
  toggle_work_relax(TRUE);
  
  update_time();
}

void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  current_duration = current_duration + increment_time;
  
  update_time();
}

void down_doubleclick_handler(ClickRecognizerRef recognizer, void *context) {
  current_duration = current_duration - increment_time;
  
  update_time();
}

void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  toggle_pause();
}

static void handle_tick(struct tm *tick_time, TimeUnits units_changed) {
  if (exec_state != RUNNING_EXEC_STATE) {
    return;
  }

  cur_time = time(NULL);
  
  if(cur_time - last_time > current_duration) {
    last_time = cur_time;
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
  read_settings();

  window = window_create();
  window_stack_push(window, true /* Animated */);
  
  last_time = time(NULL);
  
  window_set_click_config_provider(window, config_provider);
  
  tick_timer_service_subscribe(SECOND_UNIT, handle_tick);

  // Init the layer for display the image
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_frame(window_layer);
  layer = layer_create(bounds);
  layer_set_update_proc(layer, layer_draw_image);
  layer_add_child(window_layer, layer);

  bounds =  (GRect) { .origin = { 10, 10 }, .size = { 70, 24 } };
  time_layer = text_layer_create(bounds);
  text_layer_set_background_color(time_layer, GColorClear);
  text_layer_set_text_color(time_layer, GColorBlack);
  text_layer_set_font(time_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  layer_add_child(window_layer, text_layer_get_layer(time_layer));

  bounds =  (GRect) { .origin = { 7, 2 }, .size = { 50, 50 } };
  arc_layer = layer_create(bounds);
  layer_set_update_proc(arc_layer, layer_draw_arc);
  layer_add_child(window_layer, arc_layer);

  bounds =  (GRect) { .origin = { 144 - 30, 148 - 30 }, .size = { 25, 25 } };
  iteration_layer = text_layer_create(bounds);
  text_layer_set_background_color(iteration_layer, GColorWhite);
  text_layer_set_text_color(iteration_layer, GColorBlack);
  text_layer_set_font(iteration_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(iteration_layer, GTextAlignmentRight);
  layer_add_child(window_layer, text_layer_get_layer(iteration_layer));
  
  print_iteration();

  work_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_WORK);
  relax_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_READ);

  app_event_loop();

  gbitmap_destroy(work_image);
  gbitmap_destroy(relax_image);
  
  window_destroy(window);
  layer_destroy(layer);
  layer_destroy(arc_layer);
  text_layer_destroy(time_layer);
  text_layer_destroy(iteration_layer);
}

static void deinit(void) {
  save_settings();
}