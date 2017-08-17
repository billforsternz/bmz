// checksum.h, header for checksum.c, see that file for full discussion
#ifndef  CHECKSUM_H
#define  CHECKSUM_H
#include "types.h"
u16 checksum_calculate( const byte *data, u16 len );
bool checksum_test(  byte *data, u16 len, u16 offset );
#endif  // CHECKSUM_H
