/*
 * Copyright (c) 1997-2015, Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file
 *
 * @brief Public APIs for the nanokernel.
 */

#ifndef __NANOKERNEL_H__
#define __NANOKERNEL_H__

/**
 * @defgroup nanokernel_services Nanokernel Services
 */

/* fundamental include files */

#include <stddef.h>
#include <stdint.h>
#include <toolchain.h>

/* generic kernel public APIs */

#include <kernel_version.h>
#include <sys_clock.h>
#include <drivers/rand32.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * @cond internal
 * nanokernel private APIs that are exposed via the public API
 *
 * THESE ITEMS SHOULD NOT BE REFERENCED EXCEPT BY THE KERNEL ITSELF!
 */

struct _nano_queue {
	void *head;
	void *tail;
};

#include <misc/dlist.h>

struct _nano_timeout {
	sys_dlist_t node;
	struct _nano_queue *wait_q;
	int32_t delta_ticks_from_prev;
};
/**
 * @endcond
 */

struct tcs;

/* architecture-independent nanokernel public APIs */

typedef struct tcs *nano_thread_id_t;

typedef void (*nano_fiber_entry_t)(int i1, int i2);

typedef int nano_context_type_t;

#define NANO_CTX_ISR (0)
#define NANO_CTX_FIBER (1)
#define NANO_CTX_TASK (2)

/* timeout special values */
#define TICKS_UNLIMITED (-1)
#define TICKS_NONE 0

/*
 * execution context APIs
 */


/**
 * @brief Return the currently executing thread
 *
 * This routine returns a pointer to the thread control block of the currently
 * executing thread.  It is cast to a nano_thread_id_t for use publicly.
 *
 * @return nano_thread_id_t of the currently executing thread.
 */
extern nano_thread_id_t sys_thread_self_get(void);

/**
 *
 * @brief Return the type of the current execution context
 *
 * This routine returns the type of execution context currently executing.
 *
 * @return nano_context_type_t of the currently executing thread.
 */
extern nano_context_type_t sys_execution_context_type_get(void);

extern int _is_thread_essential(nano_thread_id_t pCtx);

/**
 *
 * @brief Busy wait the currently executing thread
 *
 * This routine causes the current task or fiber to execute a "do nothing"
 * loop for a specified period of time.
 *
 * @warning This routine utilizes the system clock, so it must not be invoked
 *          until the system clock is fully operational or while interrupts
 *          are locked.
 *
 * @param usec_to_wait Number of microseconds to busy wait.
 *
 * @return N/A
 */
extern void sys_thread_busy_wait(uint32_t usec_to_wait);

/**
 * @brief Nanokernel Fibers
 * @defgroup nanokernel_fiber Nanokernel Fibers
 * @ingroup nanokernel_services
 * @{
 */
/* execution context-independent method (when context is not known) */

/**
 * @brief Initialize and start a fiber
 *
 * This routine initializes and starts a fiber; it can be called from
 * either a fiber or a task.  When this routine is called from a
 * task, the newly created fiber will start executing immediately.
 *
 * @internal
 * Given that this routine is _not_ ISR-callable, the following code is used
 * to differentiate between a task and fiber:
 *
 *    if ((_nanokernel.current->flags & TASK) == TASK)
 *
 * Given that the _fiber_start() primitive is not considered real-time
 * performance critical, a runtime check to differentiate between a calling
 * task or fiber is performed in order to conserve footprint.
 * @endinternal
 *
 * @param stack Pointer to the stack space
 * @param stack_size Stack size in bytes
 * @param entry Fiber entry
 * @param arg1 1st parameter to entry point
 * @param arg2 2nd parameter to entry point
 * @param prio Fiber priority
 * @param options unused
 * @return N/A
 */
void fiber_start(char *stack, unsigned stack_size, nano_fiber_entry_t entry,
		int arg1, int arg2, unsigned prio, unsigned options);

/* methods for fibers */

/**
 * @brief Initialize and start a fiber
 *
 * This routine initializes and starts a fiber; it can be called from
 * a fiber.
 *
 * @param pStack Pointer to the stack space
 * @param stackSize Stack size in bytes
 * @param entry Fiber entry
 * @param arg1 1st parameter to entry point
 * @param arg2 2nd parameter to entry point
 * @param prio Fiber priority
 * @param options unused
 * @return N/A
 */
extern void fiber_fiber_start(char *pStack, unsigned int stackSize,
		nano_fiber_entry_t entry, int arg1, int arg2, unsigned prio,
		unsigned options);

/**
 * @brief Yield the current fiber
 *
 * Invocation of this routine results in the current fiber yielding to
 * another fiber of the same or higher priority.  If there doesn't exist
 * any other fibers of the same or higher priority that are runnable, this
 * routine will return immediately.
 *
 * This routine can only be called from a fiber.
 *
 * @return N/A
 */
extern void fiber_yield(void);


/**
 * @brief Abort the currently executing fiber
 *
 * This routine is used to abort the currently executing fiber. This can occur
 * because:
 * - the fiber has explicitly aborted itself (by calling this routine),
 * - the fiber has implicitly aborted itself (by returning from its entry point),
 * - the fiber has encountered a fatal exception.
 *
 * This routine can only be called from a fiber.
 *
 * @return This function never returns
 */
extern void fiber_abort(void);

#ifdef CONFIG_NANO_TIMEOUTS

/**
 * @brief Put the current fiber to sleep
 *
 * Put the currently running fiber to sleep for the number of system ticks
 * passed in the @a timeout_in_ticks parameter.
 *
 * @param timeout_in_ticks Number of system ticks to sleep
 *
 * @return N/A
 */
extern void fiber_sleep(int32_t timeout_in_ticks);

#ifndef CONFIG_MICROKERNEL
/**
 * @brief Put the task to sleep
 *
 * Put the task to sleep for the number of system ticks passed in the
 * @a timeout_in_ticks parameter.
 *
 * @param timeout_in_ticks Number of system ticks to sleep. A value of
 * TICKS_UNLIMITED is considered invalid and may result in unexpected behavior.
 *
 * @return N/A
 */
