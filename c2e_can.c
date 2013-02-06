#include <string.h>
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_nvic.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "utils/ringbuf.h"
#include "utils/ustdlib.h"
#include "drivers/rit128x96x4.h"
#include "config.h"
#include "c2e_can.h"
#include "c2e_udp.h"
#include "c2e_utils.h"

static can_struct_t CAN_data;                                                   // structure to hold CAN RX and TX data
static volatile uint32_t rx_message_count = 0;                                  // CAN RX message count
static volatile uint32_t tx_message_count = 0;                                  // CAN TX message count
static volatile uint32_t update_count = 0;                                      // print CAN updates once this threshold is reached
static volatile uint32_t lost_message_count = 0;                                // lost CAN message count
static char print_buf[PRINT_BUF_SIZE];                                   // buffer for print messages
static unsigned char g_can_rxbuf[CAN_RINGBUF_SIZE];                      // memory for CAN ring buffer
static tRingBufObject g_can_ringbuf;                                            // ring buffer to receive CAN frames

void display_CAN_statistics(void)
{    
    if (update_count >= CAN_UPDATERATE)
    {
        usprintf(print_buf, "CAN TX %u   ", tx_message_count);
        RIT128x96x4StringDraw(print_buf, 5, 70, 15);
        usprintf(print_buf, "CAN RX %u/%u   ", lost_message_count, rx_message_count);
        RIT128x96x4StringDraw(print_buf, 5, 80, 15);
        update_count = 0;                                   // reset the update count
    } 
}

static void write_to_ringbuf(tCANMsgObject *can_object, uint32_t id)                                         // write the CAN data into a ring buffer so that it is ready to be sent via UDP
{
    unsigned char frame[CAN_FRAME_SIZE];
    uint32_to_uchar(&frame[0], can_object->ulMsgID);                                  // convert ID to char so that it is suitable to sending over UDP

    memcpy(&frame[CAN_DATA_POS], &(can_object->pucMsgData[id*8]), 8 );                         // copy CAN data into frame
    frame[EXT_FLAG_POS] = (can_object->ulFlags & MSG_OBJ_EXTENDED_ID) ? 1 : 0;        // flag to indicate whether CAN message is using extended ID's
    frame[RTR_FLAG_POS] = (can_object->ulFlags & MSG_OBJ_REMOTE_FRAME) ? 1 : 0;       // flag to indicate whether CAN frame transmission was requested by remote node
    RingBufWrite(&g_can_ringbuf, &frame[0], CAN_FRAME_SIZE);
}

// CAN controller interrupt handler.
void CAN_handler(void)
{
    uint32_t status = CANIntStatus(CAN0_BASE, CAN_INT_STS_CAUSE);             // Find the cause of the interrupt, status 1-32 = ID of message object with highest priority   
    if(status <= 8)                                                           // The first eight message objects make up the Transmit message FIFO.
    {
        CAN_data.bytes_transmitted += 8;                                      // Increment the number of bytes transmitted.
    }
    else if((status > 8) && (status <= 16))                                   // The second eight message objects make up the Receive message FIFO.
    {
        rx_message_count += 1;
        update_count += 1;
        CANMessageGet(CAN0_BASE, status, &CAN_data.rx_msg_object, 1);         // Read the data out and acknowledge that it was read.

        if(CAN_data.rx_msg_object.ulFlags & MSG_OBJ_DATA_LOST)                // Check to see if there is an indication that some messages were lost.
        {
            lost_message_count += 1;
        }

        write_to_ringbuf(&CAN_data.rx_msg_object, (status - 9) );             // write the CAN data to a ringbuffer for further processing
        
        CAN_data.rx_msg_object.pucMsgData += 8;                               // Advance the read pointer.
        CAN_data.bytes_remaining -= 8;                                        // Decrement the expected bytes remaining.
        
        if(CAN_data.bytes_remaining == 0)                                     // this is to avoid memory filling up
        {
            CAN_data.rx_msg_object.pucMsgData = CAN_data.rx_buffer;           // re-assign pointer to buffer that will hold message data - seems to be necessary to prevent lock-up (presumably due to memory fillup?)
            CAN_data.bytes_remaining = CAN_FIFO_SIZE;                         // reset number of bytes expected
        }
        HWREG(NVIC_INT_CTRL) = NVIC_INT_CTRL_PEND_SV;                         // Trigger PendSV in order to send CAN packets
    }
    else
    {
        CANStatusGet(CAN0_BASE, CAN_STS_CONTROL);                       // status interrupt so read the current status to clear the interrupt
    }
    CANIntClear(CAN0_BASE, status);                                     // Acknowledge the CAN controller interrupt has been handled.
}


