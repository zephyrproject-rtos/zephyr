/*
 * Copyright (c) 2023 Nordic Semiconductor
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

#define L2CAP_MPS CONFIG_BT_L2CAP_TX_MTU
#define SDU_NUM   3
#define SDU_LEN   (2 * L2CAP_MPS)
/* We intentionally send smaller SDUs than the channel can fit */
#define L2CAP_MTU (2 * SDU_LEN)

/* Only one SDU transmitted or received at a time */
NET_BUF_POOL_DEFINE(sdu_pool, 1, BT_L2CAP_SDU_BUF_SIZE(L2CAP_MTU),
		    CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

static uint8_t tx_data[SDU_LEN];
static uint16_t rx_cnt;

K_SEM_DEFINE(sdu_received, 0, 1);

struct test_ctx {
	struct bt_l2cap_le_chan le_chan;
	size_t tx_left;
} test_ctx;

int l2cap_chan_send(struct bt_l2cap_chan *chan, uint8_t *data, size_t len)
{
	struct net_buf *buf;
	int ret;

	LOG_DBG("chan %p conn %u data %p len %d", chan, bt_conn_index(chan->conn), data, len);

	buf = net_buf_alloc(&sdu_pool, K_NO_WAIT);

	if (buf == NULL) {
		FAIL("No more memory\n");
		return -ENOMEM;
	}

	net_buf_reserve(buf, BT_L2CAP_SDU_CHAN_SEND_RESERVE);
	net_buf_add_mem(buf, data, len);

	ret = bt_l2cap_chan_send(chan, buf);

	ASSERT(ret >= 0, "Failed sending: err %d", ret);

	LOG_DBG("sent %d len %d", ret, len);
	return ret;
}

struct net_buf *alloc_buf_cb(struct bt_l2cap_chan *chan)
{
	return net_buf_alloc(&sdu_pool, K_NO_WAIT);
}

void continue_sending(struct test_ctx *ctx)
{
	struct bt_l2cap_chan *chan = &ctx->le_chan.chan;

	LOG_DBG("%p, left %d", chan, ctx->tx_left);

	if (ctx->tx_left) {
		l2cap_chan_send(chan, tx_data, sizeof(tx_data));
	} else {
		LOG_DBG("Done sending %u", bt_conn_index(chan->conn));
	}
}

void sent_cb(struct bt_l2cap_chan *chan)
{
	LOG_DBG("%p", chan);

	if (test_ctx.tx_left) {
		test_ctx.tx_left--;
	}

	continue_sending(&test_ctx);
}

void recv_cb(struct bt_l2cap_chan *l2cap_chan, size_t sdu_len, off_t seg_offset,
	     struct net_buf_simple *seg)
{
	LOG_DBG("sdu len %u frag offset %u frag len %u", sdu_len, seg_offset, seg->len);

	ASSERT(sdu_len == sizeof(tx_data), "Recv SDU length does not match send length.");

	/* Verify SDU data matches TX'd data. */
	ASSERT(memcmp(seg->data, &tx_data[seg_offset], seg->len) == 0, "RX data doesn't match TX");

	if (seg_offset + seg->len == sdu_len) {
		/* Don't give credits right away. The taker of this
		 * semaphore will give the credits after sleeping a bit.
		 */
		rx_cnt++;
		k_sem_give(&sdu_received);
	} else {
		/* To test that we really gave two initial credits, we
		 * "forget" to send one credit after the first PDU of
		 * the first SDU.
		 */
		if (!(rx_cnt == 0 && seg_offset == 0)) {
			/* Give more credits to complete SDU. */
			LOG_DBG("Giving credits for continuing SDU.");
			bt_l2cap_chan_give_credits(&test_ctx.le_chan.chan, 1);
		}
	}
}

void l2cap_chan_connected_cb(struct bt_l2cap_chan *l2cap_chan)
{
	struct bt_l2cap_le_chan *chan = CONTAINER_OF(l2cap_chan, struct bt_l2cap_le_chan, chan);

	/* TODO: check that actual MPS < expected MPS */

	SET_FLAG(flag_l2cap_connected);
	LOG_DBG("%x (tx mtu %d mps %d) (tx mtu %d mps %d)", l2cap_chan, chan->tx.mtu, chan->tx.mps,
		chan->rx.mtu, chan->rx.mps);
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
	.seg_recv = recv_cb,
	.sent = sent_cb,
};

int server_accept_cb(struct bt_conn *conn, struct bt_l2cap_server *server,
		     struct bt_l2cap_chan **chan)
{
	struct bt_l2cap_le_chan *le_chan = &test_ctx.le_chan;

	*chan = &le_chan->chan;

	/* Always default initialize the chan. */
	memset(le_chan, 0, sizeof(*le_chan));
	le_chan->chan.ops = &ops;

	le_chan->rx.mtu = L2CAP_MTU;
	le_chan->rx.mps = BT_L2CAP_RX_MTU;

	/* Credits can be given before returning from this
	 * accept-handler and after the 'connected' event. Credits given
	 * before completing the accept are sent in the 'initial
	 * credits' field of the connection response PDU.
	 */
	bt_l2cap_chan_give_credits(*chan, 2);

	return 0;
}

static struct bt_l2cap_server test_l2cap_server = {.accept = server_accept_cb};

static int l2cap_server_register(bt_security_t sec_level)
{
	int err;

	test_l2cap_server.psm = 0;
	test_l2cap_server.sec_level = sec_level;

	err = bt_l2cap_server_register(&test_l2cap_server);

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

#define BT_LE_ADV_CONN_NAME_OT                                                                     \
	BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONNECTABLE | BT_LE_ADV_OPT_USE_NAME |                       \
				BT_LE_ADV_OPT_ONE_TIME,                                            \
			BT_GAP_ADV_FAST_INT_MIN_2, BT_GAP_ADV_FAST_INT_MAX_2, NULL)

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
};