extern void task_sleep(int32_t timeout_in_ticks);
#endif

/**
 * @brief start a fiber, but delay its execution
 *
 * @param stack pointer to the stack space
 * @param stack_size_in_bytes stack size in bytes
 * @param entry_point fiber entry point
 * @param param1 1st parameter to entry point
 * @param param2 2nd parameter to entry point
 * @param priority fiber priority
 * @param options unused
 * @param timeout_in_ticks timeout in ticks
 *
 * @return a handle to allow canceling the delayed start
 */
extern void *fiber_fiber_delayed_start(char *stack,
		unsigned int stack_size_in_bytes,
		nano_fiber_entry_t entry_point, int param1,
		int param2, unsigned int priority,
		unsigned int options, int32_t timeout_in_ticks);

extern void *fiber_delayed_start(char *stack, unsigned int stack_size_in_bytes,
		nano_fiber_entry_t entry_point, int param1,
		int param2, unsigned int priority,
		unsigned int options, int32_t timeout_in_ticks);
extern void fiber_delayed_start_cancel(void *handle);

/**
 * @brief Cancel a delayed fiber start
 *
 * @param handle A handle returned when asking to start the fiber
 *
 * @return N/A
 */
extern void fiber_fiber_delayed_start_cancel(void *handle);
#endif

/**
 * @}
 * @brief Nanokernel Task
 * @defgroup nanokernel_task Nanokernel Task
 * @ingroup nanokernel_services
 * @{
 */

/* methods for tasks */

/**
 * @brief Initialize and start a fiber from a task
 *
 * @sa fiber_fiber_start
 */
extern void task_fiber_start(char *pStack, unsigned int stackSize,
		nano_fiber_entry_t entry, int arg1, int arg2, unsigned prio,
		unsigned options);
#ifdef CONFIG_NANO_TIMEOUTS

/**
 * @brief Start a fiber from a task, but delay its execution
 *
 * @sa fiber_fiber_delayed_start
 */
extern void *task_fiber_delayed_start(char *stack,
		unsigned int stack_size_in_bytes,
		nano_fiber_entry_t entry_point, int param1,
		int param2, unsigned int priority,
		unsigned int options, int32_t timeout_in_ticks);
/**
 * @brief Cancel a delayed fiber start in task
 *
 * @sa fiber_fiber_delayed_start
 */
extern void task_fiber_delayed_start_cancel(void *handle);
#endif

/**
 * @}
 * @brief Nanokernel FIFOs
 * @defgroup nanokernel_fifo Nanokernel FIFO
 * @ingroup nanokernel_services
 * @{
 */
struct nano_fifo {
	union {
		struct _nano_queue wait_q;
		struct _nano_queue data_q;
	};
	int stat;
#ifdef CONFIG_DEBUG_TRACING_KERNEL_OBJECTS
	struct nano_fifo *next;
#endif
};

/**
 *
 * @brief Initialize a nanokernel multiple-waiter fifo (fifo) object
 *
 * This function initializes a nanokernel multiple-waiter fifo (fifo) object
 * structure.
 *
 * It may be called from either a fiber or task.
 *
 * The wait queue and data queue occupy the same space since there cannot
 * be both queued data and pending fibers in the FIFO. Care must be taken
 * that, when one of the queues becomes empty, it is reset to a state
 * that reflects an empty queue to both the data and wait queues.
 *
 * If the 'stat' field is a positive value, it indicates how many data
 * elements reside in the FIFO.  If the 'stat' field is a negative value,
 * its absolute value indicates how many fibers are pending on the LIFO
 * object.  Thus a value of '0' indicates that there are no data elements
 * in the LIFO _and_ there are no pending fibers.
 *
 * @param fifo FIFO to initialize.
 *
 * @return N/A
 */
extern void nano_fifo_init(struct nano_fifo *fifo);
/* execution context-independent methods (when context is not known) */

/**
 *
 * @brief Add an element to the end of a fifo
 *
 * This is a convenience wrapper for the execution context-specific APIs. This
 * is helpful whenever the exact execution context is not known, but should be
 * avoided when the context is known up-front (to avoid unnecessary overhead).
 *
 * @param fifo FIFO on which to interact.
 * @param data Data to send.
 *
 * @return N/A
 */
extern void nano_fifo_put(struct nano_fifo *fifo, void *data);

/**
 *
 * @brief Get an element from the head a fifo
 *
 * Remove the head element from the specified nanokernel multiple-waiter fifo
 * linked list fifo; it may be called from a fiber, task, or ISR context.
 *
 * If no elements are available, NULL is returned.  The first word in the
 * element contains invalid data because that memory location was used to store
 * a pointer to the next element in the linked list.
 *
 * @param fifo FIFO on which to interact.
 *
 * @return Pointer to head element in the list if available, otherwise NULL
 */
extern void *nano_fifo_get(struct nano_fifo *fifo);

/**
 *
 * @brief Get the head element of a fifo, poll/pend if empty
 *
 * This is a convenience wrapper for the execution context-specific APIs. This
 * is helpful whenever the exact execution context is not known, but should be
 * avoided when the context is known up-front (to avoid unnecessary overhead).
 *
 * @warning It's only valid to call this API from a fiber or a task.
 *
 * @param fifo FIFO on which to interact.
 *
 * @return Pointer to head element in the list
 */
extern void *nano_fifo_get_wait(struct nano_fifo *fifo);

/**
 *
 * @brief Get the head element of a fifo, poll/pend with timeout if empty
 *
 * This is a convenience wrapper for the execution context-specific APIs. This
 * is helpful whenever the exact execution context is not known, but should be
 * avoided when the context is known up-front (to avoid unnecessary overhead).
 *
 * @warning It's only valid to call this API from a fiber or a task.
 *
 * @param fifo FIFO on which to interact.
 * @param timeout Timeout measured in ticks
 *
 * @return Pointer to head element in the list
 */
extern void *nano_fifo_get_wait_timeout(struct nano_fifo *fifo,
		int32_t timeout);

/*
 * methods for ISRs
 */

