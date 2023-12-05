/*
 * Copyright (c) 2017 Intel Corporation
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/bluetooth/mesh.h>
#include <zephyr/bluetooth/conn.h>
#include "net.h"
#include "proxy.h"
#include "prov.h"
#include "pb_gatt.h"
#include "proxy_msg.h"
#include "pb_gatt_srv.h"
#include "pb_gatt_cli.h"

#include <zephyr/bluetooth/hci.h>

#include "common/bt_str.h"

#define LOG_LEVEL CONFIG_BT_MESH_PROV_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_mesh_pb_gatt);

struct prov_bearer_send_cb {
	prov_bearer_send_complete_t cb;
	void *cb_data;
};

struct prov_link {
	struct bt_conn *conn;
	const struct prov_bearer_cb *cb;
	void *cb_data;
	struct prov_bearer_send_cb comp;
	struct k_work_delayable prot_timer;
};

static struct prov_link link;

static void reset_state(void)
{
	if (link.conn) {
		bt_conn_unref(link.conn);
		link.conn = NULL;
	}

	/* If this fails, the protocol timeout handler will exit early. */
	(void)k_work_cancel_delayable(&link.prot_timer);
}

static void link_closed(enum prov_bearer_link_status status)
{
	const struct prov_bearer_cb *cb = link.cb;
	void *cb_data = link.cb_data;

	reset_state();

	cb->link_closed(&bt_mesh_pb_gatt, cb_data, status);
}

static void protocol_timeout(struct k_work *work)
{
	if (!atomic_test_bit(bt_mesh_prov_link.flags, LINK_ACTIVE)) {
		return;
	}

	/* If connection failed or timeout, not allow establish connection */
	if (IS_ENABLED(CONFIG_BT_MESH_PB_GATT_CLIENT) &&
	    atomic_test_bit(bt_mesh_prov_link.flags, PROVISIONER)) {
		if (link.conn) {
			(void)bt_conn_disconnect(link.conn,
						 BT_HCI_ERR_REMOTE_USER_TERM_CONN);
		} else {
			(void)bt_mesh_pb_gatt_cli_setup(NULL);
		}
	}

	LOG_DBG("Protocol timeout");

	link_closed(PROV_BEARER_LINK_STATUS_TIMEOUT);
}

int bt_mesh_pb_gatt_recv(struct bt_conn *conn, struct net_buf_simple *buf)
{
	LOG_DBG("%u bytes: %s", buf->len, bt_hex(buf->data, buf->len));

	if (link.conn != conn || !link.cb) {
		LOG_WRN("Data for unexpected connection");
		return -ENOTCONN;
	}

	if (buf->len < 1) {
		LOG_WRN("Too short provisioning packet (len %u)", buf->len);
		return -EINVAL;
	}

	k_work_reschedule(&link.prot_timer, bt_mesh_prov_protocol_timeout_get());

	link.cb->recv(&bt_mesh_pb_gatt, link.cb_data, buf);

	return 0;
}

int bt_mesh_pb_gatt_start(struct bt_conn *conn)
{
	LOG_DBG("conn %p", (void *)conn);

	if (link.conn) {
		return -EBUSY;
	}

	link.conn = bt_conn_ref(conn);
	k_work_reschedule(&link.prot_timer, bt_mesh_prov_protocol_timeout_get());

	link.cb->link_opened(&bt_mesh_pb_gatt, link.cb_data);

	return 0;
}

int bt_mesh_pb_gatt_close(struct bt_conn *conn)
{
	LOG_DBG("conn %p", (void *)conn);

	if (link.conn != conn) {
		LOG_DBG("Not connected");
		return -ENOTCONN;
	}

	link_closed(PROV_BEARER_LINK_STATUS_SUCCESS);

	return 0;
}

#if defined(CONFIG_BT_MESH_PB_GATT_CLIENT)
int bt_mesh_pb_gatt_cli_start(struct bt_conn *conn)
{
	LOG_DBG("conn %p", (void *)conn);

	if (link.conn) {
		return -EBUSY;
	}

	link.conn = bt_conn_ref(conn);
	k_work_reschedule(&link.prot_timer, bt_mesh_prov_protocol_timeout_get());

	return 0;
}

int bt_mesh_pb_gatt_cli_open(struct bt_conn *conn)
{
	LOG_DBG("conn %p", (void *)conn);

	if (link.conn != conn) {
		LOG_DBG("Not connected");
		return -ENOTCONN;
	}

	link.cb->link_opened(&bt_mesh_pb_gatt, link.cb_data);

	return 0;
}

static int prov_link_open(const uint8_t uuid[16], uint8_t timeout,
			  const struct prov_bearer_cb *cb, void *cb_data)
{
	LOG_DBG("uuid %s", bt_hex(uuid, 16));

	link.cb = cb;
	link.cb_data = cb_data;

	k_work_reschedule(&link.prot_timer, K_SECONDS(timeout));

	return bt_mesh_pb_gatt_cli_setup(uuid);
}

static void prov_link_close(enum prov_bearer_link_status status)
{
	(void)bt_conn_disconnect(link.conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
}
#endif

#if defined(CONFIG_BT_MESH_PB_GATT)
static int link_accept(const struct prov_bearer_cb *cb, void *cb_data)
{
	int err;

	err = bt_mesh_adv_enable();
	if (err) {
		LOG_ERR("Failed enabling advertiser");
		return err;
	}

	(void)bt_mesh_pb_gatt_srv_enable();
	bt_mesh_adv_gatt_update();

	link.cb = cb;
	link.cb_data = cb_data;

	return 0;
}
#endif

static void buf_send_end(struct bt_conn *conn, void *user_data)
{
	if (link.comp.cb) {
		link.comp.cb(0, link.comp.cb_data);
	}
}

static int buf_send(struct net_buf_simple *buf, prov_bearer_send_complete_t cb,
		    void *cb_data)
{
	if (!link.conn) {
		return -ENOTCONN;
	}

	link.comp.cb = cb;
	link.comp.cb_data = cb_data;

	k_work_reschedule(&link.prot_timer, bt_mesh_prov_protocol_timeout_get());

	return bt_mesh_proxy_msg_send(link.conn, BT_MESH_PROXY_PROV,
				      buf, buf_send_end, NULL);
}

static void clear_tx(void)
{
	/* No action */
}

void bt_mesh_pb_gatt_init(void)
{
	k_work_init_delayable(&link.prot_timer, protocol_timeout);
}

void bt_mesh_pb_gatt_reset(void)
{
	reset_state();
}

const struct prov_bearer bt_mesh_pb_gatt = {
	.type = BT_MESH_PROV_GATT,
#if defined(CONFIG_BT_MESH_PB_GATT_CLIENT)
	.link_open = prov_link_open,
	.link_close = prov_link_close,
#endif
#if defined(CONFIG_BT_MESH_PB_GATT)
	.link_accept = link_accept,
#endif
	.send = buf_send,
	.clear_tx = clear_tx,
};
