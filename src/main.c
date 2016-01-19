#include "main.h"

#include "pebble_process_info.h"
extern const PebbleProcessInfo __pbl_app_info;
#define APP_VERSION_LENGTH 10
char app_version[APP_VERSION_LENGTH];

Timer timer={0, 0, 0};
Settings settings;

GBitmap *bitmap_matrix;
GBitmap *bitmap_pause;
GBitmap *bitmap_play;
GBitmap *bitmap_add;
GBitmap *bitmap_settings;
GBitmap *bitmap_delete;
GBitmap *bitmap_edit;
GBitmap *bitmap_adjust;
GBitmap *bitmap_reset;
GBitmap *bitmap_minus;
GBitmap *bitmap_tick;
GBitmap *bitmap_export;

static bool JS_ready = false;
static bool data_loaded_from_watch = false;
uint8_t stored_version=0;
bool export_after_save=false;

// *****************************************************************************************************
// MESSAGES
// *****************************************************************************************************

#define KEY_CONFIG_DATA  0 
#define KEY_VERSION      1
#define KEY_TIMER        2
#define KEY_SHOW_CLOCK   3
#define KEY_AUTO_SORT    4
#define KEY_HRS_PER_DAY  5
#define KEY_EXPORT       6
#define KEY_APP_VERSION  7
#define KEY_LAST_RESET   8
#define KEY_JOBS       100

static void send_settings_to_phone() {
  if (!JS_ready) return;
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);
  int dummy_int;
  
  dict_write_cstring(iter, KEY_APP_VERSION, app_version);
  dummy_int=CURRENT_STORAGE_VERSION; dict_write_int(iter, KEY_VERSION, &dummy_int, sizeof(int), true);
  dummy_int=settings.Show_clock;     dict_write_int(iter, KEY_SHOW_CLOCK, &dummy_int, sizeof(int), true);
  dummy_int=settings.Auto_sort;      dict_write_int(iter, KEY_AUTO_SORT, &dummy_int, sizeof(int), true);
  dummy_int=settings.Hrs_day_x10;    dict_write_int(iter, KEY_HRS_PER_DAY, &dummy_int, sizeof(int), true);
  dummy_int=settings.Last_reset;     dict_write_int(iter, KEY_LAST_RESET, &dummy_int, sizeof(int), true);
  
  if (timer.Active) dict_write_data(iter, KEY_TIMER, (void*) &timer, sizeof(timer));
  jobs_list_write_dict(iter, KEY_JOBS);

  if (export_after_save) {
    dummy_int=true;
    dict_write_int(iter, KEY_EXPORT, &dummy_int, sizeof(int), true);
    export_after_save=false;
  }
  
  dict_write_end(iter);
  LOG("sending outbox...");
  app_message_outbox_send();
}

static void inbox_received_handler(DictionaryIterator *iter, void *context) {
  LOG("Inbox received...");
  JS_ready = true;
  Tuple *tuple_t;
  bool new_data_from_config_page = dict_find(iter, KEY_CONFIG_DATA);
  
  tuple_t=dict_find(iter, KEY_VERSION);     stored_version = (tuple_t) ? tuple_t->value->int32 : 1;
  tuple_t=dict_find(iter, KEY_SHOW_CLOCK);  if (tuple_t && settings.Show_clock != (tuple_t->value->int8 > 0) ) main_menu_toggle_clock();
  tuple_t=dict_find(iter, KEY_AUTO_SORT);   if (tuple_t) settings.Auto_sort = tuple_t->value->int8 > 0;
  tuple_t=dict_find(iter, KEY_HRS_PER_DAY); if (tuple_t) settings.Hrs_day_x10 = tuple_t->value->int32;
  tuple_t=dict_find(iter, KEY_LAST_RESET); if (tuple_t) settings.Last_reset = tuple_t->value->int32;
  
  tuple_t=dict_find(iter,KEY_TIMER);
  if (tuple_t) {
    memcpy(&timer, tuple_t->value->data, tuple_t->length);
    tick_timer_service_subscribe(MINUTE_UNIT + SECOND_UNIT, handle_ticktimer_tick);
  } else {
    timer.Active = false;
  }
  if (new_data_from_config_page) jobs_delete_all_jobs();
  jobs_list_read_dict(iter, KEY_JOBS, stored_version);
  
  LOG("Inbox processed.");
  main_menu_update();
  main_menu_highlight_top();
  if (new_data_from_config_page) main_save_data();
  if (stored_version < CURRENT_STORAGE_VERSION) {
    update_show(stored_version);
    stored_version = CURRENT_STORAGE_VERSION;
    send_settings_to_phone();
  }
}