/**
 *
 * @brief Add an element to the end of a FIFO from an ISR context.
 *
 * This is an alias for the execution context-specific API. This is
 * helpful whenever the exact execution context is known. Its use
 * avoids unnecessary overhead.
 *
 * @param fifo FIFO on which to interact.
 * @param data Data to send.
 *
 * @return N/A
 */
extern void nano_isr_fifo_put(struct nano_fifo *fifo, void *data);

/**
 * @brief Get an element from the head of a FIFO from an ISR context.
 *
 * Remove the head element from the specified nanokernel multiple-waiter fifo
 * linked list fifo. It may be called from an ISR context.
 *
 * The first word in the element contains invalid data because that memory
 * location was used to store a pointer to the next element in the linked list.
 *
 * @param fifo FIFO on which to interact.
 *
 * @return Pointer to head element in the list if available, otherwise NULL
 */
extern void *nano_isr_fifo_get(struct nano_fifo *fifo);

/* methods for fibers */

/**
 *
 * @brief Add an element to the end of a FIFO from a fiber.
 *
 * This is an alias for the execution context-specific API. This is
 * helpful whenever the exact execution context is known. Its use
 * avoids unnecessary overhead.
 *
 * @param fifo FIFO on which to interact.
 * @param data Data to send.
 *
 * @return N/A
 */
extern void nano_fiber_fifo_put(struct nano_fifo *fifo, void *data);

/**
 * @brief Get an element from the head of a FIFO from a fiber.
 *
 * Remove the head element from the specified nanokernel multiple-waiter fifo
 * linked list fifo. It may be called from a fiber.
 *
 * The first word in the element contains invalid data because that memory
 * location was used to store a pointer to the next element in the linked list.
 *
 * @param fifo FIFO on which to interact.
 *
 * @return Pointer to head element in the list if available, otherwise NULL
 */
extern void *nano_fiber_fifo_get(struct nano_fifo *fifo);

/**
 *
 * @brief Get the head element of a fifo, wait if empty
 *
 * Remove the head element from the specified system-level multiple-waiter
 * fifo; it can only be called from a fiber.
 *
 * If no elements are available, the calling fiber will pend until an element
 * is put onto the fifo.
 *
 * The first word in the element contains invalid data because that memory
 * location was used to store a pointer to the next element in the linked list.
 *
 * @param fifo FIFO on which to interact.
 *
 * @return Pointer to head element in the list
 *
 * @note There exists a separate nano_task_fifo_get_wait() implementation
 * since a task cannot pend on a nanokernel object. Instead tasks will
 * poll the fifo object.
 */
extern void *nano_fiber_fifo_get_wait(struct nano_fifo *fifo);
#ifdef CONFIG_NANO_TIMEOUTS

/**
 * @brief get the head element of a fifo, pend with a timeout if empty
 *
 * Remove the head element from the specified nanokernel fifo; it can only be
 * called from a fiber.
 *
 * If no elements are available, the calling fiber will pend until an element
 * is put onto the fifo, or the timeout expires, whichever comes first.
 *
 * The first word in the element contains invalid data because that memory
 * location was used to store a pointer to the next element in the linked
 * list.
 *
 * @sa nano_task_stack_pop_wait()
 *
 * @param fifo the FIFO on which to interact.
 * @param timeout_in_ticks time to wait in ticks
 *
 * @return Pointer to head element in the list, NULL if timed out
 */
extern void *nano_fiber_fifo_get_wait_timeout(struct nano_fifo *fifo,
		int32_t timeout_in_ticks);
#endif

/* methods for tasks */

/**
 *
 * @brief Add an element to the end of a fifo
 *
 * This routine adds an element to the end of a fifo object; it can be called
 * from only a task.  A fiber pending on the fifo object will be made
 * ready, and will preempt the running task immediately.
 *
 * If a fiber is waiting on the fifo, the address of the element is returned to
 * the waiting fiber.  Otherwise, the element is linked to the end of the list.
 *
 * @param fifo FIFO on which to interact.
 * @param data Data to send.
 *
 * @return N/A
 */
extern void nano_task_fifo_put(struct nano_fifo *fifo, void *data);

extern void *nano_task_fifo_get(struct nano_fifo *fifo);

/**
 *
 * @brief Get the head element of a fifo, poll if empty
 *
 * Remove the head element from the specified system-level multiple-waiter
 * fifo; it can only be called from a task.
 *
 * If no elements are available, the calling task will poll until an
 * until an element is put onto the fifo.
 *
 * The first word in the element contains invalid data because that memory
 * location was used to store a pointer to the next element in the linked list.
 *
 * @param fifo FIFO on which to interact.
 *
 * @sa nano_task_stack_pop_wait()
 *
 * @return Pointer to head element in the list
 */
extern void *nano_task_fifo_get_wait(struct nano_fifo *fifo);
#ifdef CONFIG_NANO_TIMEOUTS

/**
 * @brief get the head element of a fifo, poll with a timeout if empty
 *
 * Remove the head element from the specified nanokernel fifo; it can only be
 * called from a task.
 *
 * If no elements are available, the calling task will poll until an element
 * is put onto the fifo, or the timeout expires, whichever comes first.
 *
 * The first word in the element contains invalid data because that memory
 * location was used to store a pointer to the next element in the linked
 * list.
 *
 * @sa nano_task_stack_pop_wait()
 *
 * @param fifo the FIFO on which to operate
 * @param timeout_in_ticks time to wait in ticks
 *
 * @return Pointer to head element in the list, NULL if timed out
 */
extern void *nano_task_fifo_get_wait_timeout(struct nano_fifo *fifo,
		int32_t timeout_in_ticks);
#endif

/* LIFO APIs */

/**
 * @}
 * @brief Nanokernel LIFOs
 * @defgroup nanokernel_lifo Nanokernel LIFOs
 * @ingroup nanokernel_services
 * @{
 */
struct nano_lifo {
	struct _nano_queue wait_q;
	void *list;
#ifdef CONFIG_DEBUG_TRACING_KERNEL_OBJECTS
	struct nano_lifo *next;
#endif
};

