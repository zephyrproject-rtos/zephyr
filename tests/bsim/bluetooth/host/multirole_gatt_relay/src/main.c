/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
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

/*
 * Test topology:
 *   Device 0: Central (central_notify_receive) - receives notifications from Device 1
 *   Device 1: Multirole (central_peripheral_notify) - peripheral to Dev 0, central to Dev 2
 *   Device 2: Peripheral (peripheral_gatt_notify) - sends notifications to Device 1
 *
 * Notification flow: Device 2 -> Device 1 -> Device 0
 */

/* Number of notifications to receive/send before declaring the test passed.
 * This needs to be large enough to ensure notifications flow for at least
 * one metrics interval (1 second) on each link.
 */
#define COUNT_PERIPHERAL  5000U
#define COUNT_MULTIROLE   5000U
#define COUNT_CENTRAL     5000U

extern uint32_t peripheral_gatt_notify(uint32_t count);
extern uint32_t central_peripheral_notify(uint32_t count);
extern uint32_t central_notify_receive(uint32_t count);

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
	uint32_t rate;

	rate = central_notify_receive(COUNT_CENTRAL);

	printk("%s: Notify RX Rate = %u bps\n", __func__, rate);
	if (rate > 0U) {
		PASS("Central tests passed\n");
	} else {
		FAIL("Central tests failed\n");
	}

	/* Give extra time for other devices to finish */
	k_sleep(K_SECONDS(1));

	bs_trace_silent_exit(0);
}

static void test_multirole_main(void)
{
	uint32_t rate;

	rate = central_peripheral_notify(COUNT_MULTIROLE);

	printk("%s: Notify TX Rate = %u bps\n", __func__, rate);
	if (rate > 0U) {
		PASS("Multirole tests passed\n");
	} else {
		FAIL("Multirole tests failed\n");
	}

	/* Give extra time for other devices to finish */
	k_sleep(K_SECONDS(1));

	bs_trace_silent_exit(0);
}

static void test_peripheral_main(void)
{
	uint32_t rate;

	rate = peripheral_gatt_notify(COUNT_PERIPHERAL);

	printk("%s: Notify TX Rate = %u bps\n", __func__, rate);
	if (rate > 0U) {
		PASS("Peripheral tests passed\n");
	} else {
		FAIL("Peripheral tests failed\n");
	}
}

static void test_init(void)
{
	bst_ticker_set_next_tick_absolute(600e6);
	bst_result = In_progress;
}

static void test_tick(bs_time_t HW_device_time)
{
	bst_result = Failed;
	bs_trace_error_line("Test multirole GATT relay timed out.\n");
}

static const struct bst_test_instance test_def[] = {
	{
		.test_id = "central",
		.test_descr = "Central Notify Receive",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_central_main,
	},
	{
		.test_id = "multirole",
		.test_descr = "Multirole Notify Send Receive",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_multirole_main,
	},
	{
		.test_id = "peripheral",
		.test_descr = "Peripheral GATT Notify",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_peripheral_main,
	},
	BSTEST_END_MARKER
};

struct bst_test_list *test_multirole_gatt_relay_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_def);
}

bst_test_install_t test_installers[] = {
	test_multirole_gatt_relay_install,
	NULL
};

int main(void)
{
	bst_main();
	return 0;
}
