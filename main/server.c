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
const static char http_index_hml[] =
    "<!DOCTYPE html>\n"
    "<title>rbclock Cmdline</title>\n"
    "<meta charset='iso-2022-jp'>\n"
    "<h1>rbclock</h1>\n"
    "<h2>cmdline</h2>\n"
    "<form action='./cmd' method='get'>\n"
    "<input type='text' name='q'>\n"
    "<input type='submit' value='submit'>\n"
    "</form>\n"
    "<h2>binary(16)</h2>\n"
    "<form action='./prog' method='post'>\n"
    "<textarea name='q'></textarea>\n"
    "<input type='submit' value='submit'>\n";
const static char http_index_last[] = 
    "</form>\n"
    "</html>\n";

char cmd_raw_data[300];
const int cmd_raw_data_len = 300;
bool prog_updated = false;
char prog_error[400];

uint8_t 
#if defined __GNUC__
__attribute__((aligned(4)))
#elif defined _MSC_VER
__declspec(align(4))
#endif
prog_mrb[prog_size];

#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/api.h"

int copy_to_a_char(int l, char c, char *str, char *dst) {
    int res = 0;
    for (; (str[res] != '\0') &&
           (str[res] != c) &&
           (res < l - 1); res++){
        dst[res] = str[res];
    }
    dst[res] = '\0';
    return res;
}
int c16(char c){
    if ('0' <= c && c <= '9') return c - '0';
    if ('A' <= c && c <= 'F') return c - 'A' + 10;
    if ('a' <= c && c <= 'f') return c - 'a' + 10;
    else return -1;
}
void persent_decode(char* str) {
    int s = 0, t = 0;
    while (str[s] != '\0') {
        if (str[s] == '%') {
            str[t] = c16(str[s+1]) * 16 + c16(str[s+2]);
            s += 2;
        }else{
            str[t] = str[s];
        }
        s++; t++;
    }
    str[t] = '\0';
}
int parse_query(char *str, char* query) {
    if (strlen(str) >= 2) { // /q=Hoge
        str += 3;
        strcpy(query, str);
        persent_decode(query);
        return 0;
    }
    return -1;
}
int parse_prog(char *str, uint8_t *prog) {
    int res = 0;
    while(str[0] != '\0' && c16(str[0]) >= 0) {
        *prog = c16(str[0]) * 16 + c16(str[1]);
        str += 2; prog += 1;
        res += 1;
    }
    return res;
}
char prog_raw_data[prog_size * 2];
bool get(char *path) {
    printf("Get %s\n", path);
    if (strncmp(path, "/restart", 8) == 0) {
        printf("Restarting now.\n");
        fflush(stdout);
        esp_restart();
    }
    if (strncmp(path, "/cmd", 4) == 0) {
        printf("CMD = %s\n", path + 4);
        parse_query(path + 4, cmd_raw_data);
        printf("DATA = %s\n", cmd_raw_data);
    }
    return false;
}
bool post(char *path, char *data) {
    printf("Post %s\n", path);
    printf("Data %s\n", data);
    if (strncmp(path, "/prog", 5) == 0) {
        printf("PROG\n");
        parse_query(data, prog_raw_data);
        printf("DATA = %s\n", prog_raw_data);
        memset(prog_mrb, 0, sizeof(prog_mrb));
        parse_prog(prog_raw_data, prog_mrb);
        printf("PROG = %x %x %x ...\n", prog_mrb[0], prog_mrb[1], prog_mrb[2]);
        prog_updated = true;
    }
    return false;
}

#define path_len 512u
static char path[path_len];
static char header[path_len];
static char buf[prog_size * 3];

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
    //printf("GET BUF");
    //u16_t buflen = netbuf_copy(inbuf, buf, sizeof(buf));

    // strncpy(_mBuffer, buf, buflen);

    /* Is this an HTTP GET command? (only check the first 5 chars, since
    there are other formats for GET, and we're keeping it very simple )*/
    printf("buffer = %s\n", buf);
    if (buflen>=6) {
        if (strncmp(buf, "GET ", 4) == 0) {
            copy_to_a_char(path_len, ' ', buf + 4, path);
            get(path);
        }
        if (strncmp(buf, "POST ", 5) == 0) {
            char *nbuf = buf + 5;
            int path_l = copy_to_a_char(path_len, ' ', nbuf, path);
            printf("PATH = %s\n", path);
            nbuf += path_l + 1;
            assert(nbuf[-1] != '\0');
            while (nbuf[0] != '\n' && nbuf[0] != '\r') {
                nbuf += copy_to_a_char(path_len, '\n', nbuf, header) + 1;
                assert(nbuf[-1] != '\0');
            }
            post(path, nbuf + 1);
        }

      /* Send the HTML header
             * subtract 1 from the size, since we dont send the \0 in the string
             * NETCONN_NOCOPY: our data is const static, so no need to copy it
       */
      printf("PRINT\n");
      netconn_write(conn, http_html_hdr, sizeof(http_html_hdr)-1, NETCONN_NOCOPY);
      netconn_write(conn, http_index_hml, sizeof(http_index_hml)-1, NETCONN_NOCOPY);
      //netconn_write(conn, prog_error, sizeof(prog_error)-1, NETCONN_NOCOPY);
      //netconn_write(conn, http_index_last, sizeof(http_index_last)-1, NETCONN_NOCOPY);
    }

  }
  /* Close the connection (server closes in HTTP) */
  netconn_close(conn);
  printf("CLOSE\n");

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
