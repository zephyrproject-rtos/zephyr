/* bnep.c - Bluetooth Network Encapsulation Protocol */

/*
 * Copyright (c) 2025
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stddef.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/buf.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/l2cap.h>

#include "host/conn_internal.h"
#include "host/hci_core.h"
#include "l2cap_br_internal.h"
#include "bnep_internal.h"

#define LOG_LEVEL CONFIG_BT_BNEP_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_bnep);

#define BNEP_HDR_SIZE 1
#define BNEP_DROP(_fmt, ...) LOG_ERR("Dropping BNEP packet: " _fmt, ##__VA_ARGS__)

NET_BUF_POOL_DEFINE(bnep_tx_pool, CONFIG_BT_BUF_ACL_TX_COUNT,
		    BT_BUF_ACL_SIZE(CONFIG_BT_BUF_ACL_TX_SIZE),
		    CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

NET_BUF_POOL_DEFINE(bnep_rx_pool, BT_BUF_ACL_RX_COUNT,
		    BT_BUF_ACL_SIZE(CONFIG_BT_BUF_ACL_RX_SIZE),
		    CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

static const struct bt_bnep_cb *bnep_cb;
static bt_bnep_accept_fn bnep_accept_fn;

static struct bt_bnep *bnep_from_chan(struct bt_l2cap_chan *chan)
{
	return (struct bt_bnep *)((char *)chan - offsetof(struct bt_bnep, chan.chan));
}

static struct net_buf *bnep_alloc_buf(struct bt_l2cap_chan *chan)
{
	ARG_UNUSED(chan);

	return net_buf_alloc(&bnep_rx_pool, K_NO_WAIT);
}

static int bnep_send_control(struct bt_bnep *bnep, uint8_t control_type,
			     const void *data, size_t len)
{
	struct net_buf *buf;
	uint8_t *hdr;

	buf = net_buf_alloc(&bnep_tx_pool, K_NO_WAIT);
	if (buf == NULL) {
		return -ENOMEM;
	}

	net_buf_reserve(buf, BT_L2CAP_CHAN_SEND_RESERVE);
	hdr = net_buf_add(buf, BNEP_HDR_SIZE + 1 + len);
	hdr[0] = BNEP_TYPE_CONTROL;
	hdr[1] = control_type;
	if (len > 0) {
		memcpy(&hdr[2], data, len);
	}

	return bt_l2cap_br_chan_send(&bnep->chan.chan, buf);
}

static int bnep_send_setup_rsp(struct bt_bnep *bnep, uint16_t result)
{
	uint16_t rsp = sys_cpu_to_be16(result);

	return bnep_send_control(bnep, BNEP_CONTROL_SETUP_CONN_RSP, &rsp, sizeof(rsp));
}

static int bnep_send_setup_req(struct bt_bnep *bnep)
{
	uint8_t data[1 + (2U * sizeof(uint16_t))];

	data[0] = sizeof(uint16_t);
	sys_put_be16(bnep->remote_service, &data[1]);
	sys_put_be16(bnep->local_service, &data[1 + sizeof(uint16_t)]);

	return bnep_send_control(bnep, BNEP_CONTROL_SETUP_CONN_REQ, data, sizeof(data));
}

static int bnep_send_filter_rsp(struct bt_bnep *bnep, uint8_t control_type, uint16_t result)
{
	uint16_t rsp = sys_cpu_to_be16(result);

	return bnep_send_control(bnep, control_type, &rsp, sizeof(rsp));
}

static void bnep_connected_notify(struct bt_bnep *bnep)
{
	bnep->state = BT_BNEP_STATE_CONNECTED;

	if (bnep->cb && bnep->cb->connected) {
		bnep->cb->connected(bnep);
	}
}

/*
 * PAN service UUIDs are sent big-endian and may use the 16-bit, 32-bit or
 * 128-bit form of the Bluetooth base UUID. Reduce them to the 16-bit value.
 */
static int bnep_pull_service_uuid(struct net_buf *buf, uint8_t uuid_size, uint16_t *service)
{
	static const uint8_t base_uuid_tail[] = {0x00, 0x00, 0x10, 0x00, 0x80, 0x00,
						 0x00, 0x80, 0x5f, 0x9b, 0x34, 0xfb};
	const uint8_t *uuid;

	if (buf->len < uuid_size) {
		return -EMSGSIZE;
	}

	uuid = net_buf_pull_mem(buf, uuid_size);

	switch (uuid_size) {
	case 2U:
		*service = sys_get_be16(uuid);
		return 0;
	case 4U:
		if (sys_get_be16(uuid) != 0U) {
			return -EINVAL;
		}
		*service = sys_get_be16(&uuid[2]);
		return 0;
	case 16U:
		if ((sys_get_be16(uuid) != 0U) ||
		    (memcmp(&uuid[4], base_uuid_tail, sizeof(base_uuid_tail)) != 0)) {
			return -EINVAL;
		}
		*service = sys_get_be16(&uuid[2]);
		return 0;
	default:
		return -EINVAL;
	}
}

