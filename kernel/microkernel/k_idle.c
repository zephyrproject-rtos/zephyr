/* k_idle.c - microkernel idle logic */

/*
 * Copyright (c) 1997-2010, 2012-2014 Wind River Systems, Inc.
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
Microkernel idle logic. Different forms of idling are performed by the idle
task, depending on how the kernel is configured.
*/

#include <micro_private.h>
#include <nano_private.h>
#include <arch/cpu.h>
#include <toolchain.h>
#include <sections.h>
#include <microkernel.h>
#include <misc/util.h>
#include <drivers/system_timer.h>

#if defined(CONFIG_WORKLOAD_MONITOR)

static unsigned int _k_workload_slice = 0x0;
static unsigned int _k_workload_ticks = 0x0;
static unsigned int _k_workload_ref_time = 0x0;
static unsigned int _k_workload_t0 = 0x0;
static unsigned int _k_workload_t1 = 0x0;
static volatile unsigned int _k_workload_n0 = 0x0;
static volatile unsigned int _k_workload_n1 = 0x0;
static volatile unsigned int _k_workload_i = 0x0;
static volatile unsigned int _k_workload_i0 = 0x0;
static volatile unsigned int _k_workload_delta = 0x0;
static volatile unsigned int _k_workload_start_time = 0x0;
static volatile unsigned int _k_workload_end_time = 0x0;

#ifdef WL_SCALE
static extern uint32_t _k_workload_scale;
#endif

#define MSEC_PER_SEC 1000

/*******************************************************************************
*
* workload_loop - shared code between workload calibration and monitoring
*
* Perform idle task "dummy work".
*
* This routine increments _k_workload_i and checks it against _k_workload_n1.
* _k_workload_n1 is updated by the system tick handler, and both are kept
* in close synchronization.
*
* RETURNS: N/A
*
*/

static void workload_loop(void)
{
	volatile int x = 87654321;
	volatile int y = 4;

	/* loop never terminates, except during calibration phase */

	while (++_k_workload_i != _k_workload_n1) {
		unsigned int s_iCountDummyProc = 0;
		while (64 != s_iCountDummyProc++) { /* 64 == 2^6 */
			x >>= y;
			x <<= y;
			y++;
			x >>= y;
			x <<= y;
			y--;
		}
	}
}

/*******************************************************************************
*
* _k_workload_monitor_calibrate - calibrate the workload monitoring subsystem
*
* Measures the time required to do a fixed amount of "dummy work", and
* sets default values for the workload measuring period.
*
* RETURNS: N/A
*
*/

void _k_workload_monitor_calibrate(void)
{
	_k_workload_n0 = _k_workload_i = 0;
	_k_workload_n1 = 1000;

	_k_workload_t0 = timer_read();
	workload_loop();
	_k_workload_t1 = timer_read();

	_k_workload_delta = _k_workload_t1 - _k_workload_t0;
	_k_workload_i0 = _k_workload_i;
#ifdef WL_SCALE
	_k_workload_ref_time =
		(_k_workload_t1 - _k_workload_t0) >> (_k_workload_scale);
#else
	_k_workload_ref_time = (_k_workload_t1 - _k_workload_t0) >> (4 + 6);
#endif

	_k_workload_slice = 100;
	_k_workload_ticks = 100;
}

/*******************************************************************************
*
* _k_workload_monitor_update - workload monitor tick handler
*
* If workload monitor is configured this routine updates the global variables
* it uses to record the passage of time.
*
* RETURNS: N/A
*
* \NOMANUAL
*/

void _k_workload_monitor_update(void)
{
	if (--_k_workload_ticks == 0) {
		_k_workload_t0 = _k_workload_t1;
		_k_workload_t1 = timer_read();
		_k_workload_n0 = _k_workload_n1;
		_k_workload_n1 = _k_workload_i - 1;
		_k_workload_ticks = _k_workload_slice;
	}
}

/*******************************************************************************
*
* _k_workload_monitor_idle_start - workload monitor "start idling" handler
*
* Records time when idle task was selected for execution by the microkernel.
*
* RETURNS: N/A
*/

void _k_workload_monitor_idle_start(void)
{
	_k_workload_start_time = timer_read();
}

