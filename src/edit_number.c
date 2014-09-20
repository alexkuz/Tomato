#include <pebble.h>
#include "edit_number.h"

// BEGIN AUTO-GENERATED UI CODE; DO NOT MODIFY
static Window *s_window;
static GBitmap *s_res_image_action_increment;
static GBitmap *s_res_image_action_decrement;
static GFont s_res_font_red_october_40;
static GFont s_res_gothic_18;
static ActionBarLayer *action_bar_layer;
static TextLayer *number_text_layer;
static TextLayer *title_text_layer;

static void initialise_ui(void) {
  s_window = window_create();
  window_set_fullscreen(s_window, false);
  
  s_res_image_action_increment = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_ACTION_INCREMENT);
  s_res_image_action_decrement = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_ACTION_DECREMENT);
  s_res_font_red_october_40 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_RED_OCTOBER_40));
  s_res_gothic_18 = fonts_get_system_font(FONT_KEY_GOTHIC_18);
  // action_bar_layer
  action_bar_layer = action_bar_layer_create();
  action_bar_layer_add_to_window(action_bar_layer, s_window);
  action_bar_layer_set_background_color(action_bar_layer, GColorWhite);
  action_bar_layer_set_icon(action_bar_layer, BUTTON_ID_UP, s_res_image_action_increment);
  action_bar_layer_set_icon(action_bar_layer, BUTTON_ID_DOWN, s_res_image_action_decrement);
  layer_add_child(window_get_root_layer(s_window), (Layer *)action_bar_layer);
  
  // number_text_layer
  number_text_layer = text_layer_create(GRect(6, 47, 116, 40));
  text_layer_set_text(number_text_layer, "0");
  text_layer_set_text_alignment(number_text_layer, GTextAlignmentCenter);
  text_layer_set_font(number_text_layer, s_res_font_red_october_40);
  layer_add_child(window_get_root_layer(s_window), (Layer *)number_text_layer);
  
  // title_text_layer
  title_text_layer = text_layer_create(GRect(2, 93, 123, 20));
  text_layer_set_text(title_text_layer, "Text layer");
  text_layer_set_text_alignment(title_text_layer, GTextAlignmentCenter);
  text_layer_set_font(title_text_layer, s_res_gothic_18);
  layer_add_child(window_get_root_layer(s_window), (Layer *)title_text_layer);
}

static void destroy_ui(void) {
  window_destroy(s_window);
  action_bar_layer_destroy(action_bar_layer);
  text_layer_destroy(number_text_layer);
  text_layer_destroy(title_text_layer);
  gbitmap_destroy(s_res_image_action_increment);
  gbitmap_destroy(s_res_image_action_decrement);
  fonts_unload_custom_font(s_res_font_red_october_40);
}
// END AUTO-GENERATED UI CODE

static int s_value = 0;
static int s_min_value = 0;
static int s_max_value = 0;
static int s_default_value = 0;
static int s_setting_key = 0;

static void handle_window_unload(Window* window) {
  destroy_ui();
}

static void update_number() {
  static char number_text[] = "0  ";
  snprintf(number_text, sizeof(number_text), "%u", s_value);
  text_layer_set_text(number_text_layer, number_text);
}

static void increment_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (s_value >= s_max_value) {
    return;
  }
  s_value++;
  update_number();
  persist_write_int(s_setting_key, s_value);
}

static void decrement_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (s_value <= s_min_value) {
    return;
  }
  s_value--;
  update_number();
  persist_write_int(s_setting_key, s_value);
}

static void reset_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (s_value == s_default_value) {
    return;
  }
  s_value = s_default_value;
  update_number();
  persist_write_int(s_setting_key, s_value);
}

static void click_config_provider(void *context) {
  const uint16_t repeat_interval_ms = 50;
  window_single_repeating_click_subscribe(BUTTON_ID_UP, repeat_interval_ms, (ClickHandler) increment_click_handler);
  window_single_repeating_click_subscribe(BUTTON_ID_DOWN, repeat_interval_ms, (ClickHandler) decrement_click_handler);

  window_single_click_subscribe(BUTTON_ID_SELECT, (ClickHandler) reset_click_handler);
}

static void init_action_bar() {
  action_bar_layer_set_click_config_provider(action_bar_layer, click_config_provider);
}

static void set_values(int setting_key, int value, SettingParams params) {
  s_setting_key = setting_key;
  s_value = value;
  s_default_value = params.default_value;
  s_max_value = params.max_value;
  s_min_value = params.min_value;
  
  update_number();
  text_layer_set_text(title_text_layer, params.title);
}

void show_edit_number(int setting_key, int value, SettingParams params) {
  initialise_ui();
  init_action_bar();
  
  set_values(setting_key, value, params);
  
  window_set_window_handlers(s_window, (WindowHandlers) {
    .unload = handle_window_unload,
  });
  window_stack_push(s_window, true);
}

void hide_edit_number(void) {
  window_stack_remove(s_window, true);
}
