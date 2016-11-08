/* system clock support for nanokernel-only systems */

/*
 * Copyright (c) 1997-2015 Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#include <kernel_structs.h>
#include <toolchain.h>
#include <sections.h>
#include <wait_q.h>
#include <drivers/system_timer.h>

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

static inline void handle_expired_timeouts(int32_t ticks)
{
	struct _timeout *head =
		(struct _timeout *)sys_dlist_peek_head(&_timeout_q);

	K_DEBUG("head: %p, delta: %d\n",
		head, head ? head->delta_ticks_from_prev : -2112);

	if (head) {
		head->delta_ticks_from_prev -= ticks;
		_handle_timeouts();
	}
}
#else
	#define handle_expired_timeouts(ticks) do { } while ((0))
#endif

#ifdef CONFIG_TIMESLICING
int32_t _time_slice_elapsed;
int32_t _time_slice_duration = CONFIG_TIMESLICE_SIZE;
int  _time_slice_prio_ceiling = CONFIG_TIMESLICE_PRIORITY;

static void handle_time_slicing(int32_t ticks)
{
	if (_time_slice_duration == 0) {
		return;
	}

	if (_is_prio_higher(_current->base.prio, _time_slice_prio_ceiling)) {
		return;
	}

	_time_slice_elapsed += _ticks_to_ms(ticks);
	if (_time_slice_elapsed >= _time_slice_duration) {
		_time_slice_elapsed = 0;
		_move_thread_to_end_of_prio_q(_current);
	}
}
#else
#define handle_time_slicing(ticks) do { } while (0)
#endif
/**
 *
 * @brief Announce a tick to the nanokernel
 *
 * This function is only to be called by the system clock timer driver when a
 * tick is to be announced to the nanokernel. It takes care of dequeuing the
 * timers that have expired and wake up the fibers pending on them.
 *
 * @return N/A
 */
void _nano_sys_clock_tick_announce(int32_t ticks)
{
	unsigned int  key;

	K_DEBUG("ticks: %d\n", ticks);

	key = irq_lock();
	_sys_clock_tick_count += ticks;
	handle_expired_timeouts(ticks);

	handle_time_slicing(ticks);

	irq_unlock(key);
}
