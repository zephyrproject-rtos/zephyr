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

#include "babblekit/flags.h"
#include "babblekit/testcase.h"

/* local includes */
#include "data.h"

LOG_MODULE_REGISTER(dut, CONFIG_APP_LOG_LEVEL);

static DEFINE_FLAG(ADVERTISING);

static void sdu_destroy(struct net_buf *buf)
{
	LOG_DBG("%p", buf);

	net_buf_destroy(buf);
}

/* Only one SDU per link will be transmitted at a time */
NET_BUF_POOL_DEFINE(sdu_tx_pool, CONFIG_BT_MAX_CONN, BT_L2CAP_SDU_BUF_SIZE(SDU_LEN),
		    CONFIG_BT_CONN_TX_USER_DATA_SIZE, sdu_destroy);

static uint8_t tx_data[SDU_LEN];

struct test_ctx {
	bt_addr_le_t peer;
	struct bt_l2cap_le_chan le_chan;
	size_t sdu_count; /* the number of SDUs that have been transferred until now */
};

static struct test_ctx contexts[CONFIG_BT_MAX_CONN];

static int send_data_over_l2cap(struct bt_l2cap_chan *chan, uint8_t *data, size_t len)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(chan->conn), addr, sizeof(addr));

	LOG_DBG("[%s] chan %p data %p len %d", addr, chan, data, len);

	struct net_buf *buf = net_buf_alloc(&sdu_tx_pool, K_NO_WAIT);

	if (buf == NULL) {
		TEST_FAIL("No more memory");
		return -ENOMEM;
	}

	net_buf_reserve(buf, BT_L2CAP_SDU_CHAN_SEND_RESERVE);
	net_buf_add_mem(buf, data, len);

	int ret = bt_l2cap_chan_send(chan, buf);

	TEST_ASSERT(ret == 0, "Failed sending: err %d", ret);
	LOG_DBG("queued SDU", len);

	return ret;
}

static void resume_sending_until_done(struct test_ctx *ctx)
{
	struct bt_l2cap_chan *chan = &ctx->le_chan.chan;

	TEST_ASSERT(ctx->le_chan.state == BT_L2CAP_CONNECTED,
		    "attempting to send on disconnected channel (%p)", chan);

	LOG_DBG("%p, transmitted %d SDUs", chan, ctx->sdu_count);

	if (ctx->sdu_count < SDU_NUM) {
		send_data_over_l2cap(chan, tx_data, sizeof(tx_data));
	} else {
		char addr[BT_ADDR_LE_STR_LEN];

		bt_addr_le_to_str(bt_conn_get_dst(chan->conn), addr, sizeof(addr));

		LOG_DBG("[%s] Done sending", addr);
	}
}

static struct test_ctx *get_ctx_from_chan(struct bt_l2cap_chan *chan)
{
	struct bt_l2cap_le_chan *le_chan = CONTAINER_OF(chan, struct bt_l2cap_le_chan, chan);
	struct test_ctx *ctx = CONTAINER_OF(le_chan, struct test_ctx, le_chan);

	TEST_ASSERT(PART_OF_ARRAY(contexts, ctx), "memory corruption");

	return ctx;
}

static void sent_cb(struct bt_l2cap_chan *chan)
{
	struct test_ctx *ctx = get_ctx_from_chan(chan);

	LOG_DBG("%p", chan);

	ctx->sdu_count++;

	resume_sending_until_done(ctx);
}

static void l2cap_chan_connected_cb(struct bt_l2cap_chan *chan)
{
	struct bt_l2cap_le_chan *le_chan = CONTAINER_OF(chan, struct bt_l2cap_le_chan, chan);

	LOG_DBG("%p (tx mtu %d mps %d) (tx mtu %d mps %d)", chan, le_chan->tx.mtu, le_chan->tx.mps,
		le_chan->rx.mtu, le_chan->rx.mps);

	LOG_DBG("initiating SDU transfer");
	resume_sending_until_done(get_ctx_from_chan(chan));
}

static void l2cap_chan_disconnected_cb(struct bt_l2cap_chan *chan)
{
	LOG_DBG("%p", chan);
}

static int recv_cb(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	TEST_FAIL("DUT should not receive data");

	return 0;
}

static int connect_l2cap_channel(struct bt_conn *conn, struct bt_l2cap_le_chan *le_chan)
{
	static struct bt_l2cap_chan_ops ops = {
		.connected = l2cap_chan_connected_cb,
		.disconnected = l2cap_chan_disconnected_cb,
		.recv = recv_cb,
		.sent = sent_cb,
	};

	memset(le_chan, 0, sizeof(*le_chan));
	le_chan->chan.ops = &ops;

	return bt_l2cap_chan_connect(conn, &le_chan->chan, 0x0080);
}

static bool addr_in_use(bt_addr_le_t *address)
{
	return !bt_addr_le_eq(address, BT_ADDR_LE_ANY);
}

static struct test_ctx *alloc_ctx(void)
{
	for (size_t i = 0; i < ARRAY_SIZE(contexts); i++) {
		struct test_ctx *context = &contexts[i];
		struct bt_l2cap_le_chan *le_chan = &context->le_chan;

		if (le_chan->state != BT_L2CAP_DISCONNECTED) {
			continue;
		}

		if (addr_in_use(&context->peer)) {
			continue;
		}

		memset(context, 0, sizeof(struct test_ctx));

		return context;
	}

