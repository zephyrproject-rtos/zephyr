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

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_MESH_DEBUG_PROXY)
#define LOG_MODULE_NAME bt_mesh_proxy_server
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
#include "gatt_services.h"
#include "proxy_common.h"
#include "proxy_server.h"

#define CLIENT_BUF_SIZE 68

#if defined(CONFIG_BT_MESH_GATT_PROXY)
static void proxy_send_beacons(struct k_work *work);
static void proxy_filter_recv(struct bt_conn *conn,
			      struct bt_mesh_net_rx *rx, struct net_buf_simple *buf);
#endif

static int proxy_send(struct bt_conn *conn, const void *data,
		      uint16_t len);

static struct bt_mesh_proxy_client {
	struct bt_mesh_proxy_object object;
	uint16_t filter[CONFIG_BT_MESH_PROXY_FILTER_SIZE];
	enum __packed {
		NONE,
		WHITELIST,
		BLACKLIST,
		PROV,
	} filter_type;
#if defined(CONFIG_BT_MESH_GATT_PROXY)
	struct k_work send_beacons;
#endif
} clients[CONFIG_BT_MAX_CONN] = {
	[0 ... (CONFIG_BT_MAX_CONN - 1)] = {
#if defined(CONFIG_BT_MESH_GATT_PROXY)
		.send_beacons = Z_WORK_INITIALIZER(proxy_send_beacons),
#endif
		.object.cb = {
			.send_cb = proxy_send,
#if defined(CONFIG_BT_MESH_GATT_PROXY)
			.recv_cb = proxy_filter_recv,
#endif
		}
	},
};

static sys_slist_t idle_waiters;
static atomic_t pending_notifications;
static uint8_t __noinit client_buf_data[CLIENT_BUF_SIZE * CONFIG_BT_MAX_CONN];

static struct bt_mesh_proxy_client *find_client(struct bt_conn *conn)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(clients); i++) {
		if (clients[i].object.conn == conn) {
			return &clients[i];
		}
	}

	return NULL;
}

#if defined(CONFIG_BT_MESH_GATT_PROXY)
static int filter_set(struct bt_mesh_proxy_client *client,
		      struct net_buf_simple *buf)
{
	uint8_t type;

	if (buf->len < 1) {
		BT_WARN("Too short Filter Set message");
		return -EINVAL;
	}

	type = net_buf_simple_pull_u8(buf);
	BT_DBG("type 0x%02x", type);

	switch (type) {
	case 0x00:
		(void)memset(client->filter, 0, sizeof(client->filter));
		client->filter_type = WHITELIST;
		break;
	case 0x01:
		(void)memset(client->filter, 0, sizeof(client->filter));
		client->filter_type = BLACKLIST;
		break;
	default:
		BT_WARN("Prohibited Filter Type 0x%02x", type);
		return -EINVAL;
	}

	return 0;
}

static void filter_add(struct bt_mesh_proxy_client *client, uint16_t addr)
{
	int i;

	BT_DBG("addr 0x%04x", addr);

	if (addr == BT_MESH_ADDR_UNASSIGNED) {
		return;
	}

	for (i = 0; i < ARRAY_SIZE(client->filter); i++) {
		if (client->filter[i] == addr) {
			return;
		}
	}

	for (i = 0; i < ARRAY_SIZE(client->filter); i++) {
		if (client->filter[i] == BT_MESH_ADDR_UNASSIGNED) {
			client->filter[i] = addr;
			return;
		}
	}
}

static void filter_remove(struct bt_mesh_proxy_client *client, uint16_t addr)
{
	int i;

	BT_DBG("addr 0x%04x", addr);

	if (addr == BT_MESH_ADDR_UNASSIGNED) {
		return;
	}

	for (i = 0; i < ARRAY_SIZE(client->filter); i++) {
		if (client->filter[i] == addr) {
			client->filter[i] = BT_MESH_ADDR_UNASSIGNED;
			return;
		}
	}
}

static void send_filter_status(struct bt_mesh_proxy_client *client,
			       struct bt_mesh_net_rx *rx,
			       struct net_buf_simple *buf)
{
	struct bt_mesh_net_tx tx = {
		.sub = rx->sub,
		.ctx = &rx->ctx,
		.src = bt_mesh_primary_addr(),
	};
	uint16_t filter_size;
	int i, err;

	/* Configuration messages always have dst unassigned */
	tx.ctx->addr = BT_MESH_ADDR_UNASSIGNED;

	net_buf_simple_reset(buf);
	net_buf_simple_reserve(buf, 10);

	net_buf_simple_add_u8(buf, CFG_FILTER_STATUS);

