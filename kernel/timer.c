/*
 * Copyright (c) 1997-2016 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include <zephyr/init.h>
#include <ksched.h>
#include <zephyr/wait_q.h>
#include <zephyr/syscall_handler.h>
#include <stdbool.h>
#include <zephyr/spinlock.h>

static struct k_spinlock lock;

/**
 * @brief Handle expiration of a kernel timer object.
 *
 * @param t  Timeout used by the timer.
 */
void z_timer_expiration_handler(struct _timeout *t)
{
	struct k_timer *timer = CONTAINER_OF(t, struct k_timer, timeout);
	struct k_thread *thread;
	k_spinlock_key_t key = k_spin_lock(&lock);

	/*
	 * if the timer is periodic, start it again; don't add _TICK_ALIGN
	 * since we're already aligned to a tick boundary
	 */
	if (!K_TIMEOUT_EQ(timer->period, K_NO_WAIT) &&
	    !K_TIMEOUT_EQ(timer->period, K_FOREVER)) {
		k_timeout_t next = timer->period;

#ifdef CONFIG_TIMEOUT_64BIT
		/* Exploit the fact that uptime during a kernel
		 * timeout handler reflects the time of the scheduled
		 * event and not real time to get some inexpensive
		 * protection against late interrupts.  If we're
		 * delayed for any reason, we still end up calculating
		 * the next expiration as a regular stride from where
		 * we "should" have run.  Requires absolute timeouts.
		 * (Note offset by one: we're nominally at the
		 * beginning of a tick, so need to defeat the "round
		 * down" behavior on timeout addition).
		 */
		next = K_TIMEOUT_ABS_TICKS(k_uptime_ticks() + 1
					   + timer->period.ticks);
#endif
		z_add_timeout(&timer->timeout, z_timer_expiration_handler,
			      next);
	}

	/* update timer's status */
	timer->status += 1U;

	/* invoke timer expiry function */
	if (timer->expiry_fn != NULL) {
		/* Unlock for user handler. */
		k_spin_unlock(&lock, key);
		timer->expiry_fn(timer);
		key = k_spin_lock(&lock);
	}

	if (!IS_ENABLED(CONFIG_MULTITHREADING)) {
		k_spin_unlock(&lock, key);
		return;
	}

	thread = z_waitq_head(&timer->wait_q);

	if (thread == NULL) {
		k_spin_unlock(&lock, key);
		return;
	}

	z_unpend_thread_no_timeout(thread);

	arch_thread_return_value_set(thread, 0);

	k_spin_unlock(&lock, key);

	z_ready_thread(thread);
}


void k_timer_init(struct k_timer *timer,
			 k_timer_expiry_t expiry_fn,
			 k_timer_stop_t stop_fn)
{
	timer->expiry_fn = expiry_fn;
	timer->stop_fn = stop_fn;
	timer->status = 0U;

	if (IS_ENABLED(CONFIG_MULTITHREADING)) {
		z_waitq_init(&timer->wait_q);
	}

	z_init_timeout(&timer->timeout);

	SYS_PORT_TRACING_OBJ_INIT(k_timer, timer);

	timer->user_data = NULL;

	z_object_init(timer);
}


void z_impl_k_timer_start(struct k_timer *timer, k_timeout_t duration,
			  k_timeout_t period)
{
	SYS_PORT_TRACING_OBJ_FUNC(k_timer, start, timer);

	if (K_TIMEOUT_EQ(duration, K_FOREVER)) {
		return;
	}

	/* z_add_timeout() always adds one to the incoming tick count
	 * to round up to the next tick (by convention it waits for
	 * "at least as long as the specified timeout"), but the
	 * period interval is always guaranteed to be reset from
	 * within the timer ISR, so no round up is desired.  Subtract
	 * one.
	 *
	 * Note that the duration (!) value gets the same treatment
	 * for backwards compatibility.  This is unfortunate
	 * (i.e. k_timer_start() doesn't treat its initial sleep
	 * argument the same way k_sleep() does), but historical.  The
	 * timer_api test relies on this behavior.
	 */
	if (!K_TIMEOUT_EQ(period, K_FOREVER) && period.ticks != 0 &&
	    Z_TICK_ABS(period.ticks) < 0) {
		period.ticks = MAX(period.ticks - 1, 1);
	}
	if (Z_TICK_ABS(duration.ticks) < 0) {
		duration.ticks = MAX(duration.ticks - 1, 0);
	}

	(void)z_abort_timeout(&timer->timeout);
	timer->period = period;
	timer->status = 0U;

	z_add_timeout(&timer->timeout, z_timer_expiration_handler,
		     duration);
}

#ifdef CONFIG_USERSPACE
static inline void z_vrfy_k_timer_start(struct k_timer *timer,
					k_timeout_t duration,
					k_timeout_t period)
{
	Z_OOPS(Z_SYSCALL_OBJ(timer, K_OBJ_TIMER));
	z_impl_k_timer_start(timer, duration, period);
}
#include <syscalls/k_timer_start_mrsh.c>
#endif

