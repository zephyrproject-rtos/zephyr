/*  Bluetooth Mesh */

/*
 * Copyright (c) 2017 Intel Corporation
 * Copyright (c) 2021 Lingao Meng
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/net/buf.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/mesh.h>

#include <zephyr/bluetooth/hci.h>

#include "common/bt_str.h"

#include "mesh.h"
#include "adv.h"
#include "net.h"
#include "rpl.h"
#include "transport.h"
#include "host/ecc.h"
#include "prov.h"
#include "beacon.h"
#include "foundation.h"
#include "access.h"
#include "proxy.h"
#include "proxy_msg.h"

#define LOG_LEVEL CONFIG_BT_MESH_PROXY_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_mesh_proxy);

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

static uint8_t __noinit bufs[CONFIG_BT_MAX_CONN * CONFIG_BT_MESH_PROXY_MSG_LEN];

static struct bt_mesh_proxy_role roles[CONFIG_BT_MAX_CONN];

static int conn_count;

static void proxy_sar_timeout(struct k_work *work)
{
	struct bt_mesh_proxy_role *role;
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);

	LOG_WRN("Proxy SAR timeout");

	role = CONTAINER_OF(dwork, struct bt_mesh_proxy_role, sar_timer);
	if (role->conn) {
		bt_conn_disconnect(role->conn,
				   BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	}
}

ssize_t bt_mesh_proxy_msg_recv(struct bt_conn *conn,
			       const void *buf, uint16_t len)
{
	const uint8_t *data = buf;
	struct bt_mesh_proxy_role *role = &roles[bt_conn_index(conn)];

	switch (PDU_SAR(data)) {
	case SAR_COMPLETE:
		if (role->buf.len) {
			LOG_WRN("Complete PDU while a pending incomplete one");
			return -EINVAL;
		}

		role->msg_type = PDU_TYPE(data);
		net_buf_simple_add_mem(&role->buf, data + 1, len - 1);
		role->cb.recv(role);
		net_buf_simple_reset(&role->buf);
		break;

	case SAR_FIRST:
		if (role->buf.len) {
			LOG_WRN("First PDU while a pending incomplete one");
			return -EINVAL;
		}

		k_work_reschedule(&role->sar_timer, PROXY_SAR_TIMEOUT);
		role->msg_type = PDU_TYPE(data);
		net_buf_simple_add_mem(&role->buf, data + 1, len - 1);
		break;

	case SAR_CONT:
		if (!role->buf.len) {
			LOG_WRN("Continuation with no prior data");
			return -EINVAL;
		}

		if (role->msg_type != PDU_TYPE(data)) {
			LOG_WRN("Unexpected message type in continuation");
			return -EINVAL;
		}

		k_work_reschedule(&role->sar_timer, PROXY_SAR_TIMEOUT);
		net_buf_simple_add_mem(&role->buf, data + 1, len - 1);
		break;

	case SAR_LAST:
		if (!role->buf.len) {
			LOG_WRN("Last SAR PDU with no prior data");
			return -EINVAL;
		}

		if (role->msg_type != PDU_TYPE(data)) {
			LOG_WRN("Unexpected message type in last SAR PDU");
			return -EINVAL;
		}

		/* If this fails, the work handler exits early, as there's no
		 * active SAR buffer.
		 */
		(void)k_work_cancel_delayable(&role->sar_timer);
		net_buf_simple_add_mem(&role->buf, data + 1, len - 1);
		role->cb.recv(role);
		net_buf_simple_reset(&role->buf);
		break;
	}

	return len;
}

int bt_mesh_proxy_msg_send(struct bt_conn *conn, uint8_t type,
			   struct net_buf_simple *msg,
			   bt_gatt_complete_func_t end, void *user_data)
{
	int err;
	uint16_t mtu;
	struct bt_mesh_proxy_role *role = &roles[bt_conn_index(conn)];

	LOG_DBG("conn %p type 0x%02x len %u: %s", (void *)conn, type, msg->len,
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
		if (msg->len + 1 <= mtu) {
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

static void buf_send_end(struct bt_conn *conn, void *user_data)
{
	struct net_buf *buf = user_data;

	net_buf_unref(buf);
}

int bt_mesh_proxy_relay_send(struct bt_conn *conn, struct net_buf *buf)
{
	int err;

	NET_BUF_SIMPLE_DEFINE(msg, 1 + BT_MESH_NET_MAX_PDU_LEN);

	/* Proxy PDU sending modifies the original buffer,
	 * so we need to make a copy.
	 */
	net_buf_simple_reserve(&msg, 1);
	net_buf_simple_add_mem(&msg, buf->data, buf->len);

	err = bt_mesh_proxy_msg_send(conn, BT_MESH_PROXY_NET_PDU,
				     &msg, buf_send_end, net_buf_ref(buf));

	bt_mesh_adv_send_start(0, err, BT_MESH_ADV(buf));
	if (err) {
		LOG_ERR("Failed to send proxy message (err %d)", err);

		/* If segment_and_send() fails the buf_send_end() callback will
		 * not be called, so we need to clear the user data (net_buf,
		 * which is just opaque data to segment_and send) reference given
		 * to segment_and_send() here.
		 */
		net_buf_unref(buf);
	}

	return err;
}

static void proxy_msg_init(struct bt_mesh_proxy_role *role)
{
	/* Check if buf has been allocated, in this way, we no longer need
	 * to repeat the operation.
	 */
	if (role->buf.__buf) {
		net_buf_simple_reset(&role->buf);
		return;
	}

	net_buf_simple_init_with_data(&role->buf,
				      &bufs[bt_conn_index(role->conn) *
					    CONFIG_BT_MESH_PROXY_MSG_LEN],
				      CONFIG_BT_MESH_PROXY_MSG_LEN);

	net_buf_simple_reset(&role->buf);

	k_work_init_delayable(&role->sar_timer, proxy_sar_timeout);
}

struct bt_mesh_proxy_role *bt_mesh_proxy_role_setup(struct bt_conn *conn,
						    proxy_send_cb_t send,
						    proxy_recv_cb_t recv)
{
	struct bt_mesh_proxy_role *role;

	conn_count++;

	role = &roles[bt_conn_index(conn)];

	role->conn = bt_conn_ref(conn);
	proxy_msg_init(role);

	role->cb.recv = recv;
	role->cb.send = send;

	return role;
}

void bt_mesh_proxy_role_cleanup(struct bt_mesh_proxy_role *role)
{
	/* If this fails, the work handler exits early, as
	 * there's no active connection.
	 */
	(void)k_work_cancel_delayable(&role->sar_timer);
	bt_conn_unref(role->conn);
	role->conn = NULL;

	conn_count--;

	bt_mesh_adv_gatt_update();
}

bool bt_mesh_proxy_has_avail_conn(void)
{
	return conn_count < CONFIG_BT_MESH_MAX_CONN;
}
