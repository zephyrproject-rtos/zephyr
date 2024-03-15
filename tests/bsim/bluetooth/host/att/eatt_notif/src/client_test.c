/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * EATT notification reliability test:
 * A central acting as a GATT client scans and connects
 * to a peripheral acting as a GATT server.
 * The GATT client will then attempt to connect a number of CONFIG_BT_EATT_MAX bearers
 * over EATT, send notifications, disconnect all bearers and reconnect EATT_BEARERS_TEST
 * and send start a transaction with a request, then send a lot of notifications
 * before the response is received.
 * The test might be expanded by checking that all the notifications all transmitted
 * on EATT channels.
 */

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/att.h>

#include "common.h"

CREATE_FLAG(flag_is_connected);
CREATE_FLAG(flag_discover_complete);
CREATE_FLAG(flag_is_encrypted);

static struct bt_conn *g_conn;
static const struct bt_gatt_attr *local_attr;
static const struct bt_uuid *test_svc_uuid = TEST_SERVICE_UUID;

#define NUM_NOTIF 100
#define SAMPLE_DATA 1
#define EATT_BEARERS_TEST 1

volatile int num_eatt_channels;

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

static void security_changed(struct bt_conn *conn, bt_security_t level,
			     enum bt_security_err security_err)
{
	if (security_err == BT_SECURITY_ERR_SUCCESS && level > BT_SECURITY_L1) {
		SET_FLAG(flag_is_encrypted);
	}
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
	.security_changed = security_changed,
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
	const uint8_t sample_dat = SAMPLE_DATA;
	int err;

	do {
		err = bt_gatt_notify(g_conn, local_attr, &sample_dat, sizeof(sample_dat));
		if (!err) {
			return;
		} else if (err != -ENOMEM) {
			printk("GATT notify failed (err %d)\n", err);
			return;
		}
		k_sleep(K_TICKS(1));
	} while (err == -ENOMEM);
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
	discover_params.chan_opt = BT_ATT_CHAN_OPT_NONE;

	err = bt_gatt_discover(g_conn, &discover_params);
	if (err != 0) {
		FAIL("Discover failed(err %d)\n", err);
	}
}

BT_GATT_SERVICE_DEFINE(g_svc,
	BT_GATT_PRIMARY_SERVICE(TEST_SERVICE_UUID),
	BT_GATT_CHARACTERISTIC(TEST_CHRC_UUID, BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ, NULL, NULL, NULL),
	BT_GATT_CCC(NULL,
		    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE));

static void test_main(void)
{
	int err;

	device_sync_init(PERIPHERAL_ID);

	err = bt_enable(NULL);
	if (err != 0) {
		FAIL("Bluetooth enable failed (err %d)\n", err);
	}

	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, device_found);
	if (err != 0) {
		FAIL("Scanning failed to start (err %d)\n", err);
	}

	printk("Scanning successfully started\n");

	WAIT_FOR_FLAG(flag_is_connected);

	err = bt_conn_set_security(g_conn, BT_SECURITY_L2);
	if (err) {
		FAIL("Failed to start encryption procedure\n");
	}

	WAIT_FOR_FLAG(flag_is_encrypted);

	err = bt_eatt_connect(g_conn, CONFIG_BT_EATT_MAX);
	if (err) {
		FAIL("Sending credit based connection request failed (err %d)\n", err);
	}

	/* Wait for the channels to be connected */
	while (bt_eatt_count(g_conn) < CONFIG_BT_EATT_MAX) {
		k_sleep(K_TICKS(1));
	}

	printk("Waiting for sync\n");
	device_sync_wait();

	local_attr = &g_svc.attrs[1];

	printk("############# Notification test\n");
	for (int idx = 0; idx < NUM_NOTIF; idx++) {
		printk("Notification %d\n", idx);
		send_notification();
	}

	printk("############# Disconnect and reconnect\n");
	for (int idx = 0; idx < CONFIG_BT_EATT_MAX; idx++) {
		bt_eatt_disconnect_one(g_conn);
		while (bt_eatt_count(g_conn) != (CONFIG_BT_EATT_MAX - idx)) {
			k_sleep(K_TICKS(1));
		}
	}

	printk("Connecting %d bearers\n", EATT_BEARERS_TEST);
	err = bt_eatt_connect(g_conn, EATT_BEARERS_TEST);
	if (err) {
		FAIL("Sending credit based connection request failed (err %d)\n", err);
	}

	/* Wait for the channels to be connected */
	while (bt_eatt_count(g_conn) < EATT_BEARERS_TEST) {
		k_sleep(K_TICKS(1));
	}

	printk("############# Send notifications during discovery request\n");
	gatt_discover();
	while (!TEST_FLAG(flag_discover_complete)) {
		printk("Notifying...\n");
		send_notification();
	}

	printk("Sending final sync\n");
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
