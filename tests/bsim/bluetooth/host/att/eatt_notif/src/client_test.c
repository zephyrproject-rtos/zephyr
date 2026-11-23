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

#include <stddef.h>
#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/att.h>

#include "babblekit/testcase.h"
#include "babblekit/flags.h"
#include "babblekit/sync.h"
#include "common.h"

DEFINE_FLAG_STATIC(flag_is_connected);
DEFINE_FLAG_STATIC(flag_discover_complete);
DEFINE_FLAG_STATIC(flag_is_encrypted);
DEFINE_FLAG_STATIC(flag_mtu_exchanged);
DEFINE_FLAG_STATIC(flag_small_sent);
DEFINE_FLAG_STATIC(flag_small_indicated);

static struct bt_conn *g_conn;
static const struct bt_gatt_attr *local_attr;
static const struct bt_uuid *test_svc_uuid = TEST_SERVICE_UUID;

#define NUM_NOTIF 100
#define SAMPLE_DATA 1
#define EATT_BEARERS_TEST 1

volatile int num_eatt_channels;

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
	int err;

	if (g_conn != NULL) {
		return;
	}

	/* We're only interested in connectable events */
	if (type != BT_HCI_ADV_IND && type != BT_HCI_ADV_DIRECT_IND) {
		return;
	}

	printk("Device found: %s (RSSI %d)\n", bt_addr_le_str(addr), rssi);

	printk("Stopping scan\n");
	err = bt_le_scan_stop();
	if (err != 0) {
		TEST_FAIL("Could not stop scan: %d");
		return;
	}

	err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN,
				BT_LE_CONN_PARAM_DEFAULT, &g_conn);
	if (err != 0) {
		TEST_FAIL("Could not connect to peer: %d", err);
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
		TEST_FAIL("Discover failed(err %d)", err);
	}
}

BT_GATT_SERVICE_DEFINE(g_svc,
	BT_GATT_PRIMARY_SERVICE(TEST_SERVICE_UUID),
	BT_GATT_CHARACTERISTIC(TEST_CHRC_UUID, BT_GATT_CHRC_NOTIFY | BT_GATT_CHRC_INDICATE,
			       BT_GATT_PERM_READ, NULL, NULL, NULL),
	BT_GATT_CCC(NULL,
		    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE));

static void test_main(void)
{
	int err;

	TEST_ASSERT(bk_sync_init() == 0, "Failed to open backchannel");

	err = bt_enable(NULL);
	if (err != 0) {
		TEST_FAIL("Bluetooth enable failed (err %d)", err);
	}

	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, device_found);
	if (err != 0) {
		TEST_FAIL("Scanning failed to start (err %d)", err);
	}

	printk("Scanning successfully started\n");

	WAIT_FOR_FLAG(flag_is_connected);

	err = bt_conn_set_security(g_conn, BT_SECURITY_L2);
	if (err) {
		TEST_FAIL("Failed to start encryption procedure");
	}

	WAIT_FOR_FLAG(flag_is_encrypted);

	err = bt_eatt_connect(g_conn, CONFIG_BT_EATT_MAX);
	if (err) {
		TEST_FAIL("Sending credit based connection request failed (err %d)", err);
	}

	/* Wait for the channels to be connected */
	while (bt_eatt_count(g_conn) < CONFIG_BT_EATT_MAX) {
		k_sleep(K_TICKS(1));
	}

	printk("Waiting for sync\n");
	bk_sync_wait();

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
		TEST_FAIL("Sending credit based connection request failed (err %d)", err);
	}

	/* Wait for the channels to be connected */
	while (bt_eatt_count(g_conn) < EATT_BEARERS_TEST) {
		k_sleep(K_TICKS(1));
	}

	printk("############# Send notifications during discovery request\n");
	gatt_discover();
	while (!IS_FLAG_SET(flag_discover_complete)) {
		printk("Notifying...\n");
		send_notification();
	}

	printk("Sending final sync\n");
	bk_sync_send();

	TEST_PASS("Client Passed");
}

static void exchange_func(struct bt_conn *conn, uint8_t err,
			  struct bt_gatt_exchange_params *params)
{
	if (err != 0) {
		TEST_FAIL("MTU exchange failed (err %u)", err);
	}

	SET_FLAG(flag_mtu_exchanged);
}

static void small_sent(struct bt_conn *conn, void *user_data)
{
	SET_FLAG(flag_small_sent);
}

/* Connect, optionally exchange the ATT_MTU, connect the EATT bearers and wait
 * for the peer to subscribe.
 */
