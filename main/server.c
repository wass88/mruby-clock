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
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/api.h"

#include "./str.h"
#include "./server_mod.h"

#include "server.h"
#include "mruby_task.h"

const static char http_html_hdr[] =
    "HTTP/1.1 200 OK\r\nContent-type: text/html\r\n\r\n";
const static char http_index_hml[] =
    "<!DOCTYPE html>\n"
    "<title>rbclock Cmdline</title>\n"
    "<meta charset='iso-2022-jp'>\n"
    "<h1>rbclock</h1>\n"
    "<h2>cmdline</h2>\n"
    "<form action='./cmd' method='get'>\n"
    "<input type='text' name='q'>\n"
    "<input type='submit' value='submit'>\n"
    "</form>\n";
const static char http_index_last[] = 
    "</form>\n"
    "</html>\n";

//static char buf[prog_size * 3];

static void
http_server_netconn_serve(struct netconn *conn)
{
  struct netbuf *inbuf;
  char *buf;
  u16_t buflen;
  err_t err;
  struct response_t res;
  new_response(&res);

  /* Read the data from the port, blocking if nothing yet there.
   We assume the request (the part we care about) is in one netbuf */
  err = netconn_recv(conn, &inbuf); // TODO: CALL Repeatedly

  if (err == ERR_OK) {
    netbuf_data(inbuf, (void**)&buf, &buflen);
    printf("len = %d, buffer = %s\n", buflen, buf);
    exec_http(buflen, buf, &res);
    while(netbuf_next(inbuf) >= 0){
        printf("NEXT \n");
        netbuf_data(inbuf, (void**)&buf, &buflen);
        exec_next(buflen, buf, &res);
    }
    exec_end(&res);
    /* Send the HTML header
            * subtract 1 from the size, since we dont send the \0 in the string
            * NETCONN_NOCOPY: our data is const static, so no need to copy it
    */
    netconn_write(conn, http_html_hdr, sizeof(http_html_hdr)-1, NETCONN_NOCOPY);
    if (res.res_err) {
        netconn_write(conn, prog_error, strlen(prog_error), NETCONN_NOCOPY);
    }else{
        netconn_write(conn, http_index_hml, sizeof(http_index_hml)-1, NETCONN_NOCOPY);
    }
  }
  /* Close the connection (server closes in HTTP) */
  netconn_close(conn);
  printf("CLOSE\n");
  if (res.restart) {
    esp_restart();
  }

  /* Delete the buffer (netconn_recv gives us ownership,
   so we have to make sure to deallocate the buffer) */
  netbuf_delete(inbuf);
}

void http_server(void *pvParameters) {
    printf("Start HTTP server\n");fflush(stdout);
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
