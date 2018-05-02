#include <stdio.h>
#include <string.h>
#include <time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_types.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/gpio.h"

#include "mruby.h"
#include "mruby/irep.h"
#include "mruby/compile.h"
#include "mruby/error.h"
#include "mruby/string.h"
#include "mruby/value.h"

#include "example_mrb.h"
#include "./font.h"
#include "./fonts.h"
#include "./sntp.h"
#include "./server.h"

#define TAG "mruby_task"

#define R0_PIN 21
#define G0_PIN 23
#define B0_PIN 19

#define R1_PIN 18
#define G1_PIN 22
#define B1_PIN 17

#define A_PIN 13
#define B_PIN 25
#define C_PIN 14
#define D_PIN 33

#define CLK_PIN 27
#define STB_PIN 32
#define OE_PIN 26

#define TONE 8

#define LEN(a) sizeof(a) / sizeof(a[0])

static uint8_t 
#if defined __GNUC__
__attribute__((aligned(4)))
#elif defined _MSC_VER
__declspec(align(4))
#endif
prog_running[prog_size];

int output_pins[] = {
    R0_PIN, G0_PIN, B0_PIN,
    R1_PIN, G1_PIN, B1_PIN,
    A_PIN, B_PIN, C_PIN, D_PIN,
    CLK_PIN, STB_PIN, OE_PIN
};

bool gpio_output(int pin) {
    gpio_config_t io_conf;
    //disable interrupt
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    //set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    //bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = 1ULL << pin;
    //disable pull-down mode
    io_conf.pull_down_en = 0;
    //disable pull-up mode
    io_conf.pull_up_en = 0;
    //configure GPIO with the given settings
   return gpio_config(&io_conf) == ESP_OK;
}

void led_init(void) {
    for (int i = 0; i < LEN(output_pins); i++) {
        gpio_output(output_pins[i]);
        gpio_set_level(output_pins[i], 0);
    }
    gpio_set_level(OE_PIN, 1);
}


time_t time_now_ = -1;
struct tm time_info;
static mrb_value time_update(mrb_state* mrb, mrb_value self) { 
  get_time(&time_now_, &time_info);
  return self;
}

static mrb_value time_now(mrb_state* mrb, mrb_value self) { 
  return mrb_fixnum_value(time_now_);
}

static mrb_value time_str(mrb_state* mrb, mrb_value self) { 
  char *s;
  mrb_get_args(mrb, "z", &s);
  const int max_res = 100;
  char res[max_res];
  strftime(res, max_res, s, &time_info);
  return mrb_str_new_cstr(mrb, res); 
}

static mrb_value time_num(mrb_state* mrb, mrb_value self) { 
  int i;
  mrb_get_args(mrb, "i", &i);
  switch (i){
    case 0: return mrb_fixnum_value(time_info.tm_year);
    case 1: return mrb_fixnum_value(time_info.tm_mon);
    case 2: return mrb_fixnum_value(time_info.tm_wday);
    case 3: return mrb_fixnum_value(time_info.tm_hour);
    case 4: return mrb_fixnum_value(time_info.tm_min);
    case 5: return mrb_fixnum_value(time_info.tm_sec);
  }
  char * msg = "Out of index";
  mrb_f_raise(mrb, mrb_str_new_cstr(mrb, msg));
  return mrb_nil_value();
}


#define SIZE 32
#define CHAN 3
#define BSIZE SIZE*SIZE*CHAN

int buffer[BSIZE];
int display[BSIZE];
int ac(int x, int y, int c) { return y * (SIZE * CHAN) + x * (CHAN) + c; }

struct bitmap_font const * const font_list[] =  {
  &font_tom_thumb, &font_5x7, &font_6x9, &font_3x8,
  &font_k6x8, &font_k8x8
};

void b_flash() {
  memcpy(display, buffer, BSIZE * sizeof(int));
}

void b_clear(int c) {
  memset(buffer, c, BSIZE * sizeof(int));
}

void b_set(int x, int y, int r, int g, int b) {
  buffer[ac(x,y,0)] = r;
  buffer[ac(x,y,1)] = g;
  buffer[ac(x,y,2)] = b;
}

void b_line(int x0, int y0, int x1, int y1, int r, int g, int b) {
  mrb_int dx = abs(x1-x0), dy = abs(y1-y0),
          sx = (x0 < x1) ? 1 : -1, sy = (y0 < y1) ? 1 : -1,
          err = dx-dy;
 
  while (true){
    b_set(x0, y0, r, g, b);
    if (x0 == x1 && y0 == y1) break;
    mrb_int e2 = 2 * err;
    if (e2 > -dy) {
      err = err - dy;
      x0 = x0 + sx;
    }
    if (e2 < dx) {
      err = err + dx;
      y0 = y0 + sy;
    }
  }
}

