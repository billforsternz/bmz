// console.h, header for console.c, see that file for full discussion
#ifndef CONSOLE_H
#define CONSOLE_H
int kbhit(void);
void putch( unsigned char c );
unsigned char getch( void );
unsigned char getche( void );
void putstr( const char *s );
void putu32( u32 n );
#endif  // CONSOLE_H

