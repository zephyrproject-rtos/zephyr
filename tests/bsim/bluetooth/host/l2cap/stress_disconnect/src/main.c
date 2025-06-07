/* main_l2cap_stress.c - Application main entry point */

/*
 * Copyright (c) 2022 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>

#include <zephyr/types.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/l2cap.h>
#include "babblekit/testcase.h"
#include "babblekit/flags.h"
#include "bsim_args_runner.h"

#define LOG_MODULE_NAME main
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME, LOG_LEVEL_INF);

DEFINE_FLAG_STATIC(is_connected);
DEFINE_FLAG_STATIC(flag_l2cap_connected);

#define NUM_PERIPHERALS 6
#define L2CAP_CHANS     NUM_PERIPHERALS
#define SDU_NUM         20
#define SDU_LEN         3000
#define RESCHEDULE_DELAY K_MSEC(100)

/* This is a modification of the original stress test. The original test
 * connected one central to six peripherals, and sent each peripheral
 * 20 SDUs, each with length 3000. The peripheral disconnects when
 * it has received 20 SDUs.
 *
 * The purpose of this test is to validate proper disconnect
 * handling, in particular for buffers that were still in flight
 * when the disconnect occured.
 *
 * The change is to the each of the six peripherals disconnect
 * before receiving all SDUs. We could make the disconnects random
 * but prefer determistic results so the disconnects will be
 * as follows:
 *
 * Peripheral 1: disconnects after all 20 SDUs as before
 * Peripheral 2: disconnects immediately before receiving anything
 * Peripheral 3: disconnects after receiving first SDU
 * Peripheral 4: disconnects after receiving first PDU in second SDU
 * Peripheral 5: disconnects after receiving third PDU in third SDU
 * Peripheral 6: disconnects atfer receiving tenth PDU in tenth SDU
 */
static unsigned int device_nbr;

static void sdu_destroy(struct net_buf *buf)
{
	LOG_DBG("device_nbr %u: buf %p, buf->__buf %p", device_nbr, buf, buf->__buf);

	net_buf_destroy(buf);
}

static void rx_destroy(struct net_buf *buf)
{
	LOG_DBG("device_nbr %u: %p", device_nbr, buf);

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
static uint16_t sdu_rx_cnt;
static uint16_t pdu_rx_cnt;
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

	TEST_ASSERT(ctx >= &contexts[0] && ctx <= &contexts[L2CAP_CHANS], "memory corruption");

	return ctx;
}

int l2cap_chan_send(struct bt_l2cap_chan *chan, uint8_t *data, size_t len)
{
	LOG_DBG("device_nbr %u: chan %p conn %u data %p len %d", device_nbr, chan, bt_conn_index(chan->conn), data, len);

	struct net_buf *buf = net_buf_alloc(&sdu_tx_pool, K_NO_WAIT);

	if (buf == NULL) {
		TEST_FAIL("No more memory");
		return -ENOMEM;
	}

	net_buf_reserve(buf, BT_L2CAP_SDU_CHAN_SEND_RESERVE);
	net_buf_add_mem(buf, data, len);

	int ret = bt_l2cap_chan_send(chan, buf);

	if (ret == -EAGAIN) {
		LOG_DBG("device_nbr %u: L2CAP error %d, attempting to reschedule sending", device_nbr, ret);
		net_buf_unref(buf);
		k_work_reschedule(&(get_ctx(chan)->work_item), RESCHEDULE_DELAY);

		return ret;
	}

	TEST_ASSERT(ret >= 0, "Failed sending: err %d", ret);

	LOG_DBG("device_nbr %u: sent %d len %d", device_nbr, ret, len);
	return ret;
}

struct net_buf *alloc_buf_cb(struct bt_l2cap_chan *chan)
{
	return net_buf_alloc(&sdu_rx_pool, K_NO_WAIT);
}

