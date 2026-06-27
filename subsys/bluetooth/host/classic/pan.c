/* pan.c - Bluetooth Personal Area Networking Profile */

/*
 * Copyright (c) 2025
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stddef.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/classic/sdp.h>
#include <zephyr/bluetooth/classic/pan.h>

#if defined(CONFIG_BT_PAN_NET)
#include <zephyr/net/net_if.h>
#endif

#include "host/conn_internal.h"
#include "bnep_internal.h"
#include "pan_internal.h"

#define LOG_LEVEL CONFIG_BT_PAN_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_pan);

#define PAN_VERSION BT_PAN_VERSION

static enum bt_pan_role pan_role;
static const struct bt_pan_cb *pan_cb;

static struct bt_pan connections[CONFIG_BT_MAX_CONN];

static struct bt_pan *pan_from_bnep(struct bt_bnep *bnep)
{
	return (struct bt_pan *)((char *)bnep - offsetof(struct bt_pan, bnep));
}

static struct bt_sdp_attribute panu_attrs[] = {
	BT_SDP_NEW_SERVICE,
	BT_SDP_LIST(
		BT_SDP_ATTR_SVCLASS_ID_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
			BT_SDP_ARRAY_16(BT_SDP_PANU_SVCLASS)
		},
		)
	),
	BT_SDP_LIST(
		BT_SDP_ATTR_PROTO_DESC_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 16),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
				BT_SDP_ARRAY_16(BT_SDP_PROTO_L2CAP)
			},
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
				BT_SDP_ARRAY_16(BT_UUID_BNEP_VAL)
			},
			)
		},
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
				BT_SDP_ARRAY_16(BT_SDP_PROTO_BNEP)
			},
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
				BT_SDP_ARRAY_16(PAN_VERSION)
			},
			)
		},
		)
	),
	BT_SDP_LIST(
		BT_SDP_ATTR_PROFILE_DESC_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 8),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
				BT_SDP_ARRAY_16(BT_SDP_GENERIC_NETWORKING_SVCLASS)
			},
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
				BT_SDP_ARRAY_16(PAN_VERSION)
			},
			)
		},
		)
	),
	BT_SDP_SERVICE_NAME("PANU"),
};

static struct bt_sdp_attribute nap_attrs[] = {
	BT_SDP_NEW_SERVICE,
	BT_SDP_LIST(
		BT_SDP_ATTR_SVCLASS_ID_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
			BT_SDP_ARRAY_16(BT_SDP_NAP_SVCLASS)
		},
		)
	),
	BT_SDP_LIST(
		BT_SDP_ATTR_PROTO_DESC_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 16),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
				BT_SDP_ARRAY_16(BT_SDP_PROTO_L2CAP)
			},
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
				BT_SDP_ARRAY_16(BT_UUID_BNEP_VAL)
			},
			)
		},
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
				BT_SDP_ARRAY_16(BT_SDP_PROTO_BNEP)
			},
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
				BT_SDP_ARRAY_16(PAN_VERSION)
			},
			)
		},
		)
	),
	BT_SDP_LIST(
		BT_SDP_ATTR_PROFILE_DESC_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 8),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
				BT_SDP_ARRAY_16(BT_SDP_GENERIC_NETWORKING_SVCLASS)
			},
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
				BT_SDP_ARRAY_16(PAN_VERSION)
			},
			)
		},
		)
	),
	{
		BT_SDP_ATTR_NET_ACCESS_TYPE,
		{ BT_SDP_TYPE_SIZE(BT_SDP_UINT16), BT_SDP_ARRAY_16(0x0000) }
	},
	{
		BT_SDP_ATTR_MAX_NET_ACCESSRATE,
		{ BT_SDP_TYPE_SIZE(BT_SDP_UINT32), BT_SDP_ARRAY_32(0x00000000) }
	},
	BT_SDP_SERVICE_NAME("NAP"),
};

static struct bt_sdp_record panu_rec = BT_SDP_RECORD(panu_attrs);
static struct bt_sdp_record nap_rec = BT_SDP_RECORD(nap_attrs);

static struct bt_pan *pan_get_session(struct bt_conn *conn)
{
	size_t index = bt_conn_index(conn);

	if (index >= ARRAY_SIZE(connections)) {
		return NULL;
	}

	return &connections[index];
}

static uint8_t pan_local_service(enum bt_pan_role role)
{
	return role == BT_PAN_ROLE_NAP ? BNEP_SVC_NAP : BNEP_SVC_PANU;
}

static void pan_bnep_connected(struct bt_bnep *bnep)
{
	struct bt_pan *pan = pan_from_bnep(bnep);

	LOG_DBG("PAN connected");

#if defined(CONFIG_BT_PAN_NET)
	if (pan->iface != NULL) {
		net_if_carrier_on(pan->iface);
	}
#endif

	if (pan->cb && pan->cb->connected) {
		pan->cb->connected(pan);
	}
}

static void pan_bnep_disconnected(struct bt_bnep *bnep)
{
	struct bt_pan *pan = pan_from_bnep(bnep);

	LOG_DBG("PAN disconnected");

#if defined(CONFIG_BT_PAN_NET)
	if (pan->iface != NULL) {
		net_if_carrier_off(pan->iface);
	}
#endif

	if (pan->cb && pan->cb->disconnected) {
		pan->cb->disconnected(pan);
	}

	memset(pan, 0, sizeof(*pan));
}

#if defined(CONFIG_BT_PAN_NET)
void bt_pan_net_rx(struct bt_pan *pan, struct net_buf *buf);
#endif

static void pan_bnep_recv(struct bt_bnep *bnep, struct net_buf *buf)
{
	struct bt_pan *pan = pan_from_bnep(bnep);

#if defined(CONFIG_BT_PAN_NET)
	if (pan->iface != NULL) {
		bt_pan_net_rx(pan, buf);
		return;
	}
#endif

	if (pan->cb && pan->cb->recv) {
		pan->cb->recv(pan, buf);
	}
}

static const struct bt_bnep_cb bnep_pan_cb = {
	.connected = pan_bnep_connected,
	.disconnected = pan_bnep_disconnected,
	.recv = pan_bnep_recv,
};

static int pan_bnep_accept(struct bt_conn *conn, struct bt_bnep **bnep)
{
	struct bt_pan *pan;
	int err;

	if (pan_cb == NULL) {
		return -EINVAL;
	}

	pan = pan_get_session(conn);
	if (pan == NULL) {
		return -ENOMEM;
	}

	if (pan_cb->accept != NULL) {
		err = pan_cb->accept(conn, &pan);
		if (err != 0) {
			return err;
		}
	} else {
		if (pan_role != BT_PAN_ROLE_NAP) {
			return -ECONNREFUSED;
		}
	}

	err = bt_bnep_accept(conn, &pan->bnep, pan_local_service(pan_role));
	if (err != 0) {
		return err;
	}

	pan->role = pan_role;
	pan->cb = pan_cb;
	*bnep = &pan->bnep;

	return 0;
}

static void pan_init(void)
{
	static bool initialized;

	if (initialized) {
		return;
	}

	bt_bnep_init();
	bt_bnep_register_cb(&bnep_pan_cb);
	bt_bnep_register_accept(pan_bnep_accept);

	if (pan_role == BT_PAN_ROLE_NAP) {
		bt_sdp_register_service(&nap_rec);
	} else {
		bt_sdp_register_service(&panu_rec);
	}

	initialized = true;
}

int bt_pan_register(enum bt_pan_role role, const struct bt_pan_cb *cb)
{
	if (cb == NULL) {
		return -EINVAL;
	}

	if (role != BT_PAN_ROLE_PANU && role != BT_PAN_ROLE_NAP) {
		return -EINVAL;
	}

	pan_role = role;
	pan_cb = cb;
	pan_init();

	return 0;
}

struct bt_pan *bt_pan_lookup(struct bt_conn *conn)
{
	return pan_get_session(conn);
}

struct bt_pan *bt_pan_get(struct bt_conn *conn)
{
	struct bt_pan *pan = pan_get_session(conn);

	if (pan == NULL || pan->bnep.chan.chan.conn == NULL) {
		return NULL;
	}

	return pan;
}

int bt_pan_connect(struct bt_conn *conn, struct bt_pan *pan)
{
	int err;

	if (conn == NULL || pan == NULL || pan_role != BT_PAN_ROLE_PANU) {
		return -EINVAL;
	}

	pan->role = BT_PAN_ROLE_PANU;
	pan->cb = pan_cb;

	err = bt_bnep_connect(conn, &pan->bnep, BNEP_SVC_PANU, BNEP_SVC_NAP);
	if (err != 0) {
		return err;
	}

	return 0;
}

int bt_pan_disconnect(struct bt_pan *pan)
{
	if (pan == NULL) {
		return -EINVAL;
	}

	return bt_bnep_disconnect(&pan->bnep);
}

int bt_pan_send(struct bt_pan *pan, struct net_buf *buf)
{
	if (pan == NULL) {
		return -EINVAL;
	}

	return bt_bnep_send(&pan->bnep, buf);
}

struct net_buf *bt_pan_alloc_buf(size_t len)
{
	return bt_bnep_alloc_buf(len);
}

enum bt_pan_state bt_pan_get_state(const struct bt_pan *pan)
{
	if (pan == NULL) {
		return BT_PAN_STATE_DISCONNECTED;
	}

	switch (pan->bnep.state) {
	case BT_BNEP_STATE_CONNECTING:
		return BT_PAN_STATE_CONNECTING;
	case BT_BNEP_STATE_CONNECTED:
		return BT_PAN_STATE_CONNECTED;
	default:
		return BT_PAN_STATE_DISCONNECTED;
	}
}

enum bt_pan_role bt_pan_get_role(const struct bt_pan *pan)
{
	if (pan == NULL) {
		return pan_role;
	}

	return pan->role;
}

struct bt_conn *bt_pan_get_conn(const struct bt_pan *pan)
{
	if (pan == NULL) {
		return NULL;
	}

	return pan->bnep.conn;
}
