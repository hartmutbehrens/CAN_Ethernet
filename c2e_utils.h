#ifndef __C2E_UTILS_H
#define __C2E_UTILS_H
#include <stdint.h>

void uint32_to_uchar(unsigned char *data, uint32_t value);			// convert uint32_t to unsigned char - useful for transmission purposes
uint32_t uchar_to_uint32(unsigned char *data);					// convert unsigned char back to uint32_t

#endif