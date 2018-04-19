#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "soc/timer_group_struct.h"
#include "driver/periph_ctrl.h"
#include "driver/timer.h"
#include "driver/gpio.h"
#include "esp_system.h"
#include "esp_types.h"
#include "esp_spi_flash.h"

#include "mruby.h"
#include "mruby/irep.h"
#include "mruby/compile.h"
#include "mruby/error.h"
#include "mruby/string.h"
#include "mruby/value.h"

/* required arguments */
#define ARGS_REQ(n)     (((n)&0x1f) << 19)
/* optional arguments */
#define ARGS_OPT(n)     (((n)&0x1f) << 14)
/* rest argument */
#define ARGS_REST()     (1 << 13)
/* required arguments after rest */
#define ARGS_POST(n)    (((n)&0x1f) << 8)
/* keyword arguments (n of keys, kdict) */
#define ARGS_KEY(n1,n2) ((((n1)&0x1f) << 3) | ((n2)?(1<<2):0))
/* block argument */
#define ARGS_BLOCK()    (1 << 1)
 
/* accept any number of arguments */
#define ARGS_ANY()      ARGS_REST()
/* accept no arguments */
#define ARGS_NONE()     0


#include "example_mrb.h"

#define TAG "mruby_task"

#include "./wifi.h"
#include "./server.h"
#include "./sntp.h"


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

