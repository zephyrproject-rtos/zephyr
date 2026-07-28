/*
 * Copyright (c) 1997-2016 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include <limits.h>
#include <stdbool.h>

#include <zephyr/init.h>
#include <zephyr/internal/syscall_handler.h>
#include <zephyr/sys/check.h>
#include <stdbool.h>
#include <zephyr/logging/log.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/timeutil.h>
#include <ksched.h>
#include <wait_q.h>

static struct k_spinlock timer_lock;

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

#ifdef CONFIG_TIMER_CLOCK
/* convert a non-negative timespec to ticks (ceil), saturating at K_TICK_MAX */
static uint64_t z_timer_ts_to_ticks(const struct timespec *ts)
{
	if (ts->tv_sec < 0) {
		return 0;
	}

	if ((uint64_t)ts->tv_sec >= (uint64_t)K_TICK_MAX / CONFIG_SYS_CLOCK_TICKS_PER_SEC) {
		return (uint64_t)K_TICK_MAX;
	}

	return k_sec_to_ticks_ceil64((uint64_t)ts->tv_sec) + k_ns_to_ticks_ceil64(ts->tv_nsec);
}

static inline bool z_timer_ts_is_zero(const struct timespec *ts)
{
	return (ts->tv_sec == 0) && (ts->tv_nsec == 0);
}
#endif /* CONFIG_TIMER_CLOCK */

#ifdef CONFIG_TIMER_REALTIME
/* armed absolute-realtime timers, re-targeted on sys_clock_settime(); guarded by 'lock' */
static sys_slist_t rt_timer_list = SYS_SLIST_STATIC_INIT(&rt_timer_list);

/* callers hold 'lock' */
static void z_timer_rt_register(struct k_timer *timer)
{
	if ((timer->flags & Z_TIMER_FLAG_REALTIME) == 0U) {
		timer->flags |= Z_TIMER_FLAG_REALTIME;
		sys_slist_append(&rt_timer_list, &timer->rt_node);
	}
}

/* callers hold 'lock' */
static void z_timer_rt_deregister(struct k_timer *timer)
{
	if ((timer->flags & Z_TIMER_FLAG_REALTIME) != 0U) {
		timer->flags &= (uint8_t)~Z_TIMER_FLAG_REALTIME;
		(void)sys_slist_find_and_remove(&rt_timer_list, &timer->rt_node);
	}
}
#else
#define z_timer_rt_register(timer) (void)0
#define z_timer_rt_deregister(timer) (void)0
#endif /* CONFIG_TIMER_REALTIME */

#ifdef CONFIG_TIMER_SIGNAL
LOG_MODULE_REGISTER(k_timer, CONFIG_KERNEL_LOG_LEVEL);

/* called from the expiry path with the timer lock held (timer lock -> k_sig_lock order) */
static void z_timer_signal_generate(struct k_timer *timer)
{
	if (timer->sig_target == NULL) {
		return;
	}

	if (!atomic_cas(&timer->sig_pending, 0, 1)) {
		/* the previous instance is still pending: counted as overrun at delivery */
		return;
	}

	/* uptime in a timeout handler is pinned to the scheduled expiry: an exact grid stamp */
	timer->sig_gen_ticks = (uint64_t)sys_clock_tick_get();
	timer->sig_freeze_ticks = 0U;

	if ((timer->flags & Z_TIMER_FLAG_EXACT) != 0U) {
		timer->sig_gen_period = timer->period_ticks;
	} else if (!K_TIMEOUT_EQ(timer->period, K_NO_WAIT) &&
		   !K_TIMEOUT_EQ(timer->period, K_FOREVER)) {
		timer->sig_gen_period = timer->period.ticks;
	} else {
		timer->sig_gen_period = 0;
	}

	if (z_sig_queue_timer(timer, timer->sig_target, timer->sig_signo, timer->sig_value) != 0) {
		/* best-effort: this period's signal is lost; retried on the next expiry */
		atomic_clear(&timer->sig_pending);
		timer->sig_missed++;
		LOG_DBG_RATELIMIT("timer %p: %u expiry signal(s) dropped for thread %p "
				  "(signal queue full; see SIGNAL_QUEUE_SIZE)",
				  timer, timer->sig_missed, timer->sig_target);
	}
}

