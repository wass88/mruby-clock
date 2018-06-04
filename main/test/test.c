#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "./str.c"
#include "./server_mod.c"

bool _assert_eq_int(int line, int exp, int real) {
    if (exp != real) {
        printf("line:%d Assert Failed %d != %d\n", line, exp, real);
        return false;
    }
    return true;
}
#define assert_eq_int(e, r) _assert_eq_int(__LINE__, (e), (r))

bool _assert_eq_str(int line, char* exp, char* real) {
    if (strcmp(exp, real) != 0) {
        printf("line:%d Assert Failed %s != %s\n", line, exp, real);
        return false;
    }
    return true;
}
#define assert_eq_str(e, r) _assert_eq_str(__LINE__, (e), (r))

bool _assert_eq_uint8_p(int line, int num, uint8_t* exp, uint8_t* real) {
    for (int i = 0; i < num; i++) {
        if (exp != real) {
            printf("line:%d Assert Failed %d != %d at %d\n", line, exp[i], real[i], i);
            return false;
        }
    }
    return true;
}
#define assert_eq_uint8_p(n, e, r) _assert_eq_uint8_p(__LINE__, (n), (e), (r))

int main(void){
    assert_eq_int(10, c16('A'));
    struct response_t ires = { .get = true, .post = false, .restart = false, .ok = false };
    struct response_t res;
    res = ires;
    exec_http(10, "GET /cmd?q=HOGE", &res);
    res = ires;
    exec_http(10, "POST /prog\r\n\r\nA", &res);
}