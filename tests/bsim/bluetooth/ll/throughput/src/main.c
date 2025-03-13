/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
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

/* There are 13 iterations of PHY update every 3 seconds, and based on actual
 * simulation 10000 iterations are sufficient to finish these iterations with
 * a stable 2M throughput value to be verified. If Central and Peripheral take
 * different duration to complete these iterations, the test will fail due to
 * the throughput calculated over one second duration will be low due to the
 * connection being disconnected before the other device could complete all the
 * iterations.
 */
#define COUNT 10000

/* Write Throughput calculation:
 *  Measure interval = 1 s
 *  Connection interval = 50 ms
 *  No. of connection intervals = 20
 *  Single Tx time, 2M PHY = 1064 us
 *  tIFS = 150 us
 *  Single Tx duration = 1214 us
 *  Full duplex Tx-Rx duration = 2428 us
 *  Implementation dependent event overhead = 340 us
 *  Max. incomplete PDU time = 1064 us
 *  Max. radio idle time per 1 second = (1064 + 340) * 20 = 28080 us
 *  Packets per 1 second = (1000000 - 28080) / 2428 = 400.297
 *  GATT Write data length = 244 bytes
 *  Throughput = 400 * 244 * 8 = 780800 bps
 */
#define WRITE_RATE 780800 /* GATT Write bps recorded in this test */

extern uint32_t central_gatt_write(uint32_t count);
extern uint32_t peripheral_gatt_write(uint32_t count);

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
	uint32_t write_rate;

	write_rate = central_gatt_write(COUNT);

	printk("%s: Write Rate = %u bps\n", __func__, write_rate);
	if (write_rate == WRITE_RATE) {
		PASS("Central tests passed\n");
	} else {
		FAIL("Central tests failed\n");
	}

	/* Give extra time for peripheral side to finish its iterations */
	k_sleep(K_SECONDS(1));

	bs_trace_silent_exit(0);
}

static void test_peripheral_main(void)
{
	uint32_t write_rate;

	write_rate = peripheral_gatt_write(COUNT);

	printk("%s: Write Rate = %u bps\n", __func__, write_rate);
	if (write_rate == WRITE_RATE) {
		PASS("Peripheral tests passed\n");
	} else {
		FAIL("Peripheral tests failed\n");
	}
}

static void test_gatt_write_init(void)
{
	bst_ticker_set_next_tick_absolute(60e6);
	bst_result = In_progress;
}

static void test_gatt_write_tick(bs_time_t HW_device_time)
{
	bst_result = Failed;
	bs_trace_error_line("Test GATT Write finished.\n");
}

static const struct bst_test_instance test_def[] = {
	{
		.test_id = "central",
		.test_descr = "Central GATT Write",
		.test_pre_init_f = test_gatt_write_init,
		.test_tick_f = test_gatt_write_tick,
		.test_main_f = test_central_main
	},
	{
		.test_id = "peripheral",
		.test_descr = "Peripheral GATT Write",
		.test_pre_init_f = test_gatt_write_init,
		.test_tick_f = test_gatt_write_tick,
		.test_main_f = test_peripheral_main
	},
	BSTEST_END_MARKER
};

struct bst_test_list *test_gatt_write_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_def);
}

bst_test_install_t test_installers[] = {
	test_gatt_write_install,
	NULL
};

int main(void)
{
	bst_main();
	return 0;
}
