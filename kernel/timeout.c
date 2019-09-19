/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <timeout_q.h>
#include <drivers/timer/system_timer.h>
#include <sys_clock.h>
#include <spinlock.h>
#include <ksched.h>
#include <syscall_handler.h>

#define LOCKED(lck) for (k_spinlock_key_t __i = {},			\
					  __key = k_spin_lock(lck);	\
			__i.key == 0;					\
			k_spin_unlock(lck, __key), __i.key = 1)

#ifdef CONFIG_SYS_TIMEOUT_LEGACY_API
#define FOREVER_TICKS K_FOREVER
#else
#define FOREVER_TICKS (K_FOREVER.ticks)
#endif

static u64_t curr_tick;

static sys_dlist_t timeout_list = SYS_DLIST_STATIC_INIT(&timeout_list);

static struct k_spinlock timeout_lock;

#define MAX_WAIT (IS_ENABLED(CONFIG_SYSTEM_CLOCK_SLOPPY_IDLE) \
		  ? K_FOREVER_TICKS : INT_MAX)

/* Cycles left to process in the currently-executing z_clock_announce() */
static int announce_remaining;

static bool announcing;

#if defined(CONFIG_TIMER_READS_ITS_FREQUENCY_AT_RUNTIME)
int z_clock_hw_cycles_per_sec = CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC;

#ifdef CONFIG_USERSPACE
static inline int z_vrfy_z_clock_hw_cycles_per_sec_runtime_get(void)
{
	return z_impl_z_clock_hw_cycles_per_sec_runtime_get();
}
#include <syscalls/z_clock_hw_cycles_per_sec_runtime_get_mrsh.c>
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

static s32_t elapsed(void)
{
	return announce_remaining == 0 ? z_clock_elapsed() : 0;
}

static s32_t next_timeout(void)
{
	struct _timeout *to = first();
	s32_t ticks_elapsed = elapsed();
	s32_t ret = to == NULL ? MAX_WAIT : MAX(0, to->dticks - ticks_elapsed);

#ifdef CONFIG_TIMESLICING
	if (_current_cpu->slice_ticks && _current_cpu->slice_ticks < ret) {
		ret = _current_cpu->slice_ticks;
	}
#endif
	return ret;
}

static bool is_absolute(k_ticks_t t)
{
#ifdef Z_TIMEOUT_ABSOLUTE_TICKS
	return ((s64_t)t) < ((s64_t)K_FOREVER_TICKS);
#else
	return false;
#endif
}

void z_add_timeout(struct _timeout *to, _timeout_func_t fn,
		   k_timeout_t expires)
{
#ifdef CONFIG_SYS_TIMEOUT_LEGACY_API
	k_ticks_t ticks = k_ms_to_ticks_ceil32(expires);
#else
	k_ticks_t ticks = expires.ticks;
#endif

	__ASSERT(!sys_dnode_is_linked(&to->node), "");

	if (!is_absolute(ticks)) {
		/* In the ISR, we can assume we're perfectly aligned
		 * to the start of the current tick, so new timeouts
		 * can use exact tick counts.  At other, arbitrary
		 * times, we need to wait for the next tick to start
		 * counting so we don't expire too soon.
		 */
		if (!announcing) {
			ticks += 1;
		}

		ticks = MAX(1, ticks);
	}

	to->fn = fn;

	LOCKED(&timeout_lock) {
		struct _timeout *t;

#ifdef Z_TIMEOUT_ABSOLUTE_TICKS
		/* Handle absolute expirations */
		if (is_absolute(ticks)) {
			s64_t abs = K_FOREVER_TICKS - 1 - ticks;

			ticks = MAX(0, abs - curr_tick - z_clock_elapsed());
		}
#endif

		to->dticks = ticks + elapsed();
		for (t = first(); t != NULL; t = next(t)) {
			__ASSERT(t->dticks >= 0, "");

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
			z_clock_set_timeout(next_timeout(), false);
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

k_ticks_t z_timeout_end(struct _timeout *timeout)
{
	k_ticks_t ticks = 0;

	if (z_is_inactive_timeout(timeout)) {
		return K_FOREVER_TICKS;
	}

	LOCKED(&timeout_lock) {
		ticks = (k_ticks_t) curr_tick;

		for (struct _timeout *t = first(); t != NULL; t = next(t)) {
			ticks += t->dticks;
			if (timeout == t) {
				break;
			}
		}
	}

	return ticks;
}

k_ticks_t z_timeout_remaining(struct _timeout *timeout)
{
	if (z_is_inactive_timeout(timeout)) {
		return K_FOREVER_TICKS;
	}

	k_ticks_t rem = z_timeout_end(timeout) - ((k_ticks_t)curr_tick + elapsed());

	/* Note: also handles the case where z_timeout_end() returns FOREVER */
	if (rem < 0) {
		return K_FOREVER_TICKS;
	}

	return rem;
}

k_ticks_t z_get_next_timeout_expiry(void)
{
	k_ticks_t ret = FOREVER_TICKS;

	LOCKED(&timeout_lock) {
		ret = next_timeout();
	}
	return ret;
}

void z_set_timeout_expiry(k_ticks_t ticks, bool idle)
{
	LOCKED(&timeout_lock) {
		k_ticks_t next = next_timeout();
		bool sooner = (next == FOREVER_TICKS) || (ticks < next);
		bool imminent = next <= 1;

		/* Only set new timeouts when they are sooner than
		 * what we have.  Also don't try to set a timeout when
		 * one is about to expire: drivers have internal logic
		 * that will bump the timeout to the "next" tick if
		 * it's not considered to be settable as directed.
		 * SMP can't use this optimization though: we don't
		 * know when context switches happen until interrupt
		 * exit and so can't get the timeslicing clamp folded
		 * in.
		 */
		if (!imminent && (sooner || IS_ENABLED(CONFIG_SMP))) {
			z_clock_set_timeout(ticks, idle);
		}
	}
}

void z_clock_announce(s32_t ticks)
{
#ifdef CONFIG_TIMESLICING
	z_time_slice(ticks);
#endif

	k_spinlock_key_t key = k_spin_lock(&timeout_lock);

	announce_remaining = ticks;
	announcing = true;

	while (first() != NULL && first()->dticks <= announce_remaining) {
		struct _timeout *t = first();
		int dt = t->dticks;

		curr_tick += dt;
		announce_remaining -= dt;
		t->dticks = 0;
		remove_timeout(t);

		k_spin_unlock(&timeout_lock, key);
		t->fn(t);
		key = k_spin_lock(&timeout_lock);
	}

	if (first() != NULL) {
		first()->dticks -= announce_remaining;
	}

	curr_tick += announce_remaining;
	announce_remaining = 0;
	announcing = false;

	z_clock_set_timeout(next_timeout(), false);

	k_spin_unlock(&timeout_lock, key);
}

s64_t z_tick_get(void)
{
	u64_t t = 0U;

	LOCKED(&timeout_lock) {
		t = curr_tick + z_clock_elapsed();
	}
	return t;
}

u32_t z_tick_get_32(void)
{
#ifdef CONFIG_TICKLESS_KERNEL
	return (u32_t)z_tick_get();
#else
	return (u32_t)curr_tick;
#endif
}

u64_t z_impl_k_uptime_ticks(void)
{
	return z_tick_get();
}

#ifdef CONFIG_USERSPACE
static inline u64_t z_vrfy_k_uptime_ticks(void)
{
	return z_impl_k_uptime_ticks();
}
#include <syscalls/k_uptime_ticks_mrsh.c>
#endif
