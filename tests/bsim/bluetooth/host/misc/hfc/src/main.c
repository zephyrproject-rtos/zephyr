/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/bluetooth/att.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/conn.h>

#include "testlib/att_read.h"

#include <argparse.h>		/* For get_device_nbr() */
#include "utils.h"
#include "bstests.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

DEFINE_FLAG(is_connected);
DEFINE_FLAG(is_subscribed);

/* Default connection */
static struct bt_conn *dconn;

#define NUM_NOTIFICATIONS 200

struct dut_state {
	struct bt_conn *conn;
	int rx;
};

static struct dut_state g_dut_state;

static void connected(struct bt_conn *conn, uint8_t conn_err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (conn_err) {
		FAIL("Failed to connect to %s (%u)", addr, conn_err);
		return;
	}

	LOG_DBG("%s", addr);

	dconn = bt_conn_ref(conn);
	SET_FLAG(is_connected);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_DBG("%p %s (reason 0x%02x)", conn, addr, reason);

	UNSET_FLAG(is_connected);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			 struct net_buf_simple *ad)
{
	char str[BT_ADDR_LE_STR_LEN];
	struct bt_le_conn_param *param;
	struct bt_conn *conn;
	int err;

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, addr);
	if (conn) {
		LOG_DBG("Old connection is not yet purged");
		bt_conn_unref(conn);
		return;
	}

	err = bt_le_scan_stop();
	if (err) {
		FAIL("Stop LE scan failed (err %d)", err);
		return;
	}

	bt_addr_le_to_str(addr, str, sizeof(str));
	LOG_DBG("Connecting to %s", str);

	param = BT_LE_CONN_PARAM_DEFAULT;
	err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN, param, &conn);
	if (err) {
		k_oops();
		FAIL("Create conn failed (err %d)", err);
		return;
	}
}

static struct bt_conn *connect_as_peripheral(void)
{
	int err;
	struct bt_conn *conn;

	UNSET_FLAG(is_connected);

	err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, NULL, 0, NULL, 0);
	ASSERT(!err, "Adving failed to start (err %d)\n", err);

	LOG_DBG("advertising");
	WAIT_FOR_FLAG(is_connected);
	LOG_DBG("connected as peripheral");

	conn = dconn;
	dconn = NULL;

	return conn;
}

static struct bt_conn *connect_as_central(void)
{
	int err;
	struct bt_conn *conn;
	struct bt_le_scan_param scan_param = {
		.type = BT_LE_SCAN_TYPE_ACTIVE,
		.options = BT_LE_SCAN_OPT_NONE,
		.interval = BT_GAP_SCAN_FAST_INTERVAL,
		.window = BT_GAP_SCAN_FAST_WINDOW,
	};

	UNSET_FLAG(is_connected);

	err = bt_le_scan_start(&scan_param, device_found);
	ASSERT(!err, "Scanning failed to start (err %d)\n", err);

	LOG_DBG("Central initiating connection...");
	WAIT_FOR_FLAG(is_connected);
	LOG_DBG("Connected as central");

	conn = dconn;
	dconn = NULL;

	return conn;
}

static void find_the_chrc(struct bt_conn *conn,
			  const struct bt_uuid *svc,
			  const struct bt_uuid *chrc,
			  uint16_t *chrc_value_handle)
{
	uint16_t svc_handle;
	uint16_t svc_end_handle;
	uint16_t chrc_end_handle;
	int err;

	err = bt_testlib_gatt_discover_primary(&svc_handle, &svc_end_handle, conn, svc, 1, 0xffff);
	ASSERT(!err, "Failed to discover service %d");

	LOG_DBG("svc_handle: %u, svc_end_handle: %u", svc_handle, svc_end_handle);

	err = bt_testlib_gatt_discover_characteristic(chrc_value_handle, &chrc_end_handle,
						      NULL, conn, chrc, (svc_handle + 1),
						      svc_end_handle);
	ASSERT(!err, "Failed to get value handle %d");

	LOG_DBG("chrc_value_handle: %u, chrc_end_handle: %u", *chrc_value_handle, chrc_end_handle);
}

