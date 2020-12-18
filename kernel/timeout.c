/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <spinlock.h>
#include <ksched.h>
#include <timeout_q.h>
#include <syscall_handler.h>
#include <drivers/timer/system_timer.h>
#include <sys_clock.h>

#define LOCKED(lck) for (k_spinlock_key_t __i = {},			\
					  __key = k_spin_lock(lck);	\
			__i.key == 0;					\
			k_spin_unlock(lck, __key), __i.key = 1)

static uint64_t curr_tick;

static sys_dlist_t timeout_list = SYS_DLIST_STATIC_INIT(&timeout_list);

static struct k_spinlock timeout_lock;

#define MAX_WAIT (IS_ENABLED(CONFIG_SYSTEM_CLOCK_SLOPPY_IDLE) \
		  ? K_TICKS_FOREVER : INT_MAX)

/* Cycles left to process in the currently-executing z_clock_announce() */
static int announce_remaining;

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

static int32_t elapsed(void)
{
	return announce_remaining == 0 ? z_clock_elapsed() : 0U;
}

static int32_t next_timeout(void)
{
	struct _timeout *to = first();
	int32_t ticks_elapsed = elapsed();
	int32_t ret = to == NULL ? MAX_WAIT
		: CLAMP(to->dticks - ticks_elapsed, 0, MAX_WAIT);

#ifdef CONFIG_TIMESLICING
	if (_current_cpu->slice_ticks && _current_cpu->slice_ticks < ret) {
		ret = _current_cpu->slice_ticks;
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

#ifdef KERNEL_COHERENCE
	__ASSERT_NO_MSG(arch_mem_coherent(to));
#endif

	k_ticks_t ticks = timeout.ticks + 1;

	if (IS_ENABLED(CONFIG_TIMEOUT_64BIT) && Z_TICK_ABS(ticks) >= 0) {
		ticks = Z_TICK_ABS(ticks) - (curr_tick + elapsed());
	}

	__ASSERT(!sys_dnode_is_linked(&to->node), "");
	to->fn = fn;
	ticks = MAX(1, ticks);

	LOCKED(&timeout_lock) {
		struct _timeout *t;

		to->dticks = ticks + elapsed();
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
		int next_to = next_timeout();
		bool sooner = (next_to == K_TICKS_FOREVER)
			      || (ticks < next_to);
		bool imminent = next_to <= 1;

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
			z_clock_set_timeout(ticks, is_idle);
		}
	}
}

void z_clock_announce(int32_t ticks)
{
#ifdef CONFIG_TIMESLICING
	z_time_slice(ticks);
#endif

	k_spinlock_key_t key = k_spin_lock(&timeout_lock);

	announce_remaining = ticks;

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

	z_clock_set_timeout(next_timeout(), false);

	k_spin_unlock(&timeout_lock, key);
}

int64_t z_tick_get(void)
{
	uint64_t t = 0U;

	LOCKED(&timeout_lock) {
		t = curr_tick + z_clock_elapsed();
	}
	return t;
}

uint32_t z_tick_get_32(void)
{
#ifdef CONFIG_TICKLESS_KERNEL
	return (uint32_t)z_tick_get();
#else
	return (uint32_t)curr_tick;
#endif
}

int64_t z_impl_k_uptime_ticks(void)
{
	return z_tick_get();
}

#ifdef CONFIG_USERSPACE
static inline int64_t z_vrfy_k_uptime_ticks(void)
{
	return z_impl_k_uptime_ticks();
}
#include <syscalls/k_uptime_ticks_mrsh.c>
#endif

/* Returns the uptime expiration (relative to an unlocked "now"!) of a
 * timeout object.  When used correctly, this should be called once,
 * synchronously with the user passing a new timeout value.  It should
 * not be used iteratively to adjust a timeout.
 */
uint64_t z_timeout_end_calc(k_timeout_t timeout)
{
	k_ticks_t dt;

	if (K_TIMEOUT_EQ(timeout, K_FOREVER)) {
		return UINT64_MAX;
	} else if (K_TIMEOUT_EQ(timeout, K_NO_WAIT)) {
		return z_tick_get();
	}

	dt = timeout.ticks;

	if (IS_ENABLED(CONFIG_TIMEOUT_64BIT) && Z_TICK_ABS(dt) >= 0) {
		return Z_TICK_ABS(dt);
	}
	return z_tick_get() + MAX(1, dt);
}
