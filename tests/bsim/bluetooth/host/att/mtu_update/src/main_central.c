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

#include <zephyr/bluetooth/gatt.h>

#include "babblekit/testcase.h"
#include "babblekit/flags.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_bsim_mtu_update, LOG_LEVEL_DBG);

extern void run_central_sample(void *cb);

DEFINE_FLAG_STATIC(flag_notification_received);
uint8_t notify_data[100] = {};
uint8_t is_data_equal;

static uint8_t notify_cb(struct bt_conn *conn, struct bt_gatt_subscribe_params *params,
			 const void *data, uint16_t length)
{
	printk("BSIM NOTIFY_CALLBACK\n");

	is_data_equal = (length == sizeof(notify_data) && !memcmp(notify_data, data, length));

	LOG_HEXDUMP_DBG(data, length, "notification data");
	LOG_HEXDUMP_DBG(notify_data, sizeof(notify_data), "expected data");

	SET_FLAG(flag_notification_received);

	return 0;
}

static void test_central_main(void)
{
	notify_data[13] = 0x7f;
	notify_data[99] = 0x55;

	run_central_sample(notify_cb);

	WAIT_FOR_FLAG(flag_notification_received);

	if (is_data_equal) {
		TEST_PASS("MTU Update test passed");
	} else {
		TEST_FAIL("MTU Update test failed");
	}
}

static const struct bst_test_instance test_def[] = {
	{
		.test_id = "central",
		.test_descr = "Central GATT MTU Update",
		.test_main_f = test_central_main
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