/**
 * @brief Initialize a nanokernel linked list lifo object
 *
 * This function initializes a nanokernel system-level linked list lifo
 * object structure.
 *
 * It may be called from either a fiber or task.
 *
 * @param lifo LIFO to initialize.
 *
 * @return N/A
 */
extern void nano_lifo_init(struct nano_lifo *lifo);

/**
 * @brief Prepend an element to a LIFO
 *
 * This is a convenience wrapper for the execution context-specific APIs. This
 * is helpful whenever the exact execution context is not known, but should be
 * avoided when the context is known up-front (to avoid unnecessary overhead).
 *
 * @param lifo LIFO on which to put.
 * @param data Data to insert.
 *
 * @return N/A
 */
extern void nano_lifo_put(struct nano_lifo *lifo, void *data);

/**
 * @brief Get the first element from a LIFO
 *
 * This is a convenience wrapper for the execution context-specific APIs. This
 * is helpful whenever the exact execution context is not known, but should be
 * avoided when the context is known up-front (to avoid unnecessary overhead).
 *
 * @param lifo LIFO on which to receive
 *
 * @return Pointer to first element in the list if available, otherwise NULL
 */
extern void *nano_lifo_get(struct nano_lifo *lifo);

/**
 * @brief Get the first element from a LIFO, poll/pend if empty
 *
 * This is a convenience wrapper for the execution context-specific APIs. This
 * is helpful whenever the exact execution context is not known, but should be
 * avoided when the context is known up-front (to avoid unnecessary overhead).
 *
 * @warning It's only valid to call this API from a fiber or a task.
 *
 * @param lifo LIFO on which to receive
 *
 * @return Pointer to first element in the list
 */
extern void *nano_lifo_get_wait(struct nano_lifo *lifo);

/**
 * @brief Get the first element from a LIFO, poll/pend with a timeout if empty
 *
 * This is a convenience wrapper for the execution context-specific APIs. This
 * is helpful whenever the exact execution context is not known, but should be
 * avoided when the context is known up-front (to avoid unnecessary overhead).
 *
 * @warning It's only valid to call this API from a fiber or a task.
 *
 * @param lifo LIFO on which to receive
 * @param timeout Timeout in ticks
 *
 * @return Pointer to first element in the list
 */
extern void *nano_lifo_get_wait_timeout(struct nano_lifo *lifo,
		int32_t timeout);

/* methods for ISRs */

/**
 * @brief Prepend an element to a LIFO without a context switch.
 *
 * This routine adds an element to the head of a LIFO object; it may be
 * called from an ISR context. A fiber pending on the LIFO
 * object will be made ready, but will NOT be scheduled to execute.
 *
 * @param lifo LIFO on which to put.
 * @param data Data to insert.
 *
 * @return N/A
 */
extern void nano_isr_lifo_put(struct nano_lifo *lifo, void *data);

/**
 * @brief Remove the first element from a linked list LIFO
 *
 * Remove the first element from the specified nanokernel linked list LIFO;
 * it may be called from an ISR context.
 *
 * If no elements are available, NULL is returned. The first word in the
 * element contains invalid data because that memory location was used to store
 * a pointer to the next element in the linked list.
 *
 * @param lifo LIFO from which to receive.
 *
 * @return Pointer to first element in the list if available, otherwise NULL
 */
extern void *nano_isr_lifo_get(struct nano_lifo *lifo);

/* methods for fibers */

/**
 * @brief Prepend an element to a LIFO without a context switch.
 *
 * This routine adds an element to the head of a LIFO object; it may be
 * called from a fiber. A fiber pending on the LIFO
 * object will be made ready, but will NOT be scheduled to execute.
 *
 * @param lifo LIFO from which to put.
 * @param data Data to insert.
 *
 * @return N/A
 */
extern void nano_fiber_lifo_put(struct nano_lifo *lifo, void *data);

/**
 * @brief Remove the first element from a linked list LIFO
 *
 * Remove the first element from the specified nanokernel linked list LIFO;
 * it may be called from a fiber.
 *
 * If no elements are available, NULL is returned. The first word in the
 * element contains invalid data because that memory location was used to store
 * a pointer to the next element in the linked list.
 *
 * @param lifo LIFO from which to receive
 *
 * @return Pointer to first element in the list if available, otherwise NULL
 */
extern void *nano_fiber_lifo_get(struct nano_lifo *lifo);

/**
 * @brief Get the first element from a LIFO, wait if empty.
 *
 * Remove the first element from the specified system-level linked list LIFO;
 * it can only be called from a fiber.
 *
 * If no elements are available, the calling fiber will pend until an element
 * is put onto the list.
 *
 * The first word in the element contains invalid data because that memory
 * location was used to store a pointer to the next element in the linked list.
 *
 * @param lifo LIFO from which to receive.
 *
 * @return Pointer to first element in the list
 */
extern void *nano_fiber_lifo_get_wait(struct nano_lifo *lifo);

#ifdef CONFIG_NANO_TIMEOUTS

/**
 * @brief get the first element from a LIFO, wait with a timeout if empty
 *
 * Remove the first element from the specified system-level linked list lifo;
 * it can only be called from a fiber.
 *
 * If no elements are available, the calling fiber will pend until an element
 * is put onto the list, or the timeout expires, whichever comes first.
 *
 * The first word in the element contains invalid data because that memory
 * location was used to store a pointer to the next element in the linked list.
 *
 * @param lifo LIFO on which to operate.
 * @param timeout_in_ticks Time to wait in ticks.
 *
 * @return Pointer to first element in the list, NULL if timed out.
 */
extern void *nano_fiber_lifo_get_wait_timeout(struct nano_lifo *lifo,
		int32_t timeout_in_ticks);

#endif

/* methods for tasks */

/**
 * @brief Add an element to the head of a linked list LIFO
 *
 * This routine adds an element to the head of a LIFO object; it can be
 * called only from a task. A fiber pending on the LIFO
 * object will be made ready and will preempt the running task immediately.
 *
 * This API can only be called by a task.
 *
 * @param lifo LIFO from which to put.
 * @param data Data to insert.
 *
 * @return N/A
 */
extern void nano_task_lifo_put(struct nano_lifo *lifo, void *data);

