#ifndef __CAN_ETHERNET_H
#define __CAN_ETHERNET_H

#define CAN_FIFO_SIZE   (8 * 8)              // size of FIFO buffers allocated to the CAN controller - 8 messages can each hold a max of 8 bytes
#define CAN_BITRATE  1000000                 // set CAN bitrate to 1Mbps
#define UPDATE_RATE 1                        // update rate of CAN message count to OLED

#ifndef DHCP_EXPIRE_TIMER_SECS
#define DHCP_EXPIRE_TIMER_SECS 45			// Timeout for DHCP address request
#endif


/* variables */
// structure to hold CAN RX and TX data
typedef struct 
{
    tCANMsgObject rx_msg_object;			// RX object
    unsigned char rx_buffer[CAN_FIFO_SIZE];	// RX buffer

    tCANMsgObject tx_msg_object;			// TX object
    unsigned char tx_buffer[CAN_FIFO_SIZE];	// TX buffer

    unsigned long bytes_remaining;
    unsigned long bytes_transmitted;
} CAN_struct;

/* function prototypes */
void CANIntHandler(void);
int CAN_receive_FIFO(unsigned char *, unsigned long);
void display_ip_address(unsigned long ipaddr, unsigned long ulCol,unsigned long ulRow);									// display an lwIP address
void display_can_statistics(unsigned long msg_count, unsigned long lost_count, unsigned long col, unsigned long row);	// display CAN message statistics
void lwIPHostTimerHandler(void);																						// This function is required by lwIP library to support any host-related timer functions.

#endif
