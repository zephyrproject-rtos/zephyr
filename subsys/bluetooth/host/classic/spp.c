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

#define SPP_RFCOMM_MTU     CONFIG_BT_SPP_L2CAP_MTU

K_SEM_DEFINE(spp_lock, 1U, 1U);
NET_BUF_POOL_FIXED_DEFINE(sdp_pool, CONFIG_BT_MAX_CONN, SDP_CLIENT_BUF_LEN, 8, NULL);

static sys_slist_t spp_rfcomm_server = SYS_SLIST_STATIC_INIT(&spp_rfcomm_server);

static struct bt_spp_server *bt_spp_find_service_by_channel(uint8_t channel)
{
	struct bt_spp_server *server;

	SYS_SLIST_FOR_EACH_CONTAINER(&spp_rfcomm_server, server, node) {
		if (server->channel == channel) {
			return server;
		}
	}

	return NULL;
}

static struct bt_spp_server *bt_spp_find_service_by_uuid(const struct bt_uuid *uuid)
{
	struct bt_spp_server *server;

	SYS_SLIST_FOR_EACH_CONTAINER(&spp_rfcomm_server, server, node) {
		if (bt_uuid_cmp(&server->uuid, uuid) == 0) {
			return server;
		}
	}

	return NULL;
}

static void bt_spp_recv(struct bt_rfcomm_dlc *dlci, struct net_buf *buf)
{
	struct bt_spp *spp = CONTAINER_OF(dlci, struct bt_spp, rfcomm_dlc);

	if (spp->ops->recv) {
		spp->ops->recv(spp, buf);
	}
}

static void bt_spp_connected(struct bt_rfcomm_dlc *dlci)
{
	struct bt_spp *spp = CONTAINER_OF(dlci, struct bt_spp, rfcomm_dlc);

	LOG_DBG("SPP %p connected", dlci);

	atomic_set(&spp->state, SPP_STATE_CONNECTED);

	if (spp->ops->connected) {
		spp->ops->connected(spp->acl_conn, spp);
	}
}

static void bt_spp_disconnected(struct bt_rfcomm_dlc *dlci)
{
	struct bt_spp *spp = CONTAINER_OF(dlci, struct bt_spp, rfcomm_dlc);

	LOG_DBG("SPP %p disconnected", dlci);

	atomic_set(&spp->state, SPP_STATE_DISCONNECTED);

	if (spp->ops->disconnected) {
		spp->ops->disconnected(spp);
	}

	bt_conn_unref(spp->acl_conn);
	spp->acl_conn = NULL;
}

static struct bt_rfcomm_dlc_ops spp_rfcomm_ops = {
	.recv = bt_spp_recv,
	.connected = bt_spp_connected,
	.disconnected = bt_spp_disconnected,
};

static int bt_spp_accept(struct bt_conn *conn, struct bt_rfcomm_server *server,
			 struct bt_rfcomm_dlc **dlc)
{
	struct bt_spp_server *rfcomm_server =
		CONTAINER_OF(server, struct bt_spp_server, rfcomm_server);
	struct bt_spp *spp = NULL;
	int err;

	LOG_DBG("accept a spp conn %p", conn);

	k_sem_take(&spp_lock, K_FOREVER);
	if (!sys_slist_find(&spp_rfcomm_server, &rfcomm_server->node, NULL)) {
		k_sem_give(&spp_lock);
		LOG_ERR("SPP server not registered");
		return -EINVAL;
	}

	k_sem_give(&spp_lock);

	err = rfcomm_server->accept(conn, rfcomm_server, &spp);
	if (err < 0) {
		LOG_ERR("SPP accept callback failed, err:%d", err);
		return err;
	}

	if (spp == NULL) {
		LOG_ERR("SPP accept callback return null spp");
		return -EINVAL;
	}

	if (atomic_get(&spp->state) != SPP_STATE_DISCONNECTED) {
		LOG_ERR("SPP already connected or connecting");
		return -EALREADY;
	}

	spp->acl_conn = bt_conn_ref(conn);
	spp->rfcomm_dlc.ops = &spp_rfcomm_ops;
	spp->rfcomm_dlc.mtu = SPP_RFCOMM_MTU;
	spp->rfcomm_dlc.required_sec_level = BT_SECURITY_L2;
	*dlc = &spp->rfcomm_dlc;

	return 0;
}

static int bt_spp_connect_rfcomm(struct bt_conn *conn, struct bt_spp *spp)
{
	int err;

	if ((conn == NULL) || (spp == NULL)) {
		LOG_ERR("Invalid parameter");
		return -EINVAL;
	}

	spp->acl_conn = bt_conn_ref(conn);
	spp->rfcomm_dlc.ops = &spp_rfcomm_ops;
	spp->rfcomm_dlc.mtu = SPP_RFCOMM_MTU;
	spp->rfcomm_dlc.required_sec_level = BT_SECURITY_L2;

	err = bt_rfcomm_dlc_connect(conn, &spp->rfcomm_dlc, spp->channel);
	if (err < 0) {
		LOG_ERR("SPP rfcomm dlc connect fail, err:%d", err);
		return err;
	}

	return 0;
}

