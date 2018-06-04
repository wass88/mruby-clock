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
    xTaskCreatePinnedToCore(sync_sntp, "sync_sntp", 2048, NULL, 5, NULL, 0);
    xTaskCreatePinnedToCore(mruby_task, "mruby_task", 8192, NULL, 5, NULL, 0);
    xTaskCreatePinnedToCore(http_server, "http_server", 8192, NULL, 5, NULL, 0);
}
