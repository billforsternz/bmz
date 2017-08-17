// tserver.h, header for tserver.c, see that file for full discussion
#ifndef  TSERVER_H
#define  TSERVER_H
#include "bmz.h"
void *tserver_init( byte **addr_mem, u16 *addr_len );
void  tserver_idle();
void  tserver_up( MSG *msg );
#endif  // TSERVER_H
