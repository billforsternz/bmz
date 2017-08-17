/*************************************************************************
 * pool.c
 *
 *  Manage BMZ's message pools
 *  Project: eZ2944
 *************************************************************************/
#include <stdio.h>
#include <string.h>
#include "bmz.h"

/*************************************************************************
 * Initialize the pool with nbr MSGs of given size and initial offset
 *************************************************************************/
void pool_init( POOL *pool, byte **addr_mem, u16 *addr_len,
                                     byte nbr, u16 size, byte offset )
{
    bool    err = false;
    MSG     *msg;
    byte    i;
    byte    *memory = *addr_mem;
    u16     memlen  = *addr_len;
    printf( "pool_init in - nbr=%u, size=%u, memlen=%u\n",
                                                 (u16)nbr, size, memlen );

    // Allocate array of nbr MSG objects
    if( memlen < nbr*sizeof(MSG) )
        err = true;
    else
    {
        pool->array = (MSG *)memory;
        pool->nbr   = nbr;
        memory      += nbr*sizeof(MSG);
        memlen      -= nbr*sizeof(MSG);
    }

    // Initialise nbr MSG objects
    for( i=0; !err && i<nbr; i++ )
    {
        if( memlen < size )
            err = true;
        else
        {
            msg             = pool->array + i;
            msg->base       = memory;
            msg->ptr        = msg->base + offset;
            msg->offset     = offset;
            msg->size       = size;
            msg->len        = 0;
            msg->inuse      = 0;
            memory          += size;
            memlen          -= size;
        }
    }

    // Report on results
    *addr_mem = memory;
    *addr_len = memlen;
    printf( "pool_init out - err=%u, memlen=%u\n", (u16)err, memlen );
    if( err )
        bmz_panic_memory( "POOL" );
}

/*************************************************************************
 * Get the i'th message from a pool
 *************************************************************************/
MSG *pool_idx( POOL *pool, byte idx )
{
    return( pool->array + idx );
}

/*************************************************************************
 * Get a free message from a pool
 *************************************************************************/
MSG *pool_alloc( POOL *pool )
{
    byte i;
    MSG *found=NULL, *msg;
    for( i=0, msg=pool->array; i<pool->nbr; i++, msg++ )
    {
        if( !msg->inuse )
        {
            msg->inuse = MSG_INUSE_NORMAL;
            found = msg;
            msg_clear( found );
            break;
        }
    }
    return( found );
}
