/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/logging/log.h>

#include "testlib/conn.h"
#include "testlib/scan.h"

#include "babblekit/flags.h"
#include "babblekit/testcase.h"

LOG_MODULE_REGISTER(dut, CONFIG_APP_LOG_LEVEL);

static atomic_t disconnected_channels;

static struct bt_l2cap_le_chan chans[4];

void sent_cb(struct bt_l2cap_chan *chan)
{
	LOG_DBG("%p", chan);
}

int recv_cb(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	LOG_DBG("%p", chan);

	return 0;
}

void l2cap_chan_connected_cb(struct bt_l2cap_chan *l2cap_chan)
{
	TEST_ASSERT("This shouldn't happen");
}

void l2cap_chan_disconnected_cb(struct bt_l2cap_chan *chan)
{
	atomic_inc(&disconnected_channels);
}

static struct bt_l2cap_chan_ops ops = {
	.connected = l2cap_chan_connected_cb,
	.disconnected = l2cap_chan_disconnected_cb,
	.recv = recv_cb,
	.sent = sent_cb,
};

void entrypoint_dut(void)
{
	/* Test purpose:
	 *
	 * Verify that a peer that doesn't support ECRED channels doesn't result
	 * in us keeping half-open channels.
	 *
	 * Two devices:
	 * - `dut`: tries to establish 4 ecred chans
	 * - `peer`: rejects the request
	 *
	 * Initial conditions:
	 * - Both devices are connected
	 *
	 * Procedure:
	 * - [dut] request to establish 4 ecred channels
	 * - [peer] reject command as unknown
	 * - [dut] get `disconnected` called on all 4 channels
	 *
	 * [verdict]
	 * - each channel gets the `disconnected` callback called
	 */
	int err;
	bt_addr_le_t peer = {};
	struct bt_conn *conn = NULL;

	TEST_START("dut");

	/* Initialize Bluetooth */
	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Can't enable Bluetooth (err %d)", err);
	LOG_DBG("Bluetooth initialized");

	err = bt_testlib_scan_find_name(&peer, "ecred_peer");
	TEST_ASSERT(!err, "Failed to start scan (err %d)", err);

	/* Create a connection using that address */
	err = bt_testlib_connect(&peer, &conn);
	TEST_ASSERT(!err, "Failed to initiate connection (err %d)", err);

	LOG_DBG("Connected");

	LOG_INF("Send ECRED connection request");
	struct bt_l2cap_chan *chan_list[5] = {0};

	for (int i = 0; i < 4; i++) {
		/* Register the callbacks */
		chans[i].chan.ops = &ops;

		/* Add the channel to the connection request list */
		chan_list[i] = &chans[i].chan;
	}

	/* The PSM doesn't matter, as the peer doesn't support the command */
	err = bt_l2cap_ecred_chan_connect(conn, chan_list, 0x0080);
	TEST_ASSERT(!err, "Error connecting l2cap channels (err %d)\n", err);

	LOG_INF("Wait until peer rejects the channel establishment request");
	while (atomic_get(&disconnected_channels) < 4) {
		k_sleep(K_MSEC(10));
	}

	TEST_PASS_AND_EXIT("dut");
}
