/*************************************************************************
 * arp.c
 *
 *  arp = Address Resolution Protocol
 *  Project: eZ2944
 *************************************************************************/
#include <stdio.h>
#include <string.h>
#include "project.h"
#include "bmz.h"
#include "tcpip.h"
#include "arp.h"

// Values of state variable in CACHE_ENTRY
#define IDLE     0
#define WAITING  1
#define BOUND    2

// Misc
#define CACHE_NBR       4
#define OPCODE_REQUEST  1
#define OPCODE_REPLY    2
#define TIMER_RETRY     1
#define TIMER_FLUSH     10*60   // 10 minutes
#define RETRY_LIMIT     3
static const byte eth_broadcast[ETHADDR_LEN] = {0xff,0xff,0xff,0xff,0xff,0xff};
static const byte all_zero     [ETHADDR_LEN] = {0,0,0,0,0,0};

// Cache entry, one for each IP address we talk to
typedef struct
{
    byte    state;
    MQ      mq;
    IPADDR  ipaddr;
    byte    ethaddr[ ETHADDR_LEN ];
    TIMER   timer;
    byte    retry_count;
} CACHE_ENTRY;

// Module data
typedef struct
{
    POOL        pool;
    CACHE_ENTRY cache[CACHE_NBR];
} ARP;
static ARP z;

// Local prototypes
static void send_request( MSG *msg, IPADDR ipaddr );
static void send_reply( MSG *msg, IPADDR target_ipaddr,
                                                 ETHADDR target_ethaddr );
static CACHE_ENTRY *lookup( IPADDR ipaddr );
static CACHE_ENTRY *select( IPADDR ipaddr );

/*************************************************************************
 * Init
 *************************************************************************/
void *arp_init( byte **addr_mem, u16 *addr_len )
{
    byte i;
    CACHE_ENTRY *p;
    TIMER *timer;
    pool_init( &z.pool, addr_mem, addr_len,
                                    CACHE_NBR, ETH_MINFRAME, ETH_OFFSET );
    for( i=0, p=z.cache; i<CACHE_NBR; i++, p++ )
    {
        p->state = IDLE;
        timer = &p->timer;
        timer_reset( timer, i );
        mq_init( &p->mq, addr_mem, addr_len, DEFAULT_MQ_DEPTH );
    }
    return &z;
}

/*************************************************************************
 * Message down
 *************************************************************************/
// Message format in (down from IP);
//      [next_hop_ipaddr,4]
//      [ip hdr]
//      [payload]
// Message format out (down to ETH);
//      [ip ethframe]  = [dst eth][src eth][FRAME_TYPE_IP][ip hdr][payload]
//   OR [arp ethframe] = [dst eth][src eth][FRAME_TYPE_ARP][arp frame]
//                       [arp frame] = [hw type,2]
//                                     [frame type,2]
//                                     [hw len,1]
//                                     [addr len,1]
//                                     [opcode,2]
//                                     [sender ethaddr]
//                                     [sender ipaddr]
//                                     [target ethaddr]
//                                     [target ipaddr]
void arp_down( MSG *msg )
{
    CACHE_ENTRY *p;
    IPADDR ipaddr;

    // Take off parameter
    ipaddr  = msg_pop4( msg );

    // Do we have a hardware address for this IP address ?
    p = lookup(ipaddr);
    if( p )
    {

        // Yes, add ethernet prefix and send
        msg_push2( msg, FRAME_TYPE_IP );
        msg_push6( msg, config.my_ethaddr );    // src
        msg_push6( msg, p->ethaddr );           // dst
        bmz_down( TASKID_ETHER, msg );
    }
    else
    {

        // No, select a cache slot ...
        p = select(ipaddr);

        // ... and queue the message in anticipation of getting
        //  the IP address later
        if( !mq_write(&p->mq,msg) )
            msg_free(msg);

        // If necessary, kick off an ARP request
        if( p->state == IDLE )
        {
            msg = pool_idx( &z.pool, (byte)(p-z.cache) );
            send_request( msg, ipaddr );
            p->ipaddr = ipaddr;
            p->state  = WAITING;
            p->retry_count = 0;
            timer_start_seconds( &p->timer, TIMER_RETRY );
        }
    }
}


/*************************************************************************
 * Message up
 *************************************************************************/
