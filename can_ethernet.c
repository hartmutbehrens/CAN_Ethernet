
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_sysctl.h"
#include "inc/hw_can.h"
#include "inc/hw_nvic.h"
#include "inc/hw_types.h"
#include "driverlib/can.h"
#include "driverlib/ethernet.h"
#include "driverlib/flash.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "utils/lwiplib.h"
#include "utils/ustdlib.h"
#include "drivers/rit128x96x4.h"
#include "can_ethernet.h"
#include "can_conf.h"

// A set of flags.  0 indicates that a SysTick interrupt has occurred - see SYSTICK_handler
#define FLAG_SYSTICK 0
static volatile unsigned long systick_flag;

// Defines for setting up the system clock.
#define SYSTICKHZ               100
#define SYSTICKMS               (1000 / SYSTICKHZ)
#define SYSTICKUS               (1000000 / SYSTICKHZ)
#define SYSTICKNS               (1000000000 / SYSTICKHZ)

extern CAN_struct CAN_data;                            // structure to hold CAN RX and TX data
extern volatile unsigned long message_count;		   // CAN received message count
extern volatile unsigned long update_count;		       // print CAN updates once this threshold is reached
extern volatile unsigned long lost_message_count;	   // lost CAN message count
volatile unsigned long ip_displayed = 0;               // only show IP message once
static char print_buf[64];

//display an lwIP address
void display_ip_address(unsigned long ipaddr, unsigned long col, unsigned long row)
{
    char buffer[16];
    unsigned char *temp = (unsigned char *)&ipaddr;

    // Convert the IP Address into a string for display purposes
    usprintf(buffer, "IP: %d.%d.%d.%d", temp[0], temp[1], temp[2], temp[3]);

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
    //HWREGBITW(&systick_flag, FLAG_SYSTICK) = 1;                      // Indicate that a SysTick interrupt has occurred.
    lwIPTimer(SYSTICKMS);                                            // Call the lwIP timer handler - eventually results in lwIPHostTimerHandler being called
}


// This function is required by lwIP library to support any host-related timer functions.
// Define HOST_TMR_INTERVAL in lwipopts.h to use
void lwIPHostTimerHandler(void)
{
    
    static unsigned long ulLastIPAddress = 0;
    unsigned long ulIPAddress;
    ulIPAddress = lwIPLocalIPAddrGet();

    //
    // If IP Address has not yet been assigned, update the display accordingly
    //
    if( (ulLastIPAddress != ulIPAddress) && (!ip_displayed) )
    {
        ulLastIPAddress = ulIPAddress;
        //RIT128x96x4StringDraw("IP:   ", 0, 16, 15);
        //RIT128x96x4StringDraw("MASK: ", 0, 24, 15);
        //RIT128x96x4StringDraw("GW:   ", 0, 32, 15);
        //display_ip_address(ulIPAddress, 0, 16);
        //ulIPAddress = lwIPLocalNetMaskGet();
        //display_ip_address(ulIPAddress, 36, 24);
        //ulIPAddress = lwIPLocalGWAddrGet();
        //display_ip_address(ulIPAddress, 36, 32);
        ip_displayed = 1;
    }
    
}


int main(void)
{
    unsigned long user0, user1;                                         // variables to retrieve MAC address from flash
    unsigned char mac_address[8];                                       // buffer to hold MAC address
	
    // If running on Rev A2 silicon, turn the LDO voltage up to 2.75V.  This is a workaround to allow the PLL to operate reliably.
    if(REVISION_IS_A2)
    {
        SysCtlLDOSet(SYSCTL_LDO_2_75V);
    }
    // Set the clocking to run directly from the PLL at 50MHz.
    SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_8MHZ);
    // Initialize the OLED display to run at 1MHz
    RIT128x96x4Init(1000000);    
    RIT128x96x4Enable(1000000);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_ETH);                          // Enable the Ethernet Controller.
    SysCtlPeripheralReset(SYSCTL_PERIPH_ETH);                           // Reset the Ethernet Controller.

    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);                        // Enable Port F for Ethernet LEDs.
    GPIOPinTypeEthernetLED(GPIO_PORTF_BASE, GPIO_PIN_2 | GPIO_PIN_3);   // LED0-Bit 3-Output and LED1-Bit 2-Output

    SysTickPeriodSet(SysCtlClockGet() / SYSTICKHZ);                     // Configure SysTick for a periodic interrupt.
    SysTickEnable();
    SysTickIntEnable();

    // IntMasterEnable();                                                  // Enable processor interrupts.

    // Configure the hardware MAC address for Ethernet Controller filtering of incoming packets.
    // MAC address is stored in the non-volatile USER0 and USER1 registers. Read using the FlashUserGet function.
    FlashUserGet(&user0, &user1);
    if((user0 == 0xffffffff) || (user1 == 0xffffffff))
    {
        // We should never get here.  This is an error if the MAC address has not been programmed into the device.  Exit the program.
        RIT128x96x4StringDraw("MAC Address", 0, 16, 15);
        RIT128x96x4StringDraw("Not Programmed!", 0, 24, 15);
        while(1);
    }

    
    // Convert the 24/24 split MAC address from NV ram into a 32/16 split
    // MAC address needed to program the hardware registers, then program
    // the MAC address into the Ethernet Controller registers.
    mac_address[0] = ((user0 >>  0) & 0xff);
    mac_address[1] = ((user0 >>  8) & 0xff);
    mac_address[2] = ((user0 >> 16) & 0xff);
    mac_address[3] = ((user1 >>  0) & 0xff);
    mac_address[4] = ((user1 >>  8) & 0xff);
    mac_address[5] = ((user1 >> 16) & 0xff);

    // Initialze the lwIP library, using DHCP.
    lwIPInit(mac_address, 0, 0, 0, IPADDR_USE_DHCP);

    CAN_configure();                                                    // Enable the board for CAN processing
    CAN_receive_FIFO(CAN_data.rx_buffer, CAN_FIFO_SIZE, &CAN_data);     // Configure the receive message FIFO - this function should only be called once.    
    

    // loop forever
    while (1) 
    {
        //print some info to the OLED
        //NB: this uses up quite a bit of processing cycles, so use it sparingly - it should ideally not be put in a ISR
        if (update_count >= UPDATE_RATE)
        {
            display_can_statistics(message_count,lost_message_count,5,70);
            update_count = 0;                                   //reset the update count
        }
    }

}
