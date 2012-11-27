#ifndef __C2E_CONFIG_H
#define __C2E_CONFIG_H
#include <stdint.h>

typedef union 
{
    uint32_t n;
    unsigned char bytes[sizeof(uint32_t)];
} uint32_uchar_t;                           // structure to convert from unsigned long to unsigned char[]. Useful when sending via UDP

#ifndef DHCP_EXPIRE_TIMER_SECS
#define DHCP_EXPIRE_TIMER_SECS 45			// Timeout for DHCP address request
#endif
#define RING_BUF_SIZE (CAN_FRAME_SIZE*16*10)             // size of ring buffer
#define CAN_FIFO_SIZE   (8 * 8)             // size of FIFO buffers allocated to the CAN controller - 8 messages can each hold a max of 8 bytes
#define CAN_FRAME_SIZE 14                   // size of CAN frame to be sent via UDP
#define CAN_BITRATE 1000000                 // set CAN bitrate to 1Mbps
#define EXT_FLAG_POS 12                     // position of CAN message ID flag in CAN frame
#define RTR_FLAG_POS 13                     // position of CAN remote transmission flag in CAN frame
#define TRIGGER_THRESHOLD 1                 // number of received CAN frames required to trigger a UDP send

#endif