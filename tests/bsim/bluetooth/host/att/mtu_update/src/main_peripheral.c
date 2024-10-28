/*
 * Copyright (c) 2023 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/kernel.h>

#include <zephyr/sys/printk.h>

#include "common.h"
#include "time_machine.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_bsim_mtu_update, LOG_LEVEL_DBG);

extern void run_peripheral_sample(uint8_t *notify_data, size_t notify_data_size, uint16_t seconds);

uint8_t notify_data[100] = {};

static void test_peripheral_main(void)
{
	notify_data[13] = 0x7f;
	notify_data[99] = 0x55;

	run_peripheral_sample(notify_data, sizeof(notify_data), PERIPHERAL_NOTIFY_TIME);

	PASS("MTU Update test passed\n");
}

void test_tick(bs_time_t HW_device_time)
{
	if (bst_result != Passed) {
		FAIL("Test failed (not passed after %i seconds)\n", WAIT_TIME);
	}
}

static void test_mtu_update_init(void)
{
	bst_ticker_set_next_tick_absolute(WAIT_TIME);
	bst_result = In_progress;
}

static const struct bst_test_instance test_def[] = {
	{
		.test_id = "peripheral",
		.test_descr = "Peripheral GATT MTU Update",
		.test_pre_init_f = test_mtu_update_init,
		.test_tick_f = test_tick,
		.test_main_f = test_peripheral_main
	},
	BSTEST_END_MARKER
};

struct bst_test_list *test_mtu_update_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_def);
}

bst_test_install_t test_installers[] = {
	test_mtu_update_install,
	NULL
};

int main(void)
{
	bst_main();
	return 0;
}
