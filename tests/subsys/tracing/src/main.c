/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <ztest.h>

/**
 * @defgroup subsys_tracing_tests subsys tracing
 * @ingroup all_tests
 * @{
 * @}
 */

/* size of stack area used by each thread */
#define STACK_SIZE (512 + CONFIG_TEST_EXTRA_STACKSIZE)

static struct k_thread thread;
K_THREAD_STACK_DEFINE(thread_stack, STACK_SIZE);

/* define 2 semaphores */
K_SEM_DEFINE(thread1_sem, 1, 1);
K_SEM_DEFINE(thread2_sem, 0, 1);

/* thread handle for switch */
void thread_handle(void *p1, void *self_sem, void *other_sem)
{
	while (1) {
		/* take my semaphore */
		k_sem_take(self_sem, K_FOREVER);
		/* wait for a while, then let other thread have a turn. */
		k_sleep(K_MSEC(100));

		k_sem_give(other_sem);
	}
}

/**
 * @brief Test tracing data which produced from uart backend.
 *
 * @details
 * - Analyze the tracing data which produced from uart backend.
 * - tracing data will be checked with harness function in yaml file.
 *
 * @ingroup subsys_tracing_tests
 */
void test_tracing_function_uart(void)
{
#ifndef CONFIG_TRACING_BACKEND_UART
	ztest_test_skip();
#endif

	/* generate more tracing data.
	 * tracing data checked in yaml file.
	 */
	k_thread_create(&thread, thread_stack, STACK_SIZE,
		thread_handle, NULL, &thread2_sem, &thread1_sem,
		K_PRIO_PREEMPT(0), K_INHERIT_PERMS,
		K_NO_WAIT);

	thread_handle(NULL, &thread1_sem, &thread2_sem);
}

void test_main(void)
{
	k_thread_access_grant(k_current_get(),
				  &thread, &thread_stack,
				  &thread1_sem, &thread2_sem);

	ztest_test_suite(test_tracing,
			 ztest_unit_test(test_tracing_function_uart)
			 );

	ztest_run_test_suite(test_tracing);
}
