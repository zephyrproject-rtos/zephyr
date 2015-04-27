/* ticker.c - microkernel tick event handler */

/*
 * Copyright (c) 1997-2010, 2012-2015 Wind River Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Wind River Systems nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
DESCRIPTION
This module implements the microkernel's tick event handler.
*/

#include <nanokernel.h>
#include <nanokernel/cpu.h>

#include <microkernel/k_struct.h>
#include <minik.h>
#include <kticks.h> /* WL stuff */
#include <drivers/system_timer.h>
#include <ktask.h>  /* K_yield */
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
int32_t _sys_idle_elapsed_ticks = 0; /* Initial value must be 0 */
#endif

/* units: us/tick */
int sys_clock_us_per_tick = 1000000 / sys_clock_ticks_per_sec;
/* units: # clock cycles/tick */
int sys_clock_hw_cycles_per_tick =
	sys_clock_hw_cycles_per_sec / sys_clock_ticks_per_sec;

/*******************************************************************************
*
* _HandleExpiredTimers - handle expired timers
*
* If non-lite system is configured this routine processes the sorted list of
* timers associated with waiting tasks and activate each task whose timer
* has now expired.
*
* With tickless idle, a tick announcement may encompass multiple ticks.
* Due to limitations of the underlying timer driver, the number of elapsed
* ticks may -- under very rare circumstances -- exceed the first timer's
* remaining tick count, although never by more a single tick. This means that
* a task timer may occasionally expire one tick later than it was scheduled to,
* and that a periodic timer may exhibit a slow, ever-increasing degree of drift
* from the main system timer over long intervals.
*
* RETURNS: N/A
*
* \NOMANUAL
*/

static inline void _HandleExpiredTimers(int ticks)
{
#ifdef LITE
/* do nothing */
#else
	K_TIMER *T;

	while (_k_timer_list_head != NULL) {
		_k_timer_list_head->Ti -= ticks;

		if (_k_timer_list_head->Ti > 0) {
			return;
		}

		T = _k_timer_list_head;
		if (T == _k_timer_list_tail) {
			_k_timer_list_head = _k_timer_list_tail = NULL;
		} else {
			_k_timer_list_head = T->Forw;
			_k_timer_list_head->Back = NULL;
		}
		if (T->Tr) {
			T->Ti = T->Tr;
			enlist_timer(T);
		} else {
			T->Ti = -1;
		}
		TO_ALIST(&_k_command_stack, T->Args);

		ticks = 0; /* don't decrement Ti for subsequent timer(s) */
	}
#endif
}

/*******************************************************************************
*
* _WlMonitorUpdate - workload monitor tick handler
*
* If workload monitor is configured this routine updates the global variables
* it uses to record the passage of time.
*
* RETURNS: N/A
*
* \NOMANUAL
*/

static inline void _WlMonitorUpdate(void)
{
#ifdef CONFIG_WORKLOAD_MONITOR
	if (--_k_workload_ticks == 0) {
		_k_workload_t0 = _k_workload_t1;
		_k_workload_t1 = timer_read();
		_k_workload_n0 = _k_workload_n1;
		_k_workload_n1 = _k_workload_i - 1;
		_k_workload_ticks = _k_workload_slice;
	}
#else
/* do nothing */
#endif
}

/*******************************************************************************
*
* _TlDebugUpdate - task level debugging tick handler
*
* If task level debugging is configured this routine updates the low resolution
* debugging timer and determines if task level processing should be suspended.
*
* RETURNS: 0 if task level processing should be halted or 1 if not
*
* \NOMANUAL
*/

#ifdef CONFIG_TASK_DEBUG
uint32_t __noinit K_DebugLowTime;

static inline int _TlDebugUpdate(int32_t ticks)
{
	K_DebugLowTime += ticks;
	return !_k_debug_halt;
}
#else
#define _TlDebugUpdate(ticks) 1
#endif

/*******************************************************************************
*
* _TimeSliceUpdate - tick handler time slice logic
*
* This routine checks to see if it is time for the current task
* to relinquish control, and yields CPU if so.
*
* RETURNS: N/A
*
* \NOMANUAL
*/

static inline void _TimeSliceUpdate(void)
{
#ifdef CONFIG_TIMESLICING
	int yield = slice_time && (_k_current_task->Prio >= slice_prio) &&
		    (++slice_count >= slice_time);
	if (yield) {
		slice_count = 0;
		K_yield(NULL);
	}
#else
/* do nothing */
#endif /* CONFIG_TIMESLICING */
}

/*******************************************************************************
*
* _SysIdleElapsedTicksGet - get elapsed ticks
*
* If tickless idle support is configured this routine returns the number
* of ticks since going idle and then resets the global elapsed tick counter back
* to zero indicating all elapsed ticks have been consumed. This is done with
* interrupts locked to prevent the timer ISR from modifying the global elapsed
* tick counter.
* If tickless idle support is not configured in it simply returns 1.
*
* RETURNS: number of ticks to process
*
* \NOMANUAL
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

/*******************************************************************************
*
* K_ticker - microkernel tick handler
*
* This routine informs other microkernel subsystems that a tick event has
* occurred.
*
* RETURNS: 1
*/

int K_ticker(int event)
{
	(void)event; /* prevent "unused argument" compiler warning */
	int32_t ticks;

	ticks = _SysIdleElapsedTicksGet();

	_WlMonitorUpdate();

	if (_TlDebugUpdate(ticks)) {
		_TimeSliceUpdate();
		_HandleExpiredTimers(ticks);
		_LowTimeInc(ticks);
	}

	return 1;
}

#ifdef CONFIG_TIMESLICING

/*******************************************************************************
*
* scheduler_time_slice_set - set time slicing period and scope
*
* This routine controls how task time slicing is performed by the task
* scheduler, by specifying the maximum time slice length (in ticks) and
* the highest priority task level for which time slicing is performed.
*
* To enable time slicing, a non-zero time slice length must be specified.
* The task scheduler then ensures that no executing task runs for more than
* the specified number of ticks before giving other tasks of that priority
* a chance to execute. (However, any task whose priority is higher than the
* specified task priority level is exempted, and may execute as long as
* desired without being pre-empted due to time slicing.)
*
* Time slicing only limits that maximum amount of time a task may continuously
* execute. Once the scheduler selects a task for execution, there is no minimum
* guaranteed time the task will execute before tasks of greater or equal
* priority are scheduled.
*
* If the currently executing task is the only one of that priority eligible
* for execution this routine has no effect, as that task will be immediately
* rescheduled once the slice period expires.
*
* To disable timeslicing, call the API with both parameters set to zero.
*
* RETURNS: N/A
*/

void scheduler_time_slice_set(int32_t t, /* time slice in ticks */
				kpriority_t p   /* beginning priority level to which
					    time slicing applies */
				)
{
	slice_time = t;
	slice_prio = p;
}

#endif /* CONFIG_TIMESLICING */
