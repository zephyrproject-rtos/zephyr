/* k_ticker.c - microkernel tick event handler */

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
#include <arch/cpu.h>

#include <micro_private.h>
#include <drivers/system_timer.h>
#include <microkernel.h>
#include <microkernel/ticks.h>
#include <toolchain.h>
#include <sections.h>

int64_t _k_sys_clock_tick_count = 0;

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

#ifdef CONFIG_SYS_CLOCK_EXISTS
int sys_clock_us_per_tick = 1000000 / sys_clock_ticks_per_sec;
int sys_clock_hw_cycles_per_tick =
	sys_clock_hw_cycles_per_sec / sys_clock_ticks_per_sec;
#else
/* don't initialize to avoid division-by-zero error */
int sys_clock_us_per_tick;
int sys_clock_hw_cycles_per_tick;
#endif

uint32_t task_cycle_get_32(void)
{
	return _sys_clock_cycle_get();
}

int32_t task_tick_get_32(void)
{
	return (int32_t)_k_sys_clock_tick_count;
}

int64_t task_tick_get(void)
{
	int64_t ticks;
	int key = irq_lock();

	ticks = _k_sys_clock_tick_count;
	irq_unlock(key);
	return ticks;
}

/**
 *
 * @brief Increment system clock by "N" ticks
 *
 * Interrupts are locked while updating clock since some CPUs do not support
 * native atomic operations on 64 bit values.
 *
 * @return N/A
 */

static void sys_clock_increment(int inc)
{
	int key = irq_lock();

	_k_sys_clock_tick_count += inc;
	irq_unlock(key);
}

/**
 *
 * @brief Task level debugging tick handler
 *
 * If task level debugging is configured this routine updates the low resolution
 * debugging timer and determines if task level processing should be suspended.
 *
 * @return 0 if task level processing should be halted or 1 if not
 *
 * \NOMANUAL
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
 *
 * @brief Tick handler time slice logic
 *
 * This routine checks to see if it is time for the current task
 * to relinquish control, and yields CPU if so.
 *
 * @return N/A
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
		_k_task_yield(NULL);
	}
#else
/* do nothing */
#endif /* CONFIG_TIMESLICING */
}

/**
 *
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

/**
 *
 * @brief Microkernel tick handler
 *
 * This routine informs other microkernel subsystems that a tick event has
 * occurred.
 *
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
		sys_clock_increment(ticks);
		_nano_sys_clock_tick_announce((uint32_t)ticks);
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

	P->Args.c1.time2 = now - P->Args.c1.time1;
	P->Args.c1.time1 = now;
}

int64_t task_tick_delta(int64_t *reftime /* pointer to reference time */
			)
{
	struct k_args A;

	A.Comm = _K_SVC_TIME_ELAPSE;
	A.Args.c1.time1 = *reftime;
	KERNEL_ENTRY(&A);
	*reftime = A.Args.c1.time1;
	return A.Args.c1.time2;
}
