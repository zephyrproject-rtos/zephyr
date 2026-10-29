/*
 * Copyright (c) 1997-2016 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include <zephyr/init.h>
#include <zephyr/internal/syscall_handler.h>
#include <zephyr/sys/check.h>
#include <stdbool.h>
#include <zephyr/spinlock.h>
#include <ksched.h>
#include <wait_q.h>

static struct k_spinlock timer_lock;

/* The top bit of timer->status is set while an expiry is pending, from
 * k_timer_start() until k_timer_stop() or the last expiry. It is only
 * changed under timer_lock, unlike the timeout's queue linkage, which
 * the announcer changes under timeout_lock. The other bits count
 * expiries.
 */
#define TIMER_ARMED 0x80000000U

static inline bool timer_armed(const struct k_timer *timer)
{
	return (timer->status & TIMER_ARMED) != 0U;
}

/* Arm the timer and reset its expiry count. */
static inline void timer_arm(struct k_timer *timer)
{
	timer->status = TIMER_ARMED;
}

static inline void timer_disarm(struct k_timer *timer)
{
	timer->status &= ~TIMER_ARMED;
}

static inline uint32_t timer_count(const struct k_timer *timer)
{
	return timer->status & ~TIMER_ARMED;
}

/* Saturate within the count bits: a periodic timer nobody polls can reach
 * 2^31 expiries, and carrying into the armed bit would disarm it.
 */
static inline void timer_count_inc(struct k_timer *timer)
{
	if (timer_count(timer) != ~TIMER_ARMED) {
		timer->status += 1U;
	}
}

static inline void timer_count_clear(struct k_timer *timer)
{
	timer->status &= TIMER_ARMED;
}

#ifdef CONFIG_OBJ_CORE_TIMER
K_OBJ_TYPE_DEFINE(obj_type_timer, k_timer, K_OBJ_TYPE_TIMER_ID, NULL);
#endif /* CONFIG_OBJ_CORE_TIMER */

#if defined(CONFIG_TIMER_OBSERVER)
static inline void z_timer_observer_on_init(struct k_timer *timer)
{
	STRUCT_SECTION_FOREACH(k_timer_observer, obs) {
		if (obs->on_init != NULL) {
			obs->on_init(timer);
		}
	}
}

static inline void z_timer_observer_on_start(struct k_timer *timer, k_timeout_t duration,
					     k_timeout_t period)
{
	STRUCT_SECTION_FOREACH(k_timer_observer, obs) {
		if (obs->on_start != NULL) {
			obs->on_start(timer, duration, period);
		}
	}
}

static inline void z_timer_observer_on_stop(struct k_timer *timer)
{
	STRUCT_SECTION_FOREACH(k_timer_observer, obs) {
		if (obs->on_stop != NULL) {
			obs->on_stop(timer);
		}
	}
}

static inline void z_timer_observer_on_expiry(struct k_timer *timer)
{
	STRUCT_SECTION_FOREACH(k_timer_observer, obs) {
		if (obs->on_expiry != NULL) {
			obs->on_expiry(timer);
		}
	}
}
#else
#define z_timer_observer_on_init(timer) (void)0
#define z_timer_observer_on_start(timer, duration, period) (void)0
#define z_timer_observer_on_stop(timer) (void)0
#define z_timer_observer_on_expiry(timer) (void)0
#endif /* CONFIG_TIMER_OBSERVER */

/**
 * @brief Handle expiration of a kernel timer object.
 *
 * @param t  Timeout used by the timer.
 */
