#ifndef __C2E_UDP_H
#define __C2E_UDP_H

void UDP_start(void);
void UDP_send(tRingBufObject *pt_ring_buf);
void UDP_receive(void *arg, struct udp_pcb *pcb, struct pbuf *p, struct ip_addr *addr, u16_t port);

#endif