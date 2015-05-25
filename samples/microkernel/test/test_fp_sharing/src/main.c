/* main.c - load/store portion of FPU sharing test */

/*
 * Copyright (c) 2011-2014 Wind River Systems, Inc.
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
This module implements the load/store portion of the FPU sharing test. This
microkernel version of this test utilizes a pair of tasks, while the nanokernel
verions utilizes a task and a fiber.

The load/store test validates the nanokernel's floating point unit context
save/restore mechanism. (For the IA-32 architecture this includes the x87 FPU
(MMX) registers and the XMM registers.) This test utilizes a pair of contexts
of different priorities that each use the floating point registers. The context
switching that occurs exercises the kernel's ability to properly preserve the
floating point registers. The test also exercises the kernel's ability to
automatically enable floating point support for a task, if supported.

FUTURE IMPROVEMENTS
On architectures where the non-integer capabilities are provided in a hierarchy,
for example on IA-32 the USE_FP and USE_SSE options are provided, this test
should be enhanced to ensure that the architectures' _Swap() routine doesn't
context switch more registers that it needs to (which would represent a
performance issue).  For example, on the IA-32, the test should issue
a nanoCpuFpDisable() from main(), and then indicate that only x87 FPU
registers will be utilized (nanoCpuFpEnable).  The fiber context should continue
to load ALL non-integer registers, but main() should validate that only the
x87 FPU registers are being saved/restored.
*/

#if defined(CONFIG_ISA_IA32)
#ifndef CONFIG_FLOAT
#error Rebuild the nanokernel with the FLOAT config option enabled
#endif

#ifndef CONFIG_SSE
#error Rebuild the nanokernel with the SSE config option enabled
#endif

#ifndef CONFIG_FP_SHARING
#error Rebuild the nanokernel with the FP_SHARING config option enabled
#endif

#ifndef CONFIG_AUTOMATIC_FP_ENABLING
#error Rebuild the nanokernel with AUTOMATIC_FP_ENABLING config option enabled
#endif
#endif /* CONFIG_ISA_IA32 */


#ifdef CONFIG_MICROKERNEL
#include <microkernel.h>
#else
#include <nanokernel.h>
#endif

#if defined(__GNUC__)
#include <float_regs_x86_gcc.h>
#else
#include <float_regs_x86_other.h>
#endif /* __GNUC__ */

#include <arch/cpu.h>
#include <tc_util.h>
#include "float_context.h"
#include <stddef.h>
#include <string.h>

#ifndef MAX_TESTS
/* test duration, unless overridden by project builder (0 => run forever) */
#define MAX_TESTS 1000
#endif

/* macro used to read system clock value */

#ifdef CONFIG_NANOKERNEL
#define TICK_COUNT_GET() nano_tick_get_32()
#else
#define TICK_COUNT_GET() task_tick_get_32()
#endif

/* space for float register load/store area used by low priority task context */

static FP_REG_SET floatRegSetLoad;
static FP_REG_SET floatRegSetStore;

/* space for float register load/store area used by high priority context */

static FP_REG_SET floatRegisterSet;


#ifdef CONFIG_NANOKERNEL
/* stack for high priority fiber context (also use .bss for floatRegisterSet) */

static char fiberStack[1024];

static struct nano_timer fiberTimer;
static void *dummyTimerData;	/* allocate just enough room for a pointer */
#endif

/* flag indicating that an error has occurred */

int fpu_sharing_error;

/*
 * Test counters are "volatile" because GCC may not update them properly
 * otherwise. (See description of pi calculation test for more details.)
 */

static volatile unsigned int load_store_low_count = 0;
static volatile unsigned int load_store_high_count = 0;

/*******************************************************************************
*
* main -
* load_store_low - low priority FPU load/store context
*
* RETURNS: N/A
*/

