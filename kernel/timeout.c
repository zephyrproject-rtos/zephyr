/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/spinlock.h>
#include <ksched.h>
#include <timeout_q.h>
#include <zephyr/internal/syscall_handler.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/sys_clock.h>
#include <zephyr/llext/symbol.h>
#include <zephyr/sys/min_heap.h>

static uint64_t curr_tick;

/* Forward declaration for timeout_cmp */
static int timeout_cmp(const void *a, const void *b);
static uint32_t timeout_seq;

/**
 * @brief Time comparison helpers that handle wrap-around correctly
 *
 * These functions use signed arithmetic to correctly handle timer wrap-around
 * in both 32-bit and 64-bit modes. The key insight is that for unsigned values
 * a and b, (a - b) interpreted as a signed value tells us their ordering,
 * even if wrap-around has occurred (as long as they're within half the range).
 */

#ifdef CONFIG_TIMEOUT_64BIT
typedef int64_t k_ticks_diff_t;

static inline bool time_after_eq(int64_t a, int64_t b)
{
	return (k_ticks_diff_t)(a - b) >= 0;
}
#else
typedef int32_t k_ticks_diff_t;

static inline bool time_after_eq(int32_t a, int32_t b)
{
	return (k_ticks_diff_t)(a - b) >= 0;
}
#endif /* CONFIG_TIMEOUT_64BIT */

/* Heap element structure containing pointer to timeout */
struct timeout_heap_elem {
	struct _timeout *timeout;
	uint32_t seq;
};

/* Storage for timeout heap - statically initialized */
MIN_HEAP_DEFINE_STATIC(timeout_heap, CONFIG_MAX_TIMEOUTS,
		       sizeof(struct timeout_heap_elem), __alignof__(struct timeout_heap_elem),
		       timeout_cmp);

/* Callback to update timeout heap index
 * Note: Heap internally uses 0-based indexing, but we store 1-based indices
 * in timeout->heap_idx to allow TIMEOUT_NOT_IN_HEAP = 0
 */
static void timeout_update_index(void *element, size_t new_index)
{
	struct timeout_heap_elem *elem = element;

	/* Convert 0-based heap index to 1-based for storage */
	elem->timeout->heap_idx = new_index + 1;
}

/*
 * The timeout code shall take no locks other than its own (timeout_lock), nor
 * shall it call any other subsystem while holding this lock.
 */
static struct k_spinlock timeout_lock;

/* Ticks left to process in the currently-executing sys_clock_announce() */
static int announce_remaining;

#if defined(CONFIG_TIMER_READS_ITS_FREQUENCY_AT_RUNTIME)
unsigned int z_clock_hw_cycles_per_sec = CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC;

#ifdef CONFIG_USERSPACE
static inline unsigned int z_vrfy_sys_clock_hw_cycles_per_sec_runtime_get(void)
{
	return z_impl_sys_clock_hw_cycles_per_sec_runtime_get();
}
#include <zephyr/syscalls/sys_clock_hw_cycles_per_sec_runtime_get_mrsh.c>
#endif /* CONFIG_USERSPACE */
#endif /* CONFIG_TIMER_READS_ITS_FREQUENCY_AT_RUNTIME */

/* Comparator function for min-heap: compares absolute expiry ticks
 * Uses wrap-around safe comparison to handle tick counter overflow
 */
static int timeout_cmp(const void *a, const void *b)
{
	const struct timeout_heap_elem *elem_a = a;
	const struct timeout_heap_elem *elem_b = b;

#ifdef CONFIG_TIMEOUT_64BIT
	int64_t tick_a = elem_a->timeout->dticks;
	int64_t tick_b = elem_b->timeout->dticks;

	/* Use signed difference to handle wrap-around */
	k_ticks_diff_t diff = (k_ticks_diff_t)(tick_a - tick_b);

	if (diff < 0) {
		return -1;  /* a expires before b */
	} else if (diff > 0) {
		return 1;   /* a expires after b */
	}
#else
	int32_t tick_a = elem_a->timeout->dticks;
	int32_t tick_b = elem_b->timeout->dticks;

	/* Use signed difference to handle wrap-around */
	k_ticks_diff_t diff = (k_ticks_diff_t)(tick_a - tick_b);

	if (diff < 0) {
		return -1;  /* a expires before b */
	} else if (diff > 0) {
		return 1;   /* a expires after b */
	}
#endif

	/* Ticks are equal, use sequence number for stable sorting */
	if (elem_a->seq < elem_b->seq) {
		return -1;
	} else if (elem_a->seq > elem_b->seq) {
		return 1;
	}
	return 0;
}

