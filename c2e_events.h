#ifndef __C2E_EVENTS_H
#define __C2E_EVENTS_H
#include <stdint.h>
#include "inc/hw_types.h"
#include "utils/ringbuf.h"

tRingBufObject g_event_ringbuf;                              		// ring buffer to receive state machine events

void enqueue_event(tRingBufObject *ev_buf, unsigned char event);	// set the next event
uint32_t get_next_event(tRingBufObject *ev_buf);					// get the next event

#endif