static void test_peripheral_main(void)
{
	int err;
	int psm;

	LOG_DBG("*L2CAP CREDITS Peripheral started*");

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

	psm = l2cap_server_register(BT_SECURITY_L1);

	LOG_DBG("Registered server PSM %x", psm);

	LOG_DBG("Peripheral waiting for transfer completion");
	while (rx_cnt < SDU_NUM) {
		k_sem_take(&sdu_received, K_FOREVER);

		/* Sleep enough so the peer has time to attempt sending another
		 * SDU. If it still has credits, it's in its right to do so. If
		 * it does so before we release the ref below, then allocation
		 * will fail and the channel will be disconnected.
		 */
		k_sleep(K_SECONDS(5));
		LOG_DBG("release SDU ref");
		LOG_DBG("Giving credits for new SDU.");
		bt_l2cap_chan_give_credits(&test_ctx.le_chan.chan, 1);
	}

	bt_conn_foreach(BT_CONN_TYPE_LE, disconnect_device, NULL);
	LOG_INF("Total received: %d", rx_cnt);

	ASSERT(rx_cnt == SDU_NUM, "Did not receive expected no of SDUs\n");

	PASS("L2CAP CREDITS Peripheral passed\n");
}

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			 struct net_buf_simple *ad)
{
	char str[BT_ADDR_LE_STR_LEN];
	int err;
	struct bt_conn *conn;
	struct bt_le_conn_param *param;

	err = bt_le_scan_stop();
	if (err) {
		FAIL("Stop LE scan failed (err %d)", err);
		return;
	}

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
	int err;

	UNSET_FLAG(is_connected);

	err = bt_le_scan_start(&scan_param, device_found);

	ASSERT(!err, "Scanning failed to start (err %d)\n", err);

	LOG_DBG("Central initiating connection...");
	WAIT_FOR_FLAG_SET(is_connected);
}

static void connect_l2cap_channel(struct bt_conn *conn, void *data)
{
	int err;
	struct bt_l2cap_le_chan *le_chan = &test_ctx.le_chan;

	*le_chan = (struct bt_l2cap_le_chan){
		.chan.ops = &ops,
		.rx.mtu = L2CAP_MTU,
		.rx.mps = BT_L2CAP_RX_MTU,
	};

	UNSET_FLAG(flag_l2cap_connected);

	/* Credits can be given before requesting the connection and
	 * after the 'connected' event. Credits given before connecting
	 * are sent in the 'initial credits' field of the connection
	 * request PDU.
	 */
	bt_l2cap_chan_give_credits(&le_chan->chan, 1);

	err = bt_l2cap_chan_connect(conn, &le_chan->chan, 0x0080);
	ASSERT(!err, "Error connecting l2cap channel (err %d)\n", err);

	WAIT_FOR_FLAG_SET(flag_l2cap_connected);
}

static void connect_l2cap_ecred_channel(struct bt_conn *conn, void *data)
{
	int err;
	struct bt_l2cap_le_chan *le_chan = &test_ctx.le_chan;
	struct bt_l2cap_chan *chan_list[2] = {&le_chan->chan, 0};

	*le_chan = (struct bt_l2cap_le_chan){
		.chan.ops = &ops,
		.rx.mtu = L2CAP_MTU,
		.rx.mps = BT_L2CAP_RX_MTU,
	};

	UNSET_FLAG(flag_l2cap_connected);

	/* Credits can be given before requesting the connection and
	 * after the 'connected' event. Credits given before connecting
	 * are sent in the 'initial credits' field of the connection
	 * request PDU.
	 */
	bt_l2cap_chan_give_credits(&le_chan->chan, 1);

	err = bt_l2cap_ecred_chan_connect(conn, chan_list, 0x0080);
	ASSERT(!err, "Error connecting l2cap channel (err %d)\n", err);

	WAIT_FOR_FLAG_SET(flag_l2cap_connected);
}

static void test_central_main(void)
{
	int err;

	LOG_DBG("*L2CAP CREDITS Central started*");

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

	/* Send SDU_NUM SDUs to each peripheral */
	test_ctx.tx_left = SDU_NUM;
	l2cap_chan_send(&test_ctx.le_chan.chan, tx_data, sizeof(tx_data));

	LOG_DBG("Wait until all transfers are completed.");
	while (test_ctx.tx_left) {
		k_msleep(100);
	}

	WAIT_FOR_FLAG_UNSET(is_connected);
	LOG_DBG("Peripheral disconnected.");
	PASS("L2CAP CREDITS Central passed\n");
}

static const struct bst_test_instance test_def[] = {
	{.test_id = "peripheral",
	 .test_descr = "Peripheral L2CAP CREDITS",
	 .test_post_init_f = test_init,
	 .test_tick_f = test_tick,
	 .test_main_f = test_peripheral_main},
	{.test_id = "central",
	 .test_descr = "Central L2CAP CREDITS",
	 .test_post_init_f = test_init,
	 .test_tick_f = test_tick,
	 .test_main_f = test_central_main},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_main_l2cap_credits_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_def);
}

extern struct bst_test_list *test_main_l2cap_credits_install(struct bst_test_list *tests);

bst_test_install_t test_installers[] = {test_main_l2cap_credits_install, NULL};

int main(void)
{
	bst_main();

	return 0;
}