// Message format in (up from ETH);
//      [arp frame] = [hw type,2]
//                    [frame type,2]
//                    [hw len,1]
//                    [addr len,1]
//                    [opcode,2]
//                    [sender ethaddr]
//                    [sender ipaddr]
//                    [target ethaddr]
//                    [target ipaddr]
// Message format out (down to ETH);
//      [ip ethframe]  = [dst eth][src eth][FRAME_TYPE_IP][ip hdr][payload]
//   OR [arp ethframe] = [dst eth][src eth][FRAME_TYPE_ARP][arp frame]
void arp_up( MSG *msg )
{
    MSG  *reply, *queued;
    byte oldstate;

    // Get message fields
    u16  hw_type             = msg_read2(msg,0);
    u16  frame_type          = msg_read2(msg,2);
    byte hw_len              = msg_read1(msg,4);
    byte addr_len            = msg_read1(msg,5);
    u16  opcode              = msg_read2(msg,6);
    ETHADDR  sender_ethaddr  = msg_readp(msg,8);
    IPADDR   sender_ipaddr   = msg_read4(msg,14);
    ETHADDR  target_ethaddr  = msg_readp(msg,18);
    IPADDR   target_ipaddr   = msg_read4(msg,24);

    // Do we care about it ?
    if( hw_type    == 1                 &&
        frame_type == FRAME_TYPE_IP     &&
        hw_len     == ETHADDR_LEN       &&
        addr_len   == IPADDR_LEN        &&
        (config.my_ipaddr==target_ipaddr )   &&
        (opcode==OPCODE_REPLY || opcode==OPCODE_REQUEST)
      )
    {

        // If it's a request, then reply
        CACHE_ENTRY *p = select(sender_ipaddr);
        if( opcode == OPCODE_REQUEST  )
        {
            reply = pool_idx( &z.pool, (byte)(p-z.cache) );
            send_reply( reply, sender_ipaddr, sender_ethaddr );
        }

        // Save the info in a BOUND cache entry
        oldstate  = p->state;
        p->state  = BOUND;
        p->ipaddr = sender_ipaddr;
        memcpy( p->ethaddr, sender_ethaddr, ETHADDR_LEN );

        // Flush BOUND entries regularly
        timer_start_seconds( &p->timer, TIMER_FLUSH );
        if( oldstate == WAITING )
        {

            // If we were WAITING for the physical address,
            //  we can now dequeue waiting frames and send
            //  them
            while( NULL != (queued=mq_read(&p->mq)) )
            {
                msg_push2( queued, FRAME_TYPE_IP );
                msg_push6( queued, config.my_ethaddr );    // src
                msg_push6( queued, p->ethaddr );           // dst
                bmz_down( TASKID_ETHER, queued );
            }
        }
    }

    // Free the message
    msg_free(msg);
}


/*************************************************************************
 * Timeout
 *************************************************************************/
void arp_timeout( byte timer_id )
{
    CACHE_ENTRY *p = &z.cache[timer_id];
    MSG *msg;

    // Flush old entries
    if( p->state == BOUND )
        p->state = IDLE;

    // Retry if still waiting
    else if( p->state == WAITING )
    {
        if( p->retry_count <= RETRY_LIMIT )
        {
            p->retry_count++;
            msg = pool_idx( &z.pool, (byte)(p-z.cache) );
            send_request( msg, p->ipaddr );
            timer_start_seconds( &p->timer, TIMER_RETRY );
        }

        // Give up eventually
        else
        {
            mq_clear( &p->mq );
            p->state = IDLE;
        }
    }
}


/*************************************************************************
 * Send an arp request
 *************************************************************************/
static void send_request( MSG *msg, IPADDR ipaddr )
{
    msg_clear ( msg );
    msg_push2 ( msg, FRAME_TYPE_ARP );      // 2 eth type = ARP
    msg_push6 ( msg, config.my_ethaddr );   // 6 eth src
    msg_push6 ( msg, eth_broadcast  );      // 6 eth dst
    msg_write2( msg, 1              );      // 2 hw type = eth = 1
    msg_write2( msg, FRAME_TYPE_IP  );      // 2 protocol type = IP
    msg_write1( msg, ETHADDR_LEN    );      // 1 hw addr len, eth = 6
    msg_write1( msg, IPADDR_LEN     );      // 1 protocol addr len,ip=4
    msg_write2( msg, OPCODE_REQUEST );      // 2 ARP request
    msg_write6( msg, config.my_ethaddr );   // 6 sender eth addr
    msg_write4( msg, config.my_ipaddr  );   // 4 sender ip addr
    msg_write6( msg, all_zero       );      // 6 target eth addr
    msg_write4( msg, ipaddr         );      // 4 target ip addr
    bmz_down( TASKID_ETHER, msg );
}

