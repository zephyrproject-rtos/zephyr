/*
 * Copyright (c) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#define EXTRA_THREAD_STACKSIZE 2048

struct k_thread extra_thread;
K_THREAD_STACK_DEFINE(extra_stack, EXTRA_THREAD_STACKSIZE);

static void thread_entry(void *p1, void *p2, void *p3)
{
	/* This thread does not have a name so thread analyzer will display
	 * the memory address of the thread struct, which is needed for
	 * the twister console harness to match (even if CONFIG_THREAD_NAME=y).
	 */
	while (true) {
		k_sleep(K_SECONDS(300));
	}
}

int main(void)
{
	k_thread_create(&extra_thread, extra_stack, EXTRA_THREAD_STACKSIZE,
			thread_entry, NULL, NULL, NULL, K_PRIO_PREEMPT(0),
			IS_ENABLED(CONFIG_USERSPACE) ? K_USER : 0,
			K_MSEC(0));
	return 0;
}
