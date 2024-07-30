/*
 * Copyright (c) 2021 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/net/buf.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/mesh.h>

#include "mesh.h"
#include "net.h"
#include "rpl.h"
#include "transport.h"
#include "prov.h"
#include "beacon.h"
#include "foundation.h"
#include "access.h"
#include "proxy.h"
#include "gatt_cli.h"
#include "proxy_msg.h"
#include "crypto.h"

#define LOG_LEVEL CONFIG_BT_MESH_PROXY_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_mesh_proxy_client);

static struct bt_mesh_proxy_server {
	struct bt_mesh_proxy_role *role;
	bool link_opened;
	uint16_t net_idx;
} servers[CONFIG_BT_MAX_CONN] = {
	[0 ... (CONFIG_BT_MAX_CONN - 1)] = {
		.net_idx = BT_MESH_KEY_UNUSED,
	},
};

static bool allow_all_subnet;

static struct bt_mesh_proxy_server *find_proxy_srv(uint16_t net_idx,
						   bool conn, bool disconn)
{
	for (int i = 0; i < ARRAY_SIZE(servers); i++) {
		if (!servers[i].role) {
			if (!disconn) {
				continue;
			}
		} else if (!conn) {
			continue;
		}

		if (servers[i].net_idx == net_idx) {
			return &servers[i];
		}
	}

	return NULL;
}

static struct bt_mesh_proxy_server *find_proxy_srv_by_conn(struct bt_conn *conn)
{
	for (int i = 0; i < ARRAY_SIZE(servers); i++) {
		if (!servers[i].role ||
		    servers[i].role->conn != conn) {
			continue;
		}

		return &servers[i];
	}

	return NULL;
}

bool bt_mesh_proxy_cli_relay(struct bt_mesh_adv *adv)
{
	bool relayed = false;
	int i;

	for (i = 0; i < ARRAY_SIZE(servers); i++) {
		struct bt_mesh_proxy_server *server = &servers[i];

		if (!server->link_opened) {
			continue;
		}

		if (bt_mesh_proxy_relay_send(server->role->conn, adv)) {
			continue;
		}

		relayed = true;
	}

	return relayed;
}

static void proxy_msg_recv(struct bt_mesh_proxy_role *role)
{
	switch (role->msg_type) {
	case BT_MESH_PROXY_NET_PDU:
		LOG_DBG("Mesh Network PDU");
		bt_mesh_net_recv(&role->buf, 0, BT_MESH_NET_IF_PROXY);
		break;
	case BT_MESH_PROXY_BEACON:
		LOG_DBG("Mesh Beacon PDU");
		bt_mesh_beacon_recv(&role->buf);
		break;
	case BT_MESH_PROXY_CONFIG:
		LOG_DBG("Mesh Configuration PDU");
		/* TODO */
		break;
	default:
		LOG_WRN("Unhandled Message Type 0x%02x", role->msg_type);
		break;
	}
}

static void proxy_connected(struct bt_conn *conn, void *user_data)
{
	struct bt_mesh_proxy_server *srv = user_data;

	srv->role = bt_mesh_proxy_role_setup(conn, bt_mesh_gatt_send,
					     proxy_msg_recv);
}

static void proxy_link_open(struct bt_conn *conn)
{
	struct bt_mesh_proxy_server *srv = find_proxy_srv_by_conn(conn);

	srv->link_opened = true;
}

static void proxy_disconnected(struct bt_conn *conn)
{
	struct bt_mesh_proxy_server *srv = find_proxy_srv_by_conn(conn);

	bt_mesh_proxy_role_cleanup(srv->role);

	srv->role = NULL;
	srv->link_opened = false;
}

static const struct bt_mesh_gatt_cli proxy = {
	.srv_uuid		= BT_UUID_INIT_16(BT_UUID_MESH_PROXY_VAL),
	.data_in_uuid		= BT_UUID_INIT_16(BT_UUID_MESH_PROXY_DATA_IN_VAL),
	.data_out_uuid		= BT_UUID_INIT_16(BT_UUID_MESH_PROXY_DATA_OUT_VAL),
	.data_out_cccd_uuid	= BT_UUID_INIT_16(BT_UUID_GATT_CCC_VAL),

	.connected		= proxy_connected,
	.link_open		= proxy_link_open,
	.disconnected		= proxy_disconnected
};

static bool proxy_srv_check_and_get(struct bt_mesh_subnet *sub, const uint8_t *net_id,
				    struct bt_mesh_proxy_server **p_srv)
{
	struct bt_mesh_proxy_server *srv;

	srv = find_proxy_srv(sub->net_idx, true, true);
	if (srv) {
		if (srv->role) {
			return true;
		}
	} else if (!allow_all_subnet) {
		return false;
	}

	if (!srv) {
		srv = find_proxy_srv(BT_MESH_KEY_UNUSED, false, true);
		if (!srv) {
			return true;
		}
	}

	/* If net_id is NULL we already know that the networks match */
	if (!net_id || !memcmp(sub->keys[0].net_id, net_id, 8) ||
	    (bt_mesh_subnet_has_new_key(sub) && !memcmp(sub->keys[1].net_id, net_id, 8))) {

		*p_srv = srv;
		return true;
	}

	return false;
}

struct find_net_id {
	uint8_t type;

