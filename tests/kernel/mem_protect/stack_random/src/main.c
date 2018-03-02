/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <tc_util.h>
#include <zephyr.h>

#define STACKSIZE       2048
#define THREAD_COUNT	64
#define VERBOSE		0

void *last_sp = (void *)0xFFFFFFFF;
unsigned int changed;

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

void main(void)
{
	int i, ret = TC_FAIL;

	TC_START("Test Stack pointer randomization\n");

	/* Start thread */
	for (i = 0; i < THREAD_COUNT; i++) {
		k_thread_create(&alt_thread_data, alt_thread_stack_area,
				STACKSIZE, (k_thread_entry_t)alternate_thread,
				NULL, NULL, NULL, K_HIGHEST_THREAD_PRIO, 0,
				K_NO_WAIT);
	}

	printk("stack pointer changed %d times out of %d tests\n",
	       changed, THREAD_COUNT);

	if (changed) {
		ret = TC_PASS;
	}

	TC_END_RESULT(ret);
	TC_END_REPORT(ret);
}
