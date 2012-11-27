#ifndef __C2E_CAN_ETHERNET_H
#define __C2E_CAN_ETHERNET_H
#include "c2e_can.h"

#ifndef DHCP_EXPIRE_TIMER_SECS
#define DHCP_EXPIRE_TIMER_SECS 45			// Timeout for DHCP address request
#define RING_BUF_SIZE (CAN_FRAME_SIZE*16*10)             // size of ring buffer
#endif

void display_ip_address(uint32_t ipaddr, uint32_t ulCol,uint32_t ulRow);							// display an lwIP address
void display_can_statistics(uint32_t msg_count, uint32_t lost_count, uint32_t col, uint32_t row);	// display CAN message statistics
void PENDSV_handler(void);																			// PendSV interrup handler - causes UDP packets with embedded CAN frames to be sent. Triggered in SW when RingBuffer reaches a threshold.

#endif
