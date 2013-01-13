#ifndef __C2E_UDP_H
#define __C2E_UDP_H
#include <stdint.h>
#include "lwip/udp.h"

typedef struct
{
	 struct udp_pcb *pcb;
	 struct pbuf *p_out;
	 unsigned char size[4];
	 unsigned char msg_type;
	 unsigned char *msg_out;
} udp_can_struct_t;

void add_gateway(struct ip_addr gw_address);						// add an IP address to the list of known gateways
uint32_t gateway_count(void);										// return the number of known gateways
void display_gw_address(void);							            // display gateway IP addresses
void UDP_send_CAN(unsigned char *data, uint32_t size);
void UDP_broadcast_presence();
void UDP_send_data(tRingBufObject *pt_ring_buf);
void UDP_send_msg(unsigned char *message, uint32_t size, struct ip_addr *ip_address);
void UDP_start_listen(void);
void UDP_receive(void *arg, struct udp_pcb *pcb, struct pbuf *p, struct ip_addr *addr, u16_t port);

#endif