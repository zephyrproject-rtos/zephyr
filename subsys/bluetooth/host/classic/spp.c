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
#define SDP_CLIENT_BUF_LEN 512

#define SPP_CHANNEL_MIN BT_RFCOMM_CHAN_DYNAMIC_START
#define SPP_CHANNEL_MAX 0x1e

#define BT_SPP_RECORD(n, _) BT_SDP_RECORD(spp_attrs##n)
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
				 BT_SDP_DATA_ELEM_LIST({BT_SDP_TYPE_SIZE(BT_SDP_UUID16),           \
							BT_SDP_ARRAY_16(BT_SDP_PROTO_RFCOMM)},     \
						       {BT_SDP_TYPE_SIZE(BT_SDP_UINT8),            \
							&spp_channel_map[n]},)},)),                \
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

static uint8_t spp_channel_map[CONFIG_BT_SPP_MAX_SERVER_NUM];

LISTIFY(CONFIG_BT_SPP_MAX_SERVER_NUM, BT_SPP_VECTOR, ())

static struct bt_sdp_record spp_rec[] = {LISTIFY(CONFIG_BT_SPP_MAX_SERVER_NUM,
		BT_SPP_RECORD, (,))};

K_SEM_DEFINE(spp_lock, 1U, 1U);
NET_BUF_POOL_FIXED_DEFINE(sdp_pool, CONFIG_BT_MAX_CONN, SDP_CLIENT_BUF_LEN, 8, NULL);

static sys_slist_t spp_rfcomm_server = SYS_SLIST_STATIC_INIT(&spp_rfcomm_server);

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

	LOG_DBG("%s conn %p", __func__, conn);

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

	bt_sdp_register_service(server->sdp_record);

	server->rfcomm_server.accept = bt_spp_accept;
	bt_rfcomm_server_register(&server->rfcomm_server);

	sys_slist_append(&spp_rfcomm_server, &server->node);

	k_sem_give(&spp_lock);
	return 0;
}

struct bt_sdp_record *bt_spp_alloc_record(uint8_t channel)
{
	static uint8_t index;
	struct bt_sdp_record *sdp;

	k_sem_take(&spp_lock, K_FOREVER);

	if (index >= CONFIG_BT_SPP_MAX_SERVER_NUM) {
		k_sem_give(&spp_lock);
		LOG_ERR("No available SPP SDP record slots");
		return NULL;
	}

	if ((channel < SPP_CHANNEL_MIN) || (channel > SPP_CHANNEL_MAX)) {
		k_sem_give(&spp_lock);
		LOG_ERR("Invalid RFCOMM channel: %d", channel);
		return NULL;
	}

	spp_channel_map[index] = channel;
	sdp = &spp_rec[index];
	index++;

	k_sem_give(&spp_lock);

	return sdp;
}

int bt_spp_connect(struct bt_conn *conn, struct bt_spp *spp, const struct bt_uuid *uuid)
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
