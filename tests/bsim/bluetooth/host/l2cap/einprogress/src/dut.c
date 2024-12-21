/* Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/bluetooth/testing.h>
#include <zephyr/logging/log.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util_macro.h>

#include <testlib/addr.h>
#include <testlib/adv.h>
#include <testlib/conn.h>
#include <testlib/scan.h>

#include <babblekit/flags.h>
#include <babblekit/testcase.h>

#include "data.h"

LOG_MODULE_REGISTER(dut, LOG_LEVEL_INF);

/** Here we keep track of the reference count in the test
 *  application. This allows us to notice if the stack has freed
 *  references that were ours.
 */
static atomic_t acl_pool_refs_held[CONFIG_BT_BUF_ACL_RX_COUNT];

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

struct k_fifo ack_todo;

static int dut_chan_recv_cb(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	/* Move buf. Ownership is ours if we return -EINPROGRESS. */
	acl_pool_refs_held_add(buf);
	k_fifo_put(&ack_todo, buf);

	return -EINPROGRESS;
}

static const struct bt_l2cap_chan_ops ops = {
	.recv = dut_chan_recv_cb,
};

static struct bt_l2cap_le_chan le_chan = {
	.chan.ops = &ops,
};

static int dut_server_accept_cb(struct bt_conn *conn, struct bt_l2cap_server *server,
				struct bt_l2cap_chan **chan)
{
	*chan = &le_chan.chan;
	return 0;
}

static struct bt_l2cap_server test_l2cap_server = {
	.accept = dut_server_accept_cb,
	.psm = TEST_DATA_L2CAP_PSM,
};

void entrypoint_dut(void)
{
	struct net_buf *ack_buf;
	struct bt_conn *conn = NULL;
	int err;

	TEST_START("dut");

	k_fifo_init(&ack_todo);

	err = bt_id_create(&TEST_DATA_DUT_ADDR, NULL);
	__ASSERT_NO_MSG(!err);

	err = bt_enable(NULL);
	__ASSERT_NO_MSG(!err);

	err = bt_l2cap_server_register(&test_l2cap_server);
	__ASSERT_NO_MSG(!err);

	err = bt_testlib_adv_conn(&conn, BT_ID_DEFAULT, NULL);
	__ASSERT_NO_MSG(!err);

	ack_buf = k_fifo_get(&ack_todo, K_FOREVER);
	__ASSERT_NO_MSG(ack_buf);

	acl_pool_refs_held_remove(ack_buf);
	err = bt_l2cap_chan_recv_complete(&le_chan.chan, ack_buf);
	TEST_ASSERT(!err);

	TEST_PASS_AND_EXIT("dut");
}
