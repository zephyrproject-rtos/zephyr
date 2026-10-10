/*
 * Copyright (c) 2026 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* The receiver of an LE credit-based channel gives 0 initial credits and
 * grants the first one from its connected callback. The sender queues its
 * first SDU from its own connected callback, before it has any credits. All
 * SDUs must be sent once the credits arrive, and the channel must be reported
 * sendable only while it has credits.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/atomic.h>

#include "babblekit/flags.h"
#include "babblekit/testcase.h"
#include "bstests.h"

#define PSM     0x0080
#define SDU_NUM 3
#define SDU_LEN 32

NET_BUF_POOL_DEFINE(sdu_pool, SDU_NUM, BT_L2CAP_SDU_BUF_SIZE(SDU_LEN),
		    CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

DEFINE_FLAG_STATIC(is_connected);
DEFINE_FLAG_STATIC(chan_connected);
DEFINE_FLAG_STATIC(chan_sendable);
DEFINE_FLAG_STATIC(all_sent);
DEFINE_FLAG_STATIC(all_received);

static struct bt_conn *default_conn;
static struct bt_l2cap_le_chan le_chan;
static uint8_t sent_cnt;
static uint8_t rx_cnt;

static void send_sdu(struct bt_l2cap_chan *chan, uint8_t index)
{
	struct net_buf *buf;
	int err;

	buf = net_buf_alloc(&sdu_pool, K_NO_WAIT);
	TEST_ASSERT(buf != NULL, "No buffer for SDU %u", index);

	net_buf_reserve(buf, BT_L2CAP_SDU_CHAN_SEND_RESERVE);
	(void)memset(net_buf_add(buf, SDU_LEN), index, SDU_LEN);

	err = bt_l2cap_chan_send(chan, buf);
	TEST_ASSERT(err >= 0, "Failed to send SDU %u (err %d)", index, err);
}

static void sender_connected(struct bt_l2cap_chan *chan)
{
	SET_FLAG(chan_connected);

	/* No credits yet: the SDU waits in the queue until the peer grants some */
	send_sdu(chan, 0);
}

static void sender_status(struct bt_l2cap_chan *chan, atomic_t *status)
{
	if (atomic_test_bit(status, BT_L2CAP_STATUS_OUT)) {
		TEST_ASSERT(atomic_get(&le_chan.tx.credits) > 0, "Sendable without credits");
		SET_FLAG(chan_sendable);
	}
}

static void sender_sent(struct bt_l2cap_chan *chan)
{
	sent_cnt++;

	if (sent_cnt == SDU_NUM) {
		SET_FLAG(all_sent);
		return;
	}

	send_sdu(chan, sent_cnt);
}

static int sender_recv(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	TEST_FAIL("Unexpected data on the sender");

	return 0;
}

static const struct bt_l2cap_chan_ops sender_ops = {
	.connected = sender_connected,
	.status = sender_status,
	.sent = sender_sent,
	.recv = sender_recv,
};

static void receiver_connected(struct bt_l2cap_chan *chan)
{
	int err;

	SET_FLAG(chan_connected);

	err = bt_l2cap_chan_give_credits(chan, 1);
	TEST_ASSERT(err == 0, "Failed to give credits (err %d)", err);
}

static void receiver_seg_recv(struct bt_l2cap_chan *chan, size_t sdu_len, off_t seg_offset,
			      struct net_buf_simple *seg)
{
	uint8_t expected[SDU_LEN];
	int err;

	TEST_ASSERT(sdu_len == SDU_LEN && seg_offset == 0 && seg->len == SDU_LEN,
		    "Unexpected segment: SDU length %zu offset %ld length %u", sdu_len,
		    (long)seg_offset, seg->len);

	(void)memset(expected, rx_cnt, sizeof(expected));
	TEST_ASSERT(memcmp(seg->data, expected, sizeof(expected)) == 0, "Unexpected data in SDU %u",
		    rx_cnt);

	rx_cnt++;

	if (rx_cnt == SDU_NUM) {
		SET_FLAG(all_received);
		return;
	}

	err = bt_l2cap_chan_give_credits(chan, 1);
	TEST_ASSERT(err == 0, "Failed to give credits (err %d)", err);
}

