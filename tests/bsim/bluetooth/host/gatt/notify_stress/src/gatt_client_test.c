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

DEFINE_FLAG_STATIC(flag_is_connected);
DEFINE_FLAG_STATIC(flag_is_encrypted);
DEFINE_FLAG_STATIC(flag_discover_complete);
DEFINE_FLAG_STATIC(flag_long_subscribed);

static struct bt_conn *g_conn;
static uint16_t long_chrc_handle;
static const struct bt_uuid *test_svc_uuid = TEST_SERVICE_UUID;

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

void security_changed(struct bt_conn *conn, bt_security_t level, enum bt_security_err err)
{
	if (err) {
		TEST_FAIL("Encryption failure (%d)", err);
	} else if (level < BT_SECURITY_L2) {
		TEST_FAIL("Insufficient sec level (%d)", level);
	} else {
		SET_FLAG(flag_is_encrypted);
	}
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
	.security_changed = security_changed,
};

void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type, struct net_buf_simple *ad)
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
		TEST_FAIL("Could not stop scan: %d");
		return;
	}

	err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN, BT_LE_CONN_PARAM_DEFAULT, &g_conn);
	if (err != 0) {
		TEST_FAIL("Could not connect to peer: %d", err);
	}
}

static uint8_t discover_func(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			     struct bt_gatt_discover_params *params)
{
	int err;

	if (attr == NULL) {
		if (long_chrc_handle == 0) {
			TEST_FAIL("Did not discover long_chrc (%x)", long_chrc_handle);
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
			printk("Found long_chrc\n");
			long_chrc_handle = chrc->value_handle;
		}
	}

	return BT_GATT_ITER_CONTINUE;
}

static void gatt_discover(enum bt_att_chan_opt opt)
{
	static struct bt_gatt_discover_params discover_params;
	int err;

	printk("Discovering services and characteristics\n");

	discover_params.uuid = test_svc_uuid;
	discover_params.func = discover_func;
	discover_params.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
	discover_params.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
	discover_params.type = BT_GATT_DISCOVER_PRIMARY;
	discover_params.chan_opt = opt;

	err = bt_gatt_discover(g_conn, &discover_params);
	if (err != 0) {
		TEST_FAIL("Discover failed(err %d)", err);
	}

	WAIT_FOR_FLAG(flag_discover_complete);
	printk("Discover complete\n");
}

static void test_long_subscribed(struct bt_conn *conn, uint8_t err,
				 struct bt_gatt_subscribe_params *params)
{
	if (err) {
		TEST_FAIL("Subscribe failed (err %d)", err);
	}

	SET_FLAG(flag_long_subscribed);

	if (!params) {
		printk("params NULL\n");
		return;
	}

	if (params->value_handle == long_chrc_handle) {
		printk("Subscribed to long characteristic\n");
	} else {
		TEST_FAIL("Unknown handle %d", params->value_handle);
	}
}

static volatile size_t num_notifications;
uint8_t test_notify(struct bt_conn *conn, struct bt_gatt_subscribe_params *params, const void *data,
		    uint16_t length)
{
	printk("Received notification #%u with length %d\n", num_notifications++, length);

	/* This causes ACL data drop in HCI IPC driver. */
	k_sleep(K_MSEC(1000));

	return BT_GATT_ITER_CONTINUE;
}

static struct bt_gatt_discover_params disc_params_long;
static struct bt_gatt_subscribe_params sub_params_long = {
	.notify = test_notify,
	.subscribe = test_long_subscribed,
	.ccc_handle = BT_GATT_AUTO_DISCOVER_CCC_HANDLE,
	.disc_params = &disc_params_long,
	.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE,
	.value = BT_GATT_CCC_NOTIFY,
};

static void gatt_subscribe_long(enum bt_att_chan_opt opt)
{
	int err;

	UNSET_FLAG(flag_long_subscribed);
	sub_params_long.value_handle = long_chrc_handle;
	sub_params_long.chan_opt = opt;
	err = bt_gatt_subscribe(g_conn, &sub_params_long);
	if (err < 0) {
		TEST_FAIL("Failed to subscribe");
	} else {
		printk("Subscribe request sent\n");
	}
}

static void gatt_unsubscribe_long(enum bt_att_chan_opt opt)
{
	int err;

	UNSET_FLAG(flag_long_subscribed);
	sub_params_long.value_handle = long_chrc_handle;
	sub_params_long.chan_opt = opt;
	err = bt_gatt_unsubscribe(g_conn, &sub_params_long);
	if (err < 0) {
		TEST_FAIL("Failed to unsubscribe");
	} else {
		printk("Unsubscribe request sent\n");
	}
}

static void setup(void)
{
	int err;

	err = bt_enable(NULL);
	if (err != 0) {
		TEST_FAIL("Bluetooth discover failed (err %d)", err);
	}

	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, device_found);
	if (err != 0) {
		TEST_FAIL("Scanning failed to start (err %d)", err);
	}

	printk("Scanning successfully started\n");

	WAIT_FOR_FLAG(flag_is_connected);

	err = bt_conn_set_security(g_conn, BT_SECURITY_L2);
	if (err) {
		TEST_FAIL("Starting encryption procedure failed (%d)", err);
	}

	WAIT_FOR_FLAG(flag_is_encrypted);

	while (bt_eatt_count(g_conn) < CONFIG_BT_EATT_MAX) {
		k_sleep(K_MSEC(10));
	}

	printk("EATT connected\n");
}

static void test_main_client(void)
{
	setup();

	gatt_discover(BT_ATT_CHAN_OPT_ENHANCED_ONLY);
	gatt_subscribe_long(BT_ATT_CHAN_OPT_ENHANCED_ONLY);
	WAIT_FOR_FLAG(flag_long_subscribed);

	printk("Subscribed\n");

	while (num_notifications < NOTIFICATION_COUNT) {
		k_sleep(K_MSEC(100));
	}

	gatt_unsubscribe_long(BT_ATT_CHAN_OPT_ENHANCED_ONLY);
	WAIT_FOR_FLAG(flag_long_subscribed);

	printk("Unsubscribed\n");

	TEST_PASS("GATT client Passed");
}

static const struct bst_test_instance test_vcs[] = {
	{
		.test_id = "gatt_client_enhanced_notif_stress",
		.test_main_f = test_main_client,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_gatt_client_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_vcs);
}
