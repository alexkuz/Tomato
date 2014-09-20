#include <pebble.h>
#include "menu.h"
#include "edit_number.h"
#include "settings.h"
  
#define POMODORO_DURATION_ROW 0
#define BREAK_DURATION_ROW 1
#define LONG_BREAK_ENABLED_ROW 2
#define LONG_BREAK_DURATION_ROW 3
#define LONG_BREAK_DELAY_ROW 4
#define RESET_ROW 5

// BEGIN AUTO-GENERATED UI CODE; DO NOT MODIFY
static Window *s_window;
static MenuLayer *s_menu_layer;

static void initialise_ui(void) {
  s_window = window_create();
  window_set_fullscreen(s_window, false);
  
  // s_menu_layer
  s_menu_layer = menu_layer_create(GRect(0, 0, 144, 152));
  menu_layer_set_click_config_onto_window(s_menu_layer, s_window);
  layer_add_child(window_get_root_layer(s_window), (Layer *)s_menu_layer);
}

static void destroy_ui(void) {
  window_destroy(s_window);
  menu_layer_destroy(s_menu_layer);
}
// END AUTO-GENERATED UI CODE

static TomatoSettings settings;

void draw_row_callback(GContext *ctx, Layer *cell_layer, MenuIndex *cell_index, void *callback_context)
{
  static char description[32];
  switch(cell_index->row) {
  case POMODORO_DURATION_ROW:
    snprintf(description, sizeof(description), pomodoro_duration_params.format, settings.pomodoro_duration);
    menu_cell_basic_draw(ctx, cell_layer, pomodoro_duration_params.title, description, NULL);
    break;

  case BREAK_DURATION_ROW:
    snprintf(description, sizeof(description), break_duration_params.format, settings.break_duration);
    menu_cell_basic_draw(ctx, cell_layer, break_duration_params.title, description, NULL);
    break;

  case LONG_BREAK_ENABLED_ROW:
    menu_cell_basic_draw(ctx, cell_layer, long_break_enabled_params.title, settings.long_break_enabled ? "Enabled" : "Disabled", NULL);
    break;

  case LONG_BREAK_DURATION_ROW:
    snprintf(description, sizeof(description), long_break_duration_params.format, settings.long_break_duration);
    menu_cell_basic_draw(ctx, cell_layer, long_break_duration_params.title, description, NULL);
    break;

  case LONG_BREAK_DELAY_ROW:
    snprintf(description, sizeof(description), long_break_delay_params.format, settings.long_break_delay);
    menu_cell_basic_draw(ctx, cell_layer, long_break_delay_params.title, description, NULL);
    break;

  case RESET_ROW:
    menu_cell_basic_draw(ctx, cell_layer, "Reset", NULL, NULL);
    break;
  }
}
 
uint16_t num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *callback_context)
{
  return 6;
}
 
void select_click_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context)
{
  switch(cell_index->row) {
  case POMODORO_DURATION_ROW:
    show_edit_number(POMODORO_DURATION_KEY, settings.pomodoro_duration, pomodoro_duration_params);
    break;
  case BREAK_DURATION_ROW:
    show_edit_number(BREAK_DURATION_KEY, settings.break_duration, break_duration_params);
    break;
  case LONG_BREAK_ENABLED_ROW:
    settings.long_break_enabled = !settings.long_break_enabled;
    persist_write_bool(LONG_BREAK_ENABLED_KEY, settings.long_break_enabled);
    menu_layer_reload_data(menu_layer);
    break;
  case LONG_BREAK_DURATION_ROW:
    show_edit_number(LONG_BREAK_DURATION_KEY, settings.long_break_duration, long_break_duration_params);
    break;
  case LONG_BREAK_DELAY_ROW:
    show_edit_number(LONG_BREAK_DELAY_KEY, settings.long_break_delay, long_break_delay_params);
    break;
  case RESET_ROW:
    reset_settings();
    window_stack_remove(s_window, true);
    break;
  }
  
}

static void init_menu_callbacks() {
  MenuLayerCallbacks callbacks = {
    .draw_row = (MenuLayerDrawRowCallback) draw_row_callback,
    .get_num_rows = (MenuLayerGetNumberOfRowsInSectionsCallback) num_rows_callback,
    .select_click = (MenuLayerSelectCallback) select_click_callback
  };
  menu_layer_set_callbacks(s_menu_layer, NULL, callbacks);
}

static void handle_menu_window_unload(Window* window) {
  destroy_ui();
}

static void handle_menu_window_appear(Window *window) {
  settings = read_settings();  
}

void show_menu(void) {
  initialise_ui();
  
  init_menu_callbacks();
  
  window_set_window_handlers(s_window, (WindowHandlers) {
    .unload = handle_menu_window_unload,
    .appear = handle_menu_window_appear
  });
  window_stack_push(s_window, true);
}

void hide_menu(void) {
  window_stack_remove(s_window, true);
}
