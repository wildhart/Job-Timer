#include "main.h"

bool settings_changed;

// BEGIN AUTO-GENERATED UI CODE; DO NOT MODIFY
static Window *s_window;
static MenuLayer *s_menulayer;

static void initialise_ui(void) {
  s_window = window_create();
  #ifndef PBL_SDK_3
    window_set_fullscreen(s_window, false);
  #endif
  
  // s_menulayer
  s_menulayer = menu_layer_create(GRect(0, 0, 144, 168));
  menu_layer_set_click_config_onto_window(s_menulayer, s_window);
  layer_add_child(window_get_root_layer(s_window), (Layer *)s_menulayer);
}

static void destroy_ui(void) {
  window_destroy(s_window);
  menu_layer_destroy(s_menulayer);
}
// END AUTO-GENERATED UI CODE

// *****************************************************************************************************
// MENU CALLBACKS
// *****************************************************************************************************

enum { // main menu structure
  MENU_SHOW_CLOCK,
  MENU_AUTO_SORT,
  MENU_HRS_DAY,
  
  NUM_MENU_ROWS
};

static uint16_t menu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
  return NUM_MENU_ROWS;
}

static int16_t menu_get_cell_height_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
  return (cell_index->row == MENU_AUTO_SORT || cell_index->row == MENU_HRS_DAY) ? MENU_HEIGHT_DOUBLE : MENU_HEIGHT_SINGLE;
}

static int16_t menu_get_header_height_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
  return MENU_CELL_BASIC_HEADER_HEIGHT;
}

static void menu_draw_header_callback(GContext* ctx, const Layer *cell_layer, uint16_t section_index, void *data) {
  menu_cell_basic_header_draw(ctx, cell_layer, "Settings");
}

static void menu_cell_draw_setting(GContext* ctx, const Layer *cell_layer, const char *title, const char *setting, const char *hint) {
  GRect bounds = layer_get_frame(cell_layer);
    
  graphics_context_set_text_color(ctx, GColorBlack);
  graphics_draw_text(ctx, title, FONT_GOTHIC_24_BOLD, GRect(4, -4, bounds.size.w-8, 4+18), GTextOverflowModeFill, GTextAlignmentLeft, NULL);
  graphics_draw_text(ctx, setting, FONT_GOTHIC_18_BOLD, GRect(4, 2, bounds.size.w-8, 18), GTextOverflowModeFill, GTextAlignmentRight, NULL);
  graphics_draw_text(ctx, hint, FONT_GOTHIC_18, GRect(4, 20, bounds.size.w-8, 14), GTextOverflowModeFill, GTextAlignmentLeft, NULL);
}

static void menu_draw_row_callback(GContext* ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data) {
  char hrs[6];
  switch (cell_index->row) {
    case MENU_SHOW_CLOCK: menu_cell_draw_setting(ctx, cell_layer, "Show Clock", settings.Show_clock ? "YES" : "NO", NULL); break;
    case MENU_AUTO_SORT: menu_cell_draw_setting(ctx, cell_layer, "Auto Sort", settings.Auto_sort ? "YES" : "NO", "Move active job to top"); break;
    case MENU_HRS_DAY:
      snprintf(hrs,6,"%d.%d",settings.Hrs_day_x10 / 10,settings.Hrs_day_x10 % 10);
      menu_cell_draw_setting(ctx, cell_layer, "Hrs/Day", hrs, "Long press to add 4hrs");
      break;
  }
}

static void menu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
  switch (cell_index->row) {
    case MENU_SHOW_CLOCK:
      main_menu_toggle_clock();
      settings_changed=true;
      menu_layer_reload_data(s_menulayer);
      break;
    case MENU_AUTO_SORT:
      settings.Auto_sort = !settings.Auto_sort;
      settings_changed=true;
      menu_layer_reload_data(s_menulayer);
      break;
    case MENU_HRS_DAY: 
      settings.Hrs_day_x10 = ((settings.Hrs_day_x10 + 5 - 1) % 240) + 1 ;
      settings_changed=true;
      menu_layer_reload_data(s_menulayer);
      break;
  }
}

static void menu_select_long_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
  if (cell_index->row == MENU_HRS_DAY) {
      settings.Hrs_day_x10 = ((settings.Hrs_day_x10 + 40 - 1) % 240) + 1 ;
      settings_changed=true;
      menu_layer_reload_data(s_menulayer);
  }
}

// *****************************************************************************************************
// MAIN
// *****************************************************************************************************

static void handle_window_unload(Window* window) {
  destroy_ui();
  if (settings_changed) {
    main_save_data();
  }
}

void settings_menu_show(void) {
  initialise_ui();
  window_set_window_handlers(s_window, (WindowHandlers) {
    .unload = handle_window_unload,
  });
  // Set all the callbacks for the menu layer
  menu_layer_set_callbacks(s_menulayer, NULL, (MenuLayerCallbacks){
    .get_num_sections = NULL, //menu_get_num_sections_callback,
    .get_num_rows = menu_get_num_rows_callback,
    .get_header_height = menu_get_header_height_callback,
    .get_cell_height = menu_get_cell_height_callback,
    .draw_header = menu_draw_header_callback,
    .draw_row = menu_draw_row_callback,
    .select_click = menu_select_callback,
    .select_long_click = menu_select_long_callback
  });
  window_stack_push(s_window, ANIMATED);
}

void settings_menu_hide(void) {
  window_stack_remove(s_window, ANIMATED);
}
