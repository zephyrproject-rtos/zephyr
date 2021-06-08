/*
 * Copyright (c) 2017 Intel Corporation
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <bluetooth/mesh.h>
#include <bluetooth/conn.h>
#include "net.h"
#include "proxy.h"
#include "adv.h"
#include "prov.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_MESH_DEBUG_PROV)
#define LOG_MODULE_NAME bt_mesh_pb_gatt
#include "common/log.h"

struct prov_bearer_send_cb {
	prov_bearer_send_complete_t cb;
	void *cb_data;
};

struct prov_link {
	struct bt_conn *conn;
	const struct prov_bearer_cb *cb;
	void *cb_data;
	struct prov_bearer_send_cb comp;
	struct net_buf_simple *rx_buf;
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

	link.rx_buf = bt_mesh_proxy_get_buf();
}

static void link_closed(enum prov_bearer_link_status status)
{
	const struct prov_bearer_cb *cb = link.cb;
	void *cb_data = link.cb_data;

	reset_state();

	cb->link_closed(&pb_gatt, cb_data, status);
}

static void protocol_timeout(struct k_work *work)
{
	if (!link.conn) {
		/* Already disconnected */
		return;
	}

	BT_DBG("Protocol timeout");
	link_closed(PROV_BEARER_LINK_STATUS_TIMEOUT);
}

int bt_mesh_pb_gatt_recv(struct bt_conn *conn, struct net_buf_simple *buf)
{
	BT_DBG("%u bytes: %s", buf->len, bt_hex(buf->data, buf->len));

	if (link.conn != conn || !link.cb) {
		BT_WARN("Data for unexpected connection");
		return -ENOTCONN;
	}

	if (buf->len < 1) {
		BT_WARN("Too short provisioning packet (len %u)", buf->len);
		return -EINVAL;
	}

	k_work_reschedule(&link.prot_timer, PROTOCOL_TIMEOUT);

	link.cb->recv(&pb_gatt, link.cb_data, buf);

	return 0;
}

int bt_mesh_pb_gatt_open(struct bt_conn *conn)
{
	BT_DBG("conn %p", (void *)conn);

	if (link.conn) {
		return -EBUSY;
	}

	link.conn = bt_conn_ref(conn);
	k_work_reschedule(&link.prot_timer, PROTOCOL_TIMEOUT);

	link.cb->link_opened(&pb_gatt, link.cb_data);

	return 0;
}

int bt_mesh_pb_gatt_close(struct bt_conn *conn)
{
	BT_DBG("conn %p", (void *)conn);

	if (link.conn != conn) {
		BT_DBG("Not connected");
		return -ENOTCONN;
	}

	link_closed(PROV_BEARER_LINK_STATUS_SUCCESS);

	return 0;
}

static int link_accept(const struct prov_bearer_cb *cb, void *cb_data)
{
	(void)bt_mesh_proxy_prov_enable();
	bt_mesh_adv_update();

	link.cb = cb;
	link.cb_data = cb_data;

	return 0;
}

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

	k_work_reschedule(&link.prot_timer, PROTOCOL_TIMEOUT);

	return bt_mesh_pb_gatt_send(link.conn, buf, buf_send_end, NULL);
}

static void clear_tx(void)
{
	/* No action */
}

void pb_gatt_init(void)
{
	k_work_init_delayable(&link.prot_timer, protocol_timeout);
}

void pb_gatt_reset(void)
{
	reset_state();
}

const struct prov_bearer pb_gatt = {
	.type = BT_MESH_PROV_GATT,
	.link_accept = link_accept,
	.send = buf_send,
	.clear_tx = clear_tx,
};
