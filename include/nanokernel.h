/*
 * Copyright (c) 1997-2015, Wind River Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Wind River Systems nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 *
 * @brief Public APIs for the nanokernel.
 */

#ifndef __NANOKERNEL_H__
#define __NANOKERNEL_H__

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

struct ccs;

/* architecture-independent nanokernel public APIs */

typedef struct ccs *nano_context_id_t;

typedef void (*nano_fiber_entry_t)(int i1, int i2);

typedef int nano_context_type_t;

#define NANO_CTX_ISR (0)
#define NANO_CTX_FIBER (1)
#define NANO_CTX_TASK (2)

/* timeout special values */
#define TICKS_UNLIMITED (-1)
#define TICKS_NONE 0

/*
 * context APIs
 */
extern nano_context_id_t context_self_get(void);
extern nano_context_type_t context_type_get(void);
extern int _context_essential_check(nano_context_id_t pCtx);

/*
 * fiber APIs
 */
/* scheduling context independent method (when context is not known) */
void fiber_start(char *stack, unsigned stack_size, nano_fiber_entry_t entry,
		int arg1, int arg2, unsigned prio, unsigned options);

/* methods for fibers */
extern void fiber_fiber_start(char *pStack, unsigned int stackSize,
		nano_fiber_entry_t entry, int arg1, int arg2, unsigned prio,
		unsigned options);
extern void fiber_yield(void);
extern void fiber_abort(void);

#ifdef CONFIG_NANO_TIMEOUTS
extern void fiber_sleep(int32_t timeout);
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
extern void fiber_fiber_delayed_start_cancel(void *handle);
#endif

/* methods for tasks */
extern void task_fiber_start(char *pStack, unsigned int stackSize,
		nano_fiber_entry_t entry, int arg1, int arg2, unsigned prio,
		unsigned options);
#ifdef CONFIG_NANO_TIMEOUTS
extern void *task_fiber_delayed_start(char *stack,
		unsigned int stack_size_in_bytes,
		nano_fiber_entry_t entry_point, int param1,
		int param2, unsigned int priority,
		unsigned int options, int32_t timeout_in_ticks);
extern void task_fiber_delayed_start_cancel(void *handle);
#endif

/* FIFO APIs */

struct nano_fifo {
	union {
		struct _nano_queue wait_q;
		struct _nano_queue data_q;
	};
	int stat;
};

/**
 *
 * @brief Initialize a nanokernel multiple-waiter fifo (fifo) object
 *
 * This function initializes a nanokernel multiple-waiter fifo (fifo) object
 * structure.
 *
 * It may be called from either a fiber or task context.
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
 * @param nano_fifo FIFO to initialize.
 *
 * @return N/A
 */
extern void nano_fifo_init(struct nano_fifo *chan);
/* scheduling context independent methods (when context is not known) */

/**
 *
 * @brief Add an element to the end of a fifo
 *
 * This is a convenience wrapper for the context-specific APIs. This is
 * helpful whenever the exact scheduling context is not known, but should
 * be avoided when the context is known up-front (to avoid unnecessary
 * overhead).
 *
 * @param nano_fifo FIFO on which to interact.
 * @param data Data to send.
 *
 * @return N/A
 */
extern void nano_fifo_put(struct nano_fifo *chan, void *data);

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
 * @param nano_fifo FIFO on which to interact.
 *
 * @return Pointer to head element in the list if available, otherwise NULL
 */
extern void *nano_fifo_get(struct nano_fifo *chan);

/**
 *
 * @brief Get the head element of a fifo, poll/pend if empty
 *
 * This is a convenience wrapper for the context-specific APIs. This is
 * helpful whenever the exact scheduling context is not known, but should
 * be avoided when the context is known up-front (to avoid unnecessary
 * overhead).
 *
 * @warning It's only valid to call this API from a fiber or a task.
 *
 * @param nano_fifo FIFO on which to interact.
 *
 * @return Pointer to head element in the list
 */
extern void *nano_fifo_get_wait(struct nano_fifo *chan);

/* methods for ISRs */

/**
 *
 * @brief Add an element to the end of a FIFO from an ISR context.
 *
 * This is an alias for the context-specific API. This is
 * helpful whenever the exact scheduling context is known. Its use
 * avoids unnecessary overhead.
 *
 * @param nano_fifo FIFO on which to interact.
 * @param data Data to send.
 *
 * @return N/A
 */
extern void nano_isr_fifo_put(struct nano_fifo *chan, void *data);

/**
 * @brief Get an element from the head of a FIFO from an ISR context.
 *
 * Remove the head element from the specified nanokernel multiple-waiter fifo
 * linked list fifo. It may be called from an ISR context.
 *
 * The first word in the element contains invalid data because that memory
 * location was used to store a pointer to the next element in the linked list.
 *
 * @param nano_fifo FIFO on which to interact.
 *
 * @return Pointer to head element in the list if available, otherwise NULL
 */