void z_timer_expiration_handler(struct _timeout *t)
{
	struct k_timer *timer = CONTAINER_OF(t, struct k_timer, timeout);
	struct k_thread *thread;
	k_spinlock_key_t key = k_spin_lock(&timer_lock);

	/* The timer may have been restarted or stopped between the
	 * announcer popping the timeout and us taking timer_lock:
	 *  - restarted: the timeout is queued again and fires later;
	 *  - stopped: k_timer_stop() cleared the armed bit and has
	 *    delivered the stop instead.
	 */
	if (!z_is_inactive_timeout(t) || !timer_armed(timer)) {
		k_spin_unlock(&timer_lock, key);
		return;
	}

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
		 */
		next = K_TIMEOUT_ABS_TICKS(k_uptime_ticks() + next.ticks);
#endif /* CONFIG_TIMEOUT_64BIT */
		z_add_timeout(&timer->timeout, z_timer_expiration_handler,
			      next);
	} else {
		timer_disarm(timer);
	}

	/* update timer's status */
	timer_count_inc(timer);

	z_timer_observer_on_expiry(timer);

	/* invoke timer expiry function */
	if (timer->expiry_fn != NULL) {
		k_timer_expiry_t expiry_fn = timer->expiry_fn;

		/* Unlock for user handler. */
		k_spin_unlock(&timer_lock, key);

		SYS_PORT_TRACING_OBJ_FUNC_ENTER(k_timer, expiry, timer);

		expiry_fn(timer);

		SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_timer, expiry, timer);

		key = k_spin_lock(&timer_lock);
	}

	if (!IS_ENABLED(CONFIG_MULTITHREADING)) {
		k_spin_unlock(&timer_lock, key);
		return;
	}

	LOCK_SCHED_SPINLOCK {
		thread = z_waitq_head_locked(&timer->wait_q);

		if (thread != NULL) {
			unpend_thread_no_timeout(thread);
			arch_thread_return_value_set(thread, 0);
			z_sched_ready_locked(thread);
		}
	}

	k_spin_unlock(&timer_lock, key);

}


int z_timer_cleanup(struct k_timer *timer, __maybe_unused bool locked)
{
	/* Not callable from an ISR: this is the one timer path that can
	 * spin waiting for an in-flight handler, and an ISR spinning here
	 * could starve the very CPU the handler needs to make progress.
	 */
	__ASSERT(!arch_is_in_isr(), "");

	SYS_PORT_TRACING_OBJ_FUNC_ENTER(k_timer, cleanup, timer);

	int ret = 0;
	k_spinlock_key_t key;

	/* Refuse if anyone is still pending on the timer's wait queue
	 * (e.g. via k_timer_status_sync()): freeing the storage would
	 * leave dangling pended_on pointers.
	 */
retry:
	key = k_spin_lock(&timer_lock);

	CHECKIF(locked && (z_waitq_head_locked(&timer->wait_q) != NULL)) {
		ret = -EAGAIN;
		goto out;
	}

	CHECKIF(!locked && (z_waitq_head(&timer->wait_q) != NULL)) {
		ret = -EAGAIN;
		goto out;
	}

	/* Cancel the timeout AND wait for any in-flight expiration
	 * handler on another CPU to complete before returning. Unlike
	 * k_timer_stop(), we do not call any user stop_fn here: the
	 * caller is about to free the storage and there is no further
	 * consumer of the timer.
	 *
	 * This is the one timer path that waits on the handler (it must,
	 * before the storage is freed). The wait can only stall if the
	 * handler itself blocks on this CPU making progress -- e.g. an
	 * expiry_fn that aborts a thread running here. Freeing a timer
	 * whose handler does that is a caller bug; ordinary stop/start do
	 * not wait and so cannot stall.
	 */
	timer_disarm(timer);
	if (z_try_abort_timeout(&timer->timeout) == -EAGAIN) {
		k_spin_unlock(&timer_lock, key);
		goto retry;
	}

out:
	k_spin_unlock(&timer_lock, key);

#ifdef CONFIG_OBJ_CORE_TIMER
	if (ret == 0) {
		k_obj_core_unlink(K_OBJ_CORE(timer));
	}
#endif /* CONFIG_OBJ_CORE_TIMER */

	SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_timer, cleanup, timer, ret);

	return ret;
}

