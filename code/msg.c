/*************************************************************************
 * msg.c
 *
 *  BMZ's message manipulation routines
 *  Project: eZ2944
 *************************************************************************/
#include <stdio.h>
#include <string.h>
#include "bmz.h"

// Local prototypes
static void bmz_panic_msg();

/*************************************************************************
 * Clear message, set ptr to original offset
 *************************************************************************/
void msg_clear( MSG *msg )
{
    msg->ptr = msg->base + msg->offset;
    msg->len = 0;
}

/*************************************************************************
 * Free message
 *************************************************************************/
void msg_free( MSG *msg )
{
    if( msg->inuse & MSG_INUSE_USER )
        user_msg_free(msg);
    else
        msg->inuse = 0;
}

/*************************************************************************
 * Calculate remaining room to write
 *************************************************************************/
u16 msg_room( const MSG *msg )
{
    return(  (msg->base+msg->size) - (msg->ptr+msg->len)  );
}

/*************************************************************************
 * Push 1 byte onto front of message
 *************************************************************************/
void msg_push1( MSG *msg, byte dat )
{
    if( msg->ptr > msg->base )
    {
        *--msg->ptr = dat;
        msg->len++;
    }
    else
        bmz_panic_msg();
}

/*************************************************************************
 * Push 2 bytes onto front of message
 *************************************************************************/
void msg_push2( MSG *msg, u16 dat )
{
    byte *ptr=msg->ptr;
    if( ptr-msg->base >= 2 )
    {
        *--ptr = (byte)dat;
        *--ptr = (byte)(dat >> 8);
        msg->ptr = ptr;
        msg->len += 2;
    }
    else
        bmz_panic_msg();
}

/*************************************************************************
 * Push 4 bytes onto front of message
 *************************************************************************/
void msg_push4( MSG *msg, u32 dat )
{
    byte *ptr=msg->ptr;
    if( ptr-msg->base >= 4 )
    {
        *--ptr = (byte)dat;
        dat >>= 8;
        *--ptr = (byte)dat;
        dat >>= 8;
        *--ptr = (byte)dat;
        dat >>= 8;
        *--ptr = (byte)dat;
        msg->ptr = ptr;
        msg->len += 4;
    }
    else
        bmz_panic_msg();
}

/*************************************************************************
 * Push 6 bytes onto front of message
 *************************************************************************/
void msg_push6( MSG *msg, const byte *dat )
{
    byte *ptr=msg->ptr;
    if( ptr-msg->base >= 6 )
    {
        ptr -= 6;
        memcpy( ptr, dat, 6 );
        msg->ptr = ptr;
        msg->len += 6;
    }
    else
        bmz_panic_msg();
}


/*************************************************************************
 * Write 1 byte to end of message
 *************************************************************************/
void msg_write1( MSG *msg, byte dat )
{
    byte *ptr =  msg->ptr + msg->len;
    if( ptr < msg->base+msg->size )
    {
        *ptr = dat;
        msg->len++;
    }
    else
        bmz_panic_msg();
}

/*************************************************************************
 * Write 2 bytes to end of message
 *************************************************************************/
void msg_write2( MSG *msg, u16 dat )
{
    byte *ptr =  msg->ptr + msg->len;
    if( ptr+2 <= msg->base+msg->size )
    {
        *ptr++  = (byte)(dat>>8);
        *ptr    = (byte)dat;
        msg->len += 2;
    }
    else
        bmz_panic_msg();
}

/*************************************************************************
 * Write 4 bytes to end of message
 *************************************************************************/
void msg_write4( MSG *msg, u32 dat )
{
    byte *ptr =  msg->ptr + msg->len;
    if( ptr+4 <= msg->base+msg->size )
    {
        *ptr++  = (byte)(dat>>24);
        *ptr++  = (byte)(dat>>16);
        *ptr++  = (byte)(dat>>8);
        *ptr    = (byte)dat;
        msg->len += 4;
    }
    else
        bmz_panic_msg();
}

/*************************************************************************
 * Write 6 bytes to end of message
 *************************************************************************/
void msg_write6( MSG *msg, const byte *dat )
{
    byte *ptr =  msg->ptr + msg->len;
    if( ptr+6 <= msg->base+msg->size )
    {
        memcpy( ptr, dat, 6 );
        msg->len += 6;
    }
    else
        bmz_panic_msg();
}

/*************************************************************************
 * Poke 1 byte into arbitrary point in message
 *************************************************************************/
void msg_poke1( MSG *msg, byte dat, u16 offset )
{
    byte *ptr =  msg->ptr + offset;
    if( offset < msg->len  )
        *ptr = dat;
    else
        bmz_panic_msg();
}

/*************************************************************************
 * Poke 2 bytes into arbitrary point in message
 *************************************************************************/
void msg_poke2( MSG *msg, u16 dat, u16 offset )
{
    byte *ptr =  msg->ptr + offset;
    if( offset+2 <= msg->len )
    {
        *ptr++  = (byte)(dat>>8);
        *ptr    = (byte)dat;
    }
    else
        bmz_panic_msg();
}

