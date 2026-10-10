/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include <zephyr/types.h>
#include <stddef.h>
#include <errno.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>

#include "babblekit/testcase.h"
#include "babblekit/flags.h"

#include "common.h"

DEFINE_FLAG_STATIC(flag_is_connected);
DEFINE_FLAG_STATIC(flag_short_subscribe);
DEFINE_FLAG_STATIC(flag_long_subscribe);
DEFINE_FLAG_STATIC(flag_batch_sent);

static struct bt_conn *g_conn;

#define ARRAY_ITEM(i, _) i
const uint8_t chrc_data[] = { LISTIFY(CHRC_SIZE, ARRAY_ITEM, (,)) }; /* 1, 2, 3 ... */
const uint8_t long_chrc_data[] = { LISTIFY(LONG_CHRC_SIZE, ARRAY_ITEM, (,)) }; /* 1, 2, 3 ... */

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

static void short_subscribe(const struct bt_gatt_attr *attr, uint16_t value)
{
	const bool notif_enabled = (value == BT_GATT_CCC_NOTIFY);

	if (notif_enabled) {
		SET_FLAG(flag_short_subscribe);
	}

	printk("Short notifications %s\n", notif_enabled ? "enabled" : "disabled");
}

static void long_subscribe(const struct bt_gatt_attr *attr, uint16_t value)
{
	const bool notif_enabled = (value == BT_GATT_CCC_NOTIFY);

	if (notif_enabled) {
		SET_FLAG(flag_long_subscribe);
	}

	printk("Long notifications %s\n", notif_enabled ? "enabled" : "disabled");
}

