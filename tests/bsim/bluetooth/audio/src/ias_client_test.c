/*
 * Copyright (c) 2022 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#ifdef CONFIG_BT_IAS_CLIENT

#include <zephyr/bluetooth/services/ias.h>
#include "common.h"

extern enum bst_result_t bst_result;

CREATE_FLAG(g_service_discovered);

static void discover_cb(struct bt_conn *conn, int err)
{
	if (err) {
		FAIL("Failed to discover IAS (err %d)\n", err);
		return;
	}

	printk("IAS discovered\n");
	SET_FLAG(g_service_discovered);
}

static const struct bt_ias_client_cb ias_client_cb = {
	.discover = discover_cb,
};

static void test_alert_high(struct bt_conn *conn)
{
	int err;

	err = bt_ias_client_alert_write(conn, BT_IAS_ALERT_LVL_HIGH_ALERT);
	if (err == 0) {
		printk("High alert sent\n");
	} else {
		FAIL("Failed to send high alert\n");
	}
}

static void test_alert_mild(struct bt_conn *conn)
{
	int err;

	err = bt_ias_client_alert_write(conn, BT_IAS_ALERT_LVL_MILD_ALERT);
	if (err == 0) {
		printk("Mild alert sent\n");
	} else {
		FAIL("Failed to send mild alert\n");
	}
}

static void test_alert_stop(struct bt_conn *conn)
{
	int err;

	err = bt_ias_client_alert_write(conn, BT_IAS_ALERT_LVL_NO_ALERT);
	if (err == 0) {
		printk("Stop alert sent\n");
	} else {
		FAIL("Failed to send no alert\n");
	}
}

static void discover_ias(void)
{
	int err;

	UNSET_FLAG(g_service_discovered);

	err = bt_ias_discover(default_conn);
	if (err < 0) {
		FAIL("Failed to discover IAS (err %d)\n", err);
		return;
	}

	WAIT_FOR_FLAG(g_service_discovered);
}

static void test_main(void)
{
	int err;

	err = bt_enable(NULL);
	if (err < 0) {
		FAIL("Bluetooth discover failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	err = bt_ias_client_cb_register(&ias_client_cb);
	if (err < 0) {
		FAIL("Failed to register callbacks (err %d)\n", err);
		return;
	}

	bt_le_scan_cb_register(&common_scan_cb);

	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, NULL);
	if (err < 0) {
		FAIL("Scanning failed to start (err %d)\n", err);
		return;
	}

	printk("Scanning successfully started\n");

	WAIT_FOR_FLAG(flag_connected);

	discover_ias();
	discover_ias(); /* test that we can discover twice */

	/* Set alert levels with a delay to let the server handle any changes it want */
	test_alert_high(default_conn);
	k_sleep(K_SECONDS(1));

	test_alert_mild(default_conn);
	k_sleep(K_SECONDS(1));

	test_alert_stop(default_conn);
	k_sleep(K_SECONDS(1));

	PASS("IAS client PASS\n");
}

static const struct bst_test_instance test_ias[] = {
	{
		.test_id = "ias_client",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main,
	},
	BSTEST_END_MARKER
};

struct bst_test_list *test_ias_client_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_ias);
}
#else
struct bst_test_list *test_ias_client_install(struct bst_test_list *tests)
{
	return tests;
}

#endif /* CONFIG_BT_IAS_CLIENT */