	if (client->filter_type == WHITELIST) {
		net_buf_simple_add_u8(buf, 0x00);
	} else {
		net_buf_simple_add_u8(buf, 0x01);
	}

	for (filter_size = 0U, i = 0; i < ARRAY_SIZE(client->filter); i++) {
		if (client->filter[i] != BT_MESH_ADDR_UNASSIGNED) {
			filter_size++;
		}
	}

	net_buf_simple_add_be16(buf, filter_size);

	BT_DBG("%u bytes: %s", buf->len, bt_hex(buf->data, buf->len));

	err = bt_mesh_net_encode(&tx, buf, true);
	if (err) {
		BT_ERR("Encoding Proxy cfg message failed (err %d)", err);
		return;
	}

	err = bt_mesh_proxy_common_send(&client->object,
					BT_MESH_PROXY_CONFIG, buf);
	if (err) {
		BT_ERR("Failed to send proxy cfg message (err %d)", err);
	}
}

static void proxy_filter_recv(struct bt_conn *conn,
			      struct bt_mesh_net_rx *rx, struct net_buf_simple *buf)
{
	struct bt_mesh_proxy_client *client;
	uint8_t opcode;

	client = find_client(conn);
	if (!client) {
		return;
	}

	opcode = net_buf_simple_pull_u8(buf);
	switch (opcode) {
	case CFG_FILTER_SET:
		filter_set(client, buf);
		send_filter_status(client, rx, buf);
		break;
	case CFG_FILTER_ADD:
		while (buf->len >= 2) {
			uint16_t addr;

			addr = net_buf_simple_pull_be16(buf);
			filter_add(client, addr);
		}
		send_filter_status(client, rx, buf);
		break;
	case CFG_FILTER_REMOVE:
		while (buf->len >= 2) {
			uint16_t addr;

			addr = net_buf_simple_pull_be16(buf);
			filter_remove(client, addr);
		}
		send_filter_status(client, rx, buf);
		break;
	default:
		BT_WARN("Unhandled configuration OpCode 0x%02x", opcode);
		break;
	}
}

static int beacon_send(struct bt_mesh_proxy_client *client,
		       struct bt_mesh_subnet *sub)
{
	NET_BUF_SIMPLE_DEFINE(buf, 23);

	net_buf_simple_reserve(&buf, 1);
	bt_mesh_beacon_create(sub, &buf);

	return bt_mesh_proxy_common_send(&client->object, BT_MESH_PROXY_BEACON, &buf);
}

static void proxy_send_beacons(struct k_work *work)
{
	struct bt_mesh_proxy_client *client;
	int i;

	client = CONTAINER_OF(work, struct bt_mesh_proxy_client, send_beacons);

	for (i = 0; i < ARRAY_SIZE(bt_mesh.sub); i++) {
		struct bt_mesh_subnet *sub = &bt_mesh.sub[i];

		if (sub->net_idx != BT_MESH_KEY_UNUSED) {
			beacon_send(client, sub);
		}
	}
}

void bt_mesh_proxy_beacon_send(struct bt_mesh_subnet *sub)
{
	int i;

	if (!sub) {
		/* NULL means we send on all subnets */
		for (i = 0; i < ARRAY_SIZE(bt_mesh.sub); i++) {
			if (bt_mesh.sub[i].net_idx != BT_MESH_KEY_UNUSED) {
				bt_mesh_proxy_beacon_send(&bt_mesh.sub[i]);
			}
		}

		return;
	}

	for (i = 0; i < ARRAY_SIZE(clients); i++) {
		if (clients[i].object.conn) {
			beacon_send(&clients[i], sub);
		}
	}
}
#endif /* GATT_PROXY */

ssize_t bt_mesh_proxy_recv(struct bt_conn *conn,
			  const void *buf, uint16_t len)
{
	struct bt_mesh_proxy_client *client = find_client(conn);

	if (!client) {
		return -ENOTCONN;
	}

	return bt_mesh_proxy_common_recv(&client->object, buf, len);
}

void bt_mesh_proxy_connected(struct bt_conn *conn, uint8_t err)
{
	struct bt_mesh_proxy_client *client;
	int i;

	for (client = NULL, i = 0; i < ARRAY_SIZE(clients); i++) {
		if (!clients[i].object.conn) {
			client = &clients[i];
			break;
		}
	}

	if (!client) {
		BT_ERR("No free Proxy Client objects");
		return;
	}

	client->object.conn = bt_conn_ref(conn);
	client->filter_type = NONE;
	(void)memset(client->filter, 0, sizeof(client->filter));
	net_buf_simple_reset(&client->object.buf);
}

