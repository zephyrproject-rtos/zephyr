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
DEFINE_FLAG_STATIC(flag_is_encrypted);
DEFINE_FLAG_STATIC(flag_discover_complete);
DEFINE_FLAG_STATIC(flag_write_complete);
DEFINE_FLAG_STATIC(flag_subscribed_short);
DEFINE_FLAG_STATIC(flag_subscribed_long);

static struct bt_conn *g_conn;
static uint16_t chrc_handle;
static uint16_t long_chrc_handle;
static uint16_t csf_handle;
static const struct bt_uuid *test_svc_uuid = TEST_SERVICE_UUID;

static void exchange_func(struct bt_conn *conn, uint8_t err, struct bt_gatt_exchange_params *params)
{
	if (!err) {
		printk("MTU exchange done\n");
	} else {
		printk("MTU exchange failed (err %" PRIu8 ")\n", err);
	}
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];
	static struct bt_gatt_exchange_params exchange_params;

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err != 0) {
		TEST_FAIL("Failed to connect to %s (%u)", addr, err);
		return;
	}

	printk("Connected to %s\n", addr);

	SET_FLAG(flag_is_connected);

	exchange_params.func = exchange_func;
	err = bt_gatt_exchange_mtu(conn, &exchange_params);
	if (err) {
		printk("MTU exchange failed (err %d)", err);
	}
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
		TEST_FAIL("Encryption failed (%d)", err);
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
		if (chrc_handle == 0 || long_chrc_handle == 0) {
			TEST_FAIL("Did not discover chrc (%x) or long_chrc (%x)", chrc_handle,
			     long_chrc_handle);
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
			printk("Found chrc\n");
			chrc_handle = chrc->value_handle;
		} else if (bt_uuid_cmp(chrc->uuid, TEST_LONG_CHRC_UUID) == 0) {
			printk("Found long_chrc\n");
			long_chrc_handle = chrc->value_handle;
		} else if (bt_uuid_cmp(chrc->uuid, BT_UUID_GATT_CLIENT_FEATURES) == 0) {
			printk("Found csf\n");
			csf_handle = chrc->value_handle;
		}
	}

	return BT_GATT_ITER_CONTINUE;
}

static void gatt_discover(const struct bt_uuid *uuid, uint8_t type)
{
	static struct bt_gatt_discover_params discover_params;
	int err;

	printk("Discovering services and characteristics\n");

	discover_params.uuid = uuid;
	discover_params.func = discover_func;
	discover_params.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
	discover_params.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
	discover_params.type = type;

	UNSET_FLAG(flag_discover_complete);

	err = bt_gatt_discover(g_conn, &discover_params);
	if (err != 0) {
		TEST_FAIL("Discover failed(err %d)", err);
	}

	WAIT_FOR_FLAG(flag_discover_complete);
	printk("Discover complete\n");
}

static void test_subscribed(struct bt_conn *conn,
			    uint8_t err,
			    struct bt_gatt_subscribe_params *params)
{
	if (err) {
		TEST_FAIL("Subscribe failed (err %d)", err);
	}

	if (!params) {
		TEST_FAIL("params NULL");
	}

	if (params->value_handle == chrc_handle) {
		FORCE_FLAG(flag_subscribed_short, (bool)params->value);
		printk("Subscribed to short characteristic\n");
	} else if (params->value_handle == long_chrc_handle) {
		FORCE_FLAG(flag_subscribed_long, (bool)params->value);
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

	return BT_GATT_ITER_CONTINUE;
}

static struct bt_gatt_discover_params disc_params_short;
static struct bt_gatt_subscribe_params sub_params_short = {
	.notify = test_notify,
	.subscribe = test_subscribed,
	.ccc_handle = BT_GATT_AUTO_DISCOVER_CCC_HANDLE,
	.disc_params = &disc_params_short,
	.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE,
	.value = BT_GATT_CCC_NOTIFY,
};
static struct bt_gatt_discover_params disc_params_long;
static struct bt_gatt_subscribe_params sub_params_long = {
	.notify = test_notify,
	.subscribe = test_subscribed,
	.ccc_handle = BT_GATT_AUTO_DISCOVER_CCC_HANDLE,
	.disc_params = &disc_params_long,
	.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE,
	.value = BT_GATT_CCC_NOTIFY,
};

static void write_cb(struct bt_conn *conn, uint8_t err, struct bt_gatt_write_params *params)
{
	if (err != BT_ATT_ERR_SUCCESS) {
		TEST_FAIL("Write failed: 0x%02X", err);
	}

	SET_FLAG(flag_write_complete);
}

static void write_csf(void)
{
	/* Client Supported Features Characteristic Value
	 * Bit 0: Robust Caching
	 * Bit 1: EATT
	 * Bit 2: Multiple Handle Value Notifications
	 */
	static const uint8_t csf[] = { BIT(2) };
	static struct bt_gatt_write_params write_params = {
		.func = write_cb,
		.offset = 0,
		.data = csf,
		.length = sizeof(csf),
	};
	int err;

	printk("Writing to Client Supported Features Characteristic\n");

	write_params.handle = csf_handle;
	UNSET_FLAG(flag_write_complete);

	err = bt_gatt_write(g_conn, &write_params);
	if (err) {
		TEST_FAIL("bt_gatt_write failed (err %d)", err);
	}

	WAIT_FOR_FLAG(flag_write_complete);
	printk("Success\n");
}

static void subscribe(struct bt_gatt_subscribe_params *params, bool subscribe)
{
	int err;

	if (subscribe) {
		err = bt_gatt_subscribe(g_conn, params);
	} else {
		err = bt_gatt_unsubscribe(g_conn, params);
	}

	if (err < 0) {
		TEST_FAIL("Failed to %ssubscribe (err %d)", subscribe ? "un":"", err);
	} else {
		printk("%ssubscribe request sent\n", subscribe ? "un":"");
	}

}

static void test_main(void)
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

	gatt_discover(test_svc_uuid, BT_GATT_DISCOVER_PRIMARY);
	gatt_discover(BT_UUID_GATT_CLIENT_FEATURES, BT_GATT_DISCOVER_CHARACTERISTIC);
	write_csf();

	sub_params_short.value_handle = chrc_handle;
	sub_params_long.value_handle = long_chrc_handle;

	subscribe(&sub_params_short, true);
	subscribe(&sub_params_long, true);
	WAIT_FOR_FLAG(flag_subscribed_short);
	WAIT_FOR_FLAG(flag_subscribed_long);

	printk("Subscribed\n");

	while (num_notifications < NOTIFICATION_COUNT) {
		k_sleep(K_MSEC(100));
	}

	subscribe(&sub_params_short, false);
	subscribe(&sub_params_long, false);
	WAIT_FOR_FLAG_UNSET(flag_subscribed_short);
	WAIT_FOR_FLAG_UNSET(flag_subscribed_long);


	printk("Unsubscribed\n");

	TEST_PASS("GATT client Passed");
}

static const struct bst_test_instance test_vcs[] = {
	{
		.test_id = "gatt_client",
		.test_main_f = test_main,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_gatt_client_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_vcs);
}
