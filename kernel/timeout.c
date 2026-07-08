/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/minmax.h>
#include <zephyr/kernel.h>
#include <zephyr/spinlock.h>
#include <ksched.h>
#include <timeout_q.h>
#include <zephyr/internal/syscall_handler.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/sys_clock.h>
#include <zephyr/llext/symbol.h>

#include <timeslicing.h>

static uint64_t curr_tick;

static sys_dlist_t timeout_list = SYS_DLIST_STATIC_INIT(&timeout_list);

/*
 * The timeout code shall take no locks other than its own (timeout_lock), nor
 * shall it call any other subsystem while holding this lock.
 */
static struct k_spinlock timeout_lock;

/* Ticks left to process in the currently-executing sys_clock_announce() */
static uint32_t announce_remaining;

/* CPU id currently inside sys_clock_announce_locked()'s firing loop, or -1
 * when no CPU is. The SMP early-return below ensures at most one CPU is in
 * the loop at a time, so a single int suffices. Used by code that needs to
 * know whether *this* CPU is at a tick edge (announcing) versus somewhere
 * within a tick (any other context, including running on a CPU while a
 * different CPU is the announcer).
 */
static int announcing_cpu = -1;

static inline bool this_cpu_announcing(void)
{
	return announcing_cpu == CPU_ID;
}

static inline bool any_cpu_announcing(void)
{
	return announcing_cpu != -1;
}

/* Timeout whose handler is currently being dispatched, or NULL when no
 * handler is in flight. The announcing CPU sets the pointer before
 * calling the handler and clears it afterwards; any aborter may set the
 * low "superseded" bit (struct _timeout is pointer-aligned so bit 0 is
 * free). Accessors below mask the bit.
 */
static struct _timeout *inflight_timeout;

#define INFLIGHT_SUPERSEDED_BIT 1UL

static inline struct _timeout *inflight_ptr(void)
{
	return (struct _timeout *)((uintptr_t)inflight_timeout & ~INFLIGHT_SUPERSEDED_BIT);
}

static inline void inflight_mark_superseded(void)
{
	inflight_timeout = (struct _timeout *)((uintptr_t)inflight_timeout |
					       INFLIGHT_SUPERSEDED_BIT);
}

static struct _timeout *first(void)
{
	sys_dnode_t *t = sys_dlist_peek_head(&timeout_list);

	return (t == NULL) ? NULL : CONTAINER_OF(t, struct _timeout, node);
}

static struct _timeout *next(struct _timeout *t)
{
	sys_dnode_t *n = sys_dlist_peek_next(&timeout_list, &t->node);

	return (n == NULL) ? NULL : CONTAINER_OF(n, struct _timeout, node);
}

static void remove_timeout(struct _timeout *t)
{
	if (next(t) != NULL) {
		next(t)->dticks += t->dticks;
	}

	sys_dlist_remove(&t->node);
}

static uint32_t elapsed(void)
{
	/*
	 * While *this* CPU is executing sys_clock_announce_locked()'s firing
	 * loop, new relative timeouts scheduled from a callback (or from a
	 * higher-priority ISR that preempted one) are anchored to the currently
	 * firing tick (curr_tick), so we report 0.
	 *
	 * On any other CPU the picture is different: we are not at a tick edge,
	 * and curr_tick is partway through being advanced by the announcing
	 * CPU's loop. The invariant we want to preserve is
	 *
	 *   T_real - curr_tick = announce_remaining + sys_clock_elapsed()
	 *
	 * because the driver bumped its internal announced-cycle baseline to
	 * (curr_tick_initial + N) * CYC_PER_TICK at ISR entry, while the kernel
	 * has only advanced curr_tick by the K ticks processed so far -- so
	 * announce_remaining (= N - K) is the residual that must be added to
	 * sys_clock_elapsed() to get the real-time delta from curr_tick. This
	 * keeps sys_clock_tick_get() monotonic across the announce window.
	 */
	if (this_cpu_announcing()) {
		return 0U;
	}
	return sys_clock_elapsed() +
	       (IS_ENABLED(CONFIG_SMP) ? announce_remaining : 0);
}