static uint8_t notified(struct bt_conn *conn,
			struct bt_gatt_subscribe_params *params,
			const void *data,
			uint16_t length)
{
	struct dut_state *s = &g_dut_state;

	if (length) {
		s->rx++;
		LOG_DBG("peripheral RX: %d", s->rx);
	}

	/* Sleep to increase the chance RX buffers are still held by the host
	 * when we get a disconnection event from the LL.
	 */
	k_msleep(100);

	return BT_GATT_ITER_CONTINUE;
}

static void subscribed(struct bt_conn *conn,
		       uint8_t err,
		       struct bt_gatt_subscribe_params *params)
{
	ASSERT(!err, "Subscribe failed (err %d)\n", err);

	ASSERT(params, "params is NULL\n");

	SET_FLAG(is_subscribed);

	LOG_DBG("Subscribed to peer attribute (conn %p)", conn);
}

static void subscribe(struct bt_conn *conn,
		      uint16_t handle,
		      bt_gatt_notify_func_t cb)
{
	int err;
	static struct bt_gatt_subscribe_params params = {};

	params.notify = cb;
	params.subscribe = subscribed;
	params.value = BT_GATT_CCC_NOTIFY;
	params.value_handle = handle;
	params.ccc_handle = handle + 1;

	err = bt_gatt_subscribe(conn, &params);
	ASSERT(!err, "Subscribe failed (err %d)\n", err);

	WAIT_FOR_FLAG(is_subscribed);
}

static void ccc_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	/* assume we only get it for the `test_gatt_service` */
	if (value) {
		SET_FLAG(is_subscribed);
	}
}

#define test_service_uuid                                                                          \
	BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0xf0debc9a, 0x7856, 0x3412, 0x7856, 0x341278563412))
#define test_characteristic_uuid                                                                   \
	BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0xf2debc9a, 0x7856, 0x3412, 0x7856, 0x341278563412))

BT_GATT_SERVICE_DEFINE(test_gatt_service, BT_GATT_PRIMARY_SERVICE(test_service_uuid),
		       BT_GATT_CHARACTERISTIC(test_characteristic_uuid,
					      (BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE |
					       BT_GATT_CHRC_NOTIFY | BT_GATT_CHRC_INDICATE),
					      BT_GATT_PERM_READ | BT_GATT_PERM_WRITE, NULL, NULL,
					      NULL),
		       BT_GATT_CCC(ccc_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),);

static struct bt_conn *connect_and_subscribe(void)
{
	uint16_t handle;
	struct bt_conn *conn;

	LOG_DBG("Central: Connect to peer");
	conn = connect_as_central();

	LOG_DBG("Central: Subscribe to peer (conn %p)", conn);
	find_the_chrc(conn, test_service_uuid, test_characteristic_uuid, &handle);
	subscribe(conn, handle, notified);

	return conn;
}

static bool is_disconnected(struct bt_conn *conn)
{
	int err;
	struct bt_conn_info info;

	err = bt_conn_get_info(conn, &info);
	ASSERT(err == 0, "Failed to get info for %p\n", conn);

	/* Return if fully disconnected */
	return info.state == BT_CONN_STATE_DISCONNECTED;
}

static int disconnect(struct bt_conn *conn)
{
	int err;

	if (!conn) {
		return -ENOTCONN;
	}

	err = bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);

	if (!err) {
		WAIT_FOR_FLAG_UNSET(is_connected);
	}

	return err;
}