static const struct bt_l2cap_chan_ops receiver_ops = {
	.connected = receiver_connected,
	.seg_recv = receiver_seg_recv,
};

static int accept(struct bt_conn *conn, struct bt_l2cap_server *server, struct bt_l2cap_chan **chan)
{
	*chan = &le_chan.chan;

	return 0;
}

static struct bt_l2cap_server server = {
	.psm = PSM,
	.sec_level = BT_SECURITY_L1,
	.accept = accept,
};

static void connected(struct bt_conn *conn, uint8_t err)
{
	TEST_ASSERT(err == 0, "Failed to connect (err 0x%02x)", err);

	default_conn = bt_conn_ref(conn);
	SET_FLAG(is_connected);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	bt_conn_drop(&default_conn);
	UNSET_FLAG(is_connected);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			 struct net_buf_simple *ad)
{
	struct bt_conn *conn = NULL;
	int err;

	err = bt_le_scan_stop();
	TEST_ASSERT(err == 0, "Failed to stop scanning (err %d)", err);

	err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN, BT_LE_CONN_PARAM_DEFAULT, &conn);
	TEST_ASSERT(err == 0, "Failed to create connection (err %d)", err);

	bt_conn_unref(conn);
}

static void run_test(bool central, bool receiver)
{
	int err;

	if (receiver) {
		/* With seg_recv the initial credits are what rx.credits holds: 0 */
		le_chan.chan.ops = &receiver_ops;
		le_chan.rx.mtu = SDU_LEN;
		le_chan.rx.mps = BT_L2CAP_RX_MTU;
	} else {
		le_chan.chan.ops = &sender_ops;
	}

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Bluetooth init failed (err %d)", err);

	if (central) {
		err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, device_found);
		TEST_ASSERT(err == 0, "Scanning failed to start (err %d)", err);

		WAIT_FOR_FLAG(is_connected);

		err = bt_l2cap_chan_connect(default_conn, &le_chan.chan, PSM);
		TEST_ASSERT(err == 0, "Failed to connect channel (err %d)", err);
	} else {
		err = bt_l2cap_server_register(&server);
		TEST_ASSERT(err == 0, "Failed to register server (err %d)", err);

		err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, NULL, 0, NULL, 0);
		TEST_ASSERT(err == 0, "Advertising failed to start (err %d)", err);

		WAIT_FOR_FLAG(is_connected);
	}

	WAIT_FOR_FLAG(chan_connected);

	if (receiver) {
		WAIT_FOR_FLAG(all_received);
	} else {
		WAIT_FOR_FLAG(all_sent);
		TEST_ASSERT(IS_FLAG_SET(chan_sendable), "Never reported sendable");

		err = bt_conn_disconnect(default_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
		TEST_ASSERT(err == 0, "Failed to disconnect (err %d)", err);
	}

	WAIT_FOR_FLAG_UNSET(is_connected);

	TEST_PASS("%s %s passed", central ? "Central" : "Peripheral",
		  receiver ? "receiver" : "sender");
}

static void test_central_receiver(void)
{
	run_test(true, true);
}

static void test_central_sender(void)
{
	run_test(true, false);
}

static void test_peripheral_receiver(void)
{
	run_test(false, true);
}

static void test_peripheral_sender(void)
{
	run_test(false, false);
}

static const struct bst_test_instance test_def[] = {
	{
		.test_id = "central_receiver",
		.test_descr = "Central, channel initiator, 0 initial credits",
		.test_main_f = test_central_receiver,
	},
	{
		.test_id = "central_sender",
		.test_descr = "Central, channel initiator, sends",
		.test_main_f = test_central_sender,
	},
	{
		.test_id = "peripheral_receiver",
		.test_descr = "Peripheral, channel acceptor, 0 initial credits",
		.test_main_f = test_peripheral_receiver,
	},
	{
		.test_id = "peripheral_sender",
		.test_descr = "Peripheral, channel acceptor, sends",
		.test_main_f = test_peripheral_sender,
	},
	BSTEST_END_MARKER,
};

static struct bst_test_list *test_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_def);
}

bst_test_install_t test_installers[] = {test_install, NULL};

int main(void)
{
	bst_main();

	return 0;
}