#ifdef CONFIG_NANOKERNEL
void main(void)
#else
void load_store_low(void)
#endif
{
	unsigned int bufIx;
	unsigned char floatRegInitByte;
	unsigned char *floatRegSetStorePtr = (unsigned char *)&floatRegSetStore;

	volatile char volatileStackVar;

	PRINT_DATA("Floating point sharing tests started\n");
	PRINT_LINE;

#if defined(CONFIG_AUTOMATIC_FP_ENABLING)
	/*
	 * No need to invoke task_float_enable() since
	 * AUTOMATIC_FP_ENABLING is in effect
	 */
#else /* ! CONFIG_AUTOMATIC_FP_ENABLING */
#if defined(CONFIG_FLOAT)
	task_float_enable(context_self_get());
#endif
#endif /* CONFIG_AUTOMATIC_FP_ENABLING */

#ifdef CONFIG_NANOKERNEL
	/*
	 * Start a single fiber which will regularly preempt the background
	 * task, and perform similiar floating point register manipulations
	 * that the background task performs; except that a different constant
	 * is loaded into the floating point registers.
	 */

	extern void load_store_high(int regFillValue, int unused);

	task_fiber_start(fiberStack,
			 sizeof(fiberStack),
			 load_store_high,
			 0,	/* arg1 */
			 0,	/* arg2 */
			 5,	/* priority */
			 FP_OPTION /* options */
			 );
#elif defined(CONFIG_MICROKERNEL)
	/*
	 * For microkernel builds, preemption tasks are specified in the .vpf file.
	 *
	 * Enable round robin scheduling to allow both the low priority pi
	 * computation and load/store tasks to execute. The high priority pi
	 * computation and load/store tasks will preempt the low priority tasks
	 * periodically.
	 */

	scheduler_time_slice_set(1, 10);
#endif /*  CONFIG_NANOKERNEL */

	/*
	 * Initialize floating point load buffer to known values;
	 * these values must be different than the value used in other contexts.
	 */

	floatRegInitByte = MAIN_FLOAT_REG_CHECK_BYTE;
	for (bufIx = 0; bufIx < SIZEOF_FP_REG_SET; ++bufIx) {
		((unsigned char *)&floatRegSetLoad)[bufIx] = floatRegInitByte++;
	}

	/* Keep cranking forever, or until an error is detected. */

	for (load_store_low_count = 0; ; load_store_low_count++) {

		/*
		 * Clear store buffer to erase all traces of any previous
		 * floating point values that have been saved.
		 */

		memset(&floatRegSetStore, 0, SIZEOF_FP_REG_SET);

		/*
		 * Utilize an architecture specific function to load all the floating
		 * point (and XMM on IA-32) registers with known values.
		 */

		_LoadAllFloatRegisters(&floatRegSetLoad);

		/*
		 * Waste some cycles to give the high priority load/store context
		 * an opportunity to run when the low priority context is using the
		 * floating point registers.
		 *
		 * IMPORTANT: This logic requires that TICK_COUNT_GET() not perform
		 * any floating point operations!
		 */

		while ((TICK_COUNT_GET() % 5) != 0) {
			/*
			 * Use a volatile variable to prevent compiler optimizing
			 * out the spin loop.
			 */
			++volatileStackVar;
		}

		/*
		 * Utilize an architecture specific function to dump the contents
		 * of all floating point (and XMM on IA-32) register to memory.
		 */

		_StoreAllFloatRegisters(&floatRegSetStore);

		/*
		 * Compare each byte of buffer to ensure the expected value is
		 * present, indicating that the floating point registers weren't
		 * impacted by the operation of the high priority context(s).
		 *
		 * Display error message and terminate if discrepancies are detected.
		 */

		floatRegInitByte = MAIN_FLOAT_REG_CHECK_BYTE;

		for (bufIx = 0; bufIx < SIZEOF_FP_REG_SET; ++bufIx) {
			if (floatRegSetStorePtr[bufIx] != floatRegInitByte) {
				TC_ERROR("load_store_low found 0x%x instead of 0x%x"
						 " @ offset 0x%x\n",
						 floatRegSetStorePtr[bufIx], floatRegInitByte, bufIx);
				TC_ERROR("Discrepancy found during iteration %d\n",
						 load_store_low_count);
				fpu_sharing_error = 1;
			}
			floatRegInitByte++;
		}

		/*
		 * Terminate if a test error has been reported.
		 */

		if (fpu_sharing_error) {
			TC_END_RESULT(TC_FAIL);
			TC_END_REPORT(TC_FAIL);
			return;
		}

#if defined(CONFIG_AUTOMATIC_FP_ENABLING)
		/*
		 * After every 1000 iterations (arbitrarily chosen), explicitly
		 * disable floating point operations for the task. The subsequent
		 * execution of _LoadAllFloatRegisters() will result in an exception
		 * to automatically re-enable floating point support for the task.
		 *
		 * The purpose of this part of the test is to exercise the
		 * task_float_disable() API, and to also continue exercising the
		 * AUTOMATIC_FP_ENABLING (exception based) mechanism.
		 */
		if ((load_store_low_count % 1000) == 0) {
#if defined(CONFIG_FLOAT)
			task_float_disable(context_self_get());
#endif
		}
#endif /* CONFIG_AUTOMATIC_FP_ENABLING */
	}
}

