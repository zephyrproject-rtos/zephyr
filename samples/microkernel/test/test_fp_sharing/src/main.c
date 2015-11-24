/* main.c - load/store portion of FPU sharing test */

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

/*
DESCRIPTION
This module implements the load/store portion of the FPU sharing test. This
microkernel version of this test utilizes a pair of tasks, while the nanokernel
verions utilizes a task and a fiber.

The load/store test validates the nanokernel's floating point unit context
save/restore mechanism. (For the IA-32 architecture this includes the x87 FPU
(MMX) registers and the XMM registers.) This test utilizes a pair of threads
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
registers will be utilized (nanoCpuFpEnable).  The fiber should continue
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

#endif /* CONFIG_ISA_IA32 */


#include <zephyr.h>

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
#define MAX_TESTS 500
#endif

/* macro used to read system clock value */

#ifdef CONFIG_NANOKERNEL
#define TICK_COUNT_GET() sys_tick_get_32()
#else
#define TICK_COUNT_GET() task_tick_get_32()
#endif

/* space for float register load/store area used by low priority task */

static FP_REG_SET floatRegSetLoad;
static FP_REG_SET floatRegSetStore;

/* space for float register load/store area used by high priority thread */

static FP_REG_SET floatRegisterSet;


#ifdef CONFIG_NANOKERNEL
/* stack for high priority fiber (also use .bss for floatRegisterSet) */

static char __stack fiberStack[1024];

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

/**
 *
 * main -
 * @brief Low priority FPU load/store thread
 *
 * @return N/A
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

#if defined(CONFIG_FP_SHARING)
	/*
	 * No need to invoke task_float_enable() since
	 * FP_SHARING is in effect
	 */
#else /* ! CONFIG_FP_SHARING */
#if defined(CONFIG_FLOAT)
	task_float_enable(sys_thread_self_get());
#endif
#endif /* CONFIG_FP_SHARING */

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
	 * For microkernel builds, preemption tasks are specified in the .mdef file.
	 *
	 * Enable round robin scheduling to allow both the low priority pi
	 * computation and load/store tasks to execute. The high priority pi
	 * computation and load/store tasks will preempt the low priority tasks
	 * periodically.
	 */

	sys_scheduler_time_slice_set(1, 10);
#endif /*  CONFIG_NANOKERNEL */

	/*
	 * Initialize floating point load buffer to known values;
	 * these values must be different than the value used in other threads.
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
		 * Waste some cycles to give the high priority load/store thread
		 * an opportunity to run when the low priority thread is using the
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
		 * impacted by the operation of the high priority thread(s).
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

#if defined(CONFIG_FP_SHARING)
		/*
		 * After every 1000 iterations (arbitrarily chosen), explicitly
		 * disable floating point operations for the task. The subsequent
		 * execution of _LoadAllFloatRegisters() will result in an exception
		 * to automatically re-enable floating point support for the task.
		 *
		 * The purpose of this part of the test is to exercise the
		 * task_float_disable() API, and to also continue exercising the
		 * (exception based) floating enabling mechanism.
		 */
		if ((load_store_low_count % 1000) == 0) {
#if defined(CONFIG_FLOAT)
			task_float_disable(sys_thread_self_get());
#endif
		}
#endif /* CONFIG_FP_SHARING */
	}
}

/**
 *
 * @brief High priority FPU load/store thread
 *
 * @return N/A
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
		 * floatRegisterSet structure, must be different for each thread to
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
		 * that differ from the values used in other threads is to help
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
		 * clock tick, so that lower priority threads get a chance to run.
		 *
		 * This exercises the ability of the nanokernel to restore the FPU
		 * state of a low priority thread _and_ the ability of the nanokernel
		 * to provide a "clean" FPU state to this thread once the sleep ends.
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
