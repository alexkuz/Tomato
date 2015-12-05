#include "pebble.h"
#include "settings.h"
#include "menu.h"
#include "iteration.h"
  
static Window *window;

static BitmapLayer *work_layer;
static BitmapLayer *relax_layer;
#ifdef PBL_COLOR
static BitmapLayer *mask_layer;
#endif

static PropertyAnimation *work_animation;
static PropertyAnimation *relax_animation;

static TextLayer *clock_layer;
static TextLayer *relax_minute_layer;
static TextLayer *relax_second_layer;
static Layer *scale_layer;

static GFont time_font;
static GFont scale_font;

static GBitmap *pomodoro_image;
static GBitmap *break_image;
#ifdef PBL_COLOR
static GBitmap *mask_image;
#endif

static int exec_state = RUNNING_EXEC_STATE;

static TomatoSettings settings;
  
static int increment_time = INCREMENT_TIME;

static int animate_time = 0;
static int animate_time_factor = 0;
static bool is_animating;

static int max_time = 59 * 60;

static struct tm now;

static int passed_time() {
  return time(NULL) - settings.last_time;
}

time_t get_diff() {
  int animate_shift = animate_time_factor * (ANIMATION_NORMALIZED_MAX - animate_time) / (ANIMATION_NORMALIZED_MAX - ANIMATION_NORMALIZED_MIN);
  time_t diff = settings.current_duration - passed_time() - animate_shift;
  if (diff < 0) {
    diff = 0;
  } else if (diff > max_time) {
    settings.current_duration -= (diff - max_time);
    diff = max_time;
  }
  return diff;
}

const int sec_per_pixel = 60 / 8;
const int half_max_ratio = TRIG_MAX_RATIO / 2;
const int angle_90 = TRIG_MAX_ANGLE / 4;

static void layer_draw_scale(Layer *me, GContext* ctx) {
  GRect frame = layer_get_frame(me);
  int center = frame.size.w / 2;
  int extra_x = frame.size.w / 4;
  int pomodoro_width = center - 5;
  int scale_x = center + extra_x;
  static char buffer[] = "00";
  long angle, round_factor, sin;
  int mm, x;
  
  time_t diff = get_diff();
  int start = center - diff / sec_per_pixel;
  int offset = 10;
  for (int m = -offset; m < 60 + offset; m++) {
    x = start + m * 60 / sec_per_pixel;
    bool is_on_edge = x < extra_x / 2 || x > frame.size.w - extra_x / 2;
    bool is_text_on_edge = x < -extra_x / 2 || x > frame.size.w + extra_x / 2;
    if (x < -extra_x) {
      continue;
    } else if (x > frame.size.w + extra_x) {
      break;
    }
    mm = m % 60;
    if (mm < 0) {
      mm += 60;
    }
    x -= center;
    angle = x * angle_90 / scale_x;
    sin = sin_lookup(angle);
    round_factor = sin < 0 ? -half_max_ratio : half_max_ratio;
    x = center + (sin_lookup(angle) * pomodoro_width + round_factor) / TRIG_MAX_RATIO;
    graphics_context_set_stroke_color(ctx, PBL_IF_COLOR_ELSE(is_on_edge ? GColorLightGray : GColorWhite, GColorBlack));
    if (mm % 5 == 0) {
      graphics_context_set_text_color(ctx, PBL_IF_COLOR_ELSE(is_on_edge ? GColorLightGray : GColorWhite, GColorBlack));
      snprintf(buffer, sizeof("00"), "%0d", mm);
      if (!is_text_on_edge) {
        graphics_draw_text(ctx, buffer, scale_font, GRect(x - 15, 0, 30, 24), GTextOverflowModeFill, GTextAlignmentCenter, NULL);
      }
      graphics_draw_rect(ctx, GRect(x - 1, 27, 2, 8));
    } else if (mm < settings.pomodoro_duration) {
      graphics_draw_rect(ctx, GRect(x - 1, 32, 2, 3));
    }
  }
}

