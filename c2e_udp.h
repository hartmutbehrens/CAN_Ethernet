#ifndef __C2E_UDP_H
#define __C2E_UDP_H

void UDP_configure(void);
void UDP_send(void);
void UDP_receive(void *arg, struct udp_pcb *pcb, struct pbuf *p, struct ip_addr *addr, u16_t port);

#endif