/**
 * @brief Remove the first element from a linked list LIFO
 *
 * Remove the first element from the specified nanokernel linked list LIFO;
 * it may be called from a task.
 *
 * If no elements are available, NULL is returned. The first word in the
 * element contains invalid data because that memory location was used to store
 * a pointer to the next element in the linked list.
 *
 * @param lifo LIFO from which to receive.
 *
 * @return Pointer to first element in the list if available, otherwise NULL.
 */
extern void *nano_task_lifo_get(struct nano_lifo *lifo);

/**
 * @brief Get the first element from a LIFO, poll if empty.
 *
 * Remove the first element from the specified nanokernel linked list LIFO; it
 * can only be called from a task.
 *
 * If no elements are available, the calling task will poll until an element is
 * put onto the list.
 *
 * The first word in the element contains invalid data because that memory
 * location was used to store a pointer to the next element in the linked list.
 *
 * @param lifo LIFO from which to receive.
 *
 * @sa nano_task_stack_pop_wait()
 *
 * @return Pointer to first element in the list
 */
extern void *nano_task_lifo_get_wait(struct nano_lifo *lifo);

#ifdef CONFIG_NANO_TIMEOUTS

/**
 * @brief get the first element from a lifo, poll if empty.
 *
 * Remove the first element from the specified nanokernel linked list lifo; it
 * can only be called from a task.
 *
 * If no elements are available, the calling task will poll until an element is
 * put onto the list, or the timeout expires, whichever comes first.
 *
 * The first word in the element contains invalid data because that memory
 * location was used to store a pointer to the next element in the linked list.
 *
 * @param lifo LIFO on which to operate
 * @param timeout_in_ticks time to wait in ticks
 *
 * @return Pointer to first element in the list, NULL if timed out.
 */
extern void *nano_task_lifo_get_wait_timeout(struct nano_lifo *lifo,
		int32_t timeout_in_ticks);

#endif

/**
 * @}
 * @brief Nanokernel Semaphores
 * @defgroup nanokernel_semaphore Nanokernel Semaphores
 * @ingroup nanokernel_services
 * @{
 */

struct nano_sem {
	struct _nano_queue wait_q;
	int nsig;
#ifdef CONFIG_DEBUG_TRACING_KERNEL_OBJECTS
	struct nano_sem *next;
#endif
};

/**
 *
 * @brief Initialize a nanokernel semaphore object
 *
 * This function initializes a nanokernel semaphore object structure. After
 * initialization, the semaphore count will be 0.
 *
 * It may be called from either a fiber or task.
 *
 * @param sem Pointer to a nano_sem structure.
 *
 * @return N/A
 */
extern void nano_sem_init(struct nano_sem *sem);

/* execution context-independent methods (when context is not known) */

/**
 *
 * @brief Give a nanokernel semaphore
 *
 * This is a convenience wrapper for the execution context-specific APIs. This
 * is helpful whenever the exact execution context is not known, but should be
 * avoided when the context is known up-front (to avoid unnecessary overhead).
 *
 * @param sem Pointer to a nano_sem structure.
 *
 * @return N/A
 */
extern void nano_sem_give(struct nano_sem *sem);

/**
 *
 * @brief Take a nanokernel semaphore
 *
 * This is a convenience wrapper for the execution context-specific APIs. This
 * is helpful whenever the exact execution context is not known, but should be
 * avoided when the context is known up-front (to avoid unnecessary overhead).
 *
 * @param sem Pointer to a nano_sem structure.
 *
 * @return N/A
 */
extern int nano_sem_take(struct nano_sem *sem);

/**
 *
 * @brief Take a nanokernel semaphore, poll/pend if not available
 *
 * This is a convenience wrapper for the execution context-specific APIs. This
 * is helpful whenever the exact execution context is not known, but should be
 * avoided when the context is known up-front (to avoid unnecessary overhead).
 *
 * @warning It's only valid to call this API from a fiber or a task.
 *
 * @param sem Pointer to a nano_sem structure.
 *
 * @return N/A
 */
extern void nano_sem_take_wait(struct nano_sem *sem);

/**
 *
 * @brief Take a nanokernel semaphore, poll/pend with timeout if not available
 *
 * This is a convenience wrapper for the execution context-specific APIs. This
 * is helpful whenever the exact execution context is not known, but should be
 * avoided when the context is known up-front (to avoid unnecessary overhead).
 *
 * @warning It's only valid to call this API from a fiber or a task.
 *
 * @param sem Pointer to a nano_sem structure.
 * @param timeout Time to wait in ticks
 *
 * @return N/A
 */
extern void nano_sem_take_wait_timeout(struct nano_sem *sem, int32_t timeout);

/* methods for ISRs */

/**
 *
 * @brief Give a nanokernel semaphore (no context switch)
 *
 * This routine performs a "give" operation on a nanokernel semaphore object;
 * it may be call from an ISR context.  A fiber pending on
 * the semaphore object will be made ready, but will NOT be scheduled to
 * execute.
 *
 * @param sem Pointer to a nano_sem structure.
 *
 * @return N/A
 */
extern void nano_isr_sem_give(struct nano_sem *sem);

/**
 *
 * @brief Take a nanokernel semaphore, fail if unavailable
 *
 * Attempt to take a nanokernel semaphore; it may be called from a
 * ISR context.
 *
 * If the semaphore is not available, this function returns immediately, i.e.
 * a wait (pend) operation will NOT be performed.
 *
 * @param sem Pointer to a nano_sem structure.
 *
 * @return 1 if semaphore is available, 0 otherwise
 */
extern int nano_isr_sem_take(struct nano_sem *sem);

/* methods for fibers */

/**
 *
 * @brief Give a nanokernel semaphore (no context switch)
 *
 * This routine performs a "give" operation on a nanokernel semaphore object;
 * it may be call from a fiber.  A fiber pending on
 * the semaphore object will be made ready, but will NOT be scheduled to
 * execute.
 *
 * @param sem Pointer to a nano_sem structure.
 *
 * @return N/A
 */
extern void nano_fiber_sem_give(struct nano_sem *sem);

