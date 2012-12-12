#ifndef __C2E_UTILS_H
#define __C2E_UTILS_H
#include <stdint.h>
#include "inc/hw_types.h"
#include "utils/ringbuf.h"

void uint32_to_uchar(unsigned char *data, uint32_t value);			// convert uint32_t to unsigned char - useful for transmission purposes
void enqueue_event(tRingBufObject *ev_buf, unsigned char event);	// set the next event
#endif