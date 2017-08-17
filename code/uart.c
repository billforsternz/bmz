/*****************************************************************************
 * uart.c
 *
 *  eZ80F91 Uart driver
 *  Project: eZ2944
 *****************************************************************************/
#include <ez80.h>
#include "bmz.h"
#include "uart.h"
#include "console.h"

// Temporary simple debug system
#ifdef DEBUG_DBG
void DBG( char c );
#else
#define DBG(c)
#endif

// Extra register definitions etc.
#define UART_DR     0x01
#define UART_THRE   0x20
#define UARTMODE    0x01
#ifdef _EZ80F91
#define SYSTEMCLOCK 50000000
#define UART0_IVECT 0x70
#define UART1_IVECT 0x74
#endif
/*
#ifdef _EZ80L92
#define SYSTEMCLOCK 48000000
#define UART0_IVECT 0x18
#endif
#ifdef _EZ80F92
#define SYSTEMCLOCK 20000000
#define UART0_IVECT 0x18
#endif
#ifdef _EZ80F93
#define SYSTEMCLOCK 20000000
#define UART0_IVECT 0x18
#endif
*/

// Buffer sizes
#define RX_SIZE 128
#define TX_SIZE 64
#if (RX_SIZE>256 || TX_SIZE>256)
    #error "using byte size offsets"
#endif

// Static data
typedef struct
{
    bool is_init;
    byte rx_buf[RX_SIZE];
    byte rx_get;
    byte rx_put;
    byte tx_buf[TX_SIZE];
    byte tx_get;
    byte tx_put;
} UART;
static UART uart0;
static UART uart1;

/*************************************************************************
 * UART 0 interrupt service routine
 *************************************************************************/
void interrupt uart0_isr( void )
{
    byte put, get, next;
    byte *buf, c;
    UART *p = &uart0;

    // Read rx characters
    buf = p->rx_buf;
    get = p->rx_get;
    put = p->rx_put;
    while( UART0_LSR & 0x05 )   // DR? (Data ready)
    {
        c = UART0_RBR;
        next = put+1;
        if( next == RX_SIZE )
            next = 0;
        if( next != get )   // if buffer not full
        {
            buf[put] = c;
            put = next;
        }
    }
    p->rx_put = put;

    // Write tx characters
    buf = p->tx_buf;
    get = p->tx_get;
    put = p->tx_put;
    while( UART0_LSR & 0x20) //THRE ? (Transmit holding register empty)
    {
        if( put == get )    // if buffer empty
        {
            UART0_IER &= 0xfd;  // disable tx interrupts
            break;
        }
        else
        {
            c = buf[get];
            UART0_THR = c;
            next = get+1;
            if( next == TX_SIZE )
                next = 0;
            get = next;
        }
    }
    p->tx_get = get;
}

/*************************************************************************
 * UART 1 interrupt service routine
 *************************************************************************/
void interrupt uart1_isr( void )
{
    byte put, get, next;
    byte *buf, c;
    UART *p = &uart1;

    // Read rx characters
    buf = p->rx_buf;
    get = p->rx_get;
    put = p->rx_put;
    while( UART1_LSR & 0x05 )   // DR? (Data ready)
    {
        c = UART1_RBR;
        next = put+1;
        if( next == RX_SIZE )
            next = 0;
        if( next != get )   // if buffer not full
        {
            buf[put] = c;
            put = next;
        }
        DBG('X');
    }
    p->rx_put = put;

    // Write tx characters
    buf = p->tx_buf;
    get = p->tx_get;
    put = p->tx_put;
    while( UART1_LSR & 0x20) //THRE ? (Transmit holding register empty)
    {
        if( put == get )    // if buffer empty
        {
            DBG('Z');
            UART1_IER &= 0xfd;  // disable tx interrupts
            break;
        }
        else
        {
            c = buf[get];
            UART1_THR = c;
            next = get+1;
            if( next == TX_SIZE )
                next = 0;
            get = next;
            DBG('Y');
        }
    }
    p->tx_get = get;
}

