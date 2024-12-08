/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/logging/log.h>

#include "testlib/conn.h"
#include "testlib/scan.h"
#include "testlib/log_utils.h"

#include "babblekit/flags.h"
#include "babblekit/sync.h"
#include "babblekit/testcase.h"

/* local includes */
#include "data.h"

LOG_MODULE_REGISTER(dut, LOG_LEVEL_DBG);

static DEFINE_FLAG(is_subscribed);

extern unsigned long runtime_log_level;

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
		       BT_GATT_CCC(ccc_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),);

/* This is the entrypoint for the DUT.
 *
 * This is executed by the `bst_test` framework provided by the zephyr bsim
 * boards. The framework selects which "main" function to run as entrypoint
 * depending on the `-testid=` command-line parameter passed to the zephyr
 * executable.
 *
 * In our case, the `testid` is set to "dut" and `entrypoint_dut()` is mapped to
 * the "dut" ID in `entrypoints[]`.
 *
 * In our case we only have two entrypoints, as we only have a single test
 * involving two devices (so 1x 2 entrypoints). One can define more test cases
 * with different entrypoints and map them to different test ID strings in
 * `entrypoints[]`
 */
void entrypoint_dut(void)
{
	/* Please leave a comment indicating what the test is supposed to test,
	 * and what is the pass verdict. A nice place is at the beginning of
	 * each test entry point. Something like the following:
	 */

	/* Test purpose:
	 *
	 * Verifies that we are able to send a notification to the peer when
	 * `CONFIG_BT_GATT_ENFORCE_SUBSCRIPTION` is disabled and the peer has
	 * unsubscribed from the characteristic in question.
	 *
	 * Two devices:
	 * - `dut`: tries to send the notification
	 * - `peer`: will receive the notification
	 *
	 * Procedure:
	 * - [dut] establish connection to `peer`
	 * - [peer] discover GATT and subscribe to the test characteristic
	 * - [dut] send notification #1
	 * - [peer] wait for notification
	 * - [peer] unsubscribe
	 * - [dut] send notification #2
	 * - [peer] and [dut] pass test
	 *
	 * [verdict]
	 * - peer receives notifications #1 and #2
	 */
	int err;
	bt_addr_le_t peer = {};
	struct bt_conn *conn = NULL;
	const struct bt_gatt_attr *attr;
	uint8_t data[10];

	/* Mark test as in progress. */
	TEST_START("dut");

	/* Initialize device sync library */
	bk_sync_init();

	/* Set the log level given by the `log_level` CLI argument */
	bt_testlib_log_level_set("dut", runtime_log_level);

	/* Initialize Bluetooth */
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

	/* Create a connection using that address */
	err = bt_testlib_connect(&peer, &conn);
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

	LOG_INF("Send notification #1");
	LOG_HEXDUMP_DBG(data, sizeof(data), "Notification payload");

	err = bt_gatt_notify(conn, attr, payload_1, sizeof(payload_1));
	TEST_ASSERT(!err, "Failed to send notification: err %d", err);

	LOG_INF("Wait until peer unsubscribes");
	WAIT_FOR_FLAG_UNSET(is_subscribed);

	LOG_INF("Send notification #2");
	err = bt_gatt_notify(conn, attr, payload_2, sizeof(payload_2));
	TEST_ASSERT(!err, "Failed to send notification: err %d", err);

	/* We won't be using `conn` anymore */
	bt_conn_unref(conn);

	/* Wait until the peer has received notification #2.
	 *
	 * This is not strictly necessary, but serves as an example on how to
	 * use the backchannel-based synchronization mechanism between devices
	 * in a simulation.
	 */
	bk_sync_wait();

	/* Wait for the acknowledge of the DUT. If a device that uses
	 * backchannels exits prematurely (ie before the other side has read the
	 * message it sent), we are in undefined behavior territory.
	 *
	 * The simulation will continue running for its specified length.
	 *
	 * If you don't need backchannels, using `TEST_PASS_AND_EXIT()` is
	 * better as it will make the simulation exit prematurely, saving
	 * computing resources (CI compute time is not free).
	 */
	TEST_PASS("dut");
}
