/*************************************************************************
 * tcpip.h
 *
 *  TCP/IP well known numbers, and address types
 *  Project: eZ2944
 *************************************************************************/
#ifndef TCPIP_H
#define TCPIP_H
#include "bmz.h"

// Address types
typedef u32 IPADDR;
typedef byte *ETHADDR;

// Protocol numbers, for multiplexing up from IP
#define PROTOCOL_UDP  17
#define PROTOCOL_TCP  6
#define PROTOCOL_ICMP 1

// Frame type numbers, for multiplexing up from ethernet
#define FRAME_TYPE_IP  0x0800
#define FRAME_TYPE_ARP 0x0806

// TCP code bits
#define URG_BIT 0x20
#define ACK_BIT 0x10
#define PSH_BIT 0x08
#define RST_BIT 0x04
#define SYN_BIT 0x02
#define FIN_BIT 0x01

#endif // TCPIP_H
