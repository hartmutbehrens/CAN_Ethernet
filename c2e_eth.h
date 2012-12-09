#ifndef __C2E_ETH_H__
#define __C2E_ETH_H__

// A set of flags.  0 indicates that a SysTick interrupt has occurred - see SYSTICK_handler
#define FLAG_SYSTICK 0
// Defines for setting up the system clock.

uint32_t ETH_init(void);
void get_mac_address(unsigned char *mac_address);
void lwIPHostTimerHandler(void);
#endif