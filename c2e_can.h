#ifndef __C2E_CAN_H__
#define __C2E_CAN_H__
#include <stdint.h>
#include "inc/hw_types.h"
#include "driverlib/can.h"

#define CAN_FIFO_SIZE   (8 * 8)             // size of FIFO buffers allocated to the CAN controller - 8 messages can each hold a max of 8 bytes
#define CAN_BITRATE 1000000                 // set CAN bitrate to 1Mbps

typedef struct 
{
    tCANMsgObject rx_msg_object;			// RX object
    unsigned char rx_buffer[CAN_FIFO_SIZE];	// RX buffer

    tCANMsgObject tx_msg_object;			// TX object
    unsigned char tx_buffer[CAN_FIFO_SIZE];	// TX buffer

    uint32_t bytes_remaining;
    uint32_t bytes_transmitted;
} CAN_struct;								// structure to hold CAN RX and TX data

typedef struct 
{
    unsigned char id[4];
    unsigned char data[8];
    unsigned char ext_flag;
    unsigned char rtr_flag;
} CAN_UDP_struct;							// structure to hold CAN data that will be sent via UDP

void display_CAN_statistics(uint32_t update_rate, uint32_t col, uint32_t row);
void CAN_handler(void);
void CAN_configure(void);
int CAN_receive_FIFO(unsigned char *data, uint32_t rx_size, CAN_struct *CAN_data);

#endif
