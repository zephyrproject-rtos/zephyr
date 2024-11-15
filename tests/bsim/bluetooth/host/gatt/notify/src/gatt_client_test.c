/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gatt.h>

#include "common.h"

CREATE_FLAG(flag_is_connected);
CREATE_FLAG(flag_is_encrypted);
CREATE_FLAG(flag_discover_complete);
CREATE_FLAG(flag_short_subscribed);
CREATE_FLAG(flag_long_subscribed);

static struct bt_conn *g_conn;
static uint16_t chrc_handle;
static uint16_t long_chrc_handle;
static const struct bt_uuid *test_svc_uuid = TEST_SERVICE_UUID;

static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err != 0) {
		FAIL("Failed to connect to %s (%u)\n", addr, err);
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
		FAIL("Encryption failer (%d)\n", err);
	} else if (level < BT_SECURITY_L2) {
		FAIL("Insufficient sec level (%d)\n", level);
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
		FAIL("Could not stop scan: %d");
		return;
	}

	err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN, BT_LE_CONN_PARAM_DEFAULT, &g_conn);
	if (err != 0) {
		FAIL("Could not connect to peer: %d", err);
	}
}

static uint8_t discover_func(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			     struct bt_gatt_discover_params *params, int err)
{
	if (attr == NULL) {
		if (chrc_handle == 0 || long_chrc_handle == 0) {
			FAIL("Did not discover chrc (%x) or long_chrc (%x)", chrc_handle,
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
			FAIL("Discover failed (err %d)\n", err);
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
		FAIL("Discover failed(err %d)\n", err);
	}

	WAIT_FOR_FLAG(flag_discover_complete);
	printk("Discover complete\n");
}

static void test_short_subscribed(struct bt_conn *conn, uint8_t err,
				  struct bt_gatt_subscribe_params *params)
{
	if (err) {
		FAIL("Subscribe failed (err %d)\n", err);
	}

	SET_FLAG(flag_short_subscribed);

	if (!params) {
		printk("params NULL\n");
		return;
	}

	if (params->value_handle == chrc_handle) {
		printk("Subscribed to short characteristic\n");
	} else {
		FAIL("Unknown handle %d\n", params->value_handle);
	}
}

static void test_long_subscribed(struct bt_conn *conn, uint8_t err,
				 struct bt_gatt_subscribe_params *params)
{
	if (err) {
		FAIL("Subscribe failed (err %d)\n", err);
	}

	SET_FLAG(flag_long_subscribed);

	if (!params) {
		printk("params NULL\n");
		return;
	}

	if (params->value_handle == long_chrc_handle) {
		printk("Subscribed to long characteristic\n");
	} else {
		FAIL("Unknown handle %d\n", params->value_handle);
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
	.subscribe = test_short_subscribed,
	.ccc_handle = BT_GATT_AUTO_DISCOVER_CCC_HANDLE,
	.disc_params = &disc_params_short,
	.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE,
	.value = BT_GATT_CCC_NOTIFY,
};
static struct bt_gatt_discover_params disc_params_long;
static struct bt_gatt_subscribe_params sub_params_long = {
	.notify = test_notify,
	.subscribe = test_long_subscribed,
	.ccc_handle = BT_GATT_AUTO_DISCOVER_CCC_HANDLE,
	.disc_params = &disc_params_long,
	.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE,
	.value = BT_GATT_CCC_NOTIFY,
};

static void gatt_subscribe_short(enum bt_att_chan_opt opt)
{
	int err;

	sub_params_short.value_handle = chrc_handle;
	sub_params_short.chan_opt = opt;
	err = bt_gatt_subscribe(g_conn, &sub_params_short);
	if (err < 0) {
		FAIL("Failed to subscribe\n");
	} else {
		printk("Subscribe request sent\n");
	}
}

static void gatt_unsubscribe_short(enum bt_att_chan_opt opt)
{
	int err;

	sub_params_short.value_handle = chrc_handle;
	sub_params_short.chan_opt = opt;
	err = bt_gatt_unsubscribe(g_conn, &sub_params_short);
	if (err < 0) {
		FAIL("Failed to unsubscribe\n");
	} else {
		printk("Unsubscribe request sent\n");
	}
}

static void gatt_subscribe_long(enum bt_att_chan_opt opt)
{
	int err;

	UNSET_FLAG(flag_long_subscribed);
	sub_params_long.value_handle = long_chrc_handle;
	sub_params_long.chan_opt = opt;
	err = bt_gatt_subscribe(g_conn, &sub_params_long);
	if (err < 0) {
		FAIL("Failed to subscribe\n");
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
		FAIL("Failed to unsubscribe\n");
	} else {
		printk("Unsubscribe request sent\n");
	}
}

static void setup(void)
{
	int err;

	err = bt_enable(NULL);
	if (err != 0) {
		FAIL("Bluetooth discover failed (err %d)\n", err);
	}

	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, device_found);
	if (err != 0) {
		FAIL("Scanning failed to start (err %d)\n", err);
	}

	printk("Scanning successfully started\n");

	WAIT_FOR_FLAG(flag_is_connected);

	err = bt_conn_set_security(g_conn, BT_SECURITY_L2);
	if (err) {
		FAIL("Starting encryption procedure failed (%d)\n", err);
	}

	WAIT_FOR_FLAG(flag_is_encrypted);

	while (bt_eatt_count(g_conn) < CONFIG_BT_EATT_MAX) {
		k_sleep(K_MSEC(10));
	}

	printk("EATT connected\n");
}

static void test_main_none(void)
{
	setup();

	gatt_discover(BT_ATT_CHAN_OPT_NONE);
	gatt_subscribe_short(BT_ATT_CHAN_OPT_NONE);
	gatt_subscribe_long(BT_ATT_CHAN_OPT_NONE);
	WAIT_FOR_FLAG(flag_short_subscribed);
	WAIT_FOR_FLAG(flag_long_subscribed);
	printk("Subscribed\n");

	while (num_notifications < NOTIFICATION_COUNT) {
		k_sleep(K_MSEC(100));
	}

	gatt_unsubscribe_short(BT_ATT_CHAN_OPT_NONE);
	gatt_unsubscribe_long(BT_ATT_CHAN_OPT_NONE);
	WAIT_FOR_FLAG(flag_short_subscribed);
	WAIT_FOR_FLAG(flag_long_subscribed);

	printk("Unsubscribed\n");

	PASS("GATT client Passed\n");
}

static void test_main_unenhanced(void)
{
	setup();

	gatt_discover(BT_ATT_CHAN_OPT_UNENHANCED_ONLY);
	gatt_subscribe_short(BT_ATT_CHAN_OPT_UNENHANCED_ONLY);
	gatt_subscribe_long(BT_ATT_CHAN_OPT_UNENHANCED_ONLY);
	WAIT_FOR_FLAG(flag_short_subscribed);
	WAIT_FOR_FLAG(flag_long_subscribed);

	printk("Subscribed\n");

	while (num_notifications < NOTIFICATION_COUNT) {
		k_sleep(K_MSEC(100));
	}

	gatt_unsubscribe_short(BT_ATT_CHAN_OPT_UNENHANCED_ONLY);
	gatt_unsubscribe_long(BT_ATT_CHAN_OPT_UNENHANCED_ONLY);
	WAIT_FOR_FLAG(flag_short_subscribed);
	WAIT_FOR_FLAG(flag_long_subscribed);

	printk("Unsubscribed\n");

	PASS("GATT client Passed\n");
}

static void test_main_enhanced(void)
{
	setup();

	gatt_discover(BT_ATT_CHAN_OPT_ENHANCED_ONLY);
	gatt_subscribe_short(BT_ATT_CHAN_OPT_ENHANCED_ONLY);
	gatt_subscribe_long(BT_ATT_CHAN_OPT_ENHANCED_ONLY);
	WAIT_FOR_FLAG(flag_short_subscribed);
	WAIT_FOR_FLAG(flag_long_subscribed);

	printk("Subscribed\n");

	while (num_notifications < NOTIFICATION_COUNT) {
		k_sleep(K_MSEC(100));
	}

	gatt_unsubscribe_short(BT_ATT_CHAN_OPT_ENHANCED_ONLY);
	gatt_unsubscribe_long(BT_ATT_CHAN_OPT_ENHANCED_ONLY);
	WAIT_FOR_FLAG(flag_short_subscribed);
	WAIT_FOR_FLAG(flag_long_subscribed);

	printk("Unsubscribed\n");

	PASS("GATT client Passed\n");
}

static void test_main_mixed(void)
{
	setup();

	gatt_discover(BT_ATT_CHAN_OPT_ENHANCED_ONLY);
	gatt_subscribe_short(BT_ATT_CHAN_OPT_ENHANCED_ONLY);
	gatt_subscribe_long(BT_ATT_CHAN_OPT_UNENHANCED_ONLY);
	WAIT_FOR_FLAG(flag_short_subscribed);
	WAIT_FOR_FLAG(flag_long_subscribed);

	printk("Subscribed\n");

	while (num_notifications < NOTIFICATION_COUNT) {
		k_sleep(K_MSEC(100));
	}

	gatt_unsubscribe_short(BT_ATT_CHAN_OPT_UNENHANCED_ONLY);
	gatt_unsubscribe_long(BT_ATT_CHAN_OPT_ENHANCED_ONLY);
	WAIT_FOR_FLAG(flag_short_subscribed);
	WAIT_FOR_FLAG(flag_long_subscribed);

	printk("Unsubscribed\n");

	PASS("GATT client Passed\n");
}

static const struct bst_test_instance test_vcs[] = {
	{
		.test_id = "gatt_client_none",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main_none,
	},
	{
		.test_id = "gatt_client_unenhanced",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main_unenhanced,
	},
	{
		.test_id = "gatt_client_enhanced",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main_enhanced,
	},
	{
		.test_id = "gatt_client_mixed",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main_mixed,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_gatt_client_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_vcs);
}
