/*
 * Copyright (c) 2026 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <stdint.h>

#include <zephyr/bluetooth/att.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

#include "babblekit/flags.h"
#include "babblekit/testcase.h"

#include "common.h"

DEFINE_FLAG_STATIC(flag_is_connected);
DEFINE_FLAG_STATIC(flag_mtu_exchanged);
DEFINE_FLAG_STATIC(flag_discover_complete);
DEFINE_FLAG_STATIC(flag_write_complete);
DEFINE_FLAG_STATIC(flag_a_subscribed);
DEFINE_FLAG_STATIC(flag_b_subscribed);
DEFINE_FLAG_STATIC(flag_marker_received);

static struct bt_conn *g_conn;
static uint16_t chrc_a_handle;
static uint16_t chrc_b_handle;
static uint16_t csf_handle;

struct notification {
	uint16_t handle;
	uint16_t len;
};

static struct notification received[8];
static size_t num_received;

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err != 0) {
		TEST_FAIL("Failed to connect to %s (%u)", bt_conn_dst_str(conn), err);
		return;
	}

	printk("Connected to %s\n", bt_conn_dst_str(conn));

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

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			 struct net_buf_simple *ad)
{
	int err;

	if (g_conn != NULL) {
		return;
	}

	if (type != BT_HCI_ADV_IND && type != BT_HCI_ADV_DIRECT_IND) {
		return;
	}

	printk("Device found: %s (RSSI %d)\n", bt_addr_le_str(addr), rssi);

	err = bt_le_scan_stop();
	if (err != 0) {
		TEST_FAIL("Could not stop scan (err %d)", err);
		return;
	}

	err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN, BT_LE_CONN_PARAM_DEFAULT, &g_conn);
	if (err != 0) {
		TEST_FAIL("Could not connect to peer (err %d)", err);
	}
}

static void exchange_func(struct bt_conn *conn, uint8_t err,
			  struct bt_gatt_exchange_params *params)
{
	TEST_ASSERT(err == 0, "MTU exchange failed (err %u)", err);

	SET_FLAG(flag_mtu_exchanged);
}

static uint8_t discover_func(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			     struct bt_gatt_discover_params *params)
{
	const struct bt_gatt_chrc *chrc;

	if (attr == NULL) {
		SET_FLAG(flag_discover_complete);
		return BT_GATT_ITER_STOP;
	}

	chrc = attr->user_data;

	if (bt_uuid_cmp(chrc->uuid, TEST_CHRC_A_UUID) == 0) {
		chrc_a_handle = chrc->value_handle;
	} else if (bt_uuid_cmp(chrc->uuid, TEST_CHRC_B_UUID) == 0) {
		chrc_b_handle = chrc->value_handle;
	} else if (bt_uuid_cmp(chrc->uuid, BT_UUID_GATT_CLIENT_FEATURES) == 0) {
		csf_handle = chrc->value_handle;
	}

	return BT_GATT_ITER_CONTINUE;
}

static void discover(void)
{
	static struct bt_gatt_discover_params params = {
		.func = discover_func,
		.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE,
		.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE,
		.type = BT_GATT_DISCOVER_CHARACTERISTIC,
	};
	int err;

	err = bt_gatt_discover(g_conn, &params);
	TEST_ASSERT(err == 0, "Discovery failed (err %d)", err);

	WAIT_FOR_FLAG(flag_discover_complete);
	TEST_ASSERT(chrc_a_handle != 0 && chrc_b_handle != 0 && csf_handle != 0,
		    "Characteristics not found");
}

static void write_cb(struct bt_conn *conn, uint8_t err, struct bt_gatt_write_params *params)
{
	TEST_ASSERT(err == 0, "Write failed (err 0x%02x)", err);

	SET_FLAG(flag_write_complete);
}

/* Enable Multiple Handle Value Notifications in Client Supported Features */
static void write_csf(void)
{
	static const uint8_t csf[] = { BIT(2) };
	static struct bt_gatt_write_params params = {
		.func = write_cb,
		.data = csf,
		.length = sizeof(csf),
	};
	int err;

	params.handle = csf_handle;

	err = bt_gatt_write(g_conn, &params);
	TEST_ASSERT(err == 0, "Failed to write CSF (err %d)", err);

	WAIT_FOR_FLAG(flag_write_complete);
}

