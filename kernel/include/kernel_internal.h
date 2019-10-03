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
#include <stdbool.h>

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

/* Implemented by architectures. Only called from z_setup_new_thread. */
extern void z_arch_new_thread(struct k_thread *thread, k_thread_stack_t *pStack,
			      size_t stackSize, k_thread_entry_t entry,
			      void *p1, void *p2, void *p3,
			      int prio, unsigned int options);

extern void z_setup_new_thread(struct k_thread *new_thread,
			      k_thread_stack_t *stack, size_t stack_size,
			      k_thread_entry_t entry,
			      void *p1, void *p2, void *p3,
			      int prio, u32_t options, const char *name);

#if defined(CONFIG_FLOAT) && defined(CONFIG_FP_SHARING)
extern int z_arch_float_disable(struct k_thread *thread);
#endif /* CONFIG_FLOAT && CONFIG_FP_SHARING */

#ifdef CONFIG_USERSPACE
extern int z_arch_mem_domain_max_partitions_get(void);

extern void z_arch_mem_domain_thread_add(struct k_thread *thread);

extern void z_arch_mem_domain_thread_remove(struct k_thread *thread);

extern void z_arch_mem_domain_partition_remove(struct k_mem_domain *domain,
					       u32_t partition_id);

extern void z_arch_mem_domain_partition_add(struct k_mem_domain *domain,
					    u32_t partition_id);

extern void z_arch_mem_domain_destroy(struct k_mem_domain *domain);

extern int z_arch_buffer_validate(void *addr, size_t size, int write);

extern FUNC_NORETURN
void z_arch_user_mode_enter(k_thread_entry_t user_entry, void *p1, void *p2,
			   void *p3);


extern FUNC_NORETURN void z_arch_syscall_oops(void *ssf);

extern size_t z_arch_user_string_nlen(const char *s, size_t maxsize, int *err);

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

extern void z_smp_init(void);

extern void smp_timer_init(void);

extern u32_t z_early_boot_rand32_get(void);

#if CONFIG_STACK_POINTER_RANDOM
extern int z_stack_adjust_initialized;
#endif

#if defined(CONFIG_ARCH_HAS_CUSTOM_BUSY_WAIT)
extern void z_arch_busy_wait(u32_t usec_to_wait);
#endif

int z_arch_swap(unsigned int key);

extern FUNC_NORETURN void z_arch_system_halt(unsigned int reason);

#ifdef CONFIG_EXECUTION_BENCHMARKING
extern u64_t z_arch_timing_swap_start;
extern u64_t z_arch_timing_swap_end;
extern u64_t z_arch_timing_irq_start;
extern u64_t z_arch_timing_irq_end;
extern u64_t z_arch_timing_tick_start;
extern u64_t z_arch_timing_tick_end;
extern u64_t z_arch_timing_user_mode_end;
extern u32_t z_arch_timing_value_swap_end;
extern u64_t z_arch_timing_value_swap_common;
extern u64_t z_arch_timing_value_swap_temp;
#endif /* CONFIG_EXECUTION_BENCHMARKING */

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