/*************************************************************************
 * Uart init
 *************************************************************************/
//   eg uart_init( 0, 9600, 8, 'N', 1 );
//   sets UART0 to 9600 baud, 8 bits, no parity, 1 stop bit
void uart_init( byte uart, u32 baudrate, byte bits, char parity, byte stop )
{
    u16 brg;
    byte bits43, bits210;
    UART *p = (uart==0 ? &uart0 : &uart1);

    // Turn off interrupts
    if( uart == 0 )
        UART0_IER = 0x00;
    else
        UART1_IER = 0x00;

    // Reset SW fifos
    p->rx_get = p->rx_put = p->tx_put = p->tx_get = 0;

    // Calculate baud divisor
    brg = (u16)(SYSTEMCLOCK / (baudrate * 16));

    // Parity
    switch( parity )
    {
        case 'e':
        case 'E':   bits43 = 0x18;  // 11 000b, even parity=1, parity enable=1
                    break;
        case 'o':
        case 'O':   bits43 = 0x08;  // 01 000b, even parity=0, parity enable=1
                    break;
     // case 'n':
     // case 'N':
        default:    bits43 = 0x00;  // 00 000b, even parity=0, parity enable=0
                    break;
    }

    // Data bits
    switch( bits )
    {
        case 5:     bits210 = 0;  break;
        case 6:     bits210 = 1;  break;
        case 7:     bits210 = 2;  break;
     // case 8:
        default:    bits210 = 3;  break;
    }

    // Stop bits
    if( stop == 2 )
        bits210 += 4;

    // UART0
    if( uart == 0 )
    {
        if( !p->is_init )
        {

            // Enable and clear HW fifos, rx interrupt on first byte
            UART0_FCTL = 0x07;

            // Set the PortD bits D1,D0 to Alternate function mode
            //  bit 0    1010 Alternative use (TX)
            //  bit 1    1010 Alternative use (RX)
            //  bit 2..7 0010 Input from pin
            //   (2=RTS,3=CTS,4=DTR,5=DSR,6=DCD,7=RI, review this only
            //        CTS,DST,DCD and RI are inputs, others are outputs)
            PD_ALT2 = 0x03;
            PD_ALT1 = 0x00;
            PD_DDR  = 0xff;
            PD_DR   = 0x00;

            // Set UART0 interrupt vector
            set_vector( UART0_IVECT, uart0_isr );
        }

        // Set UART0 baudrate
        UART0_LCTL |= 0x80;            // Set DLAB, dont disturb other bits
        UART0_BRG_L = (byte) ( brg & 0x00ff );        //Load divisor low
        UART0_BRG_H = (byte)(( brg & 0xff00 ) >> 8);  //Load divisor high

        // Clear DLAB, configure data bits, parity, stop bits
        UART0_LCTL = bits43+bits210;   // Reset DLAB, dont disturb other bits

        // Enable receive interrupt only
        UART0_IER = 0x01;
    }

    // UART1
    else
    {
        if( !p->is_init )
        {

            // Enable and clear HW fifos, rx interrupt on first byte
            UART1_FCTL = 0x07;

            // Set the PortC bits C1,C0 to Alternate function mode
            //  bit 0    1010 Alternative use (TX)
            //  bit 1    1010 Alternative use (RX)
            //  bit 2..7 0010 Input from pin
            //   (2=RTS,3=CTS,4=DTR,5=DSR,6=DCD,7=RI, review this only
            //        CTS,DST,DCD and RI are inputs, others are outputs)
            PC_ALT2 = 0x03;
            PC_ALT1 = 0x00;
            PC_DDR  = 0xff;
            PC_DR   = 0x00;

            // Set UART1 interrupt vector
            set_vector( UART1_IVECT, uart1_isr );
        }

        // Set UART1 baudrate
        UART1_LCTL |= 0x80;            // Set DLAB, dont disturb other bits
        UART1_BRG_L = (byte) ( brg & 0x00ff );        //Load divisor low
        UART1_BRG_H = (byte)(( brg & 0xff00 ) >> 8);  //Load divisor high

        // Clear DLAB, configure data bits, parity, stop bits
        UART1_LCTL = bits43+bits210;   // Reset DLAB, dont disturb other bits

        // Enable receive interrupt only
        UART1_IER = 0x01;
    }

    // Don't re-init HW
    p->is_init = true;
}

