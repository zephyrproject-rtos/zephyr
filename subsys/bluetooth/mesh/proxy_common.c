/*  Bluetooth Mesh */

/*
 * Copyright (c) 2017 Intel Corporation
 * Copyright (c) 2020 Lingao Meng
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/byteorder.h>

#include <net/buf.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/gatt.h>
#include <bluetooth/mesh.h>

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_MESH_DEBUG_PROXY_COMMON)
#define LOG_MODULE_NAME bt_mesh_proxy_common
#include "common/log.h"

#include "mesh.h"
#include "adv.h"
#include "net.h"
#include "rpl.h"
#include "prov.h"
#include "beacon.h"
#include "foundation.h"
#include "access.h"
#include "proxy_common.h"

#define PDU_SAR(data)      (data[0] >> 6)

/* Mesh Profile 1.0 Section 6.6:
 * "The timeout for the SAR transfer is 20 seconds. When the timeout
 *  expires, the Proxy Server shall disconnect."
 *
 * Mesh Profile 1.0 Section 6.7:
 * "The timeout for the SAR transfer is 20 seconds. When the timeout
 *  expires, the Proxy Client shall disconnect."
 */
#define PROXY_SAR_TIMEOUT  K_SECONDS(20)

#define SAR_COMPLETE       0x00
#define SAR_FIRST          0x01
#define SAR_CONT           0x02
#define SAR_LAST           0x03

#define PDU_HDR(sar, type) (sar << 6 | (type & BIT_MASK(6)))