void bt_mesh_proxy_disconnected(struct bt_conn *conn, uint8_t reason)
{
	struct bt_conn_info info;
	int i;

	bt_conn_get_info(conn, &info);
	if (info.role != BT_CONN_ROLE_SLAVE) {
		return;
	}

	for (i = 0; i < ARRAY_SIZE(clients); i++) {
		struct bt_mesh_proxy_client *client = &clients[i];

		if (client->object.conn == conn) {
			if (IS_ENABLED(CONFIG_BT_MESH_PB_GATT) &&
			    client->filter_type == PROV) {
				bt_mesh_pb_gatt_close(conn);
			}

			k_delayed_work_cancel(&client->object.sar_timer);
			bt_conn_unref(client->object.conn);
			client->object.conn = NULL;
			break;
		}
	}
}

struct net_buf_simple *bt_mesh_proxy_get_buf(void)
{
	struct net_buf_simple *buf = &clients[0].object.buf;

	net_buf_simple_reset(buf);

	return buf;
}

#if defined(CONFIG_BT_MESH_PB_GATT)
ssize_t bt_mesh_prov_ccc_write(struct bt_conn *conn,
			      const struct bt_gatt_attr *attr, uint16_t value)
{
	struct bt_mesh_proxy_client *client;

	BT_DBG("value 0x%04x", value);

	if (value != BT_GATT_CCC_NOTIFY) {
		BT_WARN("Client wrote 0x%04x instead enabling notify", value);
		return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
	}

	/* If a connection exists there must be a client */
	client = find_client(conn);
	__ASSERT(client, "No client for connection");

	if (client->filter_type == NONE) {
		client->filter_type = PROV;
		bt_mesh_pb_gatt_open(conn);
	}

	return sizeof(value);
}

int bt_mesh_proxy_prov_enable(void)
{
	int i, err;

	BT_DBG("");

	err = bt_mesh_gatt_prov_enable();
	if (err) {
		return err;
	}

	for (i = 0; i < ARRAY_SIZE(clients); i++) {
		if (clients[i].object.conn) {
			clients[i].filter_type = PROV;
		}
	}

	return 0;
}

int bt_mesh_proxy_prov_disable(bool disconnect)
{
	int i, err;

	BT_DBG("");

	err = bt_mesh_gatt_prov_disable();
	if (err) {
		return err;
	}

	for (i = 0; i < ARRAY_SIZE(clients); i++) {
		struct bt_mesh_proxy_client *client = &clients[i];

		if (!client->object.conn || client->filter_type != PROV) {
			continue;
		}

		if (disconnect) {
			bt_conn_disconnect(client->object.conn,
					   BT_HCI_ERR_REMOTE_USER_TERM_CONN);
		} else {
			bt_mesh_pb_gatt_close(client->object.conn);
			client->filter_type = NONE;
		}
	}

	bt_mesh_adv_update();

	return 0;
}
#endif /* CONFIG_BT_MESH_PB_GATT */

#if defined(CONFIG_BT_MESH_GATT_PROXY)
ssize_t bt_mesh_proxy_ccc_write(struct bt_conn *conn,
			      const struct bt_gatt_attr *attr, uint16_t value)
{
	struct bt_mesh_proxy_client *client;

	BT_DBG("value: 0x%04x", value);

	if (value != BT_GATT_CCC_NOTIFY) {
		BT_WARN("Client wrote 0x%04x instead enabling notify", value);
		return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
	}

	/* If a connection exists there must be a client */
	client = find_client(conn);
	__ASSERT(client, "No client for connection");

	if (client->filter_type == NONE) {
		client->filter_type = WHITELIST;
		k_work_submit(&client->send_beacons);
	}

	return sizeof(value);
}

int bt_mesh_proxy_gatt_enable(void)
{
	int i, err;

	BT_DBG("");

	err = bt_mesh_gatt_proxy_enable();
	if (err) {
		return err;
	}

	for (i = 0; i < ARRAY_SIZE(clients); i++) {
		if (clients[i].object.conn) {
			clients[i].filter_type = WHITELIST;
		}
	}

	return 0;
}

void bt_mesh_proxy_gatt_disconnect(void)
{
	int i;

	BT_DBG("");

	for (i = 0; i < ARRAY_SIZE(clients); i++) {
		struct bt_mesh_proxy_client *client = &clients[i];

		if (client->object.conn && (client->filter_type == WHITELIST ||
					    client->filter_type == BLACKLIST)) {
			client->filter_type = NONE;
			bt_conn_disconnect(client->object.conn,
					   BT_HCI_ERR_REMOTE_USER_TERM_CONN);
		}
	}
}