/*************************************************************************
 * Write character to UART
 *************************************************************************/
void uart_write( byte uart, byte c )
{
    UART *p = (uart==0 ? &uart0 : &uart1);
    byte put, get, next;
    byte *buf;
    byte done = 0;

    // Get buffer pointers
    buf = p->tx_buf;
    get = p->tx_get;
    put = p->tx_put;
    next = put+1;
    if( next == TX_SIZE )
        next = 0;

    // If buffer not empty, put character in buffer
    if( put != get )
    {
        while( next == get )   // while buffer full
        {
            get = p->tx_get;        // wait
            DBG('4');
        }
        DBG('3');
        buf[put] = c;
        p->tx_put = next;
    }

    // Else if buffer empty, try to write to chip
    else
    {
        if( uart==0 )
        {
            if( UART0_LSR & 0x20) //THRE ? (Transmit holding register empty)
            {
                UART0_THR = c;
                done = 1;
            }
        }
        else
        {
            if( UART1_LSR & 0x20) //THRE ? (Transmit holding register empty)
            {
                UART1_THR = c;
                done = 1;
                DBG('1');
            }
        }

        // But if chip full, write to buffer
        if( !done )
        {
            buf[put]  = c;
            p->tx_put = next;
            DBG('2');
        }
    }

    // If we didn't write character to chip, we need TX interrupt enabled
    if( !done )
    {
        if( uart == 0 )
            UART0_IER |= 2;  // enable tx interrupts
        else
            UART1_IER |= 2;  // enable tx interrupts
    }
}

/*************************************************************************
 * Test whether character available to be read from uart
 *************************************************************************/
byte uart_read_test( byte uart )
{
    UART *p = (uart==0 ? &uart0 : &uart1);
    byte put, get, next;
    byte ready=0;
    get = p->rx_get;
    put = p->rx_put;
    ready = (put!=get);
    return ready;
}

/*************************************************************************
 * Read character from UART
 *************************************************************************/
byte uart_read( byte uart )
{
    UART *p = (uart==0 ? &uart0 : &uart1);
    byte put, get, next;
    byte *buf, c=0;
    buf = p->rx_buf;
    get = p->rx_get;
    put = p->rx_put;
    if( put != get )
    {
        next = get+1;
        if( next == RX_SIZE )
            next = 0;
        c = buf[get];
        p->rx_get = next;
    }
    return c;
}


/*************************************************************************
 * Is a uart initialised ?
 *************************************************************************/
bool uart_is_init( byte uart )
{
    UART *p = (uart==0 ? &uart0 : &uart1);
    return( p->is_init );
}

/*************************************************************************
 * Temporary simple debug system
 *************************************************************************/
#ifdef DEBUG_DBG
void DBG( char c )
{
    static int ptr;
    static byte triggered;
    static char buf[50];
    int i, idx;
    if( c == 0 )
    {
        if( triggered )
        {
            putch( 'D' );
            putch( 'B' );
            putch( 'G' );
            putch( '[' );
            for( i=0; i<sizeof(buf); i++ )
                putch( buf[i] );
            putch( ']' );
        }
    }
    else
    {
        if( !triggered )
        {
            idx = ptr++;
            if( idx < sizeof(buf) )
                buf[idx] = c;
            else
                triggered = 1;
        }
    }
}
#endif