static bool bnep_service_is_pan(uint16_t service)
{
	return (service == BNEP_SVC_PANU) || (service == BNEP_SVC_NAP) ||
	       (service == BNEP_SVC_GN);
}

static void bnep_handle_setup_req(struct bt_bnep *bnep, struct net_buf *buf)
{
	uint16_t dst_service;
	uint16_t src_service;
	uint8_t uuid_size;
	int err;

	if (buf->len < 1) {
		BNEP_DROP("setup request has no UUID size");
		(void)bnep_send_setup_rsp(bnep, BNEP_SETUP_INVALID_UUID_SIZE);
		return;
	}

	uuid_size = net_buf_pull_u8(buf);
	if ((uuid_size != 2U) && (uuid_size != 4U) && (uuid_size != 16U)) {
		BNEP_DROP("invalid setup UUID size %u", uuid_size);
		(void)bnep_send_setup_rsp(bnep, BNEP_SETUP_INVALID_UUID_SIZE);
		return;
	}

	err = bnep_pull_service_uuid(buf, uuid_size, &dst_service);
	if (err != 0) {
		BNEP_DROP("bad destination service UUID (%d)", err);
		(void)bnep_send_setup_rsp(bnep, (err == -EMSGSIZE) ? BNEP_SETUP_INVALID_UUID_SIZE
								   : BNEP_SETUP_INVALID_DST_UUID);
		return;
	}

	err = bnep_pull_service_uuid(buf, uuid_size, &src_service);
	if (err != 0) {
		BNEP_DROP("bad source service UUID (%d)", err);
		(void)bnep_send_setup_rsp(bnep, (err == -EMSGSIZE) ? BNEP_SETUP_INVALID_UUID_SIZE
								   : BNEP_SETUP_INVALID_SRC_UUID);
		return;
	}

	LOG_DBG("setup req dst 0x%04x src 0x%04x", dst_service, src_service);

	if (dst_service != bnep->local_service) {
		BNEP_DROP("destination service 0x%04x does not match local 0x%04x", dst_service,
			  bnep->local_service);
		(void)bnep_send_setup_rsp(bnep, BNEP_SETUP_INVALID_DST_UUID);
		return;
	}

	if (!bnep_service_is_pan(src_service)) {
		BNEP_DROP("unknown source service 0x%04x", src_service);
		(void)bnep_send_setup_rsp(bnep, BNEP_SETUP_INVALID_SRC_UUID);
		return;
	}

	bnep->remote_service = src_service;

	(void)bnep_send_setup_rsp(bnep, BNEP_SETUP_SUCCESS);
	bnep_connected_notify(bnep);
}

static void bnep_handle_setup_rsp(struct bt_bnep *bnep, struct net_buf *buf)
{
	uint16_t result;

	if (buf->len < 2) {
		BNEP_DROP("short setup response (%u < 2)", buf->len);
		bt_bnep_disconnect(bnep);
		return;
	}

	result = net_buf_pull_be16(buf);
	if (result != BNEP_SETUP_SUCCESS) {
		LOG_ERR("Peer rejected BNEP setup: 0x%04x", result);
		bt_bnep_disconnect(bnep);
		return;
	}

	bnep_connected_notify(bnep);
}

static void bnep_handle_control(struct bt_bnep *bnep, struct net_buf *buf)
{
	uint8_t control_type;

	if (buf->len < 1) {
		BNEP_DROP("empty control packet");
		return;
	}

	control_type = net_buf_pull_u8(buf);

	switch (control_type) {
	case BNEP_CONTROL_SETUP_CONN_REQ:
		bnep_handle_setup_req(bnep, buf);
		break;
	case BNEP_CONTROL_SETUP_CONN_RSP:
		bnep_handle_setup_rsp(bnep, buf);
		break;
	case BNEP_CONTROL_FILTER_MULTI_ADDR_REQ:
		(void)bnep_send_filter_rsp(bnep, BNEP_CONTROL_FILTER_MULTI_ADDR_RSP,
					   BNEP_SETUP_SUCCESS);
		break;
	case BNEP_CONTROL_FILTER_NET_TYPE_REQ:
		(void)bnep_send_filter_rsp(bnep, BNEP_CONTROL_FILTER_NET_TYPE_RSP,
					   BNEP_SETUP_SUCCESS);
		break;
	default:
		BNEP_DROP("unsupported control type 0x%02x", control_type);
		break;
	}
}

