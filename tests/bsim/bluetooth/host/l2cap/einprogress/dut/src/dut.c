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
#include <zephyr/net/buf.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util_macro.h>

#include "testlib/conn.h"
#include "testlib/scan.h"

#include "babblekit/flags.h"
#include "babblekit/testcase.h"

/* local includes */
#include "data.h"

LOG_MODULE_REGISTER(dut, CONFIG_APP_LOG_LEVEL);

struct tester_t {
	size_t sdu_count;
	struct bt_conn *conn;
	struct bt_l2cap_le_chan le_chan;
	struct k_fifo ack_todo;
};

static atomic_t acl_pool_refs_held[CONFIG_BT_BUF_ACL_RX_COUNT];
static struct tester_t tester;

BUILD_ASSERT(IS_ENABLED(CONFIG_BT_TESTING));
BUILD_ASSERT(IS_ENABLED(CONFIG_BT_HCI_ACL_FLOW_CONTROL));
void bt_testing_trace_event_acl_pool_destroy(struct net_buf *destroyed_buf)
{
	int buf_id = net_buf_id(destroyed_buf);

	__ASSERT_NO_MSG(0 <= buf_id && buf_id < ARRAY_SIZE(acl_pool_refs_held));
	TEST_ASSERT(acl_pool_refs_held[buf_id] == 0,
		    "ACL buf was destroyed while tester still held a reference");
}

static void acl_pool_refs_held_add(struct net_buf *buf)
{
	int buf_id = net_buf_id(buf);

	__ASSERT_NO_MSG(0 <= buf_id && buf_id < CONFIG_BT_BUF_ACL_RX_COUNT);
	atomic_inc(&acl_pool_refs_held[buf_id]);
}

static void acl_pool_refs_held_remove(struct net_buf *buf)
{
	int buf_id = net_buf_id(buf);

	__ASSERT_NO_MSG(0 <= buf_id && buf_id < ARRAY_SIZE(acl_pool_refs_held));
	atomic_val_t old = atomic_dec(&acl_pool_refs_held[buf_id]);

	__ASSERT(old != 0, "Tester error: releasing a reference that was not held");
}

static struct tester_t *get_tester(struct bt_conn *conn)
{
	if (tester.conn == conn) {
		return &tester;
	}

	return NULL;
}

static int recv_cb(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	char addr[BT_ADDR_LE_STR_LEN];
	struct tester_t *tester = get_tester(chan->conn);

	tester->sdu_count += 1;

	bt_addr_le_to_str(bt_conn_get_dst(chan->conn), addr, sizeof(addr));

	LOG_INF("Received SDU %d / %d from (%s)", tester->sdu_count, SDU_NUM, addr);

	/* Move buf. Ownership is ours if we return -EINPROGRESS. */
	acl_pool_refs_held_add(buf);
	net_buf_put(&tester->ack_todo, buf);

	return -EINPROGRESS;
}

static int server_accept_cb(struct bt_conn *conn, struct bt_l2cap_server *server,
			    struct bt_l2cap_chan **chan)
{
	static struct bt_l2cap_chan_ops ops = {
		.recv = recv_cb,
	};

	struct tester_t *tester = get_tester(conn);
	struct bt_l2cap_le_chan *le_chan = &tester->le_chan;

	k_fifo_init(&tester->ack_todo);
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

	total_sdu_count += tester.sdu_count;

	TEST_ASSERT(total_sdu_count <= SDU_NUM, "Received more SDUs than expected");

	return total_sdu_count == SDU_NUM;
}

void entrypoint_dut(void)
{
	/* Test reference counting in Host when using L2CAP -EINPROGRESS feature.
	 *
	 * Devices:
	 * - `dut`: receives L2CAP PDUs from tester
	 * - `tester`: send L2CAP packets
	 *
	 * Procedure:
	 *
	 * DUT:
	 * - establish connection to tester
	 * - [acl connected]
	 * - establish L2CAP channel
	 * - [l2 connected]
	 * - receive one L2CAP SDU, returning EINPROGRESS
	 * - monitor that our SDU reference is respected
	 * - return the buf to the stack using bt_l2cap_chan_recv_complete
	 * - mark test as passed and terminate simulation
	 *
	 * tester 0:
	 * - scan & connect ACL
	 * - [acl connected]
	 * - [l2cap dynamic channel connected]
	 * (and then in a loop)
	 * - send part of L2CAP PDU
	 * - wait a set amount of time
	 * - exit loop when SDU_NUM sent
	 *
	 * [verdict]
	 * - retained SDU buffer reference from recv() is respected by stack
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

	LOG_DBG("Connecting tester");
	tester.sdu_count = 0;
	tester.conn = connect_tester();

	LOG_DBG("Connected all testers");

	while (!all_data_transferred()) {
		struct net_buf *ack_buf;

		/* Wait until we have received all expected data. */
		k_sleep(K_MSEC(100));

		ack_buf = net_buf_get(&tester.ack_todo, K_NO_WAIT);
		if (ack_buf) {
			acl_pool_refs_held_remove(ack_buf);
			err = bt_l2cap_chan_recv_complete(&tester.le_chan.chan, ack_buf);
			TEST_ASSERT(!err);
		}
	}

	TEST_PASS_AND_EXIT("dut");
}
