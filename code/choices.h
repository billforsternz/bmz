/*************************************************************************
 * choices.h
 *
 *  Gather all conditional compilation directives here
 *  Project: eZ2944
 *************************************************************************/
#ifndef CHOICES_H
#define CHOICES_H

// Leave defined to suppress all printf() debugging (does not
//  effect putstr() debugging) - this is why there are still
//  a few printf()'s lying about
#define PRINTF_SUPPRESS

// Leave defined for printf of TX frames
// #define DEBUG_TX_FRAME

// Leave defined to discard occasional TX frames for testing
// #define DEBUG_TX_DISCARD

// Leave defined for printf of RX frames
// #define DEBUG_RX_FRAME

// Leave defined to discard occasional RX frames for testing
// #define DEBUG_RX_DISCARD

// Leave defined to enable the DBG() debugging function
// #define DEBUG_DBG

// Leave defined to printf TCP retry timer expiry
// #define DEBUG_TCP_TIMEOUT

// Leave defined to printf TCP delayed ack timer expiry
// #define DEBUG_TCP_DELAYED_ACK_TIMEOUT

// Leave defined to printf TCP start retry timer
// #define DEBUG_TCP_START_RETRY

// Leave defined to printf ethernet initialisation messages
// #define DEBUG_ETHER_INIT

// Leave defined to do assert() checks
// #define DEBUG_ASSERT

#endif //CHOICES_H
