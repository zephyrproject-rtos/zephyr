/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/test_toolchain.h>

#define STACKSIZE       2048
#define THREAD_COUNT	64
#define VERBOSE		0

void *last_sp = (void *)0xFFFFFFFF;
volatile unsigned int changed;

/*
 * The `alternate_thread` function deliberately makes use of a dangling pointer
 * in order to test stack randomisation.
 */
TOOLCHAIN_DISABLE_GCC_WARNING(TOOLCHAIN_WARNING_DANGLING_POINTER)

void alternate_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	int i;
	void *sp_val;

	/* If the stack isn't being randomized then sp_val will never change */
	sp_val = &i;

#if VERBOSE
	printk("stack pointer: %p last: %p\n", sp_val, last_sp);
#endif

	if (last_sp != (void *)0xFFFFFFFF && sp_val != last_sp) {
		changed++;
	}
	last_sp = sp_val;
}

TOOLCHAIN_ENABLE_GCC_WARNING(TOOLCHAIN_WARNING_DANGLING_POINTER)

K_THREAD_STACK_DEFINE(alt_thread_stack_area, STACKSIZE);
static struct k_thread alt_thread_data;

/**
 * @brief Verify that thread stack pointers are randomized between threads.
 *
 * @ingroup kernel_memprotect_tests
 *
 * @details
 * With stack pointer randomization the kernel offsets each thread's initial
 * stack pointer by a random amount, so an attacker cannot predict where a
 * thread's stack begins. The same stack area is reused by many short-lived
 * threads in turn; each one records the address of a local variable, which
 * tracks its initial stack pointer, and compares it against the address the
 * previous thread saw. Without randomization every thread would start at the
 * same offset and the address would never change.
 *
 * Test steps:
 * - Lower the test thread to a preemptible priority so each spawned thread
 *   runs to completion.
 * - Create 64 threads in turn on one stack area, at the highest priority.
 * - In each thread, take the address of a local variable and compare it with
 *   the address recorded by the previous thread, counting the differences.
 * - Restore the test thread's priority.
 *
 * Expected result:
 * - The observed stack pointer differs between threads at least once, so the
 *   initial stack pointer is not fixed.
 *
 * @see k_thread_create()
 */
ZTEST(stack_pointer_randomness, test_stack_pt_randomization)
{
	int i, sp_changed;
	int old_prio = k_thread_priority_get(k_current_get());

	/* Set preemptible priority */
	k_thread_priority_set(k_current_get(), K_PRIO_PREEMPT(1));

	printk("Test Stack pointer randomization\n");

	/* Start thread */
	for (i = 0; i < THREAD_COUNT; i++) {
		k_thread_create(&alt_thread_data, alt_thread_stack_area,
				STACKSIZE, alternate_thread,
				NULL, NULL, NULL, K_HIGHEST_THREAD_PRIO, 0,
				K_NO_WAIT);
		k_sleep(K_MSEC(10));
	}


	printk("stack pointer changed %d times out of %d tests\n",
	       changed, THREAD_COUNT);

	sp_changed = changed;
	zassert_not_equal(sp_changed, 0, "Stack pointer is not randomized");

	/* Restore priority */
	k_thread_priority_set(k_current_get(), old_prio);
}

ZTEST_SUITE(stack_pointer_randomness, NULL, NULL,
		ztest_simple_1cpu_before, ztest_simple_1cpu_after, NULL);