static struct _timeout *first(void)
{
	struct timeout_heap_elem *elem = min_heap_peek(&timeout_heap);

	return (elem == NULL) ? NULL : elem->timeout;
}

static int32_t elapsed(void)
{
	/* While sys_clock_announce() is executing, new relative timeouts will be
	 * scheduled relatively to the currently firing timeout's original tick
	 * value (=curr_tick) rather than relative to the current
	 * sys_clock_elapsed().
	 *
	 * This means that timeouts being scheduled from within timeout callbacks
	 * will be scheduled at well-defined offsets from the currently firing
	 * timeout.
	 *
	 * As a side effect, the same will happen if an ISR with higher priority
	 * preempts a timeout callback and schedules a timeout.
	 *
	 * The distinction is implemented by looking at announce_remaining which
	 * will be non-zero while sys_clock_announce() is executing and zero
	 * otherwise.
	 */
	return announce_remaining == 0 ? sys_clock_elapsed() : 0U;
}

static int32_t next_timeout(int32_t ticks_elapsed)
{
	struct _timeout *to = first();
	int32_t ret;

	if (to == NULL) {
		ret = SYS_CLOCK_MAX_WAIT;
	} else {
		/* Calculate relative time to next timeout using wrap-around safe arithmetic
		 * Note: ticks_elapsed is typically small (< 100), so we can safely subtract it
		 */
#ifdef CONFIG_TIMEOUT_64BIT
		k_ticks_diff_t diff = (k_ticks_diff_t)(to->dticks - curr_tick - ticks_elapsed);
#else
		k_ticks_diff_t diff = (k_ticks_diff_t)(to->dticks - (int32_t)curr_tick - ticks_elapsed);
#endif

		/* If diff > INT_MAX, timeout is too far in future */
		if (diff > (k_ticks_diff_t)INT_MAX) {
			ret = SYS_CLOCK_MAX_WAIT;
		} else if (diff < 0) {
			/* Timeout has already expired or will expire very soon */
			ret = 0;
		} else {
			ret = (int32_t)diff;
		}
	}

	return ret;
}