void continue_sending(struct test_ctx *ctx)
{
	struct bt_l2cap_chan *chan = &ctx->le_chan.chan;

	LOG_DBG("device_nbr %u: %p, left %d", device_nbr, chan, ctx->tx_left);

	if (ctx->tx_left) {
		l2cap_chan_send(chan, tx_data, sizeof(tx_data));
	} else {
		LOG_DBG("device_nbr %u: Done sending %u", device_nbr, bt_conn_index(chan->conn));
	}
}

void sent_cb(struct bt_l2cap_chan *chan)
{
	struct test_ctx *ctx = get_ctx(chan);

	LOG_DBG("device_nbr %u: %p", device_nbr, chan);

	if (ctx->tx_left) {
		ctx->tx_left--;
	}

	continue_sending(ctx);
}

static void disconnect_device_no_wait(struct bt_conn *conn, void *data)
{
	int err;

	SET_FLAG(is_connected);

	err = bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	TEST_ASSERT(!err, "Failed to initate disconnect (err %d)", err);
}

static void seg_recv_cb(struct bt_l2cap_chan *chan, size_t sdu_len, off_t seg_offset, struct net_buf_simple *seg)
{
	if ((seg_offset + seg->len) == sdu_len) {
		/* last segment/PDU of a SDU */
		LOG_DBG("device_nbr %u: len %d", device_nbr, seg->len);
		sdu_rx_cnt++;
		pdu_rx_cnt = 0;
	} else {
		LOG_DBG("device_nbr %u: SDU %u, pdu %u at seg_offset %u, len %u",
			device_nbr, sdu_rx_cnt, pdu_rx_cnt, seg_offset, seg->len);
		pdu_rx_cnt++;
	}

	/* Verify SDU data matches TX'd data. */
	int pos = memcmp(seg->data, &tx_data[seg_offset], seg->len);

	if (pos != 0) {
		LOG_ERR("RX data doesn't match TX: pos %d", pos + seg_offset);
		LOG_HEXDUMP_ERR(seg->data, seg->len, "RX data");
		LOG_HEXDUMP_INF(tx_data, seg->len, "TX data");

		for (uint16_t p = 0; p < seg->len; p++) {
			__ASSERT(seg->data[p] == tx_data[p + seg_offset],
				 "Failed rx[%d]=%x != expect[%d]=%x",
				 p, seg->data[p], p, tx_data[p + seg_offset]);
		}
	}

	if (((device_nbr == 4) && (sdu_rx_cnt >= 1) && (pdu_rx_cnt == 1)) ||
	    ((device_nbr == 5) && (sdu_rx_cnt >= 2) && (pdu_rx_cnt == 3)) ||
	    ((device_nbr == 6) && (sdu_rx_cnt >= 9) && (pdu_rx_cnt == 10))) {
		LOG_INF("device_nbr %u: disconnecting after receiving PDU %u of SDU %u",
			device_nbr, pdu_rx_cnt - 1, sdu_rx_cnt);
		bt_conn_foreach(BT_CONN_TYPE_LE, disconnect_device_no_wait, NULL);
		return;
	}

	bt_l2cap_chan_give_credits(chan, 1);
}

void l2cap_chan_connected_cb(struct bt_l2cap_chan *l2cap_chan)
{
	struct bt_l2cap_le_chan *chan =
		CONTAINER_OF(l2cap_chan, struct bt_l2cap_le_chan, chan);

	SET_FLAG(flag_l2cap_connected);
	LOG_DBG("device_nbr %u: %x (tx mtu %d mps %d credits %ld) (tx mtu %d mps %d credits %ld)",
		device_nbr,
 		l2cap_chan,
		chan->tx.mtu,
		chan->tx.mps,
		atomic_get(&chan->tx.credits),
		chan->rx.mtu,
		chan->rx.mps,
		atomic_get(&chan->rx.credits));
}

void l2cap_chan_disconnected_cb(struct bt_l2cap_chan *l2cap_chan)
{
	UNSET_FLAG(flag_l2cap_connected);
	LOG_DBG("device_nbr %u: %p", device_nbr, l2cap_chan);
	for (int i = 0; i < L2CAP_CHANS; i++) {
		if (&contexts[i].le_chan == CONTAINER_OF(l2cap_chan, struct bt_l2cap_le_chan, chan)) {
			if (contexts[i].tx_left > 0) {
				LOG_INF("device_nbr %u: setting tx_left to 0 because of disconnect", device_nbr);
				contexts[i].tx_left = 0;
			}
			break;
		}
	}
}