void b_char(int x, int y, short chr, int r, int g, int b, const struct bitmap_font *font){
  int idx = w_index(font->Chars, font->Index, chr);
  if(idx < 0) { idx = 1; }
  for (int i = 0; i < font->Height; i++) {
    int line = font->Bitmap[font->Height * idx + i];
    if (y + i < 0 || y + i >= SIZE) continue;
    for (int j = 0; j < font->Width; j++) {
      if (x + j >= 0 && x + j < SIZE && (line & (1 << (7 - j))) > 0) {
        b_set(x + j, y + i, r, g, b);
      }
    }
  }
}

static short tgt[300];
void b_text(int x, int y, char *str, int r, int g, int b, const struct bitmap_font *font) {
  int w = font->Width;
  int start = (x < 0) ? - x / w : 0;
  int len = iso2022_decode(str, tgt);
  for (int i = 0; x + (start + i) * w <= SIZE + w &&
                  start + i < len; i++) {
    b_char(x + (start + i) * w, y, tgt[start + i], r, g, b, font);
  }
}

void b_scroll(int t, int speed, int y, char *str, int r, int g, int b, const struct bitmap_font *font) {
  int len = iso2022_decode(str, tgt);
  int period = (len - 1) * font->Width + SIZE + 1;
  b_text(32 - ((t / speed) % period), y, str, r, g, b, font);
}

void b_show(int t, int speed, int y, char *str, int r, int g, int b, const struct bitmap_font *font) {
  int len = iso2022_decode(str, tgt);
  if (font->Width * len <= SIZE) {
    b_text(0, y, str, r, g, b, font);
  } else {
    b_scroll(t, speed, y, str, r, g, b, font);
  }
}

int p_r = TONE, p_g = TONE, p_b = TONE;
struct bitmap_font const * p_font = &font_tom_thumb;

static mrb_value ledcolor(mrb_state* mrb, mrb_value self) { 
  mrb_int r, g, b;
  mrb_get_args(mrb, "iii", &r, &g, &b);
  p_r = r; p_g = g; p_b = b;
  return self;
}
static mrb_value ledfont(mrb_state* mrb, mrb_value self) { 
  mrb_int f;
  mrb_get_args(mrb, "i", &f);
  p_font = font_list[f];
  return self;
}
static mrb_value ledflash(mrb_state* mrb, mrb_value self) { 
  b_flash();
  return self;
}
static mrb_value ledclear(mrb_state* mrb, mrb_value self) { 
  mrb_int v;
  mrb_get_args(mrb, "i", &v);
  b_clear(v);
  return self;
}
static mrb_value ledset(mrb_state* mrb, mrb_value self) { 
  mrb_int x, y;
  mrb_get_args(mrb, "ii", &x, &y);
  b_set(x, y, p_r, p_g, p_b);
  return self;
}
static mrb_value ledline(mrb_state* mrb, mrb_value self) { 
  mrb_int x0, y0, x1, y1;
  mrb_get_args(mrb, "iiii", &x0, &y0, &x1, &y1);
  b_line(x0, y0, x1, y1, p_r, p_g, p_b);
  return self;
}
static mrb_value ledchar(mrb_state* mrb, mrb_value self) { 
  mrb_int x, y, c;
  mrb_get_args(mrb, "iii", &x, &y, &c);
  b_char(x, y, c, p_r, p_g, p_b, p_font);
  return self;
}
static mrb_value ledtext(mrb_state* mrb, mrb_value self) { 
  mrb_int x, y;
  char *s;
  mrb_get_args(mrb, "iiz", &x, &y, &s);
  b_text(x, y, s, p_r, p_g, p_b, p_font);
  return self;
}
static mrb_value ledscroll(mrb_state* mrb, mrb_value self) { 
  mrb_int t, speed, y;
  char *s;
  mrb_get_args(mrb, "iiiz", &t, &speed, &y, &s);
  b_scroll(t, speed, y, s, p_r, p_g, p_b, p_font);
  return self;
}
static mrb_value ledshow(mrb_state* mrb, mrb_value self) { 
  mrb_int t, speed, y;
  char *s;
  mrb_get_args(mrb, "iiiz", &t, &speed, &y, &s);
  b_show(t, speed, y, s, p_r, p_g, p_b, p_font);
  return self;
}

static mrb_value task_loop(mrb_state* mrb, mrb_value self) { 
  ESP_LOGI(TAG, "%s", "Task_Loop");
  mrb_value block;
  mrb_get_args(mrb, "&", &block);
  ESP_LOGI(TAG, "%s", "Start");
  while (!prog_updated) {
    mrb_value val;
    mrb_yield(mrb, block, val);
    vTaskDelay(30 / portTICK_PERIOD_MS);
  }
  prog_updated = false;
  ESP_LOGI(TAG, "%s", "Task_Loop END");
  return self;
}
static mrb_value task_cmd(mrb_state* mrb, mrb_value self) { 
  return mrb_str_new_cstr(mrb, cmd_raw_data); 
}
static mrb_value task_sleep(mrb_state* mrb, mrb_value self) { 
  vTaskDelay(100 / portTICK_PERIOD_MS);
  return self;
}

