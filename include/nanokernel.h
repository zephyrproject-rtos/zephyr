/* nanokernel.h - public API for VxMicro nanokernel */

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

#ifndef __NANOKERNEL_H__
#define __NANOKERNEL_H__

/* fundamental include files */

#include <stddef.h>
#include <stdint.h>
#include <toolchain.h>

/* generic kernel public APIs */

#include <kernel_version.h>
#include <clock_vars.h>
#include <drivers/rand32.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * nanokernel private APIs that are exposed via the public API
 *
 * THESE ITEMS SHOULD NOT BE REFERENCED EXCEPT BY THE KERNEL ITSELF!
 */

typedef struct s_CCS tCCS;

struct _nano_queue {
	void *head;
	void *tail;
};

/* architecture-independent nanokernel public APIs */

typedef struct s_CCS *nano_context_id_t;

typedef void (*nano_fiber_entry_t)(int i1, int i2);

typedef int nano_context_type_t;


#define NANO_CTX_ISR (0)
#define NANO_CTX_FIBER (1)
#define NANO_CTX_TASK (2)

/* context APIs
 */
extern nano_context_id_t context_self_get(void);
extern nano_context_type_t context_type_get(void);
extern int _context_essential_check(tCCS *pCtx);

/* fiber APIs
 */
/* scheduling context independent method (when context is not known) */
void fiber_start(char *stack,
			unsigned stack_size,
			nano_fiber_entry_t entry,
			int arg1,
			int arg2,
			unsigned prio,
			unsigned options);

/* methods for fibers */
extern void fiber_fiber_start(char *pStack,
				unsigned int stackSize,
				nano_fiber_entry_t entry,
				int arg1,
				int arg2,
				unsigned prio,
				unsigned options);
extern void fiber_yield(void);
extern void fiber_abort(void);
/* methods for tasks */
extern void task_fiber_start(char *pStack,
			       unsigned int stackSize,
			       nano_fiber_entry_t entry,
			       int arg1,
			       int arg2,
			       unsigned prio,
			       unsigned options);

/* FIFO APIs */

struct nano_fifo {
	union {
		struct _nano_queue wait_q;
		struct _nano_queue data_q;
	};
	int stat;
};

extern void nano_fifo_init(struct nano_fifo *chan);
/* scheduling context independent methods (when context is not known) */
extern void nano_fifo_put(struct nano_fifo *chan, void *data);
extern void *nano_fifo_get(struct nano_fifo *chan);
extern void *nano_fifo_get_wait(struct nano_fifo *chan);
/* methods for ISRs */
extern void nano_isr_fifo_put(struct nano_fifo *chan, void *data);
extern void *nano_isr_fifo_get(struct nano_fifo *chan);
/* methods for fibers */
extern void nano_fiber_fifo_put(struct nano_fifo *chan, void *data);
extern void *nano_fiber_fifo_get(struct nano_fifo *chan);
extern void *nano_fiber_fifo_get_wait(struct nano_fifo *chan);
/* methods for tasks */
extern void nano_task_fifo_put(struct nano_fifo *chan, void *data);
extern void *nano_task_fifo_get(struct nano_fifo *chan);
extern void *nano_task_fifo_get_wait(struct nano_fifo *chan);

/* LIFO APIs */

struct nano_lifo {
	struct _nano_queue wait_q;
	void *list;
};

extern void nano_lifo_init(struct nano_lifo *chan);
/* methods for ISRs */
extern void nano_isr_lifo_put(struct nano_lifo *chan, void *data);
extern void *nano_isr_lifo_get(struct nano_lifo *chan);
/* methods for fibers */
extern void nano_fiber_lifo_put(struct nano_lifo *chan, void *data);
extern void *nano_fiber_lifo_get(struct nano_lifo *chan);
extern void *nano_fiber_lifo_get_wait(struct nano_lifo *chan);
/* methods for tasks */
extern void nano_task_lifo_put(struct nano_lifo *chan, void *data);
extern void *nano_task_lifo_get(struct nano_lifo *chan);
extern void *nano_task_lifo_get_wait(struct nano_lifo *chan);

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
/* methods for tasks */
extern void nano_task_sem_give(struct nano_sem *chan);
extern int nano_task_sem_take(struct nano_sem *chan);
extern void nano_task_sem_take_wait(struct nano_sem *chan);

/* stack APIs */

struct nano_stack {
	tCCS *fiber;
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

#if defined(CONFIG_NANOKERNEL)

/* nanokernel-only timers */

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

#endif /* CONFIG_NANOKERNEL */

/*
 * Auto-initialization
 *
 * The SYS_PREKERNEL_INIT() macro is used to indicate that the specified
 * routine be executed before the kernel initializes.  All such routines
 * are issued in order of highest priority level to lowest priority level.
 * Values are 000 (highest priority) to 999 (lowest priority).  If all
 * three (3) digits are not supplied, unexpected results may occur, as
 * the linker would interpret priority 19 as a lower priority than 2.  To
 * prevent that scenario, the proper priorities to specify would be 019
 * and 002 respectively.
 *
 * Example:
 *    void my_library_init(void)
 *    {
 *        ...
 *    }
 *
 * SYS_PREKERNEL_INIT(my_library_init, 500);
 *
 */

#define SYS_PREKERNEL_INIT(name, level) \
			void (*__ctor_##name)(void) __prekernel_init_level(level) = name

#ifdef __cplusplus
}
#endif


/* architecture-specific nanokernel public APIs */

#include <arch/cpu.h>

#endif /* __NANOKERNEL_H__ */
