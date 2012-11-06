#ifndef __C2E_CAN_ETHERNET_H
#define __C2E_CAN_ETHERNET_H

#ifndef DHCP_EXPIRE_TIMER_SECS
#define DHCP_EXPIRE_TIMER_SECS 45			// Timeout for DHCP address request
#endif


void display_ip_address(unsigned long ipaddr, unsigned long ulCol,unsigned long ulRow);									// display an lwIP address
void display_can_statistics(unsigned long msg_count, unsigned long lost_count, unsigned long col, unsigned long row);	// display CAN message statistics
void lwIPHostTimerHandler(void);																						// This function is required by lwIP library to support any host-related timer functions.

#endif
