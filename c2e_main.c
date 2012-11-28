#include "inc/hw_ints.h"
#include "inc/hw_nvic.h"
#include "inc/hw_types.h"
#include "inc/hw_sysctl.h"
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h"
#include "utils/lwiplib.h"
#include "utils/ustdlib.h"
#include "utils/ringbuf.h"
#include "drivers/rit128x96x4.h"
#include "config.h"
#include "c2e_can.h"
#include "c2e_eth.h"
#include "c2e_main.h"
#include "c2e_udp.h"

static unsigned char ring_rxbuf[RING_BUF_SIZE];
tRingBufObject g_can_ringbuf;                                               // ring buffer to receive CAN frames
char buffer[16];
unsigned char frame[12];
//display an lwIP address
void display_ip_address(uint32_t ipaddr, uint32_t col, uint32_t row)
{   
    unsigned char *temp = (unsigned char *)&ipaddr;
    // Convert the IP Address into a string for display purposes
    usprintf(buffer, "IP: %d.%d.%d.%d", temp[0], temp[1], temp[2], temp[3]);
    RIT128x96x4StringDraw(buffer, col, row, 15);
}

void PENDSV_handler(void)
{
    UDP_send(&g_can_ringbuf);                                           // send CAN frames over UDP
    HWREG(NVIC_INT_CTRL) = NVIC_INT_CTRL_UNPEND_SV;                     // clear PendSV
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
    RIT128x96x4StringDraw("CAN2ETH", 5, 10, 15);                       // Say Hello

    RingBufInit(&g_can_ringbuf, ring_rxbuf, sizeof(ring_rxbuf));        // initialize ring buffer to receive CAN frames

    Eth_configure();                                                    // Enable Ethernet controller
    IntMasterEnable();                                                  // Enable processor interrupts.
    IntPrioritySet(INT_CAN0, 0x00);                                     // Set CAN interrupt highest priority because messages to be sent via UDP are buffered
    IntPrioritySet(INT_ETH, 0x01);                                      // Set Eth interrupt priority slightly less than CAN interrupt
    HWREG(NVIC_SYS_PRI2) = 0xff;                                        // Set PendSV interrupt to lowest priority

    unsigned char mac_address[8];                                       // buffer to hold MAC address
    get_mac_address(mac_address);                                       // get MAC address from Flash
    lwIPInit(mac_address, 0, 0, 0, IPADDR_USE_DHCP);                    // Initialze the lwIP library, using DHCP.

    
    static uint32_t ulLastIPAddress = 0;
    static uint32_t has_address = 0;
    uint32_t ulIPAddress;
    while (1)                                                           // loop forever
    {
        
        display_CAN_statistics(1,5,80);                                 // print some info to the OLED NB: this uses up quite a bit of processing cycles, so use it sparingly - it should ideally not be put in a ISR

        ulIPAddress = lwIPLocalIPAddrGet();
        if( (ulLastIPAddress != ulIPAddress) && (has_address == 0) )               
        {
            display_ip_address(ulIPAddress,1,70);
            ulLastIPAddress = ulIPAddress;
            CAN_configure();                                            // Enable the board for CAN processing
            has_address = 1;
        }
        if (has_address)
        {


        }
        
    }
}