/**
 *
 * @brief Take a nanokernel semaphore, fail if unavailable
 *
 * Attempt to take a nanokernel semaphore; it may be called from a fiber.
 *
 * If the semaphore is not available, this function returns immediately, i.e.
 * a wait (pend) operation will NOT be performed.
 *
 * @param sem Pointer to a nano_sem structure.
 *
 * @return 1 if semaphore is available, 0 otherwise
 */
extern int nano_fiber_sem_take(struct nano_sem *sem);

/**
 *
 * @brief Test a nanokernel semaphore, wait if unavailable
 *
 * Take a nanokernel semaphore; it can only be called from a fiber.
 *
 * If the nanokernel semaphore is not available, i.e. the event counter
 * is 0, the calling fiber will wait (pend) until the semaphore is
 * given (via nano_fiber_sem_give/nano_task_sem_give/nano_isr_sem_give).
 *
 * @param sem Pointer to a nano_sem structure.
 *
 * @return N/A
 */
extern void nano_fiber_sem_take_wait(struct nano_sem *sem);
#ifdef CONFIG_NANO_TIMEOUTS

/**
 * @brief test a nanokernel semaphore, wait with a timeout if unavailable
 *
 * Take a nanokernel semaphore; it can only be called from a fiber.
 *
 * If the nanokernel semaphore is not available, i.e. the event counter
 * is 0, the calling fiber will wait (pend) until the semaphore is
 * given (via nano_fiber_sem_give/nano_task_sem_give/nano_isr_sem_give). A
 * timeout can be specified.
 *
 * @param sem Pointer to the semaphore to take
 * @param timeout time to wait in ticks
 *
 * @return 1 if semaphore is available, 0 if timed out
 */
extern int nano_fiber_sem_take_wait_timeout(struct nano_sem *sem,
		int32_t timeout);
#endif

/* methods for tasks */

/**
 *
 * @brief Give a nanokernel semaphore
 *
 * This routine performs a "give" operation on a nanokernel semaphore object;
 * it can only be called from a task.  A fiber pending on the
 * semaphore object will be made ready, and will preempt the running task
 * immediately.
 *
 * @param sem Pointer to a nano_sem structure.
 *
 * @return N/A
 */
extern void nano_task_sem_give(struct nano_sem *sem);

/**
 *
 * @brief Take a nanokernel semaphore, fail if unavailable
 *
 * Attempt to take a nanokernel semaphore; it can only be called from a task.
 *
 * If the semaphore is not available, this function returns immediately, i.e.
 * a wait (pend) operation will NOT be performed.
 *
 * @param sem Pointer to a nano_sem structure.
 *
 * @return 1 if semaphore is available, 0 otherwise
 */
extern int nano_task_sem_take(struct nano_sem *sem);

/**
 *
 * @brief Take a nanokernel semaphore, poll if unavailable
 *
 * Take a nanokernel semaphore; it can only be called from a task.
 *
 * If the nanokernel semaphore is not available, i.e. the event counter
 * is 0, the calling task will poll until the semaphore is given
 * (via nano_fiber_sem_give/nano_task_sem_give/nano_isr_sem_give).
 *
 * @param sem Pointer to a nano_sem structure.
 *
 * @return N/A
 */
extern void nano_task_sem_take_wait(struct nano_sem *sem);
#ifdef CONFIG_NANO_TIMEOUTS

/**
 * @brief test a nanokernel semaphore, poll with a timeout if unavailable
 *
 * Take a nanokernel semaphore; it can only be called from a task.
 *
 * If the nanokernel semaphore is not available, i.e. the event counter is 0,
 * the calling task will poll until the semaphore is given (via
 * nano_fiber_sem_give/nano_task_sem_give/nano_isr_sem_give). A timeout can be
 * specified.
 *
 * @param sem the semaphore to take
 * @param timeout time to wait in ticks
 *
 * @return 1 if semaphore is available, 0 if timed out
 */
extern int nano_task_sem_take_wait_timeout(struct nano_sem *sem,
		int32_t timeout);
#endif

/**
 * @}
 * @brief Nanokernel Stacks
 * @defgroup nanokernel_stack Nanokernel Stacks
 * @ingroup nanokernel_services
 * @{
 */
struct nano_stack {
	nano_thread_id_t fiber;
	uint32_t *base;
	uint32_t *next;
};

/**
 *
 * @brief Initialize a nanokernel stack object
 *
 * This function initializes a nanokernel stack object structure.
 *
 * It may be called from either a fiber or a task.
 *
 * @return N/A
 *
 */
extern void nano_stack_init(struct nano_stack *stack, uint32_t *data);

/**
 *
 * @brief Push data onto a stack
 *
 * This is a convenience wrapper for the execution context-specific APIs. This
 * is helpful whenever the exact execution context is not known, but should be
 * avoided when the context is known up-front (to avoid unnecessary overhead).
 *
 * @param stack Stack on which to interact
 * @param data Data to push on stack
 *
 * @return N/A
 *
 */
extern void nano_stack_push(struct nano_stack *stack, uint32_t data);

/**
 *
 * @brief Pop data from a nanokernel stack
 *
 * This is a convenience wrapper for the execution context-specific APIs. This
 * is helpful whenever the exact execution context is not known, but should be
 * avoided when the context is known up-front (to avoid unnecessary overhead).
 *
 * @param stack Stack on which to interact
 * @param data Container for data to pop
 *
 * @return 1 if stack is not empty, 0 otherwise
 *
 */
extern int nano_stack_pop(struct nano_stack *stack, uint32_t *data);

/**
 *
 * @brief Pop data from a nanokernel stack, poll/pend if empty
 *
 * This is a convenience wrapper for the execution context-specific APIs. This
 * is helpful whenever the exact execution context is not known, but should be
 * avoided when the context is known up-front (to avoid unnecessary overhead).
 *
 * @param stack Stack on which to interact
 *
 * @return Popped data
 *
 */
extern uint32_t nano_stack_pop_wait(struct nano_stack *stack);

/* methods for ISRs */

