/*
 * Copyright (c) 2025 SiFli Technologies(Nanjing) Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/buf.h>
#include <zephyr/bluetooth/classic/sco.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include "host/conn_internal.h"
#include "host/hci_core.h"
#include "sco_internal.h"

LOG_MODULE_REGISTER(bt_sco_data, CONFIG_BT_LOG_LEVEL);

static uint8_t bt_sco_tx_mtu_get(void)
{
	uint8_t mtu = CONFIG_BT_BUF_SCO_TX_SIZE;

	if (bt_dev.br.sco_mtu > 0) {
		mtu = MIN(mtu, bt_dev.br.sco_mtu);
	}

	return mtu;
}

/* SCO data is not flow controlled in either direction: voice is a constant
 * bit rate stream that must be dropped rather than queued when a peer cannot
 * keep up. The Host therefore neither enables Synchronous Flow Control nor
 * acknowledges received packets with Host Number of Completed Packets.
 */
NET_BUF_POOL_FIXED_DEFINE(sco_rx_pool, CONFIG_BT_BUF_SCO_RX_COUNT,
			  BT_BUF_SCO_SIZE(CONFIG_BT_BUF_SCO_RX_SIZE), 0, NULL);

NET_BUF_POOL_FIXED_DEFINE(sco_tx_pool, CONFIG_BT_BUF_SCO_TX_COUNT,
			  BT_BUF_SCO_SIZE(CONFIG_BT_BUF_SCO_TX_SIZE), 0, NULL);

void hci_sco(struct net_buf *buf)
{
	struct bt_hci_sco_hdr *hdr;
	uint16_t handle, len;
	struct bt_conn *conn;
	struct bt_sco_chan *chan;

	if (buf->len < sizeof(*hdr)) {
		LOG_ERR("Invalid HCI SCO packet size (%u)", buf->len);
		net_buf_unref(buf);
		return;
	}

	hdr = net_buf_pull_mem(buf, sizeof(*hdr));
	len = hdr->len;
	/* Handle field is 12 bits; upper bits carry the Packet Status Flag and RFU */
	handle = bt_sco_handle(sys_le16_to_cpu(hdr->handle));

	if (buf->len != len) {
		LOG_ERR("SCO data length mismatch (%u != %u)", buf->len, len);
		net_buf_unref(buf);
		return;
	}

	conn = bt_conn_lookup_handle(handle, BT_CONN_TYPE_SCO);
	if (conn == NULL) {
		LOG_ERR("Unable to find SCO conn for handle %u", handle);
		net_buf_unref(buf);
		return;
	}

	chan = conn->sco.chan;

	if (chan != NULL && chan->recv != NULL) {
		chan->recv(conn, buf);
	} else {
		net_buf_unref(buf);
	}

	bt_conn_unref(conn);
}

struct net_buf *bt_sco_buf_alloc(k_timeout_t timeout)
{
	struct net_buf *buf;

	buf = net_buf_alloc(&sco_tx_pool, timeout);
	if (buf != NULL) {
		net_buf_reserve(buf, BT_BUF_SCO_SIZE(0));
	}

	return buf;
}

int bt_sco_send(struct bt_conn *conn, struct net_buf *buf)
{
	struct bt_hci_sco_hdr *hdr;

	if (conn == NULL || buf == NULL) {
		return -EINVAL;
	}

	if (!bt_conn_is_sco(conn) || conn->state != BT_CONN_CONNECTED) {
		net_buf_unref(buf);
		return -ENOTCONN;
	}

	if (buf->len > bt_sco_tx_mtu_get()) {
		net_buf_unref(buf);
		return -EMSGSIZE;
	}

	if (net_buf_headroom(buf) < BT_BUF_SCO_SIZE(0)) {
		net_buf_unref(buf);
		return -EMSGSIZE;
	}

	hdr = net_buf_push(buf, sizeof(*hdr));
	/* Packet Status Flag and RFU bits must be 0 for host-to-controller SCO data */
	hdr->handle = sys_cpu_to_le16(bt_sco_handle_pack(conn->handle, 0));
	hdr->len = buf->len - sizeof(*hdr);

	net_buf_push_u8(buf, BT_HCI_H4_SCO);

	return bt_send(buf);
}

int bt_sco_recv_cb_set(struct bt_conn *sco_conn,
		       void (*recv)(struct bt_conn *sco_conn, struct net_buf *buf))
{
	struct bt_sco_chan *chan;

	if (sco_conn == NULL || !bt_conn_is_sco(sco_conn)) {
		return -EINVAL;
	}

	chan = sco_conn->sco.chan;
	if (chan == NULL) {
		return -ENOTCONN;
	}

	chan->recv = recv;

	return 0;
}

struct net_buf *bt_sco_get_rx(k_timeout_t timeout)
{
	struct net_buf *buf;

	buf = net_buf_alloc(&sco_rx_pool, timeout);
	if (buf != NULL) {
		net_buf_add_u8(buf, BT_HCI_H4_SCO);
	}

	return buf;
}
