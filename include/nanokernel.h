/*
 * Copyright (c) 1997-2015, Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
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

struct tcs;

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
	struct tcs *tcs;
	struct _nano_queue *wait_q;
	int32_t delta_ticks_from_prev;
};
/**
 * @endcond
 */

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

/**
 * @brief Execution contexts APIs
 * @defgroup execution_contexts Execution Contexts
 * @ingroup nanokernel_services
 * @{
 */

/**
 * @brief Return the ID of the currently executing thread.
 *
 * This routine returns a pointer to the thread control block of the currently
 * executing thread. It is cast to a nano_thread_id_t for public use.
 *
 * @return The ID of the currently executing thread.
 */
extern nano_thread_id_t sys_thread_self_get(void);

/**
 *
 * @brief Return the type of the current execution context.
 *
 * This routine returns the type of execution context currently executing.
 *
 * @return The type of the current execution context.
 * @retval NANO_CTX_ISR (0): executing an interrupt service routine.
 * @retval NANO_CTX_FIBER (1): current thread is a fiber.
 * @retval NANO_CTX_TASK (2): current thread is a task.
 *
 */
extern nano_context_type_t sys_execution_context_type_get(void);

extern int _is_thread_essential(void);

/**
 *
 * @brief Cause the currently executing thread to busy wait.
 *
 * This routine causes the current task or fiber to execute a "do nothing"
 * loop for a specified period of time.
 *
 * @warning This routine utilizes the system clock, so it must not be invoked
 * until the system clock is fully operational or while interrupts
 * are locked.
 *
 * @param usec_to_wait Number of microseconds to busy wait.
 *
 * @return N/A
 */
extern void sys_thread_busy_wait(uint32_t usec_to_wait);

/**
 * @}
 */

/**
 * @brief Nanokernel Fibers
 * @defgroup nanokernel_fiber Nanokernel Fibers
 * @ingroup nanokernel_services
 * @{
 */

/* Execution context-independent methods. (When context is not known.) */

/**
 * @brief Initialize and start a fiber.
 *
 * This routine initializes and starts a fiber. It can be called from
 * either a fiber or a task. When this routine is called from a
 * task, the newly created fiber will start executing immediately.
 *
 * @internal
 * Given that this routine is _not_ ISR-callable, the following code is used
 * to differentiate between a task and fiber:
 *
 * if ((_nanokernel.current->flags & TASK) == TASK)
 *
 * Given that the _fiber_start() primitive is not considered real-time
 * performance critical, a runtime check to differentiate between a calling
 * task or fiber is performed to conserve footprint.
 * @endinternal
 *
 * @param stack Pointer to the stack space.
 * @param stack_size Stack size in bytes.
 * @param entry Fiber entry.
 * @param arg1 1st entry point parameter.
 * @param arg2 2nd entry point parameter.
 * @param prio The fiber's priority.
 * @param options Not used currently.
 * @return nanokernel thread identifier
 */
extern nano_thread_id_t fiber_start(char *stack, unsigned stack_size,
		nano_fiber_entry_t entry,
		int arg1, int arg2, unsigned prio, unsigned options);

/* Methods for fibers */

/**
 * @brief Initialize and start a fiber from a fiber.
 *
 * This routine initializes and starts a fiber. It can only be
 * called from a fiber.
 *
 * @param pStack Pointer to the stack space.
 * @param stackSize Stack size in bytes.
 * @param entry Fiber entry.
 * @param arg1 1st entry point parameter.
 * @param arg2 2nd entry point parameter.
 * @param prio The fiber's priority.
 * @param options Not used currently.
 * @return nanokernel thread identifier
 */
extern nano_thread_id_t fiber_fiber_start(char *pStack, unsigned int stackSize,
		nano_fiber_entry_t entry, int arg1, int arg2, unsigned prio,
		unsigned options);

/**
 * @brief Yield the current fiber.
 *
 * Calling this routine results in the current fiber yielding to
 * another fiber of the same or higher priority. If there are no
 * other runnable fibers of the same or higher priority, the
 * routine will return immediately.
 *
 * This routine can only be called from a fiber.
 *
 * @return N/A
 */
extern void fiber_yield(void);


/**
 * @brief Abort the currently executing fiber.
 *
 * This routine aborts the currently executing fiber. An abort can occur
 * because of one of three reasons:
 * - The fiber has explicitly aborted itself by calling this routine.
 * - The fiber has implicitly aborted itself by returning from its entry point.
 * - The fiber has encountered a fatal exception.
 *
 * This routine can only be called from a fiber.
 *
 * @return N/A
 */
extern void fiber_abort(void);

/**
 * @brief Fiber configuration structure.
 *
 * Parameters such as stack size and fiber priority are often
 * user configurable.  This structure makes it simple to specify such a
 * configuration.
 */
struct fiber_config {
	char *stack;
	unsigned stack_size;
	unsigned prio;
};

/**
 * @brief Start a fiber based on a @ref fiber_config, from fiber context.
 */
static inline nano_thread_id_t
fiber_fiber_start_config(const struct fiber_config *config,
			 nano_fiber_entry_t entry,
			 int arg1, int arg2, unsigned options)
{
	return fiber_fiber_start(config->stack, config->stack_size,
				 entry, arg1, arg2, config->prio, options);
}

