/*************************************************************************
 * bmz.c
 *
 *  BMZ = Bare Metal Zilog. Core routines
 *  Project: eZ2944
 *************************************************************************/
#include <stdio.h>
#include <string.h>
#include "bmz.h"
#include "console.h"

// Size of task table
#define MAX_TASKS 10

// Entry in the task table
typedef struct
{
    HANDLER_IDLE    idle;
    HANDLER_TIMEOUT timeout;
    HANDLER_MSG     down;
    HANDLER_MSG     up;
    MQ             *mq_down;
    MQ             *mq_up;
    POOL           *pool;
    void           *instance;
    PUBLISH_STATE state;
} TASK;

// Module memory
typedef struct
{
    TASKID  max_taskid;
    TASKID  current_taskid;
    TASK    task_table[MAX_TASKS];
    TIMER *list;
} BMZ;
static BMZ z;

// In library module vectors24.asm
extern void _init_default_vectors();

/*************************************************************************
 * Initialize BMZ, call this at start of main()
 *************************************************************************/
void bmz_init()
{
    _init_default_vectors();    // setup interrupt vector system
    tick_init();                // start tick interrupts
}

/*************************************************************************
 * Panic because something has gone very wrong
 *************************************************************************/
void bmz_panic( const char *txt )
{
    if( txt )
        putstr( txt );
    putstr( " PANIC\n" );
    for(;;)
        ;
}

/*************************************************************************
 * Panic because we have run out of memory
 *************************************************************************/
void bmz_panic_memory( const char *txt )
{
    if( txt )
        putstr( txt );
    bmz_panic( " - out of memory" );
}

/*************************************************************************
 * Read task descriptor array to build task table
 *************************************************************************/
void bmz_define_system( const TASK_DESCRIPTOR *td, int td_nbr,
                                         byte **addr_mem, u16 *addr_len )
{
    bool err=false;
    byte *memory;
    u16   memlen;
    TASK *p;
    TASKID taskid;
    byte i;
    const TASK_DESCRIPTOR *td_base=td;

    // Loop through descriptors
    z.max_taskid = 0;
    for( i=0; i<td_nbr; i++, td++ )
    {
        z.current_taskid = taskid = td->taskid;
        if( taskid >= nbrof(z.task_table) )
            bmz_panic( "Task table too short" );
        p = &z.task_table[taskid];
        if( taskid > z.max_taskid )
            z.max_taskid = taskid;

        // Set handlers
        p->idle    = td->idle;
        p->timeout = td->timeout;
        p->down    = td->down;
        p->up      = td->up;

        // Create down queue
        if( td->mq_down_depth == 0 )
            p->mq_down = NULL;
        else
        {
            memory = *addr_mem;
            memlen = *addr_len;
            if( memlen < sizeof(MQ) )
                err = true;
            else
            {
                p->mq_down = (MQ *)memory;
                memory += sizeof(MQ);
                memlen -= sizeof(MQ);
                *addr_mem = memory;
                *addr_len = memlen;
                mq_init( p->mq_down, addr_mem, addr_len, td->mq_down_depth );
            }
        }

        // Create up queue
        if( td->mq_up_depth == 0 )
            p->mq_up = NULL;
        else
        {
            memory = *addr_mem;
            memlen = *addr_len;
            if( memlen < sizeof(MQ) )
                err = true;
            else
            {
                p->mq_up = (MQ *)memory;
                memory += sizeof(MQ);
                memlen -= sizeof(MQ);
                *addr_mem = memory;
                *addr_len = memlen;
                mq_init( p->mq_up, addr_mem, addr_len, td->mq_up_depth );
            }
        }

        // Create pool
        if( td->pool_nbr == 0 )
            p->pool = NULL;
        else
        {
            memory = *addr_mem;
            memlen = *addr_len;
            if( memlen < sizeof(POOL) )
                err = true;
            else
            {
                p->pool = (POOL *)memory;
                memory += sizeof(POOL);
                memlen -= sizeof(POOL);
                *addr_mem = memory;
                *addr_len = memlen;
                pool_init( p->pool, addr_mem, addr_len,
                             td->pool_nbr, td->pool_len, td->pool_offset );
            }
        }

        // Panic ?
        if( err )
            bmz_panic_memory( "bmz_define_system()" );

        // Run init function
        if( td->init )
            p->instance = (*td->init)( addr_mem, addr_len );
        else
            p->instance = NULL;
    }

    // Loop through a second time, for shared pools only
    td = td_base;
    for( i=0; i<td_nbr; i++, td++ )
    {
        p = &z.task_table[td->taskid];

        // Share another task's pool if specified
        if( td->pool_share != TASKID_NULL )
            p->pool = z.task_table[ td->pool_share ].pool;
    }

    // Eliminate tasks without msg or idle handlers from run loop
    while( z.max_taskid >= 1 )
    {
        p = &z.task_table[ z.max_taskid ];
        if( p->mq_down==NULL && p->mq_up==NULL && p->idle==NULL )
            z.max_taskid--;
        else
            break;
    }
}

