/*
 * Copyright (c) 2005, Swedish Institute of Computer Science
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 */

/**
 * \addtogroup sys
 * @{
 */

/**
 * \defgroup process Contiki processes
 *
 * A process in Contiki consists of a single \ref pt "protothread".
 *
 * @{
 */

/**
 * \file
 * Header file for the Contiki process interface.
 * \author
 * Adam Dunkels <adam@sics.se>
 *
 */
#ifndef PROCESS_H_
#define PROCESS_H_

#include "sys/pt.h"
#include "sys/cc.h"

typedef unsigned char process_event_t;
typedef void *        process_data_t;
typedef unsigned char process_num_events_t;

/**
 * \name Return values
 * @{
 */

/**
 * \brief      Return value indicating that an operation was successful.
 *
 *             This value is returned to indicate that an operation
 *             was successful.
 */
#define PROCESS_ERR_OK        0
/**
 * \brief      Return value indicating that the event queue was full.
 *
 *             This value is returned from process_post() to indicate
 *             that the event queue was full and that an event could
 *             not be posted.
 */
#define PROCESS_ERR_FULL      1
/* @} */

#define PROCESS_NONE          NULL

#ifndef PROCESS_CONF_NUMEVENTS
#define PROCESS_CONF_NUMEVENTS 32
#endif /* PROCESS_CONF_NUMEVENTS */

#define PROCESS_EVENT_NONE            0x80
#define PROCESS_EVENT_INIT            0x81
#define PROCESS_EVENT_POLL            0x82
#define PROCESS_EVENT_EXIT            0x83
#define PROCESS_EVENT_SERVICE_REMOVED 0x84
#define PROCESS_EVENT_CONTINUE        0x85
#define PROCESS_EVENT_MSG             0x86
#define PROCESS_EVENT_EXITED          0x87
#define PROCESS_EVENT_TIMER           0x88
#define PROCESS_EVENT_COM             0x89
#define PROCESS_EVENT_MAX             0x8a

#define PROCESS_BROADCAST NULL
#define PROCESS_ZOMBIE ((struct process *)0x1)

/**
 * \name Process protothread functions
 * @{
 */

/**
 * Define the beginning of a process.
 *
 * This macro defines the beginning of a process, and must always
 * appear in a PROCESS_THREAD() definition. The PROCESS_END() macro
 * must come at the end of the process.
 *
 * \hideinitializer
 */
#define PROCESS_BEGIN()             PT_BEGIN(process_pt)

/**
 * Define the end of a process.
 *
 * This macro defines the end of a process. It must appear in a
 * PROCESS_THREAD() definition and must always be included. The
 * process exits when the PROCESS_END() macro is reached.
 *
 * \hideinitializer
 */
#define PROCESS_END()               PT_END(process_pt)

/**
 * Wait for an event to be posted to the process.
 *
 * This macro blocks the currently running process until the process
 * receives an event.
 *
 * \hideinitializer
 */
#define PROCESS_WAIT_EVENT()        PROCESS_YIELD()

/**
 * Wait for an event to be posted to the process, with an extra
 * condition.
 *
 * This macro is similar to PROCESS_WAIT_EVENT() in that it blocks the
 * currently running process until the process receives an event. But
 * PROCESS_WAIT_EVENT_UNTIL() takes an extra condition which must be
 * true for the process to continue.
 *
 * \param c The condition that must be true for the process to continue.
 * \sa PT_WAIT_UNTIL()
 *
 * \hideinitializer
 */
#define PROCESS_WAIT_EVENT_UNTIL(c) PROCESS_YIELD_UNTIL(c)

/**
 * Yield the currently running process.
 *
 * \hideinitializer
 */
#define PROCESS_YIELD()             PT_YIELD(process_pt)

/**
 * Yield the currently running process until a condition occurs.
 *
 * This macro is different from PROCESS_WAIT_UNTIL() in that
 * PROCESS_YIELD_UNTIL() is guaranteed to always yield at least
 * once. This ensures that the process does not end up in an infinite
 * loop and monopolizing the CPU.
 *
 * \param c The condition to wait for.
 *
 * \hideinitializer
 */
#define PROCESS_YIELD_UNTIL(c)      PT_YIELD_UNTIL(process_pt, c)

/**
 * Wait for a condition to occur.
 *
 * This macro does not guarantee that the process yields, and should
 * therefore be used with care. In most cases, PROCESS_WAIT_EVENT(),
 * PROCESS_WAIT_EVENT_UNTIL(), PROCESS_YIELD() or
 * PROCESS_YIELD_UNTIL() should be used instead.
 *
 * \param c The condition to wait for.
 *
 * \hideinitializer
 */