/**
 * @brief Start a fiber based on a @ref fiber_config.
 *
 * This routine can be called from either a fiber or a task.
 */
static inline nano_thread_id_t
fiber_start_config(const struct fiber_config *config,
		   nano_fiber_entry_t entry,
		   int arg1, int arg2, unsigned options)
{
	return fiber_start(config->stack, config->stack_size, entry,
			   arg1, arg2, config->prio, options);
}

#ifdef CONFIG_NANO_TIMEOUTS

/**
 * @brief Put the current fiber to sleep.
 *
 * This routine puts the currently running fiber to sleep
 * for the number of system ticks passed in the
 * @a timeout_in_ticks parameter.
 *
 * @param timeout_in_ticks Number of system ticks the fiber sleeps.
 *
 * @return N/A
 */
extern void fiber_sleep(int32_t timeout_in_ticks);

/**
 * @brief Wake the specified fiber from sleep
 *
 * This routine wakes the fiber specified by @a fiber from its sleep.
 * It may only be called from an ISR.
 *
 * @param fiber Identifies fiber to wake
 *
 * @return N/A
 */
extern void isr_fiber_wakeup(nano_thread_id_t fiber);

/**
 * @brief Wake the specified fiber from sleep
 *
 * This routine wakes the fiber specified by @a fiber from its sleep.
 * It may only be called from a fiber.
 *
 * @param fiber Identifies fiber to wake
 *
 * @return N/A
 */
extern void fiber_fiber_wakeup(nano_thread_id_t fiber);

/**
 * @brief Wake the specified fiber from sleep
 *
 * This routine wakes the fiber specified by @a fiber from its sleep.
 * It may only be called from a task.
 *
 * @param fiber Identifies fiber to wake
 *
 * @return N/A
 */
extern void task_fiber_wakeup(nano_thread_id_t fiber);

/**
 * @brief Wake the specified fiber from sleep
 *
 * This routine is a convenience wrapper for the execution of context-specific
 * APIs. It is helpful when the exact execution context is not known. However,
 * it should be avoided when the context is known up-front to avoid
 * unnecessary overhead.
 *
 * @param fiber Identifies fiber to wake
 *
 * @return N/A
 */
extern void fiber_wakeup(nano_thread_id_t fiber);

#ifndef CONFIG_MICROKERNEL
/**
 * @brief Put the task to sleep.
 *
 * This routine puts the currently running task to sleep for the number
 * of system ticks passed in the @a timeout_in_ticks parameter.
 *
 * @param timeout_in_ticks Number of system ticks the task sleeps.
 *
 * @warning A value of TICKS_UNLIMITED is considered invalid and may result in
 * unexpected behavior.
 *
 * @return N/A
 * @sa TICKS_UNLIMITED
 */
extern void task_sleep(int32_t timeout_in_ticks);
#endif

/**
 * @brief Start a fiber while delaying its execution.
 *
 * This routine can only be called from a fiber.
 *
 * @param stack Pointer to the stack space.
 * @param stack_size_in_bytes Stack size in bytes.
 * @param entry_point The fiber's entry point.
 * @param param1 1st entry point parameter.
 * @param param2 2nd entry point parameter.
 * @param priority The fiber's priority.
 * @param options Not used currently.
 * @param timeout_in_ticks Timeout duration in ticks.
 *
 * @return A handle potentially used to cancel the delayed start.
 */
extern nano_thread_id_t fiber_fiber_delayed_start(char *stack,
		unsigned int stack_size_in_bytes,
		nano_fiber_entry_t entry_point, int param1,
		int param2, unsigned int priority,
		unsigned int options, int32_t timeout_in_ticks);

extern nano_thread_id_t fiber_delayed_start(char *stack,
		unsigned int stack_size_in_bytes,
		nano_fiber_entry_t entry_point, int param1,
		int param2, unsigned int priority,
		unsigned int options, int32_t timeout_in_ticks);
extern void fiber_delayed_start_cancel(nano_thread_id_t handle);

/**
 * @brief Cancel a delayed fiber start.
 *
 * @param handle The handle returned when starting the delayed fiber.
 *
 * @return N/A
 * @sa fiber_fiber_delayed_start
 */
extern void fiber_fiber_delayed_start_cancel(nano_thread_id_t handle);
#endif

/**
 * @}
*/

/**
 * @brief Nanokernel Task
 * @defgroup nanokernel_task Nanokernel Task
 * @ingroup nanokernel_services
 * @{
 */

/* methods for tasks */

/**
 * @brief Initialize and start a fiber from a task.
 *
 * @sa fiber_fiber_start
 */
extern nano_thread_id_t task_fiber_start(char *pStack, unsigned int stackSize,
		nano_fiber_entry_t entry, int arg1, int arg2, unsigned prio,
		unsigned options);

/**
 * @brief Start a fiber based on a @ref fiber_config, from task context.
 */
static inline nano_thread_id_t
task_fiber_start_config(const struct fiber_config *config,
			nano_fiber_entry_t entry,
			int arg1, int arg2, unsigned options)
{
	return task_fiber_start(config->stack, config->stack_size,
				entry, arg1, arg2, config->prio, options);
}