static uint8_t sdp_discover_cb(struct bt_conn *conn, struct bt_sdp_client_result *response,
			       const struct bt_sdp_discover_params *params)
{
	uint16_t channel;
	int err;
	struct bt_spp *spp = CONTAINER_OF(params, struct bt_spp, client.sdp_discover);

	if (response == NULL) {
		LOG_WRN("SPP response is null");
		return BT_SDP_DISCOVER_UUID_CONTINUE;
	}

	if (response->resp_buf == NULL) {
		LOG_ERR("SPP sdp resp_buf is null");
		goto exit;
	}

	err = bt_sdp_get_proto_param(response->resp_buf, BT_SDP_PROTO_RFCOMM, &channel);
	if (err < 0) {
		LOG_ERR("SPP sdp get proto fail");
		goto exit;
	}

	LOG_DBG("SPP SDP record channel:%d found for uuid:%s", channel, bt_uuid_str(params->uuid));

	spp->channel = channel;
	err = bt_spp_connect_rfcomm(conn, spp);
	if (err < 0) {
		LOG_ERR("SPP connect fail, err:%d", err);
		goto exit;
	}

	return BT_SDP_DISCOVER_UUID_STOP;

exit:
	if (spp->ops->disconnected) {
		spp->ops->disconnected(spp);
	}

	return BT_SDP_DISCOVER_UUID_STOP;
}

int bt_spp_server_register(struct bt_spp_server *server)
{
	if (!server || (server->accept == NULL) || (server->sdp_record == NULL)) {
		LOG_DBG("Invalid parameter");
		return -EINVAL;
	}

	k_sem_take(&spp_lock, K_FOREVER);

	if (sys_slist_find(&spp_rfcomm_server, &server->node, NULL)) {
		k_sem_give(&spp_lock);
		LOG_WRN("RFCOMM server has been registered");
		return -EEXIST;
	}

	if (bt_spp_find_service_by_channel(server->channel)) {
		k_sem_give(&spp_lock);
		LOG_WRN("SPP server channel:%d has been registered", server->channel);
		return -EEXIST;
	}

	if (bt_spp_find_service_by_uuid(server->uuid)) {
		k_sem_give(&spp_lock);
		LOG_WRN("SPP server channel:%d has been registered", server->channel);
		return -EEXIST;
	}

	bt_sdp_register_service(server->sdp_record);

	server->rfcomm_server.accept = bt_spp_accept;
	bt_rfcomm_server_register(&server->rfcomm_server);

	sys_slist_append(&spp_rfcomm_server, &server->node);

	k_sem_give(&spp_lock);
	return 0;
}

int bt_spp_connect(struct bt_conn *conn, struct bt_spp *spp)
{
	int err;

	if ((spp == NULL) || (spp->mode == BT_SPP_MODE_UNKNOWN) || (spp->ops == NULL)) {
		LOG_ERR("Invalid parameter");
		return -EINVAL;
	}

	if (atomic_get(&spp->state) != SPP_STATE_DISCONNECTED) {
		LOG_ERR("SPP was not in disconnected state");
		return -EALREADY;
	}

	if (spp->mode == BT_SPP_MODE_CHANNEL) {
		err = bt_spp_connect_rfcomm(conn, spp);
		if (err < 0) {
			LOG_ERR("SPP connect fail, err:%d", err);
			return err;
		}
	} else if (spp->mode == BT_SPP_MODE_UUID) {
		spp->client.sdp_discover.type = BT_SDP_DISCOVER_SERVICE_SEARCH_ATTR;
		spp->client.sdp_discover.func = sdp_discover_cb;
		spp->client.sdp_discover.pool = &sdp_pool;
		spp->client.sdp_discover.uuid = spp->uuid;

		err = bt_sdp_discover(conn, &spp->client.sdp_discover);
		if (err < 0) {
			LOG_ERR("SPP sdp discover fail, err:%d", err);
			return err;
		}
	}

	atomic_set(&spp->state, SPP_STATE_CONNECTING);
	return 0;
}

int bt_spp_send(struct bt_spp *spp, struct net_buf *buf)
{
	int err;

	if (spp == NULL) {
		LOG_ERR("spp or buf invalid");
		return -EINVAL;
	}

	if (atomic_get(&spp->state) != SPP_STATE_CONNECTED) {
		LOG_ERR("Cannot send data while not connected");
		return -ENOTCONN;
	}

	err = bt_rfcomm_dlc_send(&spp->rfcomm_dlc, buf);
	if (err < 0) {
		LOG_ERR("rfcomm unable to send: %d", err);
		return err;
	}

	return 0;
}

int bt_spp_disconnect(struct bt_spp *spp)
{
	int err;

	if ((spp == NULL) || (spp->acl_conn == NULL)) {
		LOG_ERR("SPP or conn invalid");
		return -EINVAL;
	}

	if (atomic_get(&spp->state) != SPP_STATE_CONNECTED) {
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
