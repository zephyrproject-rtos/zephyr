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

#include <stdint.h>
#ifdef __DCC__
#undef CONFIG_STACK_CANARIES
#endif

#include <nanokernel/private.h>
#include <kernel_version.h>
#include <clock_vars.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int atomic_t;
typedef atomic_t atomic_val_t;

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

/* FIFO APIs
 */
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

/* LIFO APIs
 */
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

/* semaphore APIs
 */
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

/* stack APIs
 */
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

extern uint32_t nano_tick_get_32(void);
extern uint32_t nano_cycle_get_32(void);
extern uint32_t nano_tick_delta(uint64_t *reftime);

#endif /* CONFIG_NANOKERNEL */

/* atomic operator APIs */

extern atomic_val_t atomic_add(atomic_t *target, atomic_val_t value);
extern atomic_val_t atomic_and(atomic_t *target, atomic_val_t value);
extern atomic_val_t atomic_dec(atomic_t *target);
extern atomic_val_t atomic_inc(atomic_t *target);
extern atomic_val_t atomic_nand(atomic_t *target, atomic_val_t value);
extern atomic_val_t atomic_or(atomic_t *target, atomic_val_t value);
extern atomic_val_t atomic_sub(atomic_t *target, atomic_val_t value);
extern atomic_val_t atomic_xor(atomic_t *target, atomic_val_t value);
extern atomic_val_t atomic_clear(atomic_t *target);
extern atomic_val_t atomic_get(atomic_t *target);
extern atomic_val_t atomic_set(atomic_t *target, atomic_val_t value);
extern int atomic_cas(atomic_t *target,
			 atomic_val_t oldValue,
			 atomic_val_t newValue);
#define ATOMIC_INIT(i) {(i)}

/* auto-initialization */

#define NANO_CONSTRUCTOR(lvl) FUNC_CONSTRUCT(lvl)

#define NANO_INIT_PRI_SYS_FIRST 225
#define NANO_INIT_PRI_SYS_NORMAL 250
#define NANO_INIT_PRI_SYS_LAST 275

#define NANO_INIT_PRI_DRV_FIRST 325
#define NANO_INIT_PRI_DRV_NORMAL 350
#define NANO_INIT_PRI_DRV_LAST 375

#define NANO_INIT_PRI_USR_FIRST 425
#define NANO_INIT_PRI_USR_NORMAL 450
#define NANO_INIT_PRI_USR_LAST 475

/* To define an init function, write the function as:
 * NANO_INIT_DRV_NORMAL(myDrv) {<implementation>}
 * and it will be automatically invoked when the system initializes.
 *
 * Always use _NORMAL priority init functions, unless you cant.
 * _FIRST priorities can be used to initialize ahead of the _NORMAL.
 * Likewise, _LAST priorities can be used to initialize afterwards.
 * _SYS priorities are normally for the kernel itself.
 * _DRV priorities are for device drivers.
 * _USR priorities are for utilities and libraries.
 */
#define NANO_INIT_SYS_FIRST NANO_CONSTRUCTOR(NANO_INIT_PRI_SYS_FIRST)
#define NANO_INIT_SYS_NORMAL NANO_CONSTRUCTOR(NANO_INIT_PRI_SYS_NORMAL)
#define NANO_INIT_SYS_LAST NANO_CONSTRUCTOR(NANO_INIT_PRI_SYS_LAST)

#define NANO_INIT_DRV_FIRST NANO_CONSTRUCTOR(NANO_INIT_PRI_DRV_FIRST)
#define NANO_INIT_DRV_NORMAL NANO_CONSTRUCTOR(NANO_INIT_PRI_DRV_NORMAL)
#define NANO_INIT_DRV_LAST NANO_CONSTRUCTOR(NANO_INIT_PRI_DRV_LAST)

#define NANO_INIT_USR_FIRST NANO_CONSTRUCTOR(NANO_INIT_PRI_USR_FIRST)
#define NANO_INIT_USR_NORMAL NANO_CONSTRUCTOR(NANO_INIT_PRI_USR_NORMAL)
#define NANO_INIT_USR_LAST NANO_CONSTRUCTOR(NANO_INIT_PRI_USR_LAST)

extern uint32_t kernel_version_get(void);

#ifdef __cplusplus
}
#endif

#endif /* __NANOKERNEL_H__ */