extern void *nano_isr_fifo_get(struct nano_fifo *chan);

/* methods for fibers */

/**
 *
 * @brief Add an element to the end of a FIFO from a fiber context.
 *
 * This is an alias for the context-specific API. This is
 * helpful whenever the exact scheduling context is known. Its use
 * avoids unnecessary overhead.
 *
 * @param nano_fifo FIFO on which to interact.
 * @param data Data to send.
 *
 * @return N/A
 */
extern void nano_fiber_fifo_put(struct nano_fifo *chan, void *data);

/**
 * @brief Get an element from the head of a FIFO from a fiber context.
 *
 * Remove the head element from the specified nanokernel multiple-waiter fifo
 * linked list fifo. It may be called from a fiber context.
 *
 * The first word in the element contains invalid data because that memory
 * location was used to store a pointer to the next element in the linked list.
 *
 * @param nano_fifo FIFO on which to interact.
 *
 * @return Pointer to head element in the list if available, otherwise NULL
 */
extern void *nano_fiber_fifo_get(struct nano_fifo *chan);

/**
 *
 * @brief Get the head element of a fifo, wait if emtpy
 *
 * Remove the head element from the specified system-level multiple-waiter
 * fifo; it can only be called from a fiber context.
 *
 * If no elements are available, the calling fiber will pend until an element
 * is put onto the fifo.
 *
 * The first word in the element contains invalid data because that memory
 * location was used to store a pointer to the next element in the linked list.
 *
 * @param nano_fifo FIFO on which to interact.
 *
 * @return Pointer to head element in the list
 *
 * @note There exists a separate nano_task_fifo_get_wait() implementation
 * since a task context cannot pend on a nanokernel object. Instead tasks will
 * poll the fifo object.
 */
extern void *nano_fiber_fifo_get_wait(struct nano_fifo *chan);
#ifdef CONFIG_NANO_TIMEOUTS

/**
 * @brief get the head element of a fifo, pend with a timeout if empty
 *
 * Remove the head element from the specified nanokernel fifo; it can only be
 * called from a fiber context.
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
 * @param nano_fifo the FIFO on which to interact.
 * @param timeout_in_ticks time to wait in ticks
 *
 * @return Pointer to head element in the list, NULL if timed out
 */
extern void *nano_fiber_fifo_get_wait_timeout(struct nano_fifo *chan,
		int32_t timeout_in_ticks);
#endif

/* methods for tasks */

/**
 *
 * @brief Add an element to the end of a fifo
 *
 * This routine adds an element to the end of a fifo object; it can be called
 * from only a task context.  A fiber pending on the fifo object will be made
 * ready, and will preempt the running task immediately.
 *
 * If a fiber is waiting on the fifo, the address of the element is returned to
 * the waiting fiber.  Otherwise, the element is linked to the end of the list.
 *
 * @param nano_fifo FIFO on which to interact.
 * @param data Data to send.
 *
 * @return N/A
 */
extern void nano_task_fifo_put(struct nano_fifo *chan, void *data);

extern void *nano_task_fifo_get(struct nano_fifo *chan);

/**
 *
 * @brief Get the head element of a fifo, poll if empty
 *
 * Remove the head element from the specified system-level multiple-waiter
 * fifo; it can only be called from a task context.
 *
 * If no elements are available, the calling task will poll until an
 * until an element is put onto the fifo.
 *
 * The first word in the element contains invalid data because that memory
 * location was used to store a pointer to the next element in the linked list.
 *
 * @param nano_fifo FIFO on which to interact.
 *
 * @sa nano_task_stack_pop_wait()
 *
 * @return Pointer to head element in the list
 */
extern void *nano_task_fifo_get_wait(struct nano_fifo *chan);
#ifdef CONFIG_NANO_TIMEOUTS

/**
 * @brief get the head element of a fifo, poll with a timeout if empty
 *
 * Remove the head element from the specified nanokernel fifo; it can only be
 * called from a task context.
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
extern void *nano_task_fifo_get_wait_timeout(struct nano_fifo *chan,
		int32_t timeout_in_ticks);
#endif

/* LIFO APIs */

struct nano_lifo {
	struct _nano_queue wait_q;
	void *list;
};

/**
 * @brief Initialize a nanokernel linked list lifo object
 *
 * This function initializes a nanokernel system-level linked list lifo
 * object structure.
 *
 * It may be called from either a fiber or task context.
 *
 * @param chan LIFO to initialize.
 *
 * @return N/A
 */
