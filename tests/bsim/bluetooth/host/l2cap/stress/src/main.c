/* main_l2cap_stress.c - Application main entry point */

/*
 * Copyright (c) 2022 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bstests.h"
#include "common.h"

#define LOG_MODULE_NAME main
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME, LOG_LEVEL_INF);

CREATE_FLAG(is_connected);
CREATE_FLAG(flag_l2cap_connected);

#define NUM_PERIPHERALS 6
#define L2CAP_CHANS     NUM_PERIPHERALS
#define SDU_NUM         20
#define SDU_LEN         3000
#define RESCHEDULE_DELAY K_MSEC(100)

static void sdu_destroy(struct net_buf *buf)
{
	LOG_DBG("%p", buf);

	net_buf_destroy(buf);
}

static void rx_destroy(struct net_buf *buf)
{
	LOG_DBG("%p", buf);

	net_buf_destroy(buf);
}

/* Only one SDU per link will be transmitted at a time */
NET_BUF_POOL_DEFINE(sdu_tx_pool,
		    CONFIG_BT_MAX_CONN, BT_L2CAP_SDU_BUF_SIZE(SDU_LEN),
		    CONFIG_BT_CONN_TX_USER_DATA_SIZE, sdu_destroy);

/* Only one SDU per link will be received at a time */
NET_BUF_POOL_DEFINE(sdu_rx_pool,
		    CONFIG_BT_MAX_CONN, BT_L2CAP_SDU_BUF_SIZE(SDU_LEN),
		    8, rx_destroy);

static uint8_t tx_data[SDU_LEN];
static uint16_t rx_cnt;
static uint8_t disconnect_counter;

struct test_ctx {
	struct k_work_delayable work_item;
	struct bt_l2cap_le_chan le_chan;
	size_t tx_left;
};

static struct test_ctx contexts[L2CAP_CHANS];

struct test_ctx *get_ctx(struct bt_l2cap_chan *chan)
{
	struct bt_l2cap_le_chan *le_chan = CONTAINER_OF(chan, struct bt_l2cap_le_chan, chan);
	struct test_ctx *ctx = CONTAINER_OF(le_chan, struct test_ctx, le_chan);

	ASSERT(ctx >= &contexts[0] &&
	       ctx <= &contexts[L2CAP_CHANS], "memory corruption");

	return ctx;
}

int l2cap_chan_send(struct bt_l2cap_chan *chan, uint8_t *data, size_t len)
{
	LOG_DBG("chan %p conn %u data %p len %d", chan, bt_conn_index(chan->conn), data, len);

	struct net_buf *buf = net_buf_alloc(&sdu_tx_pool, K_NO_WAIT);

	if (buf == NULL) {
		FAIL("No more memory\n");
		return -ENOMEM;
	}

	net_buf_reserve(buf, BT_L2CAP_SDU_CHAN_SEND_RESERVE);
	net_buf_add_mem(buf, data, len);

	int ret = bt_l2cap_chan_send(chan, buf);

	if (ret == -EAGAIN) {
		LOG_DBG("L2CAP error %d, attempting to reschedule sending", ret);
		net_buf_unref(buf);
		k_work_reschedule(&(get_ctx(chan)->work_item), RESCHEDULE_DELAY);

		return ret;
	}

	ASSERT(ret >= 0, "Failed sending: err %d", ret);

	LOG_DBG("sent %d len %d", ret, len);
	return ret;
}

struct net_buf *alloc_buf_cb(struct bt_l2cap_chan *chan)
{
	return net_buf_alloc(&sdu_rx_pool, K_NO_WAIT);
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
	struct test_ctx *ctx = get_ctx(chan);

	LOG_DBG("%p", chan);

	if (ctx->tx_left) {
		ctx->tx_left--;
	}

	continue_sending(ctx);
}

int recv_cb(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	LOG_DBG("len %d", buf->len);
	rx_cnt++;

	/* Verify SDU data matches TX'd data. */
	int pos = memcmp(buf->data, tx_data, buf->len);

	if (pos != 0) {
		LOG_ERR("RX data doesn't match TX: pos %d", pos);
		LOG_HEXDUMP_ERR(buf->data, buf->len, "RX data");
		LOG_HEXDUMP_INF(tx_data, buf->len, "TX data");

		for (uint16_t p = 0; p < buf->len; p++) {
			__ASSERT(buf->data[p] == tx_data[p],
				 "Failed rx[%d]=%x != expect[%d]=%x",
				 p, buf->data[p], p, tx_data[p]);
		}
	}

	return 0;
}

