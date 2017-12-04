/* system clock support */

/*
 * Copyright (c) 1997-2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <kernel_structs.h>
#include <toolchain.h>
#include <linker/sections.h>
#include <wait_q.h>
#include <drivers/system_timer.h>
#include <syscall_handler.h>

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
s32_t _sys_idle_elapsed_ticks = 1;

volatile u64_t _sys_clock_tick_count;

#ifdef CONFIG_TICKLESS_KERNEL
/*
 * If this flag is set, system clock will run continuously even if
 * there are no timer events programmed. This allows using the
 * system clock to track passage of time without interruption.
 * To save power, this should be turned on only when required.
 */
int _sys_clock_always_on;

static u32_t next_ts;
#endif
/**
 *
 * @brief Return the lower part of the current system tick count
 *
 * @return the current system tick count
 *
 */
u32_t _tick_get_32(void)
{
#ifdef CONFIG_TICKLESS_KERNEL
	return (u32_t)_get_elapsed_clock_time();
#else
	return (u32_t)_sys_clock_tick_count;
#endif
}
FUNC_ALIAS(_tick_get_32, sys_tick_get_32, u32_t);

u32_t _impl_k_uptime_get_32(void)
{
#ifdef CONFIG_TICKLESS_KERNEL
	__ASSERT(_sys_clock_always_on,
		 "Call k_enable_sys_clock_always_on to use clock API");
#endif
	return __ticks_to_ms(_tick_get_32());
}

#ifdef CONFIG_USERSPACE
_SYSCALL_HANDLER(k_uptime_get_32)
{
#ifdef CONFIG_TICKLESS_KERNEL
	_SYSCALL_VERIFY(_sys_clock_always_on);
#endif
	return _impl_k_uptime_get_32();
}
#endif

/**
 *
 * @brief Return the current system tick count
 *
 * @return the current system tick count
 *
 */
s64_t _tick_get(void)
{
	s64_t tmp_sys_clock_tick_count;
	/*
	 * Lock the interrupts when reading _sys_clock_tick_count 64-bit
	 * variable. Some architectures (x86) do not handle 64-bit atomically,
	 * so we have to lock the timer interrupt that causes change of
	 * _sys_clock_tick_count
	 */
	unsigned int imask = irq_lock();

#ifdef CONFIG_TICKLESS_KERNEL
	tmp_sys_clock_tick_count = _get_elapsed_clock_time();
#else
	tmp_sys_clock_tick_count = _sys_clock_tick_count;
#endif
	irq_unlock(imask);
	return tmp_sys_clock_tick_count;
}
FUNC_ALIAS(_tick_get, sys_tick_get, s64_t);

s64_t _impl_k_uptime_get(void)
{
#ifdef CONFIG_TICKLESS_KERNEL
	__ASSERT(_sys_clock_always_on,
		 "Call k_enable_sys_clock_always_on to use clock API");
#endif
	return __ticks_to_ms(_tick_get());
}

#ifdef CONFIG_USERSPACE
_SYSCALL_HANDLER(k_uptime_get, ret_p)
{
	u64_t *ret = (u64_t *)ret_p;

	_SYSCALL_MEMORY_WRITE(ret, sizeof(*ret));
	*ret = _impl_k_uptime_get();
	return 0;
}
#endif

s64_t k_uptime_delta(s64_t *reftime)
{
	s64_t uptime, delta;

	uptime = k_uptime_get();
	delta = uptime - *reftime;
	*reftime = uptime;

	return delta;
}

u32_t k_uptime_delta_32(s64_t *reftime)
{
	return (u32_t)k_uptime_delta(reftime);
}

/* handle the expired timeouts in the nano timeout queue */

#ifdef CONFIG_SYS_CLOCK_EXISTS
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

static inline void handle_timeouts(s32_t ticks)
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
s32_t _time_slice_elapsed;
s32_t _time_slice_duration = CONFIG_TIMESLICE_SIZE;
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
static void handle_time_slicing(s32_t ticks)
{
#ifdef CONFIG_TICKLESS_KERNEL
	next_ts = 0;
#endif
	if (!_is_thread_time_slicing(_current)) {
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
#ifdef CONFIG_TICKLESS_KERNEL
	next_ts =
	    _ms_to_ticks(_time_slice_duration - _time_slice_elapsed);
#endif
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
void _nano_sys_clock_tick_announce(s32_t ticks)
{
#ifndef CONFIG_TICKLESS_KERNEL
	unsigned int  key;

	K_DEBUG("ticks: %d\n", ticks);

	/* 64-bit value, ensure atomic access with irq lock */
	key = irq_lock();
	_sys_clock_tick_count += ticks;
	irq_unlock(key);
#endif
	handle_timeouts(ticks);

	/* time slicing is basically handled like just yet another timeout */
	handle_time_slicing(ticks);

#ifdef CONFIG_TICKLESS_KERNEL
	u32_t next_to = _get_next_timeout_expiry();

	next_to = next_to == K_FOREVER ? 0 : next_to;
	next_to = !next_to || (next_ts
			       && next_to) > next_ts ? next_ts : next_to;

	u32_t remaining = _get_remaining_program_time();

	if ((!remaining && next_to) || (next_to < remaining)) {
		/* Clears current program if next_to = 0 and remaining > 0 */
		_set_time(next_to);
	}
#endif
}
