/*
 * Copyright (c) 2011-2014 Wind River Systems, Inc.
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

/**
 * @file
 * @brief Nanokernel idle support
 *
 * This module provides routines to set the idle field in the nanokernel
 * data structure.
 */


#include <nanokernel.h>
#include <nano_private.h>
#include <toolchain.h>
#include <sections.h>
#include <drivers/system_timer.h>
#include <wait_q.h>

/**
 *
 * @brief Indicate that nanokernel is idling in tickless mode
 *
 * Sets the nanokernel data structure idle field to a non-zero value.
 *
 * @param ticks the number of ticks to idle
 *
 * @return N/A
 */
void nano_cpu_set_idle(int32_t ticks)
{
	extern tNANO _nanokernel;

	_nanokernel.idle = ticks;
}

#if defined(CONFIG_NANOKERNEL) && defined(CONFIG_TICKLESS_IDLE)

int32_t _sys_idle_ticks_threshold = CONFIG_TICKLESS_IDLE_THRESH;

#if defined(CONFIG_NANO_TIMEOUTS)
static inline int32_t get_next_timeout_expiry(void)
{
	return _nano_get_earliest_timeouts_deadline();
}
#else
#define get_next_timeout_expiry(void) TICKS_UNLIMITED
#endif

/**
*
* @brief - obtain number of ticks until next timer expires
*
* Must be called with interrupts locked to prevent the timer queues from
* changing.
*
* @return Number of ticks until next timer expires.
*
*/

static inline int32_t get_next_timer_expiry(void)
{
	return _nano_timer_list ? _nano_timer_list->ticks : TICKS_UNLIMITED;
}

static inline int was_in_tickless_idle(void)
{
	return	(_nanokernel.idle == TICKS_UNLIMITED) ||
			(_nanokernel.idle >= _sys_idle_ticks_threshold);
}

static inline int must_enter_tickless_idle(void)
{
	/* uses same logic as was_in_tickless_idle() */
	return was_in_tickless_idle();
}

static inline int32_t get_next_tick_expiry(void)
{
	int32_t timers = get_next_timer_expiry();
	int32_t timeouts = get_next_timeout_expiry();

	return (int32_t)min((uint32_t)timers, (uint32_t)timeouts);
}

void _power_save_idle(void)
{
	_nanokernel.idle = get_next_tick_expiry();

	if (must_enter_tickless_idle()) {
		_timer_idle_enter((uint32_t)_nanokernel.idle);
	}
}

void _power_save_idle_exit(void)
{
	if (was_in_tickless_idle()) {
		_timer_idle_exit();
		_nanokernel.idle = 0;
	}
}

#endif /* CONFIG_NANOKERNEL && CONFIG_TICKLESS_IDLE */
