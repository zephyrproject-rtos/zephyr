/*
 * Copyright (c) 2011-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * @brief load/store portion of FPU sharing test
 *
 * @defgroup kernel_fpsharing_tests FP Sharing Tests
 *
 * @ingroup all_tests
 *
 * @details This module implements the load/store portion of the
 * FPU sharing test. This version of this test utilizes a pair of tasks.
 *
 * The load/store test validates the floating point unit context
 * save/restore mechanism. This test utilizes a pair of threads of different
 * priorities that each use the floating point registers. The context
 * switching that occurs exercises the kernel's ability to properly preserve the
 * floating point registers. The test also exercises the kernel's ability to
 * automatically enable floating point support for a task, if supported.
 *
 * FUTURE IMPROVEMENTS
 * On architectures where the non-integer capabilities are provided in a
 *  hierarchy, for example on IA-32 the USE_FP and USE_SSE options are provided,
 * this test should be enhanced to ensure that the architectures' z_swap()
 * routine doesn't context switch more registers that it needs to (which would
 * represent a performance issue).  For example, on the IA-32, the test should
 * issue a k_fp_disable() from main(), and then indicate that only x87 FPU
 * registers will be utilized (k_fp_enable()).  The thread should continue
 * to load ALL non-integer registers, but main() should validate that only the
 * x87 FPU registers are being saved/restored.
 * @{
 * @}
 */

#ifndef CONFIG_FLOAT
#error Rebuild with the FLOAT config option enabled
#endif

#ifndef CONFIG_FP_SHARING
#error Rebuild with the FP_SHARING config option enabled
#endif

#if defined(CONFIG_X86)
#ifndef CONFIG_SSE
#error Rebuild with the SSE config option enabled
#endif
#endif /* CONFIG_X86 */

#include <zephyr.h>

#if defined(CONFIG_X86)
#if defined(__GNUC__)
#include "float_regs_x86_gcc.h"
#else
#include "float_regs_x86_other.h"
#endif /* __GNUC__ */
#elif defined(CONFIG_ARMV7_M_ARMV8_M_FP)
#if defined(__GNUC__)
#include "float_regs_arm_gcc.h"
#else
#include "float_regs_arm_other.h"
#endif /* __GNUC__ */
#elif defined(CONFIG_CPU_ARCV2)
#if defined(__GNUC__)
#include "float_regs_arc_gcc.h"
#else
#include "float_regs_arc_other.h"
#endif /* __GNUC__ */
#endif

#include <tc_util.h>
#include "float_context.h"
#include <stddef.h>
#include <string.h>
#include <debug/gcov.h>

#define MAX_TESTS 500
#define STACKSIZE 2048
#define HI_PRI  5
#define LO_PRI  10

/* space for float register load/store area used by low priority task */

static struct fp_register_set float_reg_set_load;
static struct fp_register_set float_reg_set_store;

/* space for float register load/store area used by high priority thread */

static struct fp_register_set float_reg_set;


/* flag indicating that an error has occurred */

int fpu_sharing_error;

/*
 * Test counters are "volatile" because GCC may not update them properly
 * otherwise. (See description of pi calculation test for more details.)
 */

static volatile unsigned int load_store_low_count;
static volatile unsigned int load_store_high_count;

extern void calculate_pi_low(void);
extern void calculate_pi_high(void);

/**
 *
 * @brief Low priority FPU load/store thread
 *
 * @ingroup kernel_fpsharing_tests
 *
 * @see k_sched_time_slice_set(), memset(),
 * _load_all_float_registers(), _store_all_float_registers()
 */