k_ticks_t z_add_timeout(struct _timeout *to, _timeout_func_t fn, k_timeout_t timeout)
{
	k_ticks_t ticks = 0;
	static bool heap_callback_set;

	if (K_TIMEOUT_EQ(timeout, K_FOREVER)) {
		return 0;
	}

#ifdef CONFIG_KERNEL_COHERENCE
	__ASSERT_NO_MSG(sys_cache_is_mem_coherent(to));
#endif /* CONFIG_KERNEL_COHERENCE */

	__ASSERT(to->heap_idx == TIMEOUT_NOT_IN_HEAP,
		"timeout already in heap: to=%p, heap_idx=%zu, fn=%p",
		to, to->heap_idx, fn);

	to->fn = fn;

	K_SPINLOCK(&timeout_lock) {
		/* One-time initialization of heap callback (protected by timeout_lock) */
		if (!heap_callback_set) {
			min_heap_set_update_callback(&timeout_heap, timeout_update_index);
			heap_callback_set = true;
		}

		int32_t ticks_elapsed;
		bool has_elapsed = false;

		if (Z_IS_TIMEOUT_RELATIVE(timeout)) {
			ticks_elapsed = elapsed();
			has_elapsed = true;
			/* Calculate absolute expiry tick */
			to->dticks = curr_tick + timeout.ticks + 1 + ticks_elapsed;
			ticks = to->dticks;
		} else {
			/* Already an absolute timeout */
			to->dticks = Z_TICK_ABS(timeout.ticks);
			ticks = to->dticks;
		}

		/* Create heap element and push to heap */
		struct timeout_heap_elem elem = {.timeout = to, .seq = timeout_seq++ };
		int ret = min_heap_push(&timeout_heap, &elem);

		if (ret != 0) {
			/* Heap is full - this is a critical error */
			/* Restore timeout to inactive state */
			to->heap_idx = TIMEOUT_NOT_IN_HEAP;
			to->fn = NULL;
			to->dticks = 0;
			ticks = 0;
			__ASSERT(false, "Timeout heap is full!");
			K_SPINLOCK_BREAK;
		}

		/* heap_idx is automatically updated by the callback during heapify */

		if (to == first() && announce_remaining == 0) {
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

int z_abort_timeout(struct _timeout *to)
{
	int ret = -EINVAL;

	K_SPINLOCK(&timeout_lock) {
		if (to->heap_idx != TIMEOUT_NOT_IN_HEAP) {
			/* Convert 1-based heap_idx to 0-based for heap operations */
			size_t heap_index = to->heap_idx - 1;

			if (heap_index < timeout_heap.size) {
				bool is_first = (to == first());

				struct timeout_heap_elem elem;
				bool removed = min_heap_remove(&timeout_heap, heap_index, &elem);

				if (removed) {
					to->heap_idx = TIMEOUT_NOT_IN_HEAP;
					to->dticks = TIMEOUT_DTICKS_ABORTED;
					ret = 0;

					/* heap_idx is automatically updated by the callback during heapify */

					if (is_first) {
						sys_clock_set_timeout(next_timeout(elapsed()), false);
					}
				}
			}
		}
	}

	return ret;
}

/* must be locked */
static k_ticks_t timeout_rem(const struct _timeout *timeout)
{
	if (timeout->heap_idx == TIMEOUT_NOT_IN_HEAP) {
		return K_TICKS_FOREVER;
	}

	k_ticks_t now = curr_tick + elapsed();
	k_ticks_t expiry = timeout->dticks;

	/* Use wrap-around safe comparison */
#ifdef CONFIG_TIMEOUT_64BIT
	if (time_after_eq(expiry, now)) {
		return expiry - now;
	}
#else
	if (time_after_eq(expiry, (int32_t)now)) {
		return expiry - now;
	}
#endif
	return 0;
}

k_ticks_t z_timeout_remaining(const struct _timeout *timeout)
{
	k_ticks_t ticks = 0;

	K_SPINLOCK(&timeout_lock) {
		if (!z_is_inactive_timeout(timeout)) {
			ticks = timeout_rem(timeout);
		}
	}

	return ticks;
}
EXPORT_SYMBOL(z_timeout_remaining);

k_ticks_t z_timeout_expires(const struct _timeout *timeout)
{
	k_ticks_t ticks = K_TICKS_FOREVER;

	K_SPINLOCK(&timeout_lock) {
		if (!z_is_inactive_timeout(timeout)) {
			if (timeout->heap_idx != TIMEOUT_NOT_IN_HEAP) {
				ticks = timeout->dticks;
			}
		}
	}

	return ticks;
}
EXPORT_SYMBOL(z_timeout_expires);

int32_t z_get_next_timeout_expiry(void)
{
	int32_t ret = (int32_t) K_TICKS_FOREVER;

	K_SPINLOCK(&timeout_lock) {
		ret = next_timeout(elapsed());
	}
	return ret;
}

void sys_clock_announce(int32_t ticks)
{
	k_spinlock_key_t key = k_spin_lock(&timeout_lock);

	/* We release the lock around the callbacks below, so on SMP
	 * systems someone might be already running the loop.  Don't
	 * race (which will cause parallel execution of "sequential"
	 * timeouts and confuse apps), just increment the tick count
	 * and return.
	 */
	if (IS_ENABLED(CONFIG_SMP) && (announce_remaining != 0)) {
		announce_remaining += ticks;
		k_spin_unlock(&timeout_lock, key);
		return;
	}

	announce_remaining = ticks;
	curr_tick += ticks;

	struct _timeout *t;

	/* Process all expired timeouts
	 * Use wrap-around safe comparison: timeout has expired if curr_tick >= dticks
	 */
#ifdef CONFIG_TIMEOUT_64BIT
	while ((t = first()) != NULL && time_after_eq(curr_tick, t->dticks)) {
#else
	while ((t = first()) != NULL && time_after_eq((int32_t)curr_tick, t->dticks)) {
#endif
		struct timeout_heap_elem elem;
		bool removed = min_heap_pop(&timeout_heap, &elem);

		if (!removed) {
			break;
		}

		t = elem.timeout;
		t->heap_idx = TIMEOUT_NOT_IN_HEAP;

		/* heap_idx is automatically updated by the callback during heapify */

		k_spin_unlock(&timeout_lock, key);
		t->fn(t);
		key = k_spin_lock(&timeout_lock);
	}

	announce_remaining = 0;

	sys_clock_set_timeout(next_timeout(0), false);

	k_spin_unlock(&timeout_lock, key);

#ifdef CONFIG_TIMESLICING
	z_time_slice();
#endif /* CONFIG_TIMESLICING */
}

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
