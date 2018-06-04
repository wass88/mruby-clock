#include <string.h>
#include <stdint.h>
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