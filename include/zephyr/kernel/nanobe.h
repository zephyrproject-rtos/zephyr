/*
 * Copyright (c) 2016-2026 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Nanobe, co-operative stack switched contexts within a thread
 *
 * A nanobe is a minimal co-operative execution context with its own stack
 * but no thread object, no priority and no kernel bookkeeping; switching
 * between nanobes only saves and restores the AAPCS callee-saved registers
 * and swaps the stack pointer.
 *
 * With @kconfig{CONFIG_USE_NANOBE_SWITCH}, the Zephyr threads are nanobes: the kernel
 * thread APIs are unchanged, a thread context switch is a nanobe switch and
 * preemption injects a reschedule into the interrupted thread.
 *
 * The APIs below use nanobes inside a single Zephyr thread (the nanobe owner
 * thread). The Zephyr scheduler is unaware of these nanobes: while one runs,
 * the owner thread's stack pointer simply points into the nanobe stack. The
 * owner thread may still be preempted by other Zephyr threads.
 *
 * With @kconfig{CONFIG_NANOBE_INJECTION}, an interrupt service routine can
 * inject a function call (e.g. nanobe_sched_yield()) into the interrupted
 * owner thread, so that it is executed in thread mode on the interrupted
 * nanobe's stack once the interrupt returns, before the interrupted code
 * resumes.
 */

#ifndef ZEPHYR_INCLUDE_KERNEL_NANOBE_H_
#define ZEPHYR_INCLUDE_KERNEL_NANOBE_H_

