/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
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
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>

#include "babblekit/testcase.h"
#include "babblekit/flags.h"
#include "common.h"

extern enum bst_result_t bst_result;

DEFINE_FLAG_STATIC(flag_is_connected);
DEFINE_FLAG_STATIC(flag_subscribe);

static struct bt_conn *g_conn;

#define ARRAY_ITEM(i, _) i
const uint8_t chrc_data[] = { LISTIFY(CHRC_SIZE, ARRAY_ITEM, (,)) }; /* 1, 2, 3 ... */

static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err != 0) {
		TEST_FAIL("Failed to connect to %s (%u)", addr, err);
		return;
	}

	printk("Connected to %s\n", addr);

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

	printk("Disconnected: %s (reason 0x%02x)\n", addr, reason);

	bt_conn_unref(g_conn);

	g_conn = NULL;
	UNSET_FLAG(flag_is_connected);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

static ssize_t read_test_chrc(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
				   uint16_t len, uint16_t offset)
{
	printk("Read char\n");
	return bt_gatt_attr_read(conn, attr, buf, len, offset, (void *)chrc_data,
				 sizeof(chrc_data));
}

static void subscribe(const struct bt_gatt_attr *attr, uint16_t value)
{
	const bool notif_enabled = (value == BT_GATT_CCC_NOTIFY);

	if (notif_enabled) {
		SET_FLAG(flag_subscribe);
	}

	printk("Notifications %s\n", notif_enabled ? "enabled" : "disabled");
}

BT_GATT_SERVICE_DEFINE(test_svc, BT_GATT_PRIMARY_SERVICE(TEST_SERVICE_UUID),
		       BT_GATT_CHARACTERISTIC(TEST_CHRC_UUID,
					      BT_GATT_CHRC_NOTIFY | BT_GATT_CHRC_READ,
					      BT_GATT_PERM_READ, read_test_chrc, NULL, NULL),
		       BT_GATT_CCC(subscribe, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE));

static volatile size_t num_notifications_sent;

static void notification_sent(struct bt_conn *conn, void *user_data)
{
	size_t *length = user_data;

	printk("Sent notification #%u with length %d\n", num_notifications_sent++, *length);
}

static void notify(void)
{
	static size_t length = CHRC_SIZE;
	static struct bt_gatt_notify_params params = {
		.attr = &attr_test_svc[1],
		.data = chrc_data,
		.len = CHRC_SIZE,
		.func = notification_sent,
		.user_data = &length,
		.uuid = NULL,
		.chan_opt = BT_ATT_CHAN_OPT_ENHANCED_ONLY,
	};
	int err;

	do {
		err = bt_gatt_notify_cb(g_conn, &params);

		if (err == -ENOMEM) {
			/* The delay is needed to schedule other threads, but keep it as short as
			 * possible.
			 */
			k_sleep(K_MSEC(1));
		} else if (err) {
			TEST_FAIL("Notify failed (err %d)", err);
		}
	} while (err);
}

static void setup(void)
{
	int err;
	const struct bt_data ad[] = {
		BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	};

	err = bt_enable(NULL);
	if (err != 0) {
		TEST_FAIL("Bluetooth init failed (err %d)", err);
		return;
	}

	printk("Bluetooth initialized\n");

	err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err != 0) {
		TEST_FAIL("Advertising failed to start (err %d)", err);
		return;
	}

	printk("Advertising successfully started\n");

	WAIT_FOR_FLAG(flag_is_connected);

	while (bt_eatt_count(g_conn) < CONFIG_BT_EATT_MAX) {
		k_sleep(K_MSEC(100));
	}
	printk("EATT connected\n");

	WAIT_FOR_FLAG(flag_subscribe);
}

static void test_main_server(void)
{
	setup();

	for (int i = 0; i < NOTIFICATION_COUNT; i++) {
		notify();
	}

	while (num_notifications_sent < NOTIFICATION_COUNT) {
		k_sleep(K_MSEC(100));
	}

	TEST_PASS("GATT server passed");
}

static const struct bst_test_instance test_gatt_server[] = {
	{
		.test_id = "gatt_server_enhanced_notif_stress",
		.test_main_f = test_main_server,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_gatt_server_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_gatt_server);
}