#ifdef CONFIG_NANO_TIMEOUTS

/**
 * @brief Start a fiber from a task while delaying its execution.
 *
 * @sa fiber_fiber_delayed_start
 */
extern nano_thread_id_t task_fiber_delayed_start(char *stack,
		unsigned int stack_size_in_bytes,
		nano_fiber_entry_t entry_point, int param1,
		int param2, unsigned int priority,
		unsigned int options, int32_t timeout_in_ticks);

/**
 * @brief Cancel a delayed fiber start from a task.
 *
 * @sa fiber_fiber_delayed_start_cancel
 */
extern void task_fiber_delayed_start_cancel(nano_thread_id_t handle);
#endif

/**
 * @}
 */

/**
 * @brief Nanokernel FIFOs
 * @defgroup nanokernel_fifo Nanokernel FIFO
 * @ingroup nanokernel_services
 * @{
 */
struct nano_fifo {
	struct _nano_queue wait_q;          /* waiting fibers */
	struct _nano_queue data_q;
#ifdef CONFIG_MICROKERNEL
	struct _nano_queue task_q;          /* waiting tasks */
#endif
#ifdef CONFIG_DEBUG_TRACING_KERNEL_OBJECTS
	struct nano_fifo *__next;
#endif
};

/**
 *
 * @brief Initialize a nanokernel FIFO (fifo) object.
 *
 * This function initializes a nanokernel FIFO (fifo) object
 * structure.
 *
 * It can be called from either a fiber or task.
 *
 * @param fifo FIFO to initialize.
 *
 * @return N/A
 */
extern void nano_fifo_init(struct nano_fifo *fifo);
/* execution context-independent methods (when context is not known) */

/**
 *
 * @brief Add an element to the end of a FIFO.
 *
 * This routine is a convenience wrapper for the execution of context-specific
 * APIs. It is helpful when the exact execution context is not known. However,
 * it should be avoided when the context is known up-front to avoid unnecessary
 * overhead.
 *
 * @param fifo FIFO on which to interact.
 * @param data Data to send.
 *
 * @return N/A
 */
extern void nano_fifo_put(struct nano_fifo *fifo, void *data);

/**
 *
 * @brief Get an element from the head a FIFO.
 *
 * This routine is a convenience wrapper for the execution of context-specific
 * APIs. It is helpful when the exact execution context is not known. However,
 * it should be avoided when the context is known up-front to avoid unnecessary
 * overhead.
 *
 * If no element is available, the function returns NULL. The first word in
 * the element contains invalid data because its memory location was used to
 * store a pointer to the next element in the linked list.
 *
 * @param fifo FIFO on which to interact.
 * @param timeout_in_ticks Affects the action taken should the FIFO be empty.
 * If TICKS_NONE, then return immediately. If TICKS_UNLIMITED, then wait as
 * long as necessary. Otherwise, wait up to the specified number of ticks
 * before timing out.
 *
 * @warning If it is to be called from the context of an ISR, then @a
 * timeout_in_ticks must be set to TICKS_NONE.
 *
 * @return Pointer to head element in the list when available.
 *         NULL Otherwise.
 *
 * @sa TICKS_NONE, TICKS_UNLIMITED
 */
extern void *nano_fifo_get(struct nano_fifo *fifo, int32_t timeout_in_ticks);

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
 * Remove the head element from the specified nanokernel FIFO
 * linked list FIFO. It can only be called from an ISR context.
 *
 * The first word in the element contains invalid data because its memory
 * location was used to store a pointer to the next element in the linked list.
 *
 * @param fifo FIFO on which to interact.
 * @param timeout_in_ticks Always use TICKS_NONE.
 *
 * @return Pointer to head element in the list when available.
 *         NULL Otherwise.
 */
extern void *nano_isr_fifo_get(struct nano_fifo *fifo, int32_t timeout_in_ticks);

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
 * Remove the head element from the specified nanokernel FIFO
 * linked list. It can only be called from a fiber.
 *
 * The first word in the element contains invalid data because its memory
 * location was used to store a pointer to the next element in the linked list.
 *
 * @param fifo FIFO on which to interact.
 * @param timeout_in_ticks Affects the action taken should the FIFO be empty.
 * If TICKS_NONE, then return immediately. If TICKS_UNLIMITED, then wait as
 * long as necessary. Otherwise, wait up to the specified number of ticks
 * before timing out.
 *
 * @return Pointer to head element in the list when available.
 *         NULL Otherwise.
 *
 * @sa TICKS_NONE, TICKS_UNLIMITED
 */
extern void *nano_fiber_fifo_get(struct nano_fifo *fifo,
			int32_t timeout_in_ticks);

/* Methods for tasks */

/**
 *
 * @brief Add an element to the end of a FIFO.
 *
 * This routine adds an element to the end of a FIFO object. It can only be
 * called from a task. If a fiber is pending on the FIFO object, it becomes
 * ready and will preempt the running task immediately.
 *
 * If a fiber is waiting on the FIFO, the address of the element is returned
 * to the waiting fiber. Otherwise, the element is linked to the end of the
 * list.
 *
 * @param fifo FIFO on which to interact.
 * @param data Data to send.
 *
 * @return N/A
 */