#define PROCESS_WAIT_UNTIL(c)       PT_WAIT_UNTIL(process_pt, c)
#define PROCESS_WAIT_WHILE(c)       PT_WAIT_WHILE(process_pt, c)

/**
 * Exit the currently running process.
 *
 * \hideinitializer
 */
#define PROCESS_EXIT()              PT_EXIT(process_pt)

/**
 * Spawn a protothread from the process.
 *
 * \param pt The protothread state (struct pt) for the new protothread
 * \param thread The call to the protothread function.
 * \sa PT_SPAWN()
 *
 * \hideinitializer
 */
#define PROCESS_PT_SPAWN(pt, thread)   PT_SPAWN(process_pt, pt, thread)

/**
 * Yield the process for a short while.
 *
 * This macro yields the currently running process for a short while,
 * thus letting other processes run before the process continues.
 *
 * \hideinitializer
 */
#define PROCESS_PAUSE()             do {				\
  process_post(PROCESS_CURRENT(), PROCESS_EVENT_CONTINUE, NULL);	\
  PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_CONTINUE);               \
} while(0)

/** @} end of protothread functions */

/**
 * \name Poll and exit handlers
 * @{
 */
/**
 * Specify an action when a process is polled.
 *
 * \note This declaration must come immediately before the
 * PROCESS_BEGIN() macro.
 *
 * \param handler The action to be performed.
 *
 * \hideinitializer
 */
#define PROCESS_POLLHANDLER(handler) if(ev == PROCESS_EVENT_POLL) { handler; }

/**
 * Specify an action when a process exits.
 *
 * \note This declaration must come immediately before the
 * PROCESS_BEGIN() macro.
 *
 * \param handler The action to be performed.
 *
 * \hideinitializer
 */
#define PROCESS_EXITHANDLER(handler) if(ev == PROCESS_EVENT_EXIT) { handler; }

/** @} */

/**
 * \name Process declaration and definition
 * @{
 */

/**
 * Define the body of a process.
 *
 * This macro is used to define the body (protothread) of a
 * process. The process is called whenever an event occurs in the
 * system, A process always start with the PROCESS_BEGIN() macro and
 * end with the PROCESS_END() macro.
 *
 * \hideinitializer
 */
#define PROCESS_THREAD(name, ev, data) 				\
static PT_THREAD(process_thread_##name(struct pt *process_pt,	\
				       process_event_t ev,	\
				       process_data_t data))

/**
 * Declare the name of a process.
 *
 * This macro is typically used in header files to declare the name of
 * a process that is implemented in the C file.
 *
 * \hideinitializer
 */
#define PROCESS_NAME(name) extern struct process name

/**
 * Declare a process.
 *
 * This macro declares a process. The process has two names: the
 * variable of the process structure, which is used by the C program,
 * and a human readable string name, which is used when debugging.
 * A configuration option allows removal of the readable name to save RAM.
 *
 * \param name The variable name of the process structure.
 * \param strname The string representation of the process' name.
 *
 * \hideinitializer
 */