static void proxy_sar_timeout(struct k_work *work)
{
	struct bt_mesh_proxy_object *object;

	BT_WARN("Proxy SAR timeout");

	object = CONTAINER_OF(work, struct bt_mesh_proxy_object, sar_timer);
	if (object->conn) {
		bt_conn_disconnect(object->conn,
				   BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	}
}

#if defined(CONFIG_BT_MESH_GATT_PROXY) || \
	defined(CONFIG_BT_MESH_PROXY_CLIENT)
static void proxy_cfg(struct bt_mesh_proxy_object *object)
{
	NET_BUF_SIMPLE_DEFINE(buf, 29);
	struct bt_mesh_net_rx rx;
	int err;

	err = bt_mesh_net_decode(&object->buf, BT_MESH_NET_IF_PROXY_CFG,
				 &rx, &buf);
	if (err) {
		BT_ERR("Failed to decode Proxy Configuration (err %d)", err);
		return;
	}

	rx.local_match = 1U;

	if (bt_mesh_rpl_check(&rx, NULL)) {
		BT_WARN("Replay: src 0x%04x dst 0x%04x seq 0x%06x",
			rx.ctx.addr, rx.ctx.recv_dst, rx.seq);
		return;
	}

	/* Remove network headers */
	net_buf_simple_pull(&buf, BT_MESH_NET_HDR_LEN);

	BT_DBG("%u bytes: %s", buf.len, bt_hex(buf.data, buf.len));

	if (buf.len < 1) {
		BT_WARN("Too short proxy configuration PDU");
		return;
	}

	if (object->cb.recv_cb) {
		object->cb.recv_cb(object->conn, &rx, &buf);
	}
}
#endif

static void proxy_complete_pdu(struct bt_mesh_proxy_object *object)
{
	switch (object->msg_type) {
#if defined(CONFIG_BT_MESH_GATT_PROXY)
	case BT_MESH_PROXY_NET_PDU:
		BT_DBG("Mesh Network PDU");
		bt_mesh_net_recv(&object->buf, 0, BT_MESH_NET_IF_PROXY);
		break;
	case BT_MESH_PROXY_BEACON:
		BT_DBG("Mesh Beacon PDU");
		bt_mesh_beacon_recv(&object->buf);
		break;
#endif
#if defined(CONFIG_BT_MESH_GATT_PROXY) || \
	defined(CONFIG_BT_MESH_PROXY_CLIENT)
	case BT_MESH_PROXY_CONFIG:
		BT_DBG("Mesh Configuration PDU");
		proxy_cfg(object);
		break;
#endif
#if defined(CONFIG_BT_MESH_PB_GATT)
	case BT_MESH_PROXY_PROV:
		BT_DBG("Mesh Provisioning PDU");
		bt_mesh_pb_gatt_recv(object->conn, &object->buf);
		break;
#endif
	default:
		BT_WARN("Unhandled Message Type 0x%02x", object->msg_type);
		break;
	}

	net_buf_simple_reset(&object->buf);
}

int bt_mesh_proxy_common_recv(struct bt_mesh_proxy_object *object,
			      const void *buf, uint16_t len)
{
	const uint8_t *data = buf;

	switch (PDU_SAR(data)) {
	case SAR_COMPLETE:
		if (object->buf.len) {
			BT_WARN("Complete PDU while a pending incomplete one");
			return -EINVAL;
		}

		object->msg_type = PDU_TYPE(data);
		net_buf_simple_add_mem(&object->buf, data + 1, len - 1);
		proxy_complete_pdu(object);
		break;

	case SAR_FIRST:
		if (object->buf.len) {
			BT_WARN("First PDU while a pending incomplete one");
			return -EINVAL;
		}

		k_delayed_work_submit(&object->sar_timer, PROXY_SAR_TIMEOUT);
		object->msg_type = PDU_TYPE(data);
		net_buf_simple_add_mem(&object->buf, data + 1, len - 1);
		break;

	case SAR_CONT:
		if (!object->buf.len) {
			BT_WARN("Continuation with no prior data");
			return -EINVAL;
		}

		if (object->msg_type != PDU_TYPE(data)) {
			BT_WARN("Unexpected message type in continuation");
			return -EINVAL;
		}

		k_delayed_work_submit(&object->sar_timer, PROXY_SAR_TIMEOUT);
		net_buf_simple_add_mem(&object->buf, data + 1, len - 1);
		break;

	case SAR_LAST:
		if (!object->buf.len) {
			BT_WARN("Last SAR PDU with no prior data");
			return -EINVAL;
		}

		if (object->msg_type != PDU_TYPE(data)) {
			BT_WARN("Unexpected message type in last SAR PDU");
			return -EINVAL;
		}

		k_delayed_work_cancel(&object->sar_timer);
		net_buf_simple_add_mem(&object->buf, data + 1, len - 1);
		proxy_complete_pdu(object);
		break;
	}

	return len;
}

int bt_mesh_proxy_common_send(struct bt_mesh_proxy_object *object,
			      uint8_t type, struct net_buf_simple *msg)
{
	struct bt_conn *conn = object->conn;
	uint16_t mtu;

	if (!conn) {
		BT_ERR("Not Connected");
		return -ENOTCONN;
	}

	BT_DBG("conn %p type 0x%02x len %u: %s", conn, type, msg->len,
	       bt_hex(msg->data, msg->len));

	/* ATT_MTU - OpCode (1 byte) - Handle (2 bytes) */
	mtu = bt_gatt_get_mtu(conn) - 3;
	if (mtu > msg->len) {
		net_buf_simple_push_u8(msg, PDU_HDR(SAR_COMPLETE, type));
		return object->cb.send_cb(conn, msg->data, msg->len);
	}

	net_buf_simple_push_u8(msg, PDU_HDR(SAR_FIRST, type));
	object->cb.send_cb(conn, msg->data, mtu);
	net_buf_simple_pull(msg, mtu);

	while (msg->len) {
		if (msg->len + 1 < mtu) {
			net_buf_simple_push_u8(msg, PDU_HDR(SAR_LAST, type));
			object->cb.send_cb(conn, msg->data, msg->len);
			break;
		}

		net_buf_simple_push_u8(msg, PDU_HDR(SAR_CONT, type));
		object->cb.send_cb(conn, msg->data, mtu);
		net_buf_simple_pull(msg, mtu);
	}

	return 0;
}

void bt_mesh_proxy_common_init(struct bt_mesh_proxy_object *object,
			       uint8_t *buf, uint16_t len)
{
	object->buf.size = len;
	object->buf.__buf = buf;

	k_delayed_work_init(&object->sar_timer, proxy_sar_timeout);
}
