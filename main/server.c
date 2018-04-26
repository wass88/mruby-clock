#include <string.h>
#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "freertos/portmacro.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "tcpip_adapter.h"

#include "lwip/err.h"
#include "string.h"

#include "server.h"
#include "mruby_task.h"

const static char http_html_hdr[] =
    "HTTP/1.1 200 OK\r\nContent-type: text/html\r\n\r\n";
const static char http_index_hml[] = "<!DOCTYPE html>"
      "<html>\n"
      "<title>HELLO ESP32</title>\n"
      "<body>\n"
      "<h1>Hello World, from ESP32!</h1>\n"
      "</body>\n"
      "</html>\n";

char weather_data[100];
const int weather_data_len = 100;

#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/api.h"

int copy_to_a_char(char l, char c, char *str, char *dst) {
    int res = 0;
    for (; (str[res] != '\0') &&
           (str[res] != c) &&
           (res < l - 1); res++){
        dst[res] = str[res];
    }
    dst[res] = '\0';
    return res;
}
void decode(char* str) {
    int s = 0, t = 0;
    while (str[s] != '\0') {
        if (str[s] == '%') {
            str[t] = ' ';
            s += 2;
        }else{
            str[t] = str[s];
        }
        s++; t++;
    }
    str[t] = '\0';
}
bool get(char *path) {
    printf("Get %s\n", path);
    if (strncmp(path, "/restart", 8) == 0) {
        printf("RESTART..?");
    }
    if (strncmp(path, "/weather", 8) == 0) {
        printf("WEATHER\n");
        char *query = path + 8;
        if (strlen(query) >= 3) { // /weather?q=Hoge
            query += 3;
            printf("weather data = %s\n", query);
            strcpy(weather_data, query);
            decode(weather_data);
            printf("weather data = %s\n", weather_data);
        }
    }
    return false;
}
bool post(char *path, char *data) {
    return false;
}

static void
http_server_netconn_serve(struct netconn *conn)
{
  struct netbuf *inbuf;
  char *buf;
  u16_t buflen;
  err_t err;

  /* Read the data from the port, blocking if nothing yet there.
   We assume the request (the part we care about) is in one netbuf */
  err = netconn_recv(conn, &inbuf);

  if (err == ERR_OK) {
    netbuf_data(inbuf, (void**)&buf, &buflen);

    // strncpy(_mBuffer, buf, buflen);

    /* Is this an HTTP GET command? (only check the first 5 chars, since
    there are other formats for GET, and we're keeping it very simple )*/
    printf("buffer = %s\n", buf);
    const int path_len = 100;
    char path[path_len];
    if (buflen>=6) {
        if (strncmp(buf, "GET ", 4) == 0) {
            copy_to_a_char(path_len, ' ', buf + 4, path);
            get(path);
        }
        if (strncmp(buf, "POST ", 5) == 0) {
            char * nbuf = buf + 5;
            nbuf += copy_to_a_char(path_len, ' ', nbuf, path) + 1;
            assert(nbuf[-1] != '\0');
            while (nbuf[0] != '\n') {
                nbuf += copy_to_a_char(path_len, '\n', nbuf, path) + 1;
                assert(nbuf[-1] != '\0');
            }
            printf("post %s\n  data = %s", path, nbuf + 1);
            post(path, nbuf + 1);
        }

      /* Send the HTML header
             * subtract 1 from the size, since we dont send the \0 in the string
             * NETCONN_NOCOPY: our data is const static, so no need to copy it
       */
      netconn_write(conn, http_html_hdr, sizeof(http_html_hdr)-1, NETCONN_NOCOPY);
      netconn_write(conn, http_index_hml, sizeof(http_index_hml)-1, NETCONN_NOCOPY);
    }

  }
  /* Close the connection (server closes in HTTP) */
  netconn_close(conn);

  /* Delete the buffer (netconn_recv gives us ownership,
   so we have to make sure to deallocate the buffer) */
  netbuf_delete(inbuf);
}

void http_server(void *pvParameters) {
    struct netconn *conn, *newconn;
    err_t err;
    conn = netconn_new(NETCONN_TCP);
    ESP_ERROR_CHECK(netconn_bind(conn, NULL, 80));
    ESP_ERROR_CHECK(netconn_listen(conn));
    do {
        printf("Start Listen\n");
        err = netconn_accept(conn, &newconn);
        printf("Accepted\n");
        if (err == ERR_OK) {
            http_server_netconn_serve(newconn);
            netconn_delete(newconn);
        }
    } while (err == ERR_OK);
    netconn_close(conn);
    netconn_delete(conn);
}