// *****************************************************************************************************
// DATA STORAGE
// *****************************************************************************************************

void main_save_data() {
  data_loaded_from_watch = true;
  persist_write_int(STORAGE_KEY_VERSION, CURRENT_STORAGE_VERSION);
  if (timer.Active) {
    persist_write_data(STORAGE_KEY_TIMER, &timer, sizeof(Timer));
  } else {
    persist_delete(STORAGE_KEY_TIMER);
  }
  persist_write_data(STORAGE_KEY_SETTINGS, &settings, sizeof(Settings));
  jobs_list_save(STORAGE_KEY_FIRST_JOB); 
  send_settings_to_phone();
  settings_menu_hide();
}

static void main_load_data(void) {
  settings.Auto_sort=false;
  settings.Show_clock=true;
  settings.Hrs_day_x10=240;
  settings.Last_reset=time(NULL);
  
  stored_version = persist_read_int(STORAGE_KEY_VERSION); // defaults to 0 if key is missing
  if (stored_version) {
    data_loaded_from_watch = true;
    persist_read_data(STORAGE_KEY_TIMER, &timer, sizeof(Timer));
    if (persist_exists(STORAGE_KEY_SETTINGS)) {
      persist_read_data(STORAGE_KEY_SETTINGS, &settings, sizeof(Settings));
    }
    jobs_list_load(STORAGE_KEY_FIRST_JOB, stored_version);
    if (stored_version < STORAGE_KEY_VERSION) main_save_data();
  }
}

// *****************************************************************************************************
// MAIN
// *****************************************************************************************************

void init(void) {
  snprintf(app_version,APP_VERSION_LENGTH,"%d.%d",__pbl_app_info.process_version.major, __pbl_app_info.process_version.minor);
  
  main_load_data();
  bitmap_matrix=gbitmap_create_with_resource(RESOURCE_ID_IMAGE_ICON_MATRIX);
  bitmap_pause=gbitmap_create_as_sub_bitmap(bitmap_matrix, ICON_RECT_PAUSE);
  bitmap_play=gbitmap_create_as_sub_bitmap(bitmap_matrix, ICON_RECT_PLAY);
  bitmap_add=gbitmap_create_as_sub_bitmap(bitmap_matrix, ICON_RECT_ADD);
  bitmap_settings=gbitmap_create_as_sub_bitmap(bitmap_matrix, ICON_RECT_SETTINGS);
  bitmap_delete=gbitmap_create_as_sub_bitmap(bitmap_matrix, ICON_RECT_DELETE);
  bitmap_edit=gbitmap_create_as_sub_bitmap(bitmap_matrix, ICON_RECT_EDIT);
  bitmap_adjust=gbitmap_create_as_sub_bitmap(bitmap_matrix, ICON_RECT_ADJUST);
  bitmap_reset=gbitmap_create_as_sub_bitmap(bitmap_matrix, ICON_RECT_RESET);
  bitmap_minus=gbitmap_create_as_sub_bitmap(bitmap_matrix, ICON_RECT_MINUS);
  bitmap_tick=gbitmap_create_as_sub_bitmap(bitmap_matrix, ICON_RECT_TICK);
  bitmap_export=gbitmap_create_as_sub_bitmap(bitmap_matrix, ICON_RECT_EXPORT);
  
  app_message_register_inbox_received(inbox_received_handler);
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
  
  main_menu_show();
}

void deinit(void) {
  main_menu_hide();
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