	union {
		const uint8_t *net_id;
		struct {
			const uint8_t *hash;
			const uint8_t *rand;
		} priv;
	} data;

	struct bt_mesh_proxy_server *srv;
};

static bool is_hash_equal(struct bt_mesh_subnet *sub, struct find_net_id *res, uint8_t idx)
{
	int err;
	uint8_t in[16], out[16];

	memcpy(&in[0], sub->keys[idx].net_id, 8);
	memcpy(&in[8], res->data.priv.rand, 8);
	err = bt_mesh_encrypt(&sub->keys[idx].identity, in, out);
	if (err) {
		LOG_ERR("Failed to generate hash (err: %d)", err);
		return false;
	}

	if (memcmp(&out[8], res->data.priv.hash, 8)) {
		return false;
	}

	return true;
}

static bool has_net_id(struct bt_mesh_subnet *sub, void *user_data)
{
	struct find_net_id *res = user_data;
	uint8_t *net_id = NULL;

	if (res->type == BT_MESH_ID_TYPE_NET) {
		net_id = (uint8_t *)res->data.net_id;
		goto end;
	}

	/* Additional handling for BT_MESH_ID_TYPE_PRIV_NET msg type */
	if (!(is_hash_equal(sub, res, 0) ||
	      (bt_mesh_subnet_has_new_key(sub) && is_hash_equal(sub, res, 1)))) {
		return false;
	}
end:
	return proxy_srv_check_and_get(sub, net_id, &res->srv);
}

static void handle_net_id(uint8_t type, const struct bt_le_scan_recv_info *info,
			  struct net_buf_simple *buf)
{
	int err;
	struct find_net_id res;
	struct bt_mesh_subnet *sub;

	res.type = type;
	res.srv = NULL;

	if (type == BT_MESH_ID_TYPE_NET) {
		if (buf->len != 8) {
			return;
		}
		res.data.net_id = net_buf_simple_pull_mem(buf, 8);

	} else {
		if (buf->len != 16) {
			return;
		}

		res.data.priv.hash = net_buf_simple_pull_mem(buf, 8);
		res.data.priv.rand = net_buf_simple_pull_mem(buf, 8);
	}

	sub = bt_mesh_subnet_find(has_net_id, (void *)&res);
	if (sub && res.srv) {
		err = bt_mesh_gatt_cli_connect(info->addr, &proxy, res.srv);
		if (err) {
			LOG_DBG("Failed to connect over GATT (err:%d)", err);
		}
	}
}

void bt_mesh_proxy_cli_adv_recv(const struct bt_le_scan_recv_info *info,
				struct net_buf_simple *buf)
{
	uint8_t type;

	type = net_buf_simple_pull_u8(buf);
	switch (type) {
	case BT_MESH_ID_TYPE_NET:
		/* Fallthrough */
	case BT_MESH_ID_TYPE_PRIV_NET: {
		handle_net_id(type, info, buf);
		break;
	}
	case BT_MESH_ID_TYPE_NODE: {
		/* TODO */
		break;
	}
	case BT_MESH_ID_TYPE_PRIV_NODE: {
		/* TODO */
		break;
	}
	default:
		return;
	}
}

int bt_mesh_proxy_connect(uint16_t net_idx)
{
	struct bt_mesh_proxy_server *srv;

	if (net_idx == BT_MESH_KEY_ANY) {
		if (allow_all_subnet) {
			return -EALREADY;
		}

		allow_all_subnet = true;

		return 0;
	}

	srv = find_proxy_srv(net_idx, true, true);
	if (srv) {
		return -EALREADY;
	}

	srv = find_proxy_srv(BT_MESH_KEY_UNUSED, false, true);
	if (!srv) {
		return -ENOMEM;
	}

	srv->net_idx = net_idx;

	return 0;
}

int bt_mesh_proxy_disconnect(uint16_t net_idx)
{
	int err;
	struct bt_mesh_proxy_server *srv;

	if (net_idx != BT_MESH_KEY_ANY) {
		srv = find_proxy_srv(net_idx, true, true);
		if (!srv) {
			return -EALREADY;
		}

		srv->net_idx = BT_MESH_KEY_UNUSED;

		if (!srv->role) {
			return 0;
		}

		return bt_conn_disconnect(srv->role->conn,
					  BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	}

	if (!allow_all_subnet) {
		return -EALREADY;
	}

	allow_all_subnet = false;

	for (int i = 0; i < ARRAY_SIZE(servers); i++) {
		servers[i].net_idx = BT_MESH_KEY_UNUSED;

		if (!servers[i].role) {
			continue;
		}

		err = bt_conn_disconnect(servers[i].role->conn,
					 BT_HCI_ERR_REMOTE_USER_TERM_CONN);
		if (err) {
			return err;
		}
	}

	return 0;
}

static void subnet_evt(struct bt_mesh_subnet *sub, enum bt_mesh_key_evt evt)
{
	switch (evt) {
	case BT_MESH_KEY_DELETED:
		(void)bt_mesh_proxy_disconnect(sub->net_idx);
		break;

	default:
		break;
	}
}

BT_MESH_SUBNET_CB_DEFINE(proxy_cli) = {
	.evt_handler = subnet_evt,
};

bool bt_mesh_proxy_cli_is_connected(uint16_t net_idx)
{
	if (find_proxy_srv(net_idx, true, false)) {
		return true;
	}

	return false;
}
