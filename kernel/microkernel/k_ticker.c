/* k_ticker.c - microkernel tick event handler */

/*
 * Copyright (c) 1997-2010, 2012-2015 Wind River Systems, Inc.
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

/*
 * DESCRIPTION
 * This module implements the microkernel's tick event handler.
 */

#include <nanokernel.h>
#include <arch/cpu.h>

#include <micro_private.h>
#include <drivers/system_timer.h>
#include <microkernel.h>
#include <microkernel/ticks.h>
#include <toolchain.h>
#include <sections.h>

#ifdef CONFIG_TIMESLICING
static int32_t slice_count = (int32_t)0;
static int32_t slice_time = (int32_t)CONFIG_TIMESLICE_SIZE;
static kpriority_t slice_prio =
	(kpriority_t)CONFIG_TIMESLICE_PRIORITY;
#endif /* CONFIG_TIMESLICING */

#ifdef CONFIG_TICKLESS_IDLE
/* Number of ticks elapsed that have not been announced to the microkernel */
int32_t _sys_idle_elapsed_ticks; /* Initial value must be 0 */
#endif

int32_t task_tick_get_32(void)
{
	return (int32_t)_sys_clock_tick_count;
}

int64_t task_tick_get(void)
{
	int64_t ticks;
	int key = irq_lock();

	ticks = _sys_clock_tick_count;
	irq_unlock(key);
	return ticks;
}

/**
 * @internal
 * @brief Task level debugging tick handler
 *
 * If task level debugging is configured this routine updates the low resolution
 * debugging timer and determines if task level processing should be suspended.
 *
 * @return 0 if task level processing should be halted or 1 if not
 *
 */
#ifdef CONFIG_TASK_DEBUG
uint32_t __noinit _k_debug_sys_clock_tick_count;

static inline int _TlDebugUpdate(int32_t ticks)
{
	_k_debug_sys_clock_tick_count += ticks;
	return !_k_debug_halt;
}
#else
#define _TlDebugUpdate(ticks) 1
#endif

/**
 * @internal
 * @brief Tick handler time slice logic
 *
 * This routine checks to see if it is time for the current task
 * to relinquish control, and yields CPU if so.
 *
 * @return N/A
 *
 */
static inline void _TimeSliceUpdate(void)
{
#ifdef CONFIG_TIMESLICING
	int yield = slice_time && (_k_current_task->priority >= slice_prio) &&
		    (++slice_count >= slice_time);
	if (yield) {
		slice_count = 0;
		_k_task_yield(NULL);
	}
#else
/* do nothing */
#endif /* CONFIG_TIMESLICING */
}

/**
 * @internal
 * @brief Get elapsed ticks
 *
 * If tickless idle support is configured this routine returns the number
 * of ticks since going idle and then resets the global elapsed tick counter back
 * to zero indicating all elapsed ticks have been consumed. This is done with
 * interrupts locked to prevent the timer ISR from modifying the global elapsed
 * tick counter.
 * If tickless idle support is not configured in it simply returns 1.
 *
 * @return number of ticks to process
 */
static inline int32_t _SysIdleElapsedTicksGet(void)
{
#ifdef CONFIG_TICKLESS_IDLE
	int32_t ticks;
	int key;

	key = irq_lock();
	ticks = _sys_idle_elapsed_ticks;
	_sys_idle_elapsed_ticks = 0;
	irq_unlock(key);
	return ticks;
#else
	/* A single tick always elapses when not in tickless mode */
	return 1;
#endif
}

/**
 *
 * @brief Microkernel tick handler
 *
 * This routine informs other microkernel subsystems that a tick event has
 * occurred.
 * @param even Event
 * @return 1
 */
int _k_ticker(int event)
{
	(void)event; /* prevent "unused argument" compiler warning */
	int32_t ticks;

	ticks = _SysIdleElapsedTicksGet();

	_k_workload_monitor_update();

	if (_TlDebugUpdate(ticks)) {
		_TimeSliceUpdate();
		_k_timer_list_update(ticks);
		_nano_sys_clock_tick_announce(ticks);
	}

	return 1;
}

#ifdef CONFIG_TIMESLICING

void sys_scheduler_time_slice_set(int32_t t, kpriority_t p)
{
	slice_time = t;
	slice_prio = p;
}

#endif /* CONFIG_TIMESLICING */

/**
 *
 * @brief Handle elapsed ticks calculation request
 *
 * This routine, called by _k_server(), handles the request for calculating the
 * time elapsed since the specified reference time.
 *
 * @return N/A
 */
void _k_time_elapse(struct k_args *P)
{
	int64_t now = task_tick_get();

	P->args.c1.time2 = now - P->args.c1.time1;
	P->args.c1.time1 = now;
}

int64_t task_tick_delta(int64_t *reftime /* pointer to reference time */
			)
{
	struct k_args A;

	A.Comm = _K_SVC_TIME_ELAPSE;
	A.args.c1.time1 = *reftime;
	KERNEL_ENTRY(&A);
	*reftime = A.args.c1.time1;
	return A.args.c1.time2;
}