/**
 *
 * @brief Push data onto a stack (no context switch)
 *
 * This routine pushes a data item onto a stack object; it may be called from
 * an ISR context.  A fiber pending on the stack object will be
 * made ready, but will NOT be scheduled to execute.
 *
 * @param stack Stack on which to interact
 * @param data Data to push on stack
 *
 * @return N/A
 *
 */
extern void nano_isr_stack_push(struct nano_stack *stack, uint32_t data);

/**
 *
 * @brief Pop data from a nanokernel stack
 *
 * Pop the first data word from a nanokernel stack object; it may be called
 * from an ISR context.
 *
 * If the stack is not empty, a data word is popped and copied to the provided
 * address @a data and a non-zero value is returned. If the stack is empty,
 * zero is returned.
 *
 * @param stack Stack on which to interact
 * @param data Container for data to pop
 * @return 1 if stack is not empty, 0 otherwise
 *
 */
extern int nano_isr_stack_pop(struct nano_stack *stack, uint32_t *data);
/* methods for fibers */

/**
 *
 * @brief Push data onto a stack (no context switch)
 *
 * This routine pushes a data item onto a stack object; it may be called from
 * a fiber context.  A fiber pending on the stack object will be
 * made ready, but will NOT be scheduled to execute.
 *
 * @param stack Stack on which to interact
 * @param data Data to push on stack
 *
 * @return N/A
 *
 */
extern void nano_fiber_stack_push(struct nano_stack *stack, uint32_t data);

/**
 *
 * @brief Pop data from a nanokernel stack
 *
 * Pop the first data word from a nanokernel stack object; it may be called
 * from a fiber context.
 *
 * If the stack is not empty, a data word is popped and copied to the provided
 * address @a data and a non-zero value is returned. If the stack is empty,
 * zero is returned.
 *
 * @param stack Stack on which to interact
 * @param data Container for data to pop
 *
 * @return 1 if stack is not empty, 0 otherwise
 *
 */
extern int nano_fiber_stack_pop(struct nano_stack *stack, uint32_t *data);

/**
 *
 * @brief Pop data from a nanokernel stack, wait if empty
 *
 * Pop the first data word from a nanokernel stack object; it can only be
 * called from a fiber.
 *
 * If data is not available the calling fiber will pend until data is pushed
 * onto the stack.
 *
 * @param stack Stack on which to interact
 *
 * @return the data popped from the stack
 *
 */
extern uint32_t nano_fiber_stack_pop_wait(struct nano_stack *stack);


/* methods for tasks */

/**
 *
 * @brief Push data onto a nanokernel stack
 *
 * This routine pushes a data item onto a stack object; it may be called only
 * from a task.  A fiber pending on the stack object will be
 * made ready, and will preempt the running task immediately.
 *
 * @param stack Stack on which to interact
 * @param data Data to push on stack
 *
 * @return N/A
 */
extern void nano_task_stack_push(struct nano_stack *stack, uint32_t data);

/**
 *
 * @brief Pop data from a nanokernel stack
 *
 * Pop the first data word from a nanokernel stack object; it may be called
 * from a task context.
 *
 * If the stack is not empty, a data word is popped and copied to the provided
 * address @a data and a non-zero value is returned. If the stack is empty,
 * zero is returned.
 *
 * @param stack Stack on which to interact
 * @param data Container for data to pop
 *
 * @return 1 if stack is not empty, 0 otherwise
 */
extern int nano_task_stack_pop(struct nano_stack *stack, uint32_t *data);

/**
 *
 * @brief Pop data from a nanokernel stack, poll if empty
 *
 * Pop the first data word from a nanokernel stack; it can only be called
 * from a task.
 *
 * If data is not available the calling task will poll until data is pushed
 * onto the stack.
 *
 * @param stack Stack on which to interact
 *
 * @return the data popped from the stack
 */
extern uint32_t nano_task_stack_pop_wait(struct nano_stack *stack);

/* thread custom data APIs */
#ifdef CONFIG_THREAD_CUSTOM_DATA
extern void sys_thread_custom_data_set(void *value);
extern void *sys_thread_custom_data_get(void);
#endif /* CONFIG_THREAD_CUSTOM_DATA */

/**
 * @}
 * @brief Nanokernel Timers
 * @defgroup nanokernel_timer Nanokernel Timers
 * @ingroup nanokernel_services
 * @{
 */

struct nano_timer {
	struct nano_timer *link;
	uint32_t ticks;
	struct nano_lifo lifo;
	void *userData;
#ifdef CONFIG_DEBUG_TRACING_KERNEL_OBJECTS
	struct nano_timer *next;
#endif
};

/**
 * @brief Initialize a nanokernel timer object
 *
 * This function initializes a nanokernel timer object structure.
 *
 * It may be called from either a fiber or task.
 *
 * The @a data  passed to this function must have enough space for a pointer
 * in its first field, that may be overwritten when the timer expires, plus
 * whatever data the user wishes to store and recover when the timer expires.
 *
 * @param timer Timer
 * @param data User Data
 * @return N/A
 */
extern void nano_timer_init(struct nano_timer *timer, void *data);

/**
 *
 * @brief Start a nanokernel timer
 *
 * This is a convenience wrapper for the execution context-specific APIs. This
 * is helpful whenever the exact execution context is not known, but should be
 * avoided when the context is known up-front (to avoid unnecessary overhead).
 *
 * @param timer Timer
 * @param ticks Number of ticks
 *
 * @return N/A
 *
 */
extern void nano_timer_start(struct nano_timer *timer, int ticks);

/**
 * @brief Stop a nanokernel timer
 *
 * This is a convenience wrapper for the execution context-specific APIs. This
 * is helpful whenever the exact execution context is not known, but should be
 * avoided when the context is known up-front (to avoid unnecessary overhead).
 *
 * @param timer Timer to stop
 *
 * @return N/A
 */
extern void nano_timer_stop(struct nano_timer *timer);

/**
 * @brief Check for a timer expiry
 *
 * This is a convenience wrapper for the execution context-specific APIs. This
 * is helpful whenever the exact execution context is not known, but should be
 * avoided when the context is known up-front (to avoid unnecessary overhead).
 *
 * @param timer Timer to check
 *
 * @return pointer to timer initialization data, or NULL if timer not expired
 */