void on_switch_screen_animation_stopped(Animation *anim, bool finished, void *context) {
  property_animation_destroy((PropertyAnimation*) anim);
}

static void fire_switch_screen_animation(bool relax_to_work) {
  GRect from_frame = layer_get_frame(bitmap_layer_get_layer(work_layer));
  GRect to_frame = relax_to_work ? 
    (GRect) { .origin = { 0, 0 }, .size = from_frame.size } :
    (GRect) { .origin = { -from_frame.size.w, 0 }, .size = from_frame.size };

  work_animation = property_animation_create_layer_frame(bitmap_layer_get_layer(work_layer), &from_frame, &to_frame);
  animation_set_handlers((Animation*) work_animation, (AnimationHandlers) {
    .stopped = (AnimationStoppedHandler) on_switch_screen_animation_stopped
  }, NULL);
  animation_schedule((Animation*) work_animation);

  from_frame = layer_get_frame(bitmap_layer_get_layer(relax_layer));
  to_frame = relax_to_work ? 
    (GRect) { .origin = { from_frame.size.w, 0 }, .size = from_frame.size } :
    (GRect) { .origin = { 0, 0 }, .size = from_frame.size };

  relax_animation = property_animation_create_layer_frame(bitmap_layer_get_layer(relax_layer), &from_frame, &to_frame);
  animation_set_handlers((Animation*) relax_animation, (AnimationHandlers) {
    .stopped = (AnimationStoppedHandler) on_switch_screen_animation_stopped
  }, NULL);
  animation_schedule((Animation*) relax_animation);
}

void toggle_pomodoro_relax(int skip) {
  if (settings.state == POMODORO_STATE) {
    if (!skip) {
      settings.calendar.sets[0]++;
    }
    settings.state = BREAK_STATE;
    settings.current_duration = (
      settings.long_break_enabled &&
      ((settings.calendar.sets[0] - 1) % settings.long_break_delay) == settings.long_break_delay - 1) ?
        settings.long_break_duration * 60 :
        settings.break_duration * 60;
    vibes_short_pulse();
    fire_switch_screen_animation(false);
  } else {
    settings.state = POMODORO_STATE;
    settings.current_duration = settings.pomodoro_duration * 60;
    vibes_double_pulse();
    fire_switch_screen_animation(true);
  }
}

void update_clock() {
  static char buffer[] = "00:00";
  strftime(buffer, sizeof("00:00"), "%H:%M", &now);
  text_layer_set_text(clock_layer, buffer);
}

void update_relax_minute() {
  static char buffer[] = "00";
  time_t diff = get_diff();
  strftime(buffer, sizeof("00"), "%M", localtime(&diff));
  text_layer_set_text(relax_minute_layer, buffer);
}

void update_relax_second() {
  static char buffer[] = "00";
  time_t diff = get_diff();
  strftime(buffer, sizeof("00"), "%S", localtime(&diff));
  text_layer_set_text(relax_second_layer, buffer);
}

void on_scale_animation_update(Animation* animation, uint32_t distance_normalized) {
  animate_time = distance_normalized;
  layer_mark_dirty(scale_layer);
}

void on_scale_animation_started(Animation* animation, void *data) {
  is_animating = true;
}

void on_scale_animation_stopped(Animation* animation, bool finished, void *data) {
  is_animating = false;
  animate_time = 0;
  animate_time_factor = 0;
  layer_mark_dirty(scale_layer);
  animation_destroy(animation);
}

static Animation *animation;
static AnimationImplementation animation_implementation;
  
void animate_scale() {
  animation = animation_create();
  animation_set_duration(animation, 300);
  
  animation_implementation = (AnimationImplementation) {
    .update = (AnimationUpdateImplementation) on_scale_animation_update
  };

  animation_set_implementation(animation, &animation_implementation);
  animation_set_handlers(animation, (AnimationHandlers) {
    .stopped = (AnimationStoppedHandler) on_scale_animation_stopped,
    .started = (AnimationStartedHandler) on_scale_animation_started
  }, NULL);
  animation_schedule(animation);
}

