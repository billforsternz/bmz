/*************************************************************************
 * tick.h
 *
 *  Generate system hearbeat tick and high res tick
 *  Project: eZ2944
 *************************************************************************/
#ifndef TICK_H
#define TICK_H

// Tick rate
#define TICKS_PER_SECOND 20
#define MS_TO_TICKS(ms)  (1 + ((ms)*TICKS_PER_SECOND)/1000)
                                     //+1 rounds up
#define SECS_TO_TICKS(s) (1 + ((s)*TICKS_PER_SECOND))
                                     //+1 rounds up

// Hi res tick rate
#define TICKS_PER_SECOND_HI_RES 195320UL
    //   TICKS_PER_SECOND_HI_RES = TICKS_PER_SECOND * RELOAD
    //                           = 195320
    //                             (equivalent to 5.12uS per tick)
#define US_TO_TICKS_HI_RES(us) \
             (1 + \
                  ((us)*(TICKS_PER_SECOND_HI_RES/760)) / (1000000UL/760) \
             )
             // us = microseconds
             // +1 rounds up, 760 is a factor of TICKS_PER_SECOND_HI_RES
             // max 16711935 microseconds before arithmetic saturates

// Init
void tick_init();    // called by bmz_init(), not for users

// Get system heartbeat, incrementing tick count, rate TICKS_PER_SECOND
u32 tick_get();

// Get high res tick, incrementing tick count, rate TICKS_PER_SECOND_HI_RES
u32 tick_get_hi_res();

// Simple timed delay
void tick_delay( u32 ticks );

// Simple timed delay, at hi res rate
void tick_delay_hi_res( u32 ticks_hi_res );

#endif // TICK_H
