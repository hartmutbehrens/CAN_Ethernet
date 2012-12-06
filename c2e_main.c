#include "inc/hw_ints.h"
#include "inc/hw_nvic.h"
#include "inc/hw_types.h"
#include "inc/hw_sysctl.h"
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h"
#include "lwip/netif.h"
#include "utils/lwiplib.h"
#include "utils/ustdlib.h"
#include "utils/ringbuf.h"
#include "drivers/rit128x96x4.h"
#include "config.h"
#include "c2e_can.h"
#include "c2e_eth.h"
#include "c2e_main.h"
#include "c2e_udp.h"
#include "c2e_utils.h"

static unsigned char g_can_rxbuf[CAN_RINGBUF_SIZE];                         // memory for CAN ring buffer
static unsigned char g_event_buf[EV_RINGBUF_SIZE];
tRingBufObject g_can_ringbuf;                                               // ring buffer to receive CAN frames
tRingBufObject g_event_ringbuf;                                             // ring buffer to receive state machine events

volatile struct netif *g_netif;
volatile uint32_t previous_ip = 0;

transition_t transition[] =                                                 // state machine transition
{
    { ST_INIT, EV_POWERON, &BOARD_init},
    { ST_ANY, EV_INITETH, &ETH_init},
    { ST_ANY, EV_INITCAN, &CAN_init},
    { ST_ANY, EV_INITINT, &INT_init},
    { ST_INTENABLED, EV_INITLWIP, &LWIP_init},
    { ST_ANY, EV_IPCHANGED, &display_ip_address},
    { ST_ANY, EV_ANY, &fsm_catchall}
};
#define TRANSITIONS (sizeof(transition)/sizeof(*transition)) 

//display an lwIP address
static uint32_t display_ip_address(void)
{   
    char print_buf[16];
    unsigned char *temp = (unsigned char *)&g_netif->ip_addr.addr;
    // Convert the IP Address into a string for display purposes
    usprintf(print_buf, "IP: %d.%d.%d.%d", temp[0], temp[1], temp[2], temp[3]);
    RIT128x96x4StringDraw(print_buf, 5, 20, 15);
    set_next_event(EV_ANY);
    return ST_ANY;
}

void set_next_event(unsigned char event)
{
   RingBufWriteOne(&g_event_ringbuf, event);  
}

static uint32_t get_next_event(void)
{
    if ( RingBufEmpty(&g_event_ringbuf) )
    {
        return EV_ANY;
    }
    else
    {
        unsigned char event=  RingBufReadOne(&g_event_ringbuf);
        return event;
    }
}

static uint32_t BOARD_init(void)
{
    if(REVISION_IS_A2)                                                  // If running on Rev A2 silicon, turn the LDO voltage up to 2.75V.  This is a workaround to allow the PLL to operate reliably.
    {
        SysCtlLDOSet(SYSCTL_LDO_2_75V);
    }
    
    SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_8MHZ);  // Set the clocking to run directly from the PLL at 50MHz.
    RIT128x96x4Init(1000000);    
    RIT128x96x4Enable(1000000);
    RIT128x96x4StringDraw("CAN2ETH", 5, 10, 15);                       // Say Hello
    return ST_BOARDINIT;
}

static uint32_t INT_init(void)
{
    IntMasterEnable();                                                  // Enable processor interrupts.
    IntPrioritySet(INT_CAN0, 0x00);                                     // Set CAN interrupt highest priority because messages to be sent via UDP are buffered
    IntPrioritySet(INT_ETH, 0x01);                                      // Set Eth interrupt priority slightly less than CAN interrupt
    HWREG(NVIC_SYS_PRI2) = 0xff;                                        // Set PendSV interrupt to lowest priority
    return ST_INTENABLED;
}

static uint32_t LWIP_init(void)
{
    unsigned char mac_address[8];                                       // buffer to hold MAC address
    get_mac_address(mac_address);                                       // get MAC address from Flash
    lwIPInit(mac_address, 0, 0, 0, IPADDR_USE_DHCP);                    // Initialze the lwIP library, using DHCP.
    g_netif = netif_list;
    while (! g_netif->ip_addr.addr)                                     // wait for IP address
    {  }
    return ST_LWIPINIT;
}

static void has_ipaddress_changed(void)
{
    if (previous_ip != g_netif->ip_addr.addr)
    {
        previous_ip = g_netif->ip_addr.addr;
        set_next_event(EV_IPCHANGED);
    }
}

static uint32_t fsm_catchall(void)
{
    has_ipaddress_changed();
    display_CAN_statistics();
    // RIT128x96x4StringDraw("FSM ERROR", 5, 50, 15);
    return ST_ANY;
}

void PENDSV_handler(void)
{
    //unsigned char message[19];
    unsigned char message[1];
    message[0] = ST_GWDISCOVER;
    //message[0] = C2E_DATA;
    //uint32_t size = RingBufUsed(&g_can_ringbuf);
    //uint32_to_uchar(&message[1],size);
    //RingBufRead(&g_can_ringbuf, &message[5], size);
    //UDP_send_data(&g_can_ringbuf);                                           // send CAN frames over UDP
    UDP_send_data2(message, sizeof(message));
    HWREG(NVIC_INT_CTRL) = NVIC_INT_CTRL_UNPEND_SV;                     // clear PendSV
}



int main(void)
{
    unsigned char event; 
    // static unsigned char boot_sequence[] = {EV_POWERON, EV_INITETH, EV_INITCAN, EV_INITINT, EV_INITLWIP};       //sequence of events to bring the board up and running
    static unsigned char boot_sequence[] = {EV_POWERON, EV_INITETH,  EV_INITINT, EV_INITLWIP, EV_INITCAN};       //sequence of events to bring the board up and running
    
    RingBufInit(&g_can_ringbuf, g_can_rxbuf, sizeof(g_can_rxbuf));        // initialize ring buffer to receive CAN frames
    RingBufInit(&g_event_ringbuf, g_event_buf, sizeof(g_event_buf));        // initialize ring buffer to receive events
    RingBufWrite(&g_event_ringbuf, &boot_sequence[0], 5);

    uint32_t state = ST_INIT;
    while (state != ST_TERM)                                            // run the state machine
    {
        event = get_next_event();
        for (int i = 0; i < TRANSITIONS; i++) 
        {
            if ((state == transition[i].state) || (ST_ANY == transition[i].state)) 
            {
                if ((event == transition[i].event) || (EV_ANY == transition[i].event)) 
                {
                    state = (transition[i].fn)();
                    break;
                }
            }
        }
    }
    //static uint32_t has_gateway = 0;
    /*
    while (1)                                                           // loop forever
    {
        
        display_CAN_statistics(1,5,80);                                 // print some info to the OLED NB: this uses up quite a bit of processing cycles, so use it sparingly - it should ideally not be put in a ISR

        
        if ( (netif->ip_addr.addr) && (has_address == 0) )              // show the IP address, once we have acquired one
        {
            display_ip_address(netif->ip_addr.addr,5,20);
            gw_discover_start();
            has_address = 1;
        }   
        
    }
    */
}
