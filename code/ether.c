/*************************************************************************
 * ether.c
 *
 *  eZ80F91 Ethernet driver
 *  Project: eZ2944
 *************************************************************************/
#include <stdio.h>
#include <string.h>
#include <ez80.h>
#include "bmz.h"
#include "project.h"
#include "tcpip.h"
#include "console.h"
#include "ether.h"

// TX descriptor flags
#define EMAC_OWNS       0x8000   // 1=mac owns
#define HOST_OWNS       0x0000   // 0=host owns

// RX descriptor flags
#define RX_OK           0x8000   // received frame okay
#define RX_OVR          0x0001   // rx overrun

// Descriptor HW structure
typedef struct
{
   byte    *np;
   u16      len;
   u16      flags;
} DESC;

// More HW definitions
#define TXDONE      0x01        // in register EMAC_ISTAT
#define RXDONE      0x08        // in register EMAC_ISTAT
#define RXOVRRUN    0x04        // in register EMAC_ISTAT
#define TXDONE_IEN  0x01        // in register EMAC_IEN
#define RXDONE_IEN  0x08        // in register EMAC_IEN
#define LCTLD       0x80        // in register EMAC_MIIMGT
#define RSTAT       0x40        // in register EMAC_MIIMGT
#define CLKDIV20    0x06        // in register EMAC_MIIMGT
#define MGTBUSY     0x80        // in register EMAC_MIISTAT
#define SRST        0x20        // in register EMAC_RST
#define HRTFN       0x10        // in register EMAC_RST
#define HRRFN       0x08        // in register EMAC_RST
#define HRTMC       0x04        // in register EMAC_RST
#define HRRMC       0x02        // in register EMAC_RST
#define HRMGT       0x01        // in register EMAC_RST
#define GPRAM_EN    0x80        // in register RAM_CTL
#define ERAM_EN     0x40        // in register RAM_CTL
#define PADEN       0x80        // in register EMAC_CFG1
#define CRCEN       0x10        // in register EMAC_CFG1
#define FULLD       0x08        // in register EMAC_CFG1
#define RXEN        0x01        // in register EMAC_CFG4
#define BROADCAST   0x01        // in register EMAC_AFR

// PHY registers (common to AMD 79C874 and Micrel KS8721BL)
#define PHY_CREG                       0
#define PHY_SREG                       1
#define PHY_MODE_CTRL_REG              21

// PHY register 0 bit definitions (AMD and Micrel)
#define PHY_100BT                      0x2000
#define PHY_AUTO_NEG_ENABLE            0x1000
#define PHY_POWER_DOWN                 0x0800
#define PHY_RESTART_AUTO_NEG           0x0200
#define PHY_FULLD                      0x0100
#define PHY_10BT                       0
#define PHY_HALFD                      0

// PHY register 1 bit definitions (AMD and Micrel)
#define PHY_AUTO_NEG_COMPLETE          0x0020
#define PHY_AUTO_NEG_CAPABLE           0x0008
#define PHY_LINK_ESTABLISHED           0x0004

// Misc
#define EMAC_SRAM_SIZE                 0x2000
#define EMAC_SRAM_OFFSET               0xC000
#define MAXFRAMESIZE                   1514
#define MINFRAMESIZE                   60

// Module variables
typedef struct
{
    // Status
    bool link_operational;

    // Retry timer
    TIMER timer;

    // Location of internal EMAC SRAM
    byte *emac_sram_base;
    byte *wrap; // store a wrapped rx frame at top of memory

    // SW pointers into EMAC SRAM, match HW equivalents
    byte *twp;
    byte *trp;
    byte *rrp;
    byte *tlbp;
    byte *bp;
    byte *rhbp;
} ETHER;
static ETHER z;

// Macro to align a ptr to next 32 byte boundary if it is not already at a
//  a 32 byte boundary. (How does it work? The mask operation chops down
//  to next lowest 32 byte boundary, taking off up to 31 bytes [it doesn't
//  takes off any if we are at a 32 byte boundary]. So adding 31 bytes
//  first means we round up to next boundary, unless we are on a boundary.)
#define ALIGN 32
#define ALIGN32(ptr) (   (byte *) \
                            ( \
                                (  (int)(ptr) + 31 )  \
                                    & 0xffffe0 \
                            ) \
                      )