static uint32_t next_timeout(uint32_t ticks_elapsed)
{
	struct _timeout *to = first();
	uint32_t dticks;

	/*
	 * sys_clock_announce() reports the ticks elapsed since the previous
	 * announce, so the gap between two announces must not exceed
	 * SYS_CLOCK_MAX_WAIT for the announced count to fit. The budget left
	 * from now on is SYS_CLOCK_MAX_WAIT - ticks_elapsed; if it is already
	 * spent ask for an announce right away.
	 */
	if (ticks_elapsed >= SYS_CLOCK_MAX_WAIT) {
		return 0;
	}

	/*
	 * With nothing pending under sloppy idle, report SYS_CLOCK_MAX_WAIT
	 * verbatim (not the budget): it is the "no deadline" value a driver
	 * looks for to stop the clock entirely, and a skewed uptime is
	 * acceptable. Without sloppy idle the uptime must stay correct, so
	 * respect the budget and keep waking up.
	 */
	if (to == NULL) {
		if (IS_ENABLED(CONFIG_SYSTEM_CLOCK_SLOPPY_IDLE)) {
			return SYS_CLOCK_MAX_WAIT;
		}
		return SYS_CLOCK_MAX_WAIT - ticks_elapsed;
	}

	/*
	 * A timeout further out than can be scheduled in one step: wait nearly
	 * the whole budget, but stop one short of SYS_CLOCK_MAX_WAIT so it is
	 * never mistaken for the "no deadline" value above (which would drop
	 * the timeout under sloppy idle); an intermediate announce re-evaluates.
	 * Testing to->dticks (which may be 64-bit) against the cap also keeps
	 * the remaining arithmetic in 32 bits.
	 */
	if (to->dticks >= SYS_CLOCK_MAX_WAIT) {
		return SYS_CLOCK_MAX_WAIT - 1 - ticks_elapsed;
	}

	/* Otherwise wait until the timeout, relative to now (0 if due). */
	dticks = (uint32_t)to->dticks;

	return (dticks > ticks_elapsed) ? (dticks - ticks_elapsed) : 0;
}

