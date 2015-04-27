/* idle.c - microkernel idle logic */

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

#include <microkernel/k_struct.h>
#include <minik.h>
#include <kticks.h>
#include <nanok.h>
#include <nanokernel/cpu.h>
#include <toolchain.h>
#include <sections.h>
#include <microkernel.h>

#if defined(CONFIG_WORKLOAD_MONITOR)

unsigned int _k_workload_slice = 0x0;
unsigned int _k_workload_ticks = 0x0;
unsigned int _k_workload_ref_time = 0x0;
unsigned int WldT0 = 0x0;
unsigned int WldT1 = 0x0;
volatile unsigned int WldN0 = 0x0;
volatile unsigned int WldN1 = 0x0;
volatile unsigned int Wld_i = 0x0;
volatile unsigned int Wld_i0 = 0x0;
volatile unsigned int WldTDelta = 0x0;
volatile unsigned int WldT_start = 0x0;
volatile unsigned int WldT_end = 0x0;

#ifdef WL_SCALE
extern uint32_t K_wl_scale;
#endif

#define MSEC_PER_SEC 1000

/*******************************************************************************
*
* _WlLoop - shared code between workload calibration and monitoring
*
* Perform idle task "dummy work".
*
* This routine increments Wld_i and checks it against WldN1. WldN1 is updated
* by the system tick handler, and both are kept in close synchronization.
*
* RETURNS: N/A
*
*/

static void _WlLoop(void)
{
	volatile int x = 87654321;
	volatile int y = 4;

	while (++Wld_i != WldN1) /* except for the calibration phase,
				  * this while loop should always be true.
				  */
	{
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
* wlMonitorCalibrate - calibrate the workload monitoring subsystem
*
* Measures the time required to do a fixed amount of "dummy work", and
* sets default values for the workload measuring period.
*
* RETURNS: N/A
*
*/

void wlMonitorCalibrate(void)
{
	WldN0 = Wld_i = 0;
	WldN1 = 1000;

	WldT0 = timer_read();
	_WlLoop();
	WldT1 = timer_read();

	WldTDelta = WldT1 - WldT0;
	Wld_i0 = Wld_i;
#ifdef WL_SCALE
	_k_workload_ref_time = (WldT1 - WldT0) >> (K_wl_scale);
#else
	_k_workload_ref_time = (WldT1 - WldT0) >> (4 + 6);
#endif

	_k_workload_slice = 100;
	_k_workload_ticks = 100;
}

#endif /* CONFIG_WORKLOAD_MONITOR */


#ifdef CONFIG_WORKLOAD_MONITOR
void K_workload(struct k_args *P)
{
	unsigned int k, t;
	signed int iret;

	k = (Wld_i - WldN0) * _k_workload_ref_time;
#ifdef WL_SCALE
	t = (timer_read() - WldT0) >> (K_wl_scale);
#else
	t = (timer_read() - WldT0) >> (4 + 6);
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
void K_workload(struct k_args *P)
{
	P->Args.u1.rval = 0;
}
#endif

/*******************************************************************************
*
* task_node_workload_get - read the processor workload
*
* This routine returns the workload as a number ranging from 0 to 1000.
*
* Each unit equals 0.1% of the time the CPU was not idle during the period
* set by workload_time_slice_set().
*
* RETURNS: workload
*/

int task_node_workload_get(void)
{
	struct k_args A;

	A.Comm = READWL;
	KERNEL_ENTRY(&A);
	return A.Args.u1.rval;
}

/*******************************************************************************
*
* workload_time_slice_set - set workload period
*
* This routine specifies the workload measuring period for task_node_workload_get().
*
* RETURNS: N/A
*/

void workload_time_slice_set(int32_t t)
{
#ifdef CONFIG_WORKLOAD_MONITOR
	if (t < 10)
		t = 10;
	if (t > 1000)
		t = 1000;
	_k_workload_slice = t;
#else
	ARG_UNUSED(t);
#endif
}

#if defined(CONFIG_ADVANCED_POWER_MANAGEMENT)
/*******************************************************************************
*
* _GetNextTimerExpiry - obtain number of ticks until next timer expires
*
* Must be called with interrupts locked to prevent the timer queues from
* changing.
*
* RETURNS: Number of ticks until next timer expires.
*
*/

static inline int32_t _GetNextTimerExpiry(void)
{
	if (_k_timer_list_head)
		return _k_timer_list_head->Ti;

	return TICKS_UNLIMITED;
}
#endif

/*******************************************************************************
*
* _PowerSave - power saving when idle
*
* If the BSP sets the _SysPowerSaveFlag flag, this routine will call the
* _SysPowerSaveIdle() routine in an infinite loop. If the flag is not set,
* this routine will fall through and kernel_idle() will try the next idling
* mechanism.
*
* RETURNS: N/A
*
*/

static void _PowerSave(void)
{
	extern void nano_cpu_idle(void);
	extern unsigned char _SysPowerSaveFlag;
	extern void _SysPowerSaveIdle(int32_t ticks);

	if (_SysPowerSaveFlag) {
		for (;;) {
			irq_lock_inline();
#ifdef CONFIG_ADVANCED_POWER_MANAGEMENT
			_SysPowerSaveIdle(_GetNextTimerExpiry());
#else
			/*
			 * nano_cpu_idle () is invoked here directly only if APM
			 * is
			 * disabled. Otherwise BSP decides either to invoke it
			 * or
			 * to implement advanced idle functionality
			 */
			nano_cpu_idle();
#endif
		}

		/*
		 * Code analyzers may complain that _PowerSave() uses an
		 * infinite loop
		 * unless we indicate that this is intentional
		 */

		CODE_UNREACHABLE;
	}
}

/*******************************************************************************
*
* kernel_idle - microkernel idle task
*
* If power save is on, we sleep. If power save is off, we will try to do
* workload monitoring.  If power save is off and workload monitoring
* is not included, we have to busy wait.
*
* RETURNS: N/A
*
*/

int kernel_idle(void)
{
	_PowerSave(); /* never returns if power saving is enabled */

#ifdef CONFIG_BOOT_TIME_MEASUREMENT
	/* Power saving not enabled, so record timestamp when idle begins here
	 */
	extern uint64_t __idle_tsc;
	__idle_tsc = _NanoTscRead();
#endif

#ifdef CONFIG_WORKLOAD_MONITOR
	_WlLoop();
#endif

	for (;;) {
		/* do nothing */
	}

	/*
	 * Code analyzers may complain that kernel_idle() uses an infinite loop
	 * unless we indicate that this is intentional
	 */

	CODE_UNREACHABLE;
}
