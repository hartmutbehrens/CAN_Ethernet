#ifndef __CAN_CONF_H__
#define __CAN_CONF_H__

#define CAN_FIFO_SIZE   (8 * 8)             // size of FIFO buffers allocated to the CAN controller - 8 messages can each hold a max of 8 bytes
#define CAN_BITRATE 1000000                 // set CAN bitrate to 1Mbps

typedef struct 
{
    tCANMsgObject rx_msg_object;			// RX object
    unsigned char rx_buffer[CAN_FIFO_SIZE];	// RX buffer

    tCANMsgObject tx_msg_object;			// TX object
    unsigned char tx_buffer[CAN_FIFO_SIZE];	// TX buffer

    unsigned long bytes_remaining;
    unsigned long bytes_transmitted;
} CAN_struct;								// structure to hold CAN RX and TX data

void CAN_handler(void);
void CAN_configure(void);
int CAN_receive_FIFO(unsigned char *data, unsigned long rx_size, CAN_struct *CAN_data);

#endif
