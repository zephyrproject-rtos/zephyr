/*
 * Copyright 2024 Xiaomi Corporation
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

#include "l2cap_br_internal.h"
#include "rfcomm_internal.h"

#define LOG_LEVEL CONFIG_BT_SPP_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_spp);

#define SPP_RFCOMM_MTU     CONFIG_BT_L2CAP_TX_MTU
#define SDP_CLIENT_BUF_LEN 512

static K_MUTEX_DEFINE(spp_mutex);

#define SPP_LOCK()   k_mutex_lock(&spp_mutex, K_FOREVER)
#define SPP_UNLOCK() k_mutex_unlock(&spp_mutex)

#define BT_SPP_RECORD(n, _) BT_SDP_RECORD(spp_attrs##n)

static struct bt_spp spp_pool[CONFIG_BT_MAX_CONN];

#define BT_SPP_VECTOR(n, _)                                                                        \
	static struct bt_sdp_attribute spp_attrs##n[] = {                                          \
		BT_SDP_NEW_SERVICE,                                                                \
		BT_SDP_LIST(                                                                       \
			BT_SDP_ATTR_SVCLASS_ID_LIST, BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3),         \
			BT_SDP_DATA_ELEM_LIST({BT_SDP_TYPE_SIZE(BT_SDP_UUID16),                    \
					       BT_SDP_ARRAY_16(BT_SDP_SERIAL_PORT_SVCLASS)}, )),   \
		BT_SDP_LIST(                                                                       \
			BT_SDP_ATTR_PROTO_DESC_LIST, BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 12),        \
			BT_SDP_DATA_ELEM_LIST(                                                     \
				{BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3),                             \
				 BT_SDP_DATA_ELEM_LIST({BT_SDP_TYPE_SIZE(BT_SDP_UUID16),           \
							BT_SDP_ARRAY_16(BT_SDP_PROTO_L2CAP)}, )},  \
				{BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 5),                             \
				 BT_SDP_DATA_ELEM_LIST(                                            \
					 {BT_SDP_TYPE_SIZE(BT_SDP_UUID16),                         \
					  BT_SDP_ARRAY_16(BT_SDP_PROTO_RFCOMM)},                   \
					 {BT_SDP_TYPE_SIZE(BT_SDP_UINT8),                          \
					  &spp_pool[n].rfcomm_server.channel}, )}, )),             \
		BT_SDP_LIST(BT_SDP_ATTR_PROFILE_DESC_LIST, BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 8),   \
			    BT_SDP_DATA_ELEM_LIST(                                                 \
				    {BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),                         \
				     BT_SDP_DATA_ELEM_LIST(                                        \
					     {BT_SDP_TYPE_SIZE(BT_SDP_UUID16),                     \
					      BT_SDP_ARRAY_16(BT_SDP_SERIAL_PORT_SVCLASS)},        \
					     {BT_SDP_TYPE_SIZE(BT_SDP_UINT16),                     \
					      BT_SDP_ARRAY_16(0x0102)}, )}, )),                    \
		BT_SDP_SERVICE_NAME("Serial Port"),                                                \
	};

LISTIFY(CONFIG_BT_SPP_MAX_CHANNEL_NUM, BT_SPP_VECTOR, ())

static struct bt_spp_cb *spp_cb;

NET_BUF_POOL_FIXED_DEFINE(sdp_pool, CONFIG_BT_MAX_CONN, SDP_CLIENT_BUF_LEN, 8, NULL);
NET_BUF_POOL_FIXED_DEFINE(tx_pool, CONFIG_BT_MAX_CONN, BT_L2CAP_BUF_SIZE(CONFIG_BT_L2CAP_TX_MTU),
			  16, NULL);

static struct bt_sdp_record spp_rec[] = {
	LISTIFY(CONFIG_BT_SPP_MAX_CHANNEL_NUM, BT_SPP_RECORD, (, ))};

static struct bt_uuid *spp_uuid_copy(const struct bt_uuid *uuid)
{
	struct bt_uuid *spp_uuid;

	switch (uuid->type) {
	case BT_UUID_TYPE_16:
		spp_uuid = malloc(sizeof(struct bt_uuid_16));
		memcpy(spp_uuid, uuid, sizeof(struct bt_uuid_16));
		break;
	case BT_UUID_TYPE_32:
		spp_uuid = malloc(sizeof(struct bt_uuid_32));
		memcpy(spp_uuid, uuid, sizeof(struct bt_uuid_32));
		break;
	case BT_UUID_TYPE_128:
		spp_uuid = malloc(sizeof(struct bt_uuid_128));
		memcpy(spp_uuid, uuid, sizeof(struct bt_uuid_128));
		break;
	default:
		spp_uuid = NULL;
		break;
	}

	return spp_uuid;
}

static struct bt_spp *spp_alloc_service(const struct bt_uuid *uuid)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(spp_pool); i++) {
		if (spp_pool[i].uuid) {
			continue;
		}

		spp_pool[i].uuid = spp_uuid_copy(uuid);
		spp_pool[i].id = i;
		return &spp_pool[i];
	}

	return NULL;
}

static void spp_free_service(struct bt_spp *spp)
{
	free(spp->uuid);
	memset(spp, 0, sizeof(struct bt_spp));
}

static struct bt_spp *spp_find_service_by_conn(struct bt_conn *conn)
{
	uint8_t i;

	for (i = 0; i < ARRAY_SIZE(spp_pool); i++) {
		if (spp_pool[i].conn == conn) {
			return &spp_pool[i];
		}
	}

	return NULL;
}

static struct bt_spp *spp_find_service_by_dlci(struct bt_rfcomm_dlc *dlci)
{
	uint8_t i;

	for (i = 0; i < ARRAY_SIZE(spp_pool); i++) {
		if (&spp_pool[i].rfcomm_dlc == dlci) {
			return &spp_pool[i];
		}
	}

	return NULL;
}

static void bt_spp_recv(struct bt_rfcomm_dlc *dlci, struct net_buf *buf)
{
	struct bt_spp *spp;

	SPP_LOCK();
	spp = spp_find_service_by_dlci(dlci);
	if (!spp) {
		SPP_UNLOCK();
		LOG_ERR("can't find spp for dlci:%p, spp:%p", dlci, spp);
		return;
	}

	if (spp_cb && spp_cb->recv) {
		spp_cb->recv(spp, spp->port, buf->data, buf->len);
	}
	SPP_UNLOCK();
}

static void bt_spp_connected(struct bt_rfcomm_dlc *dlci)
{
	struct bt_spp *spp;

	SPP_LOCK();
	spp = spp_find_service_by_dlci(dlci);
	if (!spp) {
		SPP_UNLOCK();
		LOG_ERR("can't find spp for dlci:%p, spp:%p", dlci, spp);
		return;
	}

	if (spp_cb && spp_cb->connected) {
		spp_cb->connected(spp, spp->port);
	}
	spp->connected = true;
	SPP_UNLOCK();
}

static void bt_spp_disconnected(struct bt_rfcomm_dlc *dlci)
{
	struct bt_spp *spp;

	SPP_LOCK();
	spp = spp_find_service_by_dlci(dlci);
	if (!spp) {
		LOG_ERR("can't find spp for dlci:%p, spp:%p", dlci, spp);
		return;
	}

	if (spp_cb && spp_cb->disconnected) {
		spp_cb->disconnected(spp, spp->port);
	}

	if (spp->connected) {
		spp_free_service(spp);
		spp->connected = false;
	}
	SPP_UNLOCK();
}

static struct bt_rfcomm_dlc_ops spp_rfcomm_ops = {
	.recv = bt_spp_recv,
	.connected = bt_spp_connected,
	.disconnected = bt_spp_disconnected,
};

static int bt_spp_accept(struct bt_conn *conn, struct bt_rfcomm_dlc **dlc)
{
	struct bt_spp *spp;

	LOG_DBG("%s conn %p", __func__, conn);

	SPP_LOCK();
	spp = &spp_pool[bt_conn_index(conn)];

	spp->conn = conn;
	*dlc = &spp->rfcomm_dlc;

	SPP_UNLOCK();
	return 0;
}
static uint8_t bt_spp_sdp_recv(struct bt_conn *conn, struct bt_sdp_client_result *response)
{
	struct bt_spp *spp;
	uint16_t channel;
	int ret;

	if (!response) {
		LOG_WRN("spp response is  null");
		return BT_SDP_DISCOVER_UUID_CONTINUE;
	}

	SPP_LOCK();
	spp = spp_find_service_by_conn(conn);
	if (!spp) {
		LOG_ERR("spp conn is  invalid");
		goto exit;
	}

	if (!response->resp_buf) {
		LOG_ERR("spp sdp resp_buf is null");
		goto exit;
	}

	ret = bt_sdp_get_proto_param(response->resp_buf, BT_SDP_PROTO_RFCOMM, &channel);
	if (ret < 0) {
		LOG_ERR("spp sdp get proto fail");
		goto exit;
	}

	spp->rfcomm_server.accept = NULL;
	spp->rfcomm_dlc.ops = &spp_rfcomm_ops;
	spp->rfcomm_dlc.mtu = SPP_RFCOMM_MTU;
	spp->port = channel;

	ret = bt_rfcomm_dlc_connect(conn, &spp->rfcomm_dlc, channel);
	if (ret < 0) {
		LOG_ERR("spp rfconn connect fail, err:%d", ret);
		goto exit;
	}

	SPP_UNLOCK();
	return BT_SDP_DISCOVER_UUID_STOP;

exit:
	if (spp_cb && spp_cb->disconnected) {
		spp_cb->disconnected(spp, spp->port);
	}

	spp_free_service(spp);
	SPP_UNLOCK();
	return BT_SDP_DISCOVER_UUID_STOP;
}

static struct bt_sdp_discover_params spp_sdp = {
	.func = bt_spp_sdp_recv,
	.pool = &sdp_pool,
};

int bt_spp_register_cb(struct bt_spp_cb *cb)
{
	if (!cb) {
		return -EINVAL;
	}

	spp_cb = cb;
	return 0;
}

int bt_spp_register_srv(uint8_t port, const struct bt_uuid *uuid)
{
	struct bt_spp *spp;
	int ret = 0;

	SPP_LOCK();
	spp = spp_alloc_service(uuid);
	if (!spp) {
		LOG_ERR("spp alloc over max:%d", CONFIG_BT_SPP_MAX_CHANNEL_NUM);
		ret = -ENOMEM;
		goto exit;
	}

	spp->rfcomm_server.channel = port;
	spp->rfcomm_server.accept = &bt_spp_accept;
	spp->rfcomm_dlc.ops = &spp_rfcomm_ops;
	spp->rfcomm_dlc.mtu = SPP_RFCOMM_MTU;
	spp->port = port;

	bt_rfcomm_server_register(&spp->rfcomm_server);
	bt_sdp_register_service(&spp_rec[spp->id]);

exit:
	SPP_UNLOCK();
	return ret;
}

struct bt_spp *bt_spp_connect(struct bt_conn *conn, const struct bt_uuid *uuid)
{
	struct bt_spp *spp;
	int ret;

	SPP_LOCK();
	spp = &spp_pool[bt_conn_index(conn)];

	spp_sdp.uuid = spp_uuid_copy(uuid);

	ret = bt_sdp_discover(conn, &spp_sdp);
	if (ret) {
		SPP_UNLOCK();
		LOG_ERR("spp sdp discover fail");
		return NULL;
	}

	spp->conn = conn;
	SPP_UNLOCK();

	return spp;
}

int bt_spp_send(struct bt_spp *spp, uint8_t *data, uint16_t len)
{
	struct net_buf *buf;
	int ret;

	SPP_LOCK();
	if (!spp || !spp->conn) {
		LOG_ERR("spp or conn invalid");
		ret = -EINVAL;
		goto exit;
	}

	buf = bt_rfcomm_create_pdu(&tx_pool);
	if (!buf) {
		LOG_ERR("spp malloc pdu fail");
		ret = -ENOMEM;
		goto exit;
	}

	net_buf_add_mem(buf, data, len);

	ret = bt_rfcomm_dlc_send(&spp->rfcomm_dlc, buf);
	if (ret < 0) {
		LOG_ERR("rfcomm unable to send: %d", ret);
		net_buf_unref(buf);
		goto exit;
	}

exit:
	SPP_UNLOCK();
	return ret;
}

int bt_spp_disconnect(struct bt_spp *spp)
{
	int ret;

	SPP_LOCK();
	if (!spp || !spp->conn) {
		LOG_ERR("spp or conn invalid");
		ret = -EINVAL;
		goto exit;
	}

	ret = bt_rfcomm_dlc_disconnect(&spp->rfcomm_dlc);
	if (ret < 0) {
		LOG_ERR("bt rfcomm disconnect err:%d", ret);
	}

exit:
	SPP_UNLOCK();
	return ret;
}