void l2cap_chan_connected_cb(struct bt_l2cap_chan *l2cap_chan)
{
	struct bt_l2cap_le_chan *chan =
		CONTAINER_OF(l2cap_chan, struct bt_l2cap_le_chan, chan);

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

void deferred_send(struct k_work *item)
{
	struct test_ctx *ctx = CONTAINER_OF(k_work_delayable_from_work(item),
					    struct test_ctx, work_item);

	struct bt_l2cap_chan *chan = &ctx->le_chan.chan;

	LOG_DBG("continue %u left %d", bt_conn_index(chan->conn), ctx->tx_left);

	continue_sending(ctx);
}

struct test_ctx *alloc_test_context(void)
{
	for (int i = 0; i < L2CAP_CHANS; i++) {
		struct bt_l2cap_le_chan *le_chan = &contexts[i].le_chan;

		if (le_chan->state != BT_L2CAP_DISCONNECTED) {
			continue;
		}

		memset(&contexts[i], 0, sizeof(struct test_ctx));
		k_work_init_delayable(&contexts[i].work_item, deferred_send);

		return &contexts[i];
	}

	return NULL;
}

int server_accept_cb(struct bt_conn *conn, struct bt_l2cap_server *server,
		     struct bt_l2cap_chan **chan)
{
	struct test_ctx *ctx = NULL;

	ctx = alloc_test_context();
	if (ctx == NULL) {
		return -ENOMEM;
	}

	struct bt_l2cap_le_chan *le_chan = &ctx->le_chan;

	memset(le_chan, 0, sizeof(*le_chan));
	le_chan->chan.ops = &ops;
	le_chan->rx.mtu = SDU_LEN;
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
	disconnect_counter++;
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

#define BT_LE_ADV_CONN_OT BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONNECTABLE | \
					  BT_LE_ADV_OPT_ONE_TIME,	\
					  BT_GAP_ADV_FAST_INT_MIN_2, \
					  BT_GAP_ADV_FAST_INT_MAX_2, NULL)

static void test_peripheral_main(void)
{
	LOG_DBG("*L2CAP STRESS Peripheral started*");
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
	err = bt_le_adv_start(BT_LE_ADV_CONN_OT, NULL, 0, NULL, 0);
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
	while (rx_cnt < SDU_NUM) {
		k_msleep(100);
	}

	bt_conn_foreach(BT_CONN_TYPE_LE, disconnect_device, NULL);
	WAIT_FOR_FLAG_UNSET(is_connected);
	LOG_INF("Total received: %d", rx_cnt);

	ASSERT(rx_cnt == SDU_NUM, "Did not receive expected no of SDUs\n");

	PASS("L2CAP STRESS Peripheral passed\n");
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
	struct test_ctx *ctx = alloc_test_context();

	ASSERT(ctx, "No more available test contexts\n");

	struct bt_l2cap_le_chan *le_chan = &ctx->le_chan;

	le_chan->chan.ops = &ops;
	le_chan->rx.mtu = SDU_LEN;

	UNSET_FLAG(flag_l2cap_connected);

	err = bt_l2cap_chan_connect(conn, &le_chan->chan, 0x0080);
	ASSERT(!err, "Error connecting l2cap channel (err %d)\n", err);

	WAIT_FOR_FLAG_SET(flag_l2cap_connected);
}

static void test_central_main(void)
{
	LOG_DBG("*L2CAP STRESS Central started*");
	int err;

	/* Prepare tx_data */
	for (size_t i = 0; i < sizeof(tx_data); i++) {
		tx_data[i] = (uint8_t)i;
	}

	err = bt_enable(NULL);
	ASSERT(err == 0, "Can't enable Bluetooth (err %d)\n", err);
	LOG_DBG("Central Bluetooth initialized.");

	/* Connect all peripherals */
	for (int i = 0; i < NUM_PERIPHERALS; i++) {
		connect_peripheral();
	}

	/* Connect L2CAP channels */
	LOG_DBG("Connect L2CAP channels");
	bt_conn_foreach(BT_CONN_TYPE_LE, connect_l2cap_channel, NULL);

	/* Send SDU_NUM SDUs to each peripheral */
	for (int i = 0; i < NUM_PERIPHERALS; i++) {
		contexts[i].tx_left = SDU_NUM;
		l2cap_chan_send(&contexts[i].le_chan.chan, tx_data, sizeof(tx_data));
	}

	LOG_DBG("Wait until all transfers are completed.");
	int remaining_tx_total;

	do {
		k_msleep(100);

		remaining_tx_total = 0;
		for (int i = 0; i < L2CAP_CHANS; i++) {
			remaining_tx_total += contexts[i].tx_left;
		}
	} while (remaining_tx_total);

	LOG_DBG("Waiting until all peripherals are disconnected..");
	while (disconnect_counter < NUM_PERIPHERALS) {
		k_msleep(100);
	}
	LOG_DBG("All peripherals disconnected.");

	PASS("L2CAP STRESS Central passed\n");
}

static const struct bst_test_instance test_def[] = {
	{
		.test_id = "peripheral",
		.test_descr = "Peripheral L2CAP STRESS",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_peripheral_main
	},
	{
		.test_id = "central",
		.test_descr = "Central L2CAP STRESS",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_central_main
	},
	BSTEST_END_MARKER
};

struct bst_test_list *test_main_l2cap_stress_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_def);
}

extern struct bst_test_list *test_main_l2cap_stress_install(struct bst_test_list *tests);

bst_test_install_t test_installers[] = {
	test_main_l2cap_stress_install,
	NULL
};

int main(void)
{
	bst_main();
	return 0;
}
