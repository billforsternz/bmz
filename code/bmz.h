/*************************************************************************
 * bmz.h
 *
 *  BMZ = Bare Metal Zilog. Core routines
 *  Project: eZ2944
 *************************************************************************/
#ifndef BMZ_H
#define BMZ_H
#include "types.h"      // byte, bool etc.
#include "choices.h"    // conditional compilation

// User only needs to #include "bmz.h" to get access to all BMZ facilities
//  including messages, message queues, pools, tick counts and timers
#include "msg.h"
#include "mq.h"
#include "pool.h"
typedef byte TASKID;    // needed for timer.h
#include "timer.h"
#include "tick.h"

// Make available this key routine in library module vectors24.asm
extern void set_vector( unsigned short vector, void (*hndlr)(void) );

// My favourite macro - how many elements in an array ?
#define nbrof(array) (  sizeof(array) / sizeof((array)[0]) )

// Simple assert macro, could be improved
#ifdef DEBUG_ASSERT
#define assert(x)  if(!(x)) bmz_panic("assert")
#else
#define assert(x)
#endif

// Define different types of handlers users must supply
typedef void *(*HANDLER_INIT)    ( byte **, u16 * );
typedef void  (*HANDLER_IDLE)    ();
typedef void  (*HANDLER_TIMEOUT) ( byte );
typedef void  (*HANDLER_MSG)     ( MSG * );

// Publish state is a (very) simple mechanism for tasks to communicate
//  basic state information between themselves without messaging
typedef enum
{
    PUBLISH_IDLE,   // task is idle
    PUBLISH_ACTIVE, // task is active/operational
    PUBLISH_OTHER   // task is coming up or going down
} PUBLISH_STATE;

// User provides an array of TASK_DESCRIPTORs to describe all the tasks
//  in the system
typedef struct
{
    TASKID          taskid;
    HANDLER_INIT    init;
    HANDLER_IDLE    idle;
    HANDLER_TIMEOUT timeout;
    HANDLER_MSG     down;
    HANDLER_MSG     up;
    byte            mq_down_depth;
    byte            mq_up_depth;
    byte            pool_nbr;
    u16             pool_len;
    byte            pool_offset;
    TASKID          pool_share;
} TASK_DESCRIPTOR;

// TASKID_NULL is a sentinel value indicating "not a task". The user
//  should define all the valid TASKIDs starting from 1
#define TASKID_NULL 0

// Initialize BMZ, call this at start of main()
void bmz_init();

// Read task descriptor array to build task table
void bmz_define_system( const TASK_DESCRIPTOR *td, int td_nbr,
                                         byte **addr_mem, u16 *addr_len );

// Run the tasks
void bmz_run();

// Send message to a task's down handler
void bmz_down( TASKID taskid, MSG *msg );

// Send message to a task's up handler
void bmz_up( TASKID taskid, MSG *msg );

// Call a task's timeout handler
void bmz_timeout( TASKID taskid, byte timer_id );

// Get the currently running task's taskid
TASKID bmz_get_current_taskid();

// Get the currently running task's instance ptr
void *bmz_get_current_instance();

// Get the currently running task's message pool
POOL *bmz_get_current_pool();

// Get a task's instance ptr
void *bmz_get_instance( TASKID taskid );

// Get a task's up MQ
MQ *bmz_get_mq_up( TASKID taskid );

// Get a task's down MQ
MQ *bmz_get_mq_down( TASKID taskid );

// Get a task's POOL
POOL *bmz_get_pool( TASKID taskid );

// Get a task's published state
PUBLISH_STATE bmz_get_publish_state( TASKID taskid );

// Set the currently running task's published state
void bmz_set_publish_state( PUBLISH_STATE state );

// Panic because something has gone very wrong
void bmz_panic( const char *txt );

// Panic because we have run out of memory
void bmz_panic_memory( const char *txt );

#endif // BMZ_H