// Module prototypes
static void say( const char *s );
static void phy_write( u16 reg, u16 data );
static u16  phy_read( u16 reg );
static bool link_init();
static void show( MSG *msg, bool rx );

/*************************************************************************
 * Init
 *************************************************************************/
void *ether_init( byte **addr_mem, u16 *addr_len )
{
    bool err;
    u16    i;

    // Point at, enable, and clear internal EMAC SRAM
    z.emac_sram_base = (byte *)
            (  (((u32)RAM_ADDR_U) << 16) + (byte*)EMAC_SRAM_OFFSET   );
    RAM_CTL |= ERAM_EN;
    memset( z.emac_sram_base, 0, EMAC_SRAM_SIZE );

    // Reset EMAC hw
    EMAC_IEN = 0;
    EMAC_RST = SRST | HRTFN | HRRFN | HRTMC | HRRMC | HRMGT;

    // Set address
    ether_set_addr( config.my_ethaddr );

    // Disable EMAC interrupts
    EMAC_IEN = 0;

    // Clear interrupt status flags
    EMAC_ISTAT = 0xff;

    // Setup EMAC "buffer size", (i.e. alignment / chunk size) to 32
    EMAC_BUFSZ = 0xc0;  // set 32 byte buffers (chunks)

    // Set pause frame control timeout value
    EMAC_TPTV_L = 0x14;
    EMAC_TPTV_H = 0x00;

    // Define EMAC SRAM region sizes
    #define REGION_WRAP_SIZE 0x600      // one max sized RX frame, aligned
    #define REGION_TX_SIZE   672        // largest tx frame we send = 576,
                                        //  aligned and with a margin

    // Set EMAC SRAM boundary ptrs to divide SRAM into regions
    z.wrap = z.emac_sram_base;          // WRAP region, allow room for a copy
                                        //  of one max sized RX frame that
                                        //  would otherwise be wrapped
    z.tlbp = z.emac_sram_base +         // TX region
                 REGION_WRAP_SIZE;
    z.rhbp = z.emac_sram_base +         // bottom, end of RX region
                 EMAC_SRAM_SIZE;

    // Only need room for one max size frame in tx region
    z.bp   = z.tlbp + REGION_TX_SIZE;
    z.bp   = ALIGN32(z.bp);

    // Set SW ptrs to EMAC SRAM ring buffers
    z.twp = z.trp = z.tlbp;
    z.rrp = z.bp;

    // Set HW ptrs from SW ptrs
    EMAC_TLBP_L = (byte) z.tlbp;
    EMAC_TLBP_H = (byte)( ((u32)z.tlbp) >> 8  );
    EMAC_BP_L   = (byte) z.bp;
    EMAC_BP_H   = (byte)( ((u32)z.bp)   >> 8  );
    EMAC_BP_U   = (byte)( ((u32)z.bp)   >> 16 );
    EMAC_RHBP_L = (byte) z.rhbp;
    EMAC_RHBP_H = (byte)( ((u32)z.rhbp) >> 8  );
    EMAC_RRP_L  = (byte)( (u32)z.rrp  );
    EMAC_RRP_H  = (byte)( ((u32)z.rrp) >> 8 );

    // Host owns first buffer
    ((DESC *)z.twp)->flags = 0x0000;

    // Set polling timer for minimum timeout period
    EMAC_PTMR = 1;

    // Disable EMAC test modes
    EMAC_TEST = 0;

    // Enable padding, crc generation, full duplex mode
    EMAC_CFG1 = PADEN | CRCEN | FULLD;

    // Set the late collision to default
    EMAC_CFG2 = 0x37;

    // Set retransmission to default
    EMAC_CFG3 = 15;

    // Enable normal receives only
    EMAC_CFG4 = RXEN;

    // Set address filter to broadcast only
    EMAC_AFR = BROADCAST;

    // Set the max bytes per packet to default value of 1536
    EMAC_MAXF_L = 0x00;
    EMAC_MAXF_H = 0x06;

    // Take EMAC out of reset
    EMAC_RST = 0;

    // Clear interrupt status register
    EMAC_ISTAT = 0xff;

    // Init the link
    timer_reset( &z.timer, 0 );
    err = link_init();
    z.link_operational = !err;
    if( !err )
        bmz_set_publish_state( PUBLISH_ACTIVE );
    else
    {
        timer_start_seconds( &z.timer, 10 );
        bmz_set_publish_state( PUBLISH_OTHER );
    }

    // Setup EMAC interrupts (currently not needed)
    //#define EMAC_RX_IVECT 0x40
    //#define EMAC_TX_IVECT 0x44
    //set_vector( EMAC_RX_IVECT, emacisr_rx );
    //set_vector( EMAC_TX_IVECT, emacisr_tx );

    // Enable required interrupts (currently none)
    EMAC_IEN = 0;
    return( &z );
}

