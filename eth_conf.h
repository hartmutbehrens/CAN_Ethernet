#ifndef __ETH_CONF_H__
#define __ETH_CONF_H__

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