/* stamp the disarm time so a pending signal's overrun stops accumulating */
static void z_timer_sig_freeze(struct k_timer *timer)
{
	if ((atomic_get(&timer->sig_pending) != 0) && (timer->sig_freeze_ticks == 0U)) {
		timer->sig_freeze_ticks = (uint64_t)sys_clock_tick_get();
	}
}
#else
#define z_timer_signal_generate(timer) (void)0
#define z_timer_sig_freeze(timer) (void)0
#endif /* CONFIG_TIMER_SIGNAL */

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

	/* A same-CPU IRQ may have raced with our dispatch between
	 * sys_clock_announce() popping us off the queue and us taking
	 * timer.c::lock. Two cases:
	 *  - The timer was restarted (z_add_timeout re-queued the timeout):
	 *    it is active again, and the new schedule fires later.
	 *  - The timer was stopped (z_try_abort_timeout marked the
	 *    in-flight slot superseded): bail without firing expiry_fn.
	 */
	if (!z_is_inactive_timeout(t) ||
	    z_timeout_inflight_superseded(t)) {
		k_spin_unlock(&timer_lock, key);
		return;
	}

	bool exact = false;

#ifdef CONFIG_TIMER_CLOCK
	exact = (timer->flags & Z_TIMER_FLAG_EXACT) != 0U;
	if (exact) {
		/*
		 * On first expiry an absolute-realtime timer leaves the re-target
		 * registry: its interval continues on the monotonic grid, and
		 * subsequent sys_clock_settime() calls no longer affect it.
		 */
		z_timer_rt_deregister(timer);

		if (timer->period_ticks > 0) {
			/*
			 * Advance along the exact grid anchored at the initial expiry.
			 * Uptime observed inside a timeout handler is pinned to the
			 * scheduled expiry, so a late or coalesced (tickless) wakeup
			 * still re-arms on-grid; if more than one period has already
			 * elapsed, skip ahead instead of replaying missed expiries.
			 */
			uint64_t now = (uint64_t)k_uptime_ticks();
			uint64_t next_tick = timer->expiry_ticks + (uint64_t)timer->period_ticks;

			if (next_tick <= now) {
				uint64_t missed = (now - timer->expiry_ticks) /
						  (uint64_t)timer->period_ticks;

				next_tick = timer->expiry_ticks +
					    (missed + 1U) * (uint64_t)timer->period_ticks;
			}
			timer->expiry_ticks = next_tick;
			z_add_timeout(&timer->timeout, z_timer_expiration_handler,
				      K_TIMEOUT_ABS_TICKS(next_tick));
		}
	}
#endif /* CONFIG_TIMER_CLOCK */

	/*
	 * if the timer is periodic, start it again; don't add _TICK_ALIGN
	 * since we're already aligned to a tick boundary
	 */
	if (!exact && !K_TIMEOUT_EQ(timer->period, K_NO_WAIT) &&
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
	}

	/* update timer's status */
	timer->status += 1U;

	z_timer_observer_on_expiry(timer);

	/* generate the expiry signal (one-pending semantics), if one is attached */
	z_timer_signal_generate(timer);

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

#ifdef CONFIG_TIMER_CLOCK
	timer->clock_id = SYS_CLOCK_MONOTONIC;
	timer->flags = 0U;
	timer->expiry_ticks = 0U;
	timer->period_ticks = 0;
#endif /* CONFIG_TIMER_CLOCK */

#ifdef CONFIG_TIMER_SIGNAL
	timer->sig_target = NULL;
	timer->sig_signo = 0;
	timer->sig_value.sival_ptr = NULL;
	atomic_clear(&timer->sig_pending);
	atomic_clear(&timer->sig_overrun);
	timer->sig_gen_ticks = 0U;
	timer->sig_gen_period = 0;
	timer->sig_freeze_ticks = 0U;
	timer->sig_missed = 0U;
#endif /* CONFIG_TIMER_SIGNAL */

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
	 * concurrent k_timer_start on the same timer. An in-flight handler
	 * (z_try_abort_timeout() returning non-zero) is flagged superseded
	 * and will bail; the re-arm below also re-links the node, which
	 * makes a not-yet-committed handler bail too. Either way we do not
	 * wait for it.
	 */
	k_spinlock_key_t key = k_spin_lock(&timer_lock);

#ifdef CONFIG_TIMER_CLOCK
	/* a legacy start resumes legacy timer semantics */
	z_timer_rt_deregister(timer);
	timer->flags &= (uint8_t)~Z_TIMER_FLAG_EXACT;
