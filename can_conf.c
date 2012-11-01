#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/can.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "can_conf.h"

void CAN_configure(void)
{
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);                        // Configure CAN 0 Pins.
    GPIOPinTypeCAN(GPIO_PORTD_BASE, GPIO_PIN_0 | GPIO_PIN_1);           // Configure CAN 0 Pins.
    SysCtlPeripheralEnable(SYSCTL_PERIPH_CAN0);                         // Enable the CAN controller.
    CANInit(CAN0_BASE);                                                 // Reset the state of all the message object and the state of the CAN module to a known state.
    CANBitRateSet(CAN0_BASE, 8000000, CAN_BITRATE);                     // Configure the clock rate to the CAN controller at 8MHz and bit rate for the CAN device to CAN_BITRATE
    CANIntEnable(CAN0_BASE, CAN_INT_MASTER | CAN_INT_ERROR);            // Enable interrupts from CAN controller.
    IntEnable(INT_CAN0);                                                // Enable interrupts for the CAN in the NVIC.
    CANEnable(CAN0_BASE);                                               // Take the CAN0 device out of INIT state.    
}

// This function configures the receive FIFO and should only be called once.
int CAN_receive_FIFO(unsigned char *data, unsigned long rx_size, CAN_struct *CAN_data)
{
	int idx;
    if(rx_size > CAN_FIFO_SIZE)
    {
        return(CAN_FIFO_SIZE);
    }
    
    // Configure the receive message FIFO to accept any messages
    CAN_data->rx_msg_object.ulMsgID = 0;
    CAN_data->rx_msg_object.ulMsgIDMask = 0;		//don't filter out any ID's
    //RIT128x96x4StringDraw("CAN FIFO", 5, 80, 15);

    // Enable interrupts for receveid messages - NB: remember to include ID_FILTER flag
    // TODO: check if CAN_FIFO example from TI has this problem - also check CAN_DEVICE_FIFO
    CAN_data->rx_msg_object.ulFlags = MSG_OBJ_RX_INT_ENABLE | MSG_OBJ_EXTENDED_ID | MSG_OBJ_USE_ID_FILTER | MSG_OBJ_USE_EXT_FILTER;
    
    // Remember the beginning of the FIFO location.
    CAN_data->rx_msg_object.pucMsgData = data;
    
    // Transfer bytes in multiples of eight bytes.
    for(idx=0; idx < (CAN_FIFO_SIZE / 8); idx++)
    {

        // If there are more than eight remaining to be sent then just queue up
        // eight bytes and go on to the next message object(s) for the
        // remaining bytes.
        if(rx_size > 8)
        {
            // The length is always eight as the full buffer is divisible by 8.
            CAN_data->rx_msg_object.ulMsgLen = 8;

            // There are now eight less bytes to receive.
            rx_size -=8;

            // Set the MSG_OBJ_FIFO to indicate that this is not the last data in a chain of FIFO entries.
            CAN_data->rx_msg_object.ulFlags |= MSG_OBJ_FIFO;
            
            // Make sure that all message objects up to the last indicate that they are part of a FIFO.
            CANMessageSet(CAN0_BASE, idx + 9, &CAN_data->rx_msg_object, MSG_OBJ_TYPE_RX);
        }
        else 
        {
            // Get the remaining bytes.
            CAN_data->rx_msg_object.ulMsgLen = rx_size;
            // Clear the MSG_OBJ_FIFO to indicate that this is the last data in a chain of FIFO entries.
            CAN_data->rx_msg_object.ulFlags &= ~MSG_OBJ_FIFO;
            // This is the last message object in a FIFO so don't set the FIFO to indicate that the FIFO ends with this message object.
            CANMessageSet(CAN0_BASE, idx + 9, &CAN_data->rx_msg_object, MSG_OBJ_TYPE_RX);
        }
    }
    return(0);
}
