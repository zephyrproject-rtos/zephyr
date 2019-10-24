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

extern void z_setup_new_thread(struct k_thread *new_thread,
			      k_thread_stack_t *stack, size_t stack_size,
			      k_thread_entry_t entry,
			      void *p1, void *p2, void *p3,
			      int prio, u32_t options, const char *name);

static ALWAYS_INLINE void z_new_thread_init(struct k_thread *thread,
					    char *pStack, size_t stackSize,
					    int prio, unsigned int options)
{
#if !defined(CONFIG_INIT_STACKS) && !defined(CONFIG_THREAD_STACK_INFO)
	ARG_UNUSED(pStack);
	ARG_UNUSED(stackSize);
#endif

#ifdef CONFIG_INIT_STACKS
	memset(pStack, 0xaa, stackSize);
#endif
#ifdef CONFIG_STACK_SENTINEL
	/* Put the stack sentinel at the lowest 4 bytes of the stack area.
	 * We periodically check that it's still present and kill the thread
	 * if it isn't.
	 */
	*((u32_t *)pStack) = STACK_SENTINEL;
#endif /* CONFIG_STACK_SENTINEL */
	/* Initialize various struct k_thread members */
	z_init_thread_base(&thread->base, prio, _THREAD_PRESTART, options);

	/* static threads overwrite it afterwards with real value */
	thread->init_data = NULL;
	thread->fn_abort = NULL;

#ifdef CONFIG_THREAD_CUSTOM_DATA
	/* Initialize custom data field (value is opaque to kernel) */
	thread->custom_data = NULL;
#endif

#ifdef CONFIG_THREAD_NAME
	thread->name[0] = '\0';
#endif

#if defined(CONFIG_USERSPACE)
	thread->mem_domain_info.mem_domain = NULL;
#endif /* CONFIG_USERSPACE */

#if defined(CONFIG_THREAD_STACK_INFO)
	thread->stack_info.start = (uintptr_t)pStack;
	thread->stack_info.size = (u32_t)stackSize;
#endif /* CONFIG_THREAD_STACK_INFO */
}

#ifdef CONFIG_USERSPACE
/**
 * @brief Zero out BSS sections for application shared memory
 *
 * This isn't handled by any platform bss zeroing, and is called from
 * z_cstart() if userspace is enabled.
 */
extern void z_app_shmem_bss_zero(void);
#endif /* CONFIG_USERSPACE */

/**
 * @brief Allocate some memory from the current thread's resource pool
 *
 * Threads may be assigned a resource pool, which will be used to allocate
 * memory on behalf of certain kernel and driver APIs. Memory reserved
 * in this way should be freed with k_free().
 *
 * @param size Memory allocation size
 * @return A pointer to the allocated memory, or NULL if there is insufficient
 * RAM in the pool or the thread has no resource pool assigned
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
z_arch_thread_return_value_set(struct k_thread *thread, unsigned int value)
{
	thread->swap_retval = value;
}
#endif

static ALWAYS_INLINE void
z_thread_return_value_set_with_data(struct k_thread *thread,
				   unsigned int value,
				   void *data)
{
	z_arch_thread_return_value_set(thread, value);
	thread->base.swap_data = data;
}

extern void z_smp_init(void);

extern void smp_timer_init(void);

extern u32_t z_early_boot_rand32_get(void);

#if CONFIG_STACK_POINTER_RANDOM
extern int z_stack_adjust_initialized;
#endif

#ifdef CONFIG_BOOT_TIME_MEASUREMENT
extern u32_t z_timestamp_main; /* timestamp when main task starts */
extern u32_t z_timestamp_idle; /* timestamp when CPU goes idle */
#endif

extern struct k_thread z_main_thread;
extern struct k_thread z_idle_thread;
extern K_THREAD_STACK_DEFINE(z_main_stack, CONFIG_MAIN_STACK_SIZE);
extern K_THREAD_STACK_DEFINE(z_idle_stack, CONFIG_IDLE_STACK_SIZE);

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_KERNEL_INCLUDE_KERNEL_INTERNAL_H_ */