/*************************************************************************
 * Timeout, try to initialize link again
 *************************************************************************/
void ether_timeout( byte timer_id )
{
    bool err;
    u32 after, before = tick_get();
    err = link_init();
    after  = tick_get();
    z.link_operational = !err;
    if( !err )
        bmz_set_publish_state( PUBLISH_ACTIVE );
    else
        timer_start_ticks( &z.timer, SECS_TO_TICKS(10) + (after-before) );
}

/*************************************************************************
 * Message down
 *************************************************************************/
void ether_down( MSG *msg )
{
    DESC *desc;
    byte *np, *dst, *src;
    u16   len, phase1;
    bool  okay=false;
    byte lo, hi, lo2, hi2;
    byte *trp;

    // Pad message to 60 bytes (not strictly needed as EMAC pads in hardware)
    if( msg_len(msg) < MINFRAMESIZE )
    {
        byte pad_bytes = MINFRAMESIZE-msg_len(msg);
        if( msg_ptr(msg) + MINFRAMESIZE <= msg->base+msg->size )
            memset( msg_ptr(msg) + msg_len(msg), 0, pad_bytes );
        else
            putstr( "Pad bytes are junk\n" );
        msg_len(msg) = MINFRAMESIZE;
    }

    // Debug dump of frame
    #ifdef DEBUG_TX_FRAME
    show( msg, false );
    #endif

    // For debugging, test retries by throwing away regular frames
    #ifdef DEBUG_TX_DISCARD
    {
        static byte discard;
        discard = 0x1f&(discard+1);
        if( discard == 0 )
        {
            msg_free(msg);
            printf( "*DISCARDED TX*\n" );
            return;
        }
    }
    #endif

    // Wait for hardware TRP to catch twp
    while( z.link_operational )
    {
        do
        {
            lo  = EMAC_TRP_L;
            hi  = EMAC_TRP_H;
            lo2 = EMAC_TRP_L;
            hi2 = EMAC_TRP_H;
        }
        while( lo!=lo2 || hi!=hi2 );
        trp = z.emac_sram_base  +  lo  +  ( ((u16)hi) << 8 );
        if( z.twp == trp )
        {
            okay = true;
            break;
        }
    }
    //DBG printf( "z.twp = %08lx\n", (u32) z.twp );
    //DBG printf( "TRP = %08lx\n", (u32) trp );

    // If not discarding, write to HW tx buffer
    if( okay )
    {

        // Assert that we own the descriptor
        desc = (DESC *)z.twp;
        assert( !(desc->flags & EMAC_OWNS) );

        // Calculate the next pointer
        np = z.twp + sizeof(DESC) + msg_len(msg);
        np = ALIGN32(np);
        if( np >= z.bp )  // check if we have wrapped
        {
            //DBG printf( "TX wrap 1, %u\n", (u16) (np-z.bp) );
            np = z.tlbp + (np-z.bp);
        }
        desc->np = np;      // note that np is at least 32 bytes from bp,
                            //  because ALIGN32 aligns it to a 32 byte
                            //  boundary (so can write a DESC at np without
                            //  fear of overrunning bp)

        // Set the packet size
        len = msg_len(msg);
        desc->len = len;

        // Move the data to the EMAC SRAM TX ring buffer
        dst    = z.twp + sizeof(DESC);
        src    = msg_ptr(msg);
        phase1 = z.bp-dst;
        if( len <= phase1 )
            memcpy( dst, src, len );
        else
        {
            //DBG printf( "TX wrap 2, %u\n", (u16) (len-phase1) );
            memcpy( dst, src, phase1 );
            memcpy( z.tlbp, src+phase1, len-phase1 );
        }

        // Update twp, HW TRP will catch up when the frame is TXed
        z.twp = np;
        //DBG printf( "Updated z.twp = %08lx\n", (u32) z.twp );

        // To queue one packet for transmission, set next packet to HOST_OWNS,
        //  this packet to EMAC_OWNS
        ((DESC *)np)->flags = HOST_OWNS;
        desc->flags = EMAC_OWNS;
    }

    // Free msg
    msg_free(msg);
}

