#include "c2e_utils.h"

void uint32_to_uchar(unsigned char *data, uint32_t value)
{
    data[0] = value & 0xff;
    data[1] = (value >> 8) & 0xff;
    data[2] = (value >> 16) & 0xff;
    data[3] = (value >> 24) & 0xff;
}

uint32_t uchar_to_uint32(unsigned char *data)
{
	uint32_t value = (data[3] << 24);
	value |= (data[2] << 16);
	value |= (data[1] << 8);
	value |= data[0];
	return value;
}