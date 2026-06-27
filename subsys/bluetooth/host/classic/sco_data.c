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

#define sco(buf) ((struct bt_conn_rx *)net_buf_user_data(buf))

static uint8_t bt_sco_tx_mtu_get(void)
{
	uint8_t mtu = CONFIG_BT_BUF_SCO_TX_SIZE;

	if (bt_dev.br.sco_mtu > 0) {
		mtu = MIN(mtu, bt_dev.br.sco_mtu);
	}

	return mtu;
}

static void sco_send_num_completed_packets(uint16_t handle)
{
	struct bt_hci_cp_host_num_completed_packets *cp;
	struct bt_hci_handle_count *hc;
	struct net_buf *buf;
	int err;

	buf = bt_hci_cmd_alloc(K_NO_WAIT);
	if (buf == NULL) {
		LOG_WRN("Unable to alloc NCP for SCO handle %u", handle);
		return;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	cp->num_handles = 1;

	hc = net_buf_add(buf, sizeof(*hc));
	hc->handle = sys_cpu_to_le16(handle);
	hc->count = sys_cpu_to_le16(1);

	err = bt_hci_cmd_send(BT_HCI_OP_HOST_NUM_COMPLETED_PACKETS, buf);
	if (err != 0) {
		LOG_WRN("Failed to send NCP for SCO handle %u (err %d)", handle, err);
	}
}

static void sco_rx_pool_destroy(struct net_buf *buf)
{
	struct bt_conn *conn;
	uint8_t index = sco(buf)->index;
	uint16_t handle = sco(buf)->handle;

	net_buf_destroy(buf);

	if (index == BT_CONN_INDEX_INVALID) {
		return;
	}

	conn = bt_conn_lookup_index(index);
	if (conn == NULL) {
		sco_send_num_completed_packets(handle);
		return;
	}

	if (conn->state == BT_CONN_CONNECTED) {
		sco_send_num_completed_packets(conn->handle);
	}

	bt_conn_unref(conn);
}

NET_BUF_POOL_FIXED_DEFINE(sco_rx_pool, CONFIG_BT_BUF_SCO_RX_COUNT,
			  BT_BUF_SCO_SIZE(CONFIG_BT_BUF_SCO_RX_SIZE),
			  sizeof(struct bt_conn_rx), sco_rx_pool_destroy);

NET_BUF_POOL_FIXED_DEFINE(sco_tx_pool, CONFIG_BT_BUF_SCO_TX_COUNT,
			  BT_BUF_SCO_SIZE(CONFIG_BT_BUF_SCO_TX_SIZE), 0, NULL);

int bt_sco_over_hci_init(void)
{
	struct bt_hci_cp_write_sco_flow_control *sco_fc;
	struct net_buf *buf;
	int err;

	if (!IS_ENABLED(CONFIG_BT_SCO_OVER_HCI)) {
		return 0;
	}

	/* The SCO buffer parameters are configured as part of the single
	 * HCI_Host_Buffer_Size command sent during bt_hci_init(). Here we only
	 * enable SCO flow control so the controller reports Number of Completed
	 * Packets for SCO handles.
	 */
	buf = bt_hci_cmd_alloc(K_FOREVER);
	if (buf == NULL) {
		return -ENOBUFS;
	}

	sco_fc = net_buf_add(buf, sizeof(*sco_fc));
	sco_fc->flow_enable = BT_HCI_SCO_FLOW_CTRL_ENABLED;

	err = bt_hci_cmd_send_sync(BT_HCI_OP_WRITE_SCO_FLOW_CONTROL, buf, NULL);
	if (err != 0) {
		LOG_ERR("Failed to enable SCO flow control (err %d)", err);
		return err;
	}

	LOG_DBG("SCO over HCI initialized");

	return 0;
}

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

	sco(buf)->handle = handle;
	sco(buf)->index = BT_CONN_INDEX_INVALID;

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

	sco(buf)->index = bt_conn_index(conn);

	chan = conn->sco.chan;

	if (chan != NULL && chan->recv != NULL) {
		chan->recv(conn, buf);
	} else {
		/* Buffer freed in sco_rx_pool_destroy which sends Num Completed Packets */
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
