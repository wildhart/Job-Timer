#include "main.h"

#define MAX_CLOCK_LENGTH 24
char clock_string[MAX_CLOCK_LENGTH];
char last_reset_string[MAX_CLOCK_LENGTH];

static Window *s_window;
static TextLayer *s_textlayer_clock;
static MenuLayer *s_menulayer;

static void initialise_ui(void) {
  s_window = window_create();
  #ifndef PBL_SDK_3
    window_set_fullscreen(s_window, 1);
  #endif
  
  GRect bounds = layer_get_bounds(window_get_root_layer(s_window));
  
  // s_textlayer_clock
  s_textlayer_clock = text_layer_create(GRect(0, -5, bounds.size.w, 35));
  text_layer_set_background_color(s_textlayer_clock, GColorBlack);
  text_layer_set_text_color(s_textlayer_clock, GColorWhite);
  text_layer_set_text_alignment(s_textlayer_clock, GTextAlignmentCenter);
  text_layer_set_font(s_textlayer_clock, FONT_BITHAM_30_BLACK);
  layer_add_child(window_get_root_layer(s_window), (Layer *)s_textlayer_clock);
  if (!settings.Show_clock) layer_set_hidden((Layer *)s_textlayer_clock, HIDDEN);
  
  // s_menulayer
  s_menulayer = menu_layer_create(GRect(0, 30*settings.Show_clock, bounds.size.w, bounds.size.h-30*settings.Show_clock));
  menu_layer_set_click_config_onto_window(s_menulayer, s_window);
  layer_add_child(window_get_root_layer(s_window), (Layer *)s_menulayer);
}

void main_menu_toggle_clock(void) {
  settings.Show_clock = !settings.Show_clock;
  persist_write_data(STORAGE_KEY_SETTINGS, &settings, sizeof(Settings));
  
  GRect bounds = layer_get_bounds(window_get_root_layer(s_window));
  layer_set_frame((Layer *)s_menulayer, GRect(0, 30*settings.Show_clock, bounds.size.w, bounds.size.h-30*settings.Show_clock));
  layer_set_hidden((Layer *)s_textlayer_clock, !settings.Show_clock);
}

static void destroy_ui(void) {
  window_destroy(s_window);
  text_layer_destroy(s_textlayer_clock);
  menu_layer_destroy(s_menulayer);
}

static void main_menu_update_clock(void) {
  clock_copy_time_string(clock_string, MAX_CLOCK_LENGTH);
  text_layer_set_text(s_textlayer_clock, clock_string);
}

void handle_ticktimer_tick(struct tm *tick_time, TimeUnits units_changed) {
  //LOG("tick timer, units changed=%d",(int) units_changed);
  if (units_changed & SECOND_UNIT)  {
    main_menu_update();
    job_menu_update();
  }
  if (units_changed & MINUTE_UNIT) main_menu_update_clock();
}

// *****************************************************************************************************
// MENU CALLBACKS
// *****************************************************************************************************

enum { // main menu structure
  MENU_SECTION_JOBS,
  MENU_SECTION_OTHER,
  
  NUM_MENU_SECTIONS,
  
  MENU_OTHER_ADD=MENU_SECTION_OTHER*100,
  MENU_OTHER_OPTIONS,
  MENU_OTHER_RESET_ALL,
  NUM_MENU_ITEMS_OTHER=3
};

static uint16_t menu_get_num_sections_callback(MenuLayer *menu_layer, void *data) {
  return NUM_MENU_SECTIONS;
}

static uint16_t menu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
  switch (section_index) {
    case MENU_SECTION_JOBS: return jobs_count;
    case MENU_SECTION_OTHER: return NUM_MENU_ITEMS_OTHER;
    default:
      return 0;
  }
}

static int16_t menu_get_cell_height_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
  if (cell_index->section == MENU_SECTION_JOBS || cell_index->row == (MENU_OTHER_RESET_ALL-MENU_SECTION_OTHER*100)) {
    return MENU_HEIGHT_DOUBLE;
  }
  return MENU_HEIGHT_SINGLE;
}


void menu_cell_draw_job(GContext* ctx, const Layer *cell_layer, const uint8_t index) {
  GRect bounds = layer_get_frame(cell_layer);
    
  graphics_context_set_text_color(ctx, GColorBlack);
  graphics_draw_text(ctx, jobs_get_job_name(index), FONT_GOTHIC_24_BOLD, GRect(28, -4, bounds.size.w-28, 4+18), GTextOverflowModeFill, GTextAlignmentLeft, NULL);
  graphics_draw_text(ctx, jobs_get_job_clock_as_text(index, false), FONT_GOTHIC_18, GRect(28, 20, bounds.size.w-28-4, 14), GTextOverflowModeFill, GTextAlignmentRight, NULL);
  
  graphics_draw_bitmap_in_rect(ctx, timer.Active && timer.Job==index ? bitmap_play : bitmap_pause, GRect(6, (bounds.size.h-16)/2, 16, 16));
}

