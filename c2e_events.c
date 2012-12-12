#include "config.h"
#include "c2e_events.h"

void enqueue_event(tRingBufObject *ev_buf, unsigned char event)
{
   RingBufWriteOne(ev_buf, event);  
}

uint32_t get_next_event(tRingBufObject *ev_buf)
{
    if ( RingBufEmpty(ev_buf) )
    {
        return EV_ANY;
    }
    else
    {
        unsigned char event=  RingBufReadOne(ev_buf);
        return event;
    }
}