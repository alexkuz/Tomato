#include "iteration.h"
#include <pebble.h>
#include "settings.h"

// BEGIN AUTO-GENERATED UI CODE; DO NOT MODIFY
static Window *s_window;
static GBitmap *s_res_image_count;
static GFont s_res_font_roboto_70;
static BitmapLayer *s_backlayer;
static TextLayer *s_count_layer;

static void initialise_ui(void) {
  s_window = window_create();
  window_set_background_color(s_window, GColorBlack);
  window_set_fullscreen(s_window, true);
  
  s_res_image_count = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_COUNT);
  s_res_font_roboto_70 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_70));
  // s_backlayer
  s_backlayer = bitmap_layer_create(GRect(0, 0, 144, 168));
  bitmap_layer_set_bitmap(s_backlayer, s_res_image_count);
  bitmap_layer_set_background_color(s_backlayer, GColorBlack);
  layer_add_child(window_get_root_layer(s_window), (Layer *)s_backlayer);
  
  // s_count_layer
  s_count_layer = text_layer_create(GRect(24, 41, 100, 78));
  text_layer_set_background_color(s_count_layer, GColorClear);
  text_layer_set_text(s_count_layer, "0");
  text_layer_set_text_alignment(s_count_layer, GTextAlignmentCenter);
  text_layer_set_font(s_count_layer, s_res_font_roboto_70);
  layer_add_child(window_get_root_layer(s_window), (Layer *)s_count_layer);
}

static void destroy_ui(void) {
  window_destroy(s_window);
  bitmap_layer_destroy(s_backlayer);
  text_layer_destroy(s_count_layer);
  gbitmap_destroy(s_res_image_count);
  fonts_unload_custom_font(s_res_font_roboto_70);
}
// END AUTO-GENERATED UI CODE

static TomatoSettings settings;
static AppTimer* close_timer;

void hide_iteration(void) {
  if (close_timer) {
    app_timer_cancel(close_timer);
    close_timer = NULL;
  }
  window_stack_remove(s_window, true);
}

static void handle_window_unload(Window* window) {
  destroy_ui();
}

void print_iteration_count() {
    static char buffer[] = "99";
    snprintf(buffer, sizeof("99"), "%d", settings.calendar.sets[0]);
    text_layer_set_text(s_count_layer, buffer);  
}

static void handle_iteration_window_appear(Window *window) {
  settings = read_settings();  
  print_iteration_count();
}

void iteration_select_click_handler(ClickRecognizerRef recognizer, void *context) {
  hide_iteration();
}

void iteration_config_provider() {
  window_single_click_subscribe(BUTTON_ID_SELECT, iteration_select_click_handler);
}

void close_callback(void *data) {
  hide_iteration();
}

void show_iteration(void) {
  initialise_ui();
  window_set_click_config_provider(s_window, iteration_config_provider);
  
  close_timer = app_timer_register(5000, close_callback, NULL);
  
  window_set_window_handlers(s_window, (WindowHandlers) {
    .unload = handle_window_unload,
    .appear = handle_iteration_window_appear
  });
  window_stack_push(s_window, true);
}