static int bnep_skip_extensions(struct net_buf *buf)
{
	for (;;) {
		uint8_t ext_type;
		uint8_t ext_len;

		if (buf->len < 2U) {
			return -EMSGSIZE;
		}

		ext_type = net_buf_pull_u8(buf);
		ext_len = net_buf_pull_u8(buf);
		if (buf->len < ext_len) {
			return -EMSGSIZE;
		}

		(void)net_buf_pull(buf, ext_len);

		if ((ext_type & BNEP_EXT_HEADER) == 0U) {
			return 0;
		}
	}
}

static int bnep_endpoint_macs(struct bt_bnep *bnep, uint8_t local[BNEP_ETH_ADDR_LEN],
			      uint8_t remote[BNEP_ETH_ADDR_LEN])
{
	const bt_addr_t *dst;

	if ((bnep == NULL) || (bnep->conn == NULL)) {
		return -ENOTCONN;
	}

	dst = bt_conn_get_dst_br(bnep->conn);
	if (dst == NULL) {
		return -ENOTCONN;
	}

	bnep_addr_to_mac(dst, remote);
	bnep_addr_to_mac(&bt_dev.id_addr[BT_ID_DEFAULT].a, local);

	return 0;
}

static void bnep_deliver_ethernet(struct bt_bnep *bnep, struct net_buf *frame)
{
	if ((bnep->state == BT_BNEP_STATE_CONNECTED) && bnep->cb && bnep->cb->recv) {
		bnep->cb->recv(bnep, frame);
	} else {
		BNEP_DROP("Ethernet frame while channel is not ready");
	}
}

static void bnep_handle_ethernet(struct bt_bnep *bnep, uint8_t type_byte, struct net_buf *buf)
{
	uint8_t type = type_byte & ~BNEP_EXT_HEADER;
	bool has_ext = (type_byte & BNEP_EXT_HEADER) != 0U;
	uint8_t local[BNEP_ETH_ADDR_LEN];
	uint8_t remote[BNEP_ETH_ADDR_LEN];
	uint8_t dst[BNEP_ETH_ADDR_LEN];
	uint8_t src[BNEP_ETH_ADDR_LEN];
	uint16_t proto;
	struct net_buf *frame;
	int err;

	if (type == BNEP_TYPE_GENERAL_ETHERNET && !has_ext) {
		/* Already a full Ethernet header + payload. */
		bnep_deliver_ethernet(bnep, buf);
		return;
	}

	err = bnep_endpoint_macs(bnep, local, remote);
	if (err != 0) {
		BNEP_DROP("cannot resolve endpoint MACs (%d)", err);
		return;
	}

	switch (type) {
	case BNEP_TYPE_GENERAL_ETHERNET:
		if (buf->len < BNEP_ETH_HDR_LEN) {
			BNEP_DROP("short general Ethernet frame (%u < %u)", buf->len,
				  BNEP_ETH_HDR_LEN);
			return;
		}
		memcpy(dst, net_buf_pull_mem(buf, BNEP_ETH_ADDR_LEN), BNEP_ETH_ADDR_LEN);
		memcpy(src, net_buf_pull_mem(buf, BNEP_ETH_ADDR_LEN), BNEP_ETH_ADDR_LEN);
		proto = net_buf_pull_be16(buf);
		break;
	case BNEP_TYPE_COMPRESSED_ETH:
		if (buf->len < sizeof(uint16_t)) {
			BNEP_DROP("short compressed Ethernet frame (%u < %zu)", buf->len,
				  sizeof(uint16_t));
			return;
		}
		memcpy(dst, local, BNEP_ETH_ADDR_LEN);
		memcpy(src, remote, BNEP_ETH_ADDR_LEN);
		proto = net_buf_pull_be16(buf);
		break;
	case BNEP_TYPE_COMPRESSED_SRC:
		if (buf->len < (BNEP_ETH_ADDR_LEN + sizeof(uint16_t))) {
			BNEP_DROP("short source-compressed frame (%u < %zu)", buf->len,
				  BNEP_ETH_ADDR_LEN + sizeof(uint16_t));
			return;
		}
		memcpy(dst, local, BNEP_ETH_ADDR_LEN);
		memcpy(src, net_buf_pull_mem(buf, BNEP_ETH_ADDR_LEN), BNEP_ETH_ADDR_LEN);
		proto = net_buf_pull_be16(buf);
		break;
	case BNEP_TYPE_COMPRESSED_DST:
		if (buf->len < (BNEP_ETH_ADDR_LEN + sizeof(uint16_t))) {
			BNEP_DROP("short destination-compressed frame (%u < %zu)", buf->len,
				  BNEP_ETH_ADDR_LEN + sizeof(uint16_t));
			return;
		}
		memcpy(dst, net_buf_pull_mem(buf, BNEP_ETH_ADDR_LEN), BNEP_ETH_ADDR_LEN);
		memcpy(src, remote, BNEP_ETH_ADDR_LEN);
		proto = net_buf_pull_be16(buf);
		break;
	default:
		BNEP_DROP("unsupported Ethernet type 0x%02x", type);
		return;
	}

	if (has_ext) {
		err = bnep_skip_extensions(buf);
		if (err != 0) {
			BNEP_DROP("invalid extension headers (%d)", err);
			return;
		}
	}

	frame = net_buf_alloc(&bnep_rx_pool, K_NO_WAIT);
	if (frame == NULL) {
		BNEP_DROP("no buffer for decompressed frame");
		return;
	}

	if (net_buf_tailroom(frame) < (BNEP_ETH_HDR_LEN + buf->len)) {
		BNEP_DROP("decompressed frame too large (%zu > %zu)",
			  BNEP_ETH_HDR_LEN + buf->len, net_buf_tailroom(frame));
		net_buf_unref(frame);
		return;
	}

	(void)net_buf_add_mem(frame, dst, BNEP_ETH_ADDR_LEN);
	(void)net_buf_add_mem(frame, src, BNEP_ETH_ADDR_LEN);
	(void)net_buf_add_be16(frame, proto);
	(void)net_buf_add_mem(frame, buf->data, buf->len);

	bnep_deliver_ethernet(bnep, frame);
	net_buf_unref(frame);
}