extern void nano_task_fifo_put(struct nano_fifo *fifo, void *data);

/**
 * @brief Get an element from a FIFO's head that comes from a task, poll if
 * empty.
 *
 * Removes the head element from the specified nanokernel FIFO
 * linked list. It can only be called from a task.
 *
 * The first word in the element contains invalid data because its memory
 * location was used to store a pointer to the next element in the linked
 * list.
 *
 * @param fifo FIFO on which to interact.
 * @param timeout_in_ticks Affects the action taken should the FIFO be empty.
 * If TICKS_NONE, then return immediately. If TICKS_UNLIMITED, then poll as
 * long as necessary. Otherwise poll up to the specified number of ticks have
 * elapsed before timing out.
 *
 * @return Pointer to head element in the list when available.
 *         NULL Otherwise.
 *
 * @sa TICKS_NONE, TICKS_UNLIMITED
 */
extern void *nano_task_fifo_get(struct nano_fifo *fifo,
			int32_t timeout_in_ticks);

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
#ifdef CONFIG_MICROKERNEL
	struct _nano_queue task_q;          /* waiting tasks */
#endif
#ifdef CONFIG_DEBUG_TRACING_KERNEL_OBJECTS
	struct nano_lifo *__next;
#endif
};

/**
 * @brief Initialize a nanokernel linked list LIFO (lifo) object.
 *
 * This function initializes a nanokernel system-level linked list LIFO
 * (lifo) object structure.
 *
 * It is called from either a fiber or task.
 *
 * @param lifo LIFO to initialize.
 *
 * @return N/A
 */
extern void nano_lifo_init(struct nano_lifo *lifo);

/**
 * @brief Prepend an element to a LIFO.
 *
 * This routine is a convenience wrapper for the execution of context-specific
 * APIs. It is helpful when the exact execution context is not known. However,
 * it should be avoided when the context is known up-front to avoid unnecessary
 * overhead.
 *
 * @param lifo LIFO on which to put.
 * @param data Data to insert.
 *
 * @return N/A
 */
extern void nano_lifo_put(struct nano_lifo *lifo, void *data);

/**
 * @brief Get the first element from a LIFO.
 *
 * This routine is a convenience wrapper for the execution of context-specific
 * APIs. It is helpful when the exact execution context is not known. However,
 * it should be avoided when the context is known up-front to avoid unnecessary
 * overhead.
 *
 * @param lifo LIFO on which to receive.
 * @param timeout_in_ticks Affects the action taken should the LIFO be empty.
 * If TICKS_NONE, then return immediately. If TICKS_UNLIMITED, then wait as
 * long as necesssary. Otherwise wait up to the specified number of ticks
 * before timing out.
 *
 * @warning If it is to be called from the context of an ISR, then @a
 * timeout_in_ticks must be set to TICKS_NONE.
 *
 * @return Pointer to head element in the list when available.
 *         NULL Otherwise.
 *
 * @sa TICKS_NONE, TICKS_UNLIMITED
 */
extern void *nano_lifo_get(struct nano_lifo *lifo, int32_t timeout_in_ticks);

/* methods for ISRs */

/**
 * @brief Prepend an element to a LIFO without a context switch.
 *
 * This routine adds an element to the LIFOs' object head; it may be
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
 * @brief Remove the first element from a LIFO linked list.
 *
 * Removes the first element from the specified nanokernel LIFO linked list;
 * it can only be called from an ISR context.
 *
 * If no elements are available, NULL is returned. The first word in the
 * element contains invalid data because its memory location was used to store
 * a pointer to the next element in the linked list.
 *
 * @param lifo LIFO from which to receive.
 * @param timeout_in_ticks Always use TICKS_NONE.
 *
 * @return Pointer to head element in the list when available.
 *         NULL Otherwise.
 *
 */
extern void *nano_isr_lifo_get(struct nano_lifo *lifo,
		int32_t timeout_in_ticks);

/* methods for fibers */

/**
 * @brief Prepend an element to a LIFO without a context switch.
 *
 * This routine adds an element to the LIFOs' object head; it can only be
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
 * @brief Remove the first element from a LIFO linked list.
 *
 * Removes the first element from the specified nanokernel LIFO linked list;
 * it can only be called from a fiber.
 *
 * If no elements are available, NULL is returned. The first word in the
 * element contains invalid data because its memory location was used to store
 * a pointer to the next element in the linked list.
 *
 * @param lifo LIFO from which to receive.
 * @param timeout_in_ticks Affects the action taken should the LIFO be empty.
 * If TICKS_NONE, then return immediately. If TICKS_UNLIMITED, then wait as
 * long as necessary. Otherwise wait up to the specified number of ticks
 * before timing out.
 *
 * @return Pointer to head element in the list when available.
 *         NULL Otherwise.
 *
 * @sa TICKS_NONE, TICKS_UNLIMITED
 */
extern void *nano_fiber_lifo_get(struct nano_lifo *lifo,
		int32_t timeout_in_ticks);

/* Methods for tasks */

