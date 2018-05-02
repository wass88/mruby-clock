#ifndef L_SERVER_H
#define L_SERVER_H
void http_server(void *pvParameters);
extern char cmd_raw_data[];
extern bool prog_updated;
extern uint8_t prog_mrb[];
#define prog_size 8192
extern char prog_error[];
#endif