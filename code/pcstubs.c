/*************************************************************************
 * pcstubs.c
 *
 *  Stub out routines we don't want to compile for simulation on PC.
 *  Needed both to avoid link errors and then to trigger processing for
 *  PC hosted debugging
 *  Project: eZ2944
 *************************************************************************/
#include <stdio.h>
#include <string.h>
#include <conio.h>
#include "project.h"
#include "bmz.h"
#include "ether.h"
#include "uart.h"
#include "tick.h"
#undef PRINTF_SUPPRESS

// Not needed on PC
void _init_default_vectors()
{
}

// Not needed on PC
void user_msg_free( MSG *msg )
{
    msg_free(msg);
}

// Not needed on PC
void *ether_init( byte **addr_mem, u16 *addr_len )
{
    return( NULL );
}

// Do a debug dump of transmitted frames
void  ether_down( MSG *msg )
{
    u16 i;
    byte *ptr = msg_ptr(msg);
    printf( "ether_down(), msg=[" );
    for( i=0; i<msg_len(msg); i++ )
    {
        if( i==6 || i==12 || i==14 || i==34 || i==54 )
            printf("-");
        printf( "%02x", 0xff & *ptr++ );
    }
    printf( "]\n" );
    msg_free(msg);
}

// Not needed on PC
u16 ether_rx_room()
{
    return( 4000 );
}

// Not needed on PC
void uart_init( byte uart, u32 baudrate, byte bits, char parity, byte stop )
{
}

// Write character to UART
void uart_write( byte uart, byte c )
{
    printf( "uart_write(%c,%c)\n", '0'+uart, c );
}

// Test whether character available to be read from uart
bool uart_read_test( byte uart )
{
    return( true );
}

// Read character from UART
byte uart_read( byte uart )
{
    return( '?' );
}

// Not needed on PC
void tick_init()
{
}

// Get system heartbeat, incrementing tick count, rate TICKS_PER_SECOND
//  (fake it, use GetTickCount() if it becomes important
u32 tick_get()
{
    static u32 sys_ticks;
    static u32 ticks;
    if( ticks++ > 2000 )
    {
        ticks = 0;
        sys_ticks++;
    }
    return( sys_ticks );
}

// Get high res tick, incrementing tick count, rate TICKS_PER_SECOND_HI_RES
u32 tick_get_hi_res()
{
    return( tick_get()*10 );
}

/* Sample frames;
   Format notes:
    protocol 1=icmp,2=igmp,6=tcp,17=udp
    frame_type 0x806=arp,0x800=ip, 0x8035=rarp

    frag 0x2000 = more bit
     0xe000 = bits mask
    ttl=32 or 64
    total_len=ip hdr+payload
*/

// Test frame for injection
byte ARP_REPLY[] =
{
    0x00,0x01,
    0x08,0x00,
    6,
    4,
    0,2,
    0xee,0xee,0x33,0x33,0x33,0x33,
    192,168,2,9,
    0xaa,0xaa,0x12,0x34,0x56,0x78,
    192,168,2,42
};

/* Sample frames;
   Format notes:
    ip hdr =
    0x4500          total_len
    id              frag
    ttl,protocol    chksum
    src_ip
    dst_ip

    hlen=5 for 20 byte hdr
    tcp header=
    src_port        dst_port
    seq
    ack
    hlen,reserved,urg,ack,psh,rst,syn,fin   window size
    chksum              urgptr
*/

// Test frame for injection
byte TCP_SYN[] =
{
    0x45,0x00,      0x00,0x28,
    0x33,0x33,      0,0,
    0x40,0x06,      0x00,0x00,
    192,168,2,9,
    192,168,2,42,


    0x08,0x00,      0x00,0x17,
    0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,
    0x50,0x02,      0x08,0x00,
    0x00,0x00,      0x00,0x00
};


// Test frame for injection
byte TCP_ACK[] =
{
    0x45,0x00,      0x00,0x28,
    0x33,0x34,      0,0,
    0x40,0x06,      0x00,0x00,
    192,168,2,9,
    192,168,2,42,


    0x08,0x00,      0x00,0x17,
    0x00,0x00,0x00,0x01,
    0x00,0x00,0x00,0x01,
    0x50,0x10,      0x08,0x00,
    0x00,0x00,      0x00,0x00
};


// Test frame for injection
byte TCP_MORE[] =
{
    0x45,0x00,      0x00,0x2b,
    0x33,0x35,      0,0,
    0x40,0x06,      0x00,0x00,
    192,168,2,9,
    192,168,2,42,


    0x08,0x00,      0x00,0x17,
    0x00,0x00,0x00,0x01,    // and send 3
    0x00,0x00,0x00,0x51,    // ack 0x50 bytes
    0x50,0x10,      0x08,0x00,
    0x00,0x00,      0x00,0x00,

    'a', 'b', 'c'
};


