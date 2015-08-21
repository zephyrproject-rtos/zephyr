/* microkernel/ticks.h - microkernel tick header file */

/*
 * Copyright (c) 1997-2015 Wind River Systems, Inc.
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

#ifndef TICKS_H
#define TICKS_H

#include <nanokernel.h>
#include <sys_clock.h>

/* externs */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Set time slicing period and scope
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
 * @return N/A
 */
extern void sys_scheduler_time_slice_set(int32_t t, kpriority_t p);

/**
 * @brief Read the processor's high precision timer
 *
 * This routine reads the processor's high precision timer.  It reads the
 * counter register on the timer device. This counter register increments
 * at a relatively high rate (e.g. 20 MHz), and thus is considered a
 * "high resolution" timer.  This is in contrast to nano_tick_get_32() and
 * task_tick_get_32() which return the value of the kernel ticks variable.
 *
 * @return current high precision clock value
 */
extern uint32_t task_cycle_get_32(void);

/**
 * @brief Read the current system clock value
 *
 * This routine returns the lower 32-bits of the current system clock value
 * as measured in ticks.
 *
 * @return lower 32-bit of the current system clock value
 */
extern int32_t task_tick_get_32(void);

/**
 *
 * @brief Read the current system clock value
 *
 * This routine returns the current system clock value as measured in ticks.
 *
 * Interrupts are locked while updating clock since some CPUs do not support
 * native atomic operations on 64 bit values.
 *
 * @return current system clock value
 */
extern int64_t task_tick_get(void);
extern ktimer_t task_timer_alloc(void);
extern void task_timer_free(ktimer_t timer);
extern void task_timer_start(ktimer_t timer,
			     int32_t duration,
			     int32_t period,
			     ksem_t sema);
/**
 *
 * @brief Restart a timer
 *
 * This routine restarts the timer specified by <timer>. The timer must have
 * already been started by a call to task_timer_start().
 *
 * @param timer      Timer to restart.
 * @param duration   Initial delay.
 * @param period     Repetition interval.
 *
 * @return N/A
 */

static inline void task_timer_restart(ktimer_t timer, int32_t duration,
										int32_t period)
{
	task_timer_start(timer, duration, period, _USE_CURRENT_SEM);
}

extern void task_timer_stop(ktimer_t timer);

/**
 * @brief Return ticks between calls
 *
 * This function is meant to be used in contained fragments of code. The first
 * call to it in a particular code fragment fills in a reference time variable
 * which then gets passed and updated every time the function is called. From
 * the second call on, the delta between the value passed to it and the current
 * tick count is the return value. Since the first call is meant to only fill in
 * the reference time, its return value should be discarded.
 *
 * Since a code fragment that wants to use task_tick_delta() passes in its
 * own reference time variable, multiple code fragments can make use of this
 * function concurrently.
 *
 * Note that it is not necessary to allocate a timer to use this call.
 *
 * @return elapsed time in system ticks
 */
extern int64_t task_tick_delta(int64_t *reftime);

static inline int32_t task_tick_delta_32(int64_t *reftime)
{
	return (int32_t)task_tick_delta(reftime);
}

extern void task_sleep(int32_t ticks);
extern int task_workload_get(void);
extern void sys_workload_time_slice_set(int32_t t);

#define isr_cycle_get_32() task_cycle_get_32()
#define isr_tick_get_32() task_tick_get_32()
#define isr_tick_get() task_tick_get()

#define fiber_cycle_get_32() task_cycle_get_32()
#define fiber_tick_get_32() task_tick_get_32()
#define fiber_tick_get() task_tick_get()

#ifdef __cplusplus
}
#endif

#endif /* TICKS_H */
