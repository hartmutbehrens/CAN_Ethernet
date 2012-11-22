#include <stdint.h>
#include "inc/hw_ints.h"
#include "inc/hw_sysctl.h"
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h"
#include "utils/lwiplib.h"
#include "utils/ustdlib.h"
#include "utils/ringbuf.h"
#include "drivers/rit128x96x4.h"
#include "can_ethernet.h"
#include "c2e_can.h"
#include "c2e_eth.h"
#include "c2e_udp.h"

static unsigned char ring_rxbuf[RING_BUF_SIZE];
tRingBufObject g_can_ringbuf;                                               // ring buffer to receive CAN frames
char buffer[16];
unsigned char frame[12];
//display an lwIP address
void display_ip_address(uint32_t ipaddr, uint32_t col, uint32_t row)
{
    /*
    if( !RingBufEmpty(&g_can_ringbuf) )
    {
        RingBufRead(&g_can_ringbuf, &frame[0], 12);
        usprintf(buffer, "CAN id: %d.%d.%d.%d", frame[0], frame[1], frame[2], frame[3]);
        RIT128x96x4StringDraw(buffer, col, row-10, 15);
    }
    */
    
    unsigned char *temp = (unsigned char *)&ipaddr;
    // Convert the IP Address into a string for display purposes
    usprintf(buffer, "IP: %d.%d.%d.%d", temp[0], temp[1], temp[2], temp[3]);
    RIT128x96x4StringDraw(buffer, col, row, 15);
}


int main(void)
{
    
    if(REVISION_IS_A2)                                                  // If running on Rev A2 silicon, turn the LDO voltage up to 2.75V.  This is a workaround to allow the PLL to operate reliably.
    {
        SysCtlLDOSet(SYSCTL_LDO_2_75V);
    }
    // Set the clocking to run directly from the PLL at 50MHz.
    SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_8MHZ);
    // Initialize the OLED display to run at 1MHz
    RIT128x96x4Init(1000000);    
    RIT128x96x4Enable(1000000);
    RIT128x96x4StringDraw("CAN2ETH", 10, 10, 15);                       // Say Hello

    RingBufInit(&g_can_ringbuf, &ring_rxbuf[0], sizeof(ring_rxbuf));        // initialize ring buffer to receive CAN frames

    Eth_configure();
    
    IntMasterEnable();                                               // Enable processor interrupts.
    unsigned char mac_address[8];                                       // buffer to hold MAC address
    get_mac_address(mac_address);
    lwIPInit(mac_address, 0, 0, 0, IPADDR_USE_DHCP);                    // Initialze the lwIP library, using DHCP.

    CAN_configure();                                                    // Enable the board for CAN processing
    
    static uint32_t ulLastIPAddress = 0;
    static uint32_t has_address = 0;
    uint32_t ulIPAddress;
    while (1)                                                           // loop forever
    {
        
        display_CAN_statistics(1,5,80);                                 // print some info to the OLED NB: this uses up quite a bit of processing cycles, so use it sparingly - it should ideally not be put in a ISR

        ulIPAddress = lwIPLocalIPAddrGet();
        if( (ulLastIPAddress != ulIPAddress) )               
        {
            display_ip_address(ulIPAddress,1,70);
            ulLastIPAddress = ulIPAddress;
            has_address = 1;
        }
        if (has_address)
        {
            //UDP_send();   
        }
        else
        {
            
        }
    }
}