static void entrypoint_dut(void)
{
	/* Test purpose:
	 *
	 * Verifies that there is no host RX buffer leak due to disconnections.
	 *
	 * That is, not actual host buffers (ie memory) but rather the number of
	 * free buffers that the controller thinks the host has.
	 *
	 * If there is a desynchronization between those two, the result is that
	 * the controller stops forwarding ACL data to the host, leading to an
	 * eventual application timeout.
	 *
	 * To do this, the DUT is connected to a peer that loops through sending
	 * a few ATT notifications then disconnecting.
	 *
	 * The test stops after an arbitrary number of notifications have been
	 * received.
	 *
	 * [verdict]
	 * - no buffer allocation failures, timeouts or stalls.
	 */
	int err;
	struct dut_state *s = &g_dut_state;

	LOG_DBG("Test start: DUT");

	s->rx = 0;

	err = bt_enable(NULL);
	ASSERT(err == 0, "Can't enable Bluetooth (err %d)\n", err);
	LOG_DBG("Central: Bluetooth initialized.");

	s->conn = connect_and_subscribe();

	LOG_DBG("Central: Connected and subscribed to both peers");

	/* Wait until we got all notifications from both peers */
	while (s->rx < NUM_NOTIFICATIONS) {
		LOG_DBG("%d packets left, waiting..",
			NUM_NOTIFICATIONS - s->rx);
		k_msleep(100);
		if ((s->rx < NUM_NOTIFICATIONS) && is_disconnected(s->conn)) {
			LOG_INF("reconnecting..");
			/* release the ref we took in the `connected` callback */
			bt_conn_unref(s->conn);

			/* release the ref we took when starting the scanner */
			bt_conn_unref(s->conn);

			s->conn = NULL;
			s->conn = connect_and_subscribe();
		}
	}

	/* linux will "unref" the conn :p */
	disconnect(s->conn);

	PASS("DUT done\n");
}

static void entrypoint_peer(void)
{
	int err;
	int tx;
	struct bt_conn *conn;
	const struct bt_gatt_attr *attr;
	uint8_t data[10];

	LOG_DBG("Test start: peer 0");

	err = bt_enable(NULL);
	ASSERT(err == 0, "Can't enable Bluetooth (err %d)\n", err);
	LOG_DBG("Bluetooth initialized.");

	/* prepare data for notifications */
	attr = &test_gatt_service.attrs[2];
	memset(data, 0xfe, sizeof(data));

	/* Pass unless something else errors out later */
	PASS("peer 0 done\n");

	tx = 0;
	while (true) {
		conn = connect_as_peripheral();

		LOG_INF("wait until DUT subscribes");
		UNSET_FLAG(is_subscribed);
		WAIT_FOR_FLAG(is_subscribed);

		LOG_INF("send notifications");
		for (int i = 0; i < 10; i++) {
			err = 1;
			while (err) {
				LOG_DBG("p0: TX %d", tx);
				err = bt_gatt_notify(conn, attr, data, sizeof(data));
			}
			tx++;
		}

		k_msleep(50);

		LOG_INF("disconnect");
		err = disconnect(conn);
		ASSERT(!err, "Failed to initate disconnect (err %d)", err);
		bt_conn_unref(conn);
		conn = NULL;
	}
}

static void test_tick(bs_time_t HW_device_time)
{
	bs_trace_debug_time(0, "Simulation ends now.\n");
	if (bst_result != Passed) {
		bst_result = Failed;
		bs_trace_error("Test did not pass before simulation ended.\n");
	}
}

static void test_init(void)
{
	bst_ticker_set_next_tick_absolute(TEST_TIMEOUT_SIMULATED);
	bst_result = In_progress;
}

static const struct bst_test_instance test_to_add[] = {
	{
		.test_id = "dut",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = entrypoint_dut,
	},
	{
		.test_id = "peer",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = entrypoint_peer,
	},
	BSTEST_END_MARKER,
};

static struct bst_test_list *install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_to_add);
};

bst_test_install_t test_installers[] = {install, NULL};

int main(void)
{
	bst_main();

	return 0;
}
