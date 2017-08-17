/*************************************************************************
 * project.c
 *
 *  Project description and startup
 *  Project: eZ2944
 *************************************************************************/
#ifndef PROJECT_H
#define PROJECT_H
#include "bmz.h"
#include "tcpip.h"
#include "lengths.h"

// Define task ids
//  Note: TASKID_NULL is #defined as 0 in bmz.h
#define TASKID_TCPAPP1      1
#define TASKID_TCPAPP2      2
#define TASKID_TCPSOCK1     3
#define TASKID_TCPSOCK2     4
#define TASKID_ETHER        5
#define TASKID_ARP          6
#define TASKID_IP           7
#define TASKID_ICMP         8
#define TASKID_TCP          9
    // Note that order is significant. If task A feeds task B's queue, put
    // task A in front of task B (eg TCPAPP1 ahead of TCPSOCK1). Put tasks
    // without idle routines or queues last, the run loop will be shortened
    // to exclude them (eg ARP,IP,ICMP and TCP)

// Hardwired configuration information
typedef struct
{
    IPADDR my_ipaddr;
    IPADDR subnet_mask;
    IPADDR default_route;
    byte   my_ethaddr[ ETHADDR_LEN ];
} CONFIG;
extern const CONFIG config;
#endif // PROJECT_H
