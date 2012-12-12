#include "c2e_events.h"

void enqueue_event(tRingBufObject *ev_buf, unsigned char event)
{
   RingBufWriteOne(ev_buf, event);  
}