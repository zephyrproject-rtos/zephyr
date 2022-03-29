/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <bluetooth/bluetooth.h>
#include <bluetooth/gatt.h>
#include <bluetooth/conn.h>
#include <bluetooth/att.h>

#include "common.h"

CREATE_FLAG(flag_is_connected);
CREATE_FLAG(flag_discover_complete);
CREATE_FLAG(flag_all_chann_conn);
CREATE_FLAG(flag_test_chann_conn);
CREATE_FLAG(flag_one_chann_discon);

static struct bt_conn *g_conn;
static const struct bt_gatt_attr *local_attr;
static struct bt_uuid *test_svc_uuid = TEST_SERVICE_UUID;

#define NUM_NOTIF 100
#define SAMPLE_DATA 1
#define EATT_BEARERS_TEST 1

volatile int num_eatt_channels;

void eatt_chan_connected(const struct bt_eatt_chan_info *info)
{
	num_eatt_channels++;
	printk("EATT channel connected: %d\n", num_eatt_channels);

	if (num_eatt_channels > CONFIG_BT_EATT_MAX) {
		FAIL("Too many EATT channels connected (%d), expected maximum %d\n",
		     num_eatt_channels, CONFIG_BT_EATT_MAX);
	}

	if (num_eatt_channels == EATT_BEARERS_TEST) {
		SET_FLAG(flag_test_chann_conn);
	}

	if (num_eatt_channels == CONFIG_BT_EATT_MAX) {
		UNSET_FLAG(flag_test_chann_conn);
		SET_FLAG(flag_all_chann_conn);
	}
}

void eatt_chan_disconnected(const struct bt_eatt_chan_info *info)
{
	if (num_eatt_channels > 0) {
		num_eatt_channels--;
		printk("EATT channel disconnected, %d channels left\n", num_eatt_channels);
		SET_FLAG(flag_one_chann_discon);
	} else {
		FAIL("No channels left to disconnect\n");
	}
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];


	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err != 0) {
		FAIL("Failed to connect to %s (%u)\n", addr, err);
		return;
	}

	printk("Connected to %s\n", addr);

	g_conn = conn;
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

void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
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
		FAIL("Could not stop scan: %d");
		return;
	}

	err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN,
				BT_LE_CONN_PARAM_DEFAULT, &g_conn);
	if (err != 0) {
		FAIL("Could not connect to peer: %d", err);
	}
}

void send_notification(void)
{
	uint8_t sample_dat = SAMPLE_DATA;
	int err;

	err = bt_gatt_notify(g_conn, local_attr, &sample_dat, sizeof(sample_dat));
	if (err) {
		printk("GATT notify failed (err %d)\n", err);
	}
}

static uint8_t discover_func(struct bt_conn *conn,
		const struct bt_gatt_attr *attr,
		struct bt_gatt_discover_params *params)
{
	SET_FLAG(flag_discover_complete);
	printk("Discover complete\n");
	return BT_GATT_ITER_STOP;
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
		FAIL("Discover failed(err %d)\n", err);
	}
}

BT_GATT_SERVICE_DEFINE(g_svc,
	BT_GATT_PRIMARY_SERVICE(TEST_SERVICE_UUID),
	BT_GATT_CHARACTERISTIC(TEST_CHRC_UUID, BT_GATT_CHRC_NOTIFY,
			       0x00, NULL, NULL, NULL));

static void test_main(void)
{
	int err;

	device_sync_init(PERIPHERAL_ID);

	err = bt_enable(NULL);
	if (err != 0) {
		FAIL("Bluetooth discover failed (err %d)\n", err);
	}

	static struct bt_eatt_cb eatt_cb = {
		.chan_connected = eatt_chan_connected,
		.chan_disconnected = eatt_chan_disconnected,
	};

	bt_eatt_cb_register(&eatt_cb);

	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, device_found);
	if (err != 0) {
		FAIL("Scanning failed to start (err %d)\n", err);
	}

	printk("Scanning successfully started\n");

	WAIT_FOR_FLAG(flag_is_connected);

	err = bt_eatt_connect(g_conn, CONFIG_BT_EATT_MAX);
	if (err) {
		FAIL("Sending credit based connection request failed (err %d)\n", err);
	}

	WAIT_FOR_FLAG(flag_all_chann_conn);

	local_attr = &g_svc.attrs[1];
	printk("############# Notification test\n");
	for (int indx = 0; indx < NUM_NOTIF; indx++) {
		printk("Notification %d\n", indx);
		send_notification();
	}

	printk("############# Disconnect and reconnect\n");
	for (int indx = 0; indx < CONFIG_BT_EATT_MAX; indx++) {
		printk("Disconnect channel %d\n", indx);
		bt_eatt_disconnect_one(g_conn);
		WAIT_FOR_FLAG(flag_one_chann_discon);
		UNSET_FLAG(flag_one_chann_discon);
	}

	printk("Connecting %d bearers\n", EATT_BEARERS_TEST);
	err = bt_eatt_connect(g_conn, EATT_BEARERS_TEST);
	if (err) {
		FAIL("Sending credit based connection request failed (err %d)\n", err);
	}

	WAIT_FOR_FLAG(flag_test_chann_conn);

	printk("############# Send notifications during discovery request\n");
	gatt_discover();
	while (!TEST_FLAG(flag_discover_complete)) {
		printk("Notifying...\n");
		send_notification();
	}

	printk("Send sync to contine\n");
	device_sync_send();

	PASS("Client Passed\n");
}

static const struct bst_test_instance test_vcs[] = {
	{
		.test_id = "client",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main
	},
	BSTEST_END_MARKER
};

struct bst_test_list *test_client_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_vcs);
}
