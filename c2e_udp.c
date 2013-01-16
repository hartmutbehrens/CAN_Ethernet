#include <string.h>
#include "inc/hw_nvic.h"
#include "inc/hw_types.h"
#include "utils/lwiplib.h"
#include "utils/ustdlib.h"
#include "drivers/rit128x96x4.h"
#include "config.h"
#include "c2e_can.h"
#include "c2e_events.h"
#include "c2e_udp.h"
#include "c2e_utils.h"

static char print_buf[PRINT_BUF_SIZE];
unsigned char C2E_BROADCAST_ID[5] = {'C', '2', 'E', 'B', 'C'};           // identifier for broadcast messages
unsigned char C2E_DATA_ID[5] = {'C', '2', 'E', 'D', 'T'};           // identifier for broadcast messages
struct ip_addr g_gateways[MAX_CAN_GATEWAYS];
static volatile uint32_t g_gw_count = 0;                                          // count of CAN gateways
static volatile uint32_t udp_tx_count = 0;
static volatile uint32_t udp_rx_count = 0;
static volatile uint32_t update_count = 0;                                      // print CAN updates once this threshold is reached

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
            enqueue_event(EV_BROADCAST);                                    // perhaps the gateay has forgotten about us (due to restart), so send out broadcast message again
            return;
        }
    }
    
    // add gateway to array of known gateways, if we have space
    if (g_gw_count < MAX_CAN_GATEWAYS)
    {
        g_gateways[g_gw_count] = gw_address;
        g_gw_count++;
        enqueue_event(EV_FOUNDGW);
    }
}

uint32_t gateway_count(void)
{
    return g_gw_count;
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
    if (ustrncmp((const char *)data, (const char *)start_str, size) == 0)
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
    if ( message_starts_with(data, C2E_DATA_ID) )           // received a message with CAN data, so send it out on the CAN i/f
    {
        uint32_t position = sizeof(C2E_DATA_ID);
        while (position < p->len)
        {
            CAN_extract(&data[position]);
            CAN_transmit();
            position += CAN_FRAME_SIZE;
        }
        udp_rx_count += 1;
        update_count += 1;
    }
    if ( message_starts_with(data, C2E_BROADCAST_ID) )      // found a gateway, so add the IP address to the list of known gateways
    {
        add_gateway(*addr);
    }
    pbuf_free(p);
}

void UDP_send_CAN(unsigned char *data, uint32_t size)
{
    uint32_t preamble_size = sizeof(C2E_DATA_ID);
    uint32_t total_size = size + preamble_size;
    unsigned char message[total_size];
    udp_tx_count += (size / CAN_FRAME_SIZE);
    update_count += udp_tx_count;
    memcpy(&message[0], &C2E_DATA_ID[0], preamble_size);
    memcpy(&message[preamble_size], &data[0], size);
    for (int i = 0; i < g_gw_count; i++)                                          
    {
        UDP_send_msg(&message[0], total_size, &g_gateways[g_gw_count]);                  // send to gateway
    }
}

void UDP_broadcast_presence()
{
     UDP_send_msg(C2E_BROADCAST_ID, sizeof(C2E_BROADCAST_ID), IP_ADDR_BROADCAST);
}

void display_UDP_statistics(void)
{    
    if (update_count >= UDP_UPDATERATE)
    {
        usprintf(print_buf, "UDP TX %u   ", udp_tx_count);
        RIT128x96x4StringDraw(print_buf, 5, 50, 15);
        usprintf(print_buf, "UDP RX %u   ", udp_rx_count);
        RIT128x96x4StringDraw(print_buf, 5, 60, 15);
        update_count = 0;                                   // reset the update count
    } 
}

// display gateway IP address
void display_gw_address(void)
{
    for (int i = 0; i < g_gw_count; i++)
    {
        unsigned char *temp = (unsigned char *)&g_gateways[i];
        usprintf(print_buf, "GW %d: %d.%d.%d.%d    ", i, temp[0], temp[1], temp[2], temp[3]);               // Convert the IP Address into a string for display purposes
        RIT128x96x4StringDraw(print_buf, 5, 30+i*10, 15);    
    }
}    