	return NULL;
}

static struct test_ctx *get_ctx_from_address(const bt_addr_le_t *address)
{
	for (size_t i = 0; i < ARRAY_SIZE(contexts); i++) {
		struct test_ctx *context = &contexts[i];

		if (bt_addr_le_eq(address, &context->peer)) {
			return context;
		}
	}

	return NULL;
}

static void acl_connected(struct bt_conn *conn, uint8_t err)
{
	const bt_addr_le_t *central = bt_conn_get_dst(conn);
	char addr[BT_ADDR_LE_STR_LEN];
	struct test_ctx *ctx;
	int ret;

	bt_addr_le_to_str(central, addr, sizeof(addr));

	TEST_ASSERT(err == 0, "Failed to connect to %s (0x%02x)", addr, err);

	UNSET_FLAG(ADVERTISING);

	LOG_DBG("[%s] Connected (conn %p)", addr, conn);

	ctx = get_ctx_from_address(central);
	if (ctx == NULL) {
		LOG_DBG("no initialized context for %s, allocating..", addr);
		ctx = alloc_ctx();
		TEST_ASSERT(ctx, "Couldn't allocate ctx for conn %p", conn);

		LOG_DBG("allocated context %p for %s", ctx, central);
		bt_addr_le_copy(&ctx->peer, central);
	}

	ret = connect_l2cap_channel(conn, &ctx->le_chan);
	TEST_ASSERT(!ret, "Error connecting l2cap channel (err %d)", ret);
}

static void acl_disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_DBG("Disconnected from %s (reason 0x%02x)", addr, reason);
}

static void increment(struct bt_conn *conn, void *user_data)
{
	size_t *conn_count = user_data;

	(*conn_count)++;
}

static bool have_free_conn(void)
{
	size_t conn_count = 0;

	bt_conn_foreach(BT_CONN_TYPE_LE, increment, &conn_count);

	return conn_count < CONFIG_BT_MAX_CONN;
}

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA(BT_DATA_NAME_COMPLETE, DUT_NAME, sizeof(DUT_NAME) - 1),
};

static void start_advertising(void)
{
	int err;

	LOG_DBG("starting advertiser");

	err = bt_le_adv_start(BT_LE_ADV_CONN_ONE_TIME, ad, ARRAY_SIZE(ad), NULL, 0);
	TEST_ASSERT(!err, "Advertising failed to start (err %d)", err);
}

static bool all_data_transferred(void)
{
	size_t total_sdu_count = 0;

	for (size_t i = 0; i < ARRAY_SIZE(contexts); i++) {
		total_sdu_count += contexts[i].sdu_count;
	}

	TEST_ASSERT(total_sdu_count <= (SDU_NUM * CONFIG_BT_MAX_CONN),
		    "Received more SDUs than expected");

	return total_sdu_count == (SDU_NUM * CONFIG_BT_MAX_CONN);
}

void entrypoint_dut(void)
{
	/* Test purpose:
	 *
	 * For a peripheral device (DUT) that has multiple ACL connections to
	 * central devices: Verify that the data streams on one connection are
	 * not affected by one of the centrals going out of range or not
	 * responding.
	 *
	 * Three devices:
	 * - `dut`: sends L2CAP packets to p0 and p1
	 *
	 * DUT (in a loop):
	 * - advertise as connectable
	 * - [acl connected]
	 * - establish L2CAP channel
	 * - [l2 connected]
	 * - send L2CAP data until ACL disconnected or SDU_NUM SDUs reached
	 *
	 * p0/1/2 (in a loop):
	 * - scan & connect ACL
	 * - [acl connected]
	 * - [l2cap dynamic channel connected]
	 * - receive data from DUT
	 * - disconnect
	 *
	 * Verdict:
	 * - DUT is able to transfer SDU_NUM SDUs to all peers. Data can be
	 * dropped but resources should not leak, and the transfer should not
	 * stall.
	 */
	int err;
	static struct bt_conn_cb peripheral_cb = {
		.connected = acl_connected,
		.disconnected = acl_disconnected,
	};

	/* Mark test as in progress. */
	TEST_START("dut");

	/* Initialize Bluetooth */
	err = bt_conn_cb_register(&peripheral_cb);
	TEST_ASSERT(err == 0, "Can't register callbacks (err %d)", err);

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Can't enable Bluetooth (err %d)", err);

	LOG_DBG("Bluetooth initialized");

	while (!all_data_transferred()) {
		if (!have_free_conn() || IS_FLAG_SET(ADVERTISING)) {
			/* Sleep to not hammer the CPU checking the `if` */
			k_sleep(K_MSEC(10));
			continue;
		}

		start_advertising();
		SET_FLAG(ADVERTISING);

		/* L2 channel is opened from conn->connected() */
		/* L2 data transfer is initiated from l2->connected() */
		/* L2 data transfer is initiated for next SDU from l2->sent() */

	}

	TEST_PASS_AND_EXIT("dut");
}