/*************************************************************************
 * Send an arp reply
 *************************************************************************/
static void send_reply( MSG *msg, IPADDR target_ipaddr,
                                                   ETHADDR target_ethaddr )
{
    msg_clear ( msg );
    msg_push2 ( msg, FRAME_TYPE_ARP );      // 2 eth type = ARP
    msg_push6 ( msg, config.my_ethaddr );   // 6 eth src
    msg_push6 ( msg, target_ethaddr );      // 6 eth dst
    msg_write2( msg, 1              );      // 2 hw type = eth = 1
    msg_write2( msg, FRAME_TYPE_IP  );      // 2 protocol type = IP
    msg_write1( msg, ETHADDR_LEN    );      // 1 hw addr len, eth = 6
    msg_write1( msg, IPADDR_LEN     );      // 1 protocol addr len,ip=4
    msg_write2( msg, OPCODE_REPLY   );      // 2 ARP reply
    msg_write6( msg, config.my_ethaddr );   // 6 sender eth addr
    msg_write4( msg, config.my_ipaddr  );   // 4 sender ip addr
    msg_write6( msg, target_ethaddr );      // 6 target eth addr
    msg_write4( msg, target_ipaddr  );      // 4 target ip addr
    bmz_down( TASKID_ETHER, msg );
}

/*************************************************************************
 * Try to find a bound cache entry for this IP address
 *************************************************************************/
static CACHE_ENTRY *lookup( IPADDR ipaddr )
{
    CACHE_ENTRY *p, *found=NULL;
    byte i;
    for( i=0, p=z.cache; i<CACHE_NBR; i++, p++ )
    {
        if( p->state==BOUND && p->ipaddr==ipaddr )
        {
            found = p;
            break;
        }
    }
    return( found );
}

/*************************************************************************
 * Select a cache entry for this IP address
 *************************************************************************/
static CACHE_ENTRY *select( IPADDR ipaddr )
{
    CACHE_ENTRY *p, *found=NULL, *idle=NULL, *oldest=NULL,
                                                     *very_reluctant=NULL;
    u32  time_remaining;
    u32  min_bound          = (u32)(-1);   //ULONG_MAX
    byte i, max_retry_count = 0;

    // Loop until done or ideal candidate found
    for( i=0, p=z.cache; !found && i<CACHE_NBR; i++, p++ )
    {

        // Choose a candidate based on state
        switch( p->state )
        {

            // If bound, a matching candidate is ideal, otherwise
            //  the oldest one is most desirable
            case BOUND:
            {
                if( p->ipaddr == ipaddr )
                    found = p;
                else
                {
                    time_remaining = timer_read(&p->timer);
                    if( time_remaining <= min_bound )
                    {
                        min_bound = time_remaining;
                        oldest = p;
                    }
                }
                break;
            }

            // First idle entry found is a good candidate
            case IDLE:
            {
                if( !idle )
                    idle = p;
                break;
            }

            // If waiting, a matching candidate is ideal, otherwise
            //  we are very reluctant, but if we must, then choose one
            //  close to timing out
            case WAITING:
            {
                if( p->ipaddr == ipaddr )
                    found = p;
                else if( p->retry_count >= max_retry_count )
                {
                    max_retry_count = p->retry_count;
                    very_reluctant = p;
                }
                break;
            }
        }
    }

    // Pick out the most desirable candidate
    if( !found )
    {
        if( idle )
            found = idle;
        else if( oldest )
            found = oldest;
        else
        {

            // If we must choose a waiting one, clear it out
            //  (don't imagine this would ever happen)
            found = very_reluctant;
            assert( found );
            mq_clear( &very_reluctant->mq );
            timer_stop( &very_reluctant->timer );
            very_reluctant->state = IDLE;
        }
    }
    return( found );
}
