/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_sched.h"

/* Shared threads */
K_THREAD_STACK_DEFINE(tstack, STACK_SIZE);
K_THREAD_STACK_ARRAY_DEFINE(tstacks, MAX_NUM_THREAD, STACK_SIZE);

/* Not in header file intentionally, see #16760 */
K_THREAD_STACK_DECLARE(ustack, STACK_SIZE);

void spin_for_ms(int ms)
{
	uint32_t t32 = k_uptime_get_32();

	while (k_uptime_get_32() - t32 < ms) {
		/* In the posix arch, a busy loop takes no time, so
		 * let's make it take some
		 */
		Z_SPIN_DELAY(50);
	}
}

static void *threads_scheduling_tests_setup(void)
{
#ifdef CONFIG_USERSPACE
	k_thread_access_grant(k_current_get(), &user_thread, &user_sem,
			      &ustack);
#endif /* CONFIG_USERSPACE */

	return NULL;
}

static bool in_stack(uintptr_t addr, const void *stack, size_t size)
{
	return addr >= (uintptr_t)stack && addr < (uintptr_t)stack + size;
}

struct leftovers {
	k_tid_t tid[MAX_NUM_THREAD + 2];
	size_t count;
};

static void find_leftover(const struct k_thread *thread, void *data)
{
	struct leftovers *left = data;
	uintptr_t start = thread->stack_info.start;

	if ((in_stack(start, tstacks, sizeof(tstacks)) ||
	     in_stack(start, tstack, sizeof(tstack)) ||
	     in_stack(start, ustack, sizeof(ustack))) &&
	    left->count < ARRAY_SIZE(left->tid)) {
		left->tid[left->count++] = (k_tid_t)thread;
	}
}

/* A failed assertion ends the test before it aborts its threads. Abort
 * them here: the next test reuses their stacks and thread objects.
 */
static void threads_scheduling_after(void *data)
{
	struct leftovers left = { .count = 0 };

	ARG_UNUSED(data);

	k_thread_foreach(find_leftover, &left);
	for (size_t i = 0; i < left.count; i++) {
		k_thread_abort(left.tid[i]);
	}
}

static void threads_scheduling_1cpu_after(void *data)
{
	threads_scheduling_after(data);
	ztest_simple_1cpu_after(data);
}

/**
 * @brief Test scheduling
 *
 * @defgroup tests_kernel_sched Scheduling tests
 *
 * @ingroup all_tests
 *
 * @{
 * @}
 */
ZTEST_SUITE(threads_scheduling, NULL, threads_scheduling_tests_setup, NULL,
	    threads_scheduling_after, NULL);
ZTEST_SUITE(threads_scheduling_1cpu, NULL, threads_scheduling_tests_setup,
			ztest_simple_1cpu_before, threads_scheduling_1cpu_after, NULL);
