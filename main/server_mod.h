#ifndef L_SERVER_H
#define L_SERVER_H
#include <stdbool.h>
#include <stdint.h>
extern char cmd_raw_data[];
extern bool cmd_updated;
extern bool prog_updated;
extern uint8_t prog_mrb[];
#define prog_size 8192
extern char prog_error[];
struct response_t {
    bool get;
    bool post;
    bool restart;
    bool ok;
    bool res_err;
};
void new_response(struct response_t *);
void exec_start();
void exec_http(int, char*, struct response_t*);
void exec_next(int, char*, struct response_t*);
void exec_end(struct response_t*);
#endif