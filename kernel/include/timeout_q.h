/*
 * Copyright (c) 2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_KERNEL_INCLUDE_TIMEOUT_Q_H_
#define ZEPHYR_KERNEL_INCLUDE_TIMEOUT_Q_H_

/**
 * @file
 * @brief timeout queue for threads on kernel objects
 */

#include <zephyr/kernel.h>

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_SYS_CLOCK_EXISTS

#ifdef CONFIG_TIMEOUT_USE_MIN_HEAP

/*
 * abs_ticks sentinels for the heap backend.
 *
 * Real abs_ticks values are always >= 1 (absolute tick counts from boot).
 * INT64_MIN and INT64_MIN+1 are unreachable as valid absolute tick counts.
 *
 * Note: K_TICKS(INT64_MIN) is a valid k_timeout_t.ticks value produced by
 * K_TIMEOUT_SUM overflow, but it maps to abs_ticks = INT64_MAX-1 via
 * Z_TICK_ABS() — never stored as INT64_MIN in abs_ticks.
 */
#define TIMEOUT_ABS_TICKS_ANNOUNCING  INT64_MIN
#define TIMEOUT_ABS_TICKS_ABORTED    (INT64_MIN + 1)

static inline void z_init_timeout(struct _timeout *to)
{
	to->heap_handle.idx = 0U;
}

/**
 * Adds the timeout to the queue.
 *
 * @return Absolute tick value when timeout will expire.
 */
k_ticks_t z_add_timeout(struct _timeout *to, _timeout_func_t fn, k_timeout_t
			timeout);

/**
 * Mark the timeout node as aborted
 *
 * @return 0 on success, -EINVAL on error.
 */
int z_abort_timeout(struct _timeout *to);

/* Backend-specific announcing check used by z_is_timeout_handler_canceled(). */
#define _TIMEOUT_IS_ANNOUNCING(to) ((to)->abs_ticks == TIMEOUT_ABS_TICKS_ANNOUNCING)

/**
 * Checks if a timeout node is active
 *
 * @return Return true if inactive (idle, announcing, or aborted)
 */
static inline bool z_is_inactive_timeout(const struct _timeout *to)
{
	return to->heap_handle.idx == 0U;
}

/**
 * Checks if the timeout was aborted.
 *
 * @return Return true if aborted.
 */
static inline bool z_is_aborted_timeout(const struct _timeout *to)
{
	return to->abs_ticks == TIMEOUT_ABS_TICKS_ABORTED;
}

#else /* !CONFIG_TIMEOUT_USE_MIN_HEAP : dlist backend */

/* Value written to dticks when timeout is aborted. */
#define TIMEOUT_DTICKS_ABORTED (IS_ENABLED(CONFIG_TIMEOUT_64BIT) ? INT64_MIN : INT32_MIN)

/* Value written to dticks when timeout is being announced (handler pending). */
#define TIMEOUT_DTICKS_ANNOUNCING (TIMEOUT_DTICKS_ABORTED + 1)

static inline void z_init_timeout(struct _timeout *to)
{
	sys_dnode_init(&to->node);
}

/* Adds the timeout to the queue.
 *
 * @return Absolute tick value when timeout will expire.
 */
k_ticks_t z_add_timeout(struct _timeout *to, _timeout_func_t fn, k_timeout_t timeout);

int z_abort_timeout(struct _timeout *to);

/* Backend-specific announcing check used by z_is_timeout_handler_canceled(). */
#define _TIMEOUT_IS_ANNOUNCING(to) ((to)->dticks == TIMEOUT_DTICKS_ANNOUNCING)

static inline bool z_is_inactive_timeout(const struct _timeout *to)
{
	return !sys_dnode_is_linked(&to->node);
}

static inline bool z_is_aborted_timeout(const struct _timeout *to)
{
	/* When timeout is aborted then dticks is set to special value. */
	return to->dticks == TIMEOUT_DTICKS_ABORTED;
}

#endif /* CONFIG_TIMEOUT_USE_MIN_HEAP */

/* Determine if the timeout handler should continue.
 *
 * The routine sys_clock_announce() both removes the timeout from the timeout
 * list/heap and unlocks interrupts prior to invoking the timeout handler. This
 * provides a small gap where another ISR (or thread on another CPU) could
 * do something that would cause the timeout handler to be canceled (e.g.
 * restarting a k_timer). This routine allows the timeout handler to check if
 * it should continue or if it should just return immediately. It assumes that
 * the handler has already locked interrupts / relevant spinlocks.
 *
 * Testing if the timeout node is inactive is insufficient as the timeout
 * struct could have been reused.
 */
static inline bool z_is_timeout_handler_canceled(const struct _timeout *to)
{
	return !_TIMEOUT_IS_ANNOUNCING(to);
}

static inline void z_init_thread_timeout(struct _thread_base *thread_base)
{
	z_init_timeout(&thread_base->timeout);
}

extern void z_thread_timeout(struct _timeout *timeout);

static inline k_ticks_t z_add_thread_timeout(struct k_thread *thread, k_timeout_t ticks)
{
	return z_add_timeout(&thread->base.timeout, z_thread_timeout, ticks);
}

static inline void z_abort_thread_timeout(struct k_thread *thread)
{
	z_abort_timeout(&thread->base.timeout);
}

static inline bool z_is_aborted_thread_timeout(struct k_thread *thread)
{

	return z_is_aborted_timeout(&thread->base.timeout);
}

int32_t z_get_next_timeout_expiry(void);

k_ticks_t z_timeout_remaining(const struct _timeout *timeout);

#else

/* Stubs when !CONFIG_SYS_CLOCK_EXISTS */
#define z_init_thread_timeout(thread_base) do {} while (false)
#define z_abort_thread_timeout(to) do {} while (false)
#define z_is_aborted_thread_timeout(to) false
#define z_is_inactive_timeout(to) 1
#define z_is_aborted_timeout(to) false
#define z_get_next_timeout_expiry() ((int32_t) K_TICKS_FOREVER)
#define z_set_timeout_expiry(ticks, is_idle) do {} while (false)

static inline k_ticks_t z_add_thread_timeout(struct k_thread *thread, k_timeout_t ticks)
{
	ARG_UNUSED(thread);
	ARG_UNUSED(ticks);
	return 0;
}

#endif /* CONFIG_SYS_CLOCK_EXISTS */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_KERNEL_INCLUDE_TIMEOUT_Q_H_ */
