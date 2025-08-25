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

NET_BUF_POOL_FIXED_DEFINE(sdp_pool, CONFIG_BT_MAX_CONN, SDP_CLIENT_BUF_LEN, 8, NULL);
NET_BUF_POOL_FIXED_DEFINE(tx_pool, CONFIG_BT_MAX_CONN, BT_L2CAP_BUF_SIZE(CONFIG_BT_L2CAP_TX_MTU),
			  16, NULL);

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

static int spp_alloc_server(const struct bt_uuid *uuid, struct bt_spp_server **srv)
{
	int i;
	struct bt_spp_server *server;

	if (uuid == NULL || srv == NULL) {
		return -EINVAL;
	}

	for (i = 0; i < ARRAY_SIZE(spp_server); i++) {
		server = &spp_server[i];
		if (server->uuid && !bt_uuid_cmp(server->uuid, uuid)) {
			LOG_ERR("SPP server with UUID %s already exists", bt_uuid_str(uuid));
			return -EALREADY;
		}

		if (server->uuid == NULL) {
			server->uuid = uuid;
			server->sdp_rec = &spp_rec[i];
			*srv = server;
			return 0;
		}
	}

	return -ENOMEM;
}

static int spp_free_server(struct bt_spp_server *server)
{
	if (server == NULL) {
		return -EINVAL;
	}

	memset(server, 0, sizeof(*server));
	return 0;
}

