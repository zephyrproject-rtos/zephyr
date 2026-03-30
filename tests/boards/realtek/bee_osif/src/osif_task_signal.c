/*
 * Copyright (c) 2018 Intel Corporation
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * Implementation based on: tests/subsys/portability/cmsis_rtos_v2/src/thread_flags.c
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <os_task.h>
#include <os_sched.h>

#include <zephyr/irq_offload.h>
#include <zephyr/kernel_structs.h>

#define STACKSZ             512
#define EXPECTED_NOTIFY_VAL 0x1
#define TIMEOUT_TICKS       (10)

static void thread1(void *arg)
{
	bool status;
	uint32_t p_notify;

	/* Wait for notification */
	status = os_task_notify_take(true, TIMEOUT_TICKS * 2, &p_notify);
	zassert_equal(status, true, "notify wait failed unexpectedly");
	zassert_equal((p_notify & EXPECTED_NOTIFY_VAL), EXPECTED_NOTIFY_VAL,
		      "os_task_notify_take value check failed");

	/* validate by passing invalid parameters to GIVE */
	zassert_equal(os_task_notify_give(NULL), false,
		      "os_task_notify_give: Invalid Task Handle is unexpectedly working!");
}

ZTEST(osif_task_signal, test_task_notified)
{
	void *id2 = NULL;
	uint32_t task_param;
	bool status;

	status = os_task_create(&id2, "task_ntf", thread1, &task_param, STACKSZ, 6);
	zassert_true(status != false, "Failed creating thread1");

	/* Give notification */
	status = os_task_notify_give(id2);
	zassert_true(status == true, "os_task_notify_give fail");

	os_delay(TIMEOUT_TICKS);

	status = os_task_delete(id2);
	zassert_true(status != false, "Thread delete failed");
}

/* IRQ offload function handler to give notify */
static void offload_function(const void *param)
{
	k_tid_t tid = (k_tid_t)param;

	/* Make sure we're in IRQ context */
	zassert_true(k_is_in_isr(), "Not in IRQ context!");

	bool status = os_task_notify_give(tid);

	zassert_not_equal(status, false, "notify give failed in ISR");
}

/* Thread for ISR test */
void test_signal_from_isr(void *param)
{
	void *id;
	bool status;
	uint32_t p_notify;

	status = os_task_handle_get(&id);
	zassert_true(status, "Failed to get handle");

	/**TESTPOINT: Offload to IRQ context*/
	irq_offload(offload_function, (const void *)id);

	/* Wait for notify from ISR */
	status = os_task_notify_take(true, TIMEOUT_TICKS * 5, &p_notify);
	zassert_equal(status, true, "notify wait from ISR failed unexpectedly");
	zassert_equal((p_notify & EXPECTED_NOTIFY_VAL), EXPECTED_NOTIFY_VAL,
		      "unexpected notify value from ISR");
}

ZTEST(osif_task_signal, test_notify_from_isr)
{
	void *id3 = NULL;
	uint32_t task_param;
	bool status;

	status = os_task_create(&id3, "test_isr", test_signal_from_isr, &task_param, STACKSZ, 6);

	zassert_true(status != false, "Thread creation failed");

	os_delay(1000);

	status = os_task_delete(id3);
	zassert_true(status != false, "Thread delete failed");
}

ZTEST_SUITE(osif_task_signal, NULL, NULL, NULL, NULL, NULL);
