#ifndef __C2E_ETH_H__
#define __C2E_ETH_H__

// A set of flags.  0 indicates that a SysTick interrupt has occurred - see SYSTICK_handler
#define FLAG_SYSTICK 0
// Defines for setting up the system clock.
#define SYSTICKHZ               100
#define SYSTICKMS               (1000 / SYSTICKHZ)
#define SYSTICKUS               (1000000 / SYSTICKHZ)
#define SYSTICKNS               (1000000000 / SYSTICKHZ)

void Eth_configure(void);
void get_MAC_address(unsigned char *mac_address);
void lwIPHostTimerHandler(void);
void SYSTICK_handler(void);
#endif