void update_time(bool animate) {
  if (settings.state == BREAK_STATE) {
    animate_time_factor = 0;
    update_relax_minute();
    update_relax_second();
  } else if (animate) {
    animate_scale();
  } else {
    animate_time_factor = 0;
    layer_mark_dirty(scale_layer);
  }
}

void up_longclick_handler(ClickRecognizerRef recognizer, void *context) {
  settings.last_time = time(NULL);
  toggle_pomodoro_relax(true);
  
  update_time(false);
}

void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  settings.current_duration += increment_time;
  if (settings.state == POMODORO_STATE) {
    animate_time_factor = increment_time;
  }
  update_time(true);
}

void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  settings.current_duration -= increment_time;
  if (settings.state == POMODORO_STATE) {
    animate_time_factor = -increment_time;  
  }
  update_time(true);
}

void select_longclick_handler(ClickRecognizerRef recognizer, void *context) {
  show_menu();
}

void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  show_iteration();
}

static void handle_tick(struct tm *tick_time, TimeUnits units_changed) {
  now = *tick_time;
  if (units_changed & MINUTE_UNIT) {
    update_clock();
  }

  if (exec_state != RUNNING_EXEC_STATE) {
    return;
  }
  
  if (is_animating) {
    return;
  }

  if(passed_time() > settings.current_duration) {
    settings.last_time = time(NULL);
    toggle_pomodoro_relax(false);
  }
  
  if (units_changed & SECOND_UNIT) {
    if (settings.state == BREAK_STATE) {
      update_relax_minute();
      update_relax_second();
    } else {
      update_time(false);
    }
  }
}

void config_provider(void *context) {
  window_long_click_subscribe(BUTTON_ID_SELECT, 300, select_longclick_handler, NULL);
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);

  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
  
  window_long_click_subscribe(BUTTON_ID_UP, 300, up_longclick_handler, NULL);
}

static void window_load(Window *window) {
  // Init the layer for display the image
  Layer *window_layer = window_get_root_layer(window);
  
  GRect window_bounds = layer_get_frame(window_layer);
  GRect bounds = window_bounds;
  int window_width = bounds.size.w;
  int window_height = bounds.size.h;
  int center_x = window_width / 2;
  int center_y = window_height / 2;
  
  work_layer = bitmap_layer_create(bounds);
  bitmap_layer_set_bitmap(work_layer, pomodoro_image);
  #ifdef PBL_COLOR
  bitmap_layer_set_compositing_mode(work_layer, GCompOpSet);
  #endif
  layer_add_child(window_layer, bitmap_layer_get_layer(work_layer));
  
  bounds = (GRect) { .origin = { window_width, 0 }, .size = bounds.size };
  relax_layer = bitmap_layer_create(bounds);
  bitmap_layer_set_bitmap(relax_layer, break_image);
  #ifdef PBL_COLOR
  bitmap_layer_set_compositing_mode(relax_layer, GCompOpSet);
  #endif
  layer_add_child(window_layer, bitmap_layer_get_layer(relax_layer));

  time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_DD_24));
  scale_font = fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD);
  GFont clock_font = fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD);
  GFont relax_font = fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD);
  
  bounds = (GRect) { .origin = { center_x - 70, center_y - 29 }, .size = { 140, 35 } };
  scale_layer = layer_create(bounds);
  layer_set_update_proc(scale_layer, layer_draw_scale);
  layer_add_child(bitmap_layer_get_layer(work_layer), scale_layer);
  
  bounds = (GRect) { .origin = { center_x - 47, center_y - 21 }, .size = { 24, 24 } };
  relax_minute_layer = text_layer_create(bounds);
  text_layer_set_text_alignment(relax_minute_layer, GTextAlignmentCenter);
  text_layer_set_font(relax_minute_layer, relax_font);
  text_layer_set_background_color(relax_minute_layer, GColorBlack);
  text_layer_set_text_color(relax_minute_layer, GColorWhite);
  layer_add_child(bitmap_layer_get_layer(relax_layer), text_layer_get_layer(relax_minute_layer));
  
  bounds = (GRect) { .origin = { center_x + 20, center_y - 21 }, .size = { 24, 24 } };
  relax_second_layer = text_layer_create(bounds);
  text_layer_set_text_alignment(relax_second_layer, GTextAlignmentCenter);
  text_layer_set_font(relax_second_layer, relax_font);
  text_layer_set_background_color(relax_second_layer, GColorBlack);
  text_layer_set_text_color(relax_second_layer, GColorWhite);
  layer_add_child(bitmap_layer_get_layer(relax_layer), text_layer_get_layer(relax_second_layer));
  
  const int clock_height = 24;

  #ifdef PBL_COLOR
  mask_layer = bitmap_layer_create(window_bounds);
  bitmap_layer_set_bitmap(mask_layer, mask_image);
  bitmap_layer_set_compositing_mode(mask_layer, GCompOpSet);
  layer_add_child(bitmap_layer_get_layer(work_layer), bitmap_layer_get_layer(mask_layer));
  #endif

  bounds =  (GRect) { .origin = { 0, window_height - clock_height - 8 }, .size = { window_width, clock_height } };
  clock_layer = text_layer_create(bounds);
  text_layer_set_font(clock_layer, clock_font);
  text_layer_set_text_color(clock_layer, GColorWhite);
  text_layer_set_background_color(clock_layer, GColorClear);
  text_layer_set_text_alignment(clock_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(clock_layer));
}

