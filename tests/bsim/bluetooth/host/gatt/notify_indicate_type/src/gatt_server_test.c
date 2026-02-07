/*
 * Copyright (c) 2025 Andrew Leech
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

DEFINE_FLAG_STATIC(flag_is_connected);
DEFINE_FLAG_STATIC(flag_subscribed);
DEFINE_FLAG_STATIC(flag_indicated);

static struct bt_conn *g_conn;

#define ARRAY_ITEM(i, _) i
static const uint8_t chrc_data[] = { LISTIFY(CHRC_SIZE, ARRAY_ITEM, (,)) };

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

static ssize_t read_test_chrc(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			      void *buf, uint16_t len, uint16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 (void *)chrc_data, sizeof(chrc_data));
}

static void ccc_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	const bool notify = (value & BT_GATT_CCC_NOTIFY) != 0;
	const bool indicate = (value & BT_GATT_CCC_INDICATE) != 0;

	printk("CCC changed: notify=%d indicate=%d (value=0x%04x)\n", notify, indicate, value);

	if (notify && indicate) {
		SET_FLAG(flag_subscribed);
	}
}

BT_GATT_SERVICE_DEFINE(test_svc,
	BT_GATT_PRIMARY_SERVICE(TEST_SERVICE_UUID),
	BT_GATT_CHARACTERISTIC(TEST_CHRC_UUID,
			       BT_GATT_CHRC_NOTIFY | BT_GATT_CHRC_INDICATE | BT_GATT_CHRC_READ,
			       BT_GATT_PERM_READ, read_test_chrc, NULL, NULL),
	BT_GATT_CCC(ccc_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
);

static void notification_sent(struct bt_conn *conn, void *user_data)
{
	printk("Notification sent\n");
}

static void indication_confirmed(struct bt_conn *conn,
				 struct bt_gatt_indicate_params *params,
				 uint8_t err)
{
	printk("Indication confirmed (err %u)\n", err);

	if (err != 0) {
		TEST_FAIL("Indication failed (err %u)", err);
		return;
	}

	SET_FLAG(flag_indicated);
}

static void test_main(void)
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
	WAIT_FOR_FLAG(flag_subscribed);

	printk("Client subscribed, sending notification\n");

	/* Send one notification */
	static size_t notify_len = CHRC_SIZE;
	static struct bt_gatt_notify_params notify_params = {
		.attr = &attr_test_svc[1],
		.data = chrc_data,
		.len = CHRC_SIZE,
		.func = notification_sent,
		.user_data = &notify_len,
	};

	do {
		err = bt_gatt_notify_cb(g_conn, &notify_params);
		if (err == -ENOMEM) {
			k_sleep(K_MSEC(10));
		} else if (err) {
			TEST_FAIL("Notify failed (err %d)", err);
		}
	} while (err);

	/* Brief delay to let the notification reach the client before indication */
	k_sleep(K_MSEC(100));

	printk("Sending indication\n");

	/* Send one indication */
	static struct bt_gatt_indicate_params ind_params = {
		.attr = &attr_test_svc[1],
		.func = indication_confirmed,
		.data = chrc_data,
		.len = CHRC_SIZE,
	};

	do {
		err = bt_gatt_indicate(g_conn, &ind_params);
		if (err == -ENOMEM) {
			k_sleep(K_MSEC(10));
		} else if (err) {
			TEST_FAIL("Indicate failed (err %d)", err);
		}
	} while (err);

	WAIT_FOR_FLAG(flag_indicated);

	TEST_PASS("GATT server passed");
}

static const struct bst_test_instance test_gatt_server[] = {
	{
		.test_id = "gatt_server",
		.test_main_f = test_main,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_gatt_server_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_gatt_server);
}
