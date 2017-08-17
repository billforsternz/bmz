// ether.h, header for ether.c, see that file for full discussion
#ifndef ETHER_H
#define ETHER_H
#include "bmz.h"
void *ether_init( byte **addr_mem, u16 *addr_len );
void ether_down( MSG *msg );
void ether_idle();
void ether_timeout( byte timer_id );
void ether_set_addr( const byte *ethaddr );
u16  ether_rx_room();
#endif // ETHER_H
