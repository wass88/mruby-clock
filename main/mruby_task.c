#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
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
}

#define SIZE 32
#define CHAN 3
#define BSIZE SIZE*SIZE*CHAN

int buffer[BSIZE];
int display[BSIZE];
int ac(int x, int y, int c) { return y * (SIZE * CHAN) + x * (CHAN) + c; }

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

int find_index(int len, const unsigned short *idxs, unsigned short chr) {
  int l = -1, r = len;
  while(r - l > 1) {
    int m = (r + l) / 2;
    if (chr <= idxs[m]) r = m;
    else l = m;
  }
  if (chr != idxs[r]) return -1;
  return r;
}

void b_char(int x, int y, short chr, int r, int g, int b, const struct bitmap_font *font){
  int idx = find_index(font->Chars, font->Index, chr);
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

void b_text(int x, int y, char *str, int r, int g, int b, const struct bitmap_font *font) {
  int w = font->Width + 1;
  int start = (x < 0) ? - x / w : 0;
  int len = strlen(str);
  for (int i = 0; x + (start + i) * w <= SIZE + w &&
                  start + i < len; i++) {
    b_char(x + (start + i) * w, y, str[start + i], r, g, b, font);
  }
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
  mrb_int x, y, r, g, b;
  mrb_get_args(mrb, "iiiii", &x, &y, &r, &g, &b);
  b_set(x, y, r, g, b);
  return self;
}
static mrb_value ledline(mrb_state* mrb, mrb_value self) { 
  mrb_int x0, y0, x1, y1, r, g, b;
  mrb_get_args(mrb, "iiiiiii", &x0, &y0, &x1, &y1, &r, &g, &b);
  b_line(x0, y0, x1, y1, r, g, b);
  return self;
}
static mrb_value ledchar(mrb_state* mrb, mrb_value self) { 
  mrb_int x, y, c, r, g, b;
  mrb_get_args(mrb, "iiiiii", &x, &y, &c, &r, &g, &b);
  b_char(x, y, c, r, g, b, &font_tom_thumb);
  return self;
}
static mrb_value ledtext(mrb_state* mrb, mrb_value self) { 
  mrb_int x, y, r, g, b;
  char *s;
  mrb_get_args(mrb, "iiziii", &x, &y, &s, &r, &g, &b);
  b_text(x, y, s, r, g, b, &font_tom_thumb);
  return self;
}

void mruby_task(void *pvParameter) {
  mrb_state *mrb = mrb_open();
  
  struct RClass *Led = mrb_define_module(mrb, "Led");
  mrb_define_class_method(mrb, Led, "flash", ledflash, MRB_ARGS_NONE());
  mrb_define_class_method(mrb, Led, "clear", ledclear, MRB_ARGS_REQ(1));
  mrb_define_class_method(mrb, Led, "set", ledset, MRB_ARGS_REQ(5));
  mrb_define_class_method(mrb, Led, "line", ledline, MRB_ARGS_REQ(7));
  mrb_define_class_method(mrb, Led, "char", ledchar, MRB_ARGS_REQ(6));
  mrb_define_class_method(mrb, Led, "text", ledtext, MRB_ARGS_REQ(6));

  mrbc_context *context = mrbc_context_new(mrb);
  int ai = mrb_gc_arena_save(mrb);
  ESP_LOGI(TAG, "%s", "Loading binary...");
  mrb_load_irep_cxt(mrb, example_mrb, context);
  if (mrb->exc) {
    ESP_LOGE(TAG, "Exception occurred: %s", mrb_str_to_cstr(mrb, mrb_inspect(mrb, mrb_obj_value(mrb->exc))));
    mrb->exc = 0;
  } else {
    ESP_LOGI(TAG, "%s", "Success");
  }
  mrb_gc_arena_restore(mrb, ai);
  mrbc_context_free(mrb, context);
  mrb_close(mrb);

  // This task should never end, even if the
  // script ends.
  while (1) {
  }
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
