#include "pebble.h"
#include "settings.h"
#include "menu.h"
#include "arc.h"
  
static Window *window;

static Layer *layer;
static Layer *time_rect_layer;
static TextLayer *time_layer;
static Layer *arc_layer;
static TextLayer *iteration_layer;
static Layer *clock_layer;

static GFont time_font;
static GFont iteration_font;

static GBitmap *pomodoro_image;
static GBitmap *break_image;

static int exec_state = RUNNING_EXEC_STATE;

static TomatoSettings settings;
  
static int increment_time = INCREMENT_TIME;

static struct tm now;

static int passed_time() {
  return time(NULL) - settings.last_time;
}

static void layer_draw_image(Layer *me, GContext* ctx) {
  GBitmap *image = (settings.state == POMODORO_STATE) ? pomodoro_image : break_image;
  GRect bounds = image->bounds;
  graphics_draw_bitmap_in_rect(ctx, image, (GRect) { .origin = { 0, 0 }, .size = bounds.size });
}

static void layer_draw_time_rect(Layer *me, GContext* ctx) {
  graphics_context_set_stroke_color(ctx, GColorBlack);
  GRect bounds = layer_get_bounds(me);
  GRect inner_bounds = (GRect) { .origin = { 1, 1 }, .size = {bounds.size.w - 2, bounds.size.h - 2} };
  
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, bounds, 3, GCornersAll);
  graphics_draw_round_rect(ctx, bounds, 3);
  
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, inner_bounds, 3, GCornersAll);
  graphics_draw_round_rect(ctx, inner_bounds, 3);
}


static void layer_draw_arc(Layer *me, GContext* ctx) {
  int end_angle = TRIG_MAX_ANGLE * passed_time() / settings.current_duration;

  graphics_draw_arc(ctx, GPoint(20, 20), 20, 2, end_angle - angle_90 + 1, angle_270, GColorBlack);
  if (end_angle) {
    graphics_draw_arc(ctx, GPoint(20, 20), 20, 5, -angle_90, end_angle - angle_90, GColorBlack);
  }
}

static void draw_hand(GContext* ctx, GPoint from, int angle, int length) {
  GPoint to = {
    .x = (sin_lookup(angle) * length / TRIG_MAX_RATIO) + from.x,
    .y = (-cos_lookup(angle) * length / TRIG_MAX_RATIO) + from.y
  };
  GPoint shift1, shift2;
  if (angle < angle_45 || angle > TRIG_MAX_ANGLE - angle_45) {
    shift1 = GPoint(0, 0);
    shift2 = GPoint(1, 0);
  } else if (angle < angle_180 - angle_45) {
    shift1 = GPoint(1, 0);
    shift2 = GPoint(1, 1);
  } else if (angle < angle_270 - angle_45) {
    shift1 = GPoint(1, 1);
    shift2 = GPoint(0, 1);    
  } else {
    shift1 = GPoint(0, 1);
    shift2 = GPoint(0, 0);
  }
  
  // shift = GPoint (shift.x ? 1 : 0, shift.y ? 1 : 0);
  
  GPoint from1 = GPoint(shift1.x + from.x, shift1.y + from.y);
  GPoint to1 = GPoint(shift1.x + to.x, shift1.y + to.y);

  GPoint from2 = GPoint(shift2.x + from.x, shift2.y + from.y);
  GPoint to2 = GPoint(shift2.x + to.x, shift2.y + to.y);

  graphics_draw_line(ctx, from1, to1);
  graphics_draw_line(ctx, from2, to2);
}

