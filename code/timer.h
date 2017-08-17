/*************************************************************************
 * timer.h
 *
 *  Implement BMZ's timers
 *  Project: eZ2944
 *************************************************************************/
#ifndef TIMER_H
#define TIMER_H
#include "bmz.h"    // for TASKID

// Define TIMER type
typedef struct tag_TIMER
{
    struct tag_TIMER *link;         // chain running timers
    TASKID            taskid;       // task to notify on expiry
    byte              id;           // allow task's to own more than one
                                    //  timer and distinguish between them
    u32               remaining;    // remaining ticks until expiry
    bool              running;      // timer is running
    bool              expired;      // timer has expired
} TIMER;

// Initialize and reset timer, assign it an ID
void timer_reset( TIMER *timer, byte id );

// Start timer, expires in N seconds
void timer_start_seconds( TIMER *timer, u16 seconds );

// Start timer, expires in N ticks
void timer_start_ticks( TIMER *timer, u32 ticks );

// Stop and unlink timer
void timer_stop( TIMER *timer );

// Read time remaining in ticks
u32 timer_read ( TIMER *timer );

// Test whether timer has expired
bool timer_expired( TIMER *timer );

// Test whether timer is running
bool timer_running( TIMER *timer );

// Run timer system, call event handlers on expiring timers
void timer_run( u32 nticks );   // called by BMZ, not for users

#endif // TIMER_H