extern void nano_lifo_init(struct nano_lifo *chan);

/* methods for ISRs */

/**
 * @brief Prepend an element to a LIFO without a context switch.
 *
 * This routine adds an element to the head of a LIFO object; it may be
 * called from an ISR context. A fiber pending on the LIFO
 * object will be made ready, but will NOT be scheduled to execute.
 *
 * @param chan LIFO on which to put.
 * @param data Data to insert.
 *
 * @return N/A
 */
extern void nano_isr_lifo_put(struct nano_lifo *chan, void *data);

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
 * @param chan LIFO from which to receive.
 *
 * @return Pointer to first element in the list if available, otherwise NULL
 */
extern void *nano_isr_lifo_get(struct nano_lifo *chan);

/* methods for fibers */

/**
 * @brief Prepend an element to a LIFO without a context switch.
 *
 * This routine adds an element to the head of a LIFO object; it may be
 * called from a fibercontext. A fiber pending on the LIFO
 * object will be made ready, but will NOT be scheduled to execute.
 *
 * @param chan LIFO from which to put.
 * @param data Data to insert.
 *
 * @return N/A
 */
extern void nano_fiber_lifo_put(struct nano_lifo *chan, void *data);

/**
 * @brief Remove the first element from a linked list LIFO
 *
 * Remove the first element from the specified nanokernel linked list LIFO;
 * it may be called from a fiber context.
 *
 * If no elements are available, NULL is returned. The first word in the
 * element contains invalid data because that memory location was used to store
 * a pointer to the next element in the linked list.
 *
 * @param chan LIFO from which to receive
 *
 * @return Pointer to first element in the list if available, otherwise NULL
 */
extern void *nano_fiber_lifo_get(struct nano_lifo *chan);

/**
 * @brief Get the first element from a LIFO, wait if empty.
 *
 * Remove the first element from the specified system-level linked list LIFO;
 * it can only be called from a fiber context.
 *
 * If no elements are available, the calling fiber will pend until an element
 * is put onto the list.
 *
 * The first word in the element contains invalid data because that memory
 * location was used to store a pointer to the next element in the linked list.
 *
 * @param chan LIFO from which to receive.
 *
 * @return Pointer to first element in the list
 */
extern void *nano_fiber_lifo_get_wait(struct nano_lifo *chan);

#ifdef CONFIG_NANO_TIMEOUTS

/**
 * @brief get the first element from a LIFO, wait with a timeout if empty
 *
 * Remove the first element from the specified system-level linked list lifo;
 * it can only be called from a fiber context.
 *
 * If no elements are available, the calling fiber will pend until an element
 * is put onto the list, or the timeout expires, whichever comes first.
 *
 * The first word in the element contains invalid data because that memory
 * location was used to store a pointer to the next element in the linked list.
 *
 * @param chan LIFO on which to operate.
 * @param timeout_in_ticks Time to wait in ticks.
 *
 * @return Pointer to first element in the list, NULL if timed out.
 */
extern void *nano_fiber_lifo_get_wait_timeout(struct nano_lifo *chan,
		int32_t timeout_in_ticks);

#endif

/* methods for tasks */

/**
 * @brief Add an element to the head of a linked list LIFO
 *
 * This routine adds an element to the head of a LIFO object; it can be
 * called only from a task context. A fiber pending on the LIFO
 * object will be made ready and will preempt the running task immediately.
 *
 * This API can only be called by a task.
 *
 * @param chan LIFO from which to put.
 * @param data Data to insert.
 *
 * @return N/A
 */
extern void nano_task_lifo_put(struct nano_lifo *chan, void *data);

/**
 * @brief Remove the first element from a linked list LIFO
 *
 * Remove the first element from the specified nanokernel linked list LIFO;
 * it may be called from a task context.
 *
 * If no elements are available, NULL is returned. The first word in the
 * element contains invalid data because that memory location was used to store
 * a pointer to the next element in the linked list.
 *
 * @param chan LIFO from which to receive.
 *
 * @return Pointer to first element in the list if available, otherwise NULL.
 */
extern void *nano_task_lifo_get(struct nano_lifo *chan);

/**
 * @brief Get the first element from a LIFO, poll if empty.
 *
 * Remove the first element from the specified nanokernel linked list LIFO; it
 * can only be called from a task context.
 *
 * If no elements are available, the calling task will poll until an element is
 * put onto the list.
 *
 * The first word in the element contains invalid data because that memory
 * location was used to store a pointer to the next element in the linked list.
 *
 * @param chan LIFO from which to receive.
 *
 * @sa nano_task_stack_pop_wait()
 *
 * @return Pointer to first element in the list
 */
extern void *nano_task_lifo_get_wait(struct nano_lifo *chan);

