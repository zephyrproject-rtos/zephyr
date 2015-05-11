/* boot_time.c - Boot Time measurement task */

/*
 * Copyright (c) 2013-2015 Wind River Systems, Inc.
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
Measure boot time for both nanokernel and microkernel project which includes
- from reset to kernel's _start
- from _start to main()
- from _start to task
- from _start to idle (for microkernel)
*/

/* includes */

#ifdef CONFIG_NANOKERNEL
#include <nanokernel.h>
#else
#include <vxmicro.h>
#include <version.h>
#endif
#include <nanokernel/cpu.h>
#include <tc_util.h>

/* externs */
extern uint64_t __start_tsc; /* timestamp when VxMicro begins executing */
extern uint64_t __main_tsc;  /* timestamp when main() begins executing */
extern uint64_t __idle_tsc;  /* timestamp when CPU went idle */

void bootTimeTask (void)
	{
	uint64_t task_tsc;  /* timestamp at beginning of first task  */
	uint64_t _start_us; /* being of __start timestamp in us	 */
	uint64_t main_us;   /* begin of main timestamp in us	 */
	uint64_t task_us;   /* begin of task timestamp in us	 */
	uint64_t s_main_tsc; /* __start->main timestamp		 */
	uint64_t s_task_tsc;  /*__start->task timestamp		 */
#ifndef  CONFIG_NANOKERNEL
	uint64_t idle_us;	/* begin of idle timestamp in us	 */
	uint64_t s_idle_tsc;  /*__start->idle timestamp		 */
#endif /* ! CONFIG_NANOKERNEL */

	task_tsc = _NanoTscRead();
#ifndef  CONFIG_NANOKERNEL
    /* Go to sleep for 1 tick in order to timestamp when IdleTask halts. */
	task_sleep(1);
#endif /* ! CONFIG_NANOKERNEL */

	_start_us = __start_tsc / CONFIG_CPU_CLOCK_FREQ_MHZ;
	s_main_tsc = __main_tsc-__start_tsc;
	main_us   = s_main_tsc / CONFIG_CPU_CLOCK_FREQ_MHZ;
	s_task_tsc = task_tsc-__start_tsc;
	task_us   = s_task_tsc / CONFIG_CPU_CLOCK_FREQ_MHZ;
#ifndef  CONFIG_NANOKERNEL
	s_idle_tsc = __idle_tsc-__start_tsc;
	idle_us   =  s_idle_tsc / CONFIG_CPU_CLOCK_FREQ_MHZ;
#endif

    /* Indicate start for sanity test suite */
	TC_START ("Boot Time Measurement");

    /* Only print lower 32bit of time result */
#ifdef CONFIG_NANOKERNEL
	TC_PRINT("NanoKernel Boot Result: Clock Frequency: %d MHz\n",
		     CONFIG_CPU_CLOCK_FREQ_MHZ);
#else	/* CONFIG_MICROKERNEL */
	TC_PRINT("MicroKernel Boot Result: Clock Frequency: %d MHz\n",
		     CONFIG_CPU_CLOCK_FREQ_MHZ);
#endif
	TC_PRINT("__start       : %d cycles, %d us\n",
	(uint32_t)(__start_tsc & 0xFFFFFFFFULL),
	(uint32_t) (_start_us  & 0xFFFFFFFFULL));
	TC_PRINT("_start->main(): %d cycles, %d us\n",
	(uint32_t)(s_main_tsc & 0xFFFFFFFFULL),
	(uint32_t)  (main_us  & 0xFFFFFFFFULL));
	TC_PRINT("_start->task  : %d cycles, %d us\n",
	(uint32_t)(s_task_tsc & 0xFFFFFFFFULL),
	(uint32_t)  (task_us  & 0xFFFFFFFFULL));
#ifndef  CONFIG_NANOKERNEL  /* CONFIG_MICROKERNEL */
	TC_PRINT("_start->idle  : %d cycles, %d us\n",
	(uint32_t)(s_idle_tsc & 0xFFFFFFFFULL),
	(uint32_t)  (idle_us  & 0xFFFFFFFFULL));

#endif

	TC_PRINT("Boot Time Measurement finished\n");

	// for sanity regression test utility.
	TC_END_RESULT(TC_PASS);
	TC_END_REPORT(TC_PASS);

	}

#ifdef CONFIG_NANOKERNEL

char fiberStack[512];

/*******************************************************************************
*
* main - nanokernel entry point
*
* RETURNS: N/A
*/

void main (void)
	{
    /* record timestamp for nanokernel's main() function */
	__main_tsc = _NanoTscRead();

    /* create bootTime fibers */
	task_fiber_start (fiberStack, 512,
	    (nano_fiber_entry_t) bootTimeTask, 0, 0, 6, 0);
	}

#endif /*  CONFIG_NANOKERNEL */
