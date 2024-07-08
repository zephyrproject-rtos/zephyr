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

/* local includes */
#include "data.h"

LOG_MODULE_REGISTER(dut, CONFIG_APP_LOG_LEVEL);

#define NUM_TESTERS CONFIG_BT_MAX_CONN

/* This test will fail when CONFIG_BT_MAX_CONN == CONFIG_BT_BUF_ACL_RX_COUNT */
BUILD_ASSERT(CONFIG_BT_BUF_ACL_RX_COUNT == CONFIG_BT_MAX_CONN);

struct tester {
	size_t sdu_count;
	struct bt_conn *conn;
	struct bt_l2cap_le_chan le_chan;
};

static struct tester testers[NUM_TESTERS];

static struct tester *get_tester(struct bt_conn *conn)
{
	for (size_t i = 0; i < ARRAY_SIZE(testers); i++) {
		if (testers[i].conn == conn) {
			return &testers[i];
		}
	}

	return NULL;
}

static void sent_cb(struct bt_l2cap_chan *chan)
{
	TEST_FAIL("Tester should not send data");
}

static int recv_cb(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	char addr[BT_ADDR_LE_STR_LEN];
	struct tester *tester = get_tester(chan->conn);

	tester->sdu_count += 1;

	bt_addr_le_to_str(bt_conn_get_dst(chan->conn), addr, sizeof(addr));

	LOG_INF("Received SDU %d / %d from (%s)", tester->sdu_count, SDU_NUM, addr);

	return 0;
}

static void l2cap_chan_connected_cb(struct bt_l2cap_chan *chan)
{
	LOG_DBG("%p", chan);
}

static void l2cap_chan_disconnected_cb(struct bt_l2cap_chan *chan)
{
	LOG_DBG("%p", chan);
}

static int server_accept_cb(struct bt_conn *conn, struct bt_l2cap_server *server,
			    struct bt_l2cap_chan **chan)
{
	static struct bt_l2cap_chan_ops ops = {
		.connected = l2cap_chan_connected_cb,
		.disconnected = l2cap_chan_disconnected_cb,
		.recv = recv_cb,
		.sent = sent_cb,
	};

	struct tester *tester = get_tester(conn);
	struct bt_l2cap_le_chan *le_chan = &tester->le_chan;

	memset(le_chan, 0, sizeof(*le_chan));
	le_chan->chan.ops = &ops;
	*chan = &le_chan->chan;

	return 0;
}

static int l2cap_server_register(bt_security_t sec_level)
{
	static struct bt_l2cap_server test_l2cap_server = {.accept = server_accept_cb};

	test_l2cap_server.psm = L2CAP_TEST_PSM;
	test_l2cap_server.sec_level = sec_level;

	int err = bt_l2cap_server_register(&test_l2cap_server);

	TEST_ASSERT(err == 0, "Failed to register l2cap server (err %d)", err);

	return test_l2cap_server.psm;
}

static struct bt_conn *connect_tester(void)
{
	int err;
	bt_addr_le_t tester = {};
	struct bt_conn *conn = NULL;
	char addr[BT_ADDR_LE_STR_LEN];

	/* The device address will not change. Scan only once in order to reduce
	 * test time.
	 */
	err = bt_testlib_scan_find_name(&tester, TESTER_NAME);
	TEST_ASSERT(!err, "Failed to start scan (err %d)", err);

	/* Create a connection using that address */
	err = bt_testlib_connect(&tester, &conn);
	TEST_ASSERT(!err, "Failed to initiate connection (err %d)", err);

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	LOG_DBG("Connected to %s", addr);

	return conn;
}

static bool all_data_transferred(void)
{
	size_t total_sdu_count = 0;

	for (size_t i = 0; i < ARRAY_SIZE(testers); i++) {
		total_sdu_count += testers[i].sdu_count;
	}

	TEST_ASSERT(total_sdu_count <= (SDU_NUM * NUM_TESTERS), "Received more SDUs than expected");

	return total_sdu_count == (SDU_NUM * NUM_TESTERS);
}

void entrypoint_dut(void)
{
	/* Multilink Host Flow Control (HFC) test
	 *
	 * Test purpose:
	 *
	 * Verifies that we are able to do L2CAP recombination on multiple links
	 * when we only have as many buffers as links.
	 *
	 * Devices:
	 * - `dut`: receives L2CAP PDUs from testers
	 * - `tester`: send ACL packets (parts of large L2CAP PDU) very slowly
	 *
	 * Procedure:
	 *
	 * DUT:
	 * - establish connection to tester
	 * - [acl connected]
	 * - establish L2CAP channel
	 * - [l2 connected]
	 * - receive L2CAP PDUs until SDU_NUM is reached
	 * - mark test as passed and terminate simulation
	 *
	 * tester 0/1/2:
	 * - scan & connect ACL
	 * - [acl connected]
	 * - [l2cap dynamic channel connected]
	 * (and then in a loop)
	 * - send part of L2CAP PDU
	 * - wait a set amount of time
	 * - exit loop when SDU_NUM sent
	 *
	 * [verdict]
	 * - dut application is able to receive all expected L2CAP packets from
	 *   the testers
	 */
	int err;

	/* Mark test as in progress. */
	TEST_START("dut");

	/* Initialize Bluetooth */
	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Can't enable Bluetooth (err %d)", err);

	LOG_DBG("Bluetooth initialized");

	int psm = l2cap_server_register(BT_SECURITY_L1);

	LOG_DBG("Registered server PSM %x", psm);

	for (size_t i = 0; i < ARRAY_SIZE(testers); i++) {
		LOG_DBG("Connecting tester %d", i);
		testers[i].sdu_count = 0;
		testers[i].conn = connect_tester();
	}

	LOG_DBG("Connected all testers");

	while (!all_data_transferred()) {
		/* Wait until we have received all expected data. */
		k_sleep(K_MSEC(100));
	}

	TEST_PASS_AND_EXIT("dut");
}
