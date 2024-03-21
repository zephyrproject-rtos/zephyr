/*
 * The goal of this test is to verify the buf->frags feature works with L2CAP
 * credit-based channels.
 *
 * Copyright (c) 2024 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bstests.h"
#include "common.h"

#define LOG_MODULE_NAME main
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME, LOG_LEVEL_DBG);

CREATE_FLAG(is_connected);
CREATE_FLAG(flag_l2cap_connected);

#define SMALL_BUF_SIZE 10
#define LARGE_BUF_SIZE 50
#define POOL_NUM 2
#define PAYLOAD_SIZE (POOL_NUM * (SMALL_BUF_SIZE + LARGE_BUF_SIZE))

#define L2CAP_MTU PAYLOAD_SIZE
#define PAYLOAD_NUM 3

NET_BUF_POOL_DEFINE(sdu_rx_pool, 1, BT_L2CAP_SDU_BUF_SIZE(L2CAP_MTU),
		    CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

static void small_buf_destroy(struct net_buf *buf)
{
	LOG_DBG("%p", buf);
	net_buf_destroy(buf);
}

NET_BUF_POOL_DEFINE(small_buf_pool, POOL_NUM, BT_L2CAP_SDU_BUF_SIZE(SMALL_BUF_SIZE),
		    CONFIG_BT_CONN_TX_USER_DATA_SIZE, small_buf_destroy);

static void large_buf_destroy(struct net_buf *buf)
{
	LOG_DBG("%p", buf);
	net_buf_destroy(buf);
}

NET_BUF_POOL_DEFINE(large_buf_pool, POOL_NUM, LARGE_BUF_SIZE,
		    CONFIG_BT_CONN_TX_USER_DATA_SIZE, large_buf_destroy);

static uint8_t tx_data[PAYLOAD_SIZE];
static uint16_t rx_cnt;

K_SEM_DEFINE(sdu_received, 0, 1);
K_SEM_DEFINE(tx_sem, 1, 1);

struct test_ctx {
	struct bt_l2cap_le_chan le_chan;
	size_t tx_remaining;
	struct net_buf *rx_sdu;
} test_ctx;

struct net_buf *alloc_and_memcpy(struct net_buf_pool *pool,
				 size_t reserve,
				 uint8_t *data,
				 size_t len)
{
	struct net_buf *buf = net_buf_alloc(pool, K_NO_WAIT);

	if (buf == NULL) {
		FAIL("No more memory\n");
		return NULL;
	}

	net_buf_reserve(buf, reserve);
	net_buf_add_mem(buf, data, len);

	return buf;
}

int l2cap_chan_send(struct bt_l2cap_chan *chan, uint8_t *data, size_t len)
{
	uint8_t *data_start = data;
	struct net_buf *buf, *b;
	int ret;

	LOG_DBG("chan %p conn %u data %p len %d",
		chan, bt_conn_index(chan->conn), data, len);

	if (k_sem_take(&tx_sem, K_NO_WAIT)) {
		FAIL("Already TXing\n");
		return -EAGAIN;
	}

	/* Payload is comprised of two small buffers and two big buffers. The
	 * very first buffer needs a reserve() called on it as per l2cap API
	 * requirements.
	 */
	buf = alloc_and_memcpy(&small_buf_pool,
			       BT_L2CAP_SDU_CHAN_SEND_RESERVE,
			       data, SMALL_BUF_SIZE);
	ASSERT(buf, "No more memory\n");

	data += SMALL_BUF_SIZE;

	b = alloc_and_memcpy(&small_buf_pool, 0, data, SMALL_BUF_SIZE);
	ASSERT(buf, "No more memory\n");

	LOG_DBG("append frag %p to buf %p", b, buf);
	net_buf_frag_add(buf, b);
	data += SMALL_BUF_SIZE;

	b = alloc_and_memcpy(&large_buf_pool, 0, data, LARGE_BUF_SIZE);
	ASSERT(buf, "No more memory\n");

	LOG_DBG("append frag %p to buf %p", b, buf);
	net_buf_frag_add(buf, b);
	data += LARGE_BUF_SIZE;

	b = alloc_and_memcpy(&large_buf_pool, 0, data, LARGE_BUF_SIZE);
	ASSERT(buf, "No more memory\n");

	LOG_DBG("append frag %p to buf %p", b, buf);
	net_buf_frag_add(buf, b);
	data += LARGE_BUF_SIZE;

	ASSERT(data == (data_start + len), "logic error\n");

	ret = bt_l2cap_chan_send(chan, buf);
	ASSERT(ret >= 0, "Failed sending: err %d", ret);

	LOG_DBG("sent: len %d", len);

	return ret;
}

struct net_buf *alloc_buf_cb(struct bt_l2cap_chan *chan)
{
	return net_buf_alloc(&sdu_rx_pool, K_NO_WAIT);
}

