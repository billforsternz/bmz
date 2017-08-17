/*************************************************************************
 * msg.h
 *
 *  BMZ's message manipulation routines
 *  Project: eZ2944
 *************************************************************************/
#ifndef MSG_H
#define MSG_H
#include "types.h"

// Define MSG type
typedef struct
{
    byte inuse;     // 0 if free, else combination of flags below
    byte offset;    // initial position of ptr relative to base
    byte *base;     // points to storage used to hold message
    byte *ptr;      // points to message within that storage
    u16  size;      // size of storage
    u16  len;       // length of message
} MSG;

// Flags for use in the inuse field
#define MSG_INUSE_NORMAL 1  // Freed by system
#define MSG_INUSE_USER   2  // Freed by custom user routine
#define MSG_INUSE_BULLET 4  // Do not queue, apply directly to handler

// Get length of message data
#define msg_len(msg)  ((msg)->len)

// Get pointer to message data
#define msg_ptr(msg)  ((msg)->ptr)

// User's msg free hook, called by msg_free() if MSG_INUSE_USER set
void user_msg_free( MSG *msg );

// Clear message, set ptr to original offset
void msg_clear( MSG *msg );

// Free message
void msg_free( MSG *msg );

// Calculate remaining room to write
u16 msg_room( const MSG *msg );

// Push 1 byte onto front of message
void msg_push1( MSG *msg, byte dat );

// Push 2 bytes onto front of message
void msg_push2( MSG *msg, u16 dat );

// Push 4 bytes onto front of message
void msg_push4( MSG *msg, u32 dat );

// Push 6 bytes onto front of message
void msg_push6( MSG *msg, const byte *dat );

// Write 1 byte to end of message
void msg_write1( MSG *msg, byte dat );

// Write 2 bytes to end of message
void msg_write2( MSG *msg, u16 dat );

// Write 4 bytes to end of message
void msg_write4( MSG *msg, u32 dat );

// Write 6 bytes to end of message
void msg_write6( MSG *msg, const byte *dat );

// Poke 1 byte into arbitrary point in message
void msg_poke1( MSG *msg, byte dat, u16 offset );

// Poke 2 bytes into arbitrary point in message
void msg_poke2( MSG *msg, u16 dat, u16 offset );

// Poke 4 bytes into arbitrary point in message
void msg_poke4( MSG *msg, u32 dat, u16 offset );

// Poke 6 bytes into arbitrary point in message
void msg_poke6( MSG *msg, const byte *dat, u16 offset );

// Pop 1 byte off front of message
byte msg_pop1( MSG *msg );

// Pop 2 bytes off front of message
u16 msg_pop2( MSG *msg );

// Pop 4 bytes off front of message
u32 msg_pop4( MSG *msg );

// Pop 6 bytes off front of message
byte *msg_pop6( MSG *msg );

// Pop an arbitrary number of bytes off front of message
void msg_pop( MSG *msg, byte how_many );

// Read 1 byte from arbitrary position in message
byte msg_read1( const MSG *msg, u16 offset );

// Read 2 bytes from arbitrary position in message
u16 msg_read2( const MSG *msg, u16 offset );

// Read 4 bytes from arbitrary position in message
u32 msg_read4( const MSG *msg, u16 offset );

// Read 6 bytes from arbitrary position in message
byte *msg_read6( const MSG *msg, u16 offset );

// Get a ptr to an arbitrary position in message
byte *msg_readp( const MSG *msg, u16 offset );

#endif // MSG_H
