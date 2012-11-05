#ifndef __C2E_CAN_ETHERNET_H
#define __C2E_CAN_ETHERNET_H


#define UPDATE_RATE 1000                    // update rate of CAN message count to OLED

#ifndef DHCP_EXPIRE_TIMER_SECS
#define DHCP_EXPIRE_TIMER_SECS 45			// Timeout for DHCP address request
#endif

/* function prototypes */
void CANIntHandler(void);
//int CAN_receive_FIFO(unsigned char *, unsigned long);
void display_ip_address(unsigned long ipaddr, unsigned long ulCol,unsigned long ulRow);									// display an lwIP address
void display_can_statistics(unsigned long msg_count, unsigned long lost_count, unsigned long col, unsigned long row);	// display CAN message statistics
void lwIPHostTimerHandler(void);																						// This function is required by lwIP library to support any host-related timer functions.

#endif
