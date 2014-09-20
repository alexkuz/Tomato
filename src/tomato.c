#include "pebble.h"
#include "settings.h"
#include "menu.h"

#include "arc.c"
  
static Window *window;

static Layer *layer;
static Layer *time_rect_layer;
static TextLayer *time_layer;
static Layer *arc_layer;
static TextLayer *iteration_layer;

static GBitmap *pomodoro_image;
static GBitmap *break_image;

static int exec_state = RUNNING_EXEC_STATE;

static TomatoSettings settings;
  
static int increment_time = INCREMENT_TIME;

static int passed_time() {
  return time(NULL) - settings.last_time;
}

static void layer_draw_image(Layer *me, GContext* ctx) {
  GBitmap *image = (settings.state == POMODORO_STATE) ? pomodoro_image : break_image;
  GRect bounds = image->bounds;
  graphics_draw_bitmap_in_rect(ctx, image, (GRect) { .origin = { 0, 0 }, .size = bounds.size });
}

static void layer_draw_time_rect(Layer *me, GContext* ctx) {
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_context_set_stroke_color(ctx, GColorBlack);
  GRect bounds = layer_get_bounds(me);
  graphics_fill_rect(ctx, bounds, 3, GCornersAll);
  graphics_draw_round_rect(ctx, bounds, 3);
}


static void layer_draw_arc(Layer *me, GContext* ctx) {
  int end_angle = TRIG_MAX_ANGLE * passed_time() / settings.current_duration;

  graphics_draw_arc(ctx, GPoint(20, 20), 20, 2, end_angle - angle_90 + 1, angle_270, GColorBlack);
  if (end_angle) {
    graphics_draw_arc(ctx, GPoint(20, 20), 20, 6, -angle_90, end_angle - angle_90, GColorBlack);
  }
}


void print_iteration() {
    static char buffer[] = "99";
    snprintf(buffer, sizeof("99"), "%d", settings.iteration);
    text_layer_set_text(iteration_layer, buffer);  
}

void toggle_pomodoro_relax(int skip) {
  if (settings.state == POMODORO_STATE) {
    if (!skip) {
      settings.iteration++;
    }
    settings.state = BREAK_STATE;
    settings.current_duration = (
      settings.long_break_enabled &&
      ((settings.iteration - 1) % settings.long_break_delay) == settings.long_break_delay - 1) ?
        settings.long_break_duration * 60 :
        settings.break_duration * 60;
    vibes_short_pulse();
  } else {
    settings.state = POMODORO_STATE;
    settings.current_duration = settings.pomodoro_duration * 60;
    vibes_double_pulse();
  }
  
  layer_mark_dirty(layer);
  print_iteration();
}

void update_time() {
  static char buffer[] = "00:00";
  time_t diff = settings.current_duration - passed_time();
  if (diff < 0) {
    diff = 0;
  }
  strftime(buffer, sizeof("00:00"), "%M:%S", localtime(&diff));
  text_layer_set_text(time_layer, buffer);
  layer_mark_dirty(arc_layer);
}

/*
void toggle_pause() {
  if (exec_state == RUNNING_EXEC_STATE) {
    last_elapsed = passed_time();
    exec_state = PAUSED_EXEC_STATE;
  } else {
    settings.last_time = time(NULL) - last_elapsed;
    exec_state = RUNNING_EXEC_STATE;
  }
}
*/

void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  settings.last_time = time(NULL);
  toggle_pomodoro_relax(true);
  
  update_time();
}

void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  settings.current_duration += increment_time;
  
  update_time();
}

void down_doubleclick_handler(ClickRecognizerRef recognizer, void *context) {
  settings.current_duration -= increment_time;
  
  update_time();
}

void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  show_menu();
}

static void handle_tick(struct tm *tick_time, TimeUnits units_changed) {
  if (exec_state != RUNNING_EXEC_STATE) {
    return;
  }

  if(passed_time() > settings.current_duration) {
    settings.last_time = time(NULL);
    toggle_pomodoro_relax(false);
  }
  
  update_time();
}

void config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
  window_multi_click_subscribe(BUTTON_ID_DOWN, 2, 2, 0, true, down_doubleclick_handler);
}

static void window_load(Window *window) {
  // Init the layer for display the image
  Layer *window_layer = window_get_root_layer(window);
  
  GRect bounds = layer_get_frame(window_layer);
  layer = layer_create(bounds);
  layer_set_update_proc(layer, layer_draw_image);
  layer_add_child(window_layer, layer);
  
  GFont custom_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_RED_OCTOBER_18));
  //GFont custom_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_RUSSIAN_16));
  //GFont custom_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_GAGARIN_24));
  
  bounds = (GRect) { .origin = { 144 - 60 - 10, 168 - 24 - 12 }, .size = { 65, 26 } };
  time_rect_layer = layer_create(bounds);
  layer_set_update_proc(time_rect_layer, layer_draw_time_rect);
  layer_add_child(window_layer, time_rect_layer);

  bounds =  (GRect) { .origin = { 144 - 60 - 5, 168 - 24 - 10 }, .size = { 60, 24 } };
  time_layer = text_layer_create(bounds);
  text_layer_set_font(time_layer, custom_font);
  text_layer_set_background_color(time_layer, GColorClear);
  text_layer_set_text_color(time_layer, GColorBlack);
  text_layer_set_text_alignment(time_layer, GTextAlignmentLeft);
  layer_add_child(window_layer, text_layer_get_layer(time_layer));
  
  bounds =  (GRect) { .origin = { 15, 17 }, .size = { 30, 24 } };
  iteration_layer = text_layer_create(bounds);
  text_layer_set_font(iteration_layer, custom_font);
  text_layer_set_background_color(iteration_layer, GColorWhite);
  text_layer_set_text_color(iteration_layer, GColorBlack);
  text_layer_set_text_alignment(iteration_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(iteration_layer));

  bounds =  (GRect) { .origin = { 8, 8 }, .size = { 40, 40 } };
  arc_layer = layer_create(bounds);
  layer_set_update_proc(arc_layer, layer_draw_arc);
  layer_add_child(window_layer, arc_layer);

  print_iteration();
}

static void window_unload(Window *window) {
  layer_destroy(layer);
  layer_destroy(time_rect_layer);
  layer_destroy(arc_layer);
  text_layer_destroy(time_layer);
  text_layer_destroy(iteration_layer);  
}

static void window_appear(Window *window) {
  settings = read_settings();  

  print_iteration();
}

static void window_disappear(Window *window) {
  save_settings(settings);
}

static void init(void) {
  pomodoro_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_WORK);
  break_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_READ);  

  window = window_create();
  window_set_fullscreen(window, true);
  
  window_set_click_config_provider(window, config_provider);
  
  window_set_window_handlers(window, (WindowHandlers) {
	  .load = window_load,
    .unload = window_unload,
    .appear = window_appear,
    .disappear = window_disappear
  });
  
  const bool animated = true;
  window_stack_push(window, animated);
  
  tick_timer_service_subscribe(SECOND_UNIT, handle_tick);
}

static void deinit(void) {
  gbitmap_destroy(pomodoro_image);
  gbitmap_destroy(break_image);

  window_destroy(window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}

