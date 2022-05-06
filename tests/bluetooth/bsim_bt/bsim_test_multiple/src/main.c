/* main.c - Application main entry point */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>

#include <zephyr/zephyr.h>

#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

#include "bs_types.h"
#include "bs_tracing.h"
#include "time_machine.h"
#include "bstests.h"

#define ITERATIONS 10

int init_central(uint8_t iterations);
int init_peripheral(uint8_t iterations);

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

	err = init_central(ITERATIONS);
	if (err) {
		goto exit;
	}

	/* Wait a little so that peripheral side completes the last
	 * connection establishment.
	 */
	k_sleep(K_SECONDS(1));

	PASS("Central tests passed\n");
	bs_trace_silent_exit(0);

	return;

exit:
	FAIL("Central tests failed (%d)\n", err);
	bs_trace_silent_exit(0);
}

static void test_peripheral_main(void)
{
	int err;

	err = init_peripheral(ITERATIONS);
	if (err) {
		goto exit;
	}

	PASS("Peripheral tests passed\n");

	return;

exit:
	FAIL("Peripheral tests failed (%d)\n", err);
	bs_trace_silent_exit(0);
}

static void test_multiple_init(void)
{
	bst_ticker_set_next_tick_absolute(4500e6);
	bst_result = In_progress;
}

static void test_multiple_tick(bs_time_t HW_device_time)
{
	bst_result = Failed;
	bs_trace_error_line("Test multiple finished.\n");
}

static const struct bst_test_instance test_def[] = {
	{
		.test_id = "central",
		.test_descr = "Central Multilink",
		.test_post_init_f = test_multiple_init,
		.test_tick_f = test_multiple_tick,
		.test_main_f = test_central_main
	},
	{
		.test_id = "peripheral",
		.test_descr = "Peripheral multiple identity",
		.test_post_init_f = test_multiple_init,
		.test_tick_f = test_multiple_tick,
		.test_main_f = test_peripheral_main
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

void main(void)
{
	bst_main();
}