static struct bt_l2cap_chan_ops ops = {
	.connected = l2cap_chan_connected_cb,
	.disconnected = l2cap_chan_disconnected_cb,
	.alloc_buf = alloc_buf_cb,
	.seg_recv = seg_recv_cb,
	.sent = sent_cb,
};

void deferred_send(struct k_work *item)
{
	struct test_ctx *ctx = CONTAINER_OF(k_work_delayable_from_work(item),
					    struct test_ctx, work_item);

	struct bt_l2cap_chan *chan = &ctx->le_chan.chan;

	LOG_DBG("device_nbr %u: continue %u left %d", device_nbr, bt_conn_index(chan->conn), ctx->tx_left);

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
	le_chan->rx.mps = BT_L2CAP_RX_MTU;
	le_chan->rx.credits = CONFIG_BT_BUF_ACL_RX_COUNT_EXTRA;
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

	TEST_ASSERT(err == 0, "Failed to register l2cap server.");

	return test_l2cap_server.psm;
}

static void connected(struct bt_conn *conn, uint8_t conn_err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (conn_err) {
		TEST_FAIL("Failed to connect to %s (%u)", addr, conn_err);
		return;
	}

	LOG_DBG("device_nbr %u: %s", device_nbr, addr);

	SET_FLAG(is_connected);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_DBG("device_nbr %u: %p %s (reason 0x%02x)", device_nbr, conn, addr, reason);

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
	TEST_ASSERT(!err, "Failed to initate disconnect (err %d)", err);

	LOG_DBG("device_nbr %u: Waiting for disconnection...", device_nbr);
	WAIT_FOR_FLAG_UNSET(is_connected);
}

static void test_peripheral_main(void)
{
	device_nbr = bsim_args_get_global_device_nbr();
	LOG_DBG("*device_nbr %u: L2CAP STRESS Peripheral started*", device_nbr);
	int err;

	/* Prepare tx_data */
	for (size_t i = 0; i < sizeof(tx_data); i++) {
		tx_data[i] = (uint8_t)i;
	}

	err = bt_enable(NULL);
	if (err) {
		TEST_FAIL("Can't enable Bluetooth (err %d)", err);
		return;
	}

	LOG_DBG("device_nbr %u: Peripheral Bluetooth initialized.", device_nbr);
	LOG_DBG("deviec_nbr %u: Connectable advertising...", device_nbr);
	err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, NULL, 0, NULL, 0);
	if (err) {
		TEST_FAIL("Advertising failed to start (err %d)", err);
		return;
	}

	LOG_DBG("device_nbr %u: Advertising started.", device_nbr);
	LOG_DBG("device_nbr %u: Peripheral waiting for connection...", device_nbr);
	WAIT_FOR_FLAG(is_connected);
	LOG_DBG("device_nbr %u: Peripheral Connected.", device_nbr);

	int psm = l2cap_server_register(BT_SECURITY_L1);

	LOG_DBG("device_nbr %u: Registered server PSM %x", device_nbr, psm);

	if (device_nbr == 2) {
		LOG_INF("device_nbr %u: disconnecting before receiving any SDU", device_nbr);
		k_msleep(1000);
		goto disconnect;
	}

	LOG_DBG("device_nbr %u: Peripheral waiting for transfer completion", device_nbr);
	while (sdu_rx_cnt < SDU_NUM) {
		if ((device_nbr == 3) && (sdu_rx_cnt >= 1)) {
			LOG_INF("device_nbr %u: disconnecting after receiving SDU %u", device_nbr, sdu_rx_cnt);
			break;
		}

		k_msleep(100);
		if (!is_connected) {
			goto done;
		}
	}

disconnect:
	bt_conn_foreach(BT_CONN_TYPE_LE, disconnect_device, NULL);
