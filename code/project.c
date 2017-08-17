/*************************************************************************
 * project.c
 *
 *  Project description and startup
 *  Project: eZ2944
 *************************************************************************/
#include <stdio.h>
#include "bmz.h"
#include "console.h"
#include "tserver.h"
#include "tcpsock.h"
#include "tcp.h"
#include "ip.h"
#include "icmp.h"
#include "arp.h"
#include "ether.h"
#include "project.h"

/*************************************************************************
 * Define the tasks in the system
 *************************************************************************/
static const TASK_DESCRIPTOR task_descriptors[] =
{
    // Ethernet driver
    {   TASKID_ETHER,        // TASKID
        ether_init,          // init handler
        ether_idle,          // idle handler
        ether_timeout,       // timeout handler
        ether_down,          // down handler
        NULL,                // up handler
        0,      // no sw queue needed
    //@ DEFAULT_MQ_DEPTH,    // mq down depth
        0,                   // mq up depth
        0,                   // pool nbr
        0,                   // pool len
        0,                   // pool offset
        TASKID_NULL          // share pool of this TASKID
    },

    // ARP
    {   TASKID_ARP,          // TASKID
        arp_init,            // init handler
        NULL,                // idle handler
        arp_timeout,         // timeout handler
        arp_down,            // down handler
        arp_up,              // up handler
        0,                   // mq down depth
        0,                   // mq up depth
        0,                   // pool nbr
        0,                   // pool len
        0,                   // pool offset
        TASKID_NULL          // share pool of this TASKID
    },

    // IP
    {   TASKID_IP,           // TASKID
        ip_init,             // init handler
        NULL,                // idle handler
        NULL,                // timeout handler
        ip_down,             // down handler
        ip_up,               // up handler
        0,                   // mq down depth
        0,                   // mq up depth
        0,                   // pool nbr
        0,                   // pool len
        0,                   // pool offset
        TASKID_NULL          // share pool of this TASKID
    },

    // ICMP
    {   TASKID_ICMP,         // TASKID
        NULL,                // init handler
        NULL,                // idle handler
        NULL,                // timeout handler
        NULL,                // down handler
        icmp_up,             // up handler
        0,                   // mq down depth
        0,                   // mq up depth
        0,                   // pool nbr
        0,                   // pool len
        0,                   // pool offset
        TASKID_TCPSOCK1      // share pool of this TASKID
                             //  (only used to send a reply)
    },

    // TCP
    {   TASKID_TCP,          // TASKID
        NULL,                // init handler
        NULL,                // idle handler
        NULL,                // timeout handler
        tcp_down,            // down handler
        tcp_up,              // up handler
        0,                   // mq down depth
        0,                   // mq up depth
        0,                   // pool nbr
        0,                   // pool len
        0,                   // pool offset
        TASKID_TCPSOCK1      // share pool of this TASKID
    },

    // TCPSOCK1
    {   TASKID_TCPSOCK1,     // TASKID
        tcpsock_init,        // init handler
        NULL,                // idle handler
        tcpsock_timeout,     // timeout handler
        tcpsock_down,        // down handler
        tcpsock_up,          // up handler
      2*DEFAULT_MQ_DEPTH,    // mq down depth
        0,                   // mq up depth
        DEFAULT_POOL_NBR,    // pool nbr
        DEFAULT_POOL_LEN,    // pool len
        DEFAULT_POOL_OFFSET, // pool offset
        TASKID_NULL          // share pool of this TASKID
    },

    // TCPSOCK2
    {   TASKID_TCPSOCK2,     // TASKID
        tcpsock_init,        // init handler
        NULL,                // idle handler
        tcpsock_timeout,     // timeout handler
        tcpsock_down,        // down handler
        tcpsock_up,          // up handler
     20,// DEFAULT_MQ_DEPTH, // mq down depth
        0,                   // mq up depth
        0,                   // pool nbr
        0,                   // pool len
        0,                   // pool offset
        TASKID_TCPSOCK1      // share pool of this TASKID
                             //  (only used to send a RST)
    },

    // TSERVER1,
    {   TASKID_TCPAPP1,      // TASKID
        tserver_init,        // init handler
        tserver_idle,        // idle handler
        NULL,                // timeout handler
        NULL,                // down handler
        tserver_up,          // up handler
        0,                   // mq down depth
        0,                   // mq up depth
        12,                  // pool nbr
        40,                  // pool len
        1,                   // pool offset
        TASKID_NULL          // share pool of this TASKID
    },

    // TSERVER2,
    {   TASKID_TCPAPP2,      // TASKID
        tserver_init,        // init handler
        tserver_idle,        // idle handler
        NULL,                // timeout handler
        NULL,                // down handler
        tserver_up,          // up handler
        0,                   // mq down depth
        0,                   // mq up depth
        0,                   // pool nbr
        0,                   // pool len
        0,                   // pool offset
        TASKID_TCPAPP1       // share pool of this TASKID
    }
};


/*************************************************************************
 * Configuration information
 *************************************************************************/
const CONFIG config =
{
    // Define macro to make life easier
    #define MK_IPADDR(a,b,c,d) \
    ( (((u32)(a))<<24) | (((u32)(b))<<16) | (((u32)(c))<<8)  | ((u32)(d)) )

    // my_ipaddr    = 192.168.2.42
    MK_IPADDR(192,168,2,42),

    // subnet_mask  = 255.255.255.0
    MK_IPADDR(255,255,255,0),

    // default_route= 192.168.2.9
    MK_IPADDR(192,168,2,9),

    // my_ethaddr = 0xaa,0xaa,0x12,0x34,0x56,0x78
    { 0xaa, 0xaa, 0x12, 0x34, 0x56, 0x78 }
};

/*************************************************************************
 * Main entry point
 *************************************************************************/
int main()
{
    static byte buf[5364];  // Tune this so that BSS leaves some room for
                            //  stack - consult map and leave about
                            //  0x300 bytes for stack
    byte *memory = buf;
    u16   memlen = sizeof(buf);
    bmz_init();
    bmz_define_system(  task_descriptors,
                        nbrof(task_descriptors),
                        &memory,
                        &memlen );
    //putstr( "Memory left is " );
    //putu32( memlen );
    //putstr( "\n" );
    bmz_run();
}

