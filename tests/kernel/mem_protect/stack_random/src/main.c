/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <zephyr.h>

#define STACKSIZE       2048
#define THREAD_COUNT	64
#define VERBOSE		0

void *last_sp = (void *)0xFFFFFFFF;
volatile unsigned int changed;

void alternate_thread(void)
{
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


K_THREAD_STACK_DEFINE(alt_thread_stack_area, STACKSIZE);
static struct k_thread alt_thread_data;

/**
 * @brief Test stack pointer randomization
 *
 * @ingroup kernel_memprotect_tests
 *
 */
void test_stack_pt_randomization(void)
{
	int i;
	int old_prio = k_thread_priority_get(k_current_get());

	/* Set preemptable priority */
	k_thread_priority_set(k_current_get(), K_PRIO_PREEMPT(1));

	printk("Test Stack pointer randomization\n");

	/* Start thread */
	for (i = 0; i < THREAD_COUNT; i++) {
		k_thread_create(&alt_thread_data, alt_thread_stack_area,
				STACKSIZE, (k_thread_entry_t)alternate_thread,
				NULL, NULL, NULL, K_HIGHEST_THREAD_PRIO, 0,
				K_NO_WAIT);
	}


	printk("stack pointer changed %d times out of %d tests\n",
	       changed, THREAD_COUNT);

	zassert_not_equal(changed, 0, "Stack pointer is not randomized");

	/* Restore priority */
	k_thread_priority_set(k_current_get(), old_prio);
}

void test_main(void)
{
	ztest_test_suite(stack_pointer_randomness,
			ztest_1cpu_unit_test(test_stack_pt_randomization));
	ztest_run_test_suite(stack_pointer_randomness);
}