int bt_mesh_proxy_gatt_disable(void)
{
	int err;
	BT_DBG("");

	err = bt_mesh_gatt_proxy_disable();
	if (err) {
		return err;
	}

	bt_mesh_proxy_gatt_disconnect();

	return 0;
}

void bt_mesh_proxy_addr_add(struct net_buf_simple *buf, uint16_t addr)
{
	struct bt_mesh_proxy_client *client =
		(void *)CONTAINER_OF(buf, struct bt_mesh_proxy_object, buf);

	BT_DBG("filter_type %u addr 0x%04x", client->filter_type, addr);

	if (client->filter_type == WHITELIST) {
		filter_add(client, addr);
	} else if (client->filter_type == BLACKLIST) {
		filter_remove(client, addr);
	}
}

static bool client_filter_match(struct bt_mesh_proxy_client *client,
				uint16_t addr)
{
	int i;

	BT_DBG("filter_type %u addr 0x%04x", client->filter_type, addr);

	if (client->filter_type == BLACKLIST) {
		for (i = 0; i < ARRAY_SIZE(client->filter); i++) {
			if (client->filter[i] == addr) {
				return false;
			}
		}

		return true;
	}

	if (addr == BT_MESH_ADDR_ALL_NODES) {
		return true;
	}

	if (client->filter_type == WHITELIST) {
		for (i = 0; i < ARRAY_SIZE(client->filter); i++) {
			if (client->filter[i] == addr) {
				return true;
			}
		}
	}

	return false;
}

bool bt_mesh_proxy_relay(struct net_buf_simple *buf, uint16_t dst)
{
	bool relayed = false;
	int i;

	BT_DBG("%u bytes to dst 0x%04x", buf->len, dst);

	for (i = 0; i < ARRAY_SIZE(clients); i++) {
		struct bt_mesh_proxy_client *client = &clients[i];
		NET_BUF_SIMPLE_DEFINE(msg, 32);

		if (!client->object.conn) {
			continue;
		}

		if (!client_filter_match(client, dst)) {
			continue;
		}

		/* Proxy PDU sending modifies the original buffer,
		 * so we need to make a copy.
		 */
		net_buf_simple_reserve(&msg, 1);
		net_buf_simple_add_mem(&msg, buf->data, buf->len);

		bt_mesh_proxy_send(client->object.conn, BT_MESH_PROXY_NET_PDU, &msg);
		relayed = true;
	}

	return relayed;
}

#endif /* CONFIG_BT_MESH_GATT_PROXY */

static void notify_complete(struct bt_conn *conn, void *user_data)
{
	sys_snode_t *n;

	if (atomic_dec(&pending_notifications) > 1) {
		return;
	}

	BT_DBG("");

	while ((n = sys_slist_get(&idle_waiters))) {
		CONTAINER_OF(n, struct bt_mesh_proxy_idle_cb, n)->cb();
	}
}

static int proxy_send(struct bt_conn *conn, const void *data,
		      uint16_t len)
{
	struct bt_gatt_notify_params params = {
		.data = data,
		.len = len,
		.func = notify_complete,
	};
	int err;

	BT_DBG("%u bytes: %s", len, bt_hex(data, len));

	err = bt_mesh_gatt_send(conn, &params);
	if (!err) {
		atomic_inc(&pending_notifications);
	}

	return err;
}

int bt_mesh_proxy_send(struct bt_conn *conn, uint8_t type,
		       struct net_buf_simple *msg)
{
	struct bt_mesh_proxy_client *client = find_client(conn);

	if (!client) {
		BT_ERR("No Proxy Client found");
		return -ENOTCONN;
	}

	if ((client->filter_type == PROV) != (type == BT_MESH_PROXY_PROV)) {
		BT_ERR("Invalid PDU type for Proxy Client");
		return -EINVAL;
	}

	return bt_mesh_proxy_common_send(&client->object, type, msg);
}

int bt_mesh_proxy_init(void)
{
	int i;

	/* Initialize the client receive buffers */
	for (i = 0; i < ARRAY_SIZE(clients); i++) {
		struct bt_mesh_proxy_client *client = &clients[i];

		bt_mesh_proxy_common_init(&client->object,
					  client_buf_data + (i * CLIENT_BUF_SIZE), CLIENT_BUF_SIZE);
	}

	bt_mesh_gatt_init();

	return 0;
}

void bt_mesh_proxy_on_idle(struct bt_mesh_proxy_idle_cb *cb)
{
	if (!atomic_get(&pending_notifications)) {
		cb->cb();
		return;
	}

	sys_slist_append(&idle_waiters, &cb->n);
}