int k_timer_cleanup(struct k_timer *timer)
{
	return z_timer_cleanup(timer, false);
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

	k_object_init(timer);

#ifdef CONFIG_OBJ_CORE_TIMER
	k_obj_core_init_and_link(K_OBJ_CORE(timer), &obj_type_timer);
#endif /* CONFIG_OBJ_CORE_TIMER */

	z_timer_observer_on_init(timer);
}


void z_impl_k_timer_start(struct k_timer *timer, k_timeout_t duration,
			  k_timeout_t period)
{
	SYS_PORT_TRACING_OBJ_FUNC(k_timer, start, timer, duration, period);

	if (K_TIMEOUT_EQ(duration, K_FOREVER)) {
		return;
	}

	/* Hold the timer lock across abort + add to serialize against a
	 * concurrent k_timer_start on the same timer. A handler in flight
	 * and not yet past its check bails, as the re-arm below queues the
	 * timeout again. We do not wait for it.
	 */
	k_spinlock_key_t key = k_spin_lock(&timer_lock);

	(void)z_try_abort_timeout(&timer->timeout);

	timer->period = period;
	timer_arm(timer);

	z_add_timeout(&timer->timeout, z_timer_expiration_handler, duration);

	z_timer_observer_on_start(timer, duration, period);

	k_spin_unlock(&timer_lock, key);
}

#ifdef CONFIG_USERSPACE
static inline void z_vrfy_k_timer_start(struct k_timer *timer,
					k_timeout_t duration,
					k_timeout_t period)
{
	K_OOPS(K_SYSCALL_OBJ(timer, K_OBJ_TIMER));
	z_impl_k_timer_start(timer, duration, period);
}
#include <zephyr/syscalls/k_timer_start_mrsh.c>
#endif /* CONFIG_USERSPACE */

void z_impl_k_timer_stop(struct k_timer *timer)
{
	SYS_PORT_TRACING_OBJ_FUNC(k_timer, stop, timer);

	k_spinlock_key_t key = k_spin_lock(&timer_lock);

	if (!timer_armed(timer)) {
		/* Not started, already stopped, or its last expiry was
		 * delivered. Nothing to stop.
		 */
		k_spin_unlock(&timer_lock, key);
		return;
	}

	/* The stop wins over any expiry not yet delivered: a handler in
	 * flight bails once it sees the armed bit cleared.
	 */
	timer_disarm(timer);
	(void)z_try_abort_timeout(&timer->timeout);

	z_timer_observer_on_stop(timer);

	if (timer->stop_fn != NULL) {
		SYS_PORT_TRACING_OBJ_FUNC_ENTER(k_timer, stop_fn_expiry, timer);
		k_spin_unlock(&timer_lock, key);

		timer->stop_fn(timer);

		key = k_spin_lock(&timer_lock);

		SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_timer, stop_fn_expiry, timer);
	}

	if (!IS_ENABLED(CONFIG_MULTITHREADING)) {
		k_spin_unlock(&timer_lock, key);
		return;

	}

	struct k_thread *pending_thread = z_unpend1_no_timeout(&timer->wait_q);

	if (pending_thread != NULL) {
		z_ready_thread(pending_thread);
		z_reschedule(&timer_lock, key);
	} else {
		k_spin_unlock(&timer_lock, key);
	}
}

#ifdef CONFIG_USERSPACE
static inline void z_vrfy_k_timer_stop(struct k_timer *timer)
{
	K_OOPS(K_SYSCALL_OBJ(timer, K_OBJ_TIMER));
	z_impl_k_timer_stop(timer);
}
#include <zephyr/syscalls/k_timer_stop_mrsh.c>
#endif /* CONFIG_USERSPACE */

uint32_t z_impl_k_timer_status_get(struct k_timer *timer)
{
	k_spinlock_key_t key = k_spin_lock(&timer_lock);
	uint32_t result = timer_count(timer);

	timer_count_clear(timer);
	k_spin_unlock(&timer_lock, key);

	return result;
}

