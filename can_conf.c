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

#define CAN_FIFO_SIZE   (8 * 8)             // size of FIFO buffers allocated to the CAN controller - 8 messages can each hold a max of 8 bytes
#define CAN_BITRATE 1000000                 // set CAN bitrate to 1Mbps

void can_configure(void)
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
