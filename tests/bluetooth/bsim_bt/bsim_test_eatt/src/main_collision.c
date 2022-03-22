/* main_collision.c - Application main entry point */

/*
 * Copyright (c) 2022 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>

#include <zephyr/types.h>
#include <sys/printk.h>
#include <sys/util.h>
#include <sys/byteorder.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/att.h>

#include "bs_types.h"
#include "bs_tracing.h"
#include "bstests.h"

extern enum bst_result_t bst_result;

#define FAIL(...)                                                                                  \
	do {                                                                                       \
		bst_result = Failed;                                                               \
		bs_trace_error_time_line(__VA_ARGS__);                                             \
	} while (0)

#define PASS(...)                                                                                  \
	do {                                                                                       \
		bst_result = Passed;                                                               \
		bs_trace_info_time(1, __VA_ARGS__);                                                \
	} while (0)

static struct bt_conn *default_conn;

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
};

static bool volatile is_connected;

static void connected(struct bt_conn *conn, uint8_t conn_err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (conn_err) {
		if (default_conn) {
			bt_conn_unref(default_conn);
			default_conn = NULL;
		}

		FAIL("Failed to connect to %s (%u)\n", addr, conn_err);
		return;
	}

	if (!default_conn) {
		default_conn = bt_conn_ref(conn);
	}

	printk("Connected: %s\n", addr);
	is_connected = true;
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Disconnected: %s (reason 0x%02x)\n", addr, reason);

	bt_conn_unref(default_conn);
	default_conn = NULL;
	is_connected = false;
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

static void test_peripheral_main(void)
{
	int err;
	size_t num_enhanced;

	err = bt_enable(NULL);
	if (err) {
		FAIL("Can't enable Bluetooth (err %d)\n", err);
	}

	err = bt_le_adv_start(BT_LE_ADV_CONN_NAME, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err) {
		FAIL("Advertising failed to start (err %d)\n", err);
	}

	while (!is_connected) {
		k_sleep(K_MSEC(100));
	}

	err = bt_eatt_connect(default_conn, CONFIG_BT_EATT_MAX);
	if (err) {
		FAIL("Sending credit based connection request failed (err %d)\n", err);
	}

	/* TODO: Use a flag set by a callback from EATT connection instead of sleeping */
	/* Wait for EATT channels to be connected */
	k_sleep(K_MSEC(10000));
	num_enhanced = bt_eatt_count(default_conn);
	printk("%d enhanced bearers connected\n", num_enhanced);
	if (num_enhanced != CONFIG_BT_EATT_MAX) {
		FAIL("Expected %d enhanced bearers, got %d\n", CONFIG_BT_EATT_MAX, num_enhanced);
	}
	/* Disconnect */
	err = bt_conn_disconnect(default_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	if (err) {
		FAIL("Disconnection failed (err %d)\n", err);
	}

	while (is_connected) {
		k_sleep(K_MSEC(100));
	}

	PASS("EATT Peripheral tests Passed\n");
}

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			 struct net_buf_simple *ad)
{
	int err;

	err = bt_le_scan_stop();
	if (err) {
		FAIL("Stop LE scan failed (err %d)\n", err);
	}

	err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN, BT_LE_CONN_PARAM_DEFAULT,
				&default_conn);
	if (err) {
		FAIL("Create conn failed (err %d)\n", err);
	}

	printk("Device connected\n");
}

static void test_central_main(void)
{
	int err;
	size_t num_enhanced;

	err = bt_enable(NULL);
	if (err) {
		FAIL("Can't enable Bluetooth (err %d)\n", err);
	}

	err = bt_le_scan_start(BT_LE_SCAN_ACTIVE, device_found);
	if (err) {
		FAIL("Scanning failed to start (err %d)\n", err);
	}

	while (!is_connected) {
		k_sleep(K_MSEC(100));
	}

	err = bt_eatt_connect(default_conn, CONFIG_BT_EATT_MAX);
	if (err) {
		FAIL("Sending credit based connection request failed (err %d)\n", err);
	}

	/* TODO: Use a flag set by a callback from EATT connection instead of sleeping */
	/* Wait for EATT channels to be connected */
	k_sleep(K_MSEC(10000));
	num_enhanced = bt_eatt_count(default_conn);
	printk("%d enhanced bearers connected\n", num_enhanced);
	if (num_enhanced != CONFIG_BT_EATT_MAX) {
		FAIL("Expected %d enhanced bearers, got %d\n", CONFIG_BT_EATT_MAX, num_enhanced);
	}

	/* Wait for disconnect */
	while (is_connected) {
		k_sleep(K_MSEC(100));
	}

	PASS("EATT Central tests Passed\n");
}

static void test_init(void)
{
	bst_ticker_set_next_tick_absolute(60e6); /* 60 seconds */
	bst_result = In_progress;
}

static void test_tick(bs_time_t HW_device_time)
{
	if (bst_result != Passed) {
		bst_result = Failed;
		bs_trace_error_time_line("Test eatt finished.\n");
	}
}

static const struct bst_test_instance test_def[] = {
	{
		.test_id = "peripheral",
		.test_descr = "Peripheral Collision",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_peripheral_main
	},
	{
		.test_id = "central",
		.test_descr = "Central Collision",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_central_main
	},
	BSTEST_END_MARKER
};

struct bst_test_list *test_main_collision_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_def);
}