/**
 * @brief Add an element to the LIFO's linked list head.
 *
 * This routine adds an element to the head of a LIFO object; it can only be
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
 * @brief Remove the first element from a LIFO linked list.
 *
 * Removes the first element from the specified nanokernel LIFO linked list;
 * it can only be called from a task.
 *
 * If no elements are available, NULL is returned. The first word in the
 * element contains invalid data because its memory location was used to store
 * a pointer to the next element in the linked list.
 *
 * @param lifo LIFO from which to receive.
 * @param timeout_in_ticks Affects the action taken should the LIFO be empty.
 * If TICKS_NONE, then return immediately. If TICKS_UNLIMITED, then wait as
 * long as necessary. Otherwise wait up to the specified number of ticks
 * before timing out.
 *
 * @return Pointer to head element in the list when available.
 *         NULL Otherwise.
 *
 * @sa TICKS_NONE, TICKS_UNLIMITED
 */
extern void *nano_task_lifo_get(struct nano_lifo *lifo,
		int32_t timeout_in_ticks);

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
#ifdef CONFIG_MICROKERNEL
	struct _nano_queue task_q;          /* waiting tasks */
#endif
#ifdef CONFIG_DEBUG_TRACING_KERNEL_OBJECTS
	struct nano_sem *__next;
#endif
};

/**
 *
 * @brief Initialize a nanokernel semaphore object.
 *
 * This function initializes a nanokernel semaphore object structure. After
 * initialization, the semaphore count is 0.
 *
 * It can be called from either a fiber or task.
 *
 * @param sem Pointer to a nano_sem structure.
 *
 * @return N/A
 */
extern void nano_sem_init(struct nano_sem *sem);

/* execution context-independent methods (when context is not known) */

/**
 *
 * @brief Give a nanokernel semaphore.
 *
 * This routine is a convenience wrapper for the execution of context-specific
 * APIs. It is helpful when the exact execution context is not known. However,
 * it should be avoided when the context is known up-front to avoid unnecessary
 * overhead.
 *
 * @param sem Pointer to a nano_sem structure.
 *
 * @return N/A
 */
extern void nano_sem_give(struct nano_sem *sem);

/**
 *
 * @brief Take a nanokernel semaphore, poll/pend if not available.
 *
 * This routine is a convenience wrapper for the execution of context-specific
 * APIs. It is helpful when the exact execution context is not known. However,
 * it should be avoided when the context is known up-front to avoid unnecessary
 * overhead.
 *
 * @param sem Pointer to a nano_sem structure.
 * @param timeout_in_ticks Determines the action to take when the semaphore is
 *        unavailable.
 *        For TICKS_NONE, return immediately.
 *        For TICKS_UNLIMITED, wait as long as necessary.
 *        Otherwise, wait up to the specified number of ticks before timing
 *        out.
 *
 * @warning If it is to be called from the context of an ISR, then @a
 * timeout_in_ticks must be set to TICKS_NONE.
 *
 * @retval 1 When semaphore is available
 * @retval 0 Otherwise
 * @sa TICKS_NONE, TICKS_UNLIMITED
 */
extern int nano_sem_take(struct nano_sem *sem, int32_t timeout_in_ticks);

/* methods for ISRs */

/**
 *
 * @brief Give a nanokernel semaphore (no context switch).
 *
 * This routine performs a "give" operation on a nanokernel semaphore object;
 * it can only be called from an ISR context. A fiber pending on
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
 * @brief Take a nanokernel semaphore, fail if unavailable.
 *
 * Attempts to take a nanokernel semaphore. It can only be called from a
 * ISR context.
 *
 * If the semaphore is not available, this function returns immediately, i.e.
 * a wait (pend) operation will NOT be performed.
 *
 * @param sem Pointer to a nano_sem structure.
 * @param timeout_in_ticks Always use TICKS_NONE.
 *
 * @retval 1 When semaphore is available
 * @retval 0 Otherwise
 */
extern int nano_isr_sem_take(struct nano_sem *sem, int32_t timeout_in_ticks);

/* methods for fibers */

/**
 *
 * @brief Give a nanokernel semaphore (no context switch).
 *
 * This routine performs a "give" operation on a nanokernel semaphore object;
 * it can only be called from a fiber. A fiber pending on the semaphore object
 * will be made ready, but will NOT be scheduled to execute.
 *
 * @param sem Pointer to a nano_sem structure.
 *
 * @return N/A
 */
extern void nano_fiber_sem_give(struct nano_sem *sem);

/**
 *
 * @brief Take a nanokernel semaphore, wait or fail if unavailable.
 *
 * Attempts to take a nanokernel semaphore. It can only be called from a fiber.
 *
 * @param sem Pointer to a nano_sem structure.
 * @param timeout_in_ticks Determines the action to take when the semaphore
 *        is unavailable.
 *        For TICKS_NONE, return immediately.
 *        For TICKS_UNLIMITED, wait as long as necessary.
 *        Otherwise, wait up to the specified number of ticks before timing
 *        out.
 *
 * @retval 1 When semaphore is available.
 * @retval 0 Otherwise.
 */
extern int nano_fiber_sem_take(struct nano_sem *sem, int32_t timeout_in_ticks);

/* methods for tasks */

