/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <sys/printk.h>
#include <sys/byteorder.h>
#include <zephyr.h>
#include <init.h>

#include <net/buf.h>

#include "ots_l2cap_internal.h"

#include <logging/log.h>

LOG_MODULE_DECLARE(bt_ots, CONFIG_BT_OTS_LOG_LEVEL);

/* According to BLE specification Assigned Numbers that are used in the
 * Logical Link Control for protocol/service multiplexers.
 */
#define BT_GATT_OTS_L2CAP_PSM	0x0025

/* Maximum size of outgoing data. */
#define OT_TX_MTU 256
NET_BUF_POOL_FIXED_DEFINE(ot_chan_tx_pool, 1, BT_L2CAP_SDU_BUF_SIZE(OT_TX_MTU),
			  NULL);

/* List of Object Transfer Channels. */
static sys_slist_t channels;

static int ots_l2cap_send(struct bt_gatt_ots_l2cap *l2cap_ctx)
{
	int ret;
	struct net_buf *buf;
	uint32_t len;

	/* Calculate maximum length of data chunk. */
	len = MIN(l2cap_ctx->ot_chan.tx.mtu, OT_TX_MTU);
	len = MIN(len, l2cap_ctx->tx.len - l2cap_ctx->tx.len_sent);

	/* Prepare buffer for sending. */
	buf = net_buf_alloc(&ot_chan_tx_pool, K_FOREVER);
	net_buf_reserve(buf, BT_L2CAP_SDU_CHAN_SEND_RESERVE);
	net_buf_add_mem(buf, &l2cap_ctx->tx.data[l2cap_ctx->tx.len_sent], len);

	ret = bt_l2cap_chan_send(&l2cap_ctx->ot_chan.chan, buf);
	if (ret < 0) {
		LOG_ERR("Unable to send data over CoC: %d", ret);
		net_buf_unref(buf);

		return -ENOEXEC;
	}

	/* Mark that L2CAP TX was accepted. */
	l2cap_ctx->tx.len_sent += len;

	LOG_DBG("Sending TX chunk with %d bytes on L2CAP CoC", len);

	return 0;
}

static void l2cap_sent(struct bt_l2cap_chan *chan)
{
	struct bt_gatt_ots_l2cap *l2cap_ctx;

	LOG_DBG("Outgoing data channel %p transmitted", chan);

	l2cap_ctx = CONTAINER_OF(chan, struct bt_gatt_ots_l2cap, ot_chan);

	/* Ongoing TX - sending next chunk. */
	if (l2cap_ctx->tx.len != l2cap_ctx->tx.len_sent) {
		ots_l2cap_send(l2cap_ctx);

		return;
	}

	/* TX completed - notify upper layers and clean up. */
	memset(&l2cap_ctx->tx, 0, sizeof(l2cap_ctx->tx));

	LOG_DBG("Scheduled TX on L2CAP CoC is complete");

	if (l2cap_ctx->tx_done) {
		l2cap_ctx->tx_done(l2cap_ctx, chan->conn);
	}
}

static void l2cap_status(struct bt_l2cap_chan *chan, atomic_t *status)
{
	LOG_DBG("Channel %p status %u", chan, *status);
}

static void l2cap_connected(struct bt_l2cap_chan *chan)
{
	LOG_DBG("Channel %p connected", chan);
}

static void l2cap_disconnected(struct bt_l2cap_chan *chan)
{
	LOG_DBG("Channel %p disconnected", chan);
}

static const struct bt_l2cap_chan_ops l2cap_ops = {
	.sent		= l2cap_sent,
	.status		= l2cap_status,
	.connected	= l2cap_connected,
	.disconnected	= l2cap_disconnected,
};

static inline void l2cap_chan_init(struct bt_l2cap_le_chan *chan)
{
	chan->rx.mtu = CONFIG_BT_OTS_L2CAP_CHAN_RX_MTU;
	chan->chan.ops = &l2cap_ops;

	LOG_DBG("RX MTU set to %u", chan->rx.mtu);
}

static int l2cap_accept(struct bt_conn *conn, struct bt_l2cap_chan **chan)
{
	struct bt_gatt_ots_l2cap *l2cap_ctx;

	LOG_DBG("Incoming conn %p", (void *)conn);

	SYS_SLIST_FOR_EACH_CONTAINER(&channels, l2cap_ctx, node) {
		if (l2cap_ctx->ot_chan.chan.conn) {
			continue;
		}

		l2cap_chan_init(&l2cap_ctx->ot_chan);
		memset(&l2cap_ctx->tx, 0, sizeof(l2cap_ctx->tx));

		*chan = &l2cap_ctx->ot_chan.chan;

		return 0;
	}

	return -ENOMEM;
}

static struct bt_l2cap_server l2cap_server = {
	.psm = BT_GATT_OTS_L2CAP_PSM,
	.accept	= l2cap_accept,
};

static int bt_gatt_ots_l2cap_init(const struct device *arg)
{
	int err;

	sys_slist_init(&channels);

	err = bt_l2cap_server_register(&l2cap_server);
	if (err) {
		LOG_ERR("Unable to register OTS PSM");
		return err;
	}

	LOG_DBG("Initialized OTS L2CAP");

	return 0;
}

bool bt_gatt_ots_l2cap_is_open(struct bt_gatt_ots_l2cap *l2cap_ctx,
				   struct bt_conn *conn)
{
	return (l2cap_ctx->ot_chan.chan.conn == conn);
}

int bt_gatt_ots_l2cap_send(struct bt_gatt_ots_l2cap *l2cap_ctx,
			       uint8_t *data, uint32_t len)
{
	int err;

	if (l2cap_ctx->tx.len != 0) {
		LOG_ERR("L2CAP TX in progress");

		return -EAGAIN;
	}

	l2cap_ctx->tx.data = data;
	l2cap_ctx->tx.len = len;

	LOG_DBG("Starting TX on L2CAP CoC with %d byte packet", len);

	err = ots_l2cap_send(l2cap_ctx);
	if (err) {
		LOG_ERR("Unable to send data over CoC: %d", err);

		return err;
	}

	return 0;
}

int bt_gatt_ots_l2cap_register(struct bt_gatt_ots_l2cap *l2cap_ctx)
{
	sys_slist_append(&channels, &l2cap_ctx->node);

	return 0;
}

int bt_gatt_ots_l2cap_unregister(struct bt_gatt_ots_l2cap *l2cap_ctx)
{
	sys_slist_find_and_remove(&channels, &l2cap_ctx->node);

	return 0;
}

SYS_INIT(bt_gatt_ots_l2cap_init, APPLICATION,
	 CONFIG_APPLICATION_INIT_PRIORITY);
