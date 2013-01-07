#include "config.h"
#include "c2e_events.h"

static tRingBufObject g_event_ringbuf;                                     // ring buffer to receive state machine events
static unsigned char g_event_buf[EV_RINGBUF_SIZE];                  // memory for event ring buffer

void init_event_buffer(void)
{
    RingBufInit(&g_event_ringbuf, g_event_buf, sizeof(g_event_buf));        // initialize ring buffer to receive events
}

void enqueue_event(unsigned char event)
{
   RingBufWriteOne(&g_event_ringbuf, event);  
}

uint32_t get_next_event(void)
{
    if ( RingBufEmpty(&g_event_ringbuf) )
    {
        return EV_ANY;
    }
    else
    {
        unsigned char event=  RingBufReadOne(&g_event_ringbuf);
        return event;
    }
}