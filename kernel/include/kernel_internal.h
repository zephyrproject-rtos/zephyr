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

#include <zephyr/kernel.h>
#include <kernel_arch_interface.h>
#include <string.h>

#ifndef _ASMLANGUAGE

#ifdef __cplusplus
extern "C" {
#endif

/* Initialize per-CPU kernel data */
void z_init_cpu(int id);

/* Initialize a thread */
void z_init_thread_base(struct _thread_base *thread_base, int priority,
			uint32_t initial_state, unsigned int options);

/* Early boot functions */
void z_early_memset(void *dst, int c, size_t n);
void z_early_memcpy(void *dst, const void *src, size_t n);

void z_bss_zero(void);
#ifdef CONFIG_XIP
void z_data_copy(void);
#else
static inline void z_data_copy(void)
{
	/* Do nothing */
}
#endif /* CONFIG_XIP */

#ifdef CONFIG_LINKER_USE_BOOT_SECTION
void z_bss_zero_boot(void);
#else
static inline void z_bss_zero_boot(void)
{
	/* Do nothing */
}
#endif /* CONFIG_LINKER_USE_BOOT_SECTION */

#ifdef CONFIG_LINKER_USE_PINNED_SECTION
void z_bss_zero_pinned(void);
#else
static inline void z_bss_zero_pinned(void)
{
	/* Do nothing */
}
#endif /* CONFIG_LINKER_USE_PINNED_SECTION */

FUNC_NORETURN void z_cstart(void);

void z_device_state_init(void);

extern FUNC_NORETURN void z_thread_entry(k_thread_entry_t entry,
			  void *p1, void *p2, void *p3);

extern char *z_setup_new_thread(struct k_thread *new_thread,
				k_thread_stack_t *stack, size_t stack_size,
				k_thread_entry_t entry,
				void *p1, void *p2, void *p3,
				int prio, uint32_t options, const char *name);

/**
 * @brief Allocate aligned memory from the current thread's resource pool
 *
 * Threads may be assigned a resource pool, which will be used to allocate
 * memory on behalf of certain kernel and driver APIs. Memory reserved
 * in this way should be freed with k_free().
 *
 * If called from an ISR, the k_malloc() system heap will be used if it exists.
 *
 * @param align Required memory alignment
 * @param size Memory allocation size
 * @return A pointer to the allocated memory, or NULL if there is insufficient
 * RAM in the pool or there is no pool to draw memory from
 */
void *z_thread_aligned_alloc(size_t align, size_t size);

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
static inline void *z_thread_malloc(size_t size)
{
	return z_thread_aligned_alloc(0, size);
}


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

#ifdef CONFIG_SMP
extern void z_smp_init(void);
#ifdef CONFIG_SYS_CLOCK_EXISTS
extern void smp_timer_init(void);
#endif /* CONFIG_SYS_CLOCK_EXISTS */
#endif /* CONFIG_SMP */

extern void z_early_rand_get(uint8_t *buf, size_t length);

#if defined(CONFIG_STACK_POINTER_RANDOM) && (CONFIG_STACK_POINTER_RANDOM != 0)
extern int z_stack_adjust_initialized;
#endif /* CONFIG_STACK_POINTER_RANDOM */

extern struct k_thread z_main_thread;


#ifdef CONFIG_MULTITHREADING
extern struct k_thread z_idle_threads[CONFIG_MP_MAX_NUM_CPUS];
#endif /* CONFIG_MULTITHREADING */
K_KERNEL_PINNED_STACK_ARRAY_DECLARE(z_interrupt_stacks, CONFIG_MP_MAX_NUM_CPUS,
				    CONFIG_ISR_STACK_SIZE);

#ifdef CONFIG_GEN_PRIV_STACKS
extern uint8_t *z_priv_stack_find(k_thread_stack_t *stack);
#endif /* CONFIG_GEN_PRIV_STACKS */

/* Calculate stack usage. */
int z_stack_space_get(const uint8_t *stack_start, size_t size, size_t *unused_ptr);

#ifdef CONFIG_USERSPACE
bool z_stack_is_user_capable(k_thread_stack_t *stack);

/* Memory domain setup hook, called from z_setup_new_thread() */
void z_mem_domain_init_thread(struct k_thread *thread);

/* Memory domain teardown hook, called from z_thread_abort() */
void z_mem_domain_exit_thread(struct k_thread *thread);

/* This spinlock:
 *
 * - Protects the full set of active k_mem_domain objects and their contents
 * - Serializes calls to arch_mem_domain_* APIs
 *
 * If architecture code needs to access k_mem_domain structures or the
 * partitions they contain at any other point, this spinlock should be held.
 * Uniprocessor systems can get away with just locking interrupts but this is
 * not recommended.
 */
extern struct k_spinlock z_mem_domain_lock;
#endif /* CONFIG_USERSPACE */

#ifdef CONFIG_GDBSTUB
struct gdb_ctx;

/* Should be called by the arch layer. This is the gdbstub main loop
 * and synchronously communicate with gdb on host.
 */
extern int z_gdb_main_loop(struct gdb_ctx *ctx);
#endif /* CONFIG_GDBSTUB */

#ifdef CONFIG_INSTRUMENT_THREAD_SWITCHING
void z_thread_mark_switched_in(void);
void z_thread_mark_switched_out(void);
#else

/**
 * @brief Called after a thread has been selected to run
 */
#define z_thread_mark_switched_in()

/**
 * @brief Called before a thread has been selected to run
 */

#define z_thread_mark_switched_out()

#endif /* CONFIG_INSTRUMENT_THREAD_SWITCHING */

/* Init hook for page frame management, invoked immediately upon entry of
 * main thread, before POST_KERNEL tasks
 */
void z_mem_manage_init(void);

/**
 * @brief Finalize page frame management at the end of boot process.
 */
void z_mem_manage_boot_finish(void);


void z_handle_obj_poll_events(sys_dlist_t *events, uint32_t state);

#ifdef CONFIG_PM

/* When the kernel is about to go idle, it calls this function to notify the
 * power management subsystem, that the kernel is ready to enter the idle state.
 *
 * At this point, the kernel has disabled interrupts and computed the maximum
 * time the system can remain idle. The function passes the time that the system
 * can remain idle. The SOC interface performs power operations that can be done
 * in the available time. The power management operations must halt execution of
 * the CPU.
 *
 * This function assumes that a wake up event has already been set up by the
 * application.
 *
 * This function is entered with interrupts disabled. It should re-enable
 * interrupts if it had entered a power state.
 *
 * @return True if the system suspended, otherwise return false
 */
bool pm_system_suspend(int32_t ticks);

#endif /* CONFIG_PM */

#ifdef CONFIG_DEMAND_PAGING_TIMING_HISTOGRAM
/**
 * Initialize the timing histograms for demand paging.
 */
void z_paging_histogram_init(void);

/**
 * Increment the counter in the timing histogram.
 *
 * @param hist The timing histogram to be updated.
 * @param cycles Time spent in measured operation.
 */
void z_paging_histogram_inc(struct k_mem_paging_histogram_t *hist,
			    uint32_t cycles);
#endif /* CONFIG_DEMAND_PAGING_TIMING_HISTOGRAM */

#ifdef CONFIG_OBJ_CORE_STATS_THREAD
int z_thread_stats_raw(struct k_obj_core *obj_core, void *stats);
int z_thread_stats_query(struct k_obj_core *obj_core, void *stats);
int z_thread_stats_reset(struct k_obj_core *obj_core);
int z_thread_stats_disable(struct k_obj_core *obj_core);
int z_thread_stats_enable(struct k_obj_core *obj_core);
#endif /* CONFIG_OBJ_CORE_STATS_THREAD */

#ifdef CONFIG_OBJ_CORE_STATS_SYSTEM
int z_cpu_stats_raw(struct k_obj_core *obj_core, void *stats);
int z_cpu_stats_query(struct k_obj_core *obj_core, void *stats);

int z_kernel_stats_raw(struct k_obj_core *obj_core, void *stats);
int z_kernel_stats_query(struct k_obj_core *obj_core, void *stats);
#endif /* CONFIG_OBJ_CORE_STATS_SYSTEM */

#if defined(CONFIG_THREAD_ABORT_NEED_CLEANUP)
/**
 * Perform cleanup at the end of k_thread_abort().
 *
 * This performs additional cleanup steps at the end of k_thread_abort()
 * where these steps require that the thread is no longer running.
 * If the target thread is not the current running thread, the cleanup
 * steps will be performed immediately. However, if the target thread is
 * the current running thread (e.g. k_thread_abort(_current)), it defers
 * the cleanup steps to later when the work will be finished in another
 * context.
 *
 * @param thread Pointer to thread to be cleaned up.
 */
void k_thread_abort_cleanup(struct k_thread *thread);

/**
 * Check if thread is the same as the one waiting for cleanup.
 *
 * This is used to guard against reusing the same thread object
 * before the previous cleanup has finished. This will perform
 * the necessary cleanups before the thread object can be
 * reused. Should mainly be used during thread creation.
 *
 * @param thread Pointer to thread to be checked.
 */
void k_thread_abort_cleanup_check_reuse(struct k_thread *thread);
#endif /* CONFIG_THREAD_ABORT_NEED_CLEANUP */

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_KERNEL_INCLUDE_KERNEL_INTERNAL_H_ */