void load_store_low(void)
{
	unsigned int i;
	unsigned char init_byte;
	unsigned char *store_ptr = (unsigned char *)&float_reg_set_store;
	unsigned char *load_ptr = (unsigned char *)&float_reg_set_load;

	volatile char volatile_stack_var = 0;

	PRINT_DATA("Floating point sharing tests started\n");
	PRINT_LINE;

	/*
	 * The high priority thread has a sleep to get this (low pri) thread
	 * running and here (low priority) we enable slicing and waste cycles
	 * to run hi pri thread in between fp ops.
	 *
	 * Enable round robin scheduling to allow both the low priority pi
	 * computation and load/store tasks to execute. The high priority pi
	 * computation and load/store tasks will preempt the low priority tasks
	 * periodically.
	 */

	k_sched_time_slice_set(10, LO_PRI);

	/*
	 * Initialize floating point load buffer to known values;
	 * these values must be different than the value used in other threads.
	 */

	init_byte = MAIN_FLOAT_REG_CHECK_BYTE;
	for (i = 0U; i < SIZEOF_FP_REGISTER_SET; i++) {
		load_ptr[i] = init_byte++;
	}

	/* Keep cranking forever, or until an error is detected. */

	for (load_store_low_count = 0U;; load_store_low_count++) {

		/*
		 * Clear store buffer to erase all traces of any previous
		 * floating point values that have been saved.
		 */

		(void)memset(&float_reg_set_store, 0, SIZEOF_FP_REGISTER_SET);

		/*
		 * Utilize an architecture specific function to load all the
		 * floating point registers with known values.
		 */

		_load_all_float_registers(&float_reg_set_load);

		/*
		 * Waste some cycles to give the high priority load/store
		 * thread an opportunity to run when the low priority thread is
		 * using the floating point registers.
		 *
		 * IMPORTANT: This logic requires that z_tick_get_32() not
		 * perform any floating point operations!
		 */

		while ((z_tick_get_32() % 5) != 0U) {
			/*
			 * Use a volatile variable to prevent compiler
			 * optimizing out the spin loop.
			 */
			++volatile_stack_var;
		}

		/*
		 * Utilize an architecture specific function to dump the
		 * contents of all floating point registers to memory.
		 */

		_store_all_float_registers(&float_reg_set_store);

		/*
		 * Compare each byte of buffer to ensure the expected value is
		 * present, indicating that the floating point registers weren't
		 * impacted by the operation of the high priority thread(s).
		 *
		 * Display error message and terminate if discrepancies are
		 * detected.
		 */

		init_byte = MAIN_FLOAT_REG_CHECK_BYTE;

		for (i = 0U; i < SIZEOF_FP_REGISTER_SET; i++) {
			if (store_ptr[i] != init_byte) {
				TC_ERROR("load_store_low found 0x%x instead "
					 "of 0x%x @ offset 0x%x\n",
					 store_ptr[i],
					 init_byte, i);
				TC_ERROR("Discrepancy found during "
					 "iteration %d\n",
					 load_store_low_count);
				fpu_sharing_error = 1;
			}
			init_byte++;
		}

		/*
		 * Terminate if a test error has been reported.
		 */

		if (fpu_sharing_error) {
			TC_END_RESULT(TC_FAIL);
			TC_END_REPORT(TC_FAIL);
			return;
		}

		/*
		 * After every 1000 iterations (arbitrarily chosen), explicitly
		 * disable floating point operations for the task.
		 */
#if (defined(CONFIG_X86) && defined(CONFIG_LAZY_FP_SHARING)) || \
		defined(CONFIG_ARMV7_M_ARMV8_M_FP)
		/*
		 * In x86:
		 * The subsequent execution of _load_all_float_registers() will
		 * result in an exception to automatically re-enable
		 * floating point support for the task.
		 *
		 * The purpose of this part of the test is to exercise the
		 * k_float_disable() API, and to also continue exercising
		 * the (exception based) floating enabling mechanism.
		 *
		 * In ARM:
		 *
		 * The routine k_float_disable() allows for thread-level
		 * granularity for disabling floating point. Furthermore, it
		 * is useful for testing automatic thread enabling of floating
		 * point as soon as FP registers are used, again by the thread.
		 */
		if ((load_store_low_count % 1000) == 0U) {
			k_float_disable(k_current_get());
		}
#endif
	}
}

/**
 *
 * @brief High priority FPU load/store thread
 *
 * @ingroup kernel_fpsharing_tests
 *
 * @see _load_then_store_all_float_registers()
 */