static int bnep_l2cap_recv(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	struct bt_bnep *bnep = bnep_from_chan(chan);
	uint8_t type_byte;
	uint8_t type;

	if (buf->len < 1) {
		BNEP_DROP("empty L2CAP payload");
		return -EINVAL;
	}

	type_byte = net_buf_pull_u8(buf);
	type = type_byte & ~BNEP_EXT_HEADER;

	switch (type) {
	case BNEP_TYPE_GENERAL_ETHERNET:
	case BNEP_TYPE_COMPRESSED_ETH:
	case BNEP_TYPE_COMPRESSED_SRC:
	case BNEP_TYPE_COMPRESSED_DST:
		bnep_handle_ethernet(bnep, type_byte, buf);
		break;
	case BNEP_TYPE_CONTROL:
		bnep_handle_control(bnep, buf);
		break;
	default:
		BNEP_DROP("unsupported packet type 0x%02x", type);
		break;
	}

	/* L2CAP owns and unrefs buf after recv returns (unless -EINPROGRESS). */
	return 0;
}

static void bnep_l2cap_connected(struct bt_l2cap_chan *chan)
{
	struct bt_bnep *bnep = bnep_from_chan(chan);

	LOG_DBG("bnep connected initiator %d", bnep->initiator);

	bnep->state = BT_BNEP_STATE_CONNECTING;

	if (bnep->initiator) {
		int err = bnep_send_setup_req(bnep);

		if (err != 0) {
			LOG_ERR("Failed to send BNEP setup request (%d)", err);
			bt_bnep_disconnect(bnep);
		}
	}
}

static void bnep_l2cap_disconnected(struct bt_l2cap_chan *chan)
{
	struct bt_bnep *bnep = bnep_from_chan(chan);

	LOG_DBG("bnep disconnected");

	bnep->state = BT_BNEP_STATE_DISCONNECTED;
	bnep->initiator = false;

	if (bnep->cb && bnep->cb->disconnected) {
		bnep->cb->disconnected(bnep);
	}
}

static const struct bt_l2cap_chan_ops bnep_l2cap_ops = {
	.alloc_buf = bnep_alloc_buf,
	.recv = bnep_l2cap_recv,
	.connected = bnep_l2cap_connected,
	.disconnected = bnep_l2cap_disconnected,
};

