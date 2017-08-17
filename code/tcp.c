/*************************************************************************
 * tcp.c
 *
 *  tcp = Transport Control Protocol
 *  Project: eZ2944
 *************************************************************************/
#include <stdio.h>
#include "project.h"
#include "bmz.h"
#include "tcpip.h"
#include "checksum.h"
#include "tcpsock.h"
#include "tcp.h"

// Misc
#define STD_TCP_HEADER_LEN 20  //Standard TCP header length, no options

/*************************************************************************
 * Message down
 *************************************************************************/
// Message format in (down from TCP_SOCKET);
//      [dst_ipaddr,4]
//      [src_port,2]
//      [dst_port,2]
//      [seq_nbr,4]
//      [ack_nbr,4]
//      [code_bits,2]
//      [window,2]
//      [user data]
// Message format out (down to IP);
//      [ip protocol nbr,1]
//      [dst_ipaddr,4]
//      [payload] = [tcp segment] = [tcp hdr + tcp data]
void tcp_down( MSG *msg )
{
    u16 segment_len, checksum, hlen_code_bits, *poke;
    byte hlen;

    // Take off parameters
    IPADDR dst_ipaddr = msg_pop4(msg);
    u16    src_port   = msg_pop2(msg);
    u16    dst_port   = msg_pop2(msg);
    u32    seq_nbr    = msg_pop4(msg);
    u32    ack_nbr    = msg_pop4(msg);
    u16    code_bits  = msg_pop2(msg);
    u16    window     = msg_pop2(msg);

    // Add tcp header
    hlen = STD_TCP_HEADER_LEN;       // hlen in bytes
    hlen >>= 2;                      // hlen in u32s
    msg_push2( msg, 0 );             // urgent pointer
    msg_push2( msg, 0 );             // checksum
    msg_push2( msg, window );        // window
    hlen_code_bits = (u16)hlen;
    hlen_code_bits <<= 12;
    hlen_code_bits += code_bits;
    msg_push2( msg, hlen_code_bits); // hlen, code bits
    msg_push4( msg, ack_nbr );       // ack nbr
    msg_push4( msg, seq_nbr );       // seq nbr
    msg_push2( msg, dst_port );      // dst port
    msg_push2( msg, src_port );      // src port

    // Add pseudo header
    segment_len = msg_len(msg);
    msg_push2( msg, segment_len );
    msg_push1( msg, PROTOCOL_TCP );
    msg_push1( msg, 0 );
    msg_push4( msg, dst_ipaddr );
    msg_push4( msg, config.my_ipaddr );    // src ipaddr
    checksum = checksum_calculate( msg_ptr(msg), msg_len(msg) );

    // Remove pseudo header, insert checksum
    msg_pop( msg, 12 );
    poke  = (u16 *)( msg_ptr(msg) + 16 );
    *poke = checksum;

    // Add output parameters
    msg_push4( msg, dst_ipaddr );
    msg_push1( msg, PROTOCOL_TCP );

    // To IP
    bmz_down( TASKID_IP, msg );
}


/*************************************************************************
 * Message up
 *************************************************************************/
// Message format in (up from IP);
//      [src_ipaddr,4]
//      [payload] = [tcp segment] = [tcp hdr+tcp data]
// Message format out (up to TCP_SOCKET);
//      [seq_nbr,4]
//      [ack_nbr,4]
//      [code_bits,2]
//      [window,2]
//      [user data]
void tcp_up( MSG *msg )
{
    bool   err=false;
    byte   hlen;
    u16    rem_port, loc_port;
    u32    seq_nbr, ack_nbr;
    u16    hlen_code_bits, window;
    TASKID taskid;
    u16    *poke;
    u16    checksum;

    // Take off parameters
    IPADDR rem_ipaddr     = msg_pop4(msg);
    u16    segment_len    = msg_len(msg);

    // Test basic validity and compatibility
    if( segment_len < STD_TCP_HEADER_LEN )
        err = true;
    else
    {
        hlen = (msg_read1(msg,12) >> 4) & 0x0f;  // hlen (in u32s)
        hlen <<= 2;                              // hlen (in bytes)
        if( hlen < STD_TCP_HEADER_LEN )
            err = true;
        if( hlen > segment_len )
            err = true;
    }

    // Add pseudo header
    if( !err )
    {
        msg_push2( msg, segment_len );
        msg_push1( msg, PROTOCOL_TCP );
        msg_push1( msg, 0 );
        msg_push4( msg, config.my_ipaddr );    // dst ipaddr
        msg_push4( msg, rem_ipaddr );

        // Test checksum (offset is 16 bytes into segment
        //  + 12 byte pseudo header)
        err = checksum_test( msg_ptr(msg), msg_len(msg), 12+16 );

        // Pop off pseudo header
        msg_pop( msg, 12 );
    }

    // Take off the standard tcp header, select socket
    if( !err )
    {
        rem_port       = msg_pop2( msg );  // src port
        loc_port       = msg_pop2( msg );  // dst port
        seq_nbr        = msg_pop4( msg );  // seq nbr
        ack_nbr        = msg_pop4( msg );  // ack nbr
        hlen_code_bits = msg_pop2( msg );  // hlen, reserved, code bits
        window         = msg_pop2( msg );  // window
                         msg_pop2( msg );  // checksum
                         msg_pop2( msg );  // urgent_ptr
        taskid = tcpsock_select( loc_port, rem_port, rem_ipaddr );

        // Add output parameters and dispatch
        if( taskid != TASKID_NULL )
        {
            msg_push2( msg, window );           // window
            msg_push2( msg, hlen_code_bits );   // code bits
            msg_push4( msg, ack_nbr );          // ack nbr
            msg_push4( msg, seq_nbr );          // seq nbr
            bmz_up( taskid, msg );
        }

        // Send RST for SYN to unknown connection
        else if( hlen_code_bits & SYN_BIT )
        {
            msg_free(msg);
            msg = pool_alloc( bmz_get_current_pool() );
            if( msg )
            {

                // Add tcp header
                msg_push2( msg, 0 );             // urgent pointer
                msg_push2( msg, 0 );             // checksum
                msg_push2( msg, 0 );             // window
                hlen_code_bits =
                        STD_TCP_HEADER_LEN;      // hlen in bytes
                hlen_code_bits >>= 2;            // hlen in u32s
                hlen_code_bits <<= 12;
                hlen_code_bits += (ACK_BIT+RST_BIT);
                msg_push2( msg, hlen_code_bits); // hlen, code bits
                msg_push4( msg, seq_nbr+1 );     // ack nbr
                msg_push4( msg, 0       );       // seq nbr
                msg_push2( msg, rem_port );      // dst port
                msg_push2( msg, loc_port );      // src port

                // Add pseudo header
                segment_len = msg_len(msg);
                msg_push2( msg, segment_len );
                msg_push1( msg, PROTOCOL_TCP );
                msg_push1( msg, 0 );
                msg_push4( msg, rem_ipaddr );           // dst ipaddr
                msg_push4( msg, config.my_ipaddr );     // src ipaddr
                checksum = checksum_calculate( msg_ptr(msg), msg_len(msg) );

                // Remove pseudo header, insert checksum
                msg_pop( msg, 12 );
                poke  = (u16 *)( msg_ptr(msg) + 16 );
                *poke = checksum;

                // Add output parameters
                msg_push4( msg, rem_ipaddr );
                msg_push1( msg, PROTOCOL_TCP );

                // To IP
                bmz_down( TASKID_IP, msg );
            }
        }
        else
            err = true;
    }

    // If error, discard
    if( err )
        msg_free(msg);
}