BT_GATT_SERVICE_DEFINE(test_svc, BT_GATT_PRIMARY_SERVICE(TEST_SERVICE_UUID),
		       BT_GATT_CHARACTERISTIC(TEST_CHRC_UUID,
					      BT_GATT_CHRC_NOTIFY | BT_GATT_CHRC_READ,
					      BT_GATT_PERM_READ, NULL, NULL, NULL),
		       BT_GATT_CUD("Short test_svc format description", BT_GATT_PERM_READ),
		       BT_GATT_CCC(short_subscribe, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
		       BT_GATT_CHARACTERISTIC(TEST_LONG_CHRC_UUID,
					      BT_GATT_CHRC_NOTIFY | BT_GATT_CHRC_READ,
					      BT_GATT_PERM_READ, NULL, NULL, NULL),
		       BT_GATT_CCC(long_subscribe, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE));

static volatile size_t num_notifications_sent;

static void notification_sent(struct bt_conn *conn, void *user_data)
{
	printk("Sent notification #%u\n", num_notifications_sent++);
}

static inline void multiple_notify(const struct bt_gatt_attr *attrs[2])
{
	int err;
	static struct bt_gatt_notify_params params[] = {
		{
			.data = long_chrc_data,
			.len = LONG_CHRC_SIZE,
			.func = notification_sent,
			.uuid = NULL,
		},
		{
			.data = chrc_data,
			.len = CHRC_SIZE,
			.func = notification_sent,
			.uuid = NULL,
		},
	};
	params[0].attr = attrs[0];
	params[1].attr = attrs[1];

	do {
		err = bt_gatt_notify_multiple(g_conn, ARRAY_SIZE(params), params);

		if (err == -ENOMEM) {
			k_sleep(K_MSEC(10));
		} else if (err) {
			TEST_FAIL("multiple notify failed (err %d)", err);
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
	WAIT_FOR_FLAG(flag_short_subscribe);
	WAIT_FOR_FLAG(flag_long_subscribe);
}

static void test_main(void)
{
	const struct bt_gatt_attr *attrs[2];

	setup();

	/* Long characteristic [attr=value] */
	attrs[0] = bt_gatt_find_by_uuid(NULL, 0, TEST_LONG_CHRC_UUID);
	/* Short characteristic [attr=descriptor] */
	attrs[1] = &attr_test_svc[1];

	for (int i = 0; i < NOTIFICATION_COUNT / 2; i++) {
		multiple_notify(attrs);
	}

	while (num_notifications_sent < NOTIFICATION_COUNT / 2) {
		k_sleep(K_MSEC(100));
	}

	k_sleep(K_MSEC(1000));

	if (num_notifications_sent != NOTIFICATION_COUNT) {
		TEST_FAIL("Unexpected notification callback value");
	}

	TEST_PASS("GATT server passed");
}

/* Send two values in one ATT_MULTIPLE_HANDLE_VALUE_NTF of pdu_len octets:
 * the opcode, then a handle, a length and the value for each attribute.
 */
static int notify_multiple_pdu_len(const struct bt_gatt_attr *attrs[2], uint16_t pdu_len)
{
	const uint16_t values_len = pdu_len - 1U - 2U * NOTIFY_MULT_TUPLE_HDR_LEN;
	struct bt_gatt_notify_params params[] = {
		{
			.attr = attrs[0],
			.data = long_chrc_data,
			.len = values_len / 2U,
			.func = notification_sent,
		},
		{
			.attr = attrs[1],
			.data = long_chrc_data,
			.len = values_len - values_len / 2U,
			.func = notification_sent,
		},
	};

	TEST_ASSERT(params[1].len <= sizeof(long_chrc_data), "MTU %u too large", pdu_len);

	return bt_gatt_notify_multiple(g_conn, ARRAY_SIZE(params), params);
}

static void test_mtu(void)
{
	const struct bt_gatt_attr *attrs[2];
	uint16_t mtu;
	int err;

	setup();

	attrs[0] = bt_gatt_find_by_uuid(NULL, 0, TEST_LONG_CHRC_UUID);
	attrs[1] = &attr_test_svc[1];
	mtu = bt_gatt_get_mtu(g_conn);

	err = notify_multiple_pdu_len(attrs, mtu + 1U);
	if (err != -ERANGE) {
		TEST_FAIL("PDU of %u octets not rejected (err %d)", mtu + 1U, err);
	}

	err = notify_multiple_pdu_len(attrs, mtu);
	if (err != 0) {
		TEST_FAIL("PDU of %u octets failed (err %d)", mtu, err);
	}

	while (num_notifications_sent < 2) {
		k_sleep(K_MSEC(10));
	}

	TEST_PASS("GATT server passed");
}

static void test_batch_mtu(void)
{
	const struct bt_gatt_attr *attrs[2];
	struct bt_gatt_notify_params params = {
		.data = long_chrc_data,
		.func = notification_sent,
		.chan_opt = BT_ATT_CHAN_OPT_ENHANCED_ONLY,
	};
	uint16_t mtu;
	int err;

	setup();

	attrs[0] = bt_gatt_find_by_uuid(NULL, 0, TEST_LONG_CHRC_UUID);
	attrs[1] = &attr_test_svc[1];
	mtu = bt_gatt_get_mtu(g_conn);

	/* The client did not exchange the ATT_MTU, so the EATT bearers have the
	 * largest one, and it is smaller than the ATT buffers, which are sized
	 * for the local UATT MTU. The notifications may only use EATT, where a
	 * PDU that exceeds the ATT_MTU can never be sent.
	 */
	TEST_ASSERT(bt_gatt_get_uatt_mtu(g_conn) < mtu, "UATT MTU %u not below %u",
		    bt_gatt_get_uatt_mtu(g_conn), mtu);

	/* Together the two values make a PDU one octet larger than the ATT_MTU */
	params.attr = attrs[0];
	params.len = LONG_CHRC_SIZE;
	err = bt_gatt_notify_cb(g_conn, &params);
	if (err != 0) {
		TEST_FAIL("First notification failed (err %d)", err);
	}

	params.attr = attrs[1];
	params.len = mtu - 2U * NOTIFY_MULT_TUPLE_HDR_LEN - LONG_CHRC_SIZE;
	TEST_ASSERT(params.len <= sizeof(long_chrc_data), "MTU %u too large", mtu);
	err = bt_gatt_notify_cb(g_conn, &params);
	if (err != 0) {
		TEST_FAIL("Second notification failed (err %d)", err);
	}

	while (num_notifications_sent < 2) {
		k_sleep(K_MSEC(10));
	}

	TEST_PASS("GATT server passed");
}

static void batch_sent(struct bt_conn *conn, void *user_data)
{
	SET_FLAG(flag_batch_sent);
}

static void test_disconnect(void)
{
	struct bt_gatt_notify_params params = {
		.attr = &attr_test_svc[1],
		.data = chrc_data,
		.len = CHRC_SIZE,
		.func = batch_sent,
	};
	int err;

	setup();

	err = bt_gatt_notify_cb(g_conn, &params);
	if (err != 0) {
		TEST_FAIL("Notification failed (err %d)", err);
	}

	/* The notification waits in the batch buffer, which is flushed while
	 * the disconnection is still in progress.
	 */
	err = bt_conn_disconnect(g_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	if (err != 0) {
		TEST_FAIL("Disconnect failed (err %d)", err);
	}

	WAIT_FOR_FLAG_UNSET(flag_is_connected);

	/* A flush that ran before the disconnection started sent the batch */
	if (IS_FLAG_SET(flag_batch_sent)) {
		TEST_FAIL("Batch flushed while connected");
	}

	TEST_PASS("GATT server passed");
}

static const struct bst_test_instance test_gatt_server[] = {
	{
		.test_id = "gatt_server",
		.test_main_f = test_main,
	},
	{
		.test_id = "gatt_server_disconnect",
		.test_main_f = test_disconnect,
	},
	{
		.test_id = "gatt_server_mtu",
		.test_main_f = test_mtu,
	},
	{
		.test_id = "gatt_server_batch_mtu",
		.test_main_f = test_batch_mtu,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_gatt_server_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_gatt_server);
}