void continue_sending(struct test_ctx *ctx)
{
	struct bt_l2cap_chan *chan = &ctx->le_chan.chan;

	LOG_DBG("%p, remaining %d", chan, ctx->tx_remaining);

	if (ctx->tx_remaining) {
		l2cap_chan_send(chan, tx_data, sizeof(tx_data));
	} else {
		LOG_DBG("Done sending %u", bt_conn_index(chan->conn));
	}
}

void sent_cb(struct bt_l2cap_chan *chan)
{
	LOG_DBG("%p", chan);

	if (test_ctx.tx_remaining) {
		test_ctx.tx_remaining--;
	}

	k_sem_give(&tx_sem);
	continue_sending(&test_ctx);
}

int recv_cb(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	LOG_DBG("len %d", buf->len);
	rx_cnt++;

	/* Verify SDU data matches TX'd data. */
	ASSERT(memcmp(buf->data, tx_data, buf->len) == 0, "RX data doesn't match TX");

	return 0;
}

void l2cap_chan_connected_cb(struct bt_l2cap_chan *l2cap_chan)
{
	struct bt_l2cap_le_chan *chan =
		CONTAINER_OF(l2cap_chan, struct bt_l2cap_le_chan, chan);

	/* TODO: check that actual MPS < expected MPS */

	SET_FLAG(flag_l2cap_connected);
	LOG_DBG("%x (tx mtu %d mps %d) (tx mtu %d mps %d)",
		l2cap_chan,
		chan->tx.mtu,
		chan->tx.mps,
		chan->rx.mtu,
		chan->rx.mps);
}

void l2cap_chan_disconnected_cb(struct bt_l2cap_chan *chan)
{
	UNSET_FLAG(flag_l2cap_connected);
	LOG_DBG("%p", chan);
}

static struct bt_l2cap_chan_ops ops = {
	.connected = l2cap_chan_connected_cb,
	.disconnected = l2cap_chan_disconnected_cb,
	.alloc_buf = alloc_buf_cb,
	.recv = recv_cb,
	.sent = sent_cb,
};

int server_accept_cb(struct bt_conn *conn, struct bt_l2cap_server *server,
		     struct bt_l2cap_chan **chan)
{
	struct bt_l2cap_le_chan *le_chan = &test_ctx.le_chan;

	memset(le_chan, 0, sizeof(*le_chan));
	le_chan->chan.ops = &ops;
	le_chan->rx.mtu = L2CAP_MTU;
	*chan = &le_chan->chan;

	return 0;
}

static struct bt_l2cap_server test_l2cap_server = {
	.accept = server_accept_cb
};

static int l2cap_server_register(bt_security_t sec_level)
{
	test_l2cap_server.psm = 0;
	test_l2cap_server.sec_level = sec_level;

	int err = bt_l2cap_server_register(&test_l2cap_server);

	ASSERT(err == 0, "Failed to register l2cap server.");

	return test_l2cap_server.psm;
}

static void connected(struct bt_conn *conn, uint8_t conn_err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (conn_err) {
		FAIL("Failed to connect to %s (%u)", addr, conn_err);
		return;
	}

	LOG_DBG("%s", addr);

	SET_FLAG(is_connected);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_DBG("%p %s (reason 0x%02x)", conn, addr, reason);

	UNSET_FLAG(is_connected);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

static void disconnect_device(struct bt_conn *conn, void *data)
{
	int err;

	SET_FLAG(is_connected);

	err = bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	ASSERT(!err, "Failed to initate disconnect (err %d)", err);

	LOG_DBG("Waiting for disconnection...");
	WAIT_FOR_FLAG_UNSET(is_connected);
}

#define BT_LE_ADV_CONN_NAME_OT BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONNECTABLE | \
					       BT_LE_ADV_OPT_USE_NAME |	\
					       BT_LE_ADV_OPT_ONE_TIME,	\
					       BT_GAP_ADV_FAST_INT_MIN_2, \
					       BT_GAP_ADV_FAST_INT_MAX_2, NULL)

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
};

static void test_peripheral_main(void)
{
	LOG_DBG("*L2CAP FRAGS Peripheral started*");
	int err;

	/* Prepare tx_data */
	for (size_t i = 0; i < sizeof(tx_data); i++) {
		tx_data[i] = (uint8_t)i;
	}

	err = bt_enable(NULL);
	if (err) {
		FAIL("Can't enable Bluetooth (err %d)", err);
		return;
	}

	LOG_DBG("Peripheral Bluetooth initialized.");
	LOG_DBG("Connectable advertising...");
	err = bt_le_adv_start(BT_LE_ADV_CONN_NAME_OT, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err) {
		FAIL("Advertising failed to start (err %d)", err);
		return;
	}

	LOG_DBG("Advertising started.");
	LOG_DBG("Peripheral waiting for connection...");
	WAIT_FOR_FLAG_SET(is_connected);
	LOG_DBG("Peripheral Connected.");

	int psm = l2cap_server_register(BT_SECURITY_L1);

	LOG_DBG("Registered server PSM %x", psm);

	LOG_DBG("Peripheral waiting for transfer completion");
	while (rx_cnt < PAYLOAD_NUM) {
		k_sleep(K_MSEC(100));
	}

	bt_conn_foreach(BT_CONN_TYPE_LE, disconnect_device, NULL);
	LOG_INF("Total received: %d", rx_cnt);

	ASSERT(rx_cnt == PAYLOAD_NUM, "Did not receive expected no of SDUs\n");

	PASS("L2CAP FRAGS Peripheral passed\n");
}

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			 struct net_buf_simple *ad)
{
	struct bt_le_conn_param *param;
	struct bt_conn *conn;
	int err;

	err = bt_le_scan_stop();
	if (err) {
		FAIL("Stop LE scan failed (err %d)", err);
		return;
	}

	char str[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(addr, str, sizeof(str));

	LOG_DBG("Connecting to %s", str);

	param = BT_LE_CONN_PARAM_DEFAULT;
	err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN, param, &conn);
	if (err) {
		FAIL("Create conn failed (err %d)", err);
		return;
	}
}

