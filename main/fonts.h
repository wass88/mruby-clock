#ifndef L_FONTS_H
#define L_FONTS_H
#include "tom_thumb.h"
#include "3x8.h"
#include "5x7.h"
#include "6x9.h"
#include "k6x8.h"
#include "k8x8.h"
int find_index(int len, const unsigned short *idxs, unsigned short chr);
int w_index(int len, const unsigned short *idxs, unsigned short chr);
short conv_hz(short c);
int iso2022_decode(char* str, short* tgt);


/*
	base64.c - by Joe DF (joedf@ahkscript.org)
	Released under the MIT License
	
	Revision: 2015-06-12 01:26:51
	
	Thank you for inspiration:
	http://www.codeproject.com/Tips/813146/Fast-base-functions-for-encode-decode
*/

#include <stdio.h>

//Base64 char table function - used internally for decoding
unsigned int b64_int(unsigned int ch);

// in_size : the number bytes to be encoded.
// Returns the recommended memory size to be allocated for the output buffer excluding the null byte
unsigned int b64e_size(unsigned int in_size);

// in_size : the number bytes to be decoded.
// Returns the recommended memory size to be allocated for the output buffer
unsigned int b64d_size(unsigned int in_size);

// in : buffer of "raw" binary to be encoded.
// in_len : number of bytes to be encoded.
// out : pointer to buffer with enough memory, user is responsible for memory allocation, receives null-terminated string
// returns size of output including null byte
unsigned int b64_encode(const unsigned int* in, unsigned int in_len, unsigned char* out);

// in : buffer of base64 string to be decoded.
// in_len : number of bytes to be decoded.
// out : pointer to buffer with enough memory, user is responsible for memory allocation, receives "raw" binary
// returns size of output excluding null byte
unsigned int b64_decode(const unsigned char* in, unsigned int in_len, unsigned int* out);

#endif