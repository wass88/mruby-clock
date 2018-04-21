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

/*
00010203
10    13
20    23
30313233
40    43
50    53
60616263
*/
int numbers[20][40] = {
    {0,1, 0,2, 1,3, 2,3, 4,3, 5,3, 1,0, 2,0, 4,0, 5,0, 6,1, 6,2, -1},
    {1,3, 2,3, 4,3, 5,3, -1},
    {0,1, 0,2, 1,3, 2,3, 3,2, 3,1, 4,0, 5,0, 6,1, 6,2, -1},
    {0,1, 0,2, 1,3, 2,3, 3,2, 3,1, 4,3, 5,3, 6,1, 6,2, -1},
    {1,0, 2,0, 1,3, 2,3, 4,3, 5,3, 3,1, 3,2, -1},
    {1,0, 2,0, 3,1, 3,2, 0,1, 0,2, 4,3, 5,3, 6,1, 6,2, -1},
    {1,0, 2,0, 3,1, 3,2, 0,1, 0,2, 4,3, 5,3, 4,0, 5,0, 6,1, 6,2, -1},
    {1,0, 2,0, 1,3, 2,3, 4,3, 5,3, 0,1, 0,2, -1},
    {0,1, 0,2, 1,3, 2,3, 3,1, 3,2, 4,3, 5,3, 1,0, 2,0, 4,0, 5,0, 6,1, 6,2, -1},
    {1,0, 2,0, 3,1, 3,2, 0,1, 0,2, 4,3, 5,3, 6,1, 6,2, 1,3, 2,3, -1},
    {1,1, 5,1, -1}
};

void print_number(int y, int x, int c) {
    int *number = numbers[c];
    for (int t = 0; t < 40; t+=2) {
        if (number[t+1] < 0) break;
        r_buf[y+number[t]][x+number[t+1]]= 10;
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

void mruby_task(void *pvParameter) {
  mrb_state *mrb = mrb_open();
  
  struct RClass *Led = mrb_define_module(mrb, "Led");
  mrb_define_class_method(mrb, Led, "flash", ledflash, MRB_ARGS_NONE());
  mrb_define_class_method(mrb, Led, "clear", ledclear, MRB_ARGS_REQ(1));
  mrb_define_class_method(mrb, Led, "set", ledset, MRB_ARGS_REQ(5));
  mrb_define_class_method(mrb, Led, "line", ledline, MRB_ARGS_REQ(7));

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
