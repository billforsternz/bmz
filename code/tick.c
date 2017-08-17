/*************************************************************************
 * tick.c
 *
 *  Generate system hearbeat tick and high res tick
 *  Project: eZ2944
 *************************************************************************/
#include <ez80.h>
#include "bmz.h"
#include "tick.h"

// Module data
typedef struct
{
    u32 tick_count;
    u32 hi_res;
} TICK;
static TICK z;

// Extra register definitions etc.
#define TIMER1_IVECT 0x58

// To set an interval of 50mS, calculate N such that T*256*N = 50ms
//  (T=1/f, f=50MHz), so N = 50,000,000 / (256*20) = 9765.625
//  So set 9766 = 0x2626, actual interval is 50.00192mS (out by 38ppm
//  which is about all that can be expected with crystal accuracy anyway).
#define RELOAD    0x2626
#define RELOAD_HI 0x26
#define RELOAD_LO 0x26

/*************************************************************************
 * Get system heartbeat, incrementing tick count, rate TICKS_PER_SECOND
 *************************************************************************/
u32 tick_get()
{

    // Use a loop to get 2 valid values in a row, that way we
    //  know an interrupt hasn't corrupted the value as we've
    //  read it
    u32 temp;
    do
    {
        temp = z.tick_count;
    } while( z.tick_count != temp );
    return( temp );
}


/*************************************************************************
 * Get high res tick, incrementing tick count, rate TICKS_PER_SECOND_HI_RES
 *************************************************************************/
//   TICKS_PER_SECOND_HI_RES = TICKS_PER_SECOND * RELOAD
//                           = 195320
//                             (equivalent to 5.12uS per tick)
u32 tick_get_hi_res()
{

    // Use a loop to get 2 valid values in a row, that way we
    //  know an interrupt hasn't corrupted the value as we've
    //  read it
    u32  temp;
    byte hi, lo;
    u16  current, elapsed;
    do
    {
        temp = z.hi_res;
        lo   = TMR1_DR_L;   // must read hw in order LO then HI
        hi   = TMR1_DR_H;
    } while( z.hi_res != temp );
    current = (u16)hi;
    current = (current<<8) + (u16)lo;
    elapsed = (RELOAD-current);
    return( temp + elapsed );
}


/*************************************************************************
 * Interrupt
 *************************************************************************/
void interrupt timer1_isr( void )
{
    byte temp;
    temp = TMR1_CTL;        // read to clear pending int
    temp = TMR1_IIR;
    z.tick_count++;
    z.hi_res += RELOAD;
}


/*************************************************************************
 * Init
 *************************************************************************/
void tick_init(void)
{
    z.tick_count = 0;
    z.hi_res     = 0;
    set_vector( TIMER1_IVECT, timer1_isr );
    TMR1_CTL  =  0x00;      // clear and disable timer
    TMR1_RR_L =  RELOAD_LO; // See reload calculation above
    TMR1_RR_H =  RELOAD_HI; //
    TMR1_CTL  =  0x1e;      // continuous, source is sysclk/256, load timer
    TMR1_CTL  =  0x1f;      // enable timer
    TMR1_IER  =  0x01;      // interrupt on end count
}


/*************************************************************************
 * Simple timed delay
 *************************************************************************/
void tick_delay( u32 ticks )
{
    u32 now, base = tick_get();
    for(;;)
    {
        now = tick_get();
        if( now-base >= ticks )
            break;
    }
}
