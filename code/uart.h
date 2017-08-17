/*****************************************************************************
 * uart.h
 *
 *  eZ80F91 Uart driver
 *  Project: eZ2944
 *****************************************************************************/
#ifndef UART_H
#define UART_H
#include "types.h"

// Uart init
//   eg uart_init( 0, 9600, 8, 'N', 1 );
//   sets UART0 to 9600 baud, 8 bits, no parity, 1 stop bit
void uart_init( byte uart, u32 baudrate, byte bits, char parity, byte stop );

// Write character to UART
void uart_write( byte uart, byte c );

// Test whether character available to be read from uart
bool uart_read_test( byte uart );

// Read character from UART
byte uart_read( byte uart );

// Is a uart initialised ?
bool uart_is_init( byte uart );

#endif // UART_H
