/* main.c - Application main entry point */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>

#include <zephyr/kernel.h>

#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

#include "bs_types.h"
#include "bs_tracing.h"
#include "time_machine.h"
#include "bstests.h"

/* The test case is performing 250 simultaneous connections and managing
 * parallel control procedures utilizing the available/configured minimum
 * buffer counts. Hence, two iterations of connect-disconnect should be
 * sufficient to catch any regressions/buffer leaks.
 */
#define ITERATIONS 2

int init_central(uint8_t max_conn, uint8_t iterations);
int init_peripheral(uint8_t max_conn, uint8_t iterations);

#define FAIL(...)					\
	do {						\
		bst_result = Failed;			\
		bs_trace_error_time_line(__VA_ARGS__);	\
	} while (0)

#define PASS(...)					\
	do {						\
		bst_result = Passed;			\
		bs_trace_info_time(1, __VA_ARGS__);	\
	} while (0)

extern enum bst_result_t bst_result;

static void test_central_main(void)
{
	int err;

	err = init_central(CONFIG_BT_MAX_CONN, ITERATIONS);
	if (err) {
		goto exit;
	}

	/* Wait a little so that peripheral side completes the last
	 * connection establishment.
	 */
	k_sleep(K_SECONDS(1));

	PASS("Central tests passed\n");

	return;

exit:
	FAIL("Central tests failed (%d)\n", err);
	bs_trace_silent_exit(0);
}

static void test_peripheral_main(void)
{
	int err;

	err = init_peripheral(CONFIG_BT_MAX_CONN, ITERATIONS);
	if (err) {
		goto exit;
	}

	PASS("Peripheral tests passed\n");

	return;

exit:
	FAIL("Peripheral tests failed (%d)\n", err);
	bs_trace_silent_exit(0);
}

static void test_central_multiple_main(void)
{
	int err;

	err = init_central(20U, ITERATIONS);
	if (err) {
		goto exit;
	}

	/* Wait a little so that peripheral side completes the last
	 * connection establishment.
	 */
	k_sleep(K_SECONDS(1));

	PASS("Central tests passed\n");

	return;

exit:
	FAIL("Central tests failed (%d)\n", err);
	bs_trace_silent_exit(0);
}

static void test_peripheral_single_main(void)
{
	int err;

	err = init_peripheral(1U, ITERATIONS);
	if (err) {
		goto exit;
	}

	PASS("Peripheral tests passed\n");

	return;

exit:
	FAIL("Peripheral tests failed (%d)\n", err);
	bs_trace_silent_exit(0);
}

static void test_central_single_main(void)
{
	int err;

	err = init_central(1U, ITERATIONS);
	if (err) {
		goto exit;
	}

	PASS("Central tests passed\n");

	return;

exit:
	FAIL("Central tests failed (%d)\n", err);
	bs_trace_silent_exit(0);
}

static void test_peripheral_multilink_main(void)
{
	int err;

	err = init_peripheral(20U, ITERATIONS);
	if (err) {
		goto exit;
	}

	k_sleep(K_SECONDS(3));

	PASS("Peripheral tests passed\n");

	return;

exit:
	FAIL("Peripheral tests failed (%d)\n", err);
	bs_trace_silent_exit(0);
}

static void test_multiple_init(void)
{
	bst_ticker_set_next_tick_absolute(2400e6);
	bst_result = In_progress;
}

static void test_multiple_tick(bs_time_t HW_device_time)
{
	if (bst_result != Passed) {
		FAIL("Test timeout (not passed after %lu seconds)",
		     (unsigned long)(HW_device_time / USEC_PER_SEC));
	}

	bs_trace_silent_exit(0);
}

static const struct bst_test_instance test_def[] = {
	{
		.test_id = "central",
		.test_descr = "Central Multilink",
		.test_pre_init_f = test_multiple_init,
		.test_tick_f = test_multiple_tick,
		.test_main_f = test_central_main
	},
	{
		.test_id = "peripheral",
		.test_descr = "Peripheral multiple identity",
		.test_pre_init_f = test_multiple_init,
		.test_tick_f = test_multiple_tick,
		.test_main_f = test_peripheral_main
	},
	{
		.test_id = "central_multiple",
		.test_descr = "Single Central Multilink device",
		.test_pre_init_f = test_multiple_init,
		.test_tick_f = test_multiple_tick,
		.test_main_f = test_central_multiple_main
	},
	{
		.test_id = "peripheral_single",
		.test_descr = "Many Peripheral single link device",
		.test_pre_init_f = test_multiple_init,
		.test_tick_f = test_multiple_tick,
		.test_main_f = test_peripheral_single_main
	},
	{
		.test_id = "central_single",
		.test_descr = "Single Central device",
		.test_pre_init_f = test_multiple_init,
		.test_tick_f = test_multiple_tick,
		.test_main_f = test_central_single_main
	},
	{
		.test_id = "peripheral_multilink",
		.test_descr = "Peripheral multilink device",
		.test_pre_init_f = test_multiple_init,
		.test_tick_f = test_multiple_tick,
		.test_main_f = test_peripheral_multilink_main
	},
	BSTEST_END_MARKER
};

struct bst_test_list *test_multiple_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_def);
}

bst_test_install_t test_installers[] = {
	test_multiple_install,
	NULL
};

int main(void)
{
	bst_main();
	return 0;
}
