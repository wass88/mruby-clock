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

/*
	base64.c - by Joe DF (joedf@ahkscript.org)
	Released under the MIT License
	
	See "base64.h", for more information.
	
	Thank you for inspiration:
	http://www.codeproject.com/Tips/813146/Fast-base-functions-for-encode-decode
*/

//Base64 char table - used internally for encoding
unsigned char b64_chr[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

unsigned int b64_int(unsigned int ch) {

	// ASCII to base64_int
	// 65-90  Upper Case  >>  0-25
	// 97-122 Lower Case  >>  26-51
	// 48-57  Numbers     >>  52-61
	// 43     Plus (+)    >>  62
	// 47     Slash (/)   >>  63
	// 61     Equal (=)   >>  64~
	if (ch==43)
	return 62;
	if (ch==47)
	return 63;
	if (ch==61)
	return 64;
	if ((ch>47) && (ch<58))
	return ch + 4;
	if ((ch>64) && (ch<91))
	return ch - 'A';
	if ((ch>96) && (ch<123))
	return (ch - 'a') + 26;
	return 0;
}

unsigned int b64e_size(unsigned int in_size) {

	// size equals 4*floor((1/3)*(in_size+2));
	int i, j = 0;
	for (i=0;i<in_size;i++) {
		if (i % 3 == 0)
		j += 1;
	}
	return (4*j);
}

unsigned int b64d_size(unsigned int in_size) {

	return ((3*in_size)/4);
}

unsigned int b64_encode(const unsigned int* in, unsigned int in_len, unsigned char* out) {

	unsigned int i=0, j=0, k=0, s[3];
	
	for (i=0;i<in_len;i++) {
		s[j++]=*(in+i);
		if (j==3) {
			out[k+0] = b64_chr[ (s[0]&255)>>2 ];
			out[k+1] = b64_chr[ ((s[0]&0x03)<<4)+((s[1]&0xF0)>>4) ];
			out[k+2] = b64_chr[ ((s[1]&0x0F)<<2)+((s[2]&0xC0)>>6) ];
			out[k+3] = b64_chr[ s[2]&0x3F ];
			j=0; k+=4;
		}
	}
	
	if (j) {
		if (j==1)
			s[1] = 0;
		out[k+0] = b64_chr[ (s[0]&255)>>2 ];
		out[k+1] = b64_chr[ ((s[0]&0x03)<<4)+((s[1]&0xF0)>>4) ];
		if (j==2)
			out[k+2] = b64_chr[ ((s[1]&0x0F)<<2) ];
		else
			out[k+2] = '=';
		out[k+3] = '=';
		k+=4;
	}

	out[k] = '\0';
	
	return k;
}

unsigned int b64_decode(const unsigned char* in, unsigned int in_len, unsigned int* out) {

	unsigned int i=0, j=0, k=0, s[4];
	
	for (i=0;i<in_len;i++) {
		s[j++]=b64_int(*(in+i));
		if (j==4) {
			out[k+0] = ((s[0]&255)<<2)+((s[1]&0x30)>>4);
			if (s[2]!=64) {
				out[k+1] = ((s[1]&0x0F)<<4)+((s[2]&0x3C)>>2);
				if ((s[3]!=64)) {
					out[k+2] = ((s[2]&0x03)<<6)+(s[3]); k+=3;
				} else {
					k+=2;
				}
			} else {
				k+=1;
			}
			j=0;
		}
	}
	
	return k;
}