#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/toolchain.h>
#include <zephyr/linker/section_tags.h>
#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nanobe_apis Nanobe APIs
 * @ingroup kernel_apis
 * @{
 */

/** @brief Nanobe entry point signature. A nanobe entry must not return. */
typedef void (*nanobe_t)(void);

/**
 * @brief Nanobe entry point signature with an argument.
 *
 * A nanobe entry must not return.
 */
typedef void (*nanobe_arg_t)(void *arg);

/** @brief Required alignment of a nanobe stack. */
#define NANOBE_STACK_ALIGN 8

/**
 * @brief Statically define a nanobe stack.
 *
 * @param name Name of the stack buffer.
 * @param size Size of the stack buffer in bytes.
 */
#define NANOBE_STACK_DEFINE(name, size) \
	static uint8_t __noinit __aligned(NANOBE_STACK_ALIGN) \
		name[ROUND_UP(size, NANOBE_STACK_ALIGN)]

/**
 * @brief Get the initial (top) stack pointer of a nanobe stack.
 *
 * @param name Name of a stack defined with NANOBE_STACK_DEFINE().
 */
#define NANOBE_STACK_TOP(name) ((void *)((uint8_t *)(name) + sizeof(name)))

/**
 * @brief Statically define an array of nanobe stacks.
 *
 * @param name Name of the stack array.
 * @param n Number of stacks.
 * @param size Size of each stack in bytes.
 */
#define NANOBE_STACK_ARRAY_DEFINE(name, n, size) \
	static uint8_t __noinit __aligned(NANOBE_STACK_ALIGN) \
		name[n][ROUND_UP(size, NANOBE_STACK_ALIGN)]

/**
 * @brief Get the initial (top) stack pointer of a stack in a stack array.
 *
 * @param name Name of an array defined with NANOBE_STACK_ARRAY_DEFINE().
 * @param i Index of the stack.
 */
#define NANOBE_STACK_ARRAY_TOP(name, i) \
	((void *)((uint8_t *)(name)[i] + sizeof((name)[i])))

/**
 * @brief Nanobe semaphore.
 *
 * Usable from nanobes and, for give, from interrupt service routines. Do not
 * access the fields directly.
 */
struct nanobe_sem {
	volatile uint32_t count;
	uint32_t limit;
};

/**
 * @brief Statically define and initialize a nanobe semaphore.
 *
 * @param name Name of the semaphore.
 * @param initial_count Initial count.
 * @param count_limit Maximum count.
 */
#define NANOBE_SEM_DEFINE(name, initial_count, count_limit) \
	struct nanobe_sem name = { \
		.count = (initial_count), \
		.limit = (count_limit), \
	}

/**
 * @brief Initialize a nanobe.
 *
 * Prepares an initial context frame on the given stack so that the first
 * switch to the returned stack pointer starts executing @p entry.
 *
 * The first call records the calling thread as the nanobe owner thread.
 * All nanobe APIs (except nanobe_isr_inject() and nanobe_sem_give()) must
 * be called in thread mode
 * from the owner thread, i.e. from its original context or from a nanobe.
 *
 * @param entry Nanobe entry function, must not return.
 * @param stack_top Top (highest address) of the nanobe stack.
 *
 * @return Nanobe stack pointer, to be passed to nanobe_switch() or
 *         nanobe_sched_enqueue().
 */
void *nanobe_init(nanobe_t entry, void *stack_top);

/**
 * @brief Initialize a nanobe with an entry argument.
 *
 * Same as nanobe_init(), @p arg is passed to @p entry.
 *
 * @param entry Nanobe entry function, must not return.
 * @param arg Argument passed to @p entry.
 * @param stack_top Top (highest address) of the nanobe stack.
 *
 * @return Nanobe stack pointer, to be passed to nanobe_switch() or
 *         nanobe_sched_enqueue().
 */
void *nanobe_init_arg(nanobe_arg_t entry, void *arg, void *stack_top);

/**
 * @brief Switch to another nanobe context.
 *
 * Saves the callee-saved registers of the current context on its stack,
 * stores the resulting stack pointer in @p curr_sp and resumes the context
 * whose saved stack pointer is @p next_sp. Returns when some context switches
 * back to the saved @p curr_sp.
 *
 * Switching also releases the nanobe scheduler lock for the resumed context.
 *
 * @param next_sp Saved stack pointer of the context to switch to.
 * @param curr_sp Location to store the current context's stack pointer.
 */
void nanobe_switch(void *next_sp, void **curr_sp);

/**
 * @brief Enqueue a nanobe context in the nanobe scheduler ready queue.
 *
 * @param nanobe_sp Stack pointer returned by nanobe_init().
 */
void nanobe_sched_enqueue(void *nanobe_sp);

/**
 * @brief Yield to the next ready nanobe.
 *
 * The yielding context is put at the tail of the ready queue on the next
 * yield and the context at the head of the ready queue is resumed. Returns
 * immediately if no other context is ready or if the nanobe scheduler is
 * locked.
 */
void nanobe_sched_yield(void);

/**
 * @brief Lock the nanobe scheduler.
 *
 * @return Previous lock state, to be passed to nanobe_sched_unlock().
 */
uint8_t nanobe_sched_lock(void);

/**
 * @brief Unlock the nanobe scheduler.
 *
 * If a yield was requested (e.g. injected by an interrupt) while the
 * scheduler was locked, the yield is performed on unlock.
 *
 * @param lock Value returned by the matching nanobe_sched_lock().
 */
void nanobe_sched_unlock(uint8_t lock);

/**
 * @brief Put the current nanobe context to sleep.
 *
 * Yields to the other ready nanobes until @p timeout has expired. The
 * sleeping context remains in the round robin and polls its timeout each
 * time it is resumed; if no other context is ready, it busy waits.
 *
 * @param timeout Time to sleep; K_FOREVER sleeps forever.
 */
void nanobe_sleep(k_timeout_t timeout);

/**
 * @brief Put the current nanobe context to sleep, in milliseconds.
 *
 * @param ms Milliseconds to sleep.
 */
static inline void nanobe_msleep(int32_t ms)
{
	nanobe_sleep(K_MSEC(ms));
}

/**
 * @brief Initialize a nanobe semaphore.
 *
 * @param sem Semaphore.
 * @param initial_count Initial count, at most @p limit.
 * @param limit Maximum count, non-zero.
 *
 * @retval 0 Initialized.
 * @retval -EINVAL Invalid count or limit.
 */
int nanobe_sem_init(struct nanobe_sem *sem, uint32_t initial_count,
		    uint32_t limit);

/**
 * @brief Take a nanobe semaphore.
 *
 * Yields to the other ready nanobes until the semaphore is available or
 * @p timeout expires. Waiters are not queued: a semaphore given while
 * several contexts wait is taken by the first of them to be resumed.
 *
 * @param sem Semaphore.
 * @param timeout K_NO_WAIT, K_FOREVER or a timeout.
 *
 * @retval 0 Taken.
 * @retval -EBUSY Not available and @p timeout is K_NO_WAIT.
 * @retval -EAGAIN Timed out.
 */
int nanobe_sem_take(struct nanobe_sem *sem, k_timeout_t timeout);

/**
 * @brief Give a nanobe semaphore.
 *
 * May be called from interrupt service routines. The count is not
 * incremented beyond the limit.
 *
 * @param sem Semaphore.
 */
void nanobe_sem_give(struct nanobe_sem *sem);

/**
 * @brief Get a nanobe semaphore's count.
 *
 * @param sem Semaphore.
 *
 * @return Current count.
 */
static inline uint32_t nanobe_sem_count_get(const struct nanobe_sem *sem)
{
	return sem->count;
}

#if defined(CONFIG_NANOBE_INJECTION) || defined(__DOXYGEN__)
/**
 * @brief Inject a call into the interrupted nanobe owner thread.
 *
 * Call from an interrupt service routine. When the interrupt returns to the
 * nanobe owner thread (i.e. it is not nested and the owner thread was the
 * one interrupted), the exception return address is redirected so that
 * @p callee is executed in thread mode, on the stack of the interrupted
 * context, after which the interrupted code resumes transparently.
 *
 * Typically @p callee is nanobe_sched_yield(), which gives pre-emptive
 * nanobe scheduling driven by interrupts.
 *
 * If an injected call is already in progress, it is re-run once it
 * completes. If the nanobe scheduler is locked, a yield is performed on
 * nanobe_sched_unlock().
 *
 * @note Must not be called from zero-latency interrupts.
 *
 * @param callee Function to call in thread mode.
 *
 * @retval 0 Call injected, or deferred to an in-progress injection or to the
 *           scheduler unlock.
 * @retval -EPERM Not returning to the nanobe owner thread, nothing done.
 * @retval -EBUSY Interrupted instruction cannot be resumed other than by
 *                exception return (IT block or interrupted LDM/STM
 *                continuation), nothing done; the caller may retry, e.g. by
 *                re-pending its interrupt.
 */
int nanobe_isr_inject(nanobe_t callee);
#endif /* CONFIG_NANOBE_INJECTION */

/**
 * @}
 */

/** @cond INTERNAL_HIDDEN */

/* Nanobe scheduler lock, see nanobe_sched_lock() */
extern volatile uint8_t z_nanobe_sgrd;

/* Prepare an initial nanobe frame, as nanobe_init_arg() but without
 * recording the nanobe owner thread.
 */
void *z_nanobe_frame_init(nanobe_arg_t entry, void *arg, void *stack_top);

/* Inject a call into the interrupted thread mode context, as
 * nanobe_isr_inject() but into any thread and regardless of the nanobe
 * scheduler lock. During an in-progress injection, @p callee replaces the
 * call to be re-run.
 */
int z_nanobe_isr_inject(nanobe_t callee);

/* Discard the IT block and interrupted LDM/STM continuation state of the
 * interrupted thread mode context, which must never be resumed.
 */
void z_nanobe_isr_iciit_discard(void);

/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_KERNEL_NANOBE_H_ */