#ifdef CONFIG_USERSPACE
static inline uint32_t z_vrfy_k_timer_status_get(struct k_timer *timer)
{
	K_OOPS(K_SYSCALL_OBJ(timer, K_OBJ_TIMER));
	return z_impl_k_timer_status_get(timer);
}
#include <zephyr/syscalls/k_timer_status_get_mrsh.c>
#endif /* CONFIG_USERSPACE */

uint32_t z_impl_k_timer_status_sync(struct k_timer *timer)
{
	__ASSERT(!arch_is_in_isr(), "");
	SYS_PORT_TRACING_OBJ_FUNC_ENTER(k_timer, status_sync, timer);

	if (!IS_ENABLED(CONFIG_MULTITHREADING)) {
		uint32_t result;

		do {
			unsigned int key = irq_lock();

			result = timer_count(timer);
			timer_count_clear(timer);
			if (result > 0U || !timer_armed(timer)) {
				irq_unlock(key);
				break;
			}
			k_cpu_atomic_idle(key);
		} while (true);

		return result;
	}

	k_spinlock_key_t key = k_spin_lock(&timer_lock);
	uint32_t result = timer_count(timer);

	if (result == 0U) {
		if (timer_armed(timer)) {
			SYS_PORT_TRACING_OBJ_FUNC_BLOCKING(k_timer, status_sync, timer, K_FOREVER);

			/* wait for timer to expire or stop */
			(void)z_pend_curr(&timer_lock, key, &timer->wait_q, K_FOREVER);

			/* get updated timer status */
			key = k_spin_lock(&timer_lock);
		} else {
			/* timer is already stopped */
		}
	} else {
		/* timer has already expired at least once */
	}

	result = timer_count(timer);
	timer_count_clear(timer);
	k_spin_unlock(&timer_lock, key);

	/**
	 * @note	New tracing hook
	 */
	SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_timer, status_sync, timer, result);

	return result;
}

#ifdef CONFIG_USERSPACE
static inline uint32_t z_vrfy_k_timer_status_sync(struct k_timer *timer)
{
	K_OOPS(K_SYSCALL_OBJ(timer, K_OBJ_TIMER));
	return z_impl_k_timer_status_sync(timer);
}
#include <zephyr/syscalls/k_timer_status_sync_mrsh.c>

static inline k_ticks_t z_vrfy_k_timer_remaining_ticks(
						const struct k_timer *timer)
{
	K_OOPS(K_SYSCALL_OBJ(timer, K_OBJ_TIMER));
	return z_impl_k_timer_remaining_ticks(timer);
}
#include <zephyr/syscalls/k_timer_remaining_ticks_mrsh.c>

static inline k_ticks_t z_vrfy_k_timer_expires_ticks(
						const struct k_timer *timer)
{
	K_OOPS(K_SYSCALL_OBJ(timer, K_OBJ_TIMER));
	return z_impl_k_timer_expires_ticks(timer);
}
#include <zephyr/syscalls/k_timer_expires_ticks_mrsh.c>

static inline void *z_vrfy_k_timer_user_data_get(const struct k_timer *timer)
{
	K_OOPS(K_SYSCALL_OBJ(timer, K_OBJ_TIMER));
	return z_impl_k_timer_user_data_get(timer);
}
#include <zephyr/syscalls/k_timer_user_data_get_mrsh.c>

static inline void z_vrfy_k_timer_user_data_set(struct k_timer *timer,
						void *user_data)
{
	K_OOPS(K_SYSCALL_OBJ(timer, K_OBJ_TIMER));
	z_impl_k_timer_user_data_set(timer, user_data);
}
#include <zephyr/syscalls/k_timer_user_data_set_mrsh.c>

#endif /* CONFIG_USERSPACE */
