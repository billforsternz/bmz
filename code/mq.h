/*************************************************************************
 * mq.h
 *
 *  BMZ's MQ (Message Queue) routines
 *  Project: eZ2944
 *************************************************************************/
#ifndef MQ_H
#define MQ_H
#include "msg.h"
#include "types.h"

// Define MQ type
typedef struct
{
    MSG  **ring;        // ptr to array of MSG ptrs
    byte nbr;           // nbr of slots in ring array
    byte get;           // get from here in ring
    byte put;           // put to here in ring
    bool pushback;      // indicates a MSG has been pushed back
} MQ;

// Init an MQ of given depth
void mq_init( MQ *mq, byte **addr_mem, u16 *addr_len, byte depth );

// Write MSG to MQ
bool mq_write( MQ *mq, MSG *msg );   // returns true if successful

// Read MSG from MQ
MSG *mq_read( MQ *mq );

// Push MSG back onto MQ
bool mq_pushback( MQ *mq, MSG *msg );   // returns true if successful

// Check if a MSG has been pushed back onto MQ (and clear pushback flag)
bool mq_pushback_check_and_clear( MQ *mq );

// Clear MQ
void mq_clear( MQ *mq );

#endif  // MQ_H
