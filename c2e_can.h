#ifndef __C2E_CAN_H__
#define __C2E_CAN_H__
#include <stdint.h>
#include "inc/hw_types.h"
#include "driverlib/can.h"
#include "config.h"


typedef struct 
{
    tCANMsgObject rx_msg_object;			// RX object
    unsigned char rx_buffer[CAN_FIFO_SIZE];	// RX buffer
    tCANMsgObject tx_msg_object;			// TX object
    unsigned char tx_buffer[CAN_FIFO_SIZE];	// TX buffer
    uint32_t bytes_remaining;
    uint32_t bytes_transmitted;
} can_struct_t;								// structure to hold CAN RX and TX data

void display_CAN_statistics(void);
void CAN_handler(void);
uint32_t CAN_init(void);
void CAN_transmit();
int CAN_receive_FIFO(unsigned char *data, uint32_t rx_size);
void CAN_extract(unsigned char *data);                              // extract CAN data from message received via UDP
void PENDSV_handler(void);                                          // PendSV interrup handler - causes UDP packets with embedded CAN frames to be sent. Triggered in SW when RingBuffer reaches a threshold.

#endif
