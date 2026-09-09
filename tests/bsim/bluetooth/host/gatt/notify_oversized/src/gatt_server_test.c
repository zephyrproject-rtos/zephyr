/*
 * Copyright (c) 2026 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

#include "babblekit/flags.h"
#include "babblekit/testcase.h"

#include "common.h"

DEFINE_FLAG_STATIC(flag_is_connected);
DEFINE_FLAG_STATIC(flag_mtu_updated);
DEFINE_FLAG_STATIC(flag_a_subscribed);
DEFINE_FLAG_STATIC(flag_b_subscribed);
DEFINE_FLAG_STATIC(flag_sent);
DEFINE_FLAG_STATIC(flag_confirmed);

static struct bt_conn *g_conn;
static uint8_t test_data[OVERSIZED_LEN];

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err != 0) {
		TEST_FAIL("Failed to connect to %s (%u)", bt_conn_dst_str(conn), err);
		return;
	}

	printk("Connected to %s\n", bt_conn_dst_str(conn));

	g_conn = bt_conn_ref(conn);
	SET_FLAG(flag_is_connected);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	if (conn != g_conn) {
		return;
	}

	printk("Disconnected: %s (reason 0x%02x)\n", bt_conn_dst_str(conn), reason);

	bt_conn_drop(&g_conn);
	UNSET_FLAG(flag_is_connected);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

static void att_mtu_updated(struct bt_conn *conn, uint16_t tx, uint16_t rx)
{
	printk("ATT MTU updated: tx %u rx %u\n", tx, rx);

	if (tx >= REQUIRED_ATT_MTU) {
		SET_FLAG(flag_mtu_updated);
	}
}

static struct bt_gatt_cb gatt_callbacks = {
	.att_mtu_updated = att_mtu_updated,
};

static void a_ccc_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	if (value == (BT_GATT_CCC_NOTIFY | BT_GATT_CCC_INDICATE)) {
		SET_FLAG(flag_a_subscribed);
	}
}

static void b_ccc_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	if (value == BT_GATT_CCC_NOTIFY) {
		SET_FLAG(flag_b_subscribed);
	}
}

BT_GATT_SERVICE_DEFINE(test_svc, BT_GATT_PRIMARY_SERVICE(TEST_SERVICE_UUID),
		       BT_GATT_CHARACTERISTIC(TEST_CHRC_A_UUID,
					      BT_GATT_CHRC_NOTIFY | BT_GATT_CHRC_INDICATE,
					      BT_GATT_PERM_NONE, NULL, NULL, NULL),
		       BT_GATT_CCC(a_ccc_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
		       BT_GATT_CHARACTERISTIC(TEST_CHRC_B_UUID, BT_GATT_CHRC_NOTIFY,
					      BT_GATT_PERM_NONE, NULL, NULL, NULL),
		       BT_GATT_CCC(b_ccc_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE));

static void notify_sent(struct bt_conn *conn, void *user_data)
{
	SET_FLAG(flag_sent);
}

static void notify(const struct bt_gatt_attr *attr, uint16_t len)
{
	struct bt_gatt_notify_params params = {
		.attr = attr,
		.data = test_data,
		.len = len,
		.func = notify_sent,
	};
	int err;

	UNSET_FLAG(flag_sent);

	do {
		err = bt_gatt_notify_cb(g_conn, &params);
		if (err == -ENOMEM) {
			k_sleep(K_MSEC(10));
		}
	} while (err == -ENOMEM);

	TEST_ASSERT(err == 0, "Failed to send %u byte notification (err %d)", len, err);
}

static void indicate_cb(struct bt_conn *conn, struct bt_gatt_indicate_params *params, uint8_t err)
{
	TEST_ASSERT(err == 0, "%u byte indication failed (err %u)", params->len, err);

	SET_FLAG(flag_confirmed);
}

static void indicate(const struct bt_gatt_attr *attr, uint16_t len)
{
	static struct bt_gatt_indicate_params params;
	int err;

	params.attr = attr;
	params.func = indicate_cb;
	params.data = test_data;
	params.len = len;

	UNSET_FLAG(flag_confirmed);

	err = bt_gatt_indicate(g_conn, &params);
	TEST_ASSERT(err == 0, "Failed to send %u byte indication (err %d)", len, err);

	WAIT_FOR_FLAG(flag_confirmed);
	printk("%u byte indication confirmed\n", len);
}

static void notify_multiple(const struct bt_gatt_attr *attr_a, const struct bt_gatt_attr *attr_b)
{
	struct bt_gatt_notify_params params[] = {
		{ .attr = attr_a, .data = test_data, .len = OVERSIZED_LEN },
		{ .attr = attr_b, .data = test_data, .len = SHORT_LEN },
	};
	int err;

	do {
		err = bt_gatt_notify_multiple(g_conn, ARRAY_SIZE(params), params);
		if (err == -ENOMEM) {
			k_sleep(K_MSEC(10));
		}
	} while (err == -ENOMEM);

	TEST_ASSERT(err == 0, "Failed to send multiple handle value notification (err %d)", err);
}

static void test_main(void)
{
	const struct bt_data ad[] = {
		BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	};
	const struct bt_gatt_attr *attr_a;
	const struct bt_gatt_attr *attr_b;
	int err;

	ARRAY_FOR_EACH(test_data, i) {
		test_data[i] = (uint8_t)i;
	}

	bt_gatt_cb_register(&gatt_callbacks);

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Bluetooth init failed (err %d)", err);

	err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, ad, ARRAY_SIZE(ad), NULL, 0);
	TEST_ASSERT(err == 0, "Advertising failed to start (err %d)", err);

	WAIT_FOR_FLAG(flag_is_connected);
	WAIT_FOR_FLAG(flag_mtu_updated);
	WAIT_FOR_FLAG(flag_a_subscribed);
	WAIT_FOR_FLAG(flag_b_subscribed);

	attr_a = bt_gatt_find_by_uuid(NULL, 0, TEST_CHRC_A_UUID);
	attr_b = bt_gatt_find_by_uuid(NULL, 0, TEST_CHRC_B_UUID);
	TEST_ASSERT(attr_a != NULL && attr_b != NULL, "Test characteristics not found");

	/* The client must discard the oversized values and deliver the rest in
	 * this order. Wait for the notifications to be sent so that the
	 * indications do not overtake them.
	 */
	notify(attr_a, OVERSIZED_LEN);
	notify(attr_a, MAX_LEN);
	WAIT_FOR_FLAG(flag_sent);
	indicate(attr_a, OVERSIZED_LEN);
	indicate(attr_a, MAX_LEN);
	notify_multiple(attr_a, attr_b);
	notify(attr_b, MARKER_LEN);

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
