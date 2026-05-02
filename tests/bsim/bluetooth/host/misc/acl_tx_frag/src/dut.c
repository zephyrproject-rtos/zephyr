/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/logging/log.h>

#include "testlib/conn.h"
#include "testlib/scan.h"
#include "testlib/log_utils.h"

#include "babblekit/flags.h"
#include "babblekit/testcase.h"

/* For the radio shenanigans */
#include "hw_testcheat_if.h"

/* local includes */
#include "data.h"

LOG_MODULE_REGISTER(dut, LOG_LEVEL_DBG);

DEFINE_FLAG_STATIC(is_subscribed);
DEFINE_FLAG_STATIC(mtu_has_been_exchanged);
DEFINE_FLAG_STATIC(conn_recycled);
DEFINE_FLAG_STATIC(conn_param_updated);
DEFINE_FLAG_STATIC(indicated);

extern unsigned long runtime_log_level;

static void recycled(void)
{
	LOG_DBG("");
	SET_FLAG(conn_recycled);
}

static void params_updated(struct bt_conn *conn, uint16_t interval, uint16_t latency,
			   uint16_t timeout)
{
	LOG_DBG("");
	SET_FLAG(conn_param_updated);
}

static struct bt_conn_cb conn_cbs = {
	.recycled = recycled,
	.le_param_updated = params_updated,
};

static void ccc_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	/* assume we only get it for the `test_gatt_service` */
	if (value != 0) {
		SET_FLAG(is_subscribed);
	} else {
		UNSET_FLAG(is_subscribed);
	}
}

BT_GATT_SERVICE_DEFINE(test_gatt_service, BT_GATT_PRIMARY_SERVICE(test_service_uuid),
		       BT_GATT_CHARACTERISTIC(test_characteristic_uuid,
					      (BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE |
					       BT_GATT_CHRC_NOTIFY | BT_GATT_CHRC_INDICATE),
					      BT_GATT_PERM_READ | BT_GATT_PERM_WRITE, NULL, NULL,
					      NULL),
		       BT_GATT_CCC(ccc_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE));

static void _mtu_exchanged(struct bt_conn *conn, uint8_t err,
			   struct bt_gatt_exchange_params *params)
{
	LOG_DBG("MTU exchanged");
	SET_FLAG(mtu_has_been_exchanged);
}

static void exchange_mtu(struct bt_conn *conn)
{
	int err;
	struct bt_gatt_exchange_params params = {
		.func = _mtu_exchanged,
	};

	UNSET_FLAG(mtu_has_been_exchanged);

	err = bt_gatt_exchange_mtu(conn, &params);
	TEST_ASSERT(!err, "Failed MTU exchange (err %d)", err);

	WAIT_FOR_FLAG(mtu_has_been_exchanged);
}

#define UPDATE_PARAM_INTERVAL_MIN 500
#define UPDATE_PARAM_INTERVAL_MAX 500
#define UPDATE_PARAM_LATENCY      1
#define UPDATE_PARAM_TIMEOUT      1000

static struct bt_le_conn_param update_params = {
	.interval_min = UPDATE_PARAM_INTERVAL_MIN,
	.interval_max = UPDATE_PARAM_INTERVAL_MAX,
	.latency = UPDATE_PARAM_LATENCY,
	.timeout = UPDATE_PARAM_TIMEOUT,
};

void slow_down_conn(struct bt_conn *conn)
{
	int err;

	UNSET_FLAG(conn_param_updated);
	err = bt_conn_le_param_update(conn, &update_params);
	TEST_ASSERT(!err, "Parameter update failed (err %d)", err);
	WAIT_FOR_FLAG(conn_param_updated);
}

static void make_peer_go_out_of_range(void)
{
	hw_radio_testcheat_set_tx_power_gain(-300);
	hw_radio_testcheat_set_rx_power_gain(-300);
}

static void make_peer_go_back_in_range(void)
{
	hw_radio_testcheat_set_tx_power_gain(+300);
	hw_radio_testcheat_set_rx_power_gain(+300);
}

void indicated_cb(struct bt_conn *conn, struct bt_gatt_indicate_params *params, uint8_t err)
{
	SET_FLAG(indicated);
}

static void params_struct_freed_cb(struct bt_gatt_indicate_params *params)
{
	k_free(params);
}