static uint8_t notify_func(struct bt_conn *conn, struct bt_gatt_subscribe_params *params,
			   const void *data, uint16_t length)
{
	const uint8_t *value = data;

	if (data == NULL) {
		return BT_GATT_ITER_STOP;
	}

	printk("Received %u bytes for handle 0x%04x\n", length, params->value_handle);

	TEST_ASSERT(num_received < ARRAY_SIZE(received), "Too many notifications");

	for (uint16_t i = 0; i < length; i++) {
		TEST_ASSERT(value[i] == (uint8_t)i, "Unexpected data at offset %u", i);
	}

	received[num_received].handle = params->value_handle;
	received[num_received].len = length;
	num_received++;

	if (params->value_handle == chrc_b_handle && length == MARKER_LEN) {
		SET_FLAG(flag_marker_received);
	}

	return BT_GATT_ITER_CONTINUE;
}

static void subscribed(struct bt_conn *conn, uint8_t err, struct bt_gatt_subscribe_params *params)
{
	TEST_ASSERT(err == 0, "Subscribe failed (err 0x%02x)", err);

	if (params->value_handle == chrc_a_handle) {
		SET_FLAG(flag_a_subscribed);
	} else if (params->value_handle == chrc_b_handle) {
		SET_FLAG(flag_b_subscribed);
	}
}

static struct bt_gatt_discover_params disc_params_a;
static struct bt_gatt_subscribe_params sub_params_a = {
	.notify = notify_func,
	.subscribe = subscribed,
	.ccc_handle = BT_GATT_AUTO_DISCOVER_CCC_HANDLE,
	.disc_params = &disc_params_a,
	.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE,
	.value = BT_GATT_CCC_NOTIFY | BT_GATT_CCC_INDICATE,
};

static struct bt_gatt_discover_params disc_params_b;
static struct bt_gatt_subscribe_params sub_params_b = {
	.notify = notify_func,
	.subscribe = subscribed,
	.ccc_handle = BT_GATT_AUTO_DISCOVER_CCC_HANDLE,
	.disc_params = &disc_params_b,
	.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE,
	.value = BT_GATT_CCC_NOTIFY,
};

static void subscribe(struct bt_gatt_subscribe_params *params, uint16_t value_handle)
{
	int err;

	params->value_handle = value_handle;

	err = bt_gatt_subscribe(g_conn, params);
	TEST_ASSERT(err == 0, "Subscribing to handle 0x%04x failed (err %d)", value_handle, err);
}

static void verify_received(void)
{
	const struct notification expected[] = {
		{ chrc_a_handle, MAX_LEN },   /* notification */
		{ chrc_a_handle, MAX_LEN },   /* indication */
		{ chrc_b_handle, SHORT_LEN }, /* tuple after the oversized one */
		{ chrc_b_handle, MARKER_LEN },
	};

	TEST_ASSERT(num_received == ARRAY_SIZE(expected),
		    "Received %zu notifications, expected %zu", num_received, ARRAY_SIZE(expected));

	ARRAY_FOR_EACH(expected, i) {
		TEST_ASSERT(received[i].handle == expected[i].handle &&
			    received[i].len == expected[i].len,
			    "Notification %zu: handle 0x%04x len %u, expected handle 0x%04x len %u",
			    i, received[i].handle, received[i].len, expected[i].handle,
			    expected[i].len);
	}
}

static void test_main(void)
{
	static struct bt_gatt_exchange_params exchange_params = {
		.func = exchange_func,
	};
	int err;

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Bluetooth init failed (err %d)", err);

	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, device_found);
	TEST_ASSERT(err == 0, "Scanning failed to start (err %d)", err);

	WAIT_FOR_FLAG(flag_is_connected);

	err = bt_gatt_exchange_mtu(g_conn, &exchange_params);
	TEST_ASSERT(err == 0, "MTU exchange failed (err %d)", err);

	WAIT_FOR_FLAG(flag_mtu_exchanged);
	TEST_ASSERT(bt_gatt_get_mtu(g_conn) >= REQUIRED_ATT_MTU, "ATT MTU %u too small",
		    bt_gatt_get_mtu(g_conn));

	discover();
	write_csf();

	subscribe(&sub_params_a, chrc_a_handle);
	subscribe(&sub_params_b, chrc_b_handle);
	WAIT_FOR_FLAG(flag_a_subscribed);
	WAIT_FOR_FLAG(flag_b_subscribed);

	WAIT_FOR_FLAG(flag_marker_received);

	verify_received();

	TEST_PASS_AND_EXIT("GATT client passed");
}

static const struct bst_test_instance test_gatt_client[] = {
	{
		.test_id = "gatt_client",
		.test_main_f = test_main,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_gatt_client_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_gatt_client);
}