static void window_unload(Window *window) {
  bitmap_layer_destroy(work_layer);
  bitmap_layer_destroy(relax_layer);
  #ifdef PBL_COLOR
  bitmap_layer_destroy(mask_layer);
  #endif
  text_layer_destroy(clock_layer);
  text_layer_destroy(relax_second_layer);
  text_layer_destroy(relax_minute_layer);
  fonts_unload_custom_font(time_font);
}

static void window_appear(Window *window) {
  settings = read_settings();
  
  if (settings.state == BREAK_STATE) {
    GRect frame = layer_get_frame(bitmap_layer_get_layer(work_layer));
    layer_set_frame(bitmap_layer_get_layer(work_layer), (GRect) {.origin = { -frame.size.w, 0}, .size = frame.size });
    layer_set_frame(bitmap_layer_get_layer(relax_layer), (GRect) {.origin = { 0, 0}, .size = frame.size });
  }

  time_t t = time(NULL);
  now = *localtime(&t);
  update_clock();

  update_relax_minute();
  update_relax_second();
  layer_mark_dirty(scale_layer);
}

static void window_disappear(Window *window) {
  save_settings(settings);
}

static void init(void) {
  time_t t = time(NULL);
  now = *localtime(&t);
  
  pomodoro_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_WORK);
  break_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_RELAX);  
  #ifdef PBL_COLOR
  mask_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_MASK);
  #endif

  window = window_create();
  
  #ifndef PBL_SDK_3
  window_set_fullscreen(window, 1);
  #endif
  
  window_set_click_config_provider(window, config_provider);
  
  window_set_window_handlers(window, (WindowHandlers) {
	  .load = window_load,
    .unload = window_unload,
    .appear = window_appear,
    .disappear = window_disappear
  });
  
  const bool animated = true;
  window_set_background_color(window, PBL_IF_COLOR_ELSE(GColorDukeBlue, GColorBlack));
  window_stack_push(window, animated);
  
  tick_timer_service_subscribe(SECOND_UNIT, handle_tick);
}

static void deinit(void) {
  gbitmap_destroy(pomodoro_image);
  gbitmap_destroy(break_image);
  #ifdef PBL_COLOR
  gbitmap_destroy(mask_image);
  #endif

  window_destroy(window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}