/*************************************************************************
 * Poke 4 bytes into arbitrary point in message
 *************************************************************************/
void msg_poke4( MSG *msg, u32 dat, u16 offset )
{
    byte *ptr =  msg->ptr + offset;
    if( offset+4 <= msg->len )
    {
        *ptr++  = (byte)(dat>>24);
        *ptr++  = (byte)(dat>>16);
        *ptr++  = (byte)(dat>>8);
        *ptr    = (byte)dat;
    }
    else
        bmz_panic_msg();
}

/*************************************************************************
 * Poke 6 bytes into arbitrary point in message
 *************************************************************************/
void msg_poke6( MSG *msg, const byte *dat, u16 offset )
{
    byte *ptr =  msg->ptr + offset;
    if( offset+6 <= msg->len )
        memcpy( ptr, dat, 6 );
    else
        bmz_panic_msg();
}


/*************************************************************************
 * Pop 1 byte off front of message
 *************************************************************************/
byte msg_pop1( MSG *msg )
{
    byte dat;
    if( msg->len >= 1 )
    {
        dat = *msg->ptr++;
        msg->len--;
    }
    else
        bmz_panic_msg();
    return( dat );
}

/*************************************************************************
 * Pop 2 bytes off front of message
 *************************************************************************/
u16 msg_pop2( MSG *msg )
{
    u16 dat;
    if( msg->len >= 2 )
    {
        dat = (u16)(*msg->ptr++);
        dat = (dat<<8) + (u16)(*msg->ptr++);
        msg->len -= 2;
    }
    else
        bmz_panic_msg();
    return( dat );
}

/*************************************************************************
 * Pop 4 bytes off front of message
 *************************************************************************/
u32 msg_pop4( MSG *msg )
{
    u32 dat;
    byte *ptr=msg->ptr;
    if( msg->len >= 4 )
    {
        dat = (u32)(*ptr++);
        dat = (dat<<8) + (u32)(*ptr++);
        dat = (dat<<8) + (u32)(*ptr++);
        dat = (dat<<8) + (u32)(*ptr++);
        msg->len -= 4;
        msg->ptr = ptr;
    }
    else
        bmz_panic_msg();
    return( dat );
}

/*************************************************************************
 * Pop 6 bytes off front of message
 *************************************************************************/
byte *msg_pop6( MSG *msg )
{
    byte *dat;
    if( msg->len >= 6 )
    {
        dat = msg->ptr;
        msg->len -= 6;
        msg->ptr += 6;
    }
    else
        bmz_panic_msg();
    return( dat );
}

/*************************************************************************
 * Pop an arbitrary number of bytes off front of message
 *************************************************************************/
void msg_pop( MSG *msg, byte how_many )
{
    if( msg->len >= how_many )
    {
        msg->len -= how_many;
        msg->ptr += how_many;
    }
    else
        bmz_panic_msg();
}

/*************************************************************************
 * Read 1 byte from arbitrary position in message
 *************************************************************************/
byte msg_read1( const MSG *msg, u16 offset )
{
    byte dat;
    if( offset < msg->len )
        dat = *(msg->ptr + offset);
    else
        bmz_panic_msg();
    return( dat );
}

/*************************************************************************
 * Read 2 bytes from arbitrary position in message
 *************************************************************************/
u16 msg_read2( const MSG *msg, u16 offset )
{
    u16 dat;
    byte *ptr = msg->ptr+offset;
    if( offset+2 <= msg->len )
    {
        dat = (u16)(*ptr++);
        dat = (dat<<8) + (u16)(*ptr);
    }
    else
        bmz_panic_msg();
    return( dat );
}

/*************************************************************************
 * Read 4 bytes from arbitrary position in message
 *************************************************************************/
u32 msg_read4( const MSG *msg, u16 offset )
{
    u32 dat;
    byte *ptr = msg->ptr+offset;
    if( offset+4 <= msg->len )
    {
        dat = (u32)(*ptr++);
        dat = (dat<<8) + (u32)(*ptr++);
        dat = (dat<<8) + (u32)(*ptr++);
        dat = (dat<<8) + (u32)(*ptr);
    }
    else
        bmz_panic_msg();
    return( dat );
}

/*************************************************************************
 * Read 6 bytes from arbitrary position in message
 *************************************************************************/
byte *msg_read6( const MSG *msg, u16 offset )
{
    byte *ptr = msg->ptr+offset;
    if( offset+6 >= msg->len )
        bmz_panic_msg();
    return( ptr );
}

/*************************************************************************
 * Get a ptr to an arbitrary position in message
 *************************************************************************/
byte *msg_readp( const MSG *msg, u16 offset )
{
    byte *ptr = msg->ptr+offset;
    if( offset > msg->len )
        bmz_panic_msg();
    return( ptr );
}

/*************************************************************************
 * Panic because something has gone very wrong with messages
 *************************************************************************/
static void bmz_panic_msg()
{
    bmz_panic( "Message error" );
}

