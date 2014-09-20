#include <pebble.h>
#include "menu.h"
#include "edit_number.h"
#include "settings.h"
  
#define WORK_DURATION_ROW 0
#define RELAX_DURATION_ROW 1

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
  case WORK_DURATION_ROW:
    snprintf(description, sizeof(description), work_duration_params.format, settings.work_duration);
    menu_cell_basic_draw(ctx, cell_layer, work_duration_params.title, description, NULL);
    break;
  case RELAX_DURATION_ROW:
    snprintf(description, sizeof(description), relax_duration_params.format, settings.relax_duration);
    menu_cell_basic_draw(ctx, cell_layer, relax_duration_params.title, description, NULL);
    break;
  }
}
 
uint16_t num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *callback_context)
{
  return 2;
}
 
void select_click_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context)
{
  switch(cell_index->row) {
  case WORK_DURATION_ROW:
    show_edit_number(WORK_DURATION_KEY, settings.work_duration, work_duration_params);
    break;
  case RELAX_DURATION_ROW:
    show_edit_number(RELAX_DURATION_KEY, settings.relax_duration, relax_duration_params);
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

static void handle_menu_window_disappear(Window *window) {
  save_settings(settings);
}

void show_menu(void) {
  initialise_ui();
  
  init_menu_callbacks();
  
  window_set_window_handlers(s_window, (WindowHandlers) {
    .unload = handle_menu_window_unload,
    .appear = handle_menu_window_appear,
    .disappear = handle_menu_window_disappear,
  });
  window_stack_push(s_window, true);
}

void hide_menu(void) {
  window_stack_remove(s_window, true);
}