/**
 *
 * @brief Give a nanokernel semaphore.
 *
 * This routine performs a "give" operation on a nanokernel semaphore object;
 * it can only be called from a task. A fiber pending on the
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
 * @brief Take a nanokernel semaphore, fail if unavailable.
 *
 * Attempts to take a nanokernel semaphore; it can only be called from a task.
 *
 * @param sem Pointer to a nano_sem structure.
 * @param timeout_in_ticks Determines the action to take when the semaphore
 *        is unavailable.
 *        For TICKS_NONE, return immediately.
 *        For TICKS_UNLIMITED, wait as long as necessary.
 *        Otherwise, wait up to the specified number of ticks before timing
 *        out.
 *
 * @retval 1 when the semaphore is available.
 * @retval 0 Otherwise.
 * @sa TICKS_NONE, TICKS_UNLIMITED
 */
extern int nano_task_sem_take(struct nano_sem *sem, int32_t timeout_in_ticks);

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
#ifdef CONFIG_DEBUG_TRACING_KERNEL_OBJECTS
	struct nano_stack *__next;
#endif
};

/**
 *
 * @brief Initialize a nanokernel stack object.
 *
 * This function initializes a nanokernel stack object structure.
 *
 * It is called from either a fiber or a task.
 *
 * @return N/A
 *
 */
extern void nano_stack_init(struct nano_stack *stack, uint32_t *data);

/**
 *
 * @brief Push data onto a stack.
 *
 * This routine is a convenience wrapper for the execution of context-specific APIs. It
 * is helpful when the exact execution context is not known. However, it
 * should be avoided when the context is known up-front to avoid unnecessary overhead.
 *
 * @param stack Stack on which to interact.
 * @param data Data to push on stack.
 *
 * @return N/A
 *
 */
extern void nano_stack_push(struct nano_stack *stack, uint32_t data);

/**
 *
 * @brief Pop data from a nanokernel stack.
 *
 * This routine is a convenience wrapper for the execution of context-specific
 * APIs. It is helpful when the exact execution context is not known. However,
 * it should be avoided when the context is known up-front to avoid unnecessary
 * overhead.
 *
 * @param stack Stack on which to interact.
 * @param data Container for data to pop.
 * @param timeout_in_ticks Determines the action to take when the FIFO
 *        is empty.
 *        For TICKS_NONE, return immediately.
 *        For TICKS_UNLIMITED, wait as long as necessary.
 *        Otherwise, wait up to the specified number of ticks before timing
 *        out.
 *
 * @retval 1 When data is popped from the stack.
 * @retval 0 Otherwise.
 *
 * @warning If called from the context of an ISR, then @a timeout_in_ticks
 *          must be TICKS_NONE.
 * @sa TICKS_NONE, TICKS_UNLIMITED
 */
extern int nano_stack_pop(struct nano_stack *stack, uint32_t *data,
			int32_t timeout_in_ticks);

/* methods for ISRs */

/**
 *
 * @brief Push data onto a stack (no context switch).
 *
 * This routine pushes a data item onto a stack object. It can only be called
 * from an ISR context. A fiber that pends on the stack object becomes ready
 * but will NOT be scheduled to execute.
 *
 * @param stack Stack on which to interact.
 * @param data Data to push on stack.
 *
 * @return N/A
 *
 */
extern void nano_isr_stack_push(struct nano_stack *stack, uint32_t data);

/**
 *
 * @brief Pop data from a nanokernel stack.
 *
 * Pops the first data word from a nanokernel stack object. It can only be
 * called from an ISR context.
 *
 * When the stack is not empty, a data word is popped and copied to the
 * provided address @a data and a non-zero value is returned. When the routine
 * finds an empty stack, zero is returned.
 *
 * @param stack Stack on which to interact.
 * @param data Container for data to pop.
 * @param timeout_in_ticks Must be TICKS_NONE.
 *
 * @retval 1 When data is popped from the stack
 * @retval 0 Otherwise.
 */
extern int nano_isr_stack_pop(struct nano_stack *stack, uint32_t *data,
			int32_t timeout_in_ticks);
/* methods for fibers */

/**
 *
 * @brief Push data onto a stack (no context switch).
 *
 * This routine pushes a data item onto a stack object. It can only be called
 * from a fiber context. A fiber that pends on the stack object becomes ready
 * but will NOT be scheduled to execute.
 *
 * @param stack Stack on which to interact.
 * @param data Data to push on stack.
 *
 * @return N/A
 *
 */
extern void nano_fiber_stack_push(struct nano_stack *stack, uint32_t data);

/**
 *
 * @brief Pop data from a nanokernel stack.
 *
 * Pops the first data word from a nanokernel stack object. It can only be called
 * from a fiber context.
 *
 * When the stack is not empty, a data word is popped and copied to the
 * provided address @a data and a non-zero value is returned. When the routine
 * finds an empty stack, zero is returned.
 *
 * @param stack Stack on which to interact.
 * @param data Container for data to pop.
 * @param timeout_in_ticks Determines the action to take when the FIFO
 *        is empty.
 *        For TICKS_NONE, return immediately.
 *        For TICKS_UNLIMITED, wait as long as necessary.
 *        Otherwise, wait up to the specified number of ticks before timing
 *        out.
 *
 * @retval 1 When data is popped from the stack
 * @retval 0 Otherwise.
 * @sa TICKS_NONE, TICKS_UNLIMITED
 */
