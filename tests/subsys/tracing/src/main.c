/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <ztest.h>
#if defined(CONFIG_TRACING_BACKEND_UART) || defined(CONFIG_TRACING_BACKEND_USB)
#include "../../../../subsys/tracing/include/tracing_backend.h"
#endif

/**
 * @defgroup subsys_tracing_tests subsys tracing
 * @ingroup all_tests
 * @{
 * @}
 */

/* size of stack area used by each thread */
#define STACK_SIZE (512 + CONFIG_TEST_EXTRA_STACKSIZE)
#define LOOP_TIMES 10

bool thread_switched_found;
bool sys_trace_void_found;
bool sys_trace_semaphore_found;

static struct k_thread thread;
K_THREAD_STACK_DEFINE(thread_stack, STACK_SIZE);

/* define 2 semaphores */
K_SEM_DEFINE(thread1_sem, 1, 1);
K_SEM_DEFINE(thread2_sem, 0, 1);

/* thread handle for switch */
void thread_handle(void *p1, void *self_sem, void *other_sem)
{
	for (int i = 0; i < LOOP_TIMES; i++) {
		/* take my semaphore */
		k_sem_take(self_sem, K_FOREVER);
		/* wait for a while, then let other thread have a turn. */
		k_sleep(K_MSEC(10));

		k_sem_give(other_sem);
	}
}

#if (defined(CONFIG_TRACING_BACKEND_UART) || defined(CONFIG_TRACING_BACKEND_USB))
static void tracing_backends_output(
		const struct tracing_backend *backend,
		uint8_t *data, uint32_t length)
{
	/* Check the output data. */
	if (strstr(data, "sys_trace_thread_switched") != NULL) {
		thread_switched_found = true;
	}

	if (strstr(data, "sys_trace_void") != NULL) {
		sys_trace_void_found = true;
	}

	if (strstr(data, "sys_trace_semaphore") != NULL) {
		sys_trace_semaphore_found = true;
	}
}
#endif

#if defined(CONFIG_TRACING_BACKEND_UART)
const struct tracing_backend_api tracing_uart_backend_api = {
	.init = NULL,
	.output  = tracing_backends_output
};

TRACING_BACKEND_DEFINE(tracing_backend_uart, tracing_uart_backend_api);
#endif

#ifdef CONFIG_TRACING_BACKEND_USB
const struct tracing_backend_api tracing_usb_backend_api = {
	.output = tracing_backends_output
};

TRACING_BACKEND_DEFINE(tracing_backend_usb, tracing_usb_backend_api);
#endif

/*
 * spawn a thread to generate more tracing data about
 * thread switch and semaphore.
 */
void generate_more_tracing_data(void)
{
	k_thread_create(&thread, thread_stack, STACK_SIZE,
		thread_handle, NULL, &thread2_sem, &thread1_sem,
		K_PRIO_PREEMPT(0), K_INHERIT_PERMS,
		K_NO_WAIT);

	thread_handle(NULL, &thread1_sem, &thread2_sem);
}

/**
 * @brief Test tracing data which produced from backends.
 *
 * @ingroup driver_sensor_subsys_tests
 *
 * @details
 * Test Objective:
 * - Check the tracing data in backends, it should include thread/semaphore.. info,
 * if not, the related variable should be false.
 *
 * Testing techniques:
 * - function and black box testing.
 * - Interface testing.
 * - Dynamic analysis and testing.
 *
 * Prerequisite Conditions:
 * - CONFIG_TRACING
 * - CONFIG_TRACING_TEST
 * - CONFIG_TRACING_BACKEND_USB
 * - CONFIG_TRACING_BACKEND_UART
 *
 * Input Specifications:
 * - N/A
 *
 * Test Procedure:
 * -# Create a thread to generate more switch thread tracing data.
 * -# Define a USB and a UART backend to get tracing data.
 * -# Check the content of the traing output.
 *
 * Expected Test Result:
 * - Tracing data include thread,idle thread info
 * based on the provided trace data.
 *
 * Pass/Fail Criteria:
 * - Successful if check points in test procedure are all passed, otherwise failure.
 *
 * Assumptions and Constraints:
 * - N/A
 */
void test_tracing_function(void)
{
#ifdef CONFIG_TRACING_CTF
	ztest_test_skip();
#endif
	generate_more_tracing_data();
#ifdef CONFIG_TRACING
	zassert_true(thread_switched_found, "thread_switched can't be found.");
	zassert_true(sys_trace_void_found, "sys_trace_void can't be found.");
	zassert_true(sys_trace_semaphore_found,
			"sys_trace_semaphore can't be found.");
#else
	/* Should produce any tracing data when tracing disabled. */
	zassert_false(thread_switched_found, "thread_switched shouldn't be found.");
	zassert_false(sys_trace_void_found, "thread_switched shouldn't be found.");
	zassert_false(sys_trace_semaphore_found, "thread_switched shouldn't be found.");
#endif
}

void test_main(void)
{
	k_thread_access_grant(k_current_get(),
				  &thread, &thread_stack,
				  &thread1_sem, &thread2_sem);

	ztest_test_suite(test_tracing,
			 ztest_unit_test(test_tracing_function)
			 );

	ztest_run_test_suite(test_tracing);
}
