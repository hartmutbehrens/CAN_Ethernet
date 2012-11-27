#include <string.h>
#include "inc/hw_types.h"
#include "utils/lwiplib.h"
#include "utils/ringbuf.h"
#include "utils/ustdlib.h"
#include "drivers/rit128x96x4.h"
#include "c2e_can.h"
#include "c2e_udp.h"

void UDP_send(tRingBufObject *pt_ring_buf)
{
    struct udp_pcb *pcb;
    struct pbuf *p;

    pcb = udp_new();
    if (!pcb) 
    {
        RIT128x96x4StringDraw("Err: No mem for PCB", 5, 20, 15); 
        return;  
    }
    
    uint32_t size = RingBufUsed(pt_ring_buf);
    p = pbuf_alloc(PBUF_TRANSPORT, size, PBUF_RAM);             // Allocate a pbuf for this data packet.
    if(!p)
    {
        RIT128x96x4StringDraw("Err: No mem for P", 5, 30, 15);
        return;
    }
    udp_bind(pcb, IP_ADDR_ANY, 23);                             // broadcast message for now

    
    RingBufRead(pt_ring_buf, p->payload, size);                 // read ringbuffer contents into data packet
    
    // err_t status = udp_sendto(pcb, p, IP_ADDR_BROADCAST, 23);   // send the message
    udp_sendto(pcb, p, IP_ADDR_BROADCAST, 23);   // send the message
    pbuf_free(p);                                               // Free the pbuf.
    udp_remove(pcb);
}

//*****************************************************************************
//
// This function is called by the lwIP TCP/IP stack when it receives a UDP
// packet from the discovery port.  It produces the response packet, which is
// sent back to the querying client.
//
//*****************************************************************************
void UDP_receive(void *arg, struct udp_pcb *pcb, struct pbuf *p, struct ip_addr *addr, u16_t port)
{
    //unsigned char *pucData;
    //uint32_t ulIdx;

/*
    //
    // Validate the contents of the datagram.
    //
    pucData = p->payload;
    if((p->len != 4) || (pucData[0] != TAG_CMD) || (pucData[1] != 4) ||
       (pucData[2] != CMD_DISCOVER_TARGET) ||
       (pucData[3] != ((0 - TAG_CMD - 4 - CMD_DISCOVER_TARGET) & 0xff)))
    {
        pbuf_free(p);
        return;
    }
*/
    //
    // The incoming pbuf is no longer needed, so free it.
    //
    pbuf_free(p);

    /*
    //
    // Allocate a new pbuf for sending the response.
    //
    p = pbuf_alloc(PBUF_TRANSPORT, sizeof(g_pucLocatorData), PBUF_RAM);
    if(p == NULL)
    {
        return;
    }

    //
    // Calcuate and fill in the checksum on the response packet.
    //
    for(ulIdx = 0, g_pucLocatorData[sizeof(g_pucLocatorData) - 1] = 0;
        ulIdx < (sizeof(g_pucLocatorData) - 1); ulIdx++)
    {
        g_pucLocatorData[sizeof(g_pucLocatorData) - 1] -=
            g_pucLocatorData[ulIdx];
    }

    //
    // Copy the response packet data into the pbuf.
    //
    pucData = p->payload;
    for(ulIdx = 0; ulIdx < sizeof(g_pucLocatorData); ulIdx++)
    {
        pucData[ulIdx] = g_pucLocatorData[ulIdx];
    }
    */

    //
    // Send the response.
    //
    //udp_sendto(pcb, p, addr, port);

    //
    // Free the pbuf.
    //
    //pbuf_free(p);
}