extern int nano_fiber_stack_pop(struct nano_stack *stack, uint32_t *data, int32_t timeout_in_ticks);

/* Methods for tasks */

/**
 *
 * @brief Push data onto a nanokernel stack.
 *
 * This routine pushes a data item onto a stack object. It can only be called
 * from a task. A fiber that pends on the stack object becomes
 * ready and preempts the running task immediately.
 *
 * @param stack Stack on which to interact.
 * @param data Data to push on stack.
 *
 * @return N/A
 */
extern void nano_task_stack_push(struct nano_stack *stack, uint32_t data);

/**
 *
 * @brief Pop data from a nanokernel stack.
 *
 * Pops the first data word from a nanokernel stack object. It can only be called
 * from a task context.
 *
 * When the stack is not empty, a data word is popped and copied to the
 * provided address @a data and a non-zero value is returned. When the routine
 * finds an empty stack, zero is returned.
 *
 * @param stack Stack on which to interact.
 * @param data Container for data to pop.
 * @param timeout_in_ticks Determines the action to take when the FIFO
 *        is empty.
 *        For TICKS_NONE, return immediately.
 *        For TICKS_UNLIMITED, wait as long as necessary.
 *        Otherwise, wait up to the specified number of ticks before timing
 *        out.
 *
 * @retval 1 When data is popped from the stack
 * @retval 0 Otherwise.
 * @sa TICKS_NONE, TICKS_UNLIMITED
 */
extern int nano_task_stack_pop(struct nano_stack *stack, uint32_t *data, int32_t timeout_in_ticks);

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
	struct _nano_timeout timeout_data;
	void *user_data;
	/*
	 * User data pointer in backup for cases when nanokernel_timer_test()
	 * has to return NULL
	 */
	void *user_data_backup;
#ifdef CONFIG_DEBUG_TRACING_KERNEL_OBJECTS
	struct nano_timer *__next;
#endif
};

/**
 * @brief Initialize a nanokernel timer object.
 *
 * This function initializes a nanokernel timer object structure.
 *
 * It can be called from either a fiber or task.
 *
 * The @a data passed to this function is a pointer to a data structure defined by the user.
 * It contains data that the user wishes to store when initializing the timer and recover
 * when the timer expires. However, the first field of this data structure must be a pointer
 * reserved for the API's use that can be overwritten by the API and, as such,
 * should not contain user data.
 *
 *
 * @param timer Timer.
 * @param data User Data.
 * @return N/A
 */
extern void nano_timer_init(struct nano_timer *timer, void *data);

/**
 *
 * @brief Start a nanokernel timer.
 *
 * This routine is a convenience wrapper for the execution of context-specific
 * APIs. It is helpful when the exact execution context is not known. However,
 * it should be avoided when the context is known up-front to avoid unnecessary
 * overhead.
 *
 * @param timer Timer.
 * @param ticks Number of ticks.
 *
 * @return N/A
 *
 */
extern void nano_timer_start(struct nano_timer *timer, int ticks);

/**
 * @brief Wait for a nanokernel timer to expire.
 *
 * This routine is a convenience wrapper for the execution of context-specific
 * APIs. It is helpful when the exact execution context is not known. However,
 * it should be avoided when the context is known up-front to avoid unnecessary
 * overhead.
 *
 * @param timer Timer.
 * @param timeout_in_ticks Determines the action to take when the timer has
 *        not expired.
 *        For TICKS_NONE, return immediately.
 *        For TICKS_UNLIMITED, wait as long as necessary.
 *
 * @return N/A
 *
 * @warning If called from an ISR, then @a timeout_in_ticks must be TICKS_NONE.
 * @sa TICKS_NONE, TICKS_UNLIMITED
 */
extern void *nano_timer_test(struct nano_timer *timer, int32_t timeout_in_ticks);

/**
 * @brief Stop a nanokernel timer.
 *
 * This routine is a convenience wrapper for the execution of context-specific
 * APIs. It is helpful when the exact execution context is not known. However,
 * it should be avoided when the context is known up-front to avoid unnecessary
 * overhead.
 *
 * @param timer Timer to stop.
 *
 * @return pointer to timer initialization data.
 */
extern void nano_timer_stop(struct nano_timer *timer);

/* methods for ISRs */

/**
 *
 * @brief Start a nanokernel timer from an ISR.
 *
 * This function starts a previously initialized nanokernel timer object.
 * The timer will expire in @a ticks system clock ticks.
 *
 * @param timer Timer.
 * @param ticks Number of ticks.
 *
 * @return N/A
 */
extern void nano_isr_timer_start(struct nano_timer *timer, int ticks);

/**
 * @brief Make the current ISR check for a timer expiry.
 *
 * This function checks if a previously started nanokernel timer object has
 * expired.
 *
 * @param timer Timer to check.
 * @param timeout_in_ticks Always use TICKS_NONE.
 *
 * @return Pointer to timer initialization data.
 * @retval NULL If timer not expired.
 */
extern void *nano_isr_timer_test(struct nano_timer *timer, int32_t timeout_in_ticks);

/**
 * @brief Stop a nanokernel timer from an ISR.
 *
 * This function stops a previously started nanokernel timer object.
 *
 * @param timer Timer to stop.
 *
 * @return N/A
 */