static void mtu_test_setup(bool exchange_mtu)
{
	static struct bt_gatt_exchange_params exchange_params = {
		.func = exchange_func,
	};
	int err;

	TEST_ASSERT(bk_sync_init() == 0, "Failed to open backchannel");

	err = bt_enable(NULL);
	if (err != 0) {
		TEST_FAIL("Bluetooth enable failed (err %d)", err);
	}

	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, device_found);
	if (err != 0) {
		TEST_FAIL("Scanning failed to start (err %d)", err);
	}

	WAIT_FOR_FLAG(flag_is_connected);

	if (exchange_mtu) {
		err = bt_gatt_exchange_mtu(g_conn, &exchange_params);
		if (err != 0) {
			TEST_FAIL("MTU exchange failed (err %d)", err);
		}

		WAIT_FOR_FLAG(flag_mtu_exchanged);
	}

	err = bt_conn_set_security(g_conn, BT_SECURITY_L2);
	if (err != 0) {
		TEST_FAIL("Failed to start encryption procedure");
	}

	WAIT_FOR_FLAG(flag_is_encrypted);

	err = bt_eatt_connect(g_conn, CONFIG_BT_EATT_MAX);
	if (err != 0) {
		TEST_FAIL("Sending credit based connection request failed (err %d)", err);
	}

	while (bt_eatt_count(g_conn) < CONFIG_BT_EATT_MAX) {
		k_sleep(K_TICKS(1));
	}

	/* Wait for the peer to subscribe */
	bk_sync_wait();
}

static void test_mtu(void)
{
	static const uint8_t data[100];
	struct bt_gatt_notify_params params = {
		.attr = &g_svc.attrs[1],
		.data = data,
		.chan_opt = BT_ATT_CHAN_OPT_ENHANCED_ONLY,
	};
	int err;

	mtu_test_setup(true);

	/* With this configuration the EATT bearers have a smaller ATT_MTU than
	 * the UATT bearer, so a notification that fills the UATT ATT_MTU fits
	 * no bearer it is restricted to. It is accepted as the UATT bearer is
	 * large enough (#119817), and must not hold up the notification after
	 * it.
	 */
	params.len = bt_gatt_get_uatt_mtu(g_conn) - 3U;
	TEST_ASSERT(params.len <= sizeof(data), "UATT MTU too large");
	err = bt_gatt_notify_cb(g_conn, &params);
	if (err != 0) {
		TEST_FAIL("Notification of %u octets failed (err %d)", params.len, err);
	}

	params.len = 1U;
	params.func = small_sent;
	err = bt_gatt_notify_cb(g_conn, &params);
	if (err != 0) {
		TEST_FAIL("Notification failed (err %d)", err);
	}

	WAIT_FOR_FLAG(flag_small_sent);

	TEST_PASS("Client Passed");
}

static void small_indicated(struct bt_conn *conn, struct bt_gatt_indicate_params *params,
			    uint8_t err)
{
	if (err != 0U) {
		TEST_FAIL("Indication failed (err 0x%02x)", err);
	}

	SET_FLAG(flag_small_indicated);
}

/* Indicate a value whose PDU fills @p mtu, which no bearer that @p chan_opt
 * allows can carry, and then a one-octet value, which must not wait for the
 * first one.
 */
static void indicate_beyond_mtu(enum bt_att_chan_opt chan_opt, uint16_t mtu)
{
	static const uint8_t data[100];
	static struct bt_gatt_indicate_params large;
	static struct bt_gatt_indicate_params small;
	int err;

	large.attr = &g_svc.attrs[1];
	large.data = data;
	large.len = mtu - 3U;
	large.chan_opt = chan_opt;
	TEST_ASSERT(large.len <= sizeof(data), "ATT_MTU too large");

	err = bt_gatt_indicate(g_conn, &large);
	if (err != 0) {
		TEST_FAIL("Indication of %u octets failed (err %d)", large.len, err);
	}

	small.attr = &g_svc.attrs[1];
	small.data = data;
	small.len = 1U;
	small.func = small_indicated;
	small.chan_opt = chan_opt;

	err = bt_gatt_indicate(g_conn, &small);
	if (err != 0) {
		TEST_FAIL("Indication failed (err %d)", err);
	}

	WAIT_FOR_FLAG(flag_small_indicated);
}

static void test_ind_mtu(void)
{
	mtu_test_setup(true);

	/* An indication restricted to EATT that fills the UATT ATT_MTU, which
	 * is larger than that of the EATT bearers here.
	 */
	indicate_beyond_mtu(BT_ATT_CHAN_OPT_ENHANCED_ONLY, bt_gatt_get_uatt_mtu(g_conn));

	TEST_PASS("Client Passed");
}

static void test_uatt_ind_mtu(void)
{
	mtu_test_setup(false);

	/* Without an ATT_MTU exchange the UATT bearer keeps the default ATT_MTU,
	 * which is smaller than that of the EATT bearers. An indication
	 * restricted to UATT that fills the EATT ATT_MTU is accepted all the
	 * same.
	 */
	TEST_ASSERT(bt_gatt_get_uatt_mtu(g_conn) < bt_gatt_get_mtu(g_conn),
		    "UATT MTU not smaller than EATT MTU");
	indicate_beyond_mtu(BT_ATT_CHAN_OPT_UNENHANCED_ONLY, bt_gatt_get_mtu(g_conn));

	TEST_PASS("Client Passed");
}

static const struct bst_test_instance test_vcs[] = {
	{
		.test_id = "client",
		.test_main_f = test_main
	},
	{
		.test_id = "client_mtu",
		.test_main_f = test_mtu
	},
	{
		.test_id = "client_ind_mtu",
		.test_main_f = test_ind_mtu
	},
	{
		.test_id = "client_uatt_ind_mtu",
		.test_main_f = test_uatt_ind_mtu
	},
	BSTEST_END_MARKER
};

struct bst_test_list *test_client_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_vcs);
}
