#include "inc/hw_ints.h"
#include "inc/hw_sysctl.h"
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"
#include "utils/lwiplib.h"
#include "utils/ustdlib.h"
#include "drivers/rit128x96x4.h"
#include "can_ethernet.h"
#include "can_conf.h"
#include "eth_conf.h"

// A set of flags.  0 indicates that a SysTick interrupt has occurred - see SYSTICK_handler
#define FLAG_SYSTICK 0
static volatile unsigned long systick_flag;

extern CAN_struct CAN_data;                            // structure to hold CAN RX and TX data
extern volatile unsigned long message_count;		   // CAN received message count
extern volatile unsigned long update_count;		       // print CAN updates once this threshold is reached
extern volatile unsigned long lost_message_count;	   // lost CAN message count
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

    Eth_configure();
    
    // IntMasterEnable();                                               // Enable processor interrupts.
    unsigned char mac_address[8];                                       // buffer to hold MAC address
    get_MAC_address(mac_address);
    lwIPInit(mac_address, 0, 0, 0, IPADDR_USE_DHCP);                    // Initialze the lwIP library, using DHCP.

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
            update_count = 0;                                   // reset the update count
        }
    }

}