/*************************************************************************
 * Idle handler, send received frames up stack
 *************************************************************************/
void ether_idle()
{
    DESC *desc;
    MSG  *msg;
    byte *rwp;
    u16  len, phase1;
    bool okay, skip=false;
    byte lo,hi,lo2,hi2;
    u16  frame_type;

    // Check blocks left for receive overrun
    #if 0
    static u16 idx;
    idx++;
    if( (idx&0x3ff) == 0 )
    {
        u16 blkslft = EMAC_BLKSLFT_L  +  ( ((u16)EMAC_BLKSLFT_H) << 8 );
        EMAC_ISTAT = RXOVRRUN;
        printf( "blkslft = %u\n", blkslft );
    }
    #endif

    // Read RWP HW register
    do
    {
        lo  = EMAC_RWP_L;
        hi  = EMAC_RWP_H;
        lo2 = EMAC_RWP_L;
        hi2 = EMAC_RWP_H;
    }
    while( lo!=lo2 || hi!=hi2 );
    rwp = z.emac_sram_base  +  lo  +  ( ((u16)hi) << 8 );

    // If rrp lags rwp, we have frames to read
    if( z.rrp != rwp )
    {

        // Read frame from EMAC SRAM
        desc  = (DESC *)z.rrp;
        okay  = ( (desc->flags & RX_OK) ? true : false );
        len   = desc->len - 4; // take off CRC
        if( len < MINFRAMESIZE )
            okay = false;
        if( len > MAXFRAMESIZE )
            okay = false;
        if( desc->flags & RX_OVR || desc->np==NULL )
        {

            // If overrun, write off what we have, set HW RRP to
            //  catch up with HW RWP
            EMAC_ISTAT = RXOVRRUN;
            z.rrp = rwp;
            EMAC_RRP_L = (byte)( (u32)rwp  );
            EMAC_RRP_H = (byte)( ((u32)rwp) >> 8 );
            skip = true;
            okay = false;
        }
        if( !okay )
        {
            // Update rrp to next pkt
            if( !skip )
                z.rrp = desc->np;
            //DBG printf( "RX updated z.rrp = %08lx\n", (u32) z.rrp );
        }
        else
        {

            // Now we do a trick, instead of allocating a MSG to hold the
            //  frame, we make the frame into a MSG. This avoids a copy,
            //  and so saves time AND space. We first create a 12 byte
            //  MSG structure on top of the first 12 bytes of the ethernet
            //  frame, which are ethernet dest and source address fields
            //  respectively. Luckily we don't need either of them.
            assert( sizeof(MSG) == 12 );
            len -= sizeof(MSG);
            msg = (MSG *)(z.rrp + sizeof(DESC));
            msg->inuse  = MSG_INUSE_USER;   // so that our msg_free() routine
                                            //  is used
            msg->offset = 2;    // pretend the first two bytes of the
                                //  message, which are the ethertype have
                                //  been pushed on the front (see msg_pop2()
                                //  below)
            msg->base   = z.rrp + sizeof(DESC) + sizeof(MSG);
            msg->ptr    = msg->base;
            msg->size   = len;
            msg->len    = len;
            //DBG printf( "RX z.rrp = %08lx\n", (u32) z.rrp );

            // We don't want a wrapped around message, so in the special
            //  case of a wrapped frame, copy the frame data to the
            //  reserved WRAP region at the front of the EMAC SRAM, the
            //  MSG structure stays in exactly the same place (only the
            //  ptrs to the data change).
            phase1 = z.rhbp - msg->ptr;
            if( len > phase1 )
            {
                //DBG printf( "RX wrap\n" );
                memcpy( z.wrap, msg->ptr, phase1 );
                memcpy( z.wrap+phase1, z.bp, len-phase1 );
                msg->base   = z.wrap;
                msg->ptr    = msg->base;
            }

            // Note where we are so that user_msg_free() can release memory
            //  back to EMAC
            z.rrp = desc->np;

            // Debug receive frames
            #ifdef DEBUG_RX_FRAME
            show( msg, true );
            #endif

            // Send the frame to known protocols
            frame_type = msg_pop2( msg );
            if( frame_type == FRAME_TYPE_IP )
            {

                // For debugging, test retries by throwing away regular frames
                #ifdef DEBUG_RX_DISCARD
                static byte discard=0x10;
                discard = 0x1f&(discard+1);
                if( discard == 0 )
                {
                    msg_free(msg);
                    printf( "*DISCARDED RX*\n" );
                }
                else
                #endif
                    bmz_up( TASKID_IP, msg );
            }
            else if( frame_type == FRAME_TYPE_ARP )
                bmz_up( TASKID_ARP, msg );

            // Discard if not known protocol
            else
                msg_free(msg);
        }
    }
}

