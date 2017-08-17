/*************************************************************************
 * pool.h
 *
 *  Manage BMZ's message pools
 *  Project: eZ2944
 *************************************************************************/
#ifndef POOL_H
#define POOL_H
#include "types.h"
#include "msg.h"

// Define POOL type
typedef struct
{
    MSG  *array;    // ptr to array of MSGs
    byte nbr;       // nbr of MSGs
} POOL;

// Initialize the pool with nbr MSGs of given size and initial offset
void pool_init( POOL *pool, byte **addr_mem, u16 *addr_len,
                                     byte nbr, u16 size, byte offset );

// Get the i'th message from a pool
MSG *pool_idx( POOL *pool, byte idx );

// Get a free message from a pool
MSG *pool_alloc( POOL *pool );

#endif //POOL_H