void load_store_high(void)
{
	unsigned int i;
	unsigned char init_byte;
	unsigned char *reg_set_ptr = (unsigned char *)&float_reg_set;

	/* test until the specified time limit, or until an error is detected */

	while (1) {
		/*
		 * Initialize the float_reg_set structure by treating it as
		 * a simple array of bytes (the arrangement and actual number
		 * of registers is not important for this generic C code).  The
		 * structure is initialized by using the byte value specified
		 * by the constant FIBER_FLOAT_REG_CHECK_BYTE, and then
		 * incrementing the value for each successive location in the
		 * float_reg_set structure.
		 *
		 * The initial byte value, and thus the contents of the entire
		 * float_reg_set structure, must be different for each
		 * thread to effectively test the kernel's ability to
		 * properly save/restore the floating point values during a
		 * context switch.
		 */

		init_byte = FIBER_FLOAT_REG_CHECK_BYTE;

		for (i = 0U; i < SIZEOF_FP_REGISTER_SET; i++) {
			reg_set_ptr[i] = init_byte++;
		}

		/*
		 * Utilize an architecture specific function to load all the
		 * floating point registers with the contents of the
		 * float_reg_set structure.
		 *
		 * The goal of the loading all floating point registers with
		 * values that differ from the values used in other threads is
		 * to help determine whether the floating point register
		 * save/restore mechanism in the kernel's context switcher
		 * is operating correctly.
		 *
		 * When a subsequent k_timer_test() invocation is
		 * performed, a (cooperative) context switch back to the
		 * preempted task will occur. This context switch should result
		 * in restoring the state of the task's floating point
		 * registers when the task was swapped out due to the
		 * occurrence of the timer tick.
		 */

		_load_then_store_all_float_registers(&float_reg_set);

		/*
		 * Relinquish the processor for the remainder of the current
		 * system clock tick, so that lower priority threads get a
		 * chance to run.
		 *
		 * This exercises the ability of the kernel to restore the
		 * FPU state of a low priority thread _and_ the ability of the
		 * kernel to provide a "clean" FPU state to this thread
		 * once the sleep ends.
		 */

		k_sleep(K_MSEC(1));

		/* periodically issue progress report */

		if ((++load_store_high_count % 100) == 0U) {
			PRINT_DATA("Load and store OK after %u (high) "
				   "+ %u (low) tests\n",
				   load_store_high_count,
				   load_store_low_count);
		}

		/* terminate testing if specified limit has been reached */

		if (load_store_high_count == MAX_TESTS) {
			TC_END_RESULT(TC_PASS);
			TC_END_REPORT(TC_PASS);
#ifdef CONFIG_COVERAGE_GCOV
			gcov_coverage_dump();
#endif
			return;
		}
	}
}

#if defined(CONFIG_X86)
#define THREAD_FP_FLAGS (K_FP_REGS | K_SSE_REGS)
#else
#define THREAD_FP_FLAGS (K_FP_REGS)
#endif

K_THREAD_DEFINE(load_low, STACKSIZE, load_store_low, NULL, NULL, NULL,
		LO_PRI, THREAD_FP_FLAGS, K_NO_WAIT);

K_THREAD_DEFINE(load_high, STACKSIZE, load_store_high, NULL, NULL, NULL,
		HI_PRI, THREAD_FP_FLAGS, K_NO_WAIT);

K_THREAD_DEFINE(pi_low, STACKSIZE, calculate_pi_low, NULL, NULL, NULL,
		LO_PRI, THREAD_FP_FLAGS, K_NO_WAIT);

K_THREAD_DEFINE(pi_high, STACKSIZE, calculate_pi_high, NULL, NULL, NULL,
		HI_PRI, THREAD_FP_FLAGS, K_NO_WAIT);

void main(void *p1, void *p2, void *p3)
{
	/* This very old test didn't have a main() function, and would
	 * dump gcov data immediately. Sleep forever, we'll invoke
	 * gcov manually later when the test completes.
	 */
	while (true) {
		k_sleep(K_MSEC(1000));
	}
}
