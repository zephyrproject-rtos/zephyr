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

#include "l2cap_br_internal.h"
#include "rfcomm_internal.h"
#include "spp_internal.h"

#define LOG_LEVEL CONFIG_BT_SPP_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_spp);

#define SPP_RFCOMM_MTU     CONFIG_BT_L2CAP_TX_MTU
#define SDP_CLIENT_BUF_LEN 512

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

static struct bt_sdp_record spp_rec[] = {LISTIFY(CONFIG_BT_SPP_MAX_CHANNEL_NUM, BT_SPP_RECORD, (, ))};

static struct bt_spp *spp_get_connection(struct bt_conn *conn)
{
	size_t index;

	if (!conn) {
		LOG_ERR("Invalid parameter");
		return NULL;
	}

	index = (size_t)bt_conn_index(conn);
	__ASSERT(index < ARRAY_SIZE(spp_pool), "Conn index is out of bounds");

	return &spp_pool[index];
}

static struct bt_spp *spp_alloc_service(const struct bt_uuid *uuid)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(spp_pool); i++) {
		if (spp_pool[i].uuid) {
			continue;
		}

		spp_pool[i].uuid = uuid;
		spp_pool[i].sdp_rec = &spp_rec[i];
		return &spp_pool[i];
	}

	return NULL;
}

static void spp_free_service(struct bt_spp *spp)
{
	if (spp->acl_conn != NULL) {
		bt_conn_unref(spp->acl_conn);
		spp->acl_conn = NULL;
	}

	memset(spp, 0, sizeof(struct bt_spp));
}

static struct bt_spp *spp_find_service_by_conn(struct bt_conn *conn)
{
	uint8_t i;

	if (conn == NULL) {
		LOG_ERR("Invalid conn parameter");
		return NULL;
	}

	for (i = 0; i < ARRAY_SIZE(spp_pool); i++) {
		if (spp_pool[i].acl_conn == conn) {
			return &spp_pool[i];
		}
	}

	return NULL;
}

static struct bt_spp *spp_find_service_by_dlci(struct bt_rfcomm_dlc *dlci)
{
	uint8_t i;

	if (dlci == NULL) {
		LOG_ERR("Invalid dlci parameter");
		return NULL;
	}

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

	spp = spp_find_service_by_dlci(dlci);
	if (spp == NULL) {
		LOG_ERR("Can't find SPP for dlci:%p", dlci);
		return;
	}

	if (spp_cb && spp_cb->recv) {
		spp_cb->recv(spp, spp->channel, buf->data, buf->len);
	}
}

static void bt_spp_connected(struct bt_rfcomm_dlc *dlci)
{
	struct bt_spp *spp;

	spp = spp_find_service_by_dlci(dlci);
	if (spp == NULL) {
		LOG_ERR("Can't find SPP for dlci:%p", dlci);
		return;
	}

	if (spp_cb && spp_cb->connected) {
		spp_cb->connected(spp, spp->channel);
	}
}

static void bt_spp_disconnected(struct bt_rfcomm_dlc *dlci)
{
	struct bt_spp *spp;

	spp = spp_find_service_by_dlci(dlci);
	if (spp == NULL) {
		LOG_ERR("Can't find SPP for dlci:%p", dlci);
		return;
	}

	if ((spp_cb != NULL) && (spp_cb->disconnected != NULL)) {
		spp_cb->disconnected(spp, spp->channel);
	}

	spp_free_service(spp);
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

	LOG_DBG("%s conn %p", __func__, conn);

	spp = spp_get_connection(conn);
	if (spp == NULL) {
		LOG_ERR("SPP get conn fail");
		return -ENOMEM;
	}

	if (spp->acl_conn != NULL) {
		LOG_ERR("SPP conn exist");
		return -EALREADY;
	}

	spp->acl_conn = bt_conn_ref(conn);
	spp->rfcomm_dlc.ops = &spp_rfcomm_ops;
	spp->rfcomm_dlc.mtu = SPP_RFCOMM_MTU;
	*dlc = &spp->rfcomm_dlc;

	return 0;
}

int bt_spp_register_cb(struct bt_spp_cb *cb)
{
	if (!cb) {
		return -EINVAL;
	}

	spp_cb = cb;
	return 0;
}

int bt_spp_register_srv(uint8_t channel, const struct bt_uuid *uuid)
{
	struct bt_spp *spp;
	int err = 0;

	spp = spp_alloc_service(uuid);
	if (spp == NULL) {
		LOG_ERR("SPP alloc fail, max:%d", CONFIG_BT_SPP_MAX_CHANNEL_NUM);
		err = -ENOMEM;
		goto exit;
	}

	spp->rfcomm_server.channel = channel;
	spp->rfcomm_server.accept = &bt_spp_accept;
	spp->channel = channel;

	bt_rfcomm_server_register(&spp->rfcomm_server);
	bt_sdp_register_service(spp->sdp_rec);

exit:
	return err;
}

int bt_spp_connect(struct bt_conn *conn, struct bt_spp **spp, const struct bt_uuid *uuid)
{
	return -EOPNOTSUPP;
}

int bt_spp_send(struct bt_spp *spp, const uint8_t *data, uint16_t len)
{
	return -EOPNOTSUPP;
}

int bt_spp_disconnect(struct bt_spp *spp)
{
	return -EOPNOTSUPP;
}
