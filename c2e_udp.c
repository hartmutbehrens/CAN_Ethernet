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

unsigned char C2E_BROADCAST_ID[5] = {'C', '2', 'E', 'B', 'C'};           // identifier for broadcast messages
unsigned char C2E_DATA_ID[5] = {'C', '2', 'E', 'D', 'T'};           // identifier for broadcast messages
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
        status++;
    }
    
    p = pbuf_alloc(PBUF_TRANSPORT, size, PBUF_RAM);             // Allocate a pbuf for this data packet.
    if(!p)
    {
        status++;
    }

    data = (unsigned char *)p->payload;                      // Get a pointer to the data packet.
    memcpy(&data[0], &message[0], size);

    status = udp_bind(pcb, IP_ADDR_ANY, UDP_PORT_TX);       // listen to any local IP address
    status = udp_sendto(pcb, p, ip_address, UDP_PORT_RX);   // send the message to the ip address

    if (status > 0)
    {
        RIT128x96x4StringDraw("UDP TX ERR", 5, 60, 15);    
    }
    
    
    pbuf_free(p);                                               // Free the pbuf.
    udp_remove(pcb);
}

static int message_starts_with(unsigned char *data, unsigned char *start_str)
{
    uint32_t size = sizeof(start_str)/sizeof(*start_str);
    if (ustrncmp((const char *)data, (const char *)start_str, sizeof(size)) == 0)
    {
       return 1;
    }
    return 0;
}




// This function is called by the lwIP TCP/IP stack when it receives a UDP packet from the discovery port.  
// It produces the response packet, which is sent back to the querying client.
void UDP_receive(void *arg, struct udp_pcb *pcb, struct pbuf *p, struct ip_addr *addr, u16_t port)
{

    unsigned char *data;
    data = p->payload;
    if ( message_starts_with(data, C2E_BROADCAST_ID) )
    {
        add_gateway(*addr);                                 // found a gateway, so add it to the list of known gateways
    }
    if ( message_starts_with(data, C2E_DATA_ID) )
    {
    }


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