void z_impl_k_timer_stop(struct k_timer *timer)
{
	SYS_PORT_TRACING_OBJ_FUNC(k_timer, stop, timer);

	int inactive = z_abort_timeout(&timer->timeout) != 0;

	if (inactive) {
		return;
	}

	if (timer->stop_fn != NULL) {
		timer->stop_fn(timer);
	}

	if (IS_ENABLED(CONFIG_MULTITHREADING)) {
		struct k_thread *pending_thread = z_unpend1_no_timeout(&timer->wait_q);

		if (pending_thread != NULL) {
			z_ready_thread(pending_thread);
			z_reschedule_unlocked();
		}
	}
}

#ifdef CONFIG_USERSPACE
static inline void z_vrfy_k_timer_stop(struct k_timer *timer)
{
	Z_OOPS(Z_SYSCALL_OBJ(timer, K_OBJ_TIMER));
	z_impl_k_timer_stop(timer);
}
#include <syscalls/k_timer_stop_mrsh.c>
#endif

uint32_t z_impl_k_timer_status_get(struct k_timer *timer)
{
	k_spinlock_key_t key = k_spin_lock(&lock);
	uint32_t result = timer->status;

	timer->status = 0U;
	k_spin_unlock(&lock, key);

	return result;
}

#ifdef CONFIG_USERSPACE
static inline uint32_t z_vrfy_k_timer_status_get(struct k_timer *timer)
{
	Z_OOPS(Z_SYSCALL_OBJ(timer, K_OBJ_TIMER));
	return z_impl_k_timer_status_get(timer);
}
#include <syscalls/k_timer_status_get_mrsh.c>
#endif

uint32_t z_impl_k_timer_status_sync(struct k_timer *timer)
{
	__ASSERT(!arch_is_in_isr(), "");
	SYS_PORT_TRACING_OBJ_FUNC_ENTER(k_timer, status_sync, timer);

	if (!IS_ENABLED(CONFIG_MULTITHREADING)) {
		uint32_t result;

		do {
			k_spinlock_key_t key = k_spin_lock(&lock);

			if (!z_is_inactive_timeout(&timer->timeout)) {
				result = *(volatile uint32_t *)&timer->status;
				timer->status = 0U;
				k_spin_unlock(&lock, key);
				if (result > 0) {
					break;
				}
			} else {
				result = timer->status;
				k_spin_unlock(&lock, key);
				break;
			}
		} while (true);

		return result;
	}

	k_spinlock_key_t key = k_spin_lock(&lock);
	uint32_t result = timer->status;

	if (result == 0U) {
		if (!z_is_inactive_timeout(&timer->timeout)) {
			SYS_PORT_TRACING_OBJ_FUNC_BLOCKING(k_timer, status_sync, timer, K_FOREVER);

			/* wait for timer to expire or stop */
			(void)z_pend_curr(&lock, key, &timer->wait_q, K_FOREVER);

			/* get updated timer status */
			key = k_spin_lock(&lock);
			result = timer->status;
		} else {
			/* timer is already stopped */
		}
	} else {
		/* timer has already expired at least once */
	}

	timer->status = 0U;
	k_spin_unlock(&lock, key);

	/**
	 * @note	New tracing hook
	 */
	SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_timer, status_sync, timer, result);

	return result;
}

#ifdef CONFIG_USERSPACE
static inline uint32_t z_vrfy_k_timer_status_sync(struct k_timer *timer)
{
	Z_OOPS(Z_SYSCALL_OBJ(timer, K_OBJ_TIMER));
	return z_impl_k_timer_status_sync(timer);
}
#include <syscalls/k_timer_status_sync_mrsh.c>

static inline k_ticks_t z_vrfy_k_timer_remaining_ticks(
						const struct k_timer *timer)
{
	Z_OOPS(Z_SYSCALL_OBJ(timer, K_OBJ_TIMER));
	return z_impl_k_timer_remaining_ticks(timer);
}
#include <syscalls/k_timer_remaining_ticks_mrsh.c>

static inline k_ticks_t z_vrfy_k_timer_expires_ticks(
						const struct k_timer *timer)
{
	Z_OOPS(Z_SYSCALL_OBJ(timer, K_OBJ_TIMER));
	return z_impl_k_timer_expires_ticks(timer);
}
#include <syscalls/k_timer_expires_ticks_mrsh.c>

static inline void *z_vrfy_k_timer_user_data_get(const struct k_timer *timer)
{
	Z_OOPS(Z_SYSCALL_OBJ(timer, K_OBJ_TIMER));
	return z_impl_k_timer_user_data_get(timer);
}
#include <syscalls/k_timer_user_data_get_mrsh.c>

static inline void z_vrfy_k_timer_user_data_set(struct k_timer *timer,
						void *user_data)
{
	Z_OOPS(Z_SYSCALL_OBJ(timer, K_OBJ_TIMER));
	z_impl_k_timer_user_data_set(timer, user_data);
}
#include <syscalls/k_timer_user_data_set_mrsh.c>

#endif
