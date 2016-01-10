#include "main.h"

#define N_LAYERS 4
#define BUFFER_LENGTH 6
static uint8_t active_layer;
static uint32_t divisors[N_LAYERS]={0,3600,60,1};
static uint16_t limits[N_LAYERS]={1000,0,60,60};
static uint16_t values[N_LAYERS];
static const char* formats[N_LAYERS]={"%dd","%02d:", "%02d:" ,"%02d"};
static char buffers[N_LAYERS][BUFFER_LENGTH];
static uint8_t job_index;

static Window *s_window;
static GFont s_res_gothic_24_bold;
static GFont s_res_gothic_18;
static GFont s_res_gothic_14;
static ActionBarLayer *s_actionbarlayer;
static TextLayer *s_textlayer_name;
static TextLayer *layers[N_LAYERS];
static TextLayer *s_textlayer_help;

static void action_bar_up_click_handler() {
  values[active_layer]=(values[active_layer]+1) % limits[active_layer];
  snprintf(buffers[active_layer],BUFFER_LENGTH,formats[active_layer],values[active_layer]);
  text_layer_set_text(layers[active_layer], buffers[active_layer]);
}

static void action_bar_down_click_handler() {
  values[active_layer]=(values[active_layer]+limits[active_layer]-1) % limits[active_layer];
  snprintf(buffers[active_layer],BUFFER_LENGTH,formats[active_layer],values[active_layer]);
  text_layer_set_text(layers[active_layer], buffers[active_layer]);
}

static void action_bar_select_click_handler() {
  if (active_layer==N_LAYERS-1) {
    uint32_t seconds=0;
    for (uint8_t l=0; l<N_LAYERS; l++) seconds += values[l]*divisors[l];
    jobs_set_job_seconds(job_index,seconds);
    job_adjust_hide();
    return;
  }
  active_layer=(active_layer+1) % N_LAYERS;
  for (uint8_t l=0; l<N_LAYERS; l++) {
    text_layer_set_background_color(layers[l], (l==active_layer) ? GColorBlack : GColorWhite);
    text_layer_set_text_color(layers[l], (l==active_layer) ? GColorWhite : GColorBlack);
  }
  if (active_layer==N_LAYERS-1) action_bar_layer_set_icon(s_actionbarlayer, BUTTON_ID_SELECT, bitmap_tick);
}

static void action_bar_click_config_provider(void *context) {
  window_single_repeating_click_subscribe(BUTTON_ID_UP, 100, (ClickHandler) action_bar_up_click_handler);
  window_single_repeating_click_subscribe(BUTTON_ID_DOWN, 100, (ClickHandler) action_bar_down_click_handler);
  window_single_click_subscribe(BUTTON_ID_SELECT, (ClickHandler) action_bar_select_click_handler);
}

static void initialise_ui(void) {
  s_window = window_create();
  #ifndef PBL_SDK_3
    window_set_fullscreen(s_window, false);
  #endif
  
  s_res_gothic_24_bold = fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD);
  s_res_gothic_18 = fonts_get_system_font(FONT_KEY_GOTHIC_18);
  s_res_gothic_14 = fonts_get_system_font(FONT_KEY_GOTHIC_14);
  // s_actionbarlayer
  s_actionbarlayer = action_bar_layer_create();
  action_bar_layer_add_to_window(s_actionbarlayer, s_window);
  action_bar_layer_set_background_color(s_actionbarlayer, GColorBlack);
  action_bar_layer_set_icon(s_actionbarlayer, BUTTON_ID_UP, bitmap_add);
  action_bar_layer_set_icon(s_actionbarlayer, BUTTON_ID_SELECT, bitmap_play);
  action_bar_layer_set_icon(s_actionbarlayer, BUTTON_ID_DOWN, bitmap_minus);
  action_bar_layer_set_click_config_provider(s_actionbarlayer, action_bar_click_config_provider);
  layer_add_child(window_get_root_layer(s_window), (Layer *)s_actionbarlayer);
  
  // s_textlayer_name
  s_textlayer_name = text_layer_create(GRect(5, 7, 100, 24));
  text_layer_set_font(s_textlayer_name, s_res_gothic_24_bold);
  layer_add_child(window_get_root_layer(s_window), (Layer *)s_textlayer_name);
  
  // s_textlayer_days
  layers[0] = text_layer_create(GRect(16-6, 60, 30+6, 24));
  text_layer_set_text_alignment(layers[0], GTextAlignmentRight);
  text_layer_set_font(layers[0], s_res_gothic_18);
  layer_add_child(window_get_root_layer(s_window), (Layer *)layers[0]);
  
  for (uint8_t l=1; l<N_LAYERS; l++) {
    layers[l] = text_layer_create(GRect(30+20*l, 60, 20, 24));
    text_layer_set_text_alignment(layers[l], GTextAlignmentCenter);
    text_layer_set_font(layers[l], s_res_gothic_18);
    layer_add_child(window_get_root_layer(s_window), (Layer *)layers[l]);
  }
  
  // s_textlayer_help
  s_textlayer_help = text_layer_create(GRect(5, 118, 109, 34));
  text_layer_set_text(s_textlayer_help, "SHORT press: 1 unit\nLONG press: 5 units");
  text_layer_set_font(s_textlayer_help, s_res_gothic_14);
  layer_add_child(window_get_root_layer(s_window), (Layer *)s_textlayer_help);
}

static void destroy_ui(void) {
  window_destroy(s_window);
  action_bar_layer_destroy(s_actionbarlayer);
  text_layer_destroy(s_textlayer_name);
  text_layer_destroy(s_textlayer_help);
  
  for (uint8_t l=0; l<N_LAYERS; l++) text_layer_destroy(layers[l]);
}

static void handle_window_unload(Window* window) {
  destroy_ui();
}

void job_adjust_show(uint8_t index) {
  initialise_ui();
  window_set_window_handlers(s_window, (WindowHandlers) {
    .unload = handle_window_unload,
  });
  job_index=index;
  text_layer_set_text(s_textlayer_name, jobs_get_job_name(index));
  active_layer=0;
  uint32_t seconds=jobs_get_job_seconds(job_index);
  divisors[0]=settings.Hrs_day_x10 * 360;
  limits[1]=settings.Hrs_day_x10 / 10;
  for (uint8_t l=0; l<N_LAYERS; l++) {
    text_layer_set_background_color(layers[l], (l==active_layer) ? GColorBlack : GColorWhite);
    text_layer_set_text_color(layers[l], (l==active_layer) ? GColorWhite : GColorBlack);
    values[l]=seconds/divisors[l];
    snprintf(buffers[l],BUFFER_LENGTH,formats[l],values[l]);
    text_layer_set_text(layers[l], buffers[l]);
    seconds %= divisors[l];
  }
  
  window_stack_push(s_window, true);
}

void job_adjust_hide(void) {
  window_stack_remove(s_window, true);
}
