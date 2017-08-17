/*************************************************************************
 * types.h
 *
 *  A basic set of simple extended types for embedded projects
 *  Project: eZ2944
 *************************************************************************/
#ifndef TYPES_H
#define TYPES_H

// Define a bool type to be compatible with C++ keywords
typedef unsigned char bool;
#define false 0
#define true  (!false)

// Unsigned 8, 16 and 32 bit types
typedef unsigned char  byte;
typedef unsigned short u16;
typedef unsigned long  u32;
#endif // TYPES_H
