/*  Bluetooth Mesh */

/*
 * Copyright (c) 2017 Intel Corporation
 * Copyright (c) 2021 Lingao Meng
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

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_MESH_DEBUG_PROXY)
#define LOG_MODULE_NAME bt_mesh_proxy
#include "common/log.h"

#include "mesh.h"
#include "adv.h"
#include "net.h"
#include "rpl.h"
#include "transport.h"
#include "prov.h"
#include "beacon.h"
#include "foundation.h"
#include "access.h"
#include "proxy.h"
#include "proxy_msg.h"

#define PDU_SAR(data)      (data[0] >> 6)

/* Mesh Profile 1.0 Section 6.6:
 * "The timeout for the SAR transfer is 20 seconds. When the timeout
 *  expires, the Proxy Server shall disconnect."
 */
#define PROXY_SAR_TIMEOUT  K_SECONDS(20)

#define SAR_COMPLETE       0x00
#define SAR_FIRST          0x01
#define SAR_CONT           0x02
#define SAR_LAST           0x03

#define PDU_HDR(sar, type) (sar << 6 | (type & BIT_MASK(6)))

static void proxy_sar_timeout(struct k_work *work)
{
	struct bt_mesh_proxy_role *role;

	BT_WARN("Proxy SAR timeout");

	role = CONTAINER_OF(work, struct bt_mesh_proxy_role, sar_timer);
	if (role->conn) {
		bt_conn_disconnect(role->conn,
				   BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	}
}

#if defined(CONFIG_BT_MESH_PROXY)
static void proxy_cfg(struct bt_mesh_proxy_role *role)
{
	NET_BUF_SIMPLE_DEFINE(buf, BT_MESH_NET_MAX_PDU_LEN);
	struct bt_mesh_net_rx rx;
	int err;

	err = bt_mesh_net_decode(&role->buf, BT_MESH_NET_IF_PROXY_CFG,
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

	role->cb.recv(role->conn, &rx, &buf);
}
#endif

static void proxy_complete_pdu(struct bt_mesh_proxy_role *role)
{
	switch (role->msg_type) {
#if defined(CONFIG_BT_MESH_PROXY)
	case BT_MESH_PROXY_NET_PDU:
		BT_DBG("Mesh Network PDU");
		bt_mesh_net_recv(&role->buf, 0, BT_MESH_NET_IF_PROXY);
		break;
	case BT_MESH_PROXY_BEACON:
		BT_DBG("Mesh Beacon PDU");
		bt_mesh_beacon_recv(&role->buf);
		break;
	case BT_MESH_PROXY_CONFIG:
		BT_DBG("Mesh Configuration PDU");
		proxy_cfg(role);
		break;
#endif
#if defined(CONFIG_BT_MESH_PB_GATT)
	case BT_MESH_PROXY_PROV:
		BT_DBG("Mesh Provisioning PDU");
		bt_mesh_pb_gatt_recv(role->conn, &role->buf);
		break;
#endif
	default:
		BT_WARN("Unhandled Message Type 0x%02x", role->msg_type);
		break;
	}

	net_buf_simple_reset(&role->buf);
}

ssize_t bt_mesh_proxy_msg_recv(struct bt_mesh_proxy_role *role,
			       const void *buf, uint16_t len)
{
	const uint8_t *data = buf;

	switch (PDU_SAR(data)) {
	case SAR_COMPLETE:
		if (role->buf.len) {
			BT_WARN("Complete PDU while a pending incomplete one");
			return -EINVAL;
		}

		role->msg_type = PDU_TYPE(data);
		net_buf_simple_add_mem(&role->buf, data + 1, len - 1);
		proxy_complete_pdu(role);
		break;

	case SAR_FIRST:
		if (role->buf.len) {
			BT_WARN("First PDU while a pending incomplete one");
			return -EINVAL;
		}

		k_work_reschedule(&role->sar_timer, PROXY_SAR_TIMEOUT);
		role->msg_type = PDU_TYPE(data);
		net_buf_simple_add_mem(&role->buf, data + 1, len - 1);
		break;

	case SAR_CONT:
		if (!role->buf.len) {
			BT_WARN("Continuation with no prior data");
			return -EINVAL;
		}

		if (role->msg_type != PDU_TYPE(data)) {
			BT_WARN("Unexpected message type in continuation");
			return -EINVAL;
		}

		k_work_reschedule(&role->sar_timer, PROXY_SAR_TIMEOUT);
		net_buf_simple_add_mem(&role->buf, data + 1, len - 1);
		break;

	case SAR_LAST:
		if (!role->buf.len) {
			BT_WARN("Last SAR PDU with no prior data");
			return -EINVAL;
		}

		if (role->msg_type != PDU_TYPE(data)) {
			BT_WARN("Unexpected message type in last SAR PDU");
			return -EINVAL;
		}

		/* If this fails, the work handler exits early, as there's no
		 * active SAR buffer.
		 */
		(void)k_work_cancel_delayable(&role->sar_timer);
		net_buf_simple_add_mem(&role->buf, data + 1, len - 1);
		proxy_complete_pdu(role);
		break;
	}

	return len;
}

int bt_mesh_proxy_msg_send(struct bt_mesh_proxy_role *role, uint8_t type,
			   struct net_buf_simple *msg,
			   bt_gatt_complete_func_t end, void *user_data)
{
	int err;
	uint16_t mtu;
	struct bt_conn *conn = role->conn;

	BT_DBG("conn %p type 0x%02x len %u: %s", (void *)conn, type, msg->len,
	       bt_hex(msg->data, msg->len));

	/* ATT_MTU - OpCode (1 byte) - Handle (2 bytes) */
	mtu = bt_gatt_get_mtu(conn) - 3;
	if (mtu > msg->len) {
		net_buf_simple_push_u8(msg, PDU_HDR(SAR_COMPLETE, type));
		return role->cb.send(conn, msg->data, msg->len, end, user_data);
	}

	net_buf_simple_push_u8(msg, PDU_HDR(SAR_FIRST, type));
	err = role->cb.send(conn, msg->data, mtu, NULL, NULL);
	if (err) {
		return err;
	}

	net_buf_simple_pull(msg, mtu);

	while (msg->len) {
		if (msg->len + 1 < mtu) {
			net_buf_simple_push_u8(msg, PDU_HDR(SAR_LAST, type));
			err = role->cb.send(conn, msg->data, msg->len, end, user_data);
			if (err) {
				return err;
			}

			break;
		}

		net_buf_simple_push_u8(msg, PDU_HDR(SAR_CONT, type));
		err = role->cb.send(conn, msg->data, mtu, NULL, NULL);
		if (err) {
			return err;
		}

		net_buf_simple_pull(msg, mtu);
	}

	return 0;
}

void bt_mesh_proxy_msg_init(struct bt_mesh_proxy_role *role)
{
	k_work_init_delayable(&role->sar_timer, proxy_sar_timeout);
}