// Test frame for injection
byte tcp_syn_from_real_world[] =
{
    0x45, 0x00, 0x00, 0x30,
    0x1b, 0x17, 0x40, 0x00,
    0x80, 0x06, 0x59, 0xcc,
    0xc0, 0xa8, 0x02, 0x6a,
    0xc0, 0xa8, 0x02, 0x2a,

    0x04, 0x70, 0x00, 0x17,
    0xad, 0xc0, 0xcc, 0xc5,
    0x00, 0x00, 0x00, 0x00,
    0x70, 0x02, 0xfa, 0xf0,
    0x83, 0x3c, 0x00, 0x00,
    0x02, 0x04, 0x05, 0xb4,
    0x01, 0x01, 0x04, 0x02
};

// Test frame for injection
byte tcp_is_this_okay1[] =
{
    0x45, 0x00, 0x00, 0x28,
    0xff, 0x7c, 0x00, 0x00,
    0x28, 0x06, 0x0d, 0x6f,
    0xc0, 0xa8, 0x02, 0x2a,
    0xc0, 0xa8, 0x02, 0x6a,
    0x0b, 0xf1, 0x04, 0x42,
    0x00, 0x5a, 0xfe, 0x4c,
    0x2b, 0x57, 0xb6, 0x2a,
    0x50, 0x10, 0x03, 0xe8,
    0x35, 0xac, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00
};


// Test frame for injection
byte tcp_is_this_okay2[] =
{
    0x45, 0x00, 0x00, 0x28,
    0x29, 0x72, 0x00, 0x00,
    0x28, 0x06, 0xe3, 0x79,
    0xc0, 0xa8, 0x02, 0x2a,
    0xc0, 0xa8, 0x02, 0x6a,
    0x13, 0x88, 0x04, 0x5a,
    0x00, 0x00, 0x00, 0x00,
    0xe2, 0x64, 0x9a, 0xc4,
    0x50, 0x14, 0x00, 0x00,
    0x94, 0xe0, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00
};


// Provide a simple testbed user interface
void ether_idle()
{
    static byte buf[100];
    static MSG m;
    MSG *msg;
    msg = &m;
    msg->base = buf;
    msg->size = sizeof(buf);
    msg->offset = 20;
    msg_clear(msg);
    if( _kbhit() )
    {
        int c = _getch();
        if( c == '0' )
        {
            printf( "Sending ARP REPLY frame\n" );
            memcpy( msg_ptr(msg), ARP_REPLY, sizeof(ARP_REPLY) );
            msg_len(msg) = sizeof(ARP_REPLY);
            bmz_up( TASKID_ARP, msg );
        }
        else if( c == '1' )
        {
            printf( "Sending TCP SYN frame\n" );
            memcpy( msg_ptr(msg), TCP_SYN, sizeof(TCP_SYN) );
            msg_len(msg) = sizeof(TCP_SYN);
            bmz_up( TASKID_IP, msg );
        }
        else if( c == '2' )
        {
            printf( "Sending TCP ACK frame\n" );
            memcpy( msg_ptr(msg), TCP_ACK, sizeof(TCP_ACK) );
            msg_len(msg) = sizeof(TCP_ACK);
            bmz_up( TASKID_IP, msg );
        }
        else if( c == '3' )
        {
            printf( "Sending TCP MORE frame\n" );
            memcpy( msg_ptr(msg), TCP_MORE, sizeof(TCP_MORE) );
            msg_len(msg) = sizeof(TCP_MORE);
            bmz_up( TASKID_IP, msg );
        }
        else if( c == '4' )
        {
            printf( "Sending TCP SYN as received in the wild\n" );
            memcpy( msg_ptr(msg), tcp_syn_from_real_world,
                                         sizeof(tcp_syn_from_real_world) );
            msg_len(msg) = sizeof(tcp_syn_from_real_world);
            bmz_up( TASKID_IP, msg );
        }
        else if( c == '5' )
        {
            printf( "Checking a TCP frame, is checksum okay?\n" );
            printf( " (hint: it is but must hack tcp.c pseudo header code\n");
            printf( "  because packet it doesn't know the dst IPADDR)\n" );
            memcpy( msg_ptr(msg), tcp_is_this_okay1,
                                                 sizeof(tcp_is_this_okay1) );
            msg_len(msg) = sizeof(tcp_is_this_okay1);
            bmz_up( TASKID_IP, msg );
        }
        else if( c == '6' )
        {
            printf( "Checking another TCP frame, is checksum okay?\n" );
            printf( " (hint: it is but must hack tcp.c pseudo header code\n");
            printf( "  because packet it doesn't know the dst IPADDR)\n" );
            memcpy( msg_ptr(msg), tcp_is_this_okay2,
                                             sizeof(tcp_is_this_okay2) );
            msg_len(msg) = sizeof(tcp_is_this_okay2);
            bmz_up( TASKID_IP, msg );
        }
    }
}

// Not needed on PC
void ether_timeout( byte timer_id )
{
}

// Don't really want to compile console.c on PC, so reproduce here
void putstr( const char *s )
{
    while( *s )
        putch( *s++ );
}

// Don't really want to compile console.c on PC, so reproduce here
void putu32( u32 n )
{
    u32 digit, power = 1000000000;
    bool nonzero = false;
    while( power )
    {
        digit = n/power;
        if( digit )
            nonzero = true;
        if( nonzero )
            putch( (char)(digit+'0') );
        n = n%power;
        power = power/10;
    }
    if( !nonzero )
        putch( '0' );
}
