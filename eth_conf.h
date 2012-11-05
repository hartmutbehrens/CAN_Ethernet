#ifndef __ETH_CONF_H__
#define __ETH_CONF_H__

void Eth_configure(void);
void get_MAC_address(unsigned char *mac_address);
void lwIPHostTimerHandler(void);
#endif