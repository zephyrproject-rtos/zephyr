/*
 * Copyright (c) 2011-2014 Wind River Systems, Inc.
 * Copyright (c) 2020 Stephanos Ioannidis <root@stephanos.io>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * @file
 * @brief load/store portion of FPU sharing test
 *
 * @defgroup kernel_fpsharing_tests FP Sharing Tests
 *
 * @ingroup all_tests
 *
 * This module implements the load/store portion of the  FPU sharing test. This
 * version of this test utilizes a pair of tasks.
 *
 * The load/store test validates the floating point unit context
 * save/restore mechanism. This test utilizes a pair of threads of different
 * priorities that each use the floating point registers. The context
 * switching that occurs exercises the kernel's ability to properly preserve
 * the floating point registers. The test also exercises the kernel's ability
 * to automatically enable floating point support for a task, if supported.
 *
 * FUTURE IMPROVEMENTS
 * On architectures where the non-integer capabilities are provided in a
 * hierarchy, for example on IA-32 the USE_FP and USE_SSE options are provided,
 * this test should be enhanced to ensure that the architectures' z_swap()
 * routine doesn't context switch more registers that it needs to (which would
 * represent a performance issue).  For example, on the IA-32, the test should
 * issue a k_fp_disable() from main(), and then indicate that only x87 FPU
 * registers will be utilized (k_fp_enable()).  The thread should continue
 * to load ALL non-integer registers, but main() should validate that only the
 * x87 FPU registers are being saved/restored.
 */

#include <zephyr/ztest.h>
#include <zephyr/debug/gcov.h>

#if defined(CONFIG_X86)
#if defined(__GNUC__)
#include "float_regs_x86_gcc.h"
#else
#include "float_regs_x86_other.h"
#endif /* __GNUC__ */
#elif defined(CONFIG_ARMV7_M_ARMV8_M_FP) || defined(CONFIG_ARMV7_R_FP)
#if defined(__GNUC__)
#include "float_regs_arm_gcc.h"
#else
#include "float_regs_arm_other.h"
#endif /* __GNUC__ */
#elif defined(CONFIG_ARM64)
#if defined(__GNUC__)
#include "float_regs_arm64_gcc.h"
#else
#include "float_regs_arm64_other.h"
#endif /* __GNUC__ */
#elif defined(CONFIG_ISA_ARCV2)
#if defined(__GNUC__)
#include "float_regs_arc_gcc.h"
#else
#include "float_regs_arc_other.h"
#endif /* __GNUC__ */
#elif defined(CONFIG_RISCV)
#if defined(__GNUC__)
#include "float_regs_riscv_gcc.h"
#else
#include "float_regs_riscv_other.h"
#endif /* __GNUC__ */
#elif defined(CONFIG_SPARC)
#include "float_regs_sparc.h"
#endif

#include "float_context.h"
#include "test_common.h"

/* space for float register load/store area used by low priority task */
static struct fp_register_set float_reg_set_load;
static struct fp_register_set float_reg_set_store;

/* space for float register load/store area used by high priority thread */
static struct fp_register_set float_reg_set;

/*
 * Test counters are "volatile" because GCC may not update them properly
 * otherwise. (See description of pi calculation test for more details.)
 */
static volatile unsigned int load_store_low_count;
static volatile unsigned int load_store_high_count;

/* Indicates that the load/store test exited */
static volatile bool test_exited;

/* Semaphore for signaling end of test */
static K_SEM_DEFINE(test_exit_sem, 0, 1);

/**
 * @brief Low priority FPU load/store thread
 *
 * @ingroup kernel_fpsharing_tests
 *
 * @see k_sched_time_slice_set(), memset(),
 * _load_all_float_registers(), _store_all_float_registers()
 */
static void load_store_low(void)
{
	unsigned int i;
	bool error = false;
	unsigned char init_byte;
	unsigned char *store_ptr = (unsigned char *)&float_reg_set_store;
	unsigned char *load_ptr = (unsigned char *)&float_reg_set_load;

	volatile char volatile_stack_var = 0;

	/*
	 * Initialize floating point load buffer to known values;
	 * these values must be different than the value used in other threads.
	 */
	init_byte = MAIN_FLOAT_REG_CHECK_BYTE;
	for (i = 0; i < SIZEOF_FP_REGISTER_SET; i++) {
		load_ptr[i] = init_byte++;
	}

	/* Loop until the test finishes, or an error is detected. */
	for (load_store_low_count = 0; !test_exited; load_store_low_count++) {

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
		 * IMPORTANT: This logic requires that sys_clock_tick_get_32() not
		 * perform any floating point operations!
		 */
		while ((sys_clock_tick_get_32() % 5) != 0) {
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

		for (i = 0; i < SIZEOF_FP_REGISTER_SET; i++) {
			if (store_ptr[i] != init_byte) {
				TC_ERROR("Found 0x%x instead of 0x%x @ "
					 "offset 0x%x\n",
					 store_ptr[i],
					 init_byte, i);
				TC_ERROR("Discrepancy found during "
					 "iteration %d\n",
					 load_store_low_count);
				error = true;
			}
			init_byte++;
		}

		/* Terminate if a test error has been reported */
		zassert_false(error);

		/*
		 * After every 1000 iterations (arbitrarily chosen), explicitly
		 * disable floating point operations for the task.
		 */
#if (defined(CONFIG_X86) && defined(CONFIG_LAZY_FPU_SHARING)) || \
		defined(CONFIG_ARMV7_M_ARMV8_M_FP) || defined(CONFIG_ARMV7_R_FP)
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
		if ((load_store_low_count % 1000) == 0) {
			k_float_disable(k_current_get());
		}
#endif
	}
}

/**
 * @brief High priority FPU load/store thread
 *
 * @ingroup kernel_fpsharing_tests
 *
 * @see _load_then_store_all_float_registers()
 */
static void load_store_high(void)
{
	unsigned int i;
	unsigned char init_byte;
	unsigned char *reg_set_ptr = (unsigned char *)&float_reg_set;

	/* Run the test until the specified maximum test count is reached */
	for (load_store_high_count = 0;
	     load_store_high_count <= MAX_TESTS;
	     load_store_high_count++) {

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

		for (i = 0; i < SIZEOF_FP_REGISTER_SET; i++) {
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

		/* Periodically issue progress report */
		if ((load_store_high_count % 100) == 0) {
			PRINT_DATA("Load and store OK after %u (high) "
				   "+ %u (low) tests\n",
				   load_store_high_count,
				   load_store_low_count);
		}
	}

#ifdef CONFIG_COVERAGE_GCOV
	gcov_coverage_dump();
#endif

	/* Signal end of test */
	test_exited = true;
	k_sem_give(&test_exit_sem);
}

K_THREAD_DEFINE(load_low, THREAD_STACK_SIZE, load_store_low, NULL, NULL, NULL,
		THREAD_LOW_PRIORITY, THREAD_FP_FLAGS, K_TICKS_FOREVER);

K_THREAD_DEFINE(load_high, THREAD_STACK_SIZE, load_store_high, NULL, NULL, NULL,
		THREAD_HIGH_PRIORITY, THREAD_FP_FLAGS, K_TICKS_FOREVER);

ZTEST(fpu_sharing_generic, test_load_store)
{
	/* Initialise test states */
	test_exited = false;
	k_sem_reset(&test_exit_sem);

	/* Start test threads */
	k_thread_start(load_low);
	k_thread_start(load_high);

	/* Wait for test threads to exit */
	k_sem_take(&test_exit_sem, K_FOREVER);
}
