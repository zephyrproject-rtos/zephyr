/* pbap.c - Phone Book Access Profile handling */

/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/types.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/kernel.h>

#include "psa/crypto.h"

#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/bluetooth/classic/rfcomm.h>
#include <zephyr/bluetooth/classic/sdp.h>
#include <zephyr/bluetooth/classic/goep.h>
#include <zephyr/bluetooth/classic/pbap.h>

#include "host/conn_internal.h"
#include "l2cap_br_internal.h"
#include "rfcomm_internal.h"
#include "obex_internal.h"
#include "pbap_internal.h"

#if defined(CONFIG_BT_PBAP)

#define LOG_LEVEL CONFIG_BT_PBAP_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_pbap);

#define PBAP_PWD_MAX_LENGTH 16U

NET_BUF_POOL_FIXED_DEFINE(bt_pbap_pool, CONFIG_BT_MAX_CONN,
			  BT_RFCOMM_BUF_SIZE(CONFIG_BT_GOEP_RFCOMM_MTU),
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

static const struct bt_uuid_128 *pbap_uuid = BT_PBAP_UUID;

#define PBAP_REQUIRED_HDR(_count, _hdrs)                                                           \
	{                                                                                          \
		.count = (_count), .hdrs = (const uint8_t *)(_hdrs),                               \
	}

#define _PBAP_REQUIRED_HDR_LIST(...)                                                               \
	PBAP_REQUIRED_HDR(sizeof((uint8_t[]){__VA_ARGS__}), ((uint8_t[]){__VA_ARGS__}))

#define PBAP_REQUIRED_HDR_LIST(...) _PBAP_REQUIRED_HDR_LIST(__VA_ARGS__)

struct pbap_required_hdr {
	uint8_t count;
	const uint8_t *hdrs;
};

#if defined(CONFIG_BT_PBAP_PCE)
typedef void (*bt_pbap_pce_cb_t)(struct bt_pbap_pce *pbap_pce, uint8_t rsp_code,
				 struct net_buf *buf);
#endif /* CONFIG_BT_PBAP_PCE */
#if defined(CONFIG_BT_PBAP_PSE)
typedef void (*bt_pbap_pse_cb_t)(struct bt_pbap_pse *pbap_pse, struct net_buf *buf);
#endif /* CONFIG_BT_PBAP_PSE */

struct pbap_pull_function {
	const char *type;
	struct pbap_required_hdr req_hdr;
#if defined(CONFIG_BT_PBAP_PSE)
	bt_pbap_pse_cb_t (*get_server_cb)(struct bt_pbap_pse *pbap_pse);
#endif /* CONFIG_BT_PBAP_PSE */
#if defined(CONFIG_BT_PBAP_PCE)
	bt_pbap_pce_cb_t (*get_client_cb)(struct bt_pbap_pce *pbap_pce);
#endif /* CONFIG_BT_PBAP_PCE */
};

#define PULL_PHONEBOOK_REQUIRED_HDR                                                                \
	BT_OBEX_HEADER_ID_NAME, BT_OBEX_HEADER_ID_TYPE, BT_OBEX_HEADER_ID_CONN_ID

#define PULL_VCARDLISTING_REQUIRED_HDR                                                             \
	BT_OBEX_HEADER_ID_NAME, BT_OBEX_HEADER_ID_TYPE, BT_OBEX_HEADER_ID_CONN_ID

#define PULL_VCARDENTRY_REQUIRED_HDR                                                               \
	BT_OBEX_HEADER_ID_NAME, BT_OBEX_HEADER_ID_TYPE, BT_OBEX_HEADER_ID_CONN_ID

#if defined(CONFIG_BT_PBAP_PCE)
static bt_pbap_pce_cb_t pbap_pce_pull_phonebook_cb(struct bt_pbap_pce *pbap_pce)
{
	return pbap_pce->cb->pull_phonebook;
}

static bt_pbap_pce_cb_t pbap_pce_pull_vcardlisting_cb(struct bt_pbap_pce *pbap_pce)
{
	return pbap_pce->cb->pull_vcardlisting;
}

static bt_pbap_pce_cb_t pbap_pce_pull_vcardentry_cb(struct bt_pbap_pce *pbap_pce)
{
	return pbap_pce->cb->pull_vcardentry;
}
#endif /* CONFIG_BT_PBAP_PCE */
#if defined(CONFIG_BT_PBAP_PSE)
static bt_pbap_pse_cb_t pbap_pse_pull_phonebook_cb(struct bt_pbap_pse *pbap_pse)
{
	return pbap_pse->cb->pull_phonebook;
}

static bt_pbap_pse_cb_t pbap_pse_pull_vcardlisting_cb(struct bt_pbap_pse *pbap_pse)
{
	return pbap_pse->cb->pull_vcardlisting;
}
static bt_pbap_pse_cb_t pbap_pse_pull_vcardentry_cb(struct bt_pbap_pse *pbap_pse)
{
	return pbap_pse->cb->pull_vcardentry;
}
#endif /* CONFIG_BT_PBAP_PSE */
static struct pbap_pull_function pbap_pull_functions[] = {
	{BT_PBAP_PULL_PHONEBOOK_TYPE, PBAP_REQUIRED_HDR_LIST(PULL_PHONEBOOK_REQUIRED_HDR),
#if defined(CONFIG_BT_PBAP_PSE)
	 pbap_pse_pull_phonebook_cb,
#endif /* CONFIG_BT_PBAP_PSE */
#if defined(CONFIG_BT_PBAP_PCE)
	 pbap_pce_pull_phonebook_cb
#endif /* CONFIG_BT_PBAP_PCE */
	},
	{BT_PBAP_PULL_VCARD_LISTING_TYPE, PBAP_REQUIRED_HDR_LIST(PULL_VCARDLISTING_REQUIRED_HDR),
#if defined(CONFIG_BT_PBAP_PSE)
	 pbap_pse_pull_vcardlisting_cb,
#endif /* CONFIG_BT_PBAP_PSE */
#if defined(CONFIG_BT_PBAP_PCE)
	 pbap_pce_pull_vcardlisting_cb
#endif /* CONFIG_BT_PBAP_PCE */
	},
	{BT_PBAP_PULL_VCARD_ENTRY_TYPE, PBAP_REQUIRED_HDR_LIST(PULL_VCARDENTRY_REQUIRED_HDR),
#if defined(CONFIG_BT_PBAP_PSE)
	 pbap_pse_pull_vcardentry_cb,
#endif /* CONFIG_BT_PBAP_PSE */
#if defined(CONFIG_BT_PBAP_PCE)
	 pbap_pce_pull_vcardentry_cb
#endif /* CONFIG_BT_PBAP_PCE */
	},
};

static bool has_required_hdrs(struct net_buf *buf, const struct pbap_required_hdr *hdr)
{
	for (uint8_t index = 0; index < hdr->count; index++) {
		if (!bt_obex_has_header(buf, hdr->hdrs[index])) {
			return false;
		}
	}
	return true;
};

static int pbap_check_header_conn_id(uint32_t id, struct net_buf *buf)
{
	uint32_t conn_id;
	int err;

	err = bt_obex_get_header_conn_id(buf, &conn_id);
	if (err != 0) {
		LOG_ERR("Failed to get conn id %d", err);
		return err;
	}

	if (conn_id != id) {
		LOG_ERR("Conn id is mismatched %u != %u", conn_id, id);
		return -EINVAL;
	}

	return 0;
}

static int pbap_check_header_who(struct net_buf *buf)
{
	uint16_t len;
	const uint8_t *who;
	int err;
	union bt_obex_uuid uuid;

	err = bt_obex_get_header_who(buf, &len, &who);
	if (err != 0) {
		LOG_ERR("Failed to get target %d", err);
		return err;
	}

	err = bt_obex_make_uuid(&uuid, who, len);
	if (err != 0) {
		LOG_ERR("Failed to make UUID from who %d", err);
		return err;
	}

	return bt_uuid_cmp(&(uuid.uuid), &pbap_uuid->uuid);
}

#if defined(CONFIG_BT_PBAP_PCE)
static void pbap_pce_clear_pending_request(struct bt_pbap_pce *pbap_pce)
{
	pbap_pce->_rsp_cb = NULL;
	pbap_pce->_req_type = NULL;
}

static void pbap_pce_rfcomm_transport_connected(struct bt_conn *conn, struct bt_goep *goep)
{
	struct bt_pbap_pce *pbap_pce = CONTAINER_OF(goep, struct bt_pbap_pce, _goep);

	atomic_set(&pbap_pce->_transport_state, BT_PBAP_TRANSPORT_STATE_CONNECTED);

	if (pbap_pce->cb != NULL && pbap_pce->cb->rfcomm_connected != NULL) {
		pbap_pce->cb->rfcomm_connected(conn, pbap_pce);
	}
}

static void pbap_pce_rfcomm_transport_disconnected(struct bt_goep *goep)
{
	struct bt_pbap_pce *pbap_pce = CONTAINER_OF(goep, struct bt_pbap_pce, _goep);

	atomic_set(&pbap_pce->_transport_state, BT_PBAP_TRANSPORT_STATE_DISCONNECTED);

	if (pbap_pce->cb != NULL && pbap_pce->cb->rfcomm_disconnected != NULL) {
		pbap_pce->cb->rfcomm_disconnected(pbap_pce);
	}
}

static struct bt_goep_transport_ops pbap_rfcomm_transport_ops = {
	.connected = pbap_pce_rfcomm_transport_connected,
	.disconnected = pbap_pce_rfcomm_transport_disconnected,
};

static void pbap_pce_l2cap_transport_connected(struct bt_conn *conn, struct bt_goep *goep)
{
	struct bt_pbap_pce *pbap_pce = CONTAINER_OF(goep, struct bt_pbap_pce, _goep);

	atomic_set(&pbap_pce->_transport_state, BT_PBAP_TRANSPORT_STATE_CONNECTED);

	if (pbap_pce->cb != NULL && pbap_pce->cb->l2cap_connected != NULL) {
		pbap_pce->cb->l2cap_connected(conn, pbap_pce);
	}
}

static void pbap_pce_l2cap_transport_disconnected(struct bt_goep *goep)
{
	struct bt_pbap_pce *pbap_pce = CONTAINER_OF(goep, struct bt_pbap_pce, _goep);

	atomic_set(&pbap_pce->_transport_state, BT_PBAP_TRANSPORT_STATE_DISCONNECTED);

	if (pbap_pce->cb != NULL && pbap_pce->cb->l2cap_disconnected != NULL) {
		pbap_pce->cb->l2cap_disconnected(pbap_pce);
	}
}

static struct bt_goep_transport_ops pbap_l2cap_transport_ops = {
	.connected = pbap_pce_l2cap_transport_connected,
	.disconnected = pbap_pce_l2cap_transport_disconnected,
};

static int bt_pbap_transport_connect(struct bt_conn *conn, struct bt_pbap_pce *pbap_pce,
				     struct bt_pbap_pce_cb *cb, uint8_t channel, uint16_t psm)
{
	int err;

	if (conn == NULL || pbap_pce == NULL || (channel == 0 && psm == 0) || cb == NULL) {
		return -EINVAL;
	}

	if (atomic_get(&pbap_pce->_transport_state) != BT_PBAP_TRANSPORT_STATE_DISCONNECTED) {
		return -EINPROGRESS;
	}

	pbap_pce->cb = cb;

	if (channel != 0 && psm == 0) {
		pbap_pce->_goep.transport_ops = &pbap_rfcomm_transport_ops;
		err = bt_goep_transport_rfcomm_connect(conn, &pbap_pce->_goep, channel);
	} else {
		pbap_pce->_goep.transport_ops = &pbap_l2cap_transport_ops;
		err = bt_goep_transport_l2cap_connect(conn, &pbap_pce->_goep, psm);
	}

	if (err != 0) {
		atomic_set(&pbap_pce->_transport_state, BT_PBAP_TRANSPORT_STATE_DISCONNECTED);
	}

	atomic_set(&pbap_pce->_transport_state, BT_PBAP_TRANSPORT_STATE_CONNECTING);

	return err;
}

static int bt_pbap_transport_disconnect(struct bt_pbap_pce *pbap_pce, bool is_rfcomm)
{
	int err;

	if (pbap_pce == NULL) {
		return -EINVAL;
	}

	if (atomic_get(&pbap_pce->_transport_state) != BT_PBAP_TRANSPORT_STATE_CONNECTED) {
		return -EINPROGRESS;
	}

	if (is_rfcomm) {
		err = bt_goep_transport_rfcomm_disconnect(&pbap_pce->_goep);
	} else {
		err = bt_goep_transport_l2cap_disconnect(&pbap_pce->_goep);
	}

	if (err != 0) {
		atomic_set(&pbap_pce->_transport_state, BT_PBAP_TRANSPORT_STATE_CONNECTED);
	}

	atomic_set(&pbap_pce->_transport_state, BT_PBAP_TRANSPORT_STATE_CONNECTING);

	return err;
}

int bt_pbap_pce_rfcomm_connect(struct bt_conn *conn, struct bt_pbap_pce *pbap_pce,
			       struct bt_pbap_pce_cb *cb, uint8_t channel)
{
	return bt_pbap_transport_connect(conn, pbap_pce, cb, channel, 0);
}

int bt_pbap_pce_rfcomm_disconnect(struct bt_pbap_pce *pbap_pce)
{
	return bt_pbap_transport_disconnect(pbap_pce, true);
}

int bt_pbap_pce_l2cap_connect(struct bt_conn *conn, struct bt_pbap_pce *pbap_pce,
			      struct bt_pbap_pce_cb *cb, uint16_t psm)
{
	return bt_pbap_transport_connect(conn, pbap_pce, cb, 0, psm);
}

int bt_pbap_pce_l2cap_disconnect(struct bt_pbap_pce *pbap_pce)
{
	return bt_pbap_transport_disconnect(pbap_pce, false);
}

static void pbap_pce_connect(struct bt_obex_client *client, uint8_t rsp_code, uint8_t version,
			     uint16_t mopl, struct net_buf *buf)
{
	struct bt_pbap_pce *pbap_pce;
	int err;

	if (client == NULL) {
		LOG_ERR("Invalid client parameter");
		return;
	}

	pbap_pce = CONTAINER_OF(client, struct bt_pbap_pce, _client);

	if (rsp_code == BT_PBAP_RSP_CODE_OK) {
		atomic_set(&pbap_pce->_state, BT_PBAP_STATE_CONNECTED);
		err = pbap_check_header_who(buf);
		if (err != 0) {
			LOG_ERR("Failed to check who header %d", err);
			err = bt_pbap_pce_disconnect(pbap_pce, NULL);
			if (err != 0) {
				LOG_ERR("Failed to disconnect %d", err);
			}
			return;
		}
		bt_obex_get_header_conn_id(buf, &pbap_pce->_conn_id);

	} else if (rsp_code != BT_PBAP_RSP_CODE_UNAUTH) {
		atomic_set(&pbap_pce->_state, BT_PBAP_STATE_DISCONNECTED);
	}

	if (pbap_pce->cb->connect != NULL) {
		pbap_pce->cb->connect(pbap_pce, rsp_code, version, mopl, buf);
	}
}

static void pbap_pce_disconnect(struct bt_obex_client *client, uint8_t rsp_code,
				struct net_buf *buf)
{
	struct bt_pbap_pce *pbap_pce = CONTAINER_OF(client, struct bt_pbap_pce, _client);

	if (rsp_code == BT_OBEX_RSP_CODE_SUCCESS) {
		atomic_set(&pbap_pce->_state, BT_PBAP_STATE_DISCONNECTED);

		pbap_pce_clear_pending_request(pbap_pce);
	} else {
		atomic_set(&pbap_pce->_state, BT_PBAP_STATE_CONNECTED);
	}

	if (pbap_pce->cb->disconnect != NULL) {
		pbap_pce->cb->disconnect(pbap_pce, rsp_code, buf);
	}
}

static void pbap_pce_get(struct bt_obex_client *client, uint8_t rsp_code, struct net_buf *buf)
{
	struct bt_pbap_pce *pbap_pce;

	pbap_pce = CONTAINER_OF(client, struct bt_pbap_pce, _client);
	if (pbap_pce == NULL) {
		LOG_WRN("No available pbap_pce");
		return;
	}

	pbap_pce->_rsp_cb(pbap_pce, rsp_code, buf);

	if (rsp_code != BT_PBAP_RSP_CODE_CONTINUE) {
		pbap_pce_clear_pending_request(pbap_pce);
	}
}

static void pbap_pce_setpath(struct bt_obex_client *client, uint8_t rsp_code, struct net_buf *buf)
{

	struct bt_pbap_pce *pbap_pce;

	if (client == NULL) {
		LOG_ERR("Invalid client parameter");
		return;
	}

	pbap_pce = CONTAINER_OF(client, struct bt_pbap_pce, _client);

	if (pbap_pce->cb->set_path) {
		pbap_pce->cb->set_path(pbap_pce, rsp_code, buf);
	}
}

static void pbap_pce_abort(struct bt_obex_client *client, uint8_t rsp_code, struct net_buf *buf)
{

	struct bt_pbap_pce *pbap_pce;

	if (client == NULL) {
		LOG_ERR("Invalid client parameter");
		return;
	}

	pbap_pce = CONTAINER_OF(client, struct bt_pbap_pce, _client);

	if (pbap_pce->cb->abort) {
		pbap_pce->cb->abort(pbap_pce, rsp_code, buf);
	}

	if (rsp_code == BT_PBAP_RSP_CODE_SUCCESS) {
		pbap_pce_clear_pending_request(pbap_pce);
	}
}

struct bt_obex_client_ops pbap_pce_ops = {
	.connect = pbap_pce_connect,
	.disconnect = pbap_pce_disconnect,
	.get = pbap_pce_get,
	.setpath = pbap_pce_setpath,
	.abort = pbap_pce_abort,
};

static int pbap_check_header_target(struct net_buf *buf)
{
	uint16_t len;
	const uint8_t *target;
	int err;
	union bt_obex_uuid uuid;

	err = bt_obex_get_header_target(buf, &len, &target);
	if (err != 0) {
		LOG_ERR("Failed to get target %d", err);
		return err;
	}

	err = bt_obex_make_uuid(&uuid, target, len);
	if (err != 0) {
		LOG_ERR("Failed to make UUID from target %d", err);
		return err;
	}

	return bt_uuid_cmp(&(uuid.uuid), &pbap_uuid->uuid);
}

int bt_pbap_pce_connect(struct bt_pbap_pce *pbap_pce, uint16_t mopl, struct net_buf *buf)
{
	int err;
	bool allocated = false;
	uint8_t uuid128[BT_UUID_SIZE_128];

	if (atomic_get(&pbap_pce->_transport_state) != BT_PBAP_TRANSPORT_STATE_CONNECTED) {
		LOG_ERR("Transport connection is not established");
		return -EINVAL;
	}

	if (atomic_get(&pbap_pce->_state) != BT_PBAP_STATE_DISCONNECTED &&
	    atomic_get(&pbap_pce->_state) != BT_PBAP_STATE_CONNECTING) {
		LOG_ERR("Invalid state %u", (uint8_t)atomic_get(&pbap_pce->_state));
		return -EINVAL;
	}

	if (buf == NULL) {
		buf = bt_goep_create_pdu(&pbap_pce->_goep, NULL);
		if (buf == NULL) {
			LOG_ERR("Failed to allocate buffer");
			return -ENOBUFS;
		}
		allocated = true;
	}
	if (bt_obex_has_header(buf, BT_OBEX_HEADER_ID_TARGET)) {
		err = pbap_check_header_target(buf);
		if (err != 0) {
			goto failed;
		}
	} else {
		sys_memcpy_swap(uuid128, (uint8_t *)pbap_uuid->val, BT_UUID_SIZE_128);
		err = bt_obex_add_header_target(buf, BT_UUID_SIZE_128, uuid128);
		if (err != 0) {
			LOG_ERR("Failed to add target header");
			goto failed;
		}
	}

	pbap_pce->_client.ops = &pbap_pce_ops;
	pbap_pce->_client.obex = &pbap_pce->_goep.obex;

	err = bt_obex_connect(&pbap_pce->_client, mopl, buf);
	if (err != 0) {
		LOG_ERR("Failed to send conn req");
		goto failed;
	}

	atomic_set(&pbap_pce->_state, BT_PBAP_STATE_CONNECTING);
	return 0;

failed:
	if (allocated) {
		net_buf_unref(buf);
	}
	return err;
}

int bt_pbap_pce_disconnect(struct bt_pbap_pce *pbap_pce, struct net_buf *buf)
{
	int err;
	bool allocated = false;

	if (pbap_pce == NULL) {
		return -EINVAL;
	}

	if (atomic_get(&pbap_pce->_state) != BT_PBAP_STATE_CONNECTED) {
		LOG_ERR("Invalid state %u", (uint8_t)atomic_get(&pbap_pce->_state));
		return -EINVAL;
	}

	if (buf == NULL) {
		buf = bt_goep_create_pdu(&pbap_pce->_goep, NULL);
		if (buf == NULL) {
			LOG_ERR("Failed to allocate buffer");
			return -ENOBUFS;
		}
		allocated = true;
	}

	err = bt_obex_disconnect(&pbap_pce->_client, buf);
	if (err != 0) {
		LOG_ERR("Failed to send conn rsp %d", err);
		if (allocated) {
			net_buf_unref(buf);
		}
		return err;
	}

	atomic_set(&pbap_pce->_state, BT_PBAP_STATE_DISCONNECTING);
	return 0;
}

static int pbap_pce_get_req_cb(struct bt_pbap_pce *pbap_pce, const char *type, struct net_buf *buf,
			       bt_pbap_pce_cb_t *cb, const char **req_type)
{
	int err;
	uint16_t len;
	const uint8_t *type_data;

	if (pbap_pce == NULL) {
		LOG_ERR("Invalid pbap_pce handle");
		return -EINVAL;
	}

	err = bt_obex_get_header_type(buf, &len, &type_data);
	if (err != 0) {
		LOG_WRN("Failed to get type header %d", err);
		return -EINVAL;
	}

	if (len < strlen(type_data)) {
		LOG_WRN("Invalid type string len %u < %u", len, strlen(type_data));
		return -EINVAL;
	}

	*cb = NULL;
	ARRAY_FOR_EACH(pbap_pull_functions, i) {
		err = strcmp(pbap_pull_functions[i].type, type);
		if (err != 0) {
			continue;
		}
		err = strcmp(pbap_pull_functions[i].type, type_data);
		if (err != 0) {
			continue;
		}

		if (!has_required_hdrs(buf, &pbap_pull_functions[i].req_hdr)) {
			continue;
		}

		/* Application parameter tag id is not checked. */

		if (pbap_pull_functions[i].get_client_cb == NULL) {
			continue;
		}

		*cb = pbap_pull_functions[i].get_client_cb(pbap_pce);
		if (*cb == NULL) {
			continue;
		}

		if (req_type != NULL) {
			*req_type = pbap_pull_functions[i].type;
		}
		break;
	}

	if (*cb == NULL) {
		LOG_WRN("Unsupported request");
		return -EINVAL;
	}

	return 0;
}

static int bt_pbap_pce_get(struct bt_pbap_pce *pbap_pce, struct net_buf *buf, const char *type)
{
	int err = 0;
	bt_pbap_pce_cb_t cb;
	bt_pbap_pce_cb_t old_cb;
	const char *req_type;
	const char *old_req_type;

	if (pbap_pce == NULL || buf == NULL) {
		return -EINVAL;
	}

	old_cb = pbap_pce->_rsp_cb;
	old_req_type = pbap_pce->_req_type;

	if (pbap_pce->_rsp_cb == NULL || bt_obex_has_header(buf, BT_OBEX_HEADER_ID_TYPE)) {
		err = pbap_pce_get_req_cb(pbap_pce, type, buf, &cb, &req_type);
		if (err != 0) {
			LOG_ERR("Invalid request %d", err);
			return err;
		}

		if (pbap_pce->_rsp_cb != NULL && cb != pbap_pce->_rsp_cb) {
			LOG_ERR("Previous operation is not completed");
			return -EINVAL;
		}

		pbap_pce->_rsp_cb = cb;
		pbap_pce->_req_type = req_type;
	}

	if (pbap_pce->_req_type != NULL && strcmp(pbap_pce->_req_type, type) != 0) {
		LOG_ERR("Invalid request type %s != %s", pbap_pce->_req_type, type);
		err = -EINVAL;
		goto failed;
	}
	if (bt_obex_has_header(buf, BT_OBEX_HEADER_ID_CONN_ID)) {
		err = pbap_check_header_conn_id(pbap_pce->_conn_id, buf);
		if (err != 0) {
			goto failed;
		}
	}

	err = bt_obex_get(&pbap_pce->_client, true, buf);

failed:
	if (err != 0) {
		pbap_pce->_rsp_cb = old_cb;
		pbap_pce->_req_type = old_req_type;
		LOG_ERR("Failed to send get req %d", err);
	}

	return err;
}

int bt_pbap_pce_pull_phonebook(struct bt_pbap_pce *pbap_pce, struct net_buf *buf)
{
	return bt_pbap_pce_get(pbap_pce, buf, BT_PBAP_PULL_PHONEBOOK_TYPE);
}

int bt_pbap_pce_pull_vcardlisting(struct bt_pbap_pce *pbap_pce, struct net_buf *buf)
{
	return bt_pbap_pce_get(pbap_pce, buf, BT_PBAP_PULL_VCARD_LISTING_TYPE);
}

int bt_pbap_pce_pull_vcardentry(struct bt_pbap_pce *pbap_pce, struct net_buf *buf)
{
	return bt_pbap_pce_get(pbap_pce, buf, BT_PBAP_PULL_VCARD_ENTRY_TYPE);
}

int bt_pbap_pce_setpath(struct bt_pbap_pce *pbap_pce, uint8_t flags, struct net_buf *buf)
{
	int err;

	if (atomic_get(&pbap_pce->_state) != BT_PBAP_STATE_CONNECTED) {
		LOG_ERR("Invalid state %u", (uint8_t)atomic_get(&pbap_pce->_state));
		return -EINVAL;
	}

	if (pbap_pce->_rsp_cb != NULL) {
		LOG_ERR("other operation is ongoing");
		return -EINVAL;
	}

	if ((flags != BT_PBAP_SET_PATH_FLAGS_UP) &&
	    (flags != BT_PBAP_SET_PATH_FLAGS_DOWN_OR_ROOT)) {
		LOG_ERR("Invalid flags %u", flags);
		return -EINVAL;
	}

	if (!bt_obex_has_header(buf, BT_OBEX_HEADER_ID_CONN_ID)) {
		LOG_ERR("No connection ID header");
		return -EINVAL;
	} else if (pbap_check_header_conn_id(pbap_pce->_conn_id, buf) != 0) {
		LOG_ERR("Failed to check connection ID");
		return -EINVAL;
	}

	err = bt_obex_setpath(&pbap_pce->_client, flags, buf);
	if (err != 0) {
		LOG_WRN("Fail to add header srm id %d", err);
	}

	return err;
}

int bt_pbap_pce_abort(struct bt_pbap_pce *pbap_pce, struct net_buf *buf)
{
	int err;
	bool allocated = false;

	if (pbap_pce == NULL) {
		return -EINVAL;
	}

	if (atomic_get(&pbap_pce->_state) != BT_PBAP_STATE_CONNECTED) {
		LOG_ERR("Invalid state %u", (uint8_t)atomic_get(&pbap_pce->_state));
		return -EINVAL;
	}

	if (pbap_pce->_rsp_cb == NULL) {
		LOG_ERR("No operation is ongoing");
		return -EINVAL;
	}

	if (buf == NULL) {
		buf = bt_goep_create_pdu(&pbap_pce->_goep, NULL);
		if (buf == NULL) {
			LOG_ERR("Failed to allocate buffer");
			return -ENOBUFS;
		}
		allocated = true;
	}

	if (bt_obex_has_header(buf, BT_OBEX_HEADER_ID_CONN_ID)) {
		err = pbap_check_header_conn_id(pbap_pce->_conn_id, buf);
		if (err != 0) {
			goto failed;
		}
	} else {
		err = bt_obex_add_header_conn_id(buf, pbap_pce->_conn_id);
		if (err != 0) {
			LOG_ERR("Failed to add header conn id %d", err);
			goto failed;
		}
	}

	err = bt_obex_abort(&pbap_pce->_client, buf);
	if (err != 0) {
		LOG_ERR("Failed to send abort request %d", err);
		goto failed;
	}

	return 0;

failed:
	if (allocated) {
		net_buf_unref(buf);
	}
	return err;
}
#endif /* CONFIG_BT_PBAP_PCE */

static struct net_buf *bt_pbap_create_pdu(struct bt_goep *goep, struct net_buf_pool *pool)
{
	if (pool == NULL) {
		return bt_goep_create_pdu(goep, &bt_pbap_pool);
	}
	return bt_goep_create_pdu(goep, pool);
}

struct net_buf *bt_pbap_pce_create_pdu(struct bt_pbap_pce *pbap_pce, struct net_buf_pool *pool)
{
	return bt_pbap_create_pdu(&pbap_pce->_goep, pool);
}

struct net_buf *bt_pbap_pse_create_pdu(struct bt_pbap_pse *pbap_pse, struct net_buf_pool *pool)
{
	return bt_pbap_create_pdu(&pbap_pse->_goep, pool);
}

int bt_pbap_generate_auth_challenge(const uint8_t *pwd, uint8_t *auth_chal_req)
{
	int64_t timestamp = k_uptime_get();
	uint8_t hash_input[PBAP_PWD_MAX_LENGTH + 1U + sizeof(timestamp)] = {0};
	size_t len;
	uint16_t pwd_len;
	int32_t status = PSA_SUCCESS;

	if (pwd == NULL) {
		LOG_WRN("no available password");
		return -EINVAL;
	}

	if (auth_chal_req == NULL) {
		LOG_WRN("no available auth_chal_req");
		return -EINVAL;
	}
	pwd_len = strlen(pwd);
	if (pwd_len == 0 || pwd_len > PBAP_PWD_MAX_LENGTH) {
		LOG_ERR("Password is invalid");
		return -EINVAL;
	}

	memcpy(hash_input, &timestamp, sizeof(timestamp));
	hash_input[sizeof(timestamp)] = ':';
	memcpy(hash_input + sizeof(timestamp) + 1U, pwd, strlen(pwd));
	status = psa_hash_compute(PSA_ALG_MD5, (const unsigned char *)hash_input,
				  sizeof(timestamp) + 1U + pwd_len, auth_chal_req, 16U, &len);
	if (status != PSA_SUCCESS) {
		LOG_WRN("Genarate auth challenge  failed %d", status);
		return status;
	}
	return 0;
}

int bt_pbap_generate_auth_response(const uint8_t *pwd, uint8_t *auth_chal_req,
				   uint8_t *auth_chal_rsp)
{
	uint8_t hash_input[PBAP_PWD_MAX_LENGTH + BT_OBEX_CHALLENGE_TAG_NONCE_LEN + 1U] = {0};
	size_t len;
	uint16_t pwd_len;
	int32_t status = PSA_SUCCESS;

	if (pwd == NULL) {
		LOG_WRN("no available password");
		return -EINVAL;
	}

	if (auth_chal_req == NULL) {
		LOG_WRN("no available auth_chal_req");
		return -EINVAL;
	}

	if (auth_chal_rsp == NULL) {
		LOG_WRN("no available auth_chal_rsp");
		return -EINVAL;
	}

	pwd_len = strlen(pwd);
	if (pwd_len == 0 || pwd_len > PBAP_PWD_MAX_LENGTH) {
		LOG_ERR("Password is invalid");
		return -EINVAL;
	}

	memcpy(hash_input, auth_chal_req, BT_OBEX_CHALLENGE_TAG_NONCE_LEN);
	hash_input[BT_OBEX_CHALLENGE_TAG_NONCE_LEN] = ':';
	memcpy(hash_input + BT_OBEX_CHALLENGE_TAG_NONCE_LEN + 1U, pwd, pwd_len);

	status = psa_hash_compute(PSA_ALG_MD5, (const unsigned char *)hash_input,
				  BT_OBEX_CHALLENGE_TAG_NONCE_LEN + 1U + pwd_len, auth_chal_rsp,
				  BT_OBEX_RESPONSE_TAG_REQ_DIGEST_LEN, &len);
	if (status != PSA_SUCCESS) {
		LOG_WRN("Generate auth response failed %d", status);
		return status;
	}
	return 0;
}

int bt_pbap_verify_auth(uint8_t *auth_chal_req, uint8_t *auth_chal_rsp, const uint8_t *pwd)
{
	uint8_t result[BT_OBEX_RESPONSE_TAG_REQ_DIGEST_LEN] = {0};
	int32_t status = PSA_SUCCESS;

	status = bt_pbap_generate_auth_response(pwd, auth_chal_req, (uint8_t *)&result);
	if (status == PSA_SUCCESS) {
		return memcmp(result, (const uint8_t *)auth_chal_rsp,
			      BT_OBEX_RESPONSE_TAG_REQ_DIGEST_LEN);
	}

	return status;
}

#if defined(CONFIG_BT_PBAP_PSE)
static void pse_connect(struct bt_obex_server *server, uint8_t version, uint16_t mopl,
			struct net_buf *buf)
{
	struct bt_pbap_pse *pbap_pse = CONTAINER_OF(server, struct bt_pbap_pse, _server);

	atomic_set(&pbap_pse->_state, BT_PBAP_STATE_CONNECTING);

	if (pbap_pse->cb->connect != NULL) {
		pbap_pse->cb->connect(pbap_pse, version, mopl, buf);
	}
}

static void pse_disconnect(struct bt_obex_server *server, struct net_buf *buf)
{
	struct bt_pbap_pse *pbap_pse = CONTAINER_OF(server, struct bt_pbap_pse, _server);

	atomic_set(&pbap_pse->_state, BT_PBAP_STATE_DISCONNECTING);

	if (pbap_pse->cb->disconnect != NULL) {
		pbap_pse->cb->disconnect(pbap_pse, buf);
	}
}

static enum bt_obex_rsp_code pbap_pse_get_req_cb(struct bt_pbap_pse *pbap_pse, struct net_buf *buf,
						 bt_pbap_pse_cb_t *cb)
{
	const uint8_t *type;
	uint16_t len;
	int err;

	err = bt_obex_get_header_type(buf, &len, &type);
	if (err != 0) {
		LOG_WRN("Failed to get type header %d", err);
		return BT_OBEX_RSP_CODE_BAD_REQ;
	}

	if (len <= strlen(type)) {
		LOG_WRN("Invalid type string len %u <= %u", len, strlen(type));
		return BT_OBEX_RSP_CODE_BAD_REQ;
	}

	*cb = NULL;
	ARRAY_FOR_EACH(pbap_pull_functions, i) {
		if (pbap_pse->_optype != NULL && pbap_pull_functions[i].type != pbap_pse->_optype) {
			continue;
		}

		if (strcmp(pbap_pull_functions[i].type, type) != 0) {
			continue;
		}

		if (!has_required_hdrs(buf, &pbap_pull_functions[i].req_hdr)) {
			continue;
		}

		/* Application parameter tag id is not checked. */

		if (pbap_pull_functions[i].get_server_cb == NULL) {
			continue;
		}

		*cb = pbap_pull_functions[i].get_server_cb(pbap_pse);
		if (*cb == NULL) {
			continue;
		}

		pbap_pse->_optype = pbap_pull_functions[i].type;
		break;
	}

	if (*cb == NULL) {
		LOG_WRN("Unsupported request");
		return BT_OBEX_RSP_CODE_NOT_IMPL;
	}

	return BT_OBEX_RSP_CODE_SUCCESS;
}

static void pse_get(struct bt_obex_server *server, bool final, struct net_buf *buf)
{
	struct bt_pbap_pse *pbap_pse = CONTAINER_OF(server, struct bt_pbap_pse, _server);
	int err = 0;
	enum bt_obex_rsp_code rsp_code;

	if (pbap_pse->_optype == NULL || bt_obex_has_header(buf, BT_OBEX_HEADER_ID_TYPE)) {
		rsp_code = pbap_pse_get_req_cb(pbap_pse, buf, &pbap_pse->_req_cb);
		if (rsp_code != BT_OBEX_RSP_CODE_SUCCESS) {
			LOG_WRN("Failed to parse req %u", (uint8_t)rsp_code);
			goto failed;
		}
	}

	if (pbap_pse->_optype == NULL || pbap_pse->_req_cb == NULL) {
		rsp_code = BT_OBEX_RSP_CODE_NOT_IMPL;
		LOG_WRN("Invalid request");
		goto failed;
	}

	pbap_pse->_req_cb(pbap_pse, buf);

	return;

failed:
	pbap_pse->_optype = NULL;
	pbap_pse->_req_cb = NULL;
	err = bt_obex_get_rsp(server, rsp_code, NULL);
	if (err != 0) {
		LOG_ERR("Failed to send get rsp %d", err);
	}
}

static void pse_setpath(struct bt_obex_server *server, uint8_t flags, struct net_buf *buf)
{
	struct bt_pbap_pse *pbap_pse = CONTAINER_OF(server, struct bt_pbap_pse, _server);

	if (pbap_pse != NULL && pbap_pse->cb != NULL && pbap_pse->cb->setpath != NULL) {
		pbap_pse->cb->setpath(pbap_pse, flags, buf);
	}
}

void pse_abort(struct bt_obex_server *server, struct net_buf *buf)
{
	struct bt_pbap_pse *pbap_pse = CONTAINER_OF(server, struct bt_pbap_pse, _server);

	if (pbap_pse != NULL && pbap_pse->cb != NULL && pbap_pse->cb->abort != NULL) {
		pbap_pse->cb->abort(pbap_pse, buf);
	}
}

struct bt_obex_server_ops pbap_pse_server_ops = {
	.connect = pse_connect,
	.disconnect = pse_disconnect,
	.get = pse_get,
	.abort = pse_abort,
	.setpath = pse_setpath,
};

static void pse_rfcomm_transport_connected(struct bt_conn *conn, struct bt_goep *goep)
{
	struct bt_pbap_pse *pbap_pse = CONTAINER_OF(goep, struct bt_pbap_pse, _goep);

	atomic_set(&pbap_pse->_transport_state, BT_PBAP_TRANSPORT_STATE_CONNECTED);

	if (pbap_pse->cb != NULL && pbap_pse->cb->rfcomm_connected != NULL) {
		pbap_pse->cb->rfcomm_connected(conn, pbap_pse);
	}
}

static void pse_rfcomm_transport_disconnected(struct bt_goep *goep)
{
	struct bt_pbap_pse *pbap_pse = CONTAINER_OF(goep, struct bt_pbap_pse, _goep);

	atomic_set(&pbap_pse->_transport_state, BT_PBAP_TRANSPORT_STATE_DISCONNECTED);

	if (pbap_pse->cb != NULL && pbap_pse->cb->rfcomm_disconnected != NULL) {
		pbap_pse->cb->rfcomm_disconnected(pbap_pse);
	}
}

struct bt_goep_transport_ops pse_rfcomm_transport_ops = {
	.connected = pse_rfcomm_transport_connected,
	.disconnected = pse_rfcomm_transport_disconnected,
};

static void pse_l2cap_transport_connected(struct bt_conn *conn, struct bt_goep *goep)
{
	struct bt_pbap_pse *pbap_pse = CONTAINER_OF(goep, struct bt_pbap_pse, _goep);

	atomic_set(&pbap_pse->_transport_state, BT_PBAP_TRANSPORT_STATE_CONNECTED);

	if (pbap_pse->cb != NULL && pbap_pse->cb->l2cap_connected != NULL) {
		pbap_pse->cb->l2cap_connected(conn, pbap_pse);
	}
}

static void pse_l2cap_transport_disconnected(struct bt_goep *goep)
{
	struct bt_pbap_pse *pbap_pse = CONTAINER_OF(goep, struct bt_pbap_pse, _goep);

	atomic_set(&pbap_pse->_transport_state, BT_PBAP_TRANSPORT_STATE_DISCONNECTED);

	if (pbap_pse->cb != NULL && pbap_pse->cb->l2cap_disconnected != NULL) {
		pbap_pse->cb->rfcomm_disconnected(pbap_pse);
	}
}

struct bt_goep_transport_ops pse_l2cap_transport_ops = {
	.connected = pse_l2cap_transport_connected,
	.disconnected = pse_l2cap_transport_disconnected,
};

static int pbap_pse_rfcomm_accept(struct bt_conn *conn,
				  struct bt_goep_transport_rfcomm_server *server,
				  struct bt_goep **goep)
{
	struct bt_pbap_pse_rfcomm *pbap_pse_rfcomm;
	struct bt_pbap_pse *pbap_pse;
	int err;

	pbap_pse_rfcomm = CONTAINER_OF(server, struct bt_pbap_pse_rfcomm, server);
	if (pbap_pse_rfcomm->accept == NULL) {
		return -ENOTSUP;
	}

	err = pbap_pse_rfcomm->accept(conn, pbap_pse_rfcomm, &pbap_pse);
	if (err != 0) {
		return err;
	}

	pbap_pse->_goep.transport_ops = &pse_rfcomm_transport_ops;
	*goep = &pbap_pse->_goep;

	atomic_set(&pbap_pse->_transport_state, BT_PBAP_TRANSPORT_STATE_CONNECTING);

	return 0;
}

static int pbap_pse_l2cap_accept(struct bt_conn *conn,
				 struct bt_goep_transport_l2cap_server *server,
				 struct bt_goep **goep)
{
	struct bt_pbap_pse_l2cap *pbap_pse_l2cap;
	struct bt_pbap_pse *pbap_pse;
	int err;

	pbap_pse_l2cap = CONTAINER_OF(server, struct bt_pbap_pse_l2cap, server);
	if (pbap_pse_l2cap->accept == NULL) {
		return -ENOTSUP;
	}

	err = pbap_pse_l2cap->accept(conn, pbap_pse_l2cap, &pbap_pse);
	if (err != 0) {
		return err;
	}
	pbap_pse->_goep.transport_ops = &pse_l2cap_transport_ops;
	*goep = &pbap_pse->_goep;

	atomic_set(&pbap_pse->_transport_state, BT_PBAP_TRANSPORT_STATE_CONNECTING);

	return 0;
}

int bt_pbap_pse_rfcomm_register(struct bt_pbap_pse_rfcomm *server)
{
	int err;

	if (server == NULL || server->accept == NULL) {
		return -EINVAL;
	}

	server->server.accept = pbap_pse_rfcomm_accept;

	err = bt_goep_transport_rfcomm_server_register(&server->server);
	if (err != 0) {
		LOG_ERR("Fail to register RFCOMM server (error %d)", err);
		return -ENOEXEC;
	}

	return 0;
}

int bt_pbap_pse_l2cap_register(struct bt_pbap_pse_l2cap *server)
{
	int err;

	if (server == NULL || server->accept == NULL) {
		return -EINVAL;
	}

	server->server.accept = pbap_pse_l2cap_accept;

	err = bt_goep_transport_l2cap_server_register(&server->server);
	if (err != 0) {
		LOG_ERR("Fail to register l2cap server (error %d)", err);
		return -ENOEXEC;
	}

	return 0;
}

static uint32_t pbap_pse_get_connect_id(void)
{
	static uint32_t connect_id;

	connect_id++;

	return connect_id;
}

int bt_pbap_pse_register(struct bt_pbap_pse *pbap_pse, struct bt_pbap_pse_cb *cb)
{
	int err;

	if (pbap_pse == NULL || cb == NULL) {
		return -EINVAL;
	}

	pbap_pse->_server.ops = &pbap_pse_server_ops;
	pbap_pse->_server.obex = &pbap_pse->_goep.obex;
	pbap_pse->cb = cb;
	pbap_pse->_conn_id = pbap_pse_get_connect_id();

	err = bt_obex_server_register(&pbap_pse->_server, pbap_uuid);
	if (err != 0) {
		LOG_ERR("Fail to register obex server %d", err);
		return err;
	}

	return 0;
}

int bt_pbap_pse_connect_rsp(struct bt_pbap_pse *pbap_pse, uint16_t mopl, uint8_t rsp_code,
			    struct net_buf *buf)
{
	int err;
	bool allocated = false;
	uint8_t uuid128[BT_UUID_SIZE_128];

	if (pbap_pse == NULL) {
		return -EINVAL;
	}

	if (atomic_get(&pbap_pse->_state) != BT_PBAP_STATE_CONNECTING) {
		LOG_ERR("Invalid state %u", (uint8_t)atomic_get(&pbap_pse->_state));
		return -EINVAL;
	}
	if (buf == NULL) {
		buf = bt_goep_create_pdu(&pbap_pse->_goep, NULL);
		if (buf == NULL) {
			LOG_ERR("Failed to allocate buffer");
			return -ENOBUFS;
		}
		allocated = true;
	}

	if (bt_obex_has_header(buf, BT_OBEX_HEADER_ID_WHO)) {
		err = pbap_check_header_who(buf);
		if (err != 0) {
			LOG_ERR("Invalid WHO header");
			goto failed;
		}
	} else {
		sys_memcpy_swap(uuid128, (uint8_t *)pbap_uuid->val, BT_UUID_SIZE_128);
		err = bt_obex_add_header_who(buf, BT_UUID_SIZE_128, uuid128);
		if (err != 0) {
			LOG_ERR("Failed to add WHO header");
			goto failed;
		}
	}

	if (bt_obex_has_header(buf, BT_OBEX_HEADER_ID_CONN_ID)) {
		err = pbap_check_header_conn_id(pbap_pse->_conn_id, buf);
		if (err != 0) {
			LOG_ERR("Invalid connection ID");
			goto failed;
		}
	} else {
		err = bt_obex_add_header_conn_id(buf, pbap_pse->_conn_id);
		if (err != 0) {
			LOG_ERR("Failed to add connection ID header");
			goto failed;
		}
	}

	err = bt_obex_connect_rsp(&pbap_pse->_server, rsp_code, mopl, buf);
	if (err != 0) {
		LOG_ERR("Failed to send CONNECT response (error %d)", err);
		goto failed;
	}

	atomic_set(&pbap_pse->_state, BT_PBAP_STATE_CONNECTED);
	return 0;
failed:
	if (allocated) {
		net_buf_unref(buf);
	}
	return err;
}

static int pbap_pse_get_rsp(struct bt_pbap_pse *pbap_pse, uint8_t rsp_code, struct net_buf *buf,
			    const char *type)
{
	int err;

	if (atomic_get(&pbap_pse->_state) != BT_PBAP_STATE_CONNECTED) {
		LOG_ERR("Invalid state %u", (uint8_t)atomic_get(&pbap_pse->_state));
		return -EINVAL;
	}

	if (pbap_pse->_optype != NULL && strcmp(pbap_pse->_optype, type) != 0) {
		LOG_ERR("Invalid operation type %s != %s", pbap_pse->_optype, type);
		return -EINVAL;
	}

	err = bt_obex_get_rsp(&pbap_pse->_server, rsp_code, buf);
	if (err != 0) {
		LOG_ERR("Failed to send get rsp %d", err);
		return err;
	}

	if (rsp_code != BT_OBEX_RSP_CODE_CONTINUE) {
		pbap_pse->_optype = NULL;
		pbap_pse->_req_cb = NULL;
	}

	return 0;
}

int bt_pbap_pse_pull_phonebook_rsp(struct bt_pbap_pse *pbap_pse, uint8_t rsp_code,
				   struct net_buf *buf)
{
	return pbap_pse_get_rsp(pbap_pse, rsp_code, buf, BT_PBAP_PULL_PHONEBOOK_TYPE);
}

int bt_pbap_pse_pull_vcardlisting_rsp(struct bt_pbap_pse *pbap_pse, uint8_t rsp_code,
				      struct net_buf *buf)
{
	return pbap_pse_get_rsp(pbap_pse, rsp_code, buf, BT_PBAP_PULL_VCARD_LISTING_TYPE);
}

int bt_pbap_pse_pull_vcardentry_rsp(struct bt_pbap_pse *pbap_pse, uint8_t rsp_code,
				    struct net_buf *buf)
{
	return pbap_pse_get_rsp(pbap_pse, rsp_code, buf, BT_PBAP_PULL_VCARD_ENTRY_TYPE);
}

int bt_pbap_pse_setpath_rsp(struct bt_pbap_pse *pbap_pse, uint8_t rsp_code, struct net_buf *buf)
{
	int err;
	bool allocated = false;

	if (atomic_get(&pbap_pse->_state) != BT_PBAP_STATE_CONNECTED) {
		LOG_ERR("Invalid state %u", (uint8_t)atomic_get(&pbap_pse->_state));
		return -EINVAL;
	}

	if (buf == NULL) {
		buf = bt_goep_create_pdu(&pbap_pse->_goep, NULL);
		if (buf == NULL) {
			LOG_ERR("Failed to allocate buffer");
			return -ENOBUFS;
		}
		allocated = true;
	}

	err = bt_obex_setpath_rsp(&pbap_pse->_server, rsp_code, buf);
	if (err != 0) {
		LOG_ERR("Failed to send SETPATH response (error %d)", err);
		if (allocated) {
			net_buf_unref(buf);
		}
	}

	return 0;
}

int bt_pbap_pse_disconnect_rsp(struct bt_pbap_pse *pbap_pse, uint8_t rsp_code, struct net_buf *buf)
{
	int err;

	if (pbap_pse == NULL) {
		return -EINVAL;
	}

	if (atomic_get(&pbap_pse->_state) != BT_PBAP_STATE_DISCONNECTING) {
		LOG_ERR("Invalid state %u", (uint8_t)atomic_get(&pbap_pse->_state));
		return -EINVAL;
	}

	err = bt_obex_disconnect_rsp(&pbap_pse->_server, rsp_code, buf);
	if (err != 0) {
		LOG_ERR("Failed to send conn rsp %d", err);
		return err;
	}

	if (rsp_code == BT_OBEX_RSP_CODE_SUCCESS) {
		atomic_set(&pbap_pse->_state, BT_PBAP_STATE_DISCONNECTED);
		pbap_pse->_optype = NULL;
		pbap_pse->_req_cb = NULL;
	} else {
		atomic_set(&pbap_pse->_state, BT_PBAP_STATE_CONNECTED);
	}

	return 0;
}

int bt_pbap_pse_abort_rsp(struct bt_pbap_pse *pbap_pse, uint8_t rsp_code, struct net_buf *buf)
{
	int err;

	if (pbap_pse == NULL) {
		return -EINVAL;
	}

	if (atomic_get(&pbap_pse->_state) != BT_PBAP_STATE_CONNECTED) {
		LOG_ERR("Invalid state %u", (uint8_t)atomic_get(&pbap_pse->_state));
		return -EINVAL;
	}

	err = bt_obex_abort_rsp(&pbap_pse->_server, rsp_code, buf);
	if (err != 0) {
		LOG_ERR("Failed to send abort rsp %d", err);
		return err;
	}

	if (rsp_code == BT_OBEX_RSP_CODE_SUCCESS) {
		pbap_pse->_optype = NULL;
		pbap_pse->_req_cb = NULL;
	}

	return 0;
}
#endif /* CONFIG_BT_PBAP_PSE */

#endif /* CONFIG_BT_PBAP */