done:

	WAIT_FOR_FLAG_UNSET(is_connected);
	LOG_INF("device_nbr %u: Total received: %d", device_nbr, sdu_rx_cnt);

	TEST_PASS("device_nbr %u: L2CAP STRESS Peripheral passed", device_nbr);
}

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			 struct net_buf_simple *ad)
{
	struct bt_le_conn_param *param;
	struct bt_conn *conn;
	int err;

	err = bt_le_scan_stop();
	if (err) {
		TEST_FAIL("Stop LE scan failed (err %d)", err);
		return;
	}

	char str[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(addr, str, sizeof(str));

	LOG_DBG("device_nbr %u: Connecting to %s", device_nbr, str);

	param = BT_LE_CONN_PARAM_DEFAULT;
	err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN, param, &conn);
	if (err) {
		TEST_FAIL("Create conn failed (err %d)", err);
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

	TEST_ASSERT(!err, "Scanning failed to start (err %d)", err);

	LOG_DBG("device_nbr %u: Central initiating connection...", device_nbr);
	WAIT_FOR_FLAG(is_connected);
}

static void connect_l2cap_channel(struct bt_conn *conn, void *data)
{
	int err;
	struct test_ctx *ctx = alloc_test_context();

	TEST_ASSERT(ctx, "No more available test contexts");

	struct bt_l2cap_le_chan *le_chan = &ctx->le_chan;

	le_chan->chan.ops = &ops;
	le_chan->rx.mtu = SDU_LEN;
	le_chan->rx.mps = BT_L2CAP_RX_MTU;
	le_chan->rx.credits = CONFIG_BT_BUF_ACL_RX_COUNT_EXTRA;

	UNSET_FLAG(flag_l2cap_connected);

	err = bt_l2cap_chan_connect(conn, &le_chan->chan, 0x0080);
	TEST_ASSERT(!err, "Error connecting l2cap channel (err %d)", err);

	WAIT_FOR_FLAG(flag_l2cap_connected);
}

static void test_central_main(void)
{
	LOG_DBG("*device_nbr %u: L2CAP STRESS Central started*", device_nbr);
	int err;

	/* Prepare tx_data */
	for (size_t i = 0; i < sizeof(tx_data); i++) {
		tx_data[i] = (uint8_t)i;
	}

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Can't enable Bluetooth (err %d)", err);
	LOG_DBG("device_nbr %u: Central Bluetooth initialized.", device_nbr);

	/* Connect all peripherals */
	for (int i = 0; i < NUM_PERIPHERALS; i++) {
		connect_peripheral();
	}

	/* Connect L2CAP channels */
	LOG_DBG("device_nbr %u: Connect L2CAP channels", device_nbr);
	bt_conn_foreach(BT_CONN_TYPE_LE, connect_l2cap_channel, NULL);

	/* Send SDU_NUM SDUs to each peripheral */
	for (int i = 0; i < NUM_PERIPHERALS; i++) {
		contexts[i].tx_left = SDU_NUM;
		l2cap_chan_send(&contexts[i].le_chan.chan, tx_data, sizeof(tx_data));
	}

	LOG_DBG("device_nbr %u: Central: wait until all transfers are completed.", device_nbr);
	int remaining_tx_total;

	do {
		k_msleep(100);

		remaining_tx_total = 0;
		for (int i = 0; i < L2CAP_CHANS; i++) {
			remaining_tx_total += contexts[i].tx_left;
		}
	} while (remaining_tx_total);

	LOG_INF("device_nbr: Waiting until all peripherals are disconnected..", device_nbr);
	while (disconnect_counter < NUM_PERIPHERALS) {
		k_msleep(100);
	}
	LOG_INF("device_nbr %u: All peripherals disconnected.", device_nbr);

	TEST_PASS("L2CAP STRESS Central passed");
}

static const struct bst_test_instance test_def[] = {
	{
		.test_id = "peripheral",
		.test_descr = "Peripheral L2CAP STRESS",
		.test_main_f = test_peripheral_main
	},
	{
		.test_id = "central",
		.test_descr = "Central L2CAP STRESS",
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