#ifdef CONFIG_NANO_TIMEOUTS

/**
 * @brief get the first element from a lifo, poll if empty.
 *
 * Remove the first element from the specified nanokernel linked list lifo; it
 * can only be called from a task context.
 *
 * If no elements are available, the calling task will poll until an element is
 * put onto the list, or the timeout expires, whichever comes first.
 *
 * The first word in the element contains invalid data because that memory
 * location was used to store a pointer to the next element in the linked list.
 *
 * @param chan LIFO on which to operate
 * @param timeout_in_ticks time to wait in ticks
 *
 * @return Pointer to first element in the list, NULL if timed out.
 */
extern void *nano_task_lifo_get_wait_timeout(struct nano_lifo *chan,
		int32_t timeout_in_ticks);

#endif

/* semaphore APIs */

struct nano_sem {
	struct _nano_queue wait_q;
	int nsig;
};

extern void nano_sem_init(struct nano_sem *chan);
/* scheduling context independent methods (when context is not known) */
extern void nano_sem_give(struct nano_sem *chan);
extern void nano_sem_take_wait(struct nano_sem *chan);
/* methods for ISRs */
extern void nano_isr_sem_give(struct nano_sem *chan);
extern int nano_isr_sem_take(struct nano_sem *chan);
/* methods for fibers */
extern void nano_fiber_sem_give(struct nano_sem *chan);
extern int nano_fiber_sem_take(struct nano_sem *chan);
extern void nano_fiber_sem_take_wait(struct nano_sem *chan);
#ifdef CONFIG_NANO_TIMEOUTS
extern int nano_fiber_sem_take_wait_timeout(struct nano_sem *chan,
		int32_t timeout);
#endif

/* methods for tasks */
extern void nano_task_sem_give(struct nano_sem *chan);
extern int nano_task_sem_take(struct nano_sem *chan);
extern void nano_task_sem_take_wait(struct nano_sem *chan);
#ifdef CONFIG_NANO_TIMEOUTS
extern int nano_task_sem_take_wait_timeout(struct nano_sem *chan,
		int32_t timeout);
#endif

/* stack APIs */

struct nano_stack {
	nano_context_id_t fiber;
	uint32_t *base;
	uint32_t *next;
};

extern void nano_stack_init(struct nano_stack *chan, uint32_t *data);
/* methods for ISRs */
extern void nano_isr_stack_push(struct nano_stack *chan, uint32_t data);
extern int nano_isr_stack_pop(struct nano_stack *chan, uint32_t *data);
/* methods for fibers */
extern void nano_fiber_stack_push(struct nano_stack *chan, uint32_t data);
extern int nano_fiber_stack_pop(struct nano_stack *chan, uint32_t *data);
extern uint32_t nano_fiber_stack_pop_wait(struct nano_stack *chan);
/* methods for tasks */
extern void nano_task_stack_push(struct nano_stack *chan, uint32_t data);
extern int nano_task_stack_pop(struct nano_stack *chan, uint32_t *data);
extern uint32_t nano_task_stack_pop_wait(struct nano_stack *chan);

/* context custom data APIs */
#ifdef CONFIG_CONTEXT_CUSTOM_DATA
extern void context_custom_data_set(void *value);
extern void *context_custom_data_get(void);
#endif /* CONFIG_CONTEXT_CUSTOM_DATA */

/* nanokernel timers */

struct nano_timer {
	struct nano_timer *link;
	uint32_t ticks;
	struct nano_lifo lifo;
	void *userData;
};

extern void nano_timer_init(struct nano_timer *chan, void *data);

/* methods for fibers */
extern void nano_fiber_timer_start(struct nano_timer *chan, int ticks);
extern void *nano_fiber_timer_test(struct nano_timer *chan);
extern void *nano_fiber_timer_wait(struct nano_timer *chan);
extern void nano_fiber_timer_stop(struct nano_timer *chan);

/* methods for tasks */
extern void nano_task_timer_start(struct nano_timer *chan, int ticks);
extern void *nano_task_timer_test(struct nano_timer *chan);
extern void *nano_task_timer_wait(struct nano_timer *chan);
extern void nano_task_timer_stop(struct nano_timer *chan);

/* methods for tasks and fibers for handling time and ticks */

extern int64_t nano_tick_get(void);
extern uint32_t nano_tick_get_32(void);
extern uint32_t nano_cycle_get_32(void);
extern int64_t nano_tick_delta(int64_t *reftime);
extern uint32_t nano_tick_delta_32(int64_t *reftime);

#ifdef __cplusplus
}
#endif

/* architecture-specific nanokernel public APIs */

#include <arch/cpu.h>

#endif /* __NANOKERNEL_H__ */