void menu_cell_draw_other(GContext* ctx, const Layer *cell_layer, const char *title, const char *sub_title, GBitmap * icon) {
  GRect bounds = layer_get_frame(cell_layer);
    
  graphics_context_set_text_color(ctx, GColorBlack);
  graphics_draw_text(ctx, title, FONT_GOTHIC_24_BOLD, GRect(28, -4, bounds.size.w-28, 4+18), GTextOverflowModeFill, GTextAlignmentLeft, NULL);
  if (sub_title) graphics_draw_text(ctx, sub_title, FONT_GOTHIC_18, GRect(28, 20, bounds.size.w-28-4, 14), GTextOverflowModeFill, GTextAlignmentLeft, NULL);

  if (icon) graphics_draw_bitmap_in_rect(ctx, icon, GRect(6,(bounds.size.h-16)/2, 16, 16));
}

static void menu_draw_row_callback(GContext* ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data) {
  switch (cell_index->section) {
    case MENU_SECTION_JOBS:
      menu_cell_draw_job(ctx, cell_layer, cell_index->row);
      break;

    default:
      switch (MENU_SECTION_CELL) {
        case MENU_OTHER_ADD: menu_cell_draw_other(ctx, cell_layer, "Add Job", NULL, bitmap_add); break;
        case MENU_OTHER_OPTIONS: menu_cell_draw_other(ctx, cell_layer, "Settings", NULL, bitmap_settings); break;
        case MENU_OTHER_RESET_ALL:
          if (settings.Last_reset) {
            strftime(last_reset_string,MAX_CLOCK_LENGTH, "%a %d/%m %R",localtime(&settings.Last_reset));
          } else {
            strncpy(last_reset_string, "Never reset", MAX_CLOCK_LENGTH);
          }
          menu_cell_draw_other(ctx, cell_layer, "Reset All", last_reset_string, bitmap_reset);
        break;
      }
  }
}

static void menu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
  switch (cell_index->section) {
    case MENU_SECTION_JOBS:
      if (timer.Active) {
        jobs_stop_timer_and_save();
        tick_timer_service_subscribe(MINUTE_UNIT, handle_ticktimer_tick);
        if (timer.Job == cell_index->row) return menu_layer_reload_data(s_menulayer);
      }
      timer.Active = true;
      timer.Start = time(NULL);
      timer.Job = cell_index->row;
      tick_timer_service_subscribe(MINUTE_UNIT + SECOND_UNIT, handle_ticktimer_tick);
      menu_layer_reload_data(s_menulayer);
      if (settings.Auto_sort && cell_index->row) {
        jobs_list_move_to_top(cell_index->row); // doesn't save
        menu_layer_set_selected_index(menu_layer, MenuIndex(MENU_SECTION_JOBS,0), MenuRowAlignTop, ANIMATED);
      }
      main_save_data();
      break;
    default:
      switch (MENU_SECTION_CELL) {
        case MENU_OTHER_ADD: jobs_add_job(); break;
        case MENU_OTHER_OPTIONS: settings_menu_show(); break;
        case MENU_OTHER_RESET_ALL:
          settings.Last_reset=time(NULL);
          for(uint8_t i=0; i<jobs_count; i++) jobs_reset_and_save(i);
          menu_layer_reload_data(s_menulayer);
        break;
      }
  }
}

static void menu_select_long_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
  if (cell_index->section == MENU_SECTION_JOBS) job_menu_show(cell_index->row);
}

// *****************************************************************************************************
// MAIN
// *****************************************************************************************************

static void handle_window_unload(Window* window) {
  destroy_ui();
  s_window=NULL;
}

void main_menu_update(void) {
  if (s_window) menu_layer_reload_data(s_menulayer);
}

void main_menu_show(void) {
  initialise_ui();
  window_set_window_handlers(s_window, (WindowHandlers) {
    .unload = handle_window_unload,
  });
  // Set all the callbacks for the menu layer
  menu_layer_set_callbacks(s_menulayer, NULL, (MenuLayerCallbacks){
    .get_num_sections = menu_get_num_sections_callback,
    .get_num_rows = menu_get_num_rows_callback,
    .get_header_height = NULL, //menu_get_header_height_callback,
    .get_cell_height = menu_get_cell_height_callback,
    .draw_header = NULL, //menu_draw_header_callback,
    .draw_row = menu_draw_row_callback,
    .select_click = menu_select_callback,
    .select_long_click = menu_select_long_callback
  });
  main_menu_update_clock();
  window_stack_push(s_window, ANIMATED);
  tick_timer_service_subscribe(MINUTE_UNIT + timer.Active, handle_ticktimer_tick);
}

void main_menu_hide(void) {
  window_stack_remove(s_window, ANIMATED);
}