void info()
{
    printf("Hello world! " __DATE__);

    /* Print chip information */
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    printf("This is ESP32 chip with %d CPU cores, WiFi%s%s, ",
            chip_info.cores,
            (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
            (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");

    printf("silicon revision %d, ", chip_info.revision);

    printf("%dMB %s flash\n", spi_flash_get_chip_size() / (1024 * 1024),
            (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

    printf("PERIOD_MS: %d\n", portTICK_PERIOD_MS);
}

#define TIMER_DIVIDER         16  //  Hardware timer clock divider
#define TIMER_SCALE           (TIMER_BASE_CLK / TIMER_DIVIDER)  // convert counter value to seconds

xQueueHandle timer_queue;

void timer_fn(int timer_idx, void (*fn)(void *), double timer_interval_sec)
{
    timer_config_t config;
    config.divider = TIMER_DIVIDER;
    config.counter_dir = TIMER_COUNT_UP;
    config.counter_en = TIMER_PAUSE;
    config.alarm_en = TIMER_ALARM_EN;
    config.intr_type = TIMER_INTR_LEVEL;
    config.auto_reload = TIMER_AUTORELOAD_EN;
    timer_init(TIMER_GROUP_0, timer_idx, &config);

    timer_set_counter_value(TIMER_GROUP_0, timer_idx, 0);

    timer_set_alarm_value(TIMER_GROUP_0, timer_idx, timer_interval_sec * TIMER_SCALE);
    timer_enable_intr(TIMER_GROUP_0, timer_idx);
    timer_isr_register(TIMER_GROUP_0, timer_idx, fn, 
        (void *) timer_idx, ESP_INTR_FLAG_IRAM, NULL);

    timer_start(TIMER_GROUP_0, timer_idx);
    printf("Set timer\n");
}

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

int r_buf[32][32];
int g_buf[32][32];
int b_buf[32][32];

/*
void IRAM_ATTR cont_timer(void *para) {
    TIMERG0.int_clr_timers.t0 = 1;
    TIMERG0.hw_timer[0].config.alarm_en = 1;
    
    int res = 0;
    xQueueSendFromISR(timer_queue, &res, NULL);
}
*/

void led_init() {
    for (int i = 0; i < LEN(output_pins); i++) {
        gpio_output(output_pins[i]);
        gpio_set_level(output_pins[i], 0);
    }
}

void led_print() {
    
    int line = 0;
    int pwm = 0;

    while(true) {
        for (int x = 0; x < 32; x++) {
            gpio_set_level(R0_PIN, r_buf[line][x] > pwm);
            gpio_set_level(G0_PIN, g_buf[line][x] > pwm);
            gpio_set_level(B0_PIN, b_buf[line][x] > pwm);
            gpio_set_level(R1_PIN, r_buf[line+16][x] > pwm);
            gpio_set_level(G1_PIN, g_buf[line+16][x] > pwm);
            gpio_set_level(B1_PIN, b_buf[line+16][x] > pwm);
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
        //vTaskDelay(0.1 / portTICK_PERIOD_MS);
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
    {0,1, 0,2, 1,3, 2,3, 4,3, 5,3, 1,0, 2,0, 4,0, 5,0, 6,1, 6,2},
    {1,3, 2,3, 4,3, 5,3},
    {0,1, 0,2, 1,3, 2,3, 3,2, 3,1, 4,0, 5,0, 6,1, 6,2},
    {0,1, 0,2, 1,3, 2,3, 3,2, 3,1, 4,3, 5,3, 6,1, 6,2},
    {1,0, 2,0, 1,3, 2,3, 4,3, 5,3, 3,1, 3,2},
    {1,0, 2,0, 3,1, 3,2, 0,1, 0,2, 4,3, 5,3, 6,1, 6,2},
    {1,0, 2,0, 3,1, 3,2, 0,1, 0,2, 4,3, 5,3, 4,0, 5,0, 6,1, 6,2},
    {1,0, 2,0, 1,3, 2,3, 4,3, 5,3, 0,1, 0,2},
    {0,1, 0,2, 1,3, 2,3, 3,1, 3,2, 4,3, 5,3, 1,0, 2,0, 4,0, 5,0, 6,1, 6,2},
    {1,0, 2,0, 3,1, 3,2, 0,1, 0,2, 4,3, 5,3, 6,1, 6,2, 1,3, 2,3},
    {1,1, 5,1}
};

void print_number(int y, int x, int c) {
    int *number = numbers[c];
    for (int t = 0; t < 40; t+=2) {
        if (number[t+1] == 0 && number[t] == 0) break;
        r_buf[y+number[t]][x+number[t+1]]= 10;
    }
}

void update_buf() {
    int i = 0;
    while(true) {
        for (int y = 0; y < 32; y++) {
            for (int x = 0; x < 32; x++) {
                r_buf[y][x] = x%TONE;
                g_buf[y][x] = y%TONE;
                b_buf[y][x] = (x/TONE) + (y/TONE);
            }
        }

/*
        time_t now;
        struct tm time_info;
        get_time(&now, &time_info);
        int sec = time_info.tm_sec,
            min = time_info.tm_min,
            hour = time_info.tm_hour;
        print_number(3,  1,  hour/10);
        print_number(3,  6,  hour%10);
        print_number(12, 10, min/10);
        print_number(12, 15, min%10);
        print_number(21, 19, sec/10);
        print_number(21, 24, sec%10);
    */
        vTaskDelay(500 / portTICK_PERIOD_MS);
        i++;
    }
}

static mrb_value ledset(mrb_state* mrb, mrb_value self) { 
    mrb_int x, y, r, g, b;
    mrb_get_args(mrb, "iiiii", &x, &y, &r, &g, &b);
    r_buf[y][x] = r;
    g_buf[y][x] = g;
    b_buf[y][x] = b;
    return self;
}

void mruby_task(void *pvParameter)
{
  mrb_state *mrb = mrb_open();
  
  struct RClass *Led = mrb_define_module(mrb, "Led");
  mrb_define_class_method(mrb, Led, "set", ledset, MRB_ARGS_REQ(5));

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


void app_main(void)
{
    nvs_flash_init();
    info();

//    initialise_wifi();
/*
    time_t now;
    struct tm time_info;
    get_time(&now, &time_info);
    char strtime[64];
    strftime(strtime, sizeof(strtime), "%F T %T", &time_info);
    printf("%s", strtime);
    */

    led_init();
    xTaskCreatePinnedToCore(led_print, "led_print", 2048, NULL, 5, NULL, 1);
    xTaskCreatePinnedToCore(mruby_task, "mruby_task", 32768, NULL, 5, NULL, 0);
//  xTaskCreatePinnedToCore(update_buf, "update_buf", 2048, NULL, 5, NULL, 0);
//  xTaskCreatePinnedToCore(http_server, "http_server", 2048, NULL, 5, NULL, 0);

    //printf("Restarting now.\n");
    //fflush(stdout);
    //esp_restart();
}
