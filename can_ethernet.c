
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_sysctl.h"
#include "inc/hw_can.h"
#include "inc/hw_types.h"
#include "driverlib/can.h"
#include "driverlib/ethernet.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "utils/lwiplib.h"
#include "utils/ustdlib.h"
#include "drivers/rit128x96x4.h"
#include "can_ethernet.h"


// A set of flags.  0 indicates that a SysTick interrupt has occurred - see SYSTICK_handler
#define FLAG_SYSTICK 0
static volatile unsigned long systick_flag;

// Defines for setting up the system clock.
#define SYSTICKHZ               100
#define SYSTICKMS               (1000 / SYSTICKHZ)
#define SYSTICKUS               (1000000 / SYSTICKHZ)
#define SYSTICKNS               (1000000000 / SYSTICKHZ)

//structure to hold CAN RX and TX data
CAN_struct CAN_data;

volatile unsigned long message_count = 0;		// received message count
volatile unsigned long update_count = 0;		// print updates once this threshold is reached
volatile unsigned long lost_message_count = 0;	// lost CAN message count
static char print_buf[64];

//display an lwIP address
void display_ip_address(unsigned long ipaddr, unsigned long col, unsigned long row)
{
    char buffer[16];
    unsigned char *temp = (unsigned char *)&ipaddr;

    // Convert the IP Address into a string for display purposes
    usprintf(buffer, "%d.%d.%d.%d", temp[0], temp[1], temp[2], temp[3]);

    // Display on OLED
    RIT128x96x4StringDraw(buffer, col, row, 15);
}

void display_can_statistics(unsigned long msg_count, unsigned long lost_count, unsigned long col, unsigned long row)
{
    usprintf(print_buf, "%u / %u  ", lost_count, msg_count);
    RIT128x96x4StringDraw(print_buf, col, row, 15);
}
// SYSTICK interrupt handler
void SYSTICK_handler(void)
{
    HWREGBITW(&systick_flag, FLAG_SYSTICK) = 1;                               // Indicate that a SysTick interrupt has occurred.
    lwIPTimer(SYSTICKMS);                                                     // Call the lwIP timer handler.
}


// This function is required by lwIP library to support any host-related timer functions.
void lwIPHostTimerHandler(void)
{
    static unsigned long last_ip_address = 0;
    unsigned long ip_address;

    ip_address = lwIPLocalIPAddrGet();

    //
    // If IP Address has not yet been assigned, update the display accordingly
    //
    if(ip_address == 0)
    {
        static int col = 6;

        //
        // Update status bar on the display.
        //
        RIT128x96x4Enable(1000000);
        if(col < 12)
        {
            RIT128x96x4StringDraw(" >", 114, 24, 15);
            RIT128x96x4StringDraw("< ", 0, 24, 15);
            RIT128x96x4StringDraw("*",col, 24, 7);
        }
        else
        {
            RIT128x96x4StringDraw(" *",col - 6, 24, 7);
        }

        col += 4;
        if(col > 114)
        {
            col = 6;
            RIT128x96x4StringDraw(" >", 114, 24, 15);
        }
        RIT128x96x4Disable();
    }

    //
    // Check if IP address has changed, and display if it has.
    //
    else if(last_ip_address != ip_address)
    {
        last_ip_address = ip_address;
        RIT128x96x4Enable(1000000);
        RIT128x96x4StringDraw("                       ", 0, 16, 15);
        RIT128x96x4StringDraw("                       ", 0, 24, 15);
        RIT128x96x4StringDraw("IP:   ", 0, 16, 15);
        RIT128x96x4StringDraw("MASK: ", 0, 24, 15);
        RIT128x96x4StringDraw("GW:   ", 0, 32, 15);
        DisplayIPAddress(ip_address, 36, 16);
        ip_address = lwIPLocalNetMaskGet();
        DisplayIPAddress(ip_address, 36, 24);
        ip_address = lwIPLocalGWAddrGet();
        DisplayIPAddress(ip_address, 36, 32);
        RIT128x96x4Disable();
    }
}