extern void nano_isr_timer_stop(struct nano_timer *timer);

/* methods for fibers */

/**
 *
 * @brief Start a nanokernel timer from a fiber.
 *
 * This function starts a previously-initialized nanokernel timer object.
 * The timer expires after @a ticks system clock ticks.
 *
 * @param timer Timer.
 * @param ticks Number of ticks.
 *
 * @return N/A
 */
extern void nano_fiber_timer_start(struct nano_timer *timer, int ticks);

/**
 * @brief Make the current fiber check for a timer expiry.
 *
 * This function tests whether or not a previously started nanokernel timer
 * object has expired, or waits until it does.
 *
 * @param timer Timer to check.
 * @param timeout_in_ticks Determines the action to take when the timer has
 *        not expired.
 *        For TICKS_NONE, return immediately.
 *        For TICKS_UNLIMITED, wait as long as necessary.
 *
 * @return Pointer to timer initialization data
 * @retval NULL If timer has not expired.
 *
 * @sa TICKS_NONE, TICKS_UNLIMITED
 */
extern void *nano_fiber_timer_test(struct nano_timer *timer, int32_t timeout_in_ticks);

/**
 * @brief Stop a nanokernel timer.
 *
 * This function stops a previously started nanokernel timer object.
 * It can only be called from a fiber.
 *
 * @param timer Timer to stop.
 *
 * @return N/A
 */
extern void nano_fiber_timer_stop(struct nano_timer *timer);

/* methods for tasks */

/**
 * @brief Start a nanokernel timer from a task.
 *
 * This function starts a previously initialized nanokernel timer object.
 * The timer expires after @a ticks system clock ticks.
 *
 * @param timer Timer.
 * @param ticks Number of ticks.
 *
 * @return N/A
 */
extern void nano_task_timer_start(struct nano_timer *timer, int ticks);

/**
 * @brief Make the current task check for a timer expiry.
 *
 * This function tests whether or not a previously started nanokernel timer
 * object has expired, or waits until it does.
 *
 * @param timer Timer to check.
 * @param timeout_in_ticks Determines the action to take when the timer has
 *        not expired.
 *        For TICKS_NONE, return immediately.
 *        For TICKS_UNLIMITED, wait as long as necessary.
 *
 * @return Pointer to timer initialization data.
 * @retval NULL If timer has not expired.
 *
 * @sa TICKS_NONE, TICKS_UNLIMITED
 */
extern void *nano_task_timer_test(struct nano_timer *timer, int32_t timeout_in_ticks);

/**
 * @brief Stop a nanokernel timer from a task.
 *
 * This function stops a previously-started nanokernel timer object.
 *
 * @param timer Timer to stop.
 *
 * @return N/A
 */
extern void nano_task_timer_stop(struct nano_timer *timer);

/**
 * @brief Get nanokernel timer remaining ticks.
 *
 * This function returns the remaining ticks of the previously
 * started nanokernel timer object.
 *
 * @return remaining ticks or 0 if the timer has expired
 */
extern int32_t nano_timer_ticks_remain(struct nano_timer *timer);

/* Methods for tasks and fibers for handling time and ticks */

/**
 *
 * @brief Return the current system tick count.
 *
 * @return The current system tick count.
 *
 */
extern int64_t sys_tick_get(void);

/**
 *
 * @brief Return the lower part of the current system tick count.
 *
 * @return The current system tick count.
 *
 */
extern uint32_t sys_tick_get_32(void);

/**
 * @brief Return a time stamp in high-resolution format.
 *
 * This routine reads the counter register on the processor's high precision
 * timer device. This counter register increments at a relatively high rate
 * (e.g. 20 MHz), and is thus considered a high-resolution timer. This is
 * in contrast to sys_tick_get_32() which returns the value of the system
 * ticks variable.
 *
 * @return The current high-precision clock value.
 */
extern uint32_t sys_cycle_get_32(void);

/**
 *
 * @brief Return number of ticks elapsed since a reference time.
 *
 * @param reftime Reference time.
 *
 * @return The tick count since reference time; undefined for first invocation.
 */
extern int64_t sys_tick_delta(int64_t *reftime);

/**
 *
 * @brief Return 32-bit number of ticks since a reference time.
 *
 * @param reftime Reference time.
 *
 * @return A 32-bit tick count since reference time. Undefined for first invocation.
 */
extern uint32_t sys_tick_delta_32(int64_t *reftime);

/**
 * @}
 * @} nanokernel services
 */

#ifdef __cplusplus
}
#endif

#if defined(CONFIG_CPLUSPLUS) && defined(__cplusplus)
/*
 * Define new and delete operators.
 * At this moment, the operators do nothing since objects are supposed
 * to be statically allocated.
 */
inline void operator delete(void *ptr)
{
	(void)ptr;
}

inline void operator delete[](void *ptr)
{
	(void)ptr;
}

inline void *operator new(size_t size)
{
	(void)size;
	return NULL;
}

inline void *operator new[](size_t size)
{
	(void)size;
	return NULL;
}
#endif

/* architecture-specific nanokernel public APIs */

#include <arch/cpu.h>

#endif /* __NANOKERNEL_H__ */