/*************************************************************************
 * Run the tasks
 *************************************************************************/
void bmz_run()
{
    TASK *p;
    u32 previous, now, elapsed;
    MQ *mq;
    MSG *msg;
    byte i;

    // Loop forever
    previous = tick_get();  // previous time for comparison purposes
    for(;;)
    {

        // Loop through the task table, skipping 0 == TASKID_NULL
        for( i=1, p=&z.task_table[1]; i<=z.max_taskid; i++, p++ )
        {
            z.current_taskid = i;

            // Feed messages from down queue to down handler
            mq = p->mq_down;
            if( mq )
            {
                msg = mq_read(mq);
                if( msg )
                {
                    (*p->down)( msg );
                    if( mq_pushback_check_and_clear(mq) )
                        break;  // msg was pushed back
                }
            }

            // Feed messages from up queue to up handler
            mq = p->mq_up;
            if( mq )
            {
                msg = mq_read(mq);
                if( msg )
                {
                    (*p->up)( msg );
                    if( mq_pushback_check_and_clear(mq) )
                        break;  // msg was pushed back
                }
            }

            // Run idle routine
            if( p->idle )
                (*p->idle)();
        }

        // If the heartbeat has ticked, run timers
        now = tick_get();
        if( now != previous )
        {
            elapsed = now-previous;
            timer_run( elapsed );
            previous = now;
        }
    }
}

/*************************************************************************
 * Send message to a task's down handler
 *************************************************************************/
void bmz_down( TASKID taskid, MSG *msg )
{
    TASK *p = &z.task_table[taskid];
    if( p->mq_down && !(msg->inuse&MSG_INUSE_BULLET) ) //don't queue bullets
        mq_write( p->mq_down, msg );
    else
    {
        TASKID save=z.current_taskid;
        z.current_taskid = taskid;
        (*p->down)( msg );
        z.current_taskid = save;
    }
}

/*************************************************************************
 * Send message to a task's up handler
 *************************************************************************/
void bmz_up( TASKID taskid, MSG *msg )
{
    TASK *p = &z.task_table[taskid];
    if( p->mq_up && !(msg->inuse&MSG_INUSE_BULLET) ) //don't queue bullets
        mq_write( p->mq_up, msg );
    else
    {
        TASKID save=z.current_taskid;
        z.current_taskid = taskid;
        (*p->up)( msg );
        z.current_taskid = save;
    }
}

/*************************************************************************
 * Call a task's timeout handler
 *************************************************************************/
void bmz_timeout( TASKID taskid, byte timer_id )
{
    TASKID save=z.current_taskid;
    z.current_taskid = taskid;
    (*z.task_table[taskid].timeout)( timer_id );
    z.current_taskid = save;
}

/*************************************************************************
 * Get the currently running task's taskid
 *************************************************************************/
TASKID bmz_get_current_taskid()
{
    return( z.current_taskid );
}

/*************************************************************************
 * Get the currently running task's instance ptr
 *************************************************************************/
void *bmz_get_current_instance()
{
    return( z.task_table[z.current_taskid].instance );
}

/*************************************************************************
 * Get the currently running task's message pool
 *************************************************************************/
POOL *bmz_get_current_pool()
{
    return( z.task_table[z.current_taskid].pool );
}

/*************************************************************************
 * Get a task's instance ptr
 *************************************************************************/
void *bmz_get_instance( TASKID taskid )
{
    return( z.task_table[taskid].instance );
}

/*************************************************************************
 * Get a task's up MQ
 *************************************************************************/
MQ *bmz_get_mq_up( TASKID taskid )
{
    return( z.task_table[taskid].mq_up );
}

/*************************************************************************
 * Get a task's down MQ
 *************************************************************************/
MQ *bmz_get_mq_down( TASKID taskid )
{
    return( z.task_table[taskid].mq_down );
}

/*************************************************************************
 * Get a task's POOL
 *************************************************************************/
POOL *bmz_get_pool( TASKID taskid )
{
    return( z.task_table[taskid].pool );
}

/*************************************************************************
 * Get a task's published state
 *************************************************************************/
PUBLISH_STATE bmz_get_publish_state( TASKID taskid )
{
    return( z.task_table[taskid].state );
}

/*************************************************************************
 * Set the currently running task's published state
 *************************************************************************/
void bmz_set_publish_state( PUBLISH_STATE state )
{
    z.task_table[z.current_taskid].state = state;
}
