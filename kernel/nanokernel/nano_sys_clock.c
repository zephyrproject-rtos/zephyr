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


#include <nano_private.h>
#include <toolchain.h>
#include <sections.h>
#include <wait_q.h>
#include <drivers/system_timer.h>

#ifdef CONFIG_NANOKERNEL

#ifdef CONFIG_SYS_CLOCK_EXISTS
int sys_clock_us_per_tick = 1000000 / sys_clock_ticks_per_sec;
int sys_clock_hw_cycles_per_tick =
	sys_clock_hw_cycles_per_sec / sys_clock_ticks_per_sec;
#else
/* don't initialize to avoid division-by-zero error */
int sys_clock_us_per_tick;
int sys_clock_hw_cycles_per_tick;
#endif


/* updated by timer driver for tickless, stays at 1 for non-tickless */
uint32_t _sys_idle_elapsed_ticks = 1;
#endif /*  CONFIG_NANOKERNEL */

int64_t _nano_ticks = 0;

/**
 *
 * @brief Return the lower part of the current system tick count
 *
 * @return the current system tick count
 *
 */
uint32_t nano_tick_get_32(void)
{
	return (uint32_t)_nano_ticks;
}

/**
 *
 * @brief Return the current system tick count
 *
 * @return the current system tick count
 *
 */
int64_t nano_tick_get(void)
{
	int64_t tmp_nano_ticks;
	/*
	 * Lock the interrupts when reading _nano_ticks 64-bit variable.
	 * Some architectures (x86) do not handle 64-bit atomically, so
	 * we have to lock the timer interrupt that causes change of
	 * _nano_ticks
	 */
	unsigned int imask = irq_lock();
	tmp_nano_ticks = _nano_ticks;
	irq_unlock(imask);
	return tmp_nano_ticks;
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
 * Since a code fragment that wants to use nano_tick_delta passes in its
 * own reference time variable, multiple code fragments can make use of this
 * function concurrently.
 *
 * e.g.
 * uint64_t  reftime;
 * (void) nano_tick_delta(&reftime);  /# prime it #/
 * [do stuff]
 * x = nano_tick_delta(&reftime);     /# how long since priming #/
 * [do more stuff]
 * y = nano_tick_delta(&reftime);     /# how long since [do stuff] #/
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
	 * Lock the interrupts when reading _nano_ticks 64-bit variable.
	 * Some architectures (x86) do not handle 64-bit atomically, so
	 * we have to lock the timer interrupt that causes change of
	 * _nano_ticks
	 */
	unsigned int imask = irq_lock();
	saved = _nano_ticks;
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
int64_t nano_tick_delta(int64_t *reftime)
{
	return _nano_tick_delta(reftime);
}


uint32_t nano_tick_delta_32(int64_t *reftime)
{
	return (uint32_t)_nano_tick_delta(reftime);
}

/* handle the expired timeouts in the nano timeout queue */

#ifdef CONFIG_NANO_TIMEOUTS
#include <wait_q.h>

static inline void handle_expired_nano_timeouts(int ticks)
{
	struct _nano_timeout *head =
		(struct _nano_timeout *)sys_dlist_peek_head(&_nanokernel.timeout_q);

	if (head) {
		head->delta_ticks_from_prev -= ticks;
		_nano_timeout_handle_timeouts();
	}
}
#else
	#define handle_expired_nano_timeouts(ticks) do { } while ((0))
#endif

/* handle the expired nano timers in the nano timers queue */
#ifdef CONFIG_NANO_TIMERS
#include <sys_clock.h>
static inline void handle_expired_nano_timers(int ticks)
{
	if (_nano_timer_list) {
		_nano_timer_list->ticks -= ticks;

		while (_nano_timer_list && (!_nano_timer_list->ticks)) {
			struct nano_timer *expired = _nano_timer_list;
			struct nano_lifo *lifo = &expired->lifo;
			_nano_timer_list = expired->link;
			nano_isr_lifo_put(lifo, expired->userData);
		}
	}
}
#else
	#define handle_expired_nano_timers(ticks) do { } while ((0))
#endif

#if defined(CONFIG_NANO_TIMEOUTS) || defined(CONFIG_NANO_TIMERS)
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
void _nano_sys_clock_tick_announce(uint32_t ticks)
{
	_nano_ticks += ticks;
	handle_expired_nano_timeouts((int)ticks);
	handle_expired_nano_timers((int)ticks);
}
#endif

/* get closest nano timers deadline expiry, (uint32_t)TICKS_UNLIMITED if none */
#ifdef CONFIG_NANO_TIMERS
static inline uint32_t _nano_get_earliest_timers_deadline(void)
{
	return _nano_timer_list ? _nano_timer_list->ticks : TICKS_UNLIMITED;
}
#else
static inline uint32_t _nano_get_earliest_timers_deadline(void)
{
	return TICKS_UNLIMITED;
}
#endif

/*
 * Get closest nano timeouts/timers deadline expiry, (uint32_t)TICKS_UNLIMITED
 * if none.
 */
uint32_t _nano_get_earliest_deadline(void)
{
	return min(_nano_get_earliest_timeouts_deadline(),
				_nano_get_earliest_timers_deadline());
}
