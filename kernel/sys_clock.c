/* system clock support */

/*
 * Copyright (c) 1997-2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <kernel_structs.h>
#include <toolchain.h>
#include <sections.h>
#include <wait_q.h>
#include <drivers/system_timer.h>

#ifdef CONFIG_SYS_CLOCK_EXISTS
#ifdef _NON_OPTIMIZED_TICKS_PER_SEC
#warning "non-optimized system clock frequency chosen: performance may suffer"
#endif
#endif

#ifdef CONFIG_SYS_CLOCK_EXISTS
int sys_clock_us_per_tick = 1000000 / sys_clock_ticks_per_sec;
int sys_clock_hw_cycles_per_tick =
	CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC / sys_clock_ticks_per_sec;
#if defined(CONFIG_TIMER_READS_ITS_FREQUENCY_AT_RUNTIME)
int sys_clock_hw_cycles_per_sec = CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC;
#endif
#else
/* don't initialize to avoid division-by-zero error */
int sys_clock_us_per_tick;
int sys_clock_hw_cycles_per_tick;
#if defined(CONFIG_TIMER_READS_ITS_FREQUENCY_AT_RUNTIME)
int sys_clock_hw_cycles_per_sec;
#endif
#endif

/* updated by timer driver for tickless, stays at 1 for non-tickless */
int32_t _sys_idle_elapsed_ticks = 1;

int64_t _sys_clock_tick_count;

/**
 *
 * @brief Return the lower part of the current system tick count
 *
 * @return the current system tick count
 *
 */
uint32_t _tick_get_32(void)
{
	return (uint32_t)_sys_clock_tick_count;
}
FUNC_ALIAS(_tick_get_32, sys_tick_get_32, uint32_t);

uint32_t k_uptime_get_32(void)
{
	return __ticks_to_ms(_tick_get_32());
}

/**
 *
 * @brief Return the current system tick count
 *
 * @return the current system tick count
 *
 */
int64_t _tick_get(void)
{
	int64_t tmp_sys_clock_tick_count;
	/*
	 * Lock the interrupts when reading _sys_clock_tick_count 64-bit
	 * variable. Some architectures (x86) do not handle 64-bit atomically,
	 * so we have to lock the timer interrupt that causes change of
	 * _sys_clock_tick_count
	 */
	unsigned int imask = irq_lock();

	tmp_sys_clock_tick_count = _sys_clock_tick_count;
	irq_unlock(imask);
	return tmp_sys_clock_tick_count;
}
FUNC_ALIAS(_tick_get, sys_tick_get, int64_t);

int64_t k_uptime_get(void)
{
	return __ticks_to_ms(_tick_get());
}

/**
 *
 * @brief Return number of ticks since a reference time
 *
 * This function is meant to be used in contained fragments of code. The first
 * call to it in a particular code fragment fills in a reference time variable
 * which then gets passed and updated every time the function is called. From
 * the second call on, the delta between the value passed to it and the current
 * tick count is the return value. Since the first call is meant to only fill in
 * the reference time, its return value should be discarded.
 *
 * Since a code fragment that wants to use sys_tick_delta() passes in its
 * own reference time variable, multiple code fragments can make use of this
 * function concurrently.
 *
 * e.g.
 * uint64_t  reftime;
 * (void) sys_tick_delta(&reftime);  /# prime it #/
 * [do stuff]
 * x = sys_tick_delta(&reftime);     /# how long since priming #/
 * [do more stuff]
 * y = sys_tick_delta(&reftime);     /# how long since [do stuff] #/
 *
 * @return tick count since reference time; undefined for first invocation
 *
 * NOTE: We use inline function for both 64-bit and 32-bit functions.
 * Compiler optimizes out 64-bit result handling in 32-bit version.
 */
static ALWAYS_INLINE int64_t _nano_tick_delta(int64_t *reftime)
{
	int64_t  delta;
	int64_t  saved;

	/*
	 * Lock the interrupts when reading _sys_clock_tick_count 64-bit
	 * variable.  Some architectures (x86) do not handle 64-bit atomically,
	 * so we have to lock the timer interrupt that causes change of
	 * _sys_clock_tick_count
	 */
	unsigned int imask = irq_lock();

	saved = _sys_clock_tick_count;
	irq_unlock(imask);
	delta = saved - (*reftime);
	*reftime = saved;

	return delta;
}

/**
 *
 * @brief Return number of ticks since a reference time
 *
 * @return tick count since reference time; undefined for first invocation
 */
int64_t sys_tick_delta(int64_t *reftime)
{
	return _nano_tick_delta(reftime);
}


uint32_t sys_tick_delta_32(int64_t *reftime)
{
	return (uint32_t)_nano_tick_delta(reftime);
}

int64_t k_uptime_delta(int64_t *reftime)
{
	int64_t uptime, delta;

	uptime = k_uptime_get();
	delta = uptime - *reftime;
	*reftime = uptime;

	return delta;
}

