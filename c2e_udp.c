#include <string.h>
#include "inc/hw_nvic.h"
#include "inc/hw_types.h"
#include "utils/lwiplib.h"
#include "utils/ringbuf.h"
#include "utils/ustdlib.h"
#include "drivers/rit128x96x4.h"
#include "config.h"
#include "c2e_events.h"
#include "c2e_udp.h"
#include "c2e_utils.h"

unsigned char MAGIC_ISO11898_ID[5] = {'1', '1', '8', '9', '8'};           // identifier for broadcast messages
struct ip_addr g_gateways[MAX_CAN_GATEWAYS];
volatile uint32_t g_gw_count = 0;                                          // count of CAN gateways
extern tRingBufObject g_event_ringbuf;                                   // ring buffer to receive state machine events

void UDP_start_listen(void)
{
	struct udp_pcb *pcb;
    pcb = udp_new();
    udp_bind(pcb, IP_ADDR_ANY, UDP_PORT_RX);
    //udp_connect(pcb, IP_ADDR_ANY, UDP_PORT_TX);
    udp_recv(pcb, UDP_receive, NULL);										// set callback for incoming UDP data
    
}

//add a gateway IP address to the list of known gateways
void add_gateway(struct ip_addr gw_address)
{
    
    //first check if we already have this gateway
    for (int i = 0; i < MAX_CAN_GATEWAYS; i++)
    {
        if (g_gateways[i].addr == gw_address.addr)
        {
            return;
        }
    }
    
    // add gateway to array of known gateways, if we have space
    if (g_gw_count < MAX_CAN_GATEWAYS)
    {
        g_gateways[g_gw_count] = gw_address;
        g_gw_count++;
        enqueue_event(&g_event_ringbuf, EV_FOUNDGW);
    }
}

void UDP_send_data(tRingBufObject *pt_ring_buf)
{
    struct udp_pcb *pcb;
    unsigned char *data;
    struct pbuf *p;
   
    pcb = udp_new();
    if (!pcb) 
    {
        RIT128x96x4StringDraw("PCB", 5, 70, 15); 
        return;  
    }
    
    
    uint32_t size = RingBufUsed(pt_ring_buf);
    
    p = pbuf_alloc(PBUF_TRANSPORT, size+5, PBUF_RAM);         // Allocate a pbuf for this data packet.
    if(!p)
    {
        RIT128x96x4StringDraw("P", 30, 70, 15);
        return;
    }
    udp_bind(pcb, IP_ADDR_ANY, UDP_PORT_TX);                  // bind to any address and specified port for TX

    data = (unsigned char *)p->payload;                       // Get a pointer to the data packet.
    data[0] = ST_CANDATA; 
    uint32_to_uchar(&data[1],size);                           // store size of message at data[1..4] 
    RingBufRead(pt_ring_buf, &data[5], size);                 // read ringbuffer contents into data packet
    
    /*
    err_t status = udp_sendto(pcb, p, IP_ADDR_BROADCAST, UDP_PORT_RX);   // send the message
    if (status != 0)
    {
        RIT128x96x4StringDraw("SEND", 45, 30, 15);
        return;
    }
    */
    pbuf_free(p);                                               // Free the pbuf.
    udp_remove(pcb);
}

void UDP_send_msg(unsigned char *message, uint32_t size, struct ip_addr *ip_address)
{
    struct udp_pcb *pcb;
    unsigned char *data;
    struct pbuf *p;
    err_t status = 0;
   
    pcb = udp_new();
    if (!pcb) 
    {
        RIT128x96x4StringDraw("PCB", 5, 70, 15); 
        return;  
    }
    
    p = pbuf_alloc(PBUF_TRANSPORT, size, PBUF_RAM);             // Allocate a pbuf for this data packet.
    if(!p)
    {
        RIT128x96x4StringDraw("P", 30, 70, 15);
        return;
    }

    data = (unsigned char *)p->payload;                      // Get a pointer to the data packet.
    memcpy(&data[0], &message[0], size);

    status = udp_bind(pcb, IP_ADDR_ANY, UDP_PORT_TX);            // listen to any local IP address
    if (status != 0)
    {
        RIT128x96x4StringDraw("BND", 60, 70, 15);
        return;
    } 
    
    status = udp_sendto(pcb, p, ip_address, UDP_PORT_RX);   // send the message to the remote gateway
    if (status != 0)
    {
        RIT128x96x4StringDraw("SEND", 45, 70, 15);
        return;
    }
    
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

    unsigned char *data;
    data = p->payload;
    if (ustrncmp((const char *)data, (const char *)&MAGIC_ISO11898_ID, sizeof(MAGIC_ISO11898_ID)) == 0)
    {
       add_gateway(*addr); 
    }

    
    
    //usprintf(print_buf, "GW PORT: %d", port);
    //RIT128x96x4StringDraw(print_buf, 5, 50, 15);
    

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

    

    //
    // Copy the response packet data into the pbuf.
    //
    /*
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