#if PROCESS_CONF_NO_PROCESS_NAMES
#define PROCESS(name, strname)				\
  PROCESS_THREAD(name, ev, data);			\
  struct process name = { NULL,		        \
                          process_thread_##name }
#else
#define PROCESS(name, strname)				\
  PROCESS_THREAD(name, ev, data);			\
  struct process name = { NULL, strname,		\
                          process_thread_##name }
#endif

/** @} */

struct process {
  struct process *next;
#if PROCESS_CONF_NO_PROCESS_NAMES
#define PROCESS_NAME_STRING(process) ""
#else
  const char *name;
#define PROCESS_NAME_STRING(process) (process)->name
#endif
  PT_THREAD((* thread)(struct pt *, process_event_t, process_data_t));
  struct pt pt;
  unsigned char state, needspoll;
};

/**
 * \name Functions called from application programs
 * @{
 */

/**
 * Start a process.
 *
 * \param p A pointer to a process structure.
 *
 * \param data An argument pointer that can be passed to the new
 * process
 *
 */
CCIF void process_start(struct process *p, process_data_t data);

/**
 * Post an asynchronous event.
 *
 * This function posts an asynchronous event to one or more
 * processes. The handing of the event is deferred until the target
 * process is scheduled by the kernel. An event can be broadcast to
 * all processes, in which case all processes in the system will be
 * scheduled to handle the event.
 *
 * \param ev The event to be posted.
 *
 * \param data The auxiliary data to be sent with the event
 *
 * \param p The process to which the event should be posted, or
 * PROCESS_BROADCAST if the event should be posted to all processes.
 *
 * \retval PROCESS_ERR_OK The event could be posted.
 *
 * \retval PROCESS_ERR_FULL The event queue was full and the event could
 * not be posted.
 */
CCIF int process_post(struct process *p, process_event_t ev, process_data_t data);

/**
 * Post a synchronous event to a process.
 *
 * \param p A pointer to the process' process structure.
 *
 * \param ev The event to be posted.
 *
 * \param data A pointer to additional data that is posted together
 * with the event.
 */
CCIF void process_post_synch(struct process *p,
			     process_event_t ev, process_data_t data);

/**
 * \brief      Cause a process to exit
 * \param p    The process that is to be exited
 *
 *             This function causes a process to exit. The process can
 *             either be the currently executing process, or another
 *             process that is currently running.
 *
 * \sa PROCESS_CURRENT()
 */
CCIF void process_exit(struct process *p);


/**
 * Get a pointer to the currently running process.
 *
 * This macro get a pointer to the currently running
 * process. Typically, this macro is used to post an event to the
 * current process with process_post().
 *
 * \hideinitializer
 */
#define PROCESS_CURRENT() process_current
CCIF extern struct process *process_current;

/**
 * Switch context to another process
 *
 * This function switch context to the specified process and executes
 * the code as if run by that process. Typical use of this function is
 * to switch context in services, called by other processes. Each
 * PROCESS_CONTEXT_BEGIN() must be followed by the
 * PROCESS_CONTEXT_END() macro to end the context switch.
 *
 * Example:
 \code
 PROCESS_CONTEXT_BEGIN(&test_process);
 etimer_set(&timer, CLOCK_SECOND);
 PROCESS_CONTEXT_END(&test_process);
 \endcode
 *
 * \param p    The process to use as context
 *
 * \sa PROCESS_CONTEXT_END()
 * \sa PROCESS_CURRENT()
 */
#define PROCESS_CONTEXT_BEGIN(p) {\
struct process *tmp_current = PROCESS_CURRENT();\
process_current = p

/**
 * End a context switch
 *
 * This function ends a context switch and changes back to the
 * previous process.
 *
 * \param p    The process used in the context switch
 *
 * \sa PROCESS_CONTEXT_START()
 */
#define PROCESS_CONTEXT_END(p) process_current = tmp_current; }

/**
 * \brief      Allocate a global event number.
 * \return     The allocated event number
 *
 *             In Contiki, event numbers above 128 are global and may
 *             be posted from one process to another. This function
 *             allocates one such event number.
 *
 * \note       There currently is no way to deallocate an allocated event
 *             number.
 */
CCIF process_event_t process_alloc_event(void);

/** @} */

/**
 * \name Functions called from device drivers
 * @{
 */

/**
 * Request a process to be polled.
 *
 * This function typically is called from an interrupt handler to
 * cause a process to be polled.
 *
 * \param p A pointer to the process' process structure.
 */
CCIF void process_poll(struct process *p);

/** @} */

/**
 * \name Functions called by the system and boot-up code
 * @{
 */

/**
 * \brief      Initialize the process module.
 *
 *             This function initializes the process module and should
 *             be called by the system boot-up code.
 */
void process_init(void);

/**
 * Run the system once - call poll handlers and process one event.
 *
 * This function should be called repeatedly from the main() program
 * to actually run the Contiki system. It calls the necessary poll
 * handlers, and processes one event. The function returns the number
 * of events that are waiting in the event queue so that the caller
 * may choose to put the CPU to sleep when there are no pending
 * events.
 *
 * \return The number of events that are currently waiting in the
 * event queue.
 */
int process_run(void);


/**
 * Check if a process is running.
 *
 * This function checks if a specific process is running.
 *
 * \param p The process.
 * \retval Non-zero if the process is running.
 * \retval Zero if the process is not running.
 */
CCIF int process_is_running(struct process *p);

/**
 *  Number of events waiting to be processed.
 *
 * \return The number of events that are currently waiting to be
 * processed.
 */
int process_nevents(void);

/** @} */

CCIF extern struct process *process_list;

#define PROCESS_LIST() process_list

#endif /* PROCESS_H_ */

/** @} */
/** @} */
