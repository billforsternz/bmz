/*************************************************************************
 * checksum.c
 *
 *  IP checksum algorithm
 *  Project: eZ2944
 *************************************************************************/
#include <stdio.h>
#include "choices.h"
#include "checksum.h"

/*************************************************************************
 * Calculate checksum (for transmit)
 *************************************************************************/
// The checksum calculated is ready to be poked into
//  memory or compared with an exising value in memory.
//  In both cases, the memory operation must be a
//  *native* big endian or little endian read or write
u16 checksum_calculate( const byte *data, u16 len )
{
    byte end[2];
    u32  sum;
    const u16 *p;
    u16 nbr_pairs;
    bool odd = ( (len&1) ? true : false );

    // Get number of byte pairs, up to but not including possible odd byte
    //  at end (so round down)
    nbr_pairs = len>>1;

    // Checksumming loop - Magic trick optimisation below means we can
    //  access 16 bit data as *p, without worrying about big/little
    //  endian issues.
    sum = 0;
    p   = (const u16 *)data;
    while( nbr_pairs-- )
    {
        sum += (u32)(*p);
        p++;
    }

    // If odd, add odd byte at end plus padding zero
    if( odd )
    {
        end[0] = data[len-1];
        end[1] = 0;
        p   = (const u16 *)end;
        sum += (u32)(*p);
    }

    // A well known but mysterious (at least to me) magic trick to get the
    //  right answer (in the sense described in the intro) on big endian
    //  and little endian machines
    sum = (sum >> 16) + (sum & 0xffff); // don't know how it
    sum += (sum >> 16);                 //  works
    sum = ~sum;
    return( (u16)sum );
}


/*************************************************************************
 * Test checksum (for receive)
 *************************************************************************/
bool checksum_test(  byte *data, u16 len, u16 offset )
{
    bool err=true;
    u16 *p;
    u16 existing, calculated;

    // Get ptr to the checksum field
    p = (u16 *)&data[offset];

    // Get existing value in native order
    existing  = *p;

    // Existing value == 0 is a special case
    if( existing == 0 )
        err = false;
    else
    {

        // Calculate checksum, with zero value temporarily in checksum
        //  field
        *p     = 0;
        calculated = checksum_calculate( data, len );
        *p     = existing;

        // Does it match existing value ?
        err = (calculated!=existing);
        if( err )
            printf( "Checksum error !" );
    }
    return( err );
}

