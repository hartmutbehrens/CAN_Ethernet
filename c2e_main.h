#ifndef __C2E_CAN_ETHERNET_H
#define __C2E_CAN_ETHERNET_H
#include <stdint.h>

typedef struct
{
	uint32_t state;
	uint32_t event;
	uint32_t (*fn)(void);
} transition_t;														// state machine transition struct

static uint32_t fsm_any(void);									    // catch all state machine function that executes when STATE = ANY and EVENT = ANY
static uint32_t BOARD_init(void);									// initialize clock, power, display
static uint32_t INT_init(void);										// Master interrupt enable
static uint32_t LWIP_init(void);									// initialize lwIP
static uint32_t wait(void);											// wait for stuff to happen
static uint32_t handle_IP_change(void);								// handle a change in IP address
static uint32_t handle_GW_change(void);								// handle an addition of GW
static uint32_t broadcast_presence(void);
static void load_boot_events(void);									// enqueue the events required to boot the board
static void display_state(void);											// display state machine state
static void display_ip_address(void);										// display an lwIP address

void netif_status_change(struct netif *netif);
void SYSTICK_handler(void);

#endif