#endif /* CONFIG_TIMER_CLOCK */

	(void)z_try_abort_timeout(&timer->timeout);

	timer->period = period;
	timer->status = 0U;

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

	/* a stopped timer must not be re-armed by a realtime clock change */
	z_timer_rt_deregister(timer);
	z_timer_sig_freeze(timer);

	if (z_try_abort_timeout(&timer->timeout) != 0) {
		/* Not removed from the queue: either the timer was not
		 * active, or its handler is in flight. In the latter case
		 * z_try_abort_timeout() has flagged it superseded so the
		 * handler bails; we do not wait for it. Nothing to stop here.
		 */
		k_spin_unlock(&timer_lock, key);
		return;
	}

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
	uint32_t result = timer->status;

	timer->status = 0U;
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

			if (!z_is_inactive_timeout(&timer->timeout)) {
				result = *(volatile uint32_t *)&timer->status;
				timer->status = 0U;
				if (result > 0) {
					irq_unlock(key);
					break;
				} else {
					k_cpu_atomic_idle(key);
				}
			} else {
				result = timer->status;
				irq_unlock(key);
				break;
			}
		} while (true);

		return result;
	}

	k_spinlock_key_t key = k_spin_lock(&timer_lock);
	uint32_t result = timer->status;

	if (result == 0U) {
		if (!z_is_inactive_timeout(&timer->timeout)) {
			SYS_PORT_TRACING_OBJ_FUNC_BLOCKING(k_timer, status_sync, timer, K_FOREVER);

			/* wait for timer to expire or stop */
			(void)z_pend_curr(&timer_lock, key, &timer->wait_q, K_FOREVER);

			/* get updated timer status */
			key = k_spin_lock(&timer_lock);
			result = timer->status;
		} else {
			/* timer is already stopped */
		}
	} else {
		/* timer has already expired at least once */
	}

	timer->status = 0U;
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

#ifdef CONFIG_TIMER_CLOCK

int z_impl_k_timer_settime(struct k_timer *timer, uint32_t flags, const struct timespec *value,
			   const struct timespec *interval, struct timespec *ovalue,
			   struct timespec *ointerval)
{
	uint64_t deadline = 0;
	k_ticks_t per = 0;
	k_ticks_t orem = 0;
	k_ticks_t oper = 0;
	bool disarm;

	if ((value == NULL) || !timespec_is_valid(value) ||
	    ((interval != NULL) && (!timespec_is_valid(interval) || (interval->tv_sec < 0))) ||
	    ((flags & ~(uint32_t)SYS_TIMER_ABSTIME) != 0U)) {
		return -EINVAL;
	}

	if (((flags & SYS_TIMER_ABSTIME) == 0U) && (value->tv_sec < 0)) {
		return -EINVAL;
	}

	if (!IS_ENABLED(CONFIG_TIMER_REALTIME) && ((flags & SYS_TIMER_ABSTIME) != 0U) &&
	    (timer->clock_id == SYS_CLOCK_REALTIME)) {
		return -ENOTSUP;
	}

	if (interval != NULL) {
		per = (k_ticks_t)z_timer_ts_to_ticks(interval);
	}

	disarm = z_timer_ts_is_zero(value);

	k_spinlock_key_t key = k_spin_lock(&timer_lock);

	/* previous state, reported as k_timer_gettime() would have */
	if (!z_is_inactive_timeout(&timer->timeout)) {
		orem = z_timeout_remaining(&timer->timeout);
	}
	if ((timer->flags & Z_TIMER_FLAG_EXACT) != 0U) {
		oper = timer->period_ticks;
	} else if (!K_TIMEOUT_EQ(timer->period, K_NO_WAIT) &&
		   !K_TIMEOUT_EQ(timer->period, K_FOREVER)) {
		oper = timer->period.ticks;
	}

	(void)z_try_abort_timeout(&timer->timeout);
	z_timer_rt_deregister(timer);
	if (disarm) {
		z_timer_sig_freeze(timer);
	}
	timer->status = 0U;
	/* the legacy re-arm path stays disabled while in exact mode */
	timer->period = K_NO_WAIT;
	timer->flags |= Z_TIMER_FLAG_EXACT;
	timer->period_ticks = per;

	if (!disarm) {
		if ((flags & SYS_TIMER_ABSTIME) != 0U) {
#ifdef CONFIG_TIMER_REALTIME
			if (timer->clock_id == SYS_CLOCK_REALTIME) {
				struct timespec mono = *value;
				struct timespec off;

				/* lock order: timer lock, then rt offset lock - never reversed */
				sys_clock_getrtoffset(&off);
				(void)timespec_sub(&mono, &off);
				deadline = z_timer_ts_to_ticks(&mono);
				timer->rt_sec = (int64_t)value->tv_sec;
				timer->rt_nsec = value->tv_nsec;
				z_timer_rt_register(timer);
			} else
#endif /* CONFIG_TIMER_REALTIME */
			{
				deadline = z_timer_ts_to_ticks(value);
			}
		} else {
			/*
			 * "at least" semantics: arming happens mid-tick, so the +1
			 * defeats the round-down to the current tick boundary.
			 */
			deadline = (uint64_t)k_uptime_ticks() + z_timer_ts_to_ticks(value) + 1U;
			deadline = MIN(deadline, (uint64_t)K_TICK_MAX);
		}

		timer->expiry_ticks = deadline;
		z_add_timeout(&timer->timeout, z_timer_expiration_handler,
			      K_TIMEOUT_ABS_TICKS(deadline));
	}

