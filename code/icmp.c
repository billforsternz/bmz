/*************************************************************************
 * icmp.c
 *
 *  icmp = Internet Control Message Protocol
 *  Project: eZ2944
 *************************************************************************/
#include <stdio.h>
#include <string.h>
#include "project.h"
#include "bmz.h"
#include "tcpip.h"
#include "checksum.h"
#include "icmp.h"

/*************************************************************************
 * Message up
 *************************************************************************/
// Message format in (up from IP);
//      [src_ipaddr,4]
//      [payload]
// Message format out (down to IP);
//      [ip protocol nbr,1]
//      [dst_ipaddr,4]
//      [payload]
void icmp_up( MSG *msg )
{
    bool   err=false;
    u16    *poke;
    u16    checksum, len;
    MSG    *reply=NULL;

    // Take off parameter
    IPADDR rem_ipaddr = msg_pop4(msg);

    // Test checksum
    err = checksum_test( msg_ptr(msg), msg_len(msg), 2 );

    // Test for echo request
    if( !err )
    {
        byte type = msg_read1(msg,0);
        byte code = msg_read1(msg,1);
        if( type==8 && code==0 )    // type==8 is request
            reply = pool_alloc( bmz_get_current_pool() );
    }

    // Send reply
    if( reply )
    {

        // Copy from msg to reply - no more than room available in reply
        len = (reply->base+reply->size - reply->ptr);    //available
        if( len > msg_len(msg) )
            len = msg_len(msg);     // shorten to size of request
        memcpy( msg_ptr(reply), msg_ptr(msg), len );
        msg_len(reply)  = len;      // assumes msg_len() is macro
        *msg_ptr(reply) = 0;        // type==0 is reply

        // Calculate and insert checksum
        poke  = (u16 *)( msg_ptr(reply) + 2 );
        *poke = 0;
        checksum = checksum_calculate( msg_ptr(reply), msg_len(reply) );
        *poke = checksum;

        // Add output parameters
        msg_push4( reply, rem_ipaddr );
        msg_push1( reply, PROTOCOL_ICMP );

        // To IP
        bmz_down( TASKID_IP, reply );
    }

    // Free message
    msg_free(msg);
}
