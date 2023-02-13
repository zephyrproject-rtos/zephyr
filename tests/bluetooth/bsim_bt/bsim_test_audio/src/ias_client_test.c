/*
 * Copyright (c) 2022 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#ifdef CONFIG_BT_IAS_CLIENT

#include "zephyr/bluetooth/services/ias.h"
#include "common.h"

extern enum bst_result_t bst_result;

CREATE_FLAG(g_is_connected);
CREATE_FLAG(g_service_discovered);

static struct bt_conn *g_conn;

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

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err > 0) {
		char addr[BT_ADDR_LE_STR_LEN];

		bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

		FAIL("Failed to connect to %s (err %u)\n", addr, err);
		return;
	}

	g_conn = conn;
	SET_FLAG(g_is_connected);
}

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

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

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

	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, device_found);
	if (err < 0) {
		FAIL("Scanning failed to start (err %d)\n", err);
		return;
	}

	printk("Scanning successfully started\n");

	WAIT_FOR_FLAG(g_is_connected);

	err = bt_ias_discover(g_conn);
	if (err < 0) {
		FAIL("Failed to discover IAS (err %d)\n", err);
		return;
	}

	WAIT_FOR_FLAG(g_service_discovered);

	test_alert_high(g_conn);
	test_alert_mild(g_conn);
	test_alert_stop(g_conn);

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