/*******************************************************************************
*
* _k_workload_monitor_idle_end - workload monitor "end idling" handler
*
* Records time when idle task was no longer selected for execution by the
* microkernel, and updates amount of time spent idling.
*
* RETURNS: N/A
*/

void _k_workload_monitor_idle_end(void)
{
	_k_workload_end_time = timer_read();
	_k_workload_i += (_k_workload_i0 *
		(_k_workload_end_time - _k_workload_start_time)) / _k_workload_delta;
}

/*******************************************************************************
*
* _k_workload_get - process request to read the processor workload
*
* Computes workload, or uses 0 if workload monitoring is not configured.
*
* RETURNS: N/A
*/

void _k_workload_get(struct k_args *P)
{
	unsigned int k, t;
	signed int iret;

	k = (_k_workload_i - _k_workload_n0) * _k_workload_ref_time;
#ifdef WL_SCALE
	t = (timer_read() - _k_workload_t0) >> (_k_workload_scale);
#else
	t = (timer_read() - _k_workload_t0) >> (4 + 6);
#endif

	iret = MSEC_PER_SEC - k / t;

	/*
	 * Due to calibration at startup, <iret> could be slightly negative.
	 * Ensure a negative value is never returned.
	 */

	if (iret < 0) {
		iret = 0;
	}

	P->Args.u1.rval = iret;
}
#else
void _k_workload_get(struct k_args *P)
{
	P->Args.u1.rval = 0;
}

#endif /* CONFIG_WORKLOAD_MONITOR */

/*******************************************************************************
*
* task_workload_get - read the processor workload
*
* This routine returns the workload as a number ranging from 0 to 1000.
*
* Each unit equals 0.1% of the time the idle task was not scheduled by the
* microkernel during the period set by sys_workload_time_slice_set().
*
* IMPORTANT: This workload monitor ignores any time spent servicing ISRs and
* fibers! Thus, a system which has no meaningful task work to do may spend
* up to 100% of its time servicing ISRs and fibers, yet report a workload of 0%
* because the idle task is always the task selected by the microkernel.
*
* RETURNS: workload
*/

int task_workload_get(void)
{
	struct k_args A;

	A.Comm = READWL;
	KERNEL_ENTRY(&A);
	return A.Args.u1.rval;
}

/*******************************************************************************
*
* sys_workload_time_slice_set - set workload period
*
* This routine specifies the workload measuring period for task_workload_get().
*
* RETURNS: N/A
*/

void sys_workload_time_slice_set(int32_t t)
{
#ifdef CONFIG_WORKLOAD_MONITOR
	if (t < 10) {
		t = 10;
	}
	if (t > 1000) {
		t = 1000;
	}
	_k_workload_slice = t;
#else
	ARG_UNUSED(t);
#endif
}

unsigned char _sys_power_save_flag = 1;

#if defined(CONFIG_ADVANCED_POWER_MANAGEMENT)

#include <nanokernel.h>
#include <microkernel/base_api.h>
#ifdef CONFIG_ADVANCED_IDLE
#include <advidle.h>
#endif
#if defined(CONFIG_TICKLESS_IDLE)
#include <drivers/system_timer.h>
#endif

extern void nano_cpu_idle(void);
extern void nano_cpu_set_idle(int32_t ticks);

#if defined(CONFIG_TICKLESS_IDLE)
/*
 * Idle time must be this value or higher for timer to go into tickless idle
 * state.
 */
int32_t _sys_idle_threshold_ticks = CONFIG_TICKLESS_IDLE_THRESH;
#endif /* CONFIG_TICKLESS_IDLE */

/*******************************************************************************
*
* _sys_power_save_idle - power management policy when kernel begins idling
*
* This routine implements the power management policy based on the time
* until the timer expires, in system ticks.
* Routine is invoked from the idle task with interrupts disabled
*
* RETURNS: N/A
*
* \NOMANUAL
*/

