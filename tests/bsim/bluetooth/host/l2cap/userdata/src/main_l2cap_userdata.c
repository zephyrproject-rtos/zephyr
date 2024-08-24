/*
 * Copyright (c) 2022 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common.h"

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/l2cap.h>

extern enum bst_result_t bst_result;

static struct bt_conn *default_conn;

#define PSM 0x80
#define DATA_SIZE 500
#define USER_DATA_SIZE 10

/* Pool to allocate a buffer that is too large to send */
NET_BUF_POOL_DEFINE(buf_pool, 1, BT_L2CAP_SDU_BUF_SIZE(DATA_SIZE), USER_DATA_SIZE, NULL);

CREATE_FLAG(is_connected);
CREATE_FLAG(is_sent);
CREATE_FLAG(has_received);
CREATE_FLAG(chan_connected);

static void chan_connected_cb(struct bt_l2cap_chan *l2cap_chan)
{
	(void)l2cap_chan;

	SET_FLAG(chan_connected);
}

static void chan_disconnected_cb(struct bt_l2cap_chan *l2cap_chan)
{
	(void)l2cap_chan;

	UNSET_FLAG(chan_connected);
}

struct net_buf *alloc_buf_cb(struct bt_l2cap_chan *chan)
{
	return net_buf_alloc(&buf_pool, K_NO_WAIT);
}

static int chan_recv_cb(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	ARG_UNUSED(chan);
	ARG_UNUSED(buf);

	SET_FLAG(has_received);

	return 0;
}

void sent_cb(struct bt_l2cap_chan *chan)
{
	SET_FLAG(is_sent);
}

static const struct bt_l2cap_chan_ops l2cap_ops = {
	.connected = chan_connected_cb,
	.disconnected = chan_disconnected_cb,
	.recv = chan_recv_cb,
	.sent = sent_cb,
	.alloc_buf = alloc_buf_cb,
};

static struct bt_l2cap_le_chan channel;

static int accept(struct bt_conn *conn, struct bt_l2cap_server *server,
		  struct bt_l2cap_chan **l2cap_chan)
{
	channel.chan.ops = &l2cap_ops;
	*l2cap_chan = &channel.chan;
	channel.rx.mtu = DATA_SIZE;

	return 0;
}

static struct bt_l2cap_server server = {
	.accept = accept,
	.sec_level = BT_SECURITY_L1,
	.psm = PSM,
};

static void connect_l2cap_channel(void)
{
	struct bt_l2cap_chan *chans[] = {&channel.chan, NULL};
	int err;

	channel.chan.ops = &l2cap_ops;

	channel.rx.mtu = DATA_SIZE;

	err = bt_l2cap_ecred_chan_connect(default_conn, chans, server.psm);
	if (err) {
		FAIL("Failed to send ecred connection request (err %d)\n", err);
	}
}

static void register_l2cap_server(void)
{
	int err;

	err = bt_l2cap_server_register(&server);
	if (err < 0) {
		FAIL("Failed to get free server (err %d)\n");
		return;
	}
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err) {
		FAIL("Failed to connect (err %d)\n", err);
		bt_conn_unref(default_conn);
		default_conn = NULL;
		return;
	}

	default_conn = bt_conn_ref(conn);

	SET_FLAG(is_connected);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	if (default_conn != conn) {
		FAIL("Connection mismatch %p %p)\n", default_conn, conn);
		return;
	}

	bt_conn_unref(default_conn);
	default_conn = NULL;
	UNSET_FLAG(is_connected);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			 struct net_buf_simple *ad)
{
	struct bt_le_conn_param *param;
	int err;

	err = bt_le_scan_stop();
	if (err) {
		FAIL("Failed to stop scanning (err %d)\n", err);
		return;
	}

	param = BT_LE_CONN_PARAM_DEFAULT;
	err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN, param, &default_conn);
	if (err) {
		FAIL("Failed to create connection (err %d)\n", err);
		return;
	}
}

static void test_peripheral_main(void)
{
	int err;
	const struct bt_data ad[] = {
		BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	};

	err = bt_enable(NULL);
	if (err != 0) {
		FAIL("Bluetooth init failed (err %d)\n", err);
		return;
	}

	err = bt_le_adv_start(BT_LE_ADV_CONN_ONE_TIME, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err != 0) {
		FAIL("Advertising failed to start (err %d)\n", err);
		return;
	}

	WAIT_FOR_FLAG_SET(is_connected);

	register_l2cap_server();

	if (!IS_ENABLED(CONFIG_NO_RUNTIME_CHECKS)) {
		PASS("Peripheral done\n");
		return;
	}

	WAIT_FOR_FLAG_SET(has_received);

	/* /\* Wait until we get the credits *\/ */
	/* k_sleep(K_MSEC(100)); */

	err = bt_conn_disconnect(default_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	if (err) {
		FAIL("Failed to disconnect (err %d)\n", err);
		return;
	}

	PASS("Test passed\n");
}

#define FILL	       0xAA

static void print_user_data(struct net_buf *buf)
{
	for (int i = 0; i < buf->user_data_size; i++) {
		printk("%02X", buf->user_data[i]);
	}
	printk("\n");
}

static void test_central_main(void)
{
	struct net_buf *buf;
	int err;
	bool has_checks = !IS_ENABLED(CONFIG_NO_RUNTIME_CHECKS);

	printk("##################\n");
	printk("(%s-checks) Starting test\n",
	       has_checks ? "Enabled" : "Disabled");

	err = bt_enable(NULL);
	if (err != 0) {
		FAIL("Bluetooth discover failed (err %d)\n", err);
	}

	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, device_found);
	if (err != 0) {
		FAIL("Scanning failed to start (err %d)\n", err);
	}

	WAIT_FOR_FLAG_SET(is_connected);

	connect_l2cap_channel();
	WAIT_FOR_FLAG_SET(chan_connected);

	buf = net_buf_alloc(&buf_pool, K_NO_WAIT);
	if (!buf) {
		FAIL("Buffer allcation failed\n");
	}

	net_buf_reserve(buf, BT_L2CAP_SDU_CHAN_SEND_RESERVE);

	(void)net_buf_add(buf, DATA_SIZE);
	/* Fill the user data with a non-zero pattern */
	(void)memset(buf->user_data, FILL, buf->user_data_size);

	printk("Buffer user_data before\n");
	print_user_data(buf);

	/* Send the buffer. We don't care that the other side receives it.
	 * Only about when we will get called.
	 */
	err = bt_l2cap_chan_send(&channel.chan, buf);

	/* L2CAP will take our `buf` with non-null user_data. We verify that:
	 * - it is cleared
	 * - we don't segfault later (e.g. in `tx_notify`)
	 */

	if (err != 0) {
		FAIL("Got error %d\n", err);
	}

	WAIT_FOR_FLAG_SET(is_sent);

	printk("Buffer user_data after (should've been cleared)\n");
	print_user_data(buf);

	printk("\n");

	/* Validate that the user data has changed */
	for (int i = 0; i < USER_DATA_SIZE; i++) {
		if (buf->user_data[i] == FILL) {
			FAIL("Buffer user data should be reset by stack.\n");
		}
	}

	PASS("(Disabled-checks) Test passed\n");
}

static const struct bst_test_instance test_def[] = {
	{
		.test_id = "peripheral",
		.test_descr = "Peripheral",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_peripheral_main,
	},
	{
		.test_id = "central",
		.test_descr = "Central",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_central_main,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_main_l2cap_ecred_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_def);
}
