#ifndef __C2E_CONFIG_H
#define __C2E_CONFIG_H
#include <stdint.h>

#ifndef DHCP_EXPIRE_TIMER_SECS
#define DHCP_EXPIRE_TIMER_SECS 45			// Timeout for DHCP address request
#endif
#define UDP_PORT_RX 11898
#define UDP_PORT_TX 11899
#define CAN_FRAME_SIZE 14                   // size of CAN frame to be sent via UDP
#define RING_BUF_SIZE (CAN_FRAME_SIZE*160)  // size of ring buffer, accomodate up to 160 frames, probably a bit excessive
#define CAN_FIFO_SIZE   (8 * 8)             // size of FIFO buffers allocated to the CAN controller - 8 messages can each hold a max of 8 bytes
#define CAN_BITRATE 1000000                 // set CAN bitrate to 1Mbps
#define EXT_FLAG_POS 12                     // position of CAN message ID flag in CAN frame
#define RTR_FLAG_POS 13                     // position of CAN remote transmission flag in CAN frame
#define TRIGGER_THRESHOLD 1                 // number of received CAN frames required to trigger a UDP send
#define C2E_REQUEST 1
#define C2E_REPLY 2
#define C2E_DATA 3

#endif