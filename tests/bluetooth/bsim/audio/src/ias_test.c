/*
 * Copyright (c) 2022 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#ifdef CONFIG_BT_IAS

#include <stddef.h>
#include <errno.h>
#include <zephyr/types.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>

#include <zephyr/bluetooth/services/ias.h>

#include "common.h"

extern enum bst_result_t bst_result;

CREATE_FLAG(g_high_alert_received);
CREATE_FLAG(g_mild_alert_received);
CREATE_FLAG(g_stop_alert_received);

static void high_alert_cb(void)
{
	SET_FLAG(g_high_alert_received);
}

static void mild_alert_cb(void)
{
	SET_FLAG(g_mild_alert_received);
}

static void no_alert_cb(void)
{
	SET_FLAG(g_stop_alert_received);
}

BT_IAS_CB_DEFINE(ias_callbacks) = {
	.high_alert = high_alert_cb,
	.mild_alert = mild_alert_cb,
	.no_alert = no_alert_cb,
};

static void test_main(void)
{
	int err;

	err = bt_enable(NULL);
	if (err != 0) {
		FAIL("Bluetooth init failed (err %d)\n", err);
		return;
	}

	err = bt_le_adv_start(BT_LE_ADV_CONN_NAME, ad, AD_SIZE, NULL, 0);
	if (err) {
		FAIL("Advertising failed to start (err %d)\n", err);
		return;
	}

	printk("Advertising successfully started\n");

	WAIT_FOR_FLAG(flag_connected);

	WAIT_FOR_FLAG(g_high_alert_received);
	printk("High alert received\n");

	err = bt_ias_local_alert_stop();
	if (err != 0) {
		FAIL("Failed to locally stop alert: %d\n", err);
		return;
	}
	WAIT_FOR_FLAG(g_stop_alert_received);

	WAIT_FOR_FLAG(g_mild_alert_received);
	printk("Mild alert received\n");

	WAIT_FOR_FLAG(g_stop_alert_received);
	printk("Stop alert received\n");

	PASS("IAS test passed\n");
}

static const struct bst_test_instance test_ias[] = {
	{
		.test_id = "ias",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main,

	},
	BSTEST_END_MARKER
};

struct bst_test_list *test_ias_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_ias);
}

#endif /* CONFIG_BT_IAS */
