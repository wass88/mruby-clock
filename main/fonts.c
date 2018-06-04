#include "./fonts.h"

int find_index(int len, const unsigned short *idxs, unsigned short chr) {
  int l = -1, r = len;
  while(r - l > 1) {
    int m = (r + l) / 2;
    if (chr <= idxs[m]) r = m;
    else l = m;
  }
  if (r < 0 || r >= len || chr != idxs[r]) return -1;
  return r;
}

int w_index(int len, const unsigned short *idxs, unsigned short chr) {
  int res = find_index(len, idxs, chr);
  if (res != -1) return res;
  return find_index(len, idxs, conv_hz(chr));
}

int iso2022_decode(char* str, short* tgt) {
    int s = 0, t = 0;
    int mode = 0;
    while (str[s] != '\0') {
        if (str[s] == '\e') {
            switch (str[s+1]) {
                case '$': mode = 1; break;
                case '(': mode = 0; break;
                default: tgt[t] = '$'; tgt++;
            }
            s += 3;
            continue;
        }
        if (mode == 0) {
            tgt[t] = str[s];
        }else{
            tgt[t] = str[s] * 256 + str[s + 1];
            s++;
        }
        s++; t++;
    }
    tgt[t] = '\0';
    return t;
}

short conv_hz_table[] = {
    0x222e, 0x222e, 0x222e, 0x222e, 0x222e, 0x222e, 0x222e, 0x222e,
    0x222e, 0x222e, 0x222e, 0x222e, 0x222e, 0x222e, 0x222e, 0x222e,
    0x222e, 0x222e, 0x222e, 0x222e, 0x222e, 0x222e, 0x222e, 0x222e,
    0x222e, 0x222e, 0x222e, 0x222e, 0x222e, 0x222e, 0x222e, 0x222e,
    0x2121, 0x212a, 0x216d, 0x2174, 0x2170, 0x2173, 0x2175, 0x216c,
    0x214a, 0x214b, 0x2176, 0x215c, 0x2124, 0x213e, 0x2125, 0x213f,
    0x2330, 0x2331, 0x2332, 0x2333, 0x2334, 0x2335, 0x2336, 0x2337,
    0x2338, 0x2339, 0x2127, 0x2128, 0x2163, 0x2161, 0x2164, 0x2129,
    0x2177, 0x2341, 0x2342, 0x2343, 0x2344, 0x2345, 0x2346, 0x2347,
    0x2348, 0x2349, 0x234a, 0x234b, 0x234c, 0x234d, 0x234e, 0x234f,
    0x2350, 0x2351, 0x2352, 0x2353, 0x2354, 0x2355, 0x2356, 0x2357,
    0x2358, 0x2359, 0x235a, 0x214e, 0x2140, 0x214f, 0x2130, 0x2132,
    0x212e, 0x2361, 0x2362, 0x2363, 0x2364, 0x2365, 0x2366, 0x2367,
    0x2368, 0x2369, 0x236a, 0x236b, 0x236c, 0x236d, 0x236e, 0x236f,
    0x2370, 0x2371, 0x2372, 0x2373, 0x2374, 0x2375, 0x2376, 0x2377,
    0x2378, 0x2379, 0x237a, 0x2150, 0x2143, 0x2151, 0x2141, 0x222e
};

short conv_hz(short c) {
    if (c < 128) {
        return conv_hz_table[c];
    }
    return -1;

} 