uint32_t k_uptime_delta_32(int64_t *reftime)
{
	return (uint32_t)k_uptime_delta(reftime);
}

/* handle the expired timeouts in the nano timeout queue */

#ifdef CONFIG_SYS_CLOCK_EXISTS
#include <wait_q.h>

/*
 * Handle timeouts by dequeuing the expired ones from _timeout_q and queue
 * them on a local one, then doing the real handling from that queue. This
 * allows going through the second queue without needing to have the
 * interrupts locked since it is a local queue. Each expired timeout is marked
 * as _EXPIRED so that an ISR preempting us and releasing an object on which
 * a thread was timing out and expired will not give the object to that thread.
 *
 * Always called from interrupt level, and always only from the system clock
 * interrupt.
 */

volatile int _handling_timeouts;

static inline void handle_timeouts(int32_t ticks)
{
	sys_dlist_t expired;
	unsigned int key;

	/* init before locking interrupts */
	sys_dlist_init(&expired);

	key = irq_lock();

	struct _timeout *head =
		(struct _timeout *)sys_dlist_peek_head(&_timeout_q);

	K_DEBUG("head: %p, delta: %d\n",
		head, head ? head->delta_ticks_from_prev : -2112);

	if (!head) {
		irq_unlock(key);
		return;
	}

	head->delta_ticks_from_prev -= ticks;

	/*
	 * Dequeue all expired timeouts from _timeout_q, relieving irq lock
	 * pressure between each of them, allowing handling of higher priority
	 * interrupts. We know that no new timeout will be prepended in front
	 * of a timeout which delta is 0, since timeouts of 0 ticks are
	 * prohibited.
	 */
	sys_dnode_t *next = &head->node;
	struct _timeout *timeout = (struct _timeout *)next;

	_handling_timeouts = 1;

	while (timeout && timeout->delta_ticks_from_prev == 0) {

		sys_dlist_remove(next);

		/*
		 * Reverse the order that that were queued in the timeout_q:
		 * timeouts expiring on the same ticks are queued in the
		 * reverse order, time-wise, that they are added to shorten the
		 * amount of time with interrupts locked while walking the
		 * timeout_q. By reversing the order _again_ when building the
		 * expired queue, they end up being processed in the same order
		 * they were added, time-wise.
		 */
		sys_dlist_prepend(&expired, next);

		timeout->delta_ticks_from_prev = _EXPIRED;

		irq_unlock(key);
		key = irq_lock();

		next = sys_dlist_peek_head(&_timeout_q);
		timeout = (struct _timeout *)next;
	}

	irq_unlock(key);

	_handle_expired_timeouts(&expired);

	_handling_timeouts = 0;
}
#else
	#define handle_timeouts(ticks) do { } while ((0))
#endif

#ifdef CONFIG_TIMESLICING
int32_t _time_slice_elapsed;
int32_t _time_slice_duration = CONFIG_TIMESLICE_SIZE;
int  _time_slice_prio_ceiling = CONFIG_TIMESLICE_PRIORITY;

/*
 * Always called from interrupt level, and always only from the system clock
 * interrupt, thus:
 * - _current does not have to be protected, since it only changes at thread
 *   level or when exiting a non-nested interrupt
 * - _time_slice_elapsed does not have to be protected, since it can only change
 *   in this function and at thread level
 * - _time_slice_duration does not have to be protected, since it can only
 *   change at thread level
 */
static void handle_time_slicing(int32_t ticks)
{
	if (_time_slice_duration == 0) {
		return;
	}

	if (_is_prio_higher(_current->base.prio, _time_slice_prio_ceiling)) {
		return;
	}

	_time_slice_elapsed += __ticks_to_ms(ticks);
	if (_time_slice_elapsed >= _time_slice_duration) {

		unsigned int key;

		_time_slice_elapsed = 0;

		key = irq_lock();
		_move_thread_to_end_of_prio_q(_current);
		irq_unlock(key);
	}
}
#else
#define handle_time_slicing(ticks) do { } while (0)
#endif
/**
 *
 * @brief Announce a tick to the kernel
 *
 * This function is only to be called by the system clock timer driver when a
 * tick is to be announced to the kernel. It takes care of dequeuing the
 * timers that have expired and wake up the threads pending on them.
 *
 * @return N/A
 */
void _nano_sys_clock_tick_announce(int32_t ticks)
{
	unsigned int  key;

	K_DEBUG("ticks: %d\n", ticks);

	/* 64-bit value, ensure atomic access with irq lock */
	key = irq_lock();
	_sys_clock_tick_count += ticks;
	irq_unlock(key);

	handle_timeouts(ticks);

	/* time slicing is basically handled like just yet another timeout */
	handle_time_slicing(ticks);
}
