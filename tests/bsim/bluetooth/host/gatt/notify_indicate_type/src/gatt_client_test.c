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
DEFINE_FLAG_STATIC(flag_discover_complete);
DEFINE_FLAG_STATIC(flag_subscribed);
DEFINE_FLAG_STATIC(flag_test_complete);

static struct bt_conn *g_conn;
static uint16_t chrc_handle;
static const struct bt_uuid *test_svc_uuid = TEST_SERVICE_UUID;

static volatile size_t notify_count;

static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err != 0) {
		TEST_FAIL("Failed to connect to %s (%u)", addr, err);
		return;
	}

	printk("Connected to %s\n", addr);

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

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			 struct net_buf_simple *ad)
{
	char addr_str[BT_ADDR_LE_STR_LEN];
	int err;

	if (g_conn != NULL) {
		return;
	}

	/* We're only interested in connectable events */
	if (type != BT_HCI_ADV_IND && type != BT_HCI_ADV_DIRECT_IND) {
		return;
	}

	bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));
	printk("Device found: %s (RSSI %d)\n", addr_str, rssi);

	printk("Stopping scan\n");
	err = bt_le_scan_stop();
	if (err != 0) {
		TEST_FAIL("Could not stop scan: %d", err);
		return;
	}

	err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN,
				BT_LE_CONN_PARAM_DEFAULT, &g_conn);
	if (err != 0) {
		TEST_FAIL("Could not connect to peer: %d", err);
	}
}

static uint8_t discover_func(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			     struct bt_gatt_discover_params *params)
{
	int err;

	if (attr == NULL) {
		if (chrc_handle == 0) {
			TEST_FAIL("Did not discover chrc (handle 0x%x)", chrc_handle);
		}

		(void)memset(params, 0, sizeof(*params));
		SET_FLAG(flag_discover_complete);

		return BT_GATT_ITER_STOP;
	}

	printk("[ATTRIBUTE] handle %u\n", attr->handle);

	if (params->type == BT_GATT_DISCOVER_PRIMARY &&
	    bt_uuid_cmp(params->uuid, TEST_SERVICE_UUID) == 0) {
		printk("Found test service\n");
		params->uuid = NULL;
		params->start_handle = attr->handle + 1;
		params->type = BT_GATT_DISCOVER_CHARACTERISTIC;

		err = bt_gatt_discover(conn, params);
		if (err != 0) {
			TEST_FAIL("Discover failed (err %d)", err);
		}

		return BT_GATT_ITER_STOP;
	} else if (params->type == BT_GATT_DISCOVER_CHARACTERISTIC) {
		const struct bt_gatt_chrc *chrc = (struct bt_gatt_chrc *)attr->user_data;

		if (bt_uuid_cmp(chrc->uuid, TEST_CHRC_UUID) == 0) {
			printk("Found chrc (value_handle 0x%x)\n", chrc->value_handle);
			chrc_handle = chrc->value_handle;
		}
	}

	return BT_GATT_ITER_CONTINUE;
}

static void gatt_discover(void)
{
	static struct bt_gatt_discover_params discover_params;
	int err;

	printk("Discovering services and characteristics\n");

	discover_params.uuid = test_svc_uuid;
	discover_params.func = discover_func;
	discover_params.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
	discover_params.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
	discover_params.type = BT_GATT_DISCOVER_PRIMARY;

	err = bt_gatt_discover(g_conn, &discover_params);
	if (err != 0) {
		TEST_FAIL("Discover failed (err %d)", err);
	}

	WAIT_FOR_FLAG(flag_discover_complete);
	printk("Discover complete\n");
}

static void test_subscribed(struct bt_conn *conn, uint8_t err,
			    struct bt_gatt_subscribe_params *params)
{
	if (err) {
		TEST_FAIL("Subscribe failed (err %d)", err);
	}

	SET_FLAG(flag_subscribed);

	if (!params) {
		printk("params NULL\n");
		return;
	}

	printk("Subscribed to handle 0x%x\n", params->value_handle);
}

static uint8_t test_notify(struct bt_conn *conn, struct bt_gatt_subscribe_params *params,
			   const void *data, uint16_t length)
{
	if (data == NULL) {
		/* Unsubscribed */
		printk("Unsubscribed\n");
		return BT_GATT_ITER_STOP;
	}

	printk("Received PDU #%u: length=%u received_opcode=0x%02x\n",
	       (unsigned int)notify_count, length, params->received_opcode);

	if (notify_count == 0) {
		TEST_ASSERT(params->received_opcode == BT_GATT_NOTIFY_TYPE_NOTIFY,
			    "PDU #0: expected notification (0x%02x), got 0x%02x",
			    BT_GATT_NOTIFY_TYPE_NOTIFY, params->received_opcode);
		printk("PDU #0 correctly identified as notification\n");
	} else if (notify_count == 1) {
		TEST_ASSERT(params->received_opcode == BT_GATT_NOTIFY_TYPE_INDICATE,
			    "PDU #1: expected indication (0x%02x), got 0x%02x",
			    BT_GATT_NOTIFY_TYPE_INDICATE, params->received_opcode);
		printk("PDU #1 correctly identified as indication\n");
		SET_FLAG(flag_test_complete);
	}

	notify_count++;

	return BT_GATT_ITER_CONTINUE;
}

static struct bt_gatt_discover_params disc_params;
static struct bt_gatt_subscribe_params sub_params = {
	.notify = test_notify,
	.subscribe = test_subscribed,
	.ccc_handle = BT_GATT_AUTO_DISCOVER_CCC_HANDLE,
	.disc_params = &disc_params,
	.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE,
	.value = BT_GATT_CCC_NOTIFY | BT_GATT_CCC_INDICATE,
};

static void test_main(void)
{
	int err;

	err = bt_enable(NULL);
	if (err != 0) {
		TEST_FAIL("Bluetooth init failed (err %d)", err);
	}

	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, device_found);
	if (err != 0) {
		TEST_FAIL("Scanning failed to start (err %d)", err);
	}

	printk("Scanning successfully started\n");

	WAIT_FOR_FLAG(flag_is_connected);

	gatt_discover();

	sub_params.value_handle = chrc_handle;
	err = bt_gatt_subscribe(g_conn, &sub_params);
	if (err < 0) {
		TEST_FAIL("Failed to subscribe (err %d)", err);
	}

	WAIT_FOR_FLAG(flag_subscribed);
	printk("Subscribed, waiting for notification and indication\n");

	WAIT_FOR_FLAG(flag_test_complete);

	TEST_PASS("GATT client passed");
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
