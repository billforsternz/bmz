// ip.h, header for ip.c, see that file for full discussion
#ifndef  IP_H
#define  IP_H
#include "bmz.h"
void *ip_init( byte **addr_mem, u16 *addr_len );
void  ip_down( MSG *msg );
void  ip_up( MSG *msg );
#endif  // IP_H