void defines(mrb_state *mrb) {
  struct RClass *Led = mrb_define_module(mrb, "Led");
  mrb_define_class_method(mrb, Led, "color", ledcolor, MRB_ARGS_NONE());
  mrb_define_class_method(mrb, Led, "font", ledfont, MRB_ARGS_REQ(1));
  mrb_define_class_method(mrb, Led, "flash", ledflash, MRB_ARGS_NONE());
  mrb_define_class_method(mrb, Led, "clear", ledclear, MRB_ARGS_REQ(1));
  mrb_define_class_method(mrb, Led, "set", ledset, MRB_ARGS_REQ(2));
  mrb_define_class_method(mrb, Led, "line", ledline, MRB_ARGS_REQ(4));
  mrb_define_class_method(mrb, Led, "char", ledchar, MRB_ARGS_REQ(3));
  mrb_define_class_method(mrb, Led, "text", ledtext, MRB_ARGS_REQ(3));
  mrb_define_class_method(mrb, Led, "scroll", ledscroll, MRB_ARGS_REQ(4));
  mrb_define_class_method(mrb, Led, "show", ledshow, MRB_ARGS_REQ(4));

  struct RClass *Time = mrb_define_module(mrb, "Time");
  mrb_define_class_method(mrb, Time, "update", time_update, MRB_ARGS_NONE());
  mrb_define_class_method(mrb, Time, "now", time_now, MRB_ARGS_NONE());
  mrb_define_class_method(mrb, Time, "str", time_str, MRB_ARGS_REQ(1));
  mrb_define_class_method(mrb, Time, "num", time_num, MRB_ARGS_REQ(1));

  struct RClass *Task = mrb_define_module(mrb, "Task");
  mrb_define_class_method(mrb, Task, "loop", task_loop, MRB_ARGS_REQ(1));
  mrb_define_class_method(mrb, Task, "cmd", task_cmd, MRB_ARGS_NONE());
  mrb_define_class_method(mrb, Task, "sleep", task_sleep, MRB_ARGS_NONE());
}

void mruby_task(void *pvParameter) {
  memcpy(prog_mrb, example_mrb, sizeof(example_mrb));
  memcpy(prog_running, prog_mrb, sizeof(prog_running));

    mrb_state *mrb = mrb_open();
    defines(mrb);
  while (1) {

    mrbc_context *context = mrbc_context_new(mrb);
    int ai = mrb_gc_arena_save(mrb);
    ESP_LOGI(TAG, "%s", "Loading binary...");

    //mrb_load_string_cxt(mrb, "puts 'say'", context);
    memcpy(prog_running, prog_mrb, sizeof(prog_running));
    mrb_load_irep_cxt(mrb, prog_running, context);
    if (mrb->exc) {
      char *err = mrb_str_to_cstr(mrb, mrb_inspect(mrb, mrb_obj_value(mrb->exc)));
      ESP_LOGE(TAG, "Exception occurred: %s", err);
      strcpy(prog_error, err);
      mrb->exc = 0;
    } else {
      ESP_LOGI(TAG, "%s", "Success");
    }

    mrb_gc_arena_restore(mrb, ai);
    mrbc_context_free(mrb, context);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
    mrb_close(mrb);

  // This task should never end, even if the
  // script ends.
  //while (1) {
  //}
}

int pwm_num[] = {0, 4, 2, 6, 1, 5, 3, 7};
void led_print(void *pvParameter) {
    
    int line = 0;
    int pwm = 0;

    while(true) {
        for (int x = 0; x < 32; x++) {
            gpio_set_level(R0_PIN, display[ac(x, line, 0)] > pwm_num[pwm]);
            gpio_set_level(G0_PIN, display[ac(x, line, 1)] > pwm_num[pwm]);
            gpio_set_level(B0_PIN, display[ac(x, line, 2)] > pwm_num[pwm]);
            gpio_set_level(R1_PIN, display[ac(x, line+16, 0)] > pwm_num[pwm]);
            gpio_set_level(G1_PIN, display[ac(x, line+16, 1)] > pwm_num[pwm]);
            gpio_set_level(B1_PIN, display[ac(x, line+16, 2)] > pwm_num[pwm]);
            gpio_set_level(CLK_PIN, 1);
            gpio_set_level(CLK_PIN, 0);

        }
        gpio_set_level(OE_PIN, 1);
        gpio_set_level(STB_PIN, 1);
        gpio_set_level(STB_PIN, 0);
        gpio_set_level(A_PIN, (line&1) > 0);
        gpio_set_level(B_PIN, (line&2) > 0);
        gpio_set_level(C_PIN, (line&4) > 0);
        gpio_set_level(D_PIN, (line&8) > 0);
        gpio_set_level(OE_PIN, 0);
        line ++;
        if (line > 15) {
            line = 0;
            pwm ++;
            if (pwm >= TONE) pwm = 0;
        }
        vTaskDelay(1 / portTICK_PERIOD_MS);
    }
}