void bt_bnep_init(void)
{
	static bool initialized;
	static struct bt_l2cap_server bnep_l2cap = {
		.psm = BT_L2CAP_PSM_BNEP,
		.sec_level = BT_SECURITY_L2,
		.accept = bt_bnep_l2cap_accept,
	};
	int err;

	if (initialized) {
		return;
	}

	err = bt_l2cap_br_server_register(&bnep_l2cap);
	if ((err < 0) && (err != -EEXIST)) {
		LOG_ERR("BNEP L2CAP registration failed %d", err);
		return;
	}

	initialized = true;
}

int bt_bnep_register_cb(const struct bt_bnep_cb *cb)
{
	if (cb == NULL) {
		return -EINVAL;
	}

	bnep_cb = cb;
	return 0;
}

int bt_bnep_register_accept(bt_bnep_accept_fn accept)
{
	if (accept == NULL) {
		return -EINVAL;
	}

	bnep_accept_fn = accept;
	return 0;
}

int bt_bnep_l2cap_accept(struct bt_conn *conn, struct bt_l2cap_server *server,
			 struct bt_l2cap_chan **chan)
{
	struct bt_bnep *bnep;
	int err;

	ARG_UNUSED(server);

	if (bnep_accept_fn == NULL) {
		return -EINVAL;
	}

	err = bnep_accept_fn(conn, &bnep);
	if (err != 0) {
		return err;
	}

	*chan = &bnep->chan.chan;
	return 0;
}

static void bnep_chan_init(struct bt_bnep *bnep, struct bt_conn *conn)
{
	memset(bnep, 0, sizeof(*bnep));
	bnep->conn = conn;
	bnep->chan.chan.conn = conn;
	bnep->chan.chan.ops = &bnep_l2cap_ops;
	bnep->chan.rx.mtu = BT_L2CAP_RX_MTU;
	bnep->chan.required_sec_level = BT_SECURITY_L2;
	bnep->cb = bnep_cb;
	bnep->state = BT_BNEP_STATE_DISCONNECTED;
}

int bt_bnep_connect(struct bt_conn *conn, struct bt_bnep *bnep, uint16_t local_service,
		    uint16_t remote_service)
{
	int err;

	if (conn == NULL || bnep == NULL || bnep_cb == NULL) {
		return -EINVAL;
	}

	if (bnep->chan.chan.conn != NULL) {
		return -EALREADY;
	}

	bnep_chan_init(bnep, conn);
	bnep->local_service = local_service;
	bnep->remote_service = remote_service;
	bnep->initiator = true;

	err = bt_l2cap_br_chan_connect(conn, &bnep->chan.chan, BT_L2CAP_PSM_BNEP);
	if (err != 0) {
		memset(bnep, 0, sizeof(*bnep));
		return err;
	}

	return 0;
}

int bt_bnep_accept(struct bt_conn *conn, struct bt_bnep *bnep, uint16_t local_service)
{
	if (conn == NULL || bnep == NULL || bnep_cb == NULL) {
		return -EINVAL;
	}

	if (bnep->chan.chan.conn != NULL) {
		return -EALREADY;
	}

	bnep_chan_init(bnep, conn);
	bnep->local_service = local_service;
	bnep->initiator = false;

	return 0;
}

int bt_bnep_disconnect(struct bt_bnep *bnep)
{
	if (bnep == NULL || bnep->chan.chan.conn == NULL) {
		return -ENOTCONN;
	}

	return bt_l2cap_br_chan_disconnect(&bnep->chan.chan);
}

int bt_bnep_send(struct bt_bnep *bnep, struct net_buf *buf)
{
	if (bnep == NULL || bnep->state != BT_BNEP_STATE_CONNECTED) {
		return -ENOTCONN;
	}

	if (buf == NULL) {
		return -EINVAL;
	}

	net_buf_push_u8(buf, BNEP_TYPE_GENERAL_ETHERNET);

	return bt_l2cap_br_chan_send(&bnep->chan.chan, buf);
}

struct net_buf *bt_bnep_alloc_buf(size_t len)
{
	struct net_buf *buf;

	buf = net_buf_alloc(&bnep_tx_pool, K_NO_WAIT);
	if (buf == NULL) {
		return NULL;
	}

	net_buf_reserve(buf, BT_L2CAP_CHAN_SEND_RESERVE + BNEP_HDR_SIZE);

	if (net_buf_tailroom(buf) < len) {
		net_buf_unref(buf);
		return NULL;
	}

	return buf;
}