static int spp_alloc_connection_with_channel(struct bt_spp **spp, struct bt_conn *conn,
					     uint16_t channel)
{
	struct bt_spp *connection;

	for (int i = 0; i < ARRAY_SIZE(spp_sessions); i++) {
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

	for (int i = 0; i < ARRAY_SIZE(spp_sessions); i++) {
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
	int i;
	struct bt_spp *connection;

	for (i = 0; i < ARRAY_SIZE(spp_sessions); i++) {
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
		spp_cb->recv(spp, spp->channel, buf->data, buf->len);
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
		spp_cb->disconnected(spp, spp->channel);
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

static uint8_t bt_spp_sdp_recv(struct bt_conn *conn, struct bt_sdp_client_result *response,
			       const struct bt_sdp_discover_params *params)
{
	struct bt_spp *spp;
	uint16_t channel;
	int err;

	if (response == NULL) {
		LOG_WRN("SPP response is  null");
		return BT_SDP_DISCOVER_UUID_CONTINUE;
	}

	if (response->resp_buf == NULL) {
		LOG_ERR("SPP sdp resp_buf is null");
		return BT_SDP_DISCOVER_UUID_STOP;
	}

	spp = find_spp_connection_by_uuid(conn, params->uuid);
	if (spp == NULL) {
		LOG_ERR("SPP connection not found for uuid:%s", bt_uuid_str(params->uuid));
		return BT_SDP_DISCOVER_UUID_STOP;
	}

	err = bt_sdp_get_proto_param(response->resp_buf, BT_SDP_PROTO_RFCOMM, &channel);
	if (err < 0) {
		LOG_ERR("SPP sdp get proto fail");
		goto exit_err;
	}

	spp->rfcomm_dlc.ops = &spp_rfcomm_ops;
	spp->rfcomm_dlc.mtu = SPP_RFCOMM_MTU;
	spp->rfcomm_dlc.required_sec_level = BT_SECURITY_L2;
	spp->channel = channel;

	err = bt_rfcomm_dlc_connect(conn, &spp->rfcomm_dlc, channel);
	if (err < 0) {
		LOG_ERR("SPP rfconn connect fail, err:%d", err);
		goto exit_err;
	}

	return BT_SDP_DISCOVER_UUID_STOP;

exit_err:
	if (spp_cb && spp_cb->disconnected) {
		spp_cb->disconnected(spp, spp->channel);
	}

	return BT_SDP_DISCOVER_UUID_STOP;
}

static struct bt_sdp_discover_params sdp_discover = {
	.type = BT_SDP_DISCOVER_SERVICE_SEARCH_ATTR,
	.func = bt_spp_sdp_recv,
	.pool = &sdp_pool,
};

int bt_spp_register_cb(const struct bt_spp_cb *cb)
{
	if (!cb) {
		return -EINVAL;
	}

	if (spp_cb) {
		LOG_ERR("SPP callback already registered");
		return -EALREADY;
	}

	spp_cb = cb;

	return 0;
}

int bt_spp_register_srv(uint8_t channel, const struct bt_uuid *uuid)
{
	struct bt_spp_server *server;
	int err;

	err = spp_alloc_server(uuid, &server);
	if (err < 0) {
		LOG_ERR("SPP alloc server fail, err:%d", err);
		return err;
	}

	server->rfcomm_server.channel = channel;
	server->rfcomm_server.accept = bt_spp_accept;

	err = bt_rfcomm_server_register(&server->rfcomm_server);
	if (err < 0) {
		LOG_ERR("SPP rfcomm server register fail, err:%d", err);
		goto exit_err;
	}

	err = bt_sdp_register_service(server->sdp_rec);
	if (err < 0) {
		LOG_ERR("SPP sdp register server fail, err:%d", err);
		/* TODO: Unregister rfcomm server */
		goto exit_err;
	}

	return 0;

exit_err:
	spp_free_server(server);
	return err;
}

int bt_spp_connect(struct bt_conn *conn, struct bt_spp **spp, const struct bt_uuid *uuid)
{
	struct bt_spp *new_spp;
	int err;

	if (conn == NULL || spp == NULL || uuid == NULL) {
		LOG_ERR("Invalid parameter");
		return -EINVAL;
	}

	err = spp_alloc_connection_with_uuid(&new_spp, conn, uuid);
	if (err < 0) {
		LOG_ERR("SPP alloc connection fail, err:%d", err);
		return err;
	}

	atomic_set(&new_spp->state, SPP_STATE_CONNECTING);

	sdp_discover.uuid = uuid;
	err = bt_sdp_discover(conn, &sdp_discover);
	if (err < 0) {
		LOG_ERR("SPP sdp discover fail");
		spp_free_connection(new_spp);
		return err;
	}

	*spp = new_spp;
	return err;
}

int bt_spp_send(struct bt_spp *spp, const uint8_t *data, uint16_t len)
{
	struct net_buf *buf;
	int err;
	uint32_t state;

	if (spp == NULL) {
		LOG_ERR("SPP or conn invalid");
		return -EINVAL;
	}

	state = atomic_get(&spp->state);
	if (state != SPP_STATE_CONNECTED) {
		LOG_ERR("Cannot send data while not connected");
		return -ENOTCONN;
	}

	buf = bt_rfcomm_create_pdu(&tx_pool);
	if (buf == NULL) {
		LOG_ERR("SPP malloc pdu fail");
		return -ENOMEM;
	}

	net_buf_add_mem(buf, data, len);

	err = bt_rfcomm_dlc_send(&spp->rfcomm_dlc, buf);
	if (err < 0) {
		LOG_ERR("rfcomm unable to send: %d", err);
		net_buf_unref(buf);
		return err;
	}

	return err;
}

int bt_spp_disconnect(struct bt_spp *spp)
{
	int err;
	uint32_t state;

	if (spp == NULL || spp->acl_conn == NULL) {
		LOG_ERR("SPP or conn invalid");
		return -EINVAL;
	}

	state = atomic_get(&spp->state);
	if (state != SPP_STATE_CONNECTED) {
		LOG_ERR("Cannot disconnect SPP connection while not connected");
		return -ENOTCONN;
	}

	err = bt_rfcomm_dlc_disconnect(&spp->rfcomm_dlc);
	if (err < 0) {
		LOG_ERR("bt rfcomm disconnect err:%d", err);
		return err;
	}

	atomic_set(&spp->state, SPP_STATE_DISCONNECTING);
	return 0;
}