	k_spin_unlock(&timer_lock, key);

	if (ovalue != NULL) {
		timespec_from_timeout(K_TICKS(orem), ovalue);
	}
	if (ointerval != NULL) {
		timespec_from_timeout(K_TICKS(oper), ointerval);
	}

	return 0;
}

#ifdef CONFIG_USERSPACE
static inline int z_vrfy_k_timer_settime(struct k_timer *timer, uint32_t flags,
					 const struct timespec *value,
					 const struct timespec *interval, struct timespec *ovalue,
					 struct timespec *ointerval)
{
	/* a handle that is not a live timer of the caller's is an invalid id, not a fault */
	if (k_object_validate(k_object_find(timer), K_OBJ_TIMER, _OBJ_INIT_TRUE) != 0) {
		return -EINVAL;
	}
	if (value != NULL) {
		K_OOPS(K_SYSCALL_MEMORY_READ(value, sizeof(*value)));
	}
	if (interval != NULL) {
		K_OOPS(K_SYSCALL_MEMORY_READ(interval, sizeof(*interval)));
	}
	if (ovalue != NULL) {
		K_OOPS(K_SYSCALL_MEMORY_WRITE(ovalue, sizeof(*ovalue)));
	}
	if (ointerval != NULL) {
		K_OOPS(K_SYSCALL_MEMORY_WRITE(ointerval, sizeof(*ointerval)));
	}
	return z_impl_k_timer_settime(timer, flags, value, interval, ovalue, ointerval);
}
#include <zephyr/syscalls/k_timer_settime_mrsh.c>
#endif /* CONFIG_USERSPACE */

int z_impl_k_timer_gettime(const struct k_timer *timer, struct timespec *value,
			   struct timespec *interval)
{
	int armed;
	k_ticks_t rem = 0;
	k_ticks_t per = 0;

	k_spinlock_key_t key = k_spin_lock(&timer_lock);

	armed = z_is_inactive_timeout(&timer->timeout) ? 0 : 1;
	if (armed != 0) {
		rem = z_timeout_remaining(&timer->timeout);
	}
	if ((timer->flags & Z_TIMER_FLAG_EXACT) != 0U) {
		per = timer->period_ticks;
	} else if (!K_TIMEOUT_EQ(timer->period, K_NO_WAIT) &&
		   !K_TIMEOUT_EQ(timer->period, K_FOREVER)) {
		per = timer->period.ticks;
	}

	k_spin_unlock(&timer_lock, key);

	if (value != NULL) {
		timespec_from_timeout(K_TICKS(rem), value);
	}
	if (interval != NULL) {
		timespec_from_timeout(K_TICKS(per), interval);
	}

	return armed;
}

#ifdef CONFIG_USERSPACE
static inline int z_vrfy_k_timer_gettime(const struct k_timer *timer, struct timespec *value,
					 struct timespec *interval)
{
	/* a handle that is not a live timer of the caller's is an invalid id, not a fault */
	if (k_object_validate(k_object_find(timer), K_OBJ_TIMER, _OBJ_INIT_TRUE) != 0) {
		return -EINVAL;
	}
	if (value != NULL) {
		K_OOPS(K_SYSCALL_MEMORY_WRITE(value, sizeof(*value)));
	}
	if (interval != NULL) {
		K_OOPS(K_SYSCALL_MEMORY_WRITE(interval, sizeof(*interval)));
	}
	return z_impl_k_timer_gettime(timer, value, interval);
}
#include <zephyr/syscalls/k_timer_gettime_mrsh.c>
#endif /* CONFIG_USERSPACE */

#endif /* CONFIG_TIMER_CLOCK */

#ifdef CONFIG_TIMER_SIGNAL

void z_timer_sig_attach(struct k_timer *timer, k_pid_t target, int signo, union k_sig_val value)
{
	k_spinlock_key_t key = k_spin_lock(&timer_lock);

	timer->sig_target = target;
	timer->sig_signo = signo;
	timer->sig_value = value;
	atomic_clear(&timer->sig_pending);
	atomic_clear(&timer->sig_overrun);
	timer->sig_gen_ticks = 0U;
	timer->sig_gen_period = 0;
	timer->sig_freeze_ticks = 0U;
	timer->sig_missed = 0U;

	k_spin_unlock(&timer_lock, key);
}