/*******************************************************************************
*
* load_store_high - high priority FPU load/store context
*
* RETURNS: N/A
*/

#ifdef CONFIG_NANOKERNEL
void load_store_high(int unused1, int unused2)
#else
void load_store_high(void)
#endif
{
	unsigned int bufIx;
	unsigned char floatRegInitByte;
#if !defined(CONFIG_ISA_IA32)
	unsigned int numNonVolatileBytes;
#endif /* !CONFIG_ISA_IA32 */
	unsigned char *floatRegisterSetBytePtr =
		(unsigned char *)&floatRegisterSet;

#ifdef CONFIG_NANOKERNEL
	ARG_UNUSED(unused1);
	ARG_UNUSED(unused2);

	/* initialize timer; data field is not used */

	nano_timer_init(&fiberTimer, (void *)dummyTimerData);
#endif

	/* test until the specified time limit, or until an error is detected */

	while (1) {
		/*
		 * Initialize the floatRegisterSet structure by treating it as a simple
		 * array of bytes (the arrangement and actual number of registers is
		 * not important for this generic C code).  The structure is
		 * initialized by using the byte value specified by the constant
		 * FIBER_FLOAT_REG_CHECK_BYTE, and then incrementing the value for each
		 * successive location in the floatRegisterSet structure.
		 *
		 * The initial byte value, and thus the contents of the entire
		 * floatRegisterSet structure, must be different for each context to
		 * effectively test the nanokernel's ability to properly save/restore
		 * the floating point values during a context switch.
		 */

		floatRegInitByte = FIBER_FLOAT_REG_CHECK_BYTE;

		for (bufIx = 0; bufIx < SIZEOF_FP_REG_SET; ++bufIx) {
			floatRegisterSetBytePtr[bufIx] = floatRegInitByte++;
		}

		/*
		 * Utilize an architecture specific function to load all the floating
		 * point (and XMM on IA-32) registers with the contents of
		 * the floatRegisterSet structure.
		 *
		 * The goal of the loading all floating point registers with values
		 * that differ from the values used in other contexts is to help
		 * determine whether the floating point register save/restore mechanism
		 * in the nanokernel's context switcher is operating correctly.
		 *
		 * When a subsequent nano_fiber_timer_wait() invocation is performed, a
		 * (cooperative) context switch back to the preempted task will occur.
		 * This context switch should result in restoring the state of the
		 * task's floating point registers when the task was swapped out due
		 * to the occurence of the timer tick.
		 */

#if defined(CONFIG_ISA_IA32)
		_LoadThenStoreAllFloatRegisters(&floatRegisterSet);
#endif

		/*
		 * Relinquish the processor for the remainder of the current system
		 * clock tick, so that lower priority contexts get a chance to run.
		 *
		 * This exercises the ability of the nanokernel to restore the FPU
		 * state of a low priority context _and_ the ability of the nanokernel
		 * to provide a "clean" FPU state to this context once the sleep ends.
		 */

#ifdef CONFIG_NANOKERNEL
		nano_fiber_timer_start(&fiberTimer, 1);
		nano_fiber_timer_wait(&fiberTimer);
#else
		task_sleep(1);
#endif

		/* periodically issue progress report */

		if ((++load_store_high_count % 100) == 0) {
			PRINT_DATA("Load and store OK after %u (high) + %u (low) tests\n",
					   load_store_high_count, load_store_low_count);
		}

#if (MAX_TESTS != 0)
		/* terminate testing if specified limit has been reached */

		if (load_store_high_count == MAX_TESTS) {
			TC_END_RESULT(TC_PASS);
			TC_END_REPORT(TC_PASS);
			return;
		}
#endif
	}
}
