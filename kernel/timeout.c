/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/spinlock.h>
#include <ksched.h>
#include <zephyr/timeout_q.h>
#include <zephyr/syscall_handler.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/sys_clock.h>

static uint64_t curr_tick;

static sys_dlist_t timeout_list = SYS_DLIST_STATIC_INIT(&timeout_list);

static struct k_spinlock timeout_lock;

#define MAX_WAIT (IS_ENABLED(CONFIG_SYSTEM_CLOCK_SLOPPY_IDLE) \
		  ? K_TICKS_FOREVER : INT_MAX)

/* Cycles left to process in the currently-executing sys_clock_announce() */
static int announce_remaining;

#if defined(CONFIG_TIMER_READS_ITS_FREQUENCY_AT_RUNTIME)
int z_clock_hw_cycles_per_sec = CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC;

#ifdef CONFIG_USERSPACE
static inline int z_vrfy_sys_clock_hw_cycles_per_sec_runtime_get(void)
{
	return z_impl_sys_clock_hw_cycles_per_sec_runtime_get();
}
#include <syscalls/sys_clock_hw_cycles_per_sec_runtime_get_mrsh.c>
#endif /* CONFIG_USERSPACE */
#endif /* CONFIG_TIMER_READS_ITS_FREQUENCY_AT_RUNTIME */

static struct _timeout *first(void)
{
	sys_dnode_t *t = sys_dlist_peek_head(&timeout_list);

	return t == NULL ? NULL : CONTAINER_OF(t, struct _timeout, node);
}

static struct _timeout *next(struct _timeout *t)
{
	sys_dnode_t *n = sys_dlist_peek_next(&timeout_list, &t->node);

	return n == NULL ? NULL : CONTAINER_OF(n, struct _timeout, node);
}

static void remove_timeout(struct _timeout *t)
{
	if (next(t) != NULL) {
		next(t)->dticks += t->dticks;
	}

	sys_dlist_remove(&t->node);
}

static int32_t elapsed(void)
{
	return announce_remaining == 0 ? sys_clock_elapsed() : 0U;
}

static int32_t next_timeout(void)
{
	struct _timeout *to = first();
	int32_t ticks_elapsed = elapsed();
	int32_t ret;

	if ((to == NULL) ||
	    ((int64_t)(to->dticks - ticks_elapsed) > (int64_t)INT_MAX)) {
		ret = MAX_WAIT;
	} else {
		ret = MAX(0, to->dticks - ticks_elapsed);
	}

#ifdef CONFIG_TIMESLICING
	if (_current_cpu->slice_deadline != 0) {
		uint32_t local_to = _current_cpu->slice_deadline - curr_tick;

		ret = MIN((uint32_t)ret, local_to);
	}
#endif
	return ret;
}

void z_add_timeout(struct _timeout *to, _timeout_func_t fn,
		   k_timeout_t timeout)
{
	if (K_TIMEOUT_EQ(timeout, K_FOREVER)) {
		return;
	}

#ifdef CONFIG_KERNEL_COHERENCE
	__ASSERT_NO_MSG(arch_mem_coherent(to));
#endif

	__ASSERT(!sys_dnode_is_linked(&to->node), "");
	to->fn = fn;

	LOCKED(&timeout_lock) {
		struct _timeout *t;

		if (IS_ENABLED(CONFIG_TIMEOUT_64BIT) &&
		    Z_TICK_ABS(timeout.ticks) >= 0) {
			k_ticks_t ticks = Z_TICK_ABS(timeout.ticks) - curr_tick;

			to->dticks = MAX(1, ticks);
		} else {
			to->dticks = timeout.ticks + 1 + elapsed();
		}

		for (t = first(); t != NULL; t = next(t)) {
			if (t->dticks > to->dticks) {
				t->dticks -= to->dticks;
				sys_dlist_insert(&t->node, &to->node);
				break;
			}
			to->dticks -= t->dticks;
		}

		if (t == NULL) {
			sys_dlist_append(&timeout_list, &to->node);
		}

		if (to == first()) {
			sys_clock_set_timeout(next_timeout(), false);
		}
	}
}

int z_abort_timeout(struct _timeout *to)
{
	int ret = -EINVAL;

	LOCKED(&timeout_lock) {
		if (sys_dnode_is_linked(&to->node)) {
			remove_timeout(to);
			ret = 0;
		}
	}

	return ret;
}

