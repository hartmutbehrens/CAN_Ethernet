#include "inc/hw_memmap.h"
#include "inc/hw_sysctl.h"
#include "inc/hw_types.h"
#include "driverlib/ethernet.h"
#include "driverlib/flash.h"
#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "utils/lwiplib.h"
#include "eth_conf.h"

volatile unsigned long ip_displayed = 0;              					// only show IP message once

void Eth_configure(void)
{
	SysCtlPeripheralEnable(SYSCTL_PERIPH_ETH);                          // Enable the Ethernet Controller.
    SysCtlPeripheralReset(SYSCTL_PERIPH_ETH);                           // Reset the Ethernet Controller.

    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);                        // Enable Port F for Ethernet LEDs.
    GPIOPinTypeEthernetLED(GPIO_PORTF_BASE, GPIO_PIN_2 | GPIO_PIN_3);   // LED0-Bit 3-Output and LED1-Bit 2-Output
}


void get_MAC_address(unsigned char *mac_address)						// Configure the hardware MAC address for Ethernet Controller filtering of incoming packets. 
{																		// MAC address is stored in the non-volatile USER0 and USER1 registers
	unsigned long user0, user1;                                         // variables to retrieve MAC address from flash
	FlashUserGet(&user0, &user1);
    if((user0 == 0xffffffff) || (user1 == 0xffffffff))
    {
        // We should never get here.  This is an error if the MAC address has not been programmed into the device.  Exit the program.
        //RIT128x96x4StringDraw("MAC Address", 0, 16, 15);
        //RIT128x96x4StringDraw("Not Programmed!", 0, 24, 15);
        while(1);
    }
    // Convert the 24/24 split MAC address from NV ram into a 32/16 split MAC address needed to program the hardware registers, 
    mac_address[0] = ((user0 >>  0) & 0xff);
    mac_address[1] = ((user0 >>  8) & 0xff);
    mac_address[2] = ((user0 >> 16) & 0xff);
    mac_address[3] = ((user1 >>  0) & 0xff);
    mac_address[4] = ((user1 >>  8) & 0xff);
    mac_address[5] = ((user1 >> 16) & 0xff);
}

void lwIPHostTimerHandler(void)											// This function is required by lwIP library to support any host-related timer functions.
{																		// Define HOST_TMR_INTERVAL in lwipopts.h to use
    static unsigned long ulLastIPAddress = 0;
    unsigned long ulIPAddress;
    ulIPAddress = lwIPLocalIPAddrGet();

    if( (ulLastIPAddress != ulIPAddress) && (!ip_displayed) )			// If IP Address has not yet been assigned, update the display accordingly
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

