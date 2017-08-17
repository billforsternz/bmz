/*************************************************************************
 * tserver.c
 *
 *  tserver = terminal server app
 *  Project: eZ2944
 *************************************************************************/
#include <stdio.h>
#include <string.h>
#include "project.h"
#include "bmz.h"
#include "tcpsock.h"
#include "uart.h"
#include "tserver.h"

// Use the listen callback feature to implement a simple method for
//  the remote telnet user to specify the baudrate
#define MAGIC_PORT1_LIST    3000    // uart 0 ports
#define MAGIC_PORT1_1200    3012    // telnet to this port for 1200 baud
#define MAGIC_PORT1_2400    3024    // ..etc
#define MAGIC_PORT1_4800    3048
#define MAGIC_PORT1_9600    3096
#define MAGIC_PORT1_19200   3019
#define MAGIC_PORT1_38400   3038
#define MAGIC_PORT1_57600   3057
#define MAGIC_PORT1_11520   3115
#define MAGIC_PORT2_LIST    4000    // uart 1 ports
#define MAGIC_PORT2_1200    4012    // telnet to this port for 1200 baud
#define MAGIC_PORT2_2400    4024    // ..etc
#define MAGIC_PORT2_4800    4048
#define MAGIC_PORT2_9600    4096
#define MAGIC_PORT2_19200   4019
#define MAGIC_PORT2_38400   4038
#define MAGIC_PORT2_57600   4057
#define MAGIC_PORT2_11520   4115

// Module memory
typedef struct
{
    TASKID    taskid_tcpsock;
    u16       loc_port;
    byte      uart_id;
} TSERVER;

/*************************************************************************
 * Init
 *************************************************************************/
void *tserver_init( byte **addr_mem, u16 *addr_len )
{
    TSERVER *z;
    byte    *memory = *addr_mem;
    u16     memlen  = *addr_len;

    // Allocate module memory
    if( memlen < sizeof(TSERVER) )
        bmz_panic_memory("tserver");
    else
    {
        z        = (TSERVER *)memory;
        memory  += sizeof(TSERVER);
        memlen  -= sizeof(TSERVER);

        // Init module memory
        if( bmz_get_current_taskid() == TASKID_TCPAPP1 )
        {
            z->taskid_tcpsock = TASKID_TCPSOCK1;
            z->uart_id        = 0;
            z->loc_port       = MAGIC_PORT1_LIST;
        }
        else
        {
            z->taskid_tcpsock = TASKID_TCPSOCK2;
            z->uart_id        = 1;
            z->loc_port       = MAGIC_PORT2_LIST;
        }
    }
    *addr_mem = memory;
    *addr_len = memlen;
    return( z );
}

/*************************************************************************
 * An incoming caller is trying to connect to a listening port
 *************************************************************************/
// TCP applications can hook this function for special features. This
//  app uses it to setup uart according to the port specified by the
//  remote user
bool tcpapp_listen_callback( u16 listen_port, u16 loc_port )
{
    u32  baud;
    bool found = false;
    if( listen_port == MAGIC_PORT1_LIST )
    {
        switch( loc_port )
        {
            case MAGIC_PORT1_1200:   baud=1200;   found=true; break;
            case MAGIC_PORT1_2400:   baud=2400;   found=true; break;
            case MAGIC_PORT1_4800:   baud=4800;   found=true; break;
            case MAGIC_PORT1_9600:   baud=9600;   found=true; break;
            case MAGIC_PORT1_19200:  baud=19200;  found=true; break;
            case MAGIC_PORT1_38400:  baud=38400;  found=true; break;
            case MAGIC_PORT1_57600:  baud=57500;  found=true; break;
            case MAGIC_PORT1_11520:  baud=11520;  found=true; break;
        }
        if( found )
            uart_init( 0, baud, 8, 'n', 1 );
    }
    else if( listen_port == MAGIC_PORT2_LIST )
    {
        switch( loc_port )
        {
            case MAGIC_PORT2_1200:   baud=1200;   found=true; break;
            case MAGIC_PORT2_2400:   baud=2400;   found=true; break;
            case MAGIC_PORT2_4800:   baud=4800;   found=true; break;
            case MAGIC_PORT2_9600:   baud=9600;   found=true; break;
            case MAGIC_PORT2_19200:  baud=19200;  found=true; break;
            case MAGIC_PORT2_38400:  baud=38400;  found=true; break;
            case MAGIC_PORT2_57600:  baud=57500;  found=true; break;
            case MAGIC_PORT2_11520:  baud=11520;  found=true; break;
        }
        if( found )
            uart_init( 1, baud, 8, 'n', 1 );
    }
    return( found );
}

/*************************************************************************
 * Idle routine
 *************************************************************************/
// Message format (down to TCPSOCK);
//      [MSG_TYPE_OPEN_ACTIVE] [loc port,2][rem port,2][rem ipaddr]
//   OR [MSG_TYPE_OPEN_PASSIVE][loc_port,2]
//   OR [MSG_TYPE_DATA]        [user data]
//   OR [MSG_TYPE_DATA_PUSH]   [user data]
//   OR [MSG_TYPE_CLOSE]
//   OR [MSG_TYPE_ABORT]
void tserver_idle()
{
    TSERVER *z = bmz_get_current_instance();
    MSG *msg;
    PUBLISH_STATE publish_state = bmz_get_publish_state(z->taskid_tcpsock);
    if( publish_state == PUBLISH_IDLE )
    {
        msg = pool_alloc( bmz_get_current_pool() );
        if( msg )
        {
            msg_write2( msg, z->loc_port );
            msg_push1( msg, MSG_TYPE_OPEN_PASSIVE );
            bmz_down( z->taskid_tcpsock, msg );
        }
    }
    else if( publish_state == PUBLISH_ACTIVE )
    {
        if( uart_read_test(z->uart_id) )
        {
            msg = pool_alloc( bmz_get_current_pool() );
            if( msg )
            {
                do
                {
                    msg_write1( msg, uart_read(z->uart_id) );
                }
                while( msg_room(msg)>=1 && uart_read_test(z->uart_id) );
                msg_push1( msg, MSG_TYPE_DATA_PUSH );
                bmz_down( z->taskid_tcpsock, msg );
            }
        }
    }
}

/*************************************************************************
 * Message up
 *************************************************************************/
// Message format (up from TCPSOCK);
//      [MSG_TYPE_DATA] [user data]
//   OR [MSG_TYPE_CLOSE]
void tserver_up( MSG *msg )
{
    TSERVER *z = bmz_get_current_instance();
    byte msg_type = msg_pop1(msg);
    if( msg_type == MSG_TYPE_DATA )
    {
        byte *ptr = msg_ptr(msg);
        u16   len = msg_len(msg);
        while( len-- )
            uart_write( z->uart_id, *ptr++ );
    }
    msg_free( msg );
}