/* must be locked */
static k_ticks_t timeout_rem(const struct _timeout *timeout)
{
	k_ticks_t ticks = 0;

	if (z_is_inactive_timeout(timeout)) {
		return 0;
	}

	for (struct _timeout *t = first(); t != NULL; t = next(t)) {
		ticks += t->dticks;
		if (timeout == t) {
			break;
		}
	}

	return ticks - elapsed();
}

k_ticks_t z_timeout_remaining(const struct _timeout *timeout)
{
	k_ticks_t ticks = 0;

	LOCKED(&timeout_lock) {
		ticks = timeout_rem(timeout);
	}

	return ticks;
}

k_ticks_t z_timeout_expires(const struct _timeout *timeout)
{
	k_ticks_t ticks = 0;

	LOCKED(&timeout_lock) {
		ticks = curr_tick + timeout_rem(timeout);
	}

	return ticks;
}

int32_t z_get_next_timeout_expiry(void)
{
	int32_t ret = (int32_t) K_TICKS_FOREVER;

	LOCKED(&timeout_lock) {
		ret = next_timeout();
	}
	return ret;
}

void z_set_timeout_expiry(int32_t ticks, bool is_idle)
{
	LOCKED(&timeout_lock) {

#ifdef CONFIG_TIMESLICING
		uint64_t slice_deadline = curr_tick + elapsed() + ticks;

		/* Let next_timeout() consider only the next global timeout */
		_current_cpu->slice_deadline = 0;
#endif

		int next_to = next_timeout();
		bool sooner = (next_to == K_TICKS_FOREVER) || (ticks < next_to);
		bool imminent = (next_to == 0) || (next_to == 1);

		/* Only set new timeouts when they are sooner than
		 * what we have.  Also don't try to set a timeout when
		 * one is about to expire: drivers have internal logic
		 * that will bump the timeout to the "next" tick if
		 * it's not considered to be settable as directed.
		 *
		 * SMP can't use this optimization though: we don't
		 * know if the next global timeout was actually set
		 * to fire on this CPU. We have to set the next timeout
		 * regardless in that case.
		 */
		if (IS_ENABLED(CONFIG_SMP) || (!imminent && sooner)) {
			sys_clock_set_timeout(sooner ? ticks : next_to, is_idle);
		}

#ifdef CONFIG_TIMESLICING
		_current_cpu->slice_deadline = slice_deadline;
#endif
	}
}

/* must be locked */
static inline uint64_t track_actual_time(int32_t ticks)
{
#if defined(CONFIG_SMP) && defined(CONFIG_TIMESLICING)
	static uint64_t curr_time;

	__ASSERT(announce_remaining != 0 || curr_time == curr_tick, "");
	curr_time += ticks;
	return curr_time;
#else
	return curr_tick;
#endif
}

/* must be locked */
static inline bool check_timeslice_expiry(bool do_rearm)
{
#ifdef CONFIG_TIMESLICING
	if (_current_cpu->slice_deadline != 0) {
		uint64_t curr_time = track_actual_time(0);

		if (curr_time >= _current_cpu->slice_deadline) {
			/* this CPU's local timeout has expired */
			_current_cpu->slice_deadline = 0;
			return true;
		} else if (do_rearm) {
			/*
			 * This happens only when this CPU is denied global
			 * timeout expiry processing in sys_clock_announce()
			 * so we have only our local timeout to consider.
			 */
			int32_t ticks =  _current_cpu->slice_deadline - curr_time;

			sys_clock_set_timeout(ticks, false);
		}
	}
#endif
	return false;
}

