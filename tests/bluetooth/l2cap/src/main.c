/* main.c - Application main entry point */

/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <stddef.h>
#include <ztest.h>

#include <bluetooth/buf.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>

/* Semaphore to test when connect completes  */
static K_SEM_DEFINE(conn_sem, 0, 1);

/* Semaphore to test when disconnect completes  */
static K_SEM_DEFINE(disc_sem, 0, 1);

/* Semaphore to test when sent completes  */
static K_SEM_DEFINE(sent_sem, 0, 1);

/* Semaphore to test when recv completes  */
static K_SEM_DEFINE(recv_sem, 0, 1);

#define DATA_MTU CONFIG_BT_L2CAP_TX_MTU - BT_L2CAP_CHAN_SEND_RESERVE
static u8_t data[DATA_MTU] = { [0 ... (DATA_MTU - 1)] = 0xff };

static void l2cap_connected(struct bt_l2cap_chan *chan)
{
	k_sem_give(&conn_sem);
}

static void l2cap_disconnected(struct bt_l2cap_chan *chan)
{
	k_sem_give(&disc_sem);
}

static void l2cap_sent(struct bt_l2cap_chan *chan)
{
	k_sem_give(&sent_sem);
}

static int l2cap_recv(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	zassert_true(net_buf_frags_len(buf) == sizeof(data),
		     "Test channel unexpected len received");

	zassert_false(memcmp(buf->data, data, sizeof(data)),
		     "Test channel received data doesn't match");

	k_sem_give(&recv_sem);

	return 0;
}

static struct bt_l2cap_chan_ops l2cap_ops = {
	.sent = l2cap_sent,
	.recv = l2cap_recv,
	.connected = l2cap_connected,
	.disconnected = l2cap_disconnected,
};

static struct bt_l2cap_le_chan test_ch = { .chan.ops = &l2cap_ops };

static int l2cap_accept(struct bt_conn *conn, struct bt_l2cap_chan **chan)
{
	if (test_ch.chan.conn) {
		return -ENOMEM;
	}

	*chan = &test_ch.chan;

	return 0;
}

static struct bt_l2cap_server test_server = {
	.accept		= l2cap_accept,
};

static struct bt_l2cap_le_chan test_fixed_ch = { .chan.ops = &l2cap_ops };

static int l2cap_fixed_accept(struct bt_conn *conn, struct bt_l2cap_chan **chan)
{
	if (test_fixed_ch.chan.conn) {
		return -ENOMEM;
	}

	*chan = &test_fixed_ch.chan;

	return 0;
}

static struct bt_l2cap_server test_fixed_server = {
	.accept		= l2cap_fixed_accept,
	.psm		= 0x007f,
};

static struct bt_l2cap_le_chan test_dyn_ch = { .chan.ops = &l2cap_ops };

static int l2cap_dyn_accept(struct bt_conn *conn, struct bt_l2cap_chan **chan)
{
	if (test_dyn_ch.chan.conn) {
		return -ENOMEM;
	}

	*chan = &test_dyn_ch.chan;

	return 0;
}

static struct bt_l2cap_server test_dyn_server = {
	.accept		= l2cap_dyn_accept,
	.psm		= 0x00ff,
};

static struct bt_l2cap_server test_inv_server = {
	.accept		= l2cap_accept,
	.psm		= 0xffff,
};

void test_l2cap_register(void)
{
	/* Attempt to register server with PSM auto allocation */
	zassert_false(bt_l2cap_server_register(&test_server),
		     "Test server registration failed");

	/* Attempt to register server with fixed PSM */
	zassert_false(bt_l2cap_server_register(&test_fixed_server),
		     "Test fixed PSM server registration failed");

	/* Attempt to register server with dynamic PSM */
	zassert_false(bt_l2cap_server_register(&test_dyn_server),
		     "Test dynamic PSM server registration failed");

	/* Attempt to register server with invalid PSM */
	zassert_true(bt_l2cap_server_register(&test_inv_server),
		     "Test invalid PSM server registration succeeded");

	/* Attempt to re-register server with PSM auto allocation */
	zassert_true(bt_l2cap_server_register(&test_server),
		     "Test server duplicate succeeded");

	/* Attempt to re-register server with fixed PSM */
	zassert_true(bt_l2cap_server_register(&test_fixed_server),
		     "Test fixed PSM server duplicate succeeded");

	/* Attempt to re-register server with dynamic PSM */
	zassert_true(bt_l2cap_server_register(&test_dyn_server),
		     "Test dynamic PSM server duplicate succeeded");
}

static struct bt_l2cap_le_chan le_ch = { .chan.ops = &l2cap_ops };

void test_l2cap_connect(void)
{
	struct bt_conn *conn;

	conn = bt_conn_create_le(BT_ADDR_LE_NONE, BT_LE_CONN_PARAM_DEFAULT);

	zassert_false(!conn, "Test loopback connection failed");

	/* Attempt to connect the auto allocated PSM channel */
	zassert_false(bt_l2cap_chan_connect(conn, &le_ch.chan, test_server.psm),
		     "Test channel connect failed");

	zassert_false(!le_ch.chan.conn, "Test channel connection not set");

	zassert_false(k_sem_take(&conn_sem, K_MSEC(100)),
		       "Tests channel connected not called within timeout");

	zassert_true(le_ch.chan.state == BT_L2CAP_CONNECTED,
		     "Test channel unexpected state");
}

NET_BUF_POOL_DEFINE(data_tx_pool, 1, DATA_MTU, BT_BUF_USER_DATA_MIN, NULL);

void test_l2cap_send_recv(void)
{
	struct net_buf *buf;

	buf = net_buf_alloc(&data_tx_pool, K_FOREVER);
	net_buf_reserve(buf, BT_L2CAP_CHAN_SEND_RESERVE);
	net_buf_add_mem(buf, data, sizeof(data));

	zassert_false(bt_l2cap_chan_send(&le_ch.chan, buf),
		     "Test channel disconnect failed");

	zassert_false(k_sem_take(&sent_sem, K_MSEC(100)),
		       "Tests channel sent not called within timeout");

	zassert_false(k_sem_take(&recv_sem, K_MSEC(100)),
		       "Tests channel recv not called within timeout");
}

void test_l2cap_disconnect(void)
{
	zassert_false(bt_l2cap_chan_disconnect(&le_ch.chan),
		     "Test channel disconnect failed");

	zassert_false(k_sem_take(&disc_sem, K_MSEC(100)),
		       "Tests channel disconnected not called within timeout");

	zassert_true(le_ch.chan.state == BT_L2CAP_DISCONNECTED,
		     "Test channel unexpected state");
}

/*test case main entry*/
void test_main(void)
{
	ztest_test_suite(test_l2cap,
			 ztest_unit_test(test_l2cap_register),
			 ztest_unit_test(test_l2cap_connect),
			 ztest_unit_test(test_l2cap_send_recv),
			 ztest_unit_test(test_l2cap_disconnect));
	ztest_run_test_suite(test_l2cap);
}