// CAN controller interrupt handler.
void CAN_handler(void)
{
    unsigned long status;
    status = CANIntStatus(CAN0_BASE, CAN_INT_STS_CAUSE);                      // Find the cause of the interrupt, 
 	
    if(status <= 8)                                                           // The first eight message objects make up the Transmit message FIFO.
    {
    	CAN_data.bytes_transmitted += 8;	                                  // Increment the number of bytes transmitted.
    }
    else if((status > 8) && (status <= 16))                                   // The second eight message objects make up the Receive message FIFO.
    {
    	message_count += 1;
    	update_count += 1;
        CANMessageGet(CAN0_BASE, status, &CAN_data.rx_msg_object, 1);         // Read the data out and acknowledge that it was read.

        if(CAN_data.rx_msg_object.ulFlags & MSG_OBJ_DATA_LOST)			       // Check to see if there is an indication that some messages were lost.
        {
            lost_message_count += 1;
        }
        /*
        for (int i=0;i<8;i++)
        {
            //ETH_struct.rx_buffer = CAN_data.rx_msg_object.pucMsgData;
            //ETH_struct.rx_buffer++;                                         // move pointer to next mem location to be assigned
            //CAN_data.rx_msg_object.pucMsgData++;                          // move pointer to next mem location to be read
            //CAN_data.bytes_remaining--;                                   // Decrement the expected bytes remaining.
        }
        for (int i=0;i<8;i++)
        {
            usprintf(print_buf, "%u %u %u %u %u %u %u %u", 
                CAN_data.rx_buffer[i*8+0],CAN_data.rx_buffer[i*8+1],CAN_data.rx_buffer[i*8+2],CAN_data.rx_buffer[i*8+3],
                CAN_data.rx_buffer[i*8+4],CAN_data.rx_buffer[i*8+5],CAN_data.rx_buffer[i*8+6],CAN_data.rx_buffer[i*8+7]);
            RIT128x96x4StringDraw(print_buf, 5, 0+i*10, 15);    
        }
        */

        CAN_data.rx_msg_object.pucMsgData += 8;		                    // Advance the read pointer.
        CAN_data.bytes_remaining -= 8;				                    // Decrement the expected bytes remaining.
        //avoid memory filling up
        if(CAN_data.bytes_remaining == 0)
    	{
    		CAN_data.rx_msg_object.pucMsgData = CAN_data.rx_buffer;	    // re-assign pointer to buffer that will hold message data
    		CAN_data.bytes_remaining = CAN_FIFO_SIZE;					    // reset number of bytes expected
    	}
    }
    else
    {
        CANStatusGet(CAN0_BASE, CAN_STS_CONTROL);		                    // status interrupt so read the current status to clear the interrupt
    }
    CANIntClear(CAN0_BASE, status);						                    // Acknowledge the CAN controller interrupt has been handled.
}



