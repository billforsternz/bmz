// arp.h, header for arp.c, see that file for full discussion
#ifndef  ARP_H
#define  ARP_H
#include "bmz.h"
void *arp_init( byte **addr_mem, u16 *addr_len );
void arp_down( MSG *msg );
void arp_up( MSG *msg );
void arp_timeout( byte timer_id );
#endif  // ARP_H
