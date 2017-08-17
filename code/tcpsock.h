/*************************************************************************
 * tcpsock.h
 *
 *  tcpsock = Transport Control Protocol, Socket
 *  Project: eZ2944
 *************************************************************************/
#ifndef  TCPSOCK_H
#define  TCPSOCK_H
#include "bmz.h"
#include "tcpip.h"

// Message types in messaging exchange with app
#define MSG_TYPE_OPEN_ACTIVE  0
#define MSG_TYPE_OPEN_PASSIVE 1
#define MSG_TYPE_DATA         2
#define MSG_TYPE_DATA_PUSH    3
#define MSG_TYPE_CLOSE        4
#define MSG_TYPE_ABORT        5

// Prototypes
void *tcpsock_init( byte **addr_mem, u16 *addr_len );
void  tcpsock_down( MSG *msg );
void  tcpsock_up( MSG *msg );
void  tcpsock_timeout( byte timer_id );
TASKID tcpsock_select( u16 loc_port, u16 rem_port, IPADDR rem_ipaddr );

#endif  // TCPSOCK_H
