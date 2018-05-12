#include <stdio.h>
#include <string.h>

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

#include "./wifi.h"
#include "./sntp.h"
#include "./server.h"
#include "./mruby_task.h"

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

/*
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
        vTaskDelay(500 / portTICK_PERIOD_MS);
        i++;
    }
}
*/

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    info();

    led_init();
    initialise_wifi();

    time_t now;
    struct tm time_info;
    get_time(&now, &time_info);
    char strtime[64];
    strftime(strtime, sizeof(strtime), "%F T %T", &time_info);
    printf("%s\n", strtime);

    xTaskCreatePinnedToCore(led_print, "led_print", 2048, NULL, 5, NULL, 1);
    xTaskCreatePinnedToCore(mruby_task, "mruby_task", 32768, NULL, 5, NULL, 0);
    xTaskCreatePinnedToCore(http_server, "http_server", 8192, NULL, 5, NULL, 0);

    //printf("Restarting now.\n");
    //fflush(stdout);
    //esp_restart();
}
