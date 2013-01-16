#include "inc/hw_ints.h"
#include "inc/hw_nvic.h"
#include "inc/hw_types.h"
#include "inc/hw_sysctl.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "lwip/netif.h"
#include "utils/lwiplib.h"
#include "utils/ustdlib.h"
#include "utils/ringbuf.h"
#include "drivers/rit128x96x4.h"
#include "config.h"
#include "c2e_can.h"
#include "c2e_eth.h"
#include "c2e_events.h"
#include "c2e_main.h"
#include "c2e_udp.h"
#include "c2e_utils.h"

static char print_buf[PRINT_BUF_SIZE];
static uint32_t g_state;                                                 // current state 
struct netif *g_netif;
volatile uint32_t previous_ip = 0;

transition_t transition[] =                                               // state machine transition
{
    { ST_INIT, EV_POWERON, &BOARD_init},
    { ST_ANY, EV_INITETH, &ETH_init},
    { ST_ANY, EV_INITINT, &INT_init},
    { ST_INTENABLED, EV_INITLWIP, &LWIP_init},
    { ST_LWIPINIT, EV_IPCHANGED, &handle_IP_change},
    { ST_LWIPINIT, EV_ANY, &wait},
    { ST_IPCHANGED, EV_FOUNDGW, &handle_GW_change},
    { ST_IPCHANGED, EV_BROADCAST, &broadcast_presence},
    { ST_IPCHANGED, EV_ANY, &wait},
    { ST_GWFOUND, EV_INITCAN, &CAN_init},
    { ST_GWFOUND, EV_BROADCAST, &broadcast_presence},
    { ST_GWFOUND, EV_ANY, &wait},
    { ST_CANINIT, EV_BROADCAST, &broadcast_presence},
    { ST_CANINIT, EV_ANY, &wait},
    { ST_ANY, EV_BROADCAST, &broadcast_presence},
    { ST_ANY, EV_ANY, &fsm_any}
};

int main(void)
{
    unsigned char event; 
    uint32_t transitions =  sizeof(transition)/sizeof(*transition); 
    
    load_boot_events();                                                     // load events required to boot the board

    g_state = ST_INIT;
    while (g_state != ST_TERM)                                              // run the state machine
    {
        event = get_next_event();
        for (int i = 0; i < transitions; i++) 
        {
            if ((g_state == transition[i].state) || (ST_ANY == transition[i].state)) 
            {
                if ((event == transition[i].event) || (EV_ANY == transition[i].event)) 
                {
                    g_state = (transition[i].fn)();
                    break;
                }
            }
        }
    }
}

//broadcast presence - return to previous state
static uint32_t broadcast_presence(void)
{
   UDP_broadcast_presence();                                                // send UDP broadcast messages to indicate the presence of a CAN2Ethernet gateway
    return g_state;
    //HWREG(NVIC_INT_CTRL) = NVIC_INT_CTRL_PEND_SV;                         // Trigger PendSV
}

// wait for stuff to happen - return to previous state
static uint32_t wait(void)
{
    display_state();
    display_UDP_statistics();
    display_CAN_statistics();
    return g_state;
}  

//display a IP address
static void display_ip_address(void)
{
    unsigned char *temp = (unsigned char *)&g_netif->ip_addr.addr;
    // Convert the IP Address into a string for display purposes
    usprintf(print_buf, "IP: %d.%d.%d.%d   ", temp[0], temp[1], temp[2], temp[3]);
    RIT128x96x4StringDraw(print_buf, 5, 20, 15);
}

//display state
static void display_state(void)
{
    usprintf(print_buf, "%d ", g_state);
    RIT128x96x4StringDraw(print_buf, 110, 10, 15);
}

// handle a change in IP address - change state to ST_IPCHANGED
static uint32_t handle_IP_change(void)
{
    display_ip_address();
    UDP_start_listen();                     // start receiving UDP messages
    return ST_IPCHANGED;
}

// handle a addition of a gateway - change state to ST_GWFOUND
static uint32_t handle_GW_change(void)
{
    display_gw_address();                   // display gateway address
    enqueue_event(EV_INITCAN);              // schedule initialization of CAN
    return ST_GWFOUND;
}

static void load_boot_events(void)
{
    init_event_buffer();
    enqueue_event(EV_POWERON);
    enqueue_event(EV_INITETH);
    enqueue_event(EV_INITINT);
    enqueue_event(EV_INITLWIP);
    enqueue_event(EV_BROADCAST);
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

// initialize lwIP library - change state to ST_LWIPINIT
static uint32_t LWIP_init(void)
{
    unsigned char mac_address[8];                                       // buffer to hold MAC address
    get_mac_address(mac_address);                                       // get MAC address from Flash
    lwIPInit(mac_address, 0, 0, 0, IPADDR_USE_DHCP);                    // Initialze the lwIP library, using DHCP.
    g_netif = netif_list;
    netif_set_status_callback(g_netif, &netif_status_change);           // call C2E netif status callback - utils/lwiplib.c had to be changed to make this possible - netif_set_up(&g_sNetIF) commented out
    return ST_LWIPINIT;
}


void netif_status_change(struct netif *netif)
{
    enqueue_event(EV_IPCHANGED);                                        // schedule activation of receiving UDP packets
}

// state machine error state
static uint32_t fsm_any(void)
{
    RIT128x96x4StringDraw("FSM ERROR", 5, 60, 15);
    return ST_ANY;
}

void SYSTICK_handler(void)                                              // SYSTICK interrupt handler
{
    lwIPTimer(SYSTICKMS);                                               // Call the lwIP timer handler - eventually results in lwIPHostTimerHandler being called
    //lwIPTimer(SYSTICKUS);                                               // Call the lwIP timer handler - eventually results in lwIPHostTimerHandler being called
}

void lwIPHostTimerHandler(void)                                         // This function is required by lwIP library to support any host-related timer functions.
{
    if (gateway_count() == 0)
    {
        enqueue_event(EV_BROADCAST);                                    // broadcast presence, if no gateways have announced themselves yet
    }
}