// This function configures the receive FIFO and should only be called once.
int CAN_receive_FIFO(unsigned char *data, unsigned long rx_size)
{
	int idx;
    if(rx_size > CAN_FIFO_SIZE)
    {
        return(CAN_FIFO_SIZE);
    }
    
    // Configure the receive message FIFO to accept any messages
    CAN_data.rx_msg_object.ulMsgID = 0;
    CAN_data.rx_msg_object.ulMsgIDMask = 0;		//don't filter out any ID's
    //RIT128x96x4StringDraw("CAN FIFO", 5, 80, 15);

    // Enable interrupts for receveid messages - NB: remember to include ID_FILTER flag
    // TODO: check if CAN_FIFO example from TI has this problem - also check CAN_DEVICE_FIFO
    CAN_data.rx_msg_object.ulFlags = MSG_OBJ_RX_INT_ENABLE | MSG_OBJ_EXTENDED_ID | MSG_OBJ_USE_ID_FILTER | MSG_OBJ_USE_EXT_FILTER;
    
    // Remember the beginning of the FIFO location.
    CAN_data.rx_msg_object.pucMsgData = data;
    
    // Transfer bytes in multiples of eight bytes.
    for(idx=0; idx < (CAN_FIFO_SIZE / 8); idx++)
    {

        // If there are more than eight remaining to be sent then just queue up
        // eight bytes and go on to the next message object(s) for the
        // remaining bytes.
        if(rx_size > 8)
        {
            // The length is always eight as the full buffer is divisible by 8.
            CAN_data.rx_msg_object.ulMsgLen = 8;

            // There are now eight less bytes to receive.
            rx_size -=8;

            // Set the MSG_OBJ_FIFO to indicate that this is not the last data in a chain of FIFO entries.
            CAN_data.rx_msg_object.ulFlags |= MSG_OBJ_FIFO;
            
            // Make sure that all message objects up to the last indicate that they are part of a FIFO.
            CANMessageSet(CAN0_BASE, idx + 9, &CAN_data.rx_msg_object, MSG_OBJ_TYPE_RX);
        }
        else 
        {
            // Get the remaining bytes.
            CAN_data.rx_msg_object.ulMsgLen = rx_size;
            // Clear the MSG_OBJ_FIFO to indicate that this is the last data in a chain of FIFO entries.
            CAN_data.rx_msg_object.ulFlags &= ~MSG_OBJ_FIFO;
            // This is the last message object in a FIFO so don't set the FIFO to indicate that the FIFO ends with this message object.
            CANMessageSet(CAN0_BASE, idx + 9, &CAN_data.rx_msg_object, MSG_OBJ_TYPE_RX);
        }
    }
    return(0);
}

int main(void)
{
	
    // If running on Rev A2 silicon, turn the LDO voltage up to 2.75V.  This is a workaround to allow the PLL to operate reliably.
    if(REVISION_IS_A2)
    {
        SysCtlLDOSet(SYSCTL_LDO_2_75V);
    }
    // Set the clocking to run directly from the PLL at 50MHz.
    SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_8MHZ);
    // Initialize the OLED display to run at 1MHz
    RIT128x96x4Init(1000000);

    
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);                // Configure CAN 0 Pins.
    GPIOPinTypeCAN(GPIO_PORTD_BASE, GPIO_PIN_0 | GPIO_PIN_1);   // Configure CAN 0 Pins.
    SysCtlPeripheralEnable(SYSCTL_PERIPH_CAN0);                 // Enable the CAN controller.
    CANInit(CAN0_BASE);                                         // Reset the state of all the message object and the state of the CAN module to a known state.
    CANBitRateSet(CAN0_BASE, 8000000, CAN_BITRATE);             // Configure the clock rate to the CAN controller at 8MHz and bit rate for the CAN device to CAN_BITRATE
    CANIntEnable(CAN0_BASE, CAN_INT_MASTER | CAN_INT_ERROR);    // Enable interrupts from CAN controller.
    IntEnable(INT_CAN0);                                        // Enable interrupts for the CAN in the NVIC.
    CANEnable(CAN0_BASE);                                       // Take the CAN0 device out of INIT state.    

    
   //assign pointer to buffer that will hold message data
    CAN_data.rx_msg_object.pucMsgData = CAN_data.rx_buffer;

    // Set the total number of bytes expected.
    CAN_data.bytes_remaining = CAN_FIFO_SIZE;

    // Configure the receive message FIFO.
    CAN_receive_FIFO(CAN_data.rx_buffer, CAN_FIFO_SIZE);

    // Enable and Reset the Ethernet Controller.
    //SysCtlPeripheralEnable(SYSCTL_PERIPH_ETH);
    //SysCtlPeripheralReset(SYSCTL_PERIPH_ETH);

    // loop forever
    while (1) 
    {
        //print some info to the OLED
        //NB: this uses up quite a bit of processing cycles, so use it sparingly - it should ideally not be put in a ISR
        if (update_count >= UPDATE_RATE)
        {
            display_can_statistics(message_count,lost_message_count,5,80);
            update_count = 0;                                   //reset the update count
        }
    }

}
