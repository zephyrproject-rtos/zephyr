/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <drivers/interrupt_controller/intc_swi.h>

#define TEST_TIMEOUT K_MSEC(100)

static struct swi_channel swi;
static K_SEM_DEFINE(swi_sem, 0, 1);

static void swi_cb(struct swi_channel *swi)
{
	k_sem_give(&swi_sem);
}

static void teardown(void)
{
	swi_channel_deinit(&swi);
	k_sem_reset(&swi_sem);
}

static void test_intc_swi_init_deinit_shall_succeed(void)
{
	int result;

	/* Init SWI twice. */
	result = swi_channel_init(&swi, swi_cb);
	zassert_true(result == 0, "swi_channel_init errno: %d", -result);
	result = swi_channel_deinit(&swi);
	zassert_true(result == 0, "swi_channel_deinit errno: %d", -result);
	result = swi_channel_init(&swi, swi_cb);
	zassert_true(result == 0, "swi_channel_init errno: %d", -result);
}

static void test_intc_swi_double_init_shall_fail(void)
{
	int result;

	result = swi_channel_init(&swi, swi_cb);
	zassert_true(result == 0, "swi_channel_init errno: %d", -result);
	result = swi_channel_init(&swi, swi_cb);
	zassert_true(result == -EALREADY, "swi_channel_init errno: %d", -result);
}

static void test_intc_swi_trigger_shall_call_function(void)
{
	int result;

	result = swi_channel_init(&swi, swi_cb);
	zassert_true(result == 0, "swi_channel_init errno: %d", -result);
	result = swi_channel_trigger(&swi);
	zassert_true(result == 0, "swi_channel_trigger errno: %d", -result);

	result = k_sem_take(&swi_sem, TEST_TIMEOUT);
	zassert_true(result == 0, "SWI trigger test timed out");
}

extern void test_intc_swi_stress(void);

void test_main(void)
{
	ztest_test_suite(test_intc_swi,
			 ztest_unit_test_setup_teardown(test_intc_swi_init_deinit_shall_succeed,
							unit_test_noop, teardown),
			 ztest_unit_test_setup_teardown(test_intc_swi_double_init_shall_fail,
							unit_test_noop, teardown),
			 ztest_unit_test_setup_teardown(test_intc_swi_trigger_shall_call_function,
							unit_test_noop, teardown),
			 ztest_unit_test(test_intc_swi_stress));
	ztest_run_test_suite(test_intc_swi);
}
