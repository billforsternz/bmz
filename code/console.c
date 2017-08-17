/*************************************************************************
 * console.c
 *
 *  Console functions
 *  Project: eZ2944
 *************************************************************************/
#include "bmz.h"
#include "uart.h"
#include "console.h"

// Which UART are we using ?
#define CONSOLE_UART 0

/*************************************************************************
 * Setup UART if necessary
 *************************************************************************/
static void init()
{
    if( !uart_is_init(CONSOLE_UART) )
        uart_init( CONSOLE_UART, 57600, 8, 'N', 1 );
}

/*************************************************************************
 * Test whether keyboard character ready (is keyboard hit?)
 *************************************************************************/
int kbhit(void)
{
    init();
    return( (int)uart_read_test(CONSOLE_UART) );
}

/*************************************************************************
 * Write character with '\n' -> '\n', '\r' expansion
 *************************************************************************/
void putch( unsigned char c )
{
    init();
    uart_write( CONSOLE_UART, c );
    if( c == '\n' )
        uart_write( CONSOLE_UART, '\r' );
}

/*************************************************************************
 * Wait for character, then read it
 *************************************************************************/
unsigned char getch( void )
{
    init();
    while( !uart_read_test(CONSOLE_UART) )
        ;
    return( uart_read(CONSOLE_UART) );
}

/*************************************************************************
 * Wait for character, then read it and echo it
 *************************************************************************/
unsigned char getche( void )
{
    unsigned char c;
    c = getch();
    putch(c);
    return( c );
}


/*************************************************************************
 * printf() can effectively be suppressed using the conditional
 *  compilation directive below in choices.h
 *************************************************************************/
#ifdef PRINTF_SUPPRESS
int printf( const char *fmt, ... )  // printf() takes multiple arguments
                                    //  so we can't turn it into a macro
{
}
#endif

/*************************************************************************
 * Use putstr() and putu32() for output we never want to suppress (like
 *  bmz_panic() messages for example)
 *************************************************************************/
void putstr( const char *s )
{
    while( *s )
        putch( *s++ );
}

/*************************************************************************
 * Use putstr() and putu32() for output we never want to suppress (like
 *  bmz_panic() messages for example)
 *************************************************************************/
void putu32( u32 n )
{
    u32 digit, power = 1000000000;
    bool nonzero = false;
    while( power )
    {
        digit = n/power;
        if( digit )
            nonzero = true;
        if( nonzero )
            putch( (char)(digit+'0') );
        n = n%power;
        power = power/10;
    }
    if( !nonzero )
        putch( '0' );
}
