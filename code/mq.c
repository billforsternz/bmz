/*************************************************************************
 * mq.c
 *
 *  BMZ's MQ (Message Queue) routines
 *  Project: eZ2944
 *************************************************************************/
#include <stdio.h>
#include <string.h>
#include "bmz.h"

/*************************************************************************
 * Init an MQ of given depth
 *************************************************************************/
void mq_init( MQ *mq, byte **addr_mem, u16 *addr_len, byte depth )
{
    bool    err = false;
    byte    *memory = *addr_mem;
    u16     memlen  = *addr_len;

    // Allocate array of depth MSG ptrs for ring buffer
    if( memlen < depth*sizeof(MSG *) )
        err = true;
    else
    {
        mq->ring  = (MSG **)memory;
        memory    += depth*sizeof(MSG *);
        memlen    -= depth*sizeof(MSG *);

        // Initialise ring buffer variables
        mq->nbr   = depth;
        mq->put   = 0;
        mq->get   = 0;
        mq->pushback = false;
    }

    // Report on results
    *addr_mem = memory;
    *addr_len = memlen;
    if( err )
        bmz_panic_memory( "MQ" );
}

/*************************************************************************
 * Write MSG to MQ
 *************************************************************************/
bool mq_write( MQ *mq, MSG *msg )   // returns true if successful
{
    bool okay=true;
    byte next;
    next = mq->put+1;
    if( next == mq->nbr )
        next = 0;
    if( next == mq->get )
        okay=false; // ring buffer is full
    else
    {
        mq->ring[mq->put] = msg;
        mq->put = next;
    }
    return( okay );
}

/*************************************************************************
 * Read MSG from MQ
 *************************************************************************/
MSG *mq_read( MQ *mq )
{
    MSG *msg=NULL;
    byte next;
    if( mq->get != mq->put )
    {
        next = mq->get+1;
        if( next == mq->nbr )
            next = 0;
        msg = mq->ring[mq->get];
        mq->get = next;
    }
    return( msg );
}

/*************************************************************************
 * Push MSG back onto MQ
 *************************************************************************/
bool mq_pushback( MQ *mq, MSG *msg )  // returns true if successful
{
    bool okay=false;
    byte where;
    if( mq->get == 0 )
        where = mq->nbr-1;
    else
        where = mq->get-1;
    if( where != mq->put )  // make sure not already full
    {
        mq->ring[where] = msg;
        mq->get = where;
        mq->pushback = true;
        okay = true;
    }
    return( okay );
}

/*************************************************************************
 * Check if a MSG has been pushed back onto MQ (and clear pushback flag)
 *************************************************************************/
bool mq_pushback_check_and_clear( MQ *mq )
{
    bool pushback = mq->pushback;
    mq->pushback  = false;
    return( pushback );
}

/*************************************************************************
 * Clear MQ
 *************************************************************************/
void mq_clear( MQ *mq )
{
    MSG *msg = mq_read(mq);
    while( msg )
    {
        msg_free(msg);
        msg = mq_read(mq);
    }
    mq->pushback = false;
}
