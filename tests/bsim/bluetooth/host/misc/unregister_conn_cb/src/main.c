/*
 * The goal of this test is to verify the bt_conn_cb_unregister() API works as expected
 *
 * Copyright (c) 2024 NXP
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>

#include <testlib/conn.h>
#include "babblekit/testcase.h"
#include "babblekit/flags.h"

DEFINE_FLAG_STATIC(flag_is_connected);

static struct bt_conn *g_conn;

static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err != 0) {
		TEST_FAIL("Failed to connect to %s (%u)", addr, err);
		return;
	}

	printk("conn_callback:Connected to %s\n", addr);

	__ASSERT_NO_MSG(g_conn == NULL);
	g_conn = bt_conn_ref(conn);
	SET_FLAG(flag_is_connected);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	if (conn != g_conn) {
		return;
	}

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("conn_callback:Disconnected: %s (reason 0x%02x)\n", addr, reason);

	bt_conn_unref(g_conn);
	g_conn = NULL;

	UNSET_FLAG(flag_is_connected);
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
};

void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type, struct net_buf_simple *ad)
{
	char addr_str[BT_ADDR_LE_STR_LEN];
	int err;
	struct bt_conn *conn;

	if (g_conn != NULL) {
		printk("g_conn != NULL\n");
		return;
	}

	/* We're only interested in connectable events */
	if (type != BT_HCI_ADV_IND && type != BT_HCI_ADV_DIRECT_IND) {
		printk("type not connectable\n");
		return;
	}

	bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));
	printk("Device found: %s (RSSI %d)\n", addr_str, rssi);

	printk("Stopping scan\n");
	err = bt_le_scan_stop();
	if (err != 0) {
		TEST_FAIL("Could not stop scan: %d");
		return;
	}

	err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN, BT_LE_CONN_PARAM_DEFAULT, &conn);
	if (err != 0) {
		TEST_FAIL("Could not connect to peer: %d", err);
	}
	printk("%s: connected to found device\n", __func__);

	bt_conn_unref(conn);
}

static void connection_info(struct bt_conn *conn, void *user_data)
{
	char addr[BT_ADDR_LE_STR_LEN];
	int *conn_count = user_data;
	struct bt_conn_info info;

	if (bt_conn_get_info(conn, &info) < 0) {
		printk("Unable to get info: conn %p", conn);
		return;
	}

	switch (info.type) {
	case BT_CONN_TYPE_LE:
		(*conn_count)++;
		bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
		printk("%s: Connected to %s\n", __func__, addr);
		break;
	default:
		break;
	}
}

static void start_adv(void)
{
	int err;
	const struct bt_data ad[] = {
		BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR))};

	err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err != 0) {
		TEST_FAIL("Advertising failed to start (err %d)", err);
		return;
	}

	printk("Advertising successfully started\n");
}

static void test_peripheral_main(void)
{
	int err;

	err = bt_enable(NULL);
	if (err != 0) {
		TEST_FAIL("Bluetooth init failed (err %d)", err);
		return;
	}

	printk("Bluetooth initialized\n");

	bt_conn_cb_register(&conn_callbacks);

	start_adv();

	WAIT_FOR_FLAG(flag_is_connected);

	WAIT_FOR_FLAG_UNSET(flag_is_connected);

	bt_conn_cb_unregister(&conn_callbacks);

	bt_testlib_conn_wait_free();
	start_adv();

	k_sleep(K_SECONDS(1));

	err = bt_disable();
	if (err != 0) {
		TEST_FAIL("Bluetooth disable failed (err %d)", err);
		return;
	}

	printk("Bluetooth successfully disabled\n");

	TEST_PASS("Peripheral device passed");
}

static void test_central_main(void)
{
	int err;
	int conn_count = 0;

	err = bt_enable(NULL);
	if (err != 0) {
		TEST_FAIL("Bluetooth discover failed (err %d)", err);
	}
	printk("Bluetooth initialized\n");
	bt_conn_cb_register(&conn_callbacks);
	/* Connect to peer device after conn_callbacks registered*/
	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, device_found);
	if (err != 0) {
		TEST_FAIL("Scanning failed to start (err %d)", err);
	}

	printk("Scanning successfully started\n");

	WAIT_FOR_FLAG(flag_is_connected);

	err = bt_conn_disconnect(g_conn, 0x13);

	if (err != 0) {
		TEST_FAIL("Disconnect failed (err %d)", err);
		return;
	}

	WAIT_FOR_FLAG_UNSET(flag_is_connected);
	bt_conn_cb_unregister(&conn_callbacks);
	/* Reconnect to the device after conn_callbacks unregistered */
	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, device_found);
	if (err != 0) {
		TEST_FAIL("Scanning failed to start (err %d)", err);
	}
	printk("Scanning successfully started\n");

	k_sleep(K_SECONDS(1));
	bt_conn_foreach(BT_CONN_TYPE_LE, connection_info, &conn_count);
	if (!conn_count) {
		TEST_FAIL("Reconnect to peer device failed!");
	}

	/* flag_is_connected not set means no conn_callbacks being called */
	if (flag_is_connected) {
		TEST_FAIL("Unregister conn_callback didn't work");
	}
	printk("Unregister connection callbacks succeed!\n");

	err = bt_disable();
	if (err != 0) {
		TEST_FAIL("Bluetooth disable failed (err %d)", err);
	}
	printk("Bluetooth successfully disabled\n");

	TEST_PASS("Central device passed");
}

static const struct bst_test_instance test_def[] = {{.test_id = "peripheral",
						     .test_descr = "Peripheral device",
						     .test_main_f = test_peripheral_main},
						    {.test_id = "central",
						     .test_descr = "Central device",
						     .test_main_f = test_central_main},
						    BSTEST_END_MARKER};

struct bst_test_list *test_unregister_conn_cb_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_def);
}

extern struct bst_test_list *test_unregister_conn_cb_install(struct bst_test_list *tests);

bst_test_install_t test_installers[] = {test_unregister_conn_cb_install, NULL};

int main(void)
{
	bst_main();
	return 0;
}