/*************************************************************************
 * Hook into special user MSG free routine, for emac rx messages
 *************************************************************************/
void user_msg_free( MSG *msg )
{
    byte *frame, *rrp;

    // Mark free
    msg->inuse = 0;

    // Point at descriptor table for this frame
    frame = (byte *)msg - sizeof(DESC);

    // Does it match HW RRP ?
    rrp = z.emac_sram_base  +  EMAC_RRP_L  +  ( ((u16)EMAC_RRP_H) << 8 );
    if( frame == rrp )
    {

        // If so release it back to EMAC ownership by bumping RRP to
        //  next frame
        for(;;)
        {
            frame = ((DESC *)frame)->np;
            EMAC_RRP_L = (byte)( (u32)frame  );
            EMAC_RRP_H = (byte)( ((u32)frame) >> 8 );

            // If we have sent no more messages stop
            if( frame == z.rrp )
                break;

            // Otherwise we have sent this frame as a MSG, so see if
            //  it has been freed by a previous user_msg_free()
            msg = (MSG *)( frame + sizeof(DESC));
            if( msg->inuse )
                break;  // no it hasn't so stop
                        // else loop around, potentially freeing more
                        //  frame memory to the EMAC
        }
    }
}


/*************************************************************************
 * Room in rx ring buffer
 *************************************************************************/
u16 ether_rx_room()
{
    u16  room, size=z.rhbp-z.bp;
    byte *rwp, *rrp;
    byte lo,hi,lo2,hi2;

    // Read RWP HW register
    do
    {
        lo  = EMAC_RWP_L;
        hi  = EMAC_RWP_H;
        lo2 = EMAC_RWP_L;
        hi2 = EMAC_RWP_H;
    }
    while( lo!=lo2 || hi!=hi2 );
    rwp = z.emac_sram_base  +  lo  +  ( ((u16)hi) << 8 );

    // Compare to RRP HW register
    rrp = z.emac_sram_base  +  EMAC_RRP_L  +  ( ((u16)EMAC_RRP_H) << 8 );
    if( rwp >= rrp )
        room = size - (rwp-rrp);
    else
        room = rrp-rwp;
    return( room );
}


/*************************************************************************
 * Set hardware address
 *************************************************************************/
void ether_set_addr( const byte *ethaddr )
{
    // Setup the EMAC station address
    EMAC_STAD_0 = *(ethaddr);
    EMAC_STAD_1 = *(ethaddr+1);
    EMAC_STAD_2 = *(ethaddr+2);
    EMAC_STAD_3 = *(ethaddr+3);
    EMAC_STAD_4 = *(ethaddr+4);
    EMAC_STAD_5 = *(ethaddr+5);
}


/*************************************************************************
 * Debug dump of message
 *************************************************************************/
