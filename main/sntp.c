/* LwIP SNTP example
   This example code is in the Public Domain (or CC0 licensed, at your option.)
   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <sys/reent.h>
#include "sntp.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_attr.h"
#include "esp_sleep.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "apps/sntp/sntp.h"

static const char *TAG = "sntp";

static void initialize_sntp(void)
{
    ESP_LOGI(TAG, "Initializing SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);

    sntp_setservername(0, "133.243.238.243");
    //sntp_set_timezone(9);
    sntp_init();
    
}

static void obtain_time(void)
{
    initialize_sntp();

    // wait for time to be set
    time_t now = 0;
    struct tm timeinfo = { 0 };
    int retry = 0;
    const int retry_count = 10;
    while(timeinfo.tm_year < (2016 - 1900) && ++retry < retry_count) {
        ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        time(&now);
        localtime_r(&now, &timeinfo);
    }
}

void get_time(time_t *now, struct tm *timeinfo) {
    time(now);
    setenv("TZ", "JST-9", 1);
    tzset();
    localtime_r(now, timeinfo);
    // Is time set? If not, tm_year will be (1970 - 1900).
    if (timeinfo->tm_year < (2016 - 1900)) {
        ESP_LOGI(TAG, "Time is not set yet. Connecting to WiFi and getting time over NTP.");
        obtain_time();
        // update 'now' variable with current time
        time(now);
        localtime_r(now, timeinfo);
    }

}

void set_unusual_time(void){
    struct tm tm;
    tm.tm_year = 2017 - 1900;
    tm.tm_mon = 10;
    tm.tm_mday = 10;
    tm.tm_hour = 10;
    tm.tm_min = 10;
    tm.tm_sec = 10;
    time_t t = mktime(&tm);


    printf("Setting time: %s", asctime(&tm));

    struct tm nowtm; time_t nowt;
    get_time(&nowt, &nowtm);
    printf("Setting time: FROM %s", asctime(&nowtm));
    
    struct timeval now = { .tv_sec = t };
    settimeofday(&now, NULL);
    get_time(&nowt, &nowtm);
    printf("Now time: %s", asctime(&nowtm));
    fflush(stdout);
}

void sync_sntp(void *pvParameter) {
    while (true) {
        vTaskDelay(60 * 1000 / portTICK_PERIOD_MS);
        ESP_LOGI(TAG, "Resync Time");
        //set_unusual_time();
        sntp_stop();
        sntp_init();
    }
}