void sys_clock_announce(int32_t ticks)
{
	k_spinlock_key_t key = k_spin_lock(&timeout_lock);
	bool ts_expired;

	track_actual_time(ticks);

	/* We release the lock around the callbacks below, so on SMP
	 * systems someone might be already running the loop.  Don't
	 * race (which will cause paralllel execution of "sequential"
	 * timeouts and confuse apps), just increment the tick count,
	 * program the next CPU local timeout if any and return.
	 */
	if (IS_ENABLED(CONFIG_SMP) && (announce_remaining != 0)) {
		announce_remaining += ticks;
		ts_expired = check_timeslice_expiry(true);
		k_spin_unlock(&timeout_lock, key);
		if (ts_expired) {
			z_time_slice_expired();
		}
		return;
	}

	announce_remaining = ticks;

	struct _timeout *t = first();

	for (t = first();
	     (t != NULL) && (t->dticks <= announce_remaining);
	     t = first()) {
		int dt = t->dticks;

		curr_tick += dt;
		t->dticks = 0;
		remove_timeout(t);

		k_spin_unlock(&timeout_lock, key);
		t->fn(t);
		key = k_spin_lock(&timeout_lock);
		announce_remaining -= dt;
	}

	if (t != NULL) {
		t->dticks -= announce_remaining;
	}

	curr_tick += announce_remaining;
	announce_remaining = 0;
	ts_expired = check_timeslice_expiry(false);
	sys_clock_set_timeout(next_timeout(), false);

	k_spin_unlock(&timeout_lock, key);

	if (ts_expired) {
		z_time_slice_expired();
	}
}

int64_t sys_clock_tick_get(void)
{
	uint64_t t = 0U;

	LOCKED(&timeout_lock) {
		t = curr_tick + elapsed();
	}
	return t;
}

uint32_t sys_clock_tick_get_32(void)
{
#ifdef CONFIG_TICKLESS_KERNEL
	return (uint32_t)sys_clock_tick_get();
#else
	return (uint32_t)curr_tick;
#endif
}

int64_t z_impl_k_uptime_ticks(void)
{
	return sys_clock_tick_get();
}

#ifdef CONFIG_USERSPACE
static inline int64_t z_vrfy_k_uptime_ticks(void)
{
	return z_impl_k_uptime_ticks();
}
#include <syscalls/k_uptime_ticks_mrsh.c>
#endif

void z_impl_k_busy_wait(uint32_t usec_to_wait)
{
	SYS_PORT_TRACING_FUNC_ENTER(k_thread, busy_wait, usec_to_wait);
	if (usec_to_wait == 0U) {
		SYS_PORT_TRACING_FUNC_EXIT(k_thread, busy_wait, usec_to_wait);
		return;
	}

#if !defined(CONFIG_ARCH_HAS_CUSTOM_BUSY_WAIT)
	uint32_t start_cycles = k_cycle_get_32();

	/* use 64-bit math to prevent overflow when multiplying */
	uint32_t cycles_to_wait = (uint32_t)(
		(uint64_t)usec_to_wait *
		(uint64_t)sys_clock_hw_cycles_per_sec() /
		(uint64_t)USEC_PER_SEC
	);

	for (;;) {
		uint32_t current_cycles = k_cycle_get_32();

		/* this handles the rollover on an unsigned 32-bit value */
		if ((current_cycles - start_cycles) >= cycles_to_wait) {
			break;
		}
	}
#else
	arch_busy_wait(usec_to_wait);
#endif /* CONFIG_ARCH_HAS_CUSTOM_BUSY_WAIT */
	SYS_PORT_TRACING_FUNC_EXIT(k_thread, busy_wait, usec_to_wait);
}

#ifdef CONFIG_USERSPACE
static inline void z_vrfy_k_busy_wait(uint32_t usec_to_wait)
{
	z_impl_k_busy_wait(usec_to_wait);
}
#include <syscalls/k_busy_wait_mrsh.c>
#endif /* CONFIG_USERSPACE */

/* Returns the uptime expiration (relative to an unlocked "now"!) of a
 * timeout object.  When used correctly, this should be called once,
 * synchronously with the user passing a new timeout value.  It should
 * not be used iteratively to adjust a timeout.
 */
uint64_t sys_clock_timeout_end_calc(k_timeout_t timeout)
{
	k_ticks_t dt;

	if (K_TIMEOUT_EQ(timeout, K_FOREVER)) {
		return UINT64_MAX;
	} else if (K_TIMEOUT_EQ(timeout, K_NO_WAIT)) {
		return sys_clock_tick_get();
	} else {

		dt = timeout.ticks;

		if (IS_ENABLED(CONFIG_TIMEOUT_64BIT) && Z_TICK_ABS(dt) >= 0) {
			return Z_TICK_ABS(dt);
		}
		return sys_clock_tick_get() + MAX(1, dt);
	}
}

#ifdef CONFIG_ZTEST
void z_impl_sys_clock_tick_set(uint64_t tick)
{
	curr_tick = tick;
}

void z_vrfy_sys_clock_tick_set(uint64_t tick)
{
	z_impl_sys_clock_tick_set(tick);
}
#endif