static void connect_peripheral(void)
{
	struct bt_le_scan_param scan_param = {
		.type = BT_LE_SCAN_TYPE_ACTIVE,
		.options = BT_LE_SCAN_OPT_NONE,
		.interval = BT_GAP_SCAN_FAST_INTERVAL,
		.window = BT_GAP_SCAN_FAST_WINDOW,
	};

	UNSET_FLAG(is_connected);

	int err = bt_le_scan_start(&scan_param, device_found);

	ASSERT(!err, "Scanning failed to start (err %d)\n", err);

	LOG_DBG("Central initiating connection...");
	WAIT_FOR_FLAG_SET(is_connected);
}

static void connect_l2cap_channel(struct bt_conn *conn, void *data)
{
	int err;
	struct bt_l2cap_le_chan *le_chan = &test_ctx.le_chan;

	le_chan->chan.ops = &ops;
	le_chan->rx.mtu = L2CAP_MTU;

	UNSET_FLAG(flag_l2cap_connected);

	err = bt_l2cap_chan_connect(conn, &le_chan->chan, 0x0080);
	ASSERT(!err, "Error connecting l2cap channel (err %d)\n", err);

	WAIT_FOR_FLAG_SET(flag_l2cap_connected);
}

static void connect_l2cap_ecred_channel(struct bt_conn *conn, void *data)
{
	int err;
	struct bt_l2cap_le_chan *le_chan = &test_ctx.le_chan;
	struct bt_l2cap_chan *chan_list[2] = { &le_chan->chan, 0 };

	le_chan->chan.ops = &ops;
	le_chan->rx.mtu = L2CAP_MTU;

	UNSET_FLAG(flag_l2cap_connected);

	err = bt_l2cap_ecred_chan_connect(conn, chan_list, 0x0080);
	ASSERT(!err, "Error connecting l2cap channel (err %d)\n", err);

	WAIT_FOR_FLAG_SET(flag_l2cap_connected);
}

static void test_central_main(void)
{
	LOG_DBG("*L2CAP FRAGS Central started*");
	int err;

	/* Prepare tx_data */
	for (size_t i = 0; i < sizeof(tx_data); i++) {
		tx_data[i] = (uint8_t)i;
	}

	err = bt_enable(NULL);
	ASSERT(err == 0, "Can't enable Bluetooth (err %d)\n", err);
	LOG_DBG("Central Bluetooth initialized.");

	connect_peripheral();

	/* Connect L2CAP channels */
	LOG_DBG("Connect L2CAP channels");
	if (IS_ENABLED(CONFIG_BT_L2CAP_ECRED)) {
		bt_conn_foreach(BT_CONN_TYPE_LE, connect_l2cap_ecred_channel, NULL);
	} else {
		bt_conn_foreach(BT_CONN_TYPE_LE, connect_l2cap_channel, NULL);
	}

	/* Send PAYLOAD_NUM SDUs to each peripheral */
	LOG_DBG("Start sending SDUs");
	test_ctx.tx_remaining = PAYLOAD_NUM;
	l2cap_chan_send(&test_ctx.le_chan.chan, tx_data, sizeof(tx_data));

	LOG_DBG("Wait until all transfers are completed.");
	while (test_ctx.tx_remaining) {
		k_msleep(100);
	}

	WAIT_FOR_FLAG_UNSET(is_connected);
	LOG_DBG("Peripheral disconnected.");
	PASS("L2CAP FRAGS Central passed\n");
}

static const struct bst_test_instance test_def[] = {
	{
		.test_id = "peripheral",
		.test_descr = "Peripheral L2CAP FRAGS",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_peripheral_main
	},
	{
		.test_id = "central",
		.test_descr = "Central L2CAP FRAGS",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_central_main
	},
	BSTEST_END_MARKER
};

struct bst_test_list *test_main_l2cap_credits_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_def);
}

extern struct bst_test_list *test_main_l2cap_credits_install(struct bst_test_list *tests);

bst_test_install_t test_installers[] = {
	test_main_l2cap_credits_install,
	NULL
};

int main(void)
{
	bst_main();

	return 0;
}