extern void *nano_timer_test(struct nano_timer *timer);

/* methods for ISRs */

/**
 *
 * @brief Start a nanokernel timer from an ISR
 *
 * This function starts a previously initialized nanokernel timer object.
 * The timer will expire in @a ticks system clock ticks.
 *
 * @param timer Timer
 * @param ticks Number of ticks
 *
 * @return N/A
 */
extern void nano_isr_timer_start(struct nano_timer *timer, int ticks);

/**
 * @brief Make the current ISR check for a timer expiry
 *
 * This function checks if a previously started nanokernel timer object has
 * expired.
 *
 * @param timer Timer to check
 *
 * @return pointer to timer initialization data, or NULL if timer not expired
 */
extern void *nano_isr_timer_test(struct nano_timer *timer);

/**
 * @brief Stop a nanokernel timer from an ISR
 *
 * This function stops a previously started nanokernel timer object.
 *
 * @param timer Timer to stop
 *
 * @return N/A
 */
extern void nano_isr_timer_stop(struct nano_timer *timer);

/* methods for fibers */

/**
 *
 * @brief Start a nanokernel timer from a fiber
 *
 * This function starts a previously initialized nanokernel timer object.
 * The timer will expire in @a ticks system clock ticks.
 *
 * @param timer Timer
 * @param ticks Number of ticks
 *
 * @return N/A
 */
extern void nano_fiber_timer_start(struct nano_timer *timer, int ticks);

/**
 * @brief Make the current fiber check for a timer expiry
 *
 * This function checks if a previously started nanokernel timer object has
 * expired.
 *
 * @param timer Timer to check
 *
 * @return pointer to timer initialization data, or NULL if timer not expired
 */
extern void *nano_fiber_timer_test(struct nano_timer *timer);

/**
 *
 * @brief Make the current fiber wait for a timer to expire
 *
 * This function pends on a previously started nanokernel timer object.
 * It assumes the timer is active and has neither expired nor been stopped.
 *
 * @param timer Timer to pend on
 *
 * @return pointer to timer initialization data
 *
 */
extern void *nano_fiber_timer_wait(struct nano_timer *timer);

/**
 * @brief Stop a nanokernel timer from a fiber
 *
 * This function stops a previously started nanokernel timer object.
 *
 * @param timer Timer to stop
 *
 * @return N/A
 */
extern void nano_fiber_timer_stop(struct nano_timer *timer);

/* methods for tasks */

/**
 * @brief Start a nanokernel timer from a task
 *
 * This function starts a previously initialized nanokernel timer object.
 * The timer will expire in @a ticks system clock ticks.
 *
 * @param timer Timer
 * @param ticks Number of ticks
 *
 * @return N/A
 */
extern void nano_task_timer_start(struct nano_timer *timer, int ticks);

/**
 * @brief Make the current task check for a timer expiry
 *
 * This function checks if a previously started nanokernel timer object has
 * expired.
 *
 * @param timer Timer to check
 *
 * @return pointer to timer initialization data, or NULL if timer not expired
 */
extern void *nano_task_timer_test(struct nano_timer *timer);

/**
 *
 * @brief Make the current task wait for a timer to expire
 *
 * This function pends on a previously started nanokernel timer object.
 * It assumes the timer is active and has neither expired nor been stopped.
 *
 * @param timer Timer to pend on
 *
 * @return pointer to timer initialization data
 *
 */
extern void *nano_task_timer_wait(struct nano_timer *timer);

/**
 * @brief Stop a nanokernel timer from a task
 *
 * This function stops a previously started nanokernel timer object.
 *
 * @param timer Timer to stop
 *
 * @return N/A
 */
extern void nano_task_timer_stop(struct nano_timer *timer);

/* methods for tasks and fibers for handling time and ticks */

/**
 *
 * @brief Return the current system tick count
 *
 * @return the current system tick count
 *
 */
extern int64_t sys_tick_get(void);

/**
 *
 * @brief Return the lower part of the current system tick count
 *
 * @return the current system tick count
 *
 */
extern uint32_t sys_tick_get_32(void);

/**
 * @brief Return a high resolution time stamp
 *
 * This routine reads the counter register on the processor's high precision
 * timer device. This counter register increments at a relatively high rate
 * (e.g. 20 MHz), and is thus considered a "high resolution" timer. This is
 * in contrast to sys_tick_get_32() which returns the value of the system
 * ticks variable.
 *
 * @return the current high precision clock value
 */
extern uint32_t sys_cycle_get_32(void);

/**
 *
 * @brief Return number of ticks since a reference time
 *
 * @param reftime Reference time
 *
 * @return tick count since reference time; undefined for first invocation
 */
extern int64_t sys_tick_delta(int64_t *reftime);

/**
 *
 * @brief Return 32-bit number of ticks since a reference time
 *
 * @param reftime Reference time
 *
 * @return 32-bit tick count since reference time; undefined for first invocation
 */
extern uint32_t sys_tick_delta_32(int64_t *reftime);


/*
 * Lists for object tracing
 */
#ifdef CONFIG_DEBUG_TRACING_KERNEL_OBJECTS

struct nano_sem *_track_list_nano_sem;

struct nano_fifo *_track_list_nano_fifo;

struct nano_lifo *_track_list_nano_lifo;

struct nano_timer *_track_list_nano_timer;

#define DEBUG_TRACING_OBJ_INIT(type, obj, list) { \
	obj->next = NULL; \
	if (list == NULL) { \
		list = obj; \
	} \
	else { \
		if (list != obj) { \
			type link = list; \
			while ((link->next != NULL) && (link->next != obj)) { \
				link = link->next; \
			} \
			if (link->next == NULL) { \
				link->next = obj; \
			} \
		} \
	} \
}
#else
#define DEBUG_TRACING_OBJ_INIT(type, obj, list) do { } while ((0))
#endif

/**
 * @}
 * @} nanokernel services
 */

#ifdef __cplusplus
}
#endif

/* architecture-specific nanokernel public APIs */

#include <arch/cpu.h>

#endif /* __NANOKERNEL_H__ */