uint32_t CAN_init(void)                                                 // Enable the board for CAN processing
{
    RingBufInit(&g_can_ringbuf, g_can_rxbuf, sizeof(g_can_rxbuf));      // initialize ring buffer to receive CAN frames
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);                        // Configure CAN 0 Pins.
    GPIOPinTypeCAN(GPIO_PORTD_BASE, GPIO_PIN_0 | GPIO_PIN_1);           // Configure CAN 0 Pins.
    SysCtlPeripheralEnable(SYSCTL_PERIPH_CAN0);                         // Enable the CAN controller.
    CANInit(CAN0_BASE);                                                 // Reset the state of all the message object and the state of the CAN module to a known state.
    CANBitRateSet(CAN0_BASE, 8000000, CAN_BITRATE);                     // Configure the clock rate to the CAN controller at 8MHz and bit rate for the CAN device to CAN_BITRATE
    CANIntEnable(CAN0_BASE, CAN_INT_MASTER | CAN_INT_ERROR);            // Enable interrupts from CAN controller.
    IntEnable(INT_CAN0);                                                // Enable interrupts for the CAN in the NVIC.
    CANEnable(CAN0_BASE);                                               // Take the CAN0 device out of INIT state.
    CAN_receive_FIFO(CAN_data.rx_buffer, CAN_FIFO_SIZE);     			// Configure the receive message FIFO - this function should only be called once.
    return ST_CANINIT;
}

// This function transmits 8 bytes at a time out on the CAN bus
void CAN_transmit()
{
    CANMessageSet(CAN0_BASE, 1, &CAN_data.tx_msg_object, MSG_OBJ_TYPE_TX);                  // Write out this message object using CAN object 1
    tx_message_count += 1;
    update_count += 1;
}

// This function configures the receive FIFO and should only be called once.
int CAN_receive_FIFO(unsigned char *data, uint32_t rx_size)
{
	int idx;
    if (rx_size > CAN_FIFO_SIZE)
    {
        return(CAN_FIFO_SIZE);                                      // give an indication that only CAN_FIFO_SIZE number of bytes can be received, should a greater amount be requested
    }
    
    CAN_data.bytes_remaining = CAN_FIFO_SIZE;                          // Set the total number of bytes expected.
    CAN_data.rx_msg_object.ulMsgID = 0;                                // Configure the receive message FIFO to accept any messages
    CAN_data.rx_msg_object.ulMsgIDMask = 0;		                    // don't filter out any ID's

    // Enable interrupts for receveid messages - NB: remember to include ID_FILTER flag
    // TODO: check if CAN_FIFO example from TI has this problem - also check CAN_DEVICE_FIFO
    CAN_data.rx_msg_object.ulFlags = MSG_OBJ_RX_INT_ENABLE | MSG_OBJ_EXTENDED_ID | MSG_OBJ_USE_ID_FILTER | MSG_OBJ_USE_EXT_FILTER;
    CAN_data.rx_msg_object.pucMsgData = data;                          // allocate the memory from the beginning of the FIFO location
    
    for( idx=0; idx < (CAN_FIFO_SIZE / 8); idx++ )                        // Transfer bytes in multiples of eight bytes.
    {

        // If there are more than eight remaining to be received then just queue up eight bytes and go on to the next message object(s) for the remaining bytes.
        if( rx_size > 8 )
        {
            CAN_data.rx_msg_object.ulMsgLen = 8;                       // The length is always eight as the full buffer is divisible by 8.
            rx_size -=8;                                                // There are now eight less bytes to receive.
            CAN_data.rx_msg_object.ulFlags |= MSG_OBJ_FIFO;            // Set the MSG_OBJ_FIFO to indicate that this is not the last data in a chain of FIFO entries.
            CANMessageSet(CAN0_BASE, idx + 9, &CAN_data.rx_msg_object, MSG_OBJ_TYPE_RX);   // Make sure that all message objects up to the last indicate that they are part of a FIFO.
        }
        else 
        {
            CAN_data.rx_msg_object.ulMsgLen = rx_size;                 // Get the remaining bytes.
            CAN_data.rx_msg_object.ulFlags &= ~MSG_OBJ_FIFO;           // Clear the MSG_OBJ_FIFO to indicate that this is the last data in a chain of FIFO entries.
            CANMessageSet(CAN0_BASE, idx + 9, &CAN_data.rx_msg_object, MSG_OBJ_TYPE_RX);   // This is the last message object in a FIFO so don't set the FIFO to indicate that the FIFO ends with this message object.
        }
    }
    return(0);
}

void CAN_extract(unsigned char *data)
{
    CAN_data.tx_msg_object.ulMsgID = uchar_to_uint32(&data[CAN_ID_POS]);                                                // set CAN id
    CAN_data.tx_msg_object.ulMsgIDMask = 0;                                                 // don't mask any messages
    CAN_data.tx_msg_object.ulFlags = MSG_OBJ_TX_INT_ENABLE;                                 // enable interrupts for transmitted messages.
    if ( data[EXT_FLAG_POS] )
    {
        CAN_data.tx_msg_object.ulFlags |= MSG_OBJ_EXTENDED_ID;
    }
    if ( data[RTR_FLAG_POS] )
    {
        CAN_data.tx_msg_object.ulFlags |= MSG_OBJ_REMOTE_FRAME;   
    }
    CAN_data.tx_msg_object.ulMsgLen = 8; 
    CAN_data.tx_msg_object.pucMsgData = &data[CAN_DATA_POS];
}

void PENDSV_handler(void)
{
    uint32_t size = RingBufUsed(&g_can_ringbuf);
    unsigned char message[size];                                        // allocate storage for message
    RingBufRead(&g_can_ringbuf, &message[0], size);
    UDP_send_CAN(&message[0], size);
    HWREG(NVIC_INT_CTRL) = NVIC_INT_CTRL_UNPEND_SV;                     // clear PendSV
}
