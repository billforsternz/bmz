/*************************************************************************
 * ip.c
 *
 *  ip = Internet Protocol
 *  Project: eZ2944
 *************************************************************************/
#include <stdio.h>
#include "project.h"
#include "bmz.h"
#include "tick.h"
#include "tcpip.h"
#include "checksum.h"
#include "ip.h"

// Misc
#define VER_HLEN_TOS 0x4500     // First IP field in all sent datagrams
                                //  (version, header length, type of service)
#define STD_IP_HEADER_LEN 20    // Standard header length, with no options
#define TTL  40                 // value of TTL field in sent datagram

// Module data
typedef struct
{
    u16     identification; // incrementing datagram identification
    bool    id_reset;       // reset identification once
} IP;
static IP z;

// Local prototypes
static IPADDR route( IPADDR dst_ipaddr );

/*************************************************************************
 * Init
 *************************************************************************/
void *ip_init( byte **addr_mem, u16 *addr_len )
{
    z.id_reset = true;
    return( &z );
}

/*************************************************************************
 * Message down
 *************************************************************************/
// Message format in (down from TCP or other protocol);
//      [ip protocol nbr,1]
//      [dst_ipaddr,4]
//      [payload]
// Message format out (down to ARP);
//      [next_hop_ipaddr,4]
//      [ip hdr]
//      [payload]
void ip_down( MSG *msg )
{
    u16 len, checksum, *poke;
    IPADDR next_hop;

    // Take off parameters
    byte   protocol   = msg_pop1(msg);
    IPADDR dst_ipaddr = msg_pop4(msg);

    // Calculate total length
    len = msg_len(msg) + STD_IP_HEADER_LEN;

    // Add ip header
    msg_push4( msg, dst_ipaddr );       // dst
    msg_push4( msg, config.my_ipaddr ); // src
    msg_push2( msg, 0 );                // chksum
    msg_push1( msg, protocol );         // protocol
    msg_push1( msg, TTL );              // ttl
    msg_push2( msg, 0 );                // fragmentation=0 (no fragmentation)
    if( z.id_reset )                    // one time only randomisation
    {
        z.id_reset = false;
        z.identification = (u16)tick_get_hi_res();
    }
    msg_push2( msg, z.identification++ );   // datagram identification
    msg_push2( msg, len );                  // total length
    msg_push2( msg, VER_HLEN_TOS );         // version, hlen, type of service

    // Insert IP header checksum
    checksum = checksum_calculate( msg_ptr(msg), STD_IP_HEADER_LEN );
    poke  = (u16 *)( msg_ptr(msg) + 10 );
    *poke = checksum;

    // Route
    next_hop = route( dst_ipaddr );

    // Add parameters
    msg_push4( msg, next_hop );  // parameter for arp - next hop ip address

    // To ARP
    bmz_down( TASKID_ARP, msg );
}


/*************************************************************************
 * Message up
 *************************************************************************/
// Message format in (up from ETHER);
//      [ip hdr]
//      [payload]
// Message format out (up to TCP or other protocol);
//      [src ipaddr,4]
//      [payload]
void ip_up( MSG *msg )
{
    u16 len = msg_len(msg);
    u16 total_len, fragmentation;
    byte ver_hlen, hlen, protocol;
    IPADDR src_ipaddr;
    bool err=false;

    // Test basic validity and compatibility
    if( len < STD_IP_HEADER_LEN )
        err = true;
    else
    {
        ver_hlen  = msg_read1(msg,0);   // version, hlen
        if( (ver_hlen&0xf0) != 0x40 )   // IP version 4 ?
            err = true;
        else
        {
            hlen = ver_hlen & 0x0f;     // hlen (in u32s)
            hlen <<= 2;                 // hlen (in bytes)
            if( hlen < STD_IP_HEADER_LEN )
                err = true;
            if( hlen > len )
                err = true;
        }
    }

    // Test checksum
    if( !err )
        err = checksum_test( msg_ptr(msg), hlen, 10 );

    // Take off the standard ip header, verify fields
    if( !err )
    {
                        msg_pop2( msg );  // ver_hlen_tos
        total_len     = msg_pop2( msg );  // total length
                        msg_pop2( msg );  // identification
        fragmentation = msg_pop2( msg );  // fragmentation
                        msg_pop1( msg );  // ttl
        protocol      = msg_pop1( msg );  // protocol
                        msg_pop2( msg );  // chksum
        src_ipaddr    = msg_pop4( msg );  // src
                        msg_pop4( msg );  // dst

        // Field total_len should be length of ip hdr + payload, which
        //  should match the length of our msg variable when it holds
        //  ip hdr + payload.
        // If total_len is longer than that we have a problem
        if( total_len > len )
            err = true;

        // Else if it's less than hlen we have a problem
        else if( total_len < hlen )
            err = true;

        // Else if total_len is less than the buffer length shrink the
        //  contents of the buffer to match field total_len
        else if( total_len < len )
            msg_len(msg) -= (len-total_len);    //assumes msg_len() is macro

        // We cannot deal with fragmentation
        if( (fragmentation&0x3fff) )    // allow don't fragment bit
            err = true;
    }

    // Take off the rest of the ip header
    if( !err )
    {
        msg_pop( msg, (byte)(hlen-STD_IP_HEADER_LEN) );   //zero if no options

        // Add parameters
        msg_push4( msg, src_ipaddr );

        // Dispatch
        switch( protocol )
        {
            case PROTOCOL_TCP:
            {
                bmz_up( TASKID_TCP, msg );
                break;
            }
            case PROTOCOL_ICMP:
            {
                bmz_up( TASKID_ICMP, msg );
                break;
            }
            default:
            {
                err = true;
                break;
            }
        }
    }

    // If error, discard
    if( err )
        msg_free(msg);
}


/*************************************************************************
 * Next hop router
 *************************************************************************/
static IPADDR route( IPADDR dst_ipaddr )
{
    u32 mask = 0xffffffff;
    IPADDR next_hop, my_ipaddr = config.my_ipaddr;
    if( config.subnet_mask )
        mask = config.subnet_mask;
    else
    {
        if( (my_ipaddr&0x80000000) == 0 )
            mask = 0xff000000;  // class A
        else if( (my_ipaddr&0xc0000000) == 0x80000000 )
            mask = 0xffff0000;  // class B
        else if( (my_ipaddr&0xe0000000) == 0xc0000000 )
            mask = 0xffffff00;  // class C
    }
    if( (dst_ipaddr&mask) == (my_ipaddr&mask) )
        next_hop = dst_ipaddr;
    else
        next_hop = config.default_route;
    return( next_hop );
}