void z_timer_sig_detach(struct k_timer *timer)
{
	k_spinlock_key_t key = k_spin_lock(&timer_lock);

	/* no further generation... */
	timer->sig_target = NULL;

	k_spin_unlock(&timer_lock, key);

	/*
	 * ...and no dangling references: purge queued-but-undelivered instances.
	 * Called after releasing the timer lock (sanctioned order is timer lock ->
	 * k_sig_lock only on the expiry path).
	 */
	z_sig_purge_timer(timer);
}

/*
 * Delivery-side hooks, called from kernel/signal.c while it may hold k_sig_lock.
 * These are atomics-only by design and MUST NOT take the timer lock: the expiry
 * path holds timer lock -> k_sig_lock, and acquiring them in the reverse order
 * here would deadlock under SMP.
 */
void z_timer_sig_delivered(struct k_timer *timer)
{
	uint64_t gen = timer->sig_gen_ticks;
	uint64_t frozen = timer->sig_freeze_ticks;
	k_ticks_t period = timer->sig_gen_period;
	uint64_t now = (frozen != 0U) ? frozen : (uint64_t)sys_clock_tick_get();
	uint32_t overrun = 0U;

	if ((period > 0) && (now > gen)) {
		overrun = (uint32_t)MIN((now - gen) / (uint64_t)period, (uint64_t)INT_MAX);
	}

	atomic_set(&timer->sig_overrun, (atomic_val_t)overrun);
	atomic_clear(&timer->sig_pending);
}

void z_timer_sig_discarded(struct k_timer *timer)
{
	atomic_clear(&timer->sig_pending);
}

int z_impl_k_timer_overrun(const struct k_timer *timer)
{
	return (int)atomic_get(&timer->sig_overrun);
}

#ifdef CONFIG_USERSPACE
static inline int z_vrfy_k_timer_overrun(const struct k_timer *timer)
{
	/* a handle that is not a live timer of the caller's is an invalid id, not a fault */
	if (k_object_validate(k_object_find(timer), K_OBJ_TIMER, _OBJ_INIT_TRUE) != 0) {
		return -EINVAL;
	}
	return z_impl_k_timer_overrun(timer);
}
#include <zephyr/syscalls/k_timer_overrun_mrsh.c>
#endif /* CONFIG_USERSPACE */

#endif /* CONFIG_TIMER_SIGNAL */

#ifdef CONFIG_TIMER_REALTIME
void z_timer_realtime_settime(void)
{
	sys_snode_t *node;
	struct timespec off;

	__ASSERT(!arch_is_in_isr(), "realtime timers cannot be re-targeted from ISR context");

	/* snapshot the new offset outside the timer lock: no nested locking here */
	sys_clock_getrtoffset(&off);

	k_spinlock_key_t key = k_spin_lock(&timer_lock);

	SYS_SLIST_FOR_EACH_NODE(&rt_timer_list, node) {
		struct k_timer *timer = CONTAINER_OF(node, struct k_timer, rt_node);
		struct timespec mono = {
			.tv_sec = (time_t)timer->rt_sec,
			.tv_nsec = timer->rt_nsec,
		};
		uint64_t deadline;

		(void)timespec_sub(&mono, &off);
		deadline = z_timer_ts_to_ticks(&mono);

		/* past deadlines (clock moved forward over them) expire immediately */
		(void)z_try_abort_timeout(&timer->timeout);
		timer->expiry_ticks = deadline;
		z_add_timeout(&timer->timeout, z_timer_expiration_handler,
			      K_TIMEOUT_ABS_TICKS(deadline));
	}

	k_spin_unlock(&timer_lock, key);
}
#endif /* CONFIG_TIMER_REALTIME */

#ifdef CONFIG_OBJ_CORE_TIMER
static int init_timer_obj_core_list(void)
{
	/* Initialize timer object type */

	z_obj_type_init(&obj_type_timer, K_OBJ_TYPE_TIMER_ID,
			offsetof(struct k_timer, obj_core));

	/* Initialize and link statically defined timers */

	STRUCT_SECTION_FOREACH(k_timer, timer) {
		k_obj_core_init_and_link(K_OBJ_CORE(timer), &obj_type_timer);
	}

	return 0;
}
SYS_INIT(init_timer_obj_core_list, PRE_KERNEL_1,
	 CONFIG_KERNEL_INIT_PRIORITY_OBJECTS);
#endif /* CONFIG_OBJ_CORE_TIMER */
