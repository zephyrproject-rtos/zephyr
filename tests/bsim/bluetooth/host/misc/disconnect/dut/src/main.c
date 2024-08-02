/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
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

#include "utils.h"
#include "sync.h"
#include "bstests.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dut, LOG_LEVEL_INF);

DEFINE_FLAG(is_connected);
DEFINE_FLAG(is_subscribed);
DEFINE_FLAG(flag_data_length_updated);

static atomic_t notifications;

/* Defined in hci_core.c */
extern k_tid_t bt_testing_tx_tid_get(void);

static struct bt_conn *dconn;

static void connected(struct bt_conn *conn, uint8_t conn_err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (conn_err) {
		FAIL("Failed to connect to %s (%u)", addr, conn_err);
		return;
	}

	LOG_INF("%s: %s", __func__, addr);

	dconn = bt_conn_ref(conn);
	SET_FLAG(is_connected);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_INF("%s: %p %s (reason 0x%02x)", __func__, conn, addr, reason);

	bt_conn_unref(dconn);
	UNSET_FLAG(is_connected);
}

static void data_len_updated(struct bt_conn *conn,
			     struct bt_conn_le_data_len_info *info)
{
	LOG_DBG("Data length updated: TX %d RX %d",
		info->tx_max_len,
		info->rx_max_len);
	SET_FLAG(flag_data_length_updated);
}

static void do_dlu(void)
{
	int err;
	struct bt_conn_le_data_len_param param;

	param.tx_max_len = CONFIG_BT_CTLR_DATA_LENGTH_MAX;
	param.tx_max_time = 2500;

	err = bt_conn_le_data_len_update(dconn, &param);
	ASSERT(err == 0, "Can't update data length (err %d)\n", err);

	WAIT_FOR_FLAG(flag_data_length_updated);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
	.le_data_len_updated = data_len_updated,
};

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			 struct net_buf_simple *ad)
{
	char str[BT_ADDR_LE_STR_LEN];
	struct bt_le_conn_param *param;
	struct bt_conn *conn;
	int err;

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
		FAIL("Create conn failed (err %d)", err);
		return;
	}
}

static void connect(void)
{
	int err;
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
	LOG_INF("Connected as central");

	/* No security support on the tinyhost unfortunately */
}

static uint8_t notified(struct bt_conn *conn, struct bt_gatt_subscribe_params *params,
			const void *data, uint16_t length)
{
	static uint8_t notification[] = NOTIFICATION_PAYLOAD;
	static uint8_t indication[] = INDICATION_PAYLOAD;
	bool is_nfy;

	LOG_HEXDUMP_DBG(data, length, "HVx data");

	if (length == 0) {
		/* The host's backward way of telling us we are unsubscribed
		 * from this characteristic.
		 */
		LOG_DBG("Unsubscribed");
		return BT_GATT_ITER_CONTINUE;
	}

	ASSERT(length >= sizeof(indication), "Unexpected data\n");
	ASSERT(length <= sizeof(notification), "Unexpected data\n");

	is_nfy = memcmp(data, notification, length) == 0;

	LOG_INF("%s from 0x%x", is_nfy ? "notified" : "indicated",
		params->value_handle);

	ASSERT(is_nfy, "Unexpected indication\n");

	atomic_inc(&notifications);

	if (atomic_get(&notifications) == 3) {
		LOG_INF("##################### BRB..");
		backchannel_sync_send();

		/* Make scheduler rotate us in and out multiple times */
		for (int i = 0; i < 10; i++) {
			LOG_DBG("sleep");
			k_sleep(K_MSEC(100));
			LOG_DBG("sleep");
		}

		LOG_INF("##################### ..back to work");
	}

	return BT_GATT_ITER_CONTINUE;
}

static void subscribed(struct bt_conn *conn,
		       uint8_t err,
		       struct bt_gatt_subscribe_params *params)
{
	ASSERT(!err, "Subscribe failed (err %d)\n", err);

	ASSERT(params, "params is NULL\n");

	SET_FLAG(is_subscribed);
	/* spoiler: tester doesn't really have attributes */
	LOG_INF("Subscribed to Tester attribute");
}

void subscribe(void)
{
	int err;

	/* Handle values don't matter, as long as they match on the tester */
	static struct bt_gatt_subscribe_params params = {
		.notify = notified,
		.subscribe = subscribed,
		.value = BT_GATT_CCC_NOTIFY | BT_GATT_CCC_INDICATE,
		.value_handle = HVX_HANDLE,
		.ccc_handle = (HVX_HANDLE + 1),
	};

	err = bt_gatt_subscribe(dconn, &params);
	ASSERT(!err, "Subscribe failed (err %d)\n", err);

	WAIT_FOR_FLAG(is_subscribed);
}

void test_procedure_0(void)
{
	ASSERT(backchannel_init() == 0, "Failed to open backchannel\n");

	LOG_DBG("Test start: ATT disconnect protocol");
	int err;

	err = bt_enable(NULL);
	ASSERT(err == 0, "Can't enable Bluetooth (err %d)\n", err);
	LOG_DBG("Central: Bluetooth initialized.");

	/* Test purpose:
	 * Make sure the host handles long blocking in notify callbacks
	 * gracefully, especially in the case of a disconnect while waiting.
	 *
	 * Test procedure:
	 *
	 * [setup]
	 * - connect ACL, DUT is central and GATT client
	 * - update data length (tinyhost doens't have recombination)
	 * - dut: subscribe to NOTIFY on tester CHRC
	 *
	 * [procedure]
	 * - tester: start periodic notifications
	 * - dut: wait 10x 100ms in notification RX callback
	 * - tester: disconnect (not gracefully) while DUT is waiting
	 *   -> simulates a power or range loss situation
	 * - dut: exit notification callback
	 * - dut: wait for `disconnected` conn callback
	 *
	 * [verdict]
	 * - The DUT gets the `disconnected` callback, no hanging or timeouts.
	 */
	connect();
	subscribe();

	do_dlu();

	WAIT_FOR_EXPR(notifications, < 4);

	WAIT_FOR_FLAG_UNSET(is_connected);

	LOG_INF("##################### END TEST #####################");

	PASS("DUT exit\n");
}

void test_tick(bs_time_t HW_device_time)
{
	bs_trace_debug_time(0, "Simulation ends now.\n");
	if (bst_result != Passed) {
		bst_result = Failed;
		bs_trace_error("Test did not pass before simulation ended.\n");
	}
}

void test_init(void)
{
	bst_ticker_set_next_tick_absolute(TEST_TIMEOUT_SIMULATED);
	bst_result = In_progress;
}

static const struct bst_test_instance test_to_add[] = {
	{
		.test_id = "dut",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_procedure_0,
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
