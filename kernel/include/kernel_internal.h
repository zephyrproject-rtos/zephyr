/*
 * Copyright (c) 2010-2012, 2014-2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Architecture-independent private kernel APIs
 *
 * This file contains private kernel APIs that are not architecture-specific.
 */

#ifndef ZEPHYR_KERNEL_INCLUDE_KERNEL_INTERNAL_H_
#define ZEPHYR_KERNEL_INCLUDE_KERNEL_INTERNAL_H_

#include <kernel.h>
#include <kernel_arch_interface.h>
#include <string.h>

#ifndef _ASMLANGUAGE

#ifdef __cplusplus
extern "C" {
#endif

/* Early boot functions */

void z_bss_zero(void);
#ifdef CONFIG_XIP
void z_data_copy(void);
#else
static inline void z_data_copy(void)
{
	/* Do nothing */
}
#endif
FUNC_NORETURN void z_cstart(void);

extern FUNC_NORETURN void z_thread_entry(k_thread_entry_t entry,
			  void *p1, void *p2, void *p3);

extern char *z_setup_new_thread(struct k_thread *new_thread,
				k_thread_stack_t *stack, size_t stack_size,
				k_thread_entry_t entry,
				void *p1, void *p2, void *p3,
				int prio, uint32_t options, const char *name);

/**
 * @brief Allocate some memory from the current thread's resource pool
 *
 * Threads may be assigned a resource pool, which will be used to allocate
 * memory on behalf of certain kernel and driver APIs. Memory reserved
 * in this way should be freed with k_free().
 *
 * If called from an ISR, the k_malloc() system heap will be used if it exists.
 *
 * @param size Memory allocation size
 * @return A pointer to the allocated memory, or NULL if there is insufficient
 * RAM in the pool or there is no pool to draw memory from
 */
void *z_thread_malloc(size_t size);

/* set and clear essential thread flag */

extern void z_thread_essential_set(void);
extern void z_thread_essential_clear(void);

/* clean up when a thread is aborted */

#if defined(CONFIG_THREAD_MONITOR)
extern void z_thread_monitor_exit(struct k_thread *thread);
#else
#define z_thread_monitor_exit(thread) \
	do {/* nothing */    \
	} while (false)
#endif /* CONFIG_THREAD_MONITOR */

#ifdef CONFIG_USE_SWITCH
/* This is a arch function traditionally, but when the switch-based
 * z_swap() is in use it's a simple inline provided by the kernel.
 */
static ALWAYS_INLINE void
arch_thread_return_value_set(struct k_thread *thread, unsigned int value)
{
	thread->swap_retval = value;
}
#endif

static ALWAYS_INLINE void
z_thread_return_value_set_with_data(struct k_thread *thread,
				   unsigned int value,
				   void *data)
{
	arch_thread_return_value_set(thread, value);
	thread->base.swap_data = data;
}

extern void z_smp_init(void);

extern void smp_timer_init(void);

extern void z_early_boot_rand_get(uint8_t *buf, size_t length);

#if CONFIG_STACK_POINTER_RANDOM
extern int z_stack_adjust_initialized;
#endif

#ifdef CONFIG_BOOT_TIME_MEASUREMENT
extern uint32_t z_timestamp_main; /* timestamp when main task starts */
extern uint32_t z_timestamp_idle; /* timestamp when CPU goes idle */
#endif

extern struct k_thread z_main_thread;


#ifdef CONFIG_MULTITHREADING
extern struct k_thread z_idle_threads[CONFIG_MP_NUM_CPUS];
#endif
extern K_KERNEL_STACK_ARRAY_DEFINE(z_interrupt_stacks, CONFIG_MP_NUM_CPUS,
				   CONFIG_ISR_STACK_SIZE);

#ifdef CONFIG_GEN_PRIV_STACKS
extern uint8_t *z_priv_stack_find(k_thread_stack_t *stack);
#endif

#ifdef CONFIG_USERSPACE
bool z_stack_is_user_capable(k_thread_stack_t *stack);
#endif /* CONFIG_USERSPACE */

#ifdef CONFIG_GDBSTUB
struct gdb_ctx;

/* Should be called by the arch layer. This is the gdbstub main loop
 * and synchronously communicate with gdb on host.
 */
extern int z_gdb_main_loop(struct gdb_ctx *ctx, bool start);
#endif


#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_KERNEL_INCLUDE_KERNEL_INTERNAL_H_ */