static int send_indication(struct bt_conn *conn, const struct bt_gatt_attr *attr, const void *data,
			   uint16_t len)
{
	struct bt_gatt_indicate_params *params = k_malloc(sizeof(struct bt_gatt_indicate_params));

	params->attr = attr;
	params->func = indicated_cb;
	params->destroy = params_struct_freed_cb;
	params->data = data;
	params->len = len;

	return bt_gatt_indicate(conn, params);
}

static const uint8_t notification_data[GATT_PAYLOAD_SIZE];

static void test_iteration(bt_addr_le_t *peer)
{
	int err;
	struct bt_conn *conn = NULL;
	const struct bt_gatt_attr *attr;

	/* Create a connection using that address */
	err = bt_testlib_connect(peer, &conn);
	TEST_ASSERT(!err, "Failed to initiate connection (err %d)", err);

	LOG_DBG("Connected");

	LOG_INF("Wait until peer subscribes");
	UNSET_FLAG(is_subscribed);
	WAIT_FOR_FLAG(is_subscribed);

	/* Prepare data for notifications
	 * attrs[0] is our service declaration
	 * attrs[1] is our characteristic declaration
	 * attrs[2] is our characteristic value
	 *
	 * We store a pointer for the characteristic value as that is the
	 * value we want to notify later.
	 *
	 * We could alternatively use `bt_gatt_notify_uuid()`.
	 */
	attr = &test_gatt_service.attrs[2];

	exchange_mtu(conn);

	slow_down_conn(conn);
	LOG_DBG("Updated params");

	LOG_INF("Send indication #1");
	UNSET_FLAG(indicated);
	err = send_indication(conn, attr, notification_data, sizeof(notification_data));
	TEST_ASSERT(!err, "Failed to send notification: err %d", err);
	LOG_DBG("Wait until peer confirms our first indication");
	WAIT_FOR_FLAG(indicated);

	LOG_INF("Send indication #2");
	UNSET_FLAG(indicated);
	err = send_indication(conn, attr, notification_data, sizeof(notification_data));
	TEST_ASSERT(!err, "Failed to send notification: err %d", err);

	LOG_DBG("Simulate RF connection loss");
	UNSET_FLAG(conn_recycled);
	make_peer_go_out_of_range();

	/* We will not access conn after this: give back our initial ref. */
	bt_testlib_conn_unref(&conn);
	WAIT_FOR_FLAG(conn_recycled);

	LOG_DBG("Connection object has been destroyed as expected");
	make_peer_go_back_in_range();
}

void entrypoint_dut(void)
{
	/* Test purpose:
	 *
	 * Verifies that we don't leak resources or mess up host state when a
	 * disconnection happens whilst the host is transmitting ACL fragments.
	 *
	 * To achieve that, we use the BabbleSim magic modem (see run.sh) to cut
	 * the RF link before we have sent all the ACL fragments the peer. We do
	 * want to send multiple fragments to the controller though, the
	 * important part is that the peer does not acknowledge them, so that
	 * the disconnection happens while the controller has its TX buffers
	 * full.
	 *
	 * Two devices:
	 * - `dut`: the device whose host we are testing
	 * - `peer`: anime side-character. not important.
	 *
	 * Procedure (for n iterations):
	 * - [dut] establish connection to `peer`
	 * - [peer] discover GATT and subscribe to the test characteristic
	 * - [dut] send long indication
	 * - [peer] wait for confirmation of indication
	 * - [dut] send another long indication
	 * - [dut] disconnect
	 *
	 * [verdict]
	 * - All test cycles complete
	 */
	int err;
	bt_addr_le_t peer = {};

	/* Mark test as in progress. */
	TEST_START("dut");

	/* Set the log level given by the `log_level` CLI argument */
	bt_testlib_log_level_set("dut", runtime_log_level);

	/* Initialize Bluetooth */
	bt_conn_cb_register(&conn_cbs);
	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Can't enable Bluetooth (err %d)", err);

	LOG_DBG("Bluetooth initialized");

	/* Find the address of the peer. In our case, both devices are actually
	 * the same executable (with the same config) but executed with
	 * different arguments. We can then just use CONFIG_BT_DEVICE_NAME which
	 * contains our device name in string form.
	 */
	err = bt_testlib_scan_find_name(&peer, CONFIG_BT_DEVICE_NAME);
	TEST_ASSERT(!err, "Failed to start scan (err %d)", err);

	for (int i = 0; i < TEST_ITERATIONS; i++) {
		LOG_INF("## Iteration %d", i);
		test_iteration(&peer);
	}

	TEST_PASS("dut");
}