static void show( MSG *msg, bool rx )
{
    bool dump=true;
    u16 i, len;
    byte *ptr = msg_ptr(msg);
    if( rx )
    {
        if( msg_read2(msg,0) != FRAME_TYPE_IP )
            dump = false;
        else
        {
            byte protocol=msg_read1(msg,11);
            if( protocol == PROTOCOL_UDP )
            {
                dump = false;
                printf( "UDP" );
            }
            else if( protocol!=PROTOCOL_TCP && protocol!=PROTOCOL_ICMP )
                dump = false;
        }
    }
    if( dump )
    {
        printf( "%s, msg=[", rx?"RX>":"TX>" );
        len = msg_len(msg);
        for( i=0; i<len; i++ )
        {
            if( rx )    // ether addresses already gone
            {
                if( i==2 || i==22 || i==42 )
                    printf("-");
            }
            else
            {
                if( i==6 || i==12 || i==14 || i==34 || i==54 )
                    printf("-");
            }
            printf( "%02x", 0xff & *ptr++ );
        }
        printf( "]\n" );
    }
}

/*************************************************************************
 * Verbose ethernet startup
 *************************************************************************/
static void say( const char *s )
{
    #ifdef DEBUG_ETHER_INIT
    printf( s );
    #endif
}

/*************************************************************************
 * Write to PHY register
 *************************************************************************/
static void phy_write( u16 reg, u16 data )
{
    EMAC_CTLD_L = (data&0xff);
    EMAC_CTLD_H = ((data>>8)&0xff);
    EMAC_RGAD   = reg;
    EMAC_MIIMGT |= LCTLD;

    // Wait for mii done
    while( EMAC_MIISTAT & MGTBUSY )
        ;
}

/*************************************************************************
 * Read from PHY register
 *************************************************************************/
static u16 phy_read( u16 reg )
{
    u16 data;
    EMAC_RGAD = reg;
    EMAC_MIIMGT |= RSTAT;

    // Wait for mii done
    while( EMAC_MIISTAT & MGTBUSY )
        ;
    data = (EMAC_PRSD_L + ((u16)EMAC_PRSD_H<<8));
    return( data );
}


/*************************************************************************
 * Link initialization
 *************************************************************************/
static bool link_init()
{
    bool err=false;
    u16  phy_data;
    byte i, j;

    // Set mmiimgt clock rate
    EMAC_MIIMGT = CLKDIV20;

    // Set PHY address
    EMAC_FIAD = 0x1f;

    // Power down the PHY then power it back up to force a reconnect
    if( !err )
    {
        say( "Power down PHY\n" );
        phy_write( PHY_CREG, PHY_POWER_DOWN );
        tick_delay( MS_TO_TICKS(200) );

        // Try auto neg first, then 1000Mbps, then 10Mbps
        for( i=0, err=true; i<3 && err; i++ )
        {
            switch( i )
            {
                case 0:
                {
                    phy_data = phy_read( PHY_SREG );
                    if( phy_data & PHY_AUTO_NEG_CAPABLE )
                    {
                        say( "Trying auto\n" );
                        phy_write( PHY_CREG,
                             PHY_RESTART_AUTO_NEG | PHY_AUTO_NEG_ENABLE);
                    }
                    else
                        continue;
                    break;
                }
                case 1:
                {
                    say( "Trying 100 Mbps\n" );
                    phy_write( PHY_CREG, PHY_100BT | PHY_FULLD );
                    break;
                }
                case 2:
                {
                    say( "Trying 10 Mbps\n" );
                    phy_write( PHY_CREG, PHY_10BT | PHY_FULLD );
                    break;
                }
            }

            // Wait for successful outcome
            for( j=0; j<40 && err; j++ )
            {
                phy_data = phy_read( PHY_SREG );
                if( phy_data & PHY_AUTO_NEG_COMPLETE )
                {
                    say( "Auto negotiation successful\n" );
                    err = false;
                }
                else if( phy_data & PHY_LINK_ESTABLISHED )
                {
                    say( "Link established\n" );
                    err = false;
                }
                else
                {
                    say(".");
                    tick_delay( MS_TO_TICKS(100) );
                }
            }
        }
    }
    if( err )
        say( "Link not established\n" );
    else
    {
        say( "Start post establishment delay\n" );
        tick_delay( MS_TO_TICKS(800) );
        say( "End post establishment delay\n" );
    }
    return( err );
}
