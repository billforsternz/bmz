/*************************************************************************
 * timer.c
 *
 *  Implement BMZ's timers
 *  Project: eZ2944
 *************************************************************************/
#include <stdio.h>
#include <string.h>
#include "bmz.h"
#include "console.h"
#include "tick.h"

// Module memory
static struct
{
    TIMER *list;
} z;

/*************************************************************************
 * Start timer, expires in N seconds
 *************************************************************************/
void timer_start_seconds( TIMER *timer, u16 seconds )
{
    u32 ticks = ((u32)seconds) * TICKS_PER_SECOND;
    timer_start_ticks( timer, ticks );
}

/*************************************************************************
 * Start timer, expires in N ticks
 *************************************************************************/
void timer_start_ticks( TIMER *timer, u32 ticks )
{
    //DBG bool after, before=check_my_timer();
    timer->remaining = ticks;
    timer->taskid    = bmz_get_current_taskid();
    //DBG if( timer->taskid == 5 /*TASKID_TCPSOCK1*/ &&
    //DBG    timer->id == 0
    //DBG  )
    //DBG    printf("TIMER, start, timer->running=%s\n",
    //DBG                          timer->running ? "true":"false" );
    if( !timer->running )
    {
        timer->running = true;
        timer->link = z.list;
        z.list      = timer;
    }
    //DBG after = check_my_timer();
    //DBG if( !before && after )
    //DBG     printf( "Created by timer_start()!\n" );
    //DBG if( before && !after )
    //DBG     printf( "Removed by timer_start()!\n" );
}

/*************************************************************************
 * Stop and unlink timer
 *************************************************************************/
void timer_stop( TIMER *timer )
{
    //DBG bool after, before=check_my_timer();
    TIMER *temp = z.list;
    TIMER **unlink = &z.list;
    //DBG if( timer->taskid == 5 /*TASKID_TCPSOCK1*/ &&
    //DBG     timer->id == 0
    //DBG   )
    //DBG     printf("TIMER, stop\n" );
    while( temp )
    {
        if( temp == timer )
        {
            timer->running = false;
            *unlink = timer->link;
            timer->link = NULL;
            break;
        }
        else
        {
            unlink = &temp->link;
            temp   = temp->link;
        }
    }
    //DBG after = check_my_timer();
    //DBG if( !before && after )
    //DBG     printf( "Created by timer_stop()!\n" );
    //DBG if( before && !after )
    //DBG     printf( "Removed by timer_stop()!\n" );
}

/*************************************************************************
 * Reset timer, assign it an ID
 *************************************************************************/
void timer_reset( TIMER *timer, byte id )
{
    timer_stop( timer );
    timer->expired   = false;
    timer->remaining = 0;
    timer->id        = id;
}


/*************************************************************************
 * Read time remaining in ticks
 *************************************************************************/
u32 timer_read ( TIMER *timer )
{
    return( timer->remaining );
}

/*************************************************************************
 * Test whether timer has expired
 *************************************************************************/
bool timer_expired( TIMER *timer )
{
    return( timer->expired );
}

/*************************************************************************
 * Test whether timer is running
 *************************************************************************/
bool timer_running( TIMER *timer )
{
    return( timer->running );
}

/*************************************************************************
 * Run timer system, call event handlers on expiring timers
 *************************************************************************/
void timer_run( u32 nticks )
{
    TIMER *next;
    TIMER *timer   = z.list;
    TIMER **unlink = &z.list;
    TIMER *expired_timers[10];
    byte i, idx=0;
    //DBG bool after, before=check_my_timer();
    while( timer )
    {
        next = timer->link;
        if( nticks < timer->remaining )
        {
            timer->remaining -= nticks;
            unlink = &timer->link;
        }
        else
        {
            //DBG if( timer->taskid == 5 /*TASKID_TCPSOCK1*/ &&
            //DBG     timer->id == 0
            //DBG   )
            //DBG     printf("TIMER, expired\n" );
            if( idx >= nbrof(expired_timers) )
            {
                timer->remaining = 1;   // postpone timeout
                unlink = &timer->link;
            }
            else
            {
                expired_timers[idx++] = timer;
                timer->link = NULL;
                *unlink = next;
                timer->remaining = 0;
                timer->running   = false;
                timer->expired   = true;
            }
        }
        timer = next;
    }
    for( i=0; i<idx; i++ )
    {
        timer = expired_timers[i];
        bmz_timeout( timer->taskid, timer->id );
    }
    //DBG after = check_my_timer();
    //DBG if( !before && after )
    //DBG     printf( "Created by timer_run()!\n" );
    //DBG if( before && !after )
    //DBG     printf( "Removed by timer_run()!\n" );
}

/*************************************************************************
 * Debug function - check whether a particular timer is running
 *************************************************************************/
#if 0
static bool check_my_timer()
{
    bool found=false;
    TIMER *timer = z.list;
    while( timer )
    {
        if( timer->taskid == 5 /*TASKID_TCPSOCK1*/ &&
            timer->id == 0
          )
            found = true;
        timer = timer->link;
    }
    return( found );
}
#endif
