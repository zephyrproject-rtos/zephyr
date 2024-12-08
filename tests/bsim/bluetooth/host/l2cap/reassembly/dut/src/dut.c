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

#include "testlib/att_read.h"
#include "testlib/att_write.h"
#include "testlib/conn.h"
#include "testlib/scan.h"
#include "testlib/log_utils.h"

#include "babblekit/flags.h"
#include "babblekit/testcase.h"

/* local includes */
#include "data.h"

LOG_MODULE_REGISTER(dut, LOG_LEVEL_DBG);

extern unsigned long runtime_log_level;

static DEFINE_FLAG(got_notification);

static uint8_t received_notification(struct bt_conn *conn,
				     struct bt_gatt_subscribe_params *params,
				     const void *data,
				     uint16_t length)
{
	if (length) {
		size_t expected_length = sizeof(NOTIFICATION_PAYLOAD);
		bool payload_is_correct = 0 == memcmp(data, NOTIFICATION_PAYLOAD, length);

		LOG_INF("Received notification");
		LOG_HEXDUMP_DBG(data, length, "payload");

		TEST_ASSERT(params->value_handle == GATT_HANDLE,
			    "Wrong handle used: expect 0x%x got 0x%x",
			    GATT_HANDLE, params->value_handle);
		TEST_ASSERT(length == expected_length,
			    "Length is incorrect: expect %d got %d",
			    expected_length, length);
		TEST_ASSERT(payload_is_correct, "Notification contents mismatch");

		SET_FLAG(got_notification);
	}

	return BT_GATT_ITER_CONTINUE;
}

/* Subscription parameters have the same lifetime as a subscription.
 * That is the backing struct should stay valid until a call to
 * `bt_gatt_unsubscribe()` is made. Hence the `static`.
 */
static struct bt_gatt_subscribe_params sub_params;

/* Link `cb` to notifications received from `peer` for `handle`. Using
 * `bt_gatt_resubscribe()` doesn't send anything on-air and just does the
 * linking in the host.
 */
static void fake_subscribe(bt_addr_le_t *peer,
			   uint16_t handle,
			   bt_gatt_notify_func_t cb)
{
	int err;

	/* Subscribe to notifications */
	sub_params.notify = cb;
	sub_params.value = BT_GATT_CCC_NOTIFY;
	sub_params.value_handle = handle;

	/* Doesn't matter for re-subscribe. */
	sub_params.ccc_handle = handle + 2;

	err = bt_gatt_resubscribe(0, peer, &sub_params);
	TEST_ASSERT(!err, "Subscribe failed (err %d)", err);
}

static void run_test_iteration(bt_addr_le_t *peer)
{
	int err;
	struct bt_conn *conn = NULL;

	/* Create a connection using that address */
	err = bt_testlib_connect(peer, &conn);
	TEST_ASSERT(!err, "Failed to initiate connection (err %d)", err);

	LOG_DBG("Connected");

	LOG_DBG("Subscribe to test characteristic: handle 0x%04x", GATT_HANDLE);
	fake_subscribe(peer, GATT_HANDLE, received_notification);

	WAIT_FOR_FLAG(got_notification);

	LOG_DBG("Wait for disconnection from peer");
	bt_testlib_wait_disconnected(conn);
	bt_testlib_conn_unref(&conn);
}

void entrypoint_dut(void)
{
	/* Test purpose:
	 *
	 * Verifies that the Host does not leak resources related to
	 * reassembling L2CAP PDUs when operating over an unreliable connection.
	 *
	 * Two devices:
	 * - `peer`: sends long GATT notifications
	 * - `dut`: receives long notifications from `peer`
	 *
	 * To do this, we configure the devices that ensures L2CAP PDUs are
	 * fragmented on-air over a long period. That mostly means smallest data
	 * length possible combined with a long connection interval.
	 *
	 * We try to disconnect when a PDU is mid-reassembly. This is slightly
	 * tricky to ensure: we rely that the implementation of the controller
	 * will forward PDU fragments as soon as they are received on-air.
	 *
	 * Procedure (loop 20x):
	 * - [dut] establish connection to `peer`
	 * - [peer] send notification #1
	 * - [dut] wait until notification #1 received
	 *
	 * - [peer] send 2 out of 3 frags of notification #2
	 * - [peer] disconnect
	 * - [dut] wait for disconnection
	 *
	 * [verdict]
	 * - dut receives notification #1 for all iterations
	 */
	int err;
	bt_addr_le_t peer = {};

	/* Mark test as in progress. */
	TEST_START("dut");

	/* Set the log level given by the `log_level` CLI argument */
	bt_testlib_log_level_set("dut", runtime_log_level);

	/* Initialize Bluetooth */
	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Can't enable Bluetooth (err %d)", err);

	LOG_DBG("Bluetooth initialized");

	/* Find the address of the peer, using its advertised name */
	err = bt_testlib_scan_find_name(&peer, "peer");
	TEST_ASSERT(!err, "Failed to start scan (err %d)", err);

	for (size_t i = 0; i < TEST_ITERATIONS; i++) {
		LOG_INF("## Iteration %d", i);
		run_test_iteration(&peer);
	}

	TEST_PASS("dut");
}
