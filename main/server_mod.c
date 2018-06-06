#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "server_mod.h"
#include "str.h"

char cmd_raw_data[300];
const int cmd_raw_data_len = 300;
bool cmd_updated = false;
bool prog_updated = false;
char prog_error[200];

uint8_t 
#if defined __GNUC__
__attribute__((aligned(4)))
#elif defined _MSC_VER
__declspec(align(4))
#endif
prog_mrb[prog_size];
static int prog_mrb_cursor = 0;

void new_response(struct response_t *res) {
    struct response_t a = { .get = false, .post = false,
      .restart = false, .ok = false, .res_err = false };
    * res = a;
}
char prog_raw_data[2048];

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

#define path_len 512u
static char path[path_len];
static char header[path_len];

void get(char *path, struct response_t* res) {
    printf("Get %s\n", path);
    if (strncmp(path, "/restart", 8) == 0) {
        printf("Restarting now.\n");
        fflush(stdout);
        res->restart = true;
        return;
    }
    if (strncmp(path, "/cmd", 4) == 0) {
        printf("CMD = %s\n", path + 4);
        parse_query(path + 4, cmd_raw_data);
        printf("DATA = %s\n", cmd_raw_data);
        cmd_updated = true;
        return;
    }
    if (strncmp(path, "/progend", 8) == 0) {
        printf("PROGEND\n");
        prog_updated = true;
        return;
    }
    if (strncmp(path, "/err", 8) == 0) {
        printf("RETURN ERR\n");
        res->res_err = true;
        return;
    }
    printf("Unknown GET\n");
    res->ok = false;
}
void post(char *path, char *data, struct response_t* res) {
    printf("Post %s\n", path);
    printf("Data %s\n", data);
    if (strncmp(path, "/prog", 5) == 0) {
        printf("PROG\n");
        parse_query(data, prog_raw_data);
        printf("DATA = %s\n", prog_raw_data);
        memset(prog_mrb, 0, sizeof(prog_mrb));
        prog_mrb_cursor = parse_prog(prog_raw_data, prog_mrb);
        printf("PROG = %x %x %x ...\n", prog_mrb[0], prog_mrb[1], prog_mrb[2]);
        return;
    }
    if (strncmp(path, "/appp", 5) == 0) {
        printf("PROGAPP\n");
        parse_query(data, prog_raw_data);
        printf("DATA = %s\n", prog_raw_data);
        prog_mrb_cursor += parse_prog(prog_raw_data, prog_mrb + prog_mrb_cursor);
        return;
    }
    printf("Unknown POST\n");
    res->ok = false;
}

void exec_http(int buflen, char *buf, struct response_t* res){
    if (! (buflen>=6)) {
        res->ok = false;
        return;
    }
    if (strncmp(buf, "GET ", 4) == 0) {
        res->get = true;
        copy_to_a_char(path_len, ' ', buf + 4, path);
        get(path, res);
        return;
    }
    if (strncmp(buf, "POST ", 5) == 0) {
        res->post = true;
        char *nbuf = buf + 5;
        int path_l = copy_to_a_char(path_len, ' ', nbuf, path);
        printf("PATH = %s\n", path);
        nbuf += path_l + 1;
        assert(nbuf[-1] != '\0');
        while (nbuf[0] != '\n' && nbuf[0] != '\r') {
            nbuf += copy_to_a_char(path_len, '\n', nbuf, header) + 1;
            assert(nbuf[-1] != '\0');
        }
        post(path, nbuf + 1, res);
        return;
    }
    printf("Unknown HTTP method");
    res->ok = false;
}

void exec_next(int buflen, char *buf, struct response_t* res) {
    printf("NEXT = %s", buf);
    prog_mrb_cursor += parse_prog(prog_raw_data, prog_mrb + prog_mrb_cursor);
}

void exec_end(struct response_t* res) {
}