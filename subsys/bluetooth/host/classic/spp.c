/*
 * Copyright 2025 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <stdlib.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/classic/rfcomm.h>
#include <zephyr/bluetooth/classic/sdp.h>
#include <zephyr/bluetooth/classic/spp.h>

#include "host/hci_core.h"
#include "host/conn_internal.h"
#include "common/bt_str.h"

#include "l2cap_br_internal.h"
#include "rfcomm_internal.h"
#include "spp_internal.h"

#define LOG_LEVEL CONFIG_BT_SPP_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_spp);

#define SPP_RFCOMM_MTU     CONFIG_BT_L2CAP_TX_MTU
#define SDP_CLIENT_BUF_LEN 512

#define BT_SPP_RECORD(n, _) BT_SDP_RECORD(spp_attrs##n)

K_SEM_DEFINE(spp_sem_lock, 1U, 1U);

NET_BUF_POOL_FIXED_DEFINE(sdp_pool, CONFIG_BT_MAX_CONN, SDP_CLIENT_BUF_LEN, 8, NULL);

static struct bt_spp_server spp_server[CONFIG_BT_SPP_MAX_SERVER_NUM];

#define BT_SPP_VECTOR(n, _)                                                                        \
	static struct bt_sdp_attribute spp_attrs##n[] = {                                          \
		BT_SDP_NEW_SERVICE,                                                                \
		BT_SDP_LIST(                                                                       \
			BT_SDP_ATTR_SVCLASS_ID_LIST, BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3),         \
			BT_SDP_DATA_ELEM_LIST({BT_SDP_TYPE_SIZE(BT_SDP_UUID16),                    \
					       BT_SDP_ARRAY_16(BT_SDP_SERIAL_PORT_SVCLASS)},)),    \
		BT_SDP_LIST(                                                                       \
			BT_SDP_ATTR_PROTO_DESC_LIST, BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 12),        \
			BT_SDP_DATA_ELEM_LIST(                                                     \
				{BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3),                             \
				 BT_SDP_DATA_ELEM_LIST({BT_SDP_TYPE_SIZE(BT_SDP_UUID16),           \
							BT_SDP_ARRAY_16(BT_SDP_PROTO_L2CAP)},)},   \
				{BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 5),                             \
				 BT_SDP_DATA_ELEM_LIST(                                            \
					 {BT_SDP_TYPE_SIZE(BT_SDP_UUID16),                         \
					  BT_SDP_ARRAY_16(BT_SDP_PROTO_RFCOMM)},                   \
					 {BT_SDP_TYPE_SIZE(BT_SDP_UINT8),                          \
					  &spp_server[n].rfcomm_server.channel},)},)),             \
		BT_SDP_LIST(BT_SDP_ATTR_PROFILE_DESC_LIST, BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 8),   \
			    BT_SDP_DATA_ELEM_LIST(                                                 \
				    {BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),                         \
				     BT_SDP_DATA_ELEM_LIST(                                        \
					     {BT_SDP_TYPE_SIZE(BT_SDP_UUID16),                     \
					      BT_SDP_ARRAY_16(BT_SDP_SERIAL_PORT_SVCLASS)},        \
					     {BT_SDP_TYPE_SIZE(BT_SDP_UINT16),                     \
					      BT_SDP_ARRAY_16(0x0102)},)},)),                      \
		BT_SDP_SERVICE_NAME("Serial Port"),                                                \
	};

LISTIFY(CONFIG_BT_SPP_MAX_SERVER_NUM, BT_SPP_VECTOR, ())

static struct bt_sdp_record spp_rec[] = {LISTIFY(CONFIG_BT_SPP_MAX_SERVER_NUM,
	BT_SPP_RECORD, (,))};

static struct bt_spp spp_sessions[CONFIG_BT_SPP_MAX_CONNECTION_NUM];
static const struct bt_spp_cb *spp_cb;

static int spp_alloc_server(struct bt_spp_server **srv)
{
	struct bt_spp_server *server;

	if (srv == NULL) {
		return -EINVAL;
	}

	ARRAY_FOR_EACH(spp_server, i) {
		server = &spp_server[i];

		if (server->sdp_rec == NULL) {
			server->sdp_rec = &spp_rec[i];
			*srv = server;
			return 0;
		}
	}

	return -ENOMEM;
}

static int spp_alloc_connection_with_channel(struct bt_spp **spp, struct bt_conn *conn,
					     uint16_t channel)
{
	struct bt_spp *connection;

	ARRAY_FOR_EACH(spp_sessions, i) {
		connection = &spp_sessions[i];

		if (connection->acl_conn == NULL) {
			connection->acl_conn = bt_conn_ref(conn);
			connection->channel = channel;
			*spp = connection;
			return 0;
		}
	}

	LOG_ERR("No available SPP connection slots");
	return -ENOMEM;
}

static int spp_alloc_connection_with_uuid(struct bt_spp **spp, struct bt_conn *conn,
					  const struct bt_uuid *uuid)
{
	struct bt_spp *connection;

	if (conn == NULL) {
		return -EINVAL;
	}

	ARRAY_FOR_EACH(spp_sessions, i) {
		connection = &spp_sessions[i];

		if ((connection->acl_conn == conn) && !bt_uuid_cmp(connection->uuid, uuid)) {
			LOG_ERR("SPP server with UUID %s already exists", bt_uuid_str(uuid));
			return -EALREADY;
		}

		if (connection->acl_conn == NULL) {
			connection->acl_conn = bt_conn_ref(conn);
			connection->uuid = uuid;
			*spp = connection;
			return 0;
		}
	}

	LOG_ERR("No available SPP connection slots");
	return -ENOMEM;
};

static int spp_free_connection(struct bt_spp *spp)
{
	if (spp == NULL) {
		return -EINVAL;
	}

	if (spp->acl_conn) {
		bt_conn_unref(spp->acl_conn);
		spp->acl_conn = NULL;
	}

	memset(spp, 0, sizeof(*spp));
	return 0;
}

static struct bt_spp *find_spp_connection_by_uuid(struct bt_conn *conn, const struct bt_uuid *uuid)
{
	struct bt_spp *connection;

	ARRAY_FOR_EACH(spp_sessions, i) {
		connection = &spp_sessions[i];

		if (connection->acl_conn == conn && connection->uuid &&
		    !bt_uuid_cmp(connection->uuid, uuid)) {
			return connection;
		}
	}

	return NULL;
}

static void bt_spp_recv(struct bt_rfcomm_dlc *dlci, struct net_buf *buf)
{
	struct bt_spp *spp = CONTAINER_OF(dlci, struct bt_spp, rfcomm_dlc);

	if (spp_cb && spp_cb->recv) {
		spp_cb->recv(spp, buf);
	}
}

static void bt_spp_connected(struct bt_rfcomm_dlc *dlci)
{
	struct bt_spp *spp = CONTAINER_OF(dlci, struct bt_spp, rfcomm_dlc);

	LOG_DBG("SPP %p connected", dlci);

	atomic_set(&spp->state, SPP_STATE_CONNECTED);

	if (spp_cb && spp_cb->connected) {
		spp_cb->connected(spp, spp->channel);
	}
}

static void bt_spp_disconnected(struct bt_rfcomm_dlc *dlci)
{
	struct bt_spp *spp = CONTAINER_OF(dlci, struct bt_spp, rfcomm_dlc);

	LOG_DBG("SPP %p disconnected", dlci);

	atomic_set(&spp->state, SPP_STATE_DISCONNECTED);

	if (spp_cb && spp_cb->disconnected != NULL) {
		spp_cb->disconnected(spp);
	}

	spp_free_connection(spp);
}

static struct bt_rfcomm_dlc_ops spp_rfcomm_ops = {
	.recv = bt_spp_recv,
	.connected = bt_spp_connected,
	.disconnected = bt_spp_disconnected,
};

static int bt_spp_accept(struct bt_conn *conn, struct bt_rfcomm_server *server,
			 struct bt_rfcomm_dlc **dlc)
{
	struct bt_spp *spp;
	int err;

	LOG_DBG("%s conn %p", __func__, conn);

	err = spp_alloc_connection_with_channel(&spp, conn, server->channel);
	if (err < 0) {
		LOG_ERR("SPP get conn fail");
		return -ENOMEM;
	}

	spp->rfcomm_dlc.ops = &spp_rfcomm_ops;
	spp->rfcomm_dlc.mtu = SPP_RFCOMM_MTU;
	*dlc = &spp->rfcomm_dlc;

	return 0;
}

int bt_spp_register_cb(const struct bt_spp_cb *cb)
{
	if (!cb) {
		return -EINVAL;
	}

	if (spp_cb != NULL) {
		LOG_ERR("SPP callback already registered");
		return -EALREADY;
	}

	spp_cb = cb;

	return 0;
}

int bt_spp_register_srv(uint8_t channel)
{
	struct bt_spp_server *server;
	int err;

	err = spp_alloc_server(&server);
	if (err < 0) {
		LOG_ERR("SPP alloc server fail, err:%d", err);
		return err;
	}

	server->rfcomm_server.channel = channel;
	server->rfcomm_server.accept = bt_spp_accept;

	bt_rfcomm_server_register(&server->rfcomm_server);
	bt_sdp_register_service(server->sdp_rec);

	return 0;
}

int bt_spp_connect(struct bt_conn *conn, struct bt_spp **spp, const struct bt_uuid *uuid)
{
	return -EOPNOTSUPP;
}

int bt_spp_send(struct bt_spp *spp, struct net_buf *buf)
{
	return -EOPNOTSUPP;
}

int bt_spp_disconnect(struct bt_spp *spp)
{
	return -EOPNOTSUPP;
}

struct net_buf *bt_spp_create_pdu(struct bt_spp *spp, struct net_buf_pool *pool)
{
	return NULL;
}
