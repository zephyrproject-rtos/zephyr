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

#include "babblekit/testcase.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_bsim_mtu_update, LOG_LEVEL_DBG);

#define PERIPHERAL_NOTIFY_TIME 10 /* seconds */

extern void run_peripheral_sample(uint8_t *notify_data, size_t notify_data_size, uint16_t seconds);

uint8_t notify_data[100] = {};

static void test_peripheral_main(void)
{
	notify_data[13] = 0x7f;
	notify_data[99] = 0x55;

	run_peripheral_sample(notify_data, sizeof(notify_data), PERIPHERAL_NOTIFY_TIME);

	TEST_PASS("MTU Update test passed");
}

static const struct bst_test_instance test_def[] = {
	{
		.test_id = "peripheral",
		.test_descr = "Peripheral GATT MTU Update",
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
