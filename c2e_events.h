#ifndef __C2E_EVENTS_H
#define __C2E_EVENTS_H
#include <stdint.h>
#include "inc/hw_types.h"
#include "utils/ringbuf.h"

void init_event_buffer(void);										// initialize the event ring buffer
void enqueue_event(unsigned char event);	// set the next event
uint32_t get_next_event(void);					// get the next event

#endif