static void layer_draw_clock(Layer *me, GContext* ctx) {
  int minute_angle = TRIG_MAX_ANGLE * now.tm_min / 60;
  int hour_angle = TRIG_MAX_ANGLE * (now.tm_hour % 12) / 12 + minute_angle / 12;

  GRect bounds = layer_get_bounds(me);
  GPoint center = GPoint(bounds.size.w / 2 - 1, bounds.size.h / 2 - 1);
  int radius = center.x;
  int minute_hand_length = radius - 3;
  int hour_hand_length = minute_hand_length - 3;
  
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_circle(ctx, center, radius);
  graphics_draw_circle(ctx, center, radius);
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_circle(ctx, center, radius - 1);
  graphics_draw_circle(ctx, center, radius - 1);
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_circle(ctx, center, 2);
  
  draw_hand(ctx, center, minute_angle, minute_hand_length);
  draw_hand(ctx, center, hour_angle, hour_hand_length);
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
  if (units_changed & MINUTE_UNIT) {
    now = *tick_time;
    layer_mark_dirty(clock_layer);
  }

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
  int window_width = bounds.size.w;
  int window_heihgt = bounds.size.h;
  
  layer = layer_create(bounds);
  layer_set_update_proc(layer, layer_draw_image);
  layer_add_child(window_layer, layer);
  
  time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_DD_24));
  iteration_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_RED_OCTOBER_18));
  
  const int time_width = 65;
  const int time_height = 26;
  const int padding = 6;
  
  bounds = (GRect) { .origin = { window_width - time_width - padding, window_heihgt - time_height - padding }, .size = { time_width, time_height } };
  time_rect_layer = layer_create(bounds);
  layer_set_update_proc(time_rect_layer, layer_draw_time_rect);
  layer_add_child(window_layer, time_rect_layer);

  bounds =  (GRect) { .origin = { window_width - time_width - padding + 6, window_heihgt - time_height - padding - 3 }, .size = { time_width - 10, time_height - 2 } };
  time_layer = text_layer_create(bounds);
  text_layer_set_font(time_layer, time_font);
  text_layer_set_background_color(time_layer, GColorClear);
  text_layer_set_text_color(time_layer, GColorBlack);
  text_layer_set_text_alignment(time_layer, GTextAlignmentLeft);
  text_layer_set_overflow_mode(time_layer, GTextOverflowModeTrailingEllipsis);
  layer_add_child(window_layer, text_layer_get_layer(time_layer));
  
  bounds =  (GRect) { .origin = { 7 + padding, 9 + padding }, .size = { 30, 24 } };
  iteration_layer = text_layer_create(bounds);
  text_layer_set_font(iteration_layer, iteration_font);
  text_layer_set_background_color(iteration_layer, GColorWhite);
  text_layer_set_text_color(iteration_layer, GColorBlack);
  text_layer_set_text_alignment(iteration_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(iteration_layer));
  
  const int arc_size = 40;

  bounds =  (GRect) { .origin = { padding, padding }, .size = { arc_size, arc_size } };
  arc_layer = layer_create(bounds);
  layer_set_update_proc(arc_layer, layer_draw_arc);
  layer_add_child(window_layer, arc_layer);
  
  const int clock_size = 30;

  bounds =  (GRect) { .origin = { padding, window_heihgt - clock_size - padding + 1 }, .size = { clock_size, clock_size } };
  clock_layer = layer_create(bounds);
  layer_set_update_proc(clock_layer, layer_draw_clock);
  layer_add_child(window_layer, clock_layer);

  print_iteration();
}

static void window_unload(Window *window) {
  layer_destroy(layer);
  layer_destroy(time_rect_layer);
  layer_destroy(arc_layer);
  text_layer_destroy(time_layer);
  text_layer_destroy(iteration_layer);
  layer_destroy(clock_layer);
  fonts_unload_custom_font(time_font);
  fonts_unload_custom_font(iteration_font);
}

static void window_appear(Window *window) {
  settings = read_settings();  

  time_t t = time(NULL);
  now = *localtime(&t);
  layer_mark_dirty(clock_layer);

  print_iteration();
  update_time();
}

static void window_disappear(Window *window) {
  save_settings(settings);
}

static void init(void) {
  time_t t = time(NULL);
  now = *localtime(&t);
  
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