k_ticks_t z_add_timeout(struct _timeout *to, _timeout_func_t fn, k_timeout_t timeout)
{
	k_ticks_t ticks = 0;

	if (K_TIMEOUT_EQ(timeout, K_FOREVER)) {
		return 0;
	}

#ifdef CONFIG_KERNEL_COHERENCE
	__ASSERT_NO_MSG(sys_cache_is_mem_coherent(to));
#endif /* CONFIG_KERNEL_COHERENCE */

	__ASSERT(!sys_dnode_is_linked(&to->node), "");
	to->fn = fn;

	K_SPINLOCK(&timeout_lock) {
		struct _timeout *t;
		uint32_t ticks_elapsed = 0U;
		bool has_elapsed = false;

		if (Z_IS_TIMEOUT_RELATIVE(timeout)) {
			ticks_elapsed = elapsed();
			has_elapsed = true;
			/*
			 * In the general case, "now" may be anywhere within
			 * the current tick. Rounding up by one tick guarantees
			 * "at least N ticks" semantics -- otherwise a request
			 * made partway through a tick would fire on the next
			 * tick edge, yielding less than N full ticks.
			 *
			 * The one moment we know *this* CPU is at a tick edge
			 * is while it is processing timeouts inside its own
			 * sys_clock_announce_locked() loop. Periodic timers
			 * rely on this when rescheduling themselves from the
			 * timer ISR: the round-up would otherwise accumulate
			 * and make every period one tick late. The check is
			 * per-CPU: a thread on another CPU running while we
			 * announce is *not* at a tick edge and still needs
			 * the round-up.
			 */
			to->dticks = timeout.ticks + ticks_elapsed +
				     (this_cpu_announcing() ? 0 : 1);
			ticks = curr_tick + to->dticks;
		} else {
			k_ticks_t dticks = Z_TICK_ABS(timeout.ticks) - curr_tick;

			to->dticks = max(1, dticks);
			ticks = timeout.ticks;
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

		if (to == first() && !any_cpu_announcing()) {
			if (!has_elapsed) {
				/* In case of absolute timeout that is first to expire
				 * elapsed need to be read from the system clock.
				 */
				ticks_elapsed = elapsed();
			}
			sys_clock_set_timeout(next_timeout(ticks_elapsed), false);
		}
	}

	return ticks;
}

int z_try_abort_timeout(struct _timeout *to)
{
	int ret = -EINVAL;

	K_SPINLOCK(&timeout_lock) {
		if (sys_dnode_is_linked(&to->node)) {
			bool is_first = (to == first());

			remove_timeout(to);
			ret = 0;
			if (is_first) {
				sys_clock_set_timeout(next_timeout(elapsed()), false);
			}
		} else if (IS_ENABLED(CONFIG_SMP) && inflight_ptr() == to &&
			   !this_cpu_announcing()) {
			/* Handler in flight on another CPU. Free-safety
			 * callers retry on -EAGAIN to wait it out; others
			 * rely on the superseded mark below and don't.
			 */
			ret = -EAGAIN;
		}

		/* Record that the in-flight timeout has been aborted, so a
		 * handler that checks (z_timeout_inflight_superseded) can
		 * tell its dispatch was overtaken.
		 */
		if (inflight_ptr() == to) {
			inflight_mark_superseded();
		}
	}

	if (IS_ENABLED(CONFIG_SMP) && ret == -EAGAIN) {
		arch_spin_relax();
	}

	return ret;
}

bool z_timeout_inflight_superseded(const struct _timeout *to)
{
	bool superseded = false;

	K_SPINLOCK(&timeout_lock) {
		superseded = inflight_timeout ==
			     (struct _timeout *)((uintptr_t)to | INFLIGHT_SUPERSEDED_BIT);
	}

	return superseded;
}

/* must be locked */
static k_ticks_t timeout_rem(const struct _timeout *timeout)
{
	k_ticks_t ticks = 0;

	for (struct _timeout *t = first(); t != NULL; t = next(t)) {
		ticks += t->dticks;
		if (timeout == t) {
			break;
		}
	}

	return ticks;
}

k_ticks_t z_timeout_remaining(const struct _timeout *timeout)
{
	k_ticks_t ticks = 0;

	K_SPINLOCK(&timeout_lock) {
		if (!z_is_inactive_timeout(timeout)) {
			ticks = timeout_rem(timeout) - elapsed();
		}
	}

	return ticks;
}
EXPORT_SYMBOL(z_timeout_remaining);

k_ticks_t z_timeout_expires(const struct _timeout *timeout)
{
	k_ticks_t ticks = 0;

	K_SPINLOCK(&timeout_lock) {
		ticks = curr_tick;
		if (!z_is_inactive_timeout(timeout)) {
			ticks += timeout_rem(timeout);
		}
	}

	return ticks;
}
EXPORT_SYMBOL(z_timeout_expires);

int32_t z_get_next_timeout_expiry(void)
{
	int32_t ret = (int32_t) K_TICKS_FOREVER;

	K_SPINLOCK(&timeout_lock) {
		uint32_t t = next_timeout(elapsed());

		/*
		 * next_timeout() is unsigned; only fold it into the signed
		 * result when it fits, otherwise leave the K_TICKS_FOREVER
		 * default.
		 */
		if (t <= INT32_MAX) {
			ret = (int32_t)t;
		}
	}
	return ret;
}

void sys_clock_announce_locked(uint32_t ticks, k_spinlock_key_t key)
{
	/* We release the lock around the callbacks below, so on SMP
	 * systems someone might be already running the loop.  Don't
	 * race (which will cause parallel execution of "sequential"
	 * timeouts and confuse apps), just increment the tick count
	 * and return.
	 */
	if (IS_ENABLED(CONFIG_SMP) && any_cpu_announcing()) {
		announce_remaining += ticks;
		k_spin_unlock(&timeout_lock, key);
		return;
	}

	announce_remaining = ticks;
	announcing_cpu = CPU_ID;

	struct _timeout *t;

	for (t = first();
	     (t != NULL) && (t->dticks <= announce_remaining);
	     t = first()) {
		_timeout_func_t handler = t->fn;

		/* Advance curr_tick and decrement announce_remaining
		 * together under the lock so non-announcing CPUs observe
		 * a consistent (curr_tick + announce_remaining +
		 * sys_clock_elapsed()) == T_real even while we drop the
		 * lock around the handler. The "we are announcing" state
		 * is carried by announcing_cpu, so announce_remaining
		 * reaching 0 mid-loop is harmless: same-tick handlers'
		 * z_add_timeout() still anchors via this_cpu_announcing(),
		 * and another CPU's announce still folds into ours via
		 * any_cpu_announcing() in the SMP early-return.
		 */
		curr_tick += t->dticks;
		announce_remaining -= t->dticks;

		sys_dlist_remove(&t->node);
		inflight_timeout = t;

		k_spin_unlock(&timeout_lock, key);
		handler(t);
		key = k_spin_lock(&timeout_lock);
		inflight_timeout = NULL;
	}

	if (t != NULL) {
		t->dticks -= announce_remaining;
	}

	curr_tick += announce_remaining;
	announce_remaining = 0;
	announcing_cpu = -1;

	sys_clock_set_timeout(next_timeout(0), false);

	k_spin_unlock(&timeout_lock, key);

#ifdef CONFIG_TIMESLICING
	z_time_slice();
#endif /* CONFIG_TIMESLICING */
}

#if defined(CONFIG_SMP) || defined(CONFIG_SPIN_VALIDATE)
k_spinlock_key_t sys_clock_lock(void)
{
	return k_spin_lock(&timeout_lock);
}

void sys_clock_unlock(k_spinlock_key_t key)
{
	k_spin_unlock(&timeout_lock, key);
}
#endif

#if defined(CONFIG_TEST) || defined(CONFIG_ASSERT)
bool sys_clock_is_locked(void)
{
	return z_spin_is_locked(&timeout_lock);
}
#endif

int64_t sys_clock_tick_get(void)
{
	uint64_t t = 0U;

	K_SPINLOCK(&timeout_lock) {
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
#endif /* CONFIG_TICKLESS_KERNEL */
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
#include <zephyr/syscalls/k_uptime_ticks_mrsh.c>
#endif /* CONFIG_USERSPACE */

k_timepoint_t sys_timepoint_calc(k_timeout_t timeout)
{
	k_timepoint_t timepoint;

	if (K_TIMEOUT_EQ(timeout, K_FOREVER)) {
		timepoint.tick = UINT64_MAX;
	} else if (K_TIMEOUT_EQ(timeout, K_NO_WAIT)) {
		timepoint.tick = 0;
	} else {
		k_ticks_t dt = timeout.ticks;

		if (Z_IS_TIMEOUT_RELATIVE(timeout)) {
			timepoint.tick = sys_clock_tick_get() + max(1, dt);
		} else {
			timepoint.tick = Z_TICK_ABS(dt);
		}
	}

	return timepoint;
}

k_timeout_t sys_timepoint_timeout(k_timepoint_t timepoint)
{
	uint64_t now, remaining;

	if (timepoint.tick == UINT64_MAX) {
		return K_FOREVER;
	}
	if (timepoint.tick == 0) {
		return K_NO_WAIT;
	}

	now = sys_clock_tick_get();
	remaining = (timepoint.tick > now) ? (timepoint.tick - now) : 0;
	return K_TICKS(remaining);
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
#endif /* CONFIG_ZTEST */