void _sys_power_save_idle(int32_t ticks)
{
#if defined(CONFIG_TICKLESS_IDLE)
	if ((ticks == TICKS_UNLIMITED) || ticks >= _sys_idle_threshold_ticks) {
		/*
		 * Stop generating system timer interrupts until it's time for
		 * the next scheduled microkernel timer to expire.
		 */

		_timer_idle_enter(ticks);
	}
#endif /* CONFIG_TICKLESS_IDLE */

#ifdef CONFIG_ADVANCED_IDLE
	/*
	 * Call the advanced sleep function, which checks if the system should
	 * enter a deep sleep state. If so, the function will return a non-zero
	 * value when the system resumes here after the deep sleep ends.
	 * If the time to sleep is too short to go to advanced sleep mode, the
	 * function returns zero immediately and we do normal idle processing.
	 */

	if (_AdvIdleFunc(ticks) == 0) {
		nano_cpu_set_idle(ticks);
		nano_cpu_idle();
	}
#else
	nano_cpu_set_idle(ticks);
	nano_cpu_idle();
#endif /* CONFIG_ADVANCED_IDLE */
}

/*******************************************************************************
*
* _sys_power_save_idle_exit - power management policy when kernel stops idling
*
* This routine is invoked when the kernel leaves the idle state.
* Routine can be modified to wake up other devices.
* The routine is invoked from interrupt context, with interrupts disabled.
*
* RETURNS: N/A
*
* \NOMANUAL
*/

void _sys_power_save_idle_exit(int32_t ticks)
{
#ifdef CONFIG_TICKLESS_IDLE
	if ((ticks == TICKS_UNLIMITED) || ticks >= _sys_idle_threshold_ticks) {
		/* Resume normal periodic system timer interrupts */

		_timer_idle_exit();
	}
#else
	ARG_UNUSED(ticks);
#endif /* CONFIG_TICKLESS_IDLE */
}

/*******************************************************************************
*
* _get_next_timer_expiry - obtain number of ticks until next timer expires
*
* Must be called with interrupts locked to prevent the timer queues from
* changing.
*
* RETURNS: Number of ticks until next timer expires.
*
*/

static inline int32_t _get_next_timer_expiry(void)
{
	uint32_t closest_deadline = (uint32_t)TICKS_UNLIMITED;

	if (_k_timer_list_head) {
		closest_deadline = _k_timer_list_head->duration;
	}

	return (int32_t)min(closest_deadline, _nano_get_earliest_deadline());
}
#endif

/*******************************************************************************
*
* _power_save - power saving when idle
*
* If the BSP sets the _sys_power_save_flag flag, this routine will call the
* _sys_power_save_idle() routine in an infinite loop. If the flag is not set,
* this routine will fall through and _k_kernel_idle() will try the next idling
* mechanism.
*
* RETURNS: N/A
*
*/

static void _power_save(void)
{
	extern void nano_cpu_idle(void);

	if (_sys_power_save_flag) {
		for (;;) {
			irq_lock_inline();
#ifdef CONFIG_ADVANCED_POWER_MANAGEMENT
			_sys_power_save_idle(_get_next_timer_expiry());
#else
			/*
			 * nano_cpu_idle () is invoked here directly only if APM is
			 * disabled. Otherwise BSP decides either to invoke it or
			 * to implement advanced idle functionality
			 */

			nano_cpu_idle();
#endif
		}

		/*
		 * Code analyzers may complain that _power_save() uses an
		 * infinite loop unless we indicate that this is intentional
		 */

		CODE_UNREACHABLE;
	}
}

/* Specify what work to do when idle task is "busy waiting" */

#ifdef CONFIG_WORKLOAD_MONITOR
#define DO_IDLE_WORK()	workload_loop()
#else
#define DO_IDLE_WORK()	do { /* do nothing */ } while (0)
#endif

/*******************************************************************************
*
* _k_kernel_idle - microkernel idle task
*
* If power save is on, we sleep; if power save is off, we "busy wait".
*
* RETURNS: N/A
*
*/

int _k_kernel_idle(void)
{
	_power_save(); /* never returns if power saving is enabled */

#ifdef CONFIG_BOOT_TIME_MEASUREMENT
	/* record timestamp when idling begins */

	extern uint64_t __idle_tsc;
	__idle_tsc = _NanoTscRead();
#endif

	for (;;) {
		DO_IDLE_WORK();
	}

	/*
	 * Code analyzers may complain that _k_kernel_idle() uses an infinite loop
	 * unless we indicate that this is intentional
	 */

	CODE_UNREACHABLE;
}
