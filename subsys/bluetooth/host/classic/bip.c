/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <errno.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include <zephyr/bluetooth/conn.h>

#include "common/assert.h"

#include <zephyr/bluetooth/classic/sdp.h>
#include <zephyr/bluetooth/classic/goep.h>
#include <zephyr/bluetooth/classic/bip.h>

#include "host/hci_core.h"
#include "host/conn_internal.h"
#include "l2cap_br_internal.h"
#include "obex_internal.h"

#define LOG_LEVEL CONFIG_BT_BIP_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_bip);

typedef void (*bt_bip_server_cb_t)(struct bt_bip_server *server, bool final, struct net_buf *buf);
typedef void (*bt_bip_client_cb_t)(struct bt_bip_client *client, uint8_t rsp_code,
				   struct net_buf *buf);

static bool is_bip_primary_connect(enum bt_bip_conn_type type)
{
	switch (type) {
	case BT_BIP_PRIM_CONN_TYPE_IMAGE_PUSH:
	case BT_BIP_PRIM_CONN_TYPE_IMAGE_PULL:
	case BT_BIP_PRIM_CONN_TYPE_ADVANCED_IMAGE_PRINTING:
	case BT_BIP_PRIM_CONN_TYPE_AUTO_ARCHIVE:
	case BT_BIP_PRIM_CONN_TYPE_REMOTE_CAMERA:
	case BT_BIP_PRIM_CONN_TYPE_REMOTE_DISPLAY:
		return true;
	default:
		break;
	}

	return false;
}

#define BIP_RFDCOMM_SERVER(server) CONTAINER_OF(server, struct bt_bip_rfcomm_server, server);

static void bip_rfcomm_connected(struct bt_conn *conn, struct bt_goep *goep)
{
	struct bt_bip *bip = CONTAINER_OF(goep, struct bt_bip, goep);

	atomic_set(&bip->_transport_state, BT_BIP_TRANSPORT_STATE_CONNECTED);

	if (bip->ops != NULL && bip->ops->connected != NULL) {
		bip->ops->connected(conn, bip);
	}
}

static void bip_rfcomm_disconnected(struct bt_goep *goep)
{
	struct bt_bip *bip = CONTAINER_OF(goep, struct bt_bip, goep);
	struct bt_bip_client *client, *cnext;
	struct bt_bip_server *server, *snext;

	atomic_set(&bip->_transport_state, BT_BIP_TRANSPORT_STATE_DISCONNECTED);

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&bip->_clients, client, cnext, _node) {
		if (is_bip_primary_connect(client->_type) && client->_secondary_server != NULL &&
		    client->_secondary_server->_bip != bip) {
			bt_bip_rfcomm_disconnect(client->_secondary_server->_bip);
		}
		atomic_set(&client->_state, BT_BIP_STATE_DISCONNECTED);
	}

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&bip->_servers, server, snext, _node) {
		if (is_bip_primary_connect(server->_type) && server->_secondary_client != NULL &&
		    server->_secondary_client->_bip != bip) {
			bt_bip_rfcomm_disconnect(server->_secondary_client->_bip);
		}
		atomic_set(&server->_state, BT_BIP_STATE_DISCONNECTED);
	}

	sys_slist_init(&bip->_clients);
	bip->role = BT_BIP_ROLE_UNKNOWN;
	bip->_supp_caps = 0;
	bip->_supp_feats = 0;
	bip->_supp_funcs = 0;

	if (bip->ops != NULL && bip->ops->disconnected != NULL) {
		bip->ops->disconnected(bip);
	}
}

static struct bt_goep_transport_ops bip_rfcomm_ops = {
	.connected = bip_rfcomm_connected,
	.disconnected = bip_rfcomm_disconnected,
};

static int bip_rfcomm_accept(struct bt_conn *conn, struct bt_goep_transport_rfcomm_server *server,
			     struct bt_goep **goep)
{
	struct bt_bip_rfcomm_server *bip_server = BIP_RFDCOMM_SERVER(server);
	struct bt_bip *bip;
	int err;

	if (bip_server->accept == NULL) {
		return -ENOTSUP;
	}

	err = bip_server->accept(conn, bip_server, &bip);
	if (err != 0) {
		return err;
	}
	*goep = &bip->goep;
	bip->role = BT_BIP_ROLE_RESPONDER;
	bip->goep.transport_ops = &bip_rfcomm_ops;
	atomic_set(&bip->_transport_state, BT_BIP_TRANSPORT_STATE_CONNECTING);

	return 0;
}

int bt_bip_rfcomm_register(struct bt_bip_rfcomm_server *server)
{
	if (server == NULL || server->accept == NULL) {
		return -EINVAL;
	}

	server->server.accept = bip_rfcomm_accept;

	return bt_goep_transport_rfcomm_server_register(&server->server);
}

int bt_bip_rfcomm_connect(struct bt_conn *conn, struct bt_bip *bip, uint8_t channel)
{
	int err;

	if (conn == NULL || bip == NULL || channel == 0) {
		return -EINVAL;
	}

	if (atomic_get(&bip->_transport_state) != BT_BIP_TRANSPORT_STATE_DISCONNECTED) {
		return -EINPROGRESS;
	}

	bip->role = BT_BIP_ROLE_INITIATOR;
	bip->goep.transport_ops = &bip_rfcomm_ops;
	atomic_set(&bip->_transport_state, BT_BIP_TRANSPORT_STATE_CONNECTING);

	err = bt_goep_transport_rfcomm_connect(conn, &bip->goep, channel);
	if (err != 0) {
		atomic_set(&bip->_transport_state, BT_BIP_TRANSPORT_STATE_DISCONNECTED);
	}

	return err;
}

int bt_bip_rfcomm_disconnect(struct bt_bip *bip)
{
	int err;

	if (bip == NULL) {
		return -EINVAL;
	}

	if (atomic_get(&bip->_transport_state) != BT_BIP_TRANSPORT_STATE_CONNECTED) {
		return -EINPROGRESS;
	}

	atomic_set(&bip->_transport_state, BT_BIP_TRANSPORT_STATE_DISCONNECTING);

	err = bt_goep_transport_rfcomm_disconnect(&bip->goep);
	if (err != 0) {
		atomic_set(&bip->_transport_state, BT_BIP_TRANSPORT_STATE_CONNECTED);
	}

	return err;
}

#define BIP_L2CAP_SERVER(server) CONTAINER_OF(server, struct bt_bip_l2cap_server, server);

static void bip_l2cap_connected(struct bt_conn *conn, struct bt_goep *goep)
{
	struct bt_bip *bip = CONTAINER_OF(goep, struct bt_bip, goep);

	atomic_set(&bip->_transport_state, BT_BIP_TRANSPORT_STATE_CONNECTED);

	if (bip->ops != NULL && bip->ops->connected != NULL) {
		bip->ops->connected(conn, bip);
	}
}

static void bip_l2cap_disconnected(struct bt_goep *goep)
{
	struct bt_bip *bip = CONTAINER_OF(goep, struct bt_bip, goep);
	struct bt_bip_client *client, *cnext;
	struct bt_bip_server *server, *snext;

	atomic_set(&bip->_transport_state, BT_BIP_TRANSPORT_STATE_DISCONNECTED);

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&bip->_clients, client, cnext, _node) {
		if (is_bip_primary_connect(client->_type) && client->_secondary_server != NULL &&
		    client->_secondary_server->_bip != bip) {
			bt_bip_l2cap_disconnect(client->_secondary_server->_bip);
		}
		atomic_set(&client->_state, BT_BIP_STATE_DISCONNECTED);
	}

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&bip->_servers, server, snext, _node) {
		if (is_bip_primary_connect(server->_type) && server->_secondary_client != NULL &&
		    server->_secondary_client->_bip != bip) {
			bt_bip_l2cap_disconnect(server->_secondary_client->_bip);
		}
		atomic_set(&server->_state, BT_BIP_STATE_DISCONNECTED);
	}

	sys_slist_init(&bip->_clients);
	bip->role = BT_BIP_ROLE_UNKNOWN;
	bip->_supp_caps = 0;
	bip->_supp_feats = 0;
	bip->_supp_funcs = 0;

	if (bip->ops != NULL && bip->ops->disconnected != NULL) {
		bip->ops->disconnected(bip);
	}
}

static struct bt_goep_transport_ops bip_l2cap_ops = {
	.connected = bip_l2cap_connected,
	.disconnected = bip_l2cap_disconnected,
};

static int bip_l2cap_accept(struct bt_conn *conn, struct bt_goep_transport_l2cap_server *server,
			    struct bt_goep **goep)
{
	struct bt_bip_l2cap_server *bip_server = BIP_L2CAP_SERVER(server);
	struct bt_bip *bip;
	int err;

	if (bip_server->accept == NULL) {
		return -ENOTSUP;
	}

	err = bip_server->accept(conn, bip_server, &bip);
	if (err != 0) {
		LOG_WRN("Incoming connection rejected");
		return err;
	}

	if (bip == NULL || bip->ops == NULL) {
		LOG_WRN("Invalid parameter");
		return -EINVAL;
	}

	bip->goep.transport_ops = &bip_l2cap_ops;
	*goep = &bip->goep;
	bip->role = BT_BIP_ROLE_RESPONDER;
	atomic_set(&bip->_transport_state, BT_BIP_TRANSPORT_STATE_CONNECTING);

	return 0;
}

int bt_bip_l2cap_register(struct bt_bip_l2cap_server *server)
{
	if (server == NULL || server->accept == NULL) {
		return -EINVAL;
	}

	server->server.accept = bip_l2cap_accept;

	return bt_goep_transport_l2cap_server_register(&server->server);
}

int bt_bip_l2cap_connect(struct bt_conn *conn, struct bt_bip *bip, uint16_t psm)
{
	int err;

	if (conn == NULL || bip == NULL || psm == 0) {
		return -EINVAL;
	}

	if (atomic_get(&bip->_transport_state) != BT_BIP_TRANSPORT_STATE_DISCONNECTED) {
		return -EINPROGRESS;
	}

	bip->role = BT_BIP_ROLE_INITIATOR;
	bip->goep.transport_ops = &bip_l2cap_ops;
	atomic_set(&bip->_transport_state, BT_BIP_TRANSPORT_STATE_CONNECTING);

	err = bt_goep_transport_l2cap_connect(conn, &bip->goep, psm);
	if (err != 0) {
		atomic_set(&bip->_transport_state, BT_BIP_TRANSPORT_STATE_DISCONNECTED);
	}

	return err;
}

int bt_bip_l2cap_disconnect(struct bt_bip *bip)
{
	int err;

	if (bip == NULL) {
		return -EINVAL;
	}

	if (atomic_get(&bip->_transport_state) != BT_BIP_TRANSPORT_STATE_CONNECTED) {
		return -EINPROGRESS;
	}

	atomic_set(&bip->_transport_state, BT_BIP_TRANSPORT_STATE_DISCONNECTING);

	err = bt_goep_transport_l2cap_disconnect(&bip->goep);
	if (err != 0) {
		atomic_set(&bip->_transport_state, BT_BIP_TRANSPORT_STATE_CONNECTED);
	}

	return err;
}

int bt_bip_set_supported_capabilities(struct bt_bip *bip, uint8_t capabilities)
{
	if (bip == NULL) {
		return -EINVAL;
	}

	if (bip->role != BT_BIP_ROLE_INITIATOR) {
		LOG_ERR("Invalid role %u", bip->role);
		return -EINVAL;
	}

	bip->_supp_caps = capabilities;
	return 0;
}

int bt_bip_set_supported_features(struct bt_bip *bip, uint16_t features)
{
	if (bip == NULL) {
		return -EINVAL;
	}

	if (bip->role != BT_BIP_ROLE_INITIATOR) {
		LOG_ERR("Invalid role %u", bip->role);
		return -EINVAL;
	}

	bip->_supp_feats = features;
	return 0;
}

int bt_bip_set_supported_functions(struct bt_bip *bip, uint32_t functions)
{
	if (bip == NULL) {
		return -EINVAL;
	}

	if (bip->role != BT_BIP_ROLE_INITIATOR) {
		LOG_ERR("Invalid role %u", bip->role);
		return -EINVAL;
	}

	bip->_supp_funcs = functions;
	return 0;
}

static const struct bt_uuid_128 *image_push = BT_BIP_UUID_IMAGE_PUSH;
static const struct bt_uuid_128 *image_pull = BT_BIP_UUID_IMAGE_PULL;
static const struct bt_uuid_128 *image_print = BT_BIP_UUID_IMAGE_PRINT;
static const struct bt_uuid_128 *auto_archive = BT_BIP_UUID_AUTO_ARCHIVE;
static const struct bt_uuid_128 *remote_camera = BT_BIP_UUID_REMOTE_CAMERA;
static const struct bt_uuid_128 *remote_display = BT_BIP_UUID_REMOTE_DISPLAY;
static const struct bt_uuid_128 *referenced_obj = BT_BIP_UUID_REFERENCED_OBJ;
static const struct bt_uuid_128 *archived_obj = BT_BIP_UUID_ARCHIVED_OBJ;

static const struct bt_uuid_128 *btp_get_uuid_from_type(enum bt_bip_conn_type type)
{
	switch (type) {
	case BT_BIP_PRIM_CONN_TYPE_IMAGE_PUSH:
		return image_push;
	case BT_BIP_PRIM_CONN_TYPE_IMAGE_PULL:
		return image_pull;
	case BT_BIP_PRIM_CONN_TYPE_ADVANCED_IMAGE_PRINTING:
		return image_print;
	case BT_BIP_PRIM_CONN_TYPE_AUTO_ARCHIVE:
		return auto_archive;
	case BT_BIP_PRIM_CONN_TYPE_REMOTE_CAMERA:
		return remote_camera;
	case BT_BIP_PRIM_CONN_TYPE_REMOTE_DISPLAY:
		return remote_display;
	case BT_BIP_2ND_CONN_TYPE_REFERENCED_OBJECTS:
		return referenced_obj;
	case BT_BIP_2ND_CONN_TYPE_ARCHIVED_OBJECTS:
		return archived_obj;
	}

	return NULL;
}

static void bip_client_connect(struct bt_obex_client *client, uint8_t rsp_code, uint8_t version,
			       uint16_t mopl, struct net_buf *buf)
{
	struct bt_bip_client *c = CONTAINER_OF(client, struct bt_bip_client, _client);

	if (rsp_code == BT_OBEX_RSP_CODE_SUCCESS) {
		atomic_set(&c->_state, BT_BIP_STATE_CONNECTED);
		bt_obex_get_header_conn_id(buf, &c->_conn_id);
	} else {
		atomic_set(&c->_state, BT_BIP_STATE_DISCONNECTED);
		sys_slist_find_and_remove(&c->_bip->_clients, &c->_node);
	}

	if (c->_cb->connect != NULL) {
		c->_cb->connect(c, rsp_code, version, mopl, buf);
	}
}

static void bip_clinet_clear_pending_request(struct bt_bip_client *client)
{
	client->_rsp_cb = NULL;
	client->_req_type = NULL;
}

static void bip_client_disconnect(struct bt_obex_client *client, uint8_t rsp_code,
				  struct net_buf *buf)
{
	struct bt_bip_client *c = CONTAINER_OF(client, struct bt_bip_client, _client);

	if (rsp_code == BT_OBEX_RSP_CODE_SUCCESS) {
		atomic_set(&c->_state, BT_BIP_STATE_DISCONNECTED);

		if (!is_bip_primary_connect(c->_type)) {
			if (c->_primary_server != NULL) {
				c->_primary_server->_secondary_client = NULL;
			}
		} else {
			if (c->_secondary_server != NULL) {
				LOG_WRN("Secondary connect should be disconnected");
				/* Consider removing transport */
			}
		}

		sys_slist_find_and_remove(&c->_bip->_clients, &c->_node);

		bip_clinet_clear_pending_request(c);
	} else {
		atomic_set(&c->_state, BT_BIP_STATE_CONNECTED);
	}

	if (c->_cb->disconnect != NULL) {
		c->_cb->disconnect(c, rsp_code, buf);
	}
}

static void bip_client_put(struct bt_obex_client *client, uint8_t rsp_code, struct net_buf *buf)
{
	struct bt_bip_client *c = CONTAINER_OF(client, struct bt_bip_client, _client);

	if (c->_rsp_cb != NULL) {
		c->_rsp_cb(c, rsp_code, buf);
	} else {
		LOG_WRN("No cb for rsp");
	}

	if (rsp_code != BT_OBEX_RSP_CODE_CONTINUE) {
		bip_clinet_clear_pending_request(c);
	}
}

static void bip_client_get(struct bt_obex_client *client, uint8_t rsp_code, struct net_buf *buf)
{
	struct bt_bip_client *c = CONTAINER_OF(client, struct bt_bip_client, _client);

	if (c->_rsp_cb != NULL) {
		c->_rsp_cb(c, rsp_code, buf);
	} else {
		LOG_WRN("No cb for rsp");
	}

	if (rsp_code != BT_OBEX_RSP_CODE_CONTINUE) {
		bip_clinet_clear_pending_request(c);
	}
}

static void bip_client_abort(struct bt_obex_client *client, uint8_t rsp_code, struct net_buf *buf)
{
	struct bt_bip_client *c = CONTAINER_OF(client, struct bt_bip_client, _client);
	int err;

	if (c->_cb->abort != NULL) {
		c->_cb->abort(c, rsp_code, buf);
	}

	if (rsp_code == BT_OBEX_RSP_CODE_SUCCESS) {
		bip_clinet_clear_pending_request(c);
		return;
	}

	err = bt_bip_disconnect(c, NULL);
	if (err != 0) {
		LOG_ERR("Failed to send BIP disconnect");
	}
}

static const struct bt_obex_client_ops bip_client_ops = {
	.connect = bip_client_connect,
	.disconnect = bip_client_disconnect,
	.put = bip_client_put,
	.get = bip_client_get,
	.abort = bip_client_abort,
	.setpath = NULL,
	.action = NULL,
};

static void bip_server_connect(struct bt_obex_server *server, uint8_t version, uint16_t mopl,
			       struct net_buf *buf)
{
	struct bt_bip_server *s = CONTAINER_OF(server, struct bt_bip_server, _server);

	if (!is_bip_primary_connect(s->_type) && s->_primary_client != NULL &&
	    atomic_get(&s->_primary_client->_state) != BT_BIP_STATE_CONNECTED) {
		LOG_ERR("Primary conn needs to be connected firstly");
		bt_bip_connect_rsp(s, BT_OBEX_RSP_CODE_NOT_ALLOW, NULL);
		return;
	}

	atomic_set(&s->_state, BT_BIP_STATE_CONNECTING);

	if (s->_cb->connect != NULL) {
		s->_cb->connect(s, version, mopl, buf);
	}
}

static void bip_server_disconnect(struct bt_obex_server *server, struct net_buf *buf)
{
	struct bt_bip_server *s = CONTAINER_OF(server, struct bt_bip_server, _server);

	if (is_bip_primary_connect(s->_type)) {
		if (s->_secondary_client != NULL) {
			bt_bip_disconnect(s->_secondary_client, NULL);
		}
	} else {
		if (s->_primary_client != NULL) {
			s->_primary_client->_secondary_server = NULL;
		}
	}

	atomic_set(&s->_state, BT_BIP_STATE_DISCONNECTING);

	if (s->_cb->disconnect != NULL) {
		s->_cb->disconnect(s, buf);
	}
}

struct bip_required_hdr {
	uint8_t count;
	const uint8_t *hdrs;
};

#define BIP_REQUIRED_HDR(_count, _hdrs)                                                            \
	{                                                                                          \
		.count = (_count), .hdrs = (const uint8_t *)(_hdrs),                               \
	}

#define _BIP_REQUIRED_HDR_LIST(...)                                                                \
	BIP_REQUIRED_HDR(sizeof((uint8_t[]){__VA_ARGS__}), ((uint8_t[]){__VA_ARGS__}))

#define BIP_REQUIRED_HDR_LIST(...) _BIP_REQUIRED_HDR_LIST(__VA_ARGS__)

struct bip_function {
	const char *type;
	bool op_get;
	uint8_t func_bit;
	uint32_t supported_features;
	uint32_t required_appl_param_tag_id;
	struct bip_required_hdr hdr;
	bt_bip_server_cb_t (*get_server_cb)(struct bt_bip_server *server);
	bt_bip_client_cb_t (*get_client_cb)(struct bt_bip_client *client);
};

#define IMAGE_PUSH     BIT(BT_BIP_PRIM_CONN_TYPE_IMAGE_PUSH)
#define IMAGE_PULL     BIT(BT_BIP_PRIM_CONN_TYPE_IMAGE_PULL)
#define IMAGE_PRINT    BIT(BT_BIP_PRIM_CONN_TYPE_ADVANCED_IMAGE_PRINTING)
#define AUTO_ARCHIVE   BIT(BT_BIP_PRIM_CONN_TYPE_AUTO_ARCHIVE)
#define REMOTE_CAMERA  BIT(BT_BIP_PRIM_CONN_TYPE_REMOTE_CAMERA)
#define REMOTE_DISPLAY BIT(BT_BIP_PRIM_CONN_TYPE_REMOTE_DISPLAY)
#define REFERENCED_OBJ BIT(BT_BIP_2ND_CONN_TYPE_REFERENCED_OBJECTS)
#define ARCHIVED_OBJ   BIT(BT_BIP_2ND_CONN_TYPE_ARCHIVED_OBJECTS)

#define AP_RETURNED_HANDLES          BIT(BT_BIP_APPL_PARAM_TAG_ID_RETURNED_HANDLES)
#define AP_LIST_START_OFFSET         BIT(BT_BIP_APPL_PARAM_TAG_ID_LIST_START_OFFSET)
#define AP_LATEST_CAPTURED_IMAGES    BIT(BT_BIP_APPL_PARAM_TAG_ID_LATEST_CAPTURED_IMAGES)
#define AP_PARTIAL_FILE_LEN          BIT(BT_BIP_APPL_PARAM_TAG_ID_PARTIAL_FILE_LEN)
#define AP_PARTIAL_FILE_START_OFFSET BIT(BT_BIP_APPL_PARAM_TAG_ID_PARTIAL_FILE_START_OFFSET)
#define AP_TOTAL_FILE_SIZE           BIT(BT_BIP_APPL_PARAM_TAG_ID_TOTAL_FILE_SIZE)
#define AP_END_FLAG                  BIT(BT_BIP_APPL_PARAM_TAG_ID_END_FLAG)
#define AP_REMOTE_DISPLAY            BIT(BT_BIP_APPL_PARAM_TAG_ID_REMOTE_DISPLAY)
#define AP_SERVICE_ID                BIT(BT_BIP_APPL_PARAM_TAG_ID_SERVICE_ID)
#define AP_STORE_FLAG                BIT(BT_BIP_APPL_PARAM_TAG_ID_STORE_FLAG)

#define GET_CAPS_FUNC_BIT      BT_BIP_SUPP_FUNC_GET_CAPS
#define GET_CAPS_SUPPORT_FEATS IMAGE_PUSH | IMAGE_PULL | IMAGE_PRINT | REMOTE_DISPLAY | ARCHIVED_OBJ
#define GET_CAPS_REQUIRED_HDR  BT_OBEX_HEADER_ID_CONN_ID, BT_OBEX_HEADER_ID_TYPE
#define GET_CAPS_AP            0

#define GET_IMAGE_LIST_FUNC_BIT      BT_BIP_SUPP_FUNC_GET_IMAGE_LIST
#define GET_IMAGE_LIST_SUPPORT_FEATS IMAGE_PULL | REMOTE_DISPLAY | ARCHIVED_OBJ
#define GET_IMAGE_LIST_REQUIRED_HDR                                                                \
	BT_OBEX_HEADER_ID_CONN_ID, BT_OBEX_HEADER_ID_TYPE, BT_OBEX_HEADER_ID_APP_PARAM,            \
		BT_BIP_HEADER_ID_IMG_DESC
#define GET_IMAGE_LIST_AP AP_RETURNED_HANDLES | AP_LIST_START_OFFSET | AP_LATEST_CAPTURED_IMAGES

#define GET_IMAGE_PROPERTIES_FUNC_BIT      BT_BIP_SUPP_FUNC_GET_IMAGE_PROPERTIES
#define GET_IMAGE_PROPERTIES_SUPPORT_FEATS IMAGE_PULL | REMOTE_CAMERA | ARCHIVED_OBJ
#define GET_IMAGE_PROPERTIES_REQUIRED_HDR                                                          \
	BT_OBEX_HEADER_ID_CONN_ID, BT_OBEX_HEADER_ID_TYPE, BT_BIP_HEADER_ID_IMG_HANDLE
#define GET_IMAGE_PROPERTIES_AP 0

#define GET_IMAGE_FUNC_BIT      BT_BIP_SUPP_FUNC_GET_IMAGE
#define GET_IMAGE_SUPPORT_FEATS IMAGE_PULL | REMOTE_CAMERA | ARCHIVED_OBJ
#define GET_IMAGE_REQUIRED_HDR                                                                     \
	BT_OBEX_HEADER_ID_CONN_ID, BT_OBEX_HEADER_ID_TYPE, BT_BIP_HEADER_ID_IMG_DESC,              \
		BT_BIP_HEADER_ID_IMG_HANDLE
#define GET_IMAGE_AP 0

#define GET_LINKED_THUMBNAIL_FUNC_BIT      BT_BIP_SUPP_FUNC_GET_LINKED_THUMBNAIL
#define GET_LINKED_THUMBNAIL_SUPPORT_FEATS IMAGE_PULL | REMOTE_CAMERA | ARCHIVED_OBJ
#define GET_LINKED_THUMBNAIL_REQUIRED_HDR                                                          \
	BT_OBEX_HEADER_ID_CONN_ID, BT_OBEX_HEADER_ID_TYPE, BT_BIP_HEADER_ID_IMG_HANDLE
#define GET_LINKED_THUMBNAIL_AP 0

#define GET_LINKED_ATTACHMENT_FUNC_BIT      BT_BIP_SUPP_FUNC_GET_LINKED_ATTACHMENT
#define GET_LINKED_ATTACHMENT_SUPPORT_FEATS IMAGE_PULL | ARCHIVED_OBJ
#define GET_LINKED_ATTACHMENT_REQUIRED_HDR                                                         \
	BT_OBEX_HEADER_ID_CONN_ID, BT_OBEX_HEADER_ID_TYPE, BT_BIP_HEADER_ID_IMG_HANDLE,            \
		BT_OBEX_HEADER_ID_NAME
#define GET_LINKED_ATTACHMENT_AP 0

#define GET_PARTIAL_IMAGE_FUNC_BIT      BT_BIP_SUPP_FUNC_GET_PARTIAL_IMAGE
#define GET_PARTIAL_IMAGE_SUPPORT_FEATS REFERENCED_OBJ
#define GET_PARTIAL_IMAGE_REQUIRED_HDR                                                             \
	BT_OBEX_HEADER_ID_CONN_ID, BT_OBEX_HEADER_ID_TYPE, BT_OBEX_HEADER_ID_NAME,                 \
		BT_OBEX_HEADER_ID_APP_PARAM
#define GET_PARTIAL_IMAGE_AP AP_PARTIAL_FILE_LEN | AP_PARTIAL_FILE_START_OFFSET

#define GET_MONITORING_IMAGE_FUNC_BIT      BT_BIP_SUPP_FUNC_GET_MONITORING_IMAGE
#define GET_MONITORING_IMAGE_SUPPORT_FEATS REMOTE_CAMERA
#define GET_MONITORING_IMAGE_REQUIRED_HDR                                                          \
	BT_OBEX_HEADER_ID_CONN_ID, BT_OBEX_HEADER_ID_TYPE, BT_OBEX_HEADER_ID_APP_PARAM
#define GET_MONITORING_IMAGE_AP AP_STORE_FLAG

#define GET_STATUS_FUNC_BIT      BT_BIP_SUPP_FUNC_GET_STATUS
#define GET_STATUS_SUPPORT_FEATS IMAGE_PRINT | AUTO_ARCHIVE
#define GET_STATUS_REQUIRED_HDR  BT_OBEX_HEADER_ID_CONN_ID, BT_OBEX_HEADER_ID_TYPE
#define GET_STATUS_AP            0

#define PUT_IMAGE_FUNC_BIT      BT_BIP_SUPP_FUNC_PUT_IMAGE
#define PUT_IMAGE_SUPPORT_FEATS IMAGE_PUSH | REMOTE_DISPLAY
#define PUT_IMAGE_REQUIRED_HDR                                                                     \
	BT_OBEX_HEADER_ID_CONN_ID, BT_OBEX_HEADER_ID_TYPE, BT_OBEX_HEADER_ID_NAME,                 \
		BT_BIP_HEADER_ID_IMG_DESC
#define PUT_IMAGE_AP 0

#define PUT_LINKED_THUMBNAIL_FUNC_BIT      BT_BIP_SUPP_FUNC_PUT_LINKED_THUMBNAIL
#define PUT_LINKED_THUMBNAIL_SUPPORT_FEATS IMAGE_PUSH | REMOTE_DISPLAY
#define PUT_LINKED_THUMBNAIL_REQUIRED_HDR                                                          \
	BT_OBEX_HEADER_ID_CONN_ID, BT_OBEX_HEADER_ID_TYPE, BT_BIP_HEADER_ID_IMG_HANDLE
#define PUT_LINKED_THUMBNAIL_AP 0

#define PUT_LINKED_ATTACHMENT_FUNC_BIT      BT_BIP_SUPP_FUNC_PUT_LINKED_ATTACHMENT
#define PUT_LINKED_ATTACHMENT_SUPPORT_FEATS IMAGE_PUSH | REMOTE_DISPLAY
#define PUT_LINKED_ATTACHMENT_REQUIRED_HDR                                                         \
	BT_OBEX_HEADER_ID_CONN_ID, BT_OBEX_HEADER_ID_TYPE, BT_BIP_HEADER_ID_IMG_HANDLE
#define PUT_LINKED_ATTACHMENT_AP 0

#define REMOTE_DISPLAY_FUNC_BIT      BT_BIP_SUPP_FUNC_REMOTE_DISPLAY
#define REMOTE_DISPLAY_SUPPORT_FEATS REMOTE_DISPLAY
#define REMOTE_DISPLAY_REQUIRED_HDR                                                                \
	BT_OBEX_HEADER_ID_CONN_ID, BT_OBEX_HEADER_ID_TYPE, BT_BIP_HEADER_ID_IMG_HANDLE,            \
		BT_OBEX_HEADER_ID_APP_PARAM
#define REMOTE_DISPLAY_AP AP_REMOTE_DISPLAY

#define DELETE_IMAGE_FUNC_BIT      BT_BIP_SUPP_FUNC_REMOTE_DISPLAY
#define DELETE_IMAGE_SUPPORT_FEATS IMAGE_PULL | ARCHIVED_OBJ
#define DELETE_IMAGE_REQUIRED_HDR                                                                  \
	BT_OBEX_HEADER_ID_CONN_ID, BT_OBEX_HEADER_ID_TYPE, BT_BIP_HEADER_ID_IMG_HANDLE
#define DELETE_IMAGE_AP 0

#define START_PRINT_FUNC_BIT      BT_BIP_SUPP_FUNC_START_PRINT
#define START_PRINT_SUPPORT_FEATS IMAGE_PRINT
#define START_PRINT_REQUIRED_HDR                                                                   \
	BT_OBEX_HEADER_ID_CONN_ID, BT_OBEX_HEADER_ID_TYPE, BT_OBEX_HEADER_ID_APP_PARAM
#define START_PRINT_AP AP_SERVICE_ID

#define START_ARCHIVE_FUNC_BIT      BT_BIP_SUPP_FUNC_START_ARCHIVE
#define START_ARCHIVE_SUPPORT_FEATS AUTO_ARCHIVE
#define START_ARCHIVE_REQUIRED_HDR                                                                 \
	BT_OBEX_HEADER_ID_CONN_ID, BT_OBEX_HEADER_ID_TYPE, BT_OBEX_HEADER_ID_APP_PARAM
#define START_ARCHIVE_AP AP_SERVICE_ID

static bt_bip_server_cb_t bip_server_get_cb_get_caps(struct bt_bip_server *server)
{
	return server->_cb->get_caps;
}

static bt_bip_server_cb_t bip_server_get_cb_get_image_list(struct bt_bip_server *server)
{
	return server->_cb->get_image_list;
}

static bt_bip_server_cb_t bip_server_get_cb_get_image_properties(struct bt_bip_server *server)
{
	return server->_cb->get_image_properties;
}

static bt_bip_server_cb_t bip_server_get_cb_get_image(struct bt_bip_server *server)
{
	return server->_cb->get_image;
}

static bt_bip_server_cb_t bip_server_get_cb_get_linked_thumbnail(struct bt_bip_server *server)
{
	return server->_cb->get_linked_thumbnail;
}

static bt_bip_server_cb_t bip_server_get_cb_get_linked_attachment(struct bt_bip_server *server)
{
	return server->_cb->get_linked_attachment;
}

static bt_bip_server_cb_t bip_server_get_cb_get_partial_image(struct bt_bip_server *server)
{
	return server->_cb->get_partial_image;
}

static bt_bip_server_cb_t bip_server_get_cb_get_monitoring_image(struct bt_bip_server *server)
{
	return server->_cb->get_monitoring_image;
}

static bt_bip_server_cb_t bip_server_get_cb_get_status(struct bt_bip_server *server)
{
	return server->_cb->get_status;
}

static bt_bip_server_cb_t bip_server_get_cb_put_image(struct bt_bip_server *server)
{
	return server->_cb->put_image;
}

static bt_bip_server_cb_t bip_server_get_cb_put_linked_thumbnail(struct bt_bip_server *server)
{
	return server->_cb->put_linked_thumbnail;
}

static bt_bip_server_cb_t bip_server_get_cb_put_linked_attachment(struct bt_bip_server *server)
{
	return server->_cb->put_linked_attachment;
}

static bt_bip_server_cb_t bip_server_get_cb_remote_display(struct bt_bip_server *server)
{
	return server->_cb->remote_display;
}

static bt_bip_server_cb_t bip_server_get_cb_delete_image(struct bt_bip_server *server)
{
	return server->_cb->delete_image;
}

static bt_bip_server_cb_t bip_server_get_cb_start_print(struct bt_bip_server *server)
{
	return server->_cb->start_print;
}

static bt_bip_server_cb_t bip_server_get_cb_start_archive(struct bt_bip_server *server)
{
	return server->_cb->start_archive;
}

static bt_bip_client_cb_t bip_client_get_cb_get_caps(struct bt_bip_client *client)
{
	return client->_cb->get_caps;
}

static bt_bip_client_cb_t bip_client_get_cb_get_image_list(struct bt_bip_client *client)
{
	return client->_cb->get_image_list;
}

static bt_bip_client_cb_t bip_client_get_cb_get_image_properties(struct bt_bip_client *client)
{
	return client->_cb->get_image_properties;
}

static bt_bip_client_cb_t bip_client_get_cb_get_image(struct bt_bip_client *client)
{
	return client->_cb->get_image;
}

static bt_bip_client_cb_t bip_client_get_cb_get_linked_thumbnail(struct bt_bip_client *client)
{
	return client->_cb->get_linked_thumbnail;
}

static bt_bip_client_cb_t bip_client_get_cb_get_linked_attachment(struct bt_bip_client *client)
{
	return client->_cb->get_linked_attachment;
}

static bt_bip_client_cb_t bip_client_get_cb_get_partial_image(struct bt_bip_client *client)
{
	return client->_cb->get_partial_image;
}

static bt_bip_client_cb_t bip_client_get_cb_get_monitoring_image(struct bt_bip_client *client)
{
	return client->_cb->get_monitoring_image;
}

static bt_bip_client_cb_t bip_client_get_cb_get_status(struct bt_bip_client *client)
{
	return client->_cb->get_status;
}

static bt_bip_client_cb_t bip_client_get_cb_put_image(struct bt_bip_client *client)
{
	return client->_cb->put_image;
}

static bt_bip_client_cb_t bip_client_get_cb_put_linked_thumbnail(struct bt_bip_client *client)
{
	return client->_cb->put_linked_thumbnail;
}

static bt_bip_client_cb_t bip_client_get_cb_put_linked_attachment(struct bt_bip_client *client)
{
	return client->_cb->put_linked_attachment;
}

static bt_bip_client_cb_t bip_client_get_cb_remote_display(struct bt_bip_client *client)
{
	return client->_cb->remote_display;
}

static bt_bip_client_cb_t bip_client_get_cb_delete_image(struct bt_bip_client *client)
{
	return client->_cb->delete_image;
}

static bt_bip_client_cb_t bip_client_get_cb_start_print(struct bt_bip_client *client)
{
	return client->_cb->start_print;
}

static bt_bip_client_cb_t bip_client_get_cb_start_archive(struct bt_bip_client *client)
{
	return client->_cb->start_archive;
}

static const struct bip_function bip_functions[] = {
	{BT_BIP_HDR_TYPE_GET_CAPS, true, GET_CAPS_FUNC_BIT, GET_CAPS_SUPPORT_FEATS, GET_CAPS_AP,
	 BIP_REQUIRED_HDR_LIST(GET_CAPS_REQUIRED_HDR), bip_server_get_cb_get_caps,
	 bip_client_get_cb_get_caps},
	{BT_BIP_HDR_TYPE_GET_IMAGE_LIST, true, GET_IMAGE_LIST_FUNC_BIT,
	 GET_IMAGE_LIST_SUPPORT_FEATS, GET_IMAGE_LIST_AP,
	 BIP_REQUIRED_HDR_LIST(GET_IMAGE_LIST_REQUIRED_HDR), bip_server_get_cb_get_image_list,
	 bip_client_get_cb_get_image_list},
	{BT_BIP_HDR_TYPE_GET_IMAGE_PROPERTIES, true, GET_IMAGE_PROPERTIES_FUNC_BIT,
	 GET_IMAGE_PROPERTIES_SUPPORT_FEATS, GET_IMAGE_PROPERTIES_AP,
	 BIP_REQUIRED_HDR_LIST(GET_IMAGE_PROPERTIES_REQUIRED_HDR),
	 bip_server_get_cb_get_image_properties, bip_client_get_cb_get_image_properties},
	{BT_BIP_HDR_TYPE_GET_IMAGE, true, GET_IMAGE_FUNC_BIT, GET_IMAGE_SUPPORT_FEATS, GET_IMAGE_AP,
	 BIP_REQUIRED_HDR_LIST(GET_IMAGE_REQUIRED_HDR), bip_server_get_cb_get_image,
	 bip_client_get_cb_get_image},
	{BT_BIP_HDR_TYPE_GET_LINKED_THUMBNAIL, true, GET_LINKED_THUMBNAIL_FUNC_BIT,
	 GET_LINKED_THUMBNAIL_SUPPORT_FEATS, GET_LINKED_THUMBNAIL_AP,
	 BIP_REQUIRED_HDR_LIST(GET_LINKED_THUMBNAIL_REQUIRED_HDR),
	 bip_server_get_cb_get_linked_thumbnail, bip_client_get_cb_get_linked_thumbnail},
	{BT_BIP_HDR_TYPE_GET_LINKED_ATTACHMENT, true, GET_LINKED_ATTACHMENT_FUNC_BIT,
	 GET_LINKED_ATTACHMENT_SUPPORT_FEATS, GET_LINKED_ATTACHMENT_AP,
	 BIP_REQUIRED_HDR_LIST(GET_LINKED_ATTACHMENT_REQUIRED_HDR),
	 bip_server_get_cb_get_linked_attachment, bip_client_get_cb_get_linked_attachment},
	{BT_BIP_HDR_TYPE_GET_PARTIAL_IMAGE, true, GET_PARTIAL_IMAGE_FUNC_BIT,
	 GET_PARTIAL_IMAGE_SUPPORT_FEATS, GET_PARTIAL_IMAGE_AP,
	 BIP_REQUIRED_HDR_LIST(GET_PARTIAL_IMAGE_REQUIRED_HDR), bip_server_get_cb_get_partial_image,
	 bip_client_get_cb_get_partial_image},
	{BT_BIP_HDR_TYPE_GET_MONITORING_IMAGE, true, GET_MONITORING_IMAGE_FUNC_BIT,
	 GET_MONITORING_IMAGE_SUPPORT_FEATS, GET_MONITORING_IMAGE_AP,
	 BIP_REQUIRED_HDR_LIST(GET_MONITORING_IMAGE_REQUIRED_HDR),
	 bip_server_get_cb_get_monitoring_image, bip_client_get_cb_get_monitoring_image},
	{BT_BIP_HDR_TYPE_GET_STATUS, true, GET_STATUS_FUNC_BIT, GET_STATUS_SUPPORT_FEATS,
	 GET_STATUS_AP, BIP_REQUIRED_HDR_LIST(GET_STATUS_REQUIRED_HDR),
	 bip_server_get_cb_get_status, bip_client_get_cb_get_status},
	{BT_BIP_HDR_TYPE_PUT_IMAGE, false, PUT_IMAGE_FUNC_BIT, PUT_IMAGE_SUPPORT_FEATS,
	 PUT_IMAGE_AP, BIP_REQUIRED_HDR_LIST(PUT_IMAGE_REQUIRED_HDR), bip_server_get_cb_put_image,
	 bip_client_get_cb_put_image},
	{BT_BIP_HDR_TYPE_PUT_LINKED_THUMBNAIL, false, PUT_LINKED_THUMBNAIL_FUNC_BIT,
	 PUT_LINKED_THUMBNAIL_SUPPORT_FEATS, PUT_LINKED_THUMBNAIL_AP,
	 BIP_REQUIRED_HDR_LIST(PUT_LINKED_THUMBNAIL_REQUIRED_HDR),
	 bip_server_get_cb_put_linked_thumbnail, bip_client_get_cb_put_linked_thumbnail},
	{BT_BIP_HDR_TYPE_PUT_LINKED_ATTACHMENT, false, PUT_LINKED_ATTACHMENT_FUNC_BIT,
	 PUT_LINKED_ATTACHMENT_SUPPORT_FEATS, PUT_LINKED_ATTACHMENT_AP,
	 BIP_REQUIRED_HDR_LIST(PUT_LINKED_ATTACHMENT_REQUIRED_HDR),
	 bip_server_get_cb_put_linked_attachment, bip_client_get_cb_put_linked_attachment},
	{BT_BIP_HDR_TYPE_REMOTE_DISPLAY, false, REMOTE_DISPLAY_FUNC_BIT,
	 REMOTE_DISPLAY_SUPPORT_FEATS, REMOTE_DISPLAY_AP,
	 BIP_REQUIRED_HDR_LIST(REMOTE_DISPLAY_REQUIRED_HDR), bip_server_get_cb_remote_display,
	 bip_client_get_cb_remote_display},
	{BT_BIP_HDR_TYPE_DELETE_IMAGE, false, DELETE_IMAGE_FUNC_BIT, DELETE_IMAGE_SUPPORT_FEATS,
	 DELETE_IMAGE_AP, BIP_REQUIRED_HDR_LIST(DELETE_IMAGE_REQUIRED_HDR),
	 bip_server_get_cb_delete_image, bip_client_get_cb_delete_image},
	{BT_BIP_HDR_TYPE_START_PRINT, false, START_PRINT_FUNC_BIT, START_PRINT_SUPPORT_FEATS,
	 START_PRINT_AP, BIP_REQUIRED_HDR_LIST(START_PRINT_REQUIRED_HDR),
	 bip_server_get_cb_start_print, bip_client_get_cb_start_print},
	{BT_BIP_HDR_TYPE_START_ARCHIVE, false, START_ARCHIVE_FUNC_BIT, START_ARCHIVE_SUPPORT_FEATS,
	 START_ARCHIVE_AP, BIP_REQUIRED_HDR_LIST(START_ARCHIVE_REQUIRED_HDR),
	 bip_server_get_cb_start_archive, bip_client_get_cb_start_archive},
};

static bool has_required_hdrs(struct net_buf *buf, const struct bip_required_hdr *hdr)
{
	for (uint8_t index = 0; index < hdr->count; index++) {
		if (!bt_obex_has_header(buf, hdr->hdrs[index])) {
			return false;
		}
	}
	return true;
}

static enum bt_obex_rsp_code bip_server_get_req_cb(struct bt_bip_server *server,
						   struct net_buf *buf, bool is_get,
						   bt_bip_server_cb_t *cb)
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
	ARRAY_FOR_EACH(bip_functions, i) {
		if (server->_optype != NULL && bip_functions[i].type != server->_optype) {
			continue;
		}

		if (strcmp(bip_functions[i].type, type) != 0) {
			continue;
		}

		if (bip_functions[i].op_get != is_get) {
			continue;
		}

		if ((bip_functions[i].supported_features & BIT(server->_type)) == 0) {
			continue;
		}

		if (!has_required_hdrs(buf, &bip_functions[i].hdr)) {
			continue;
		}

		/* Application parameter tag id is not checked. */

		if (bip_functions[i].get_server_cb == NULL) {
			continue;
		}

		*cb = bip_functions[i].get_server_cb(server);
		if (*cb == NULL) {
			continue;
		}

		server->_optype = bip_functions[i].type;
		server->_opcode = is_get ? BT_OBEX_OPCODE_GET : BT_OBEX_OPCODE_PUT;
		break;
	}

	if (*cb == NULL) {
		LOG_WRN("Unsupported request");
		return BT_OBEX_RSP_CODE_NOT_IMPL;
	}

	return BT_OBEX_RSP_CODE_SUCCESS;
}

static void bip_server_put(struct bt_obex_server *server, bool final, struct net_buf *buf)
{
	struct bt_bip_server *s = CONTAINER_OF(server, struct bt_bip_server, _server);
	int err;
	enum bt_obex_rsp_code rsp_code;

	if (s->_optype == NULL || bt_obex_has_header(buf, BT_OBEX_HEADER_ID_TYPE)) {
		rsp_code = bip_server_get_req_cb(s, buf, false, &s->_req_cb);
		if (rsp_code != BT_OBEX_RSP_CODE_SUCCESS) {
			LOG_WRN("Failed to parse req %u", (uint8_t)rsp_code);
			goto failed;
		}
	}

	if (s->_optype == NULL || s->_opcode != BT_OBEX_OPCODE_PUT || s->_req_cb == NULL) {
		rsp_code = BT_OBEX_RSP_CODE_NOT_IMPL;
		LOG_WRN("Invalid request");
		goto failed;
	}

	s->_req_cb(s, final, buf);

	return;

failed:
	s->_optype = NULL;
	s->_opcode = 0;
	s->_req_cb = NULL;
	err = bt_obex_put_rsp(server, rsp_code, NULL);
	if (err != 0) {
		LOG_ERR("Failed to send put rsp %d", err);
	}
}

static void bip_server_get(struct bt_obex_server *server, bool final, struct net_buf *buf)
{
	struct bt_bip_server *s = CONTAINER_OF(server, struct bt_bip_server, _server);
	int err;
	enum bt_obex_rsp_code rsp_code;

	if (s->_optype == NULL || bt_obex_has_header(buf, BT_OBEX_HEADER_ID_TYPE)) {
		rsp_code = bip_server_get_req_cb(s, buf, true, &s->_req_cb);
		if (rsp_code != BT_OBEX_RSP_CODE_SUCCESS) {
			LOG_WRN("Failed to parse req %u", (uint8_t)rsp_code);
			goto failed;
		}
	}

	if (s->_optype == NULL || s->_opcode != BT_OBEX_OPCODE_GET || s->_req_cb == NULL) {
		rsp_code = BT_OBEX_RSP_CODE_NOT_IMPL;
		LOG_WRN("Invalid request");
		goto failed;
	}

	s->_req_cb(s, final, buf);

	return;

failed:
	s->_optype = NULL;
	s->_opcode = 0;
	s->_req_cb = NULL;
	err = bt_obex_get_rsp(server, rsp_code, NULL);
	if (err != 0) {
		LOG_ERR("Failed to send get rsp %d", err);
	}
}

static void bip_server_abort(struct bt_obex_server *server, struct net_buf *buf)
{
	struct bt_bip_server *s = CONTAINER_OF(server, struct bt_bip_server, _server);
	int err;

	if (s->_cb->abort != NULL) {
		s->_cb->abort(s, buf);
		return;
	}

	err = bt_obex_abort_rsp(server, BT_OBEX_RSP_CODE_NOT_IMPL, NULL);
	if (err != 0) {
		LOG_ERR("Failed to send abort rsp %d", err);
	}
}

static const struct bt_obex_server_ops bip_server_ops = {
	.connect = bip_server_connect,
	.disconnect = bip_server_disconnect,
	.put = bip_server_put,
	.get = bip_server_get,
	.abort = bip_server_abort,
	.setpath = NULL,
	.action = NULL,
};

static uint32_t bip_get_connect_id(void)
{
	static uint32_t connect_id;

	connect_id++;

	return connect_id;
}

static int bt_bip_server_register(struct bt_bip *bip, struct bt_bip_server *server,
				  enum bt_bip_conn_type type, const struct bt_uuid_128 *uuid,
				  struct bt_bip_server_cb *cb, struct bt_bip_client *primary_client)
{
	struct bt_bip_server *s, *next;
	int err;

	if (atomic_get(&server->_state) != BT_BIP_STATE_DISCONNECTED) {
		LOG_ERR("Invalid state %u", (uint8_t)atomic_get(&server->_state));
		return -EINVAL;
	}

	if (is_bip_primary_connect(type)) {
		if (bip->role == BT_BIP_ROLE_INITIATOR) {
			LOG_ERR("Invalid role initiator");
			return -EINVAL;
		}

		if (primary_client != NULL) {
			LOG_ERR("primary client should be NULL");
			return -EINVAL;
		}
	} else {
		struct bt_bip *primary_bip;

		if (bip->role == BT_BIP_ROLE_RESPONDER) {
			LOG_ERR("Invalid role responder");
			return -EINVAL;
		}

		if (primary_client == NULL || primary_client->_bip == NULL) {
			LOG_ERR("Invalid primary client");
			return -EINVAL;
		}

		primary_bip = primary_client->_bip;
		if (!sys_slist_find(&primary_bip->_clients, &primary_client->_node, NULL)) {
			LOG_ERR("Primary client %p is not found", primary_client);
			return -EINVAL;
		}
	}

	if (sys_slist_find(&bip->_servers, &server->_node, NULL)) {
		LOG_ERR("Server %p has been registered", server);
		return -EALREADY;
	}

	if (uuid == NULL) {
		uuid = btp_get_uuid_from_type(type);
	}

	__ASSERT(uuid != NULL, "Invalid UUID");

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&bip->_servers, s, next, _node) {
		if (bt_uuid_cmp(&uuid->uuid, &s->_uuid->uuid) == 0) {
			LOG_ERR("UUID has been registered");
			return -EALREADY;
		}
	}

	server->_uuid = uuid;
	server->_cb = cb;
	server->_bip = bip;
	server->_type = type;
	server->_server.ops = &bip_server_ops;
	server->_server.obex = &bip->goep.obex;
	server->_secondary_client = primary_client;
	server->_conn_id = bip_get_connect_id();
	err = bt_obex_server_register(&server->_server, server->_uuid);
	if (err != 0) {
		return err;
	}

	sys_slist_append(&bip->_servers, &server->_node);
	return 0;
}

int bt_bip_primary_server_register(struct bt_bip *bip, struct bt_bip_server *server,
				   enum bt_bip_conn_type type, const struct bt_uuid_128 *uuid,
				   struct bt_bip_server_cb *cb)
{
	return bt_bip_server_register(bip, server, type, uuid, cb, NULL);
}

int bt_bip_secondary_server_register(struct bt_bip *bip, struct bt_bip_server *server,
				     enum bt_bip_conn_type type, const struct bt_uuid_128 *uuid,
				     struct bt_bip_server_cb *cb,
				     struct bt_bip_client *primary_client)
{
	return bt_bip_server_register(bip, server, type, uuid, cb, primary_client);
}

int bt_bip_server_unregister(struct bt_bip_server *server)
{
	int err;

	if (server == NULL || server->_bip == NULL) {
		return -EINVAL;
	}

	if (atomic_get(&server->_state) != BT_BIP_STATE_DISCONNECTED) {
		LOG_ERR("Invalid state %u", (uint8_t)atomic_get(&server->_state));
		return -EINVAL;
	}

	if (!sys_slist_find(&server->_bip->_servers, &server->_node, NULL)) {
		LOG_ERR("Server %p has not found", server);
		return -EALREADY;
	}

	err = bt_obex_server_unregister(&server->_server);
	if (err != 0) {
		LOG_ERR("Failed to unregister %d", err);
		return err;
	}

	sys_slist_find_and_remove(&server->_bip->_servers, &server->_node);
	return 0;
}

static int bip_check_conn_id(uint32_t id, struct net_buf *buf)
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

static int bip_check_who(const struct bt_uuid *uuid, struct net_buf *buf)
{
	uint16_t len;
	const uint8_t *who;
	union bt_obex_uuid obex_uuid;
	int err;

	err = bt_obex_get_header_who(buf, &len, &who);
	if (err != 0) {
		LOG_ERR("Failed to get who %d", err);
		return err;
	}

	err = bt_obex_make_uuid(&obex_uuid, who, len);
	if (err != 0) {
		LOG_ERR("Invalid UUID of who %d", err);
		return err;
	}

	if (bt_uuid_cmp(uuid, &obex_uuid.uuid) != 0) {
		LOG_ERR("WHO is mismatched");
		return -EINVAL;
	}

	return 0;
}

#define BIP_FEAT_IMAGE_PUSH_BITS                                                                   \
	(BIT(BT_BIP_SUPP_FEAT_IMAGE_PUSH) | BIT(BT_BIP_SUPP_FEAT_IMAGE_PUSH_STORE) |               \
	 BIT(BT_BIP_SUPP_FEAT_IMAGE_PUSH_PRINT) | BIT(BT_BIP_SUPP_FEAT_IMAGE_PUSH_DISPLAY))

#define BT_BIP_SUPP_FEAT_IMAGE_PRINT BT_BIP_SUPP_FEAT_ADVANCED_IMAGE_PRINT

#define BIP_FEAT_IMAGE_PUSH(feature)     ((feature) & BIP_FEAT_IMAGE_PUSH_BITS) != 0
#define BIP_FEAT_IMAGE_PULL(feature)     ((feature) & BIT(BT_BIP_SUPP_FEAT_IMAGE_PULL)) != 0
#define BIP_FEAT_IMAGE_PRINT(feature)    ((feature) & BIT(BT_BIP_SUPP_FEAT_IMAGE_PRINT)) != 0
#define BIP_FEAT_AUTO_ARCHIVE(feature)   ((feature) & BIT(BT_BIP_SUPP_FEAT_AUTO_ARCHIVE)) != 0
#define BIP_FEAT_REMOTE_CAMERA(feature)  ((feature) & BIT(BT_BIP_SUPP_FEAT_REMOTE_CAMERA)) != 0
#define BIP_FEAT_REMOTE_DISPLAY(feature) ((feature) & BIT(BT_BIP_SUPP_FEAT_REMOTE_DISPLAY)) != 0

static int bip_check_features(struct bt_bip *bip, enum bt_bip_conn_type type)
{
	switch (type) {
	case BT_BIP_PRIM_CONN_TYPE_IMAGE_PUSH:
		if (BIP_FEAT_IMAGE_PUSH(bip->_supp_feats)) {
			return 0;
		}
		break;
	case BT_BIP_PRIM_CONN_TYPE_IMAGE_PULL:
		if (BIP_FEAT_IMAGE_PULL(bip->_supp_feats)) {
			return 0;
		}
		break;
	case BT_BIP_PRIM_CONN_TYPE_ADVANCED_IMAGE_PRINTING:
		if (BIP_FEAT_IMAGE_PRINT(bip->_supp_feats)) {
			return 0;
		}
		break;
	case BT_BIP_PRIM_CONN_TYPE_AUTO_ARCHIVE:
		if (BIP_FEAT_AUTO_ARCHIVE(bip->_supp_feats)) {
			return 0;
		}
		break;
	case BT_BIP_PRIM_CONN_TYPE_REMOTE_CAMERA:
		if (BIP_FEAT_REMOTE_CAMERA(bip->_supp_feats)) {
			return 0;
		}
		break;
	case BT_BIP_PRIM_CONN_TYPE_REMOTE_DISPLAY:
		if (BIP_FEAT_REMOTE_DISPLAY(bip->_supp_feats)) {
			return 0;
		}
		break;
	case BT_BIP_2ND_CONN_TYPE_REFERENCED_OBJECTS:
		break;
	case BT_BIP_2ND_CONN_TYPE_ARCHIVED_OBJECTS:
		break;
	}
	return -ENOTSUP;
}

static int bt_bip_client_connect(struct bt_bip *bip, struct bt_bip_client *client,
				 enum bt_bip_conn_type type, struct bt_bip_client_cb *cb,
				 struct net_buf *buf, struct bt_bip_server *primary_server)
{
	struct bt_bip_client *c, *next;
	const struct bt_uuid_128 *uuid;
	union bt_obex_uuid obex_uuid;
	uint8_t val[BT_UUID_SIZE_128];
	int err;
	bool allocated = false;

	if (atomic_get(&client->_state) != BT_BIP_STATE_DISCONNECTED) {
		LOG_ERR("Invalid state %u", (uint8_t)atomic_get(&client->_state));
		return -EINVAL;
	}

	if (is_bip_primary_connect(type)) {
		if (bip->role == BT_BIP_ROLE_RESPONDER) {
			LOG_ERR("Invalid role responder");
			return -EINVAL;
		}

		if (primary_server != NULL) {
			LOG_ERR("primary server should be NULL");
			return -EINVAL;
		}

		err = bip_check_features(bip, type);
		if (err != 0) {
			LOG_ERR("Unsupported features for connection type %d", type);
			return err;
		}
	} else {
		struct bt_bip *primary_bip;

		if (bip->role == BT_BIP_ROLE_INITIATOR) {
			LOG_ERR("Invalid role initiator");
			return -EINVAL;
		}

		if (primary_server == NULL || primary_server->_bip == NULL) {
			LOG_ERR("Invalid primary client");
			return -EINVAL;
		}

		if (type == BT_BIP_2ND_CONN_TYPE_REFERENCED_OBJECTS &&
		    primary_server->_type != BT_BIP_PRIM_CONN_TYPE_ADVANCED_IMAGE_PRINTING) {
			LOG_ERR("Invalid primary connection type for referenced objects");
			return -EINVAL;
		}

		if (type == BT_BIP_2ND_CONN_TYPE_ARCHIVED_OBJECTS &&
		    primary_server->_type != BT_BIP_PRIM_CONN_TYPE_AUTO_ARCHIVE) {
			LOG_ERR("Invalid primary connection type for referenced objects");
			return -EINVAL;
		}

		if (atomic_get(&primary_server->_state) != BT_BIP_STATE_CONNECTED) {
			LOG_ERR("Invalid primary server state %u",
				(uint8_t)atomic_get(&primary_server->_state));
			return -EINVAL;
		}

		primary_bip = primary_server->_bip;
		if (!sys_slist_find(&primary_bip->_servers, &primary_server->_node, NULL)) {
			LOG_ERR("Primary server %p is not found", primary_server);
			return -EINVAL;
		}
	}

	if (sys_slist_find(&bip->_clients, &client->_node, NULL)) {
		LOG_ERR("Client %p is not idle", client);
		return -EALREADY;
	}

	if (!bt_obex_has_header(buf, BT_OBEX_HEADER_ID_TARGET)) {
		uuid = btp_get_uuid_from_type(type);
	} else {
		uint16_t len;
		const uint8_t *target;

		err = bt_obex_get_header_target(buf, &len, &target);
		if (err != 0) {
			LOG_ERR("Failed to get target %d", err);
			return err;
		}

		err = bt_obex_make_uuid(&obex_uuid, target, len);
		if (err != 0) {
			LOG_ERR("Invalid UUID of target %d", err);
			return err;
		}
		uuid = &obex_uuid.u128;
	}

	if (uuid == NULL) {
		LOG_ERR("Invalid UUID");
		return -EINVAL;
	}

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&bip->_clients, c, next, _node) {
		if (bt_uuid_cmp(&uuid->uuid, &c->_uuid.uuid) == 0) {
			LOG_ERR("UUID has been registered");
			return -EALREADY;
		}
	}

	if (buf == NULL) {
		buf = bt_goep_create_pdu(&bip->goep, NULL);
		if (buf == NULL) {
			LOG_ERR("Failed to allocate buffer");
			return -ENOBUFS;
		}

		allocated = true;
	}

	memcpy(&client->_uuid, uuid, sizeof(client->_uuid));
	client->_cb = cb;
	client->_bip = bip;
	client->_type = type;
	client->_client.ops = &bip_client_ops;
	client->_client.obex = &bip->goep.obex;
	client->_primary_server = primary_server;

	if (!bt_obex_has_header(buf, BT_OBEX_HEADER_ID_TARGET)) {
		sys_memcpy_swap(val, client->_uuid.val, sizeof(val));
		err = bt_obex_add_header_target(buf, sizeof(val), val);
		if (err != 0) {
			LOG_ERR("Failed to add header target");
			goto failed;
		}
	}

	err = bt_obex_connect(&client->_client, bip->goep.obex.rx.mtu, buf);
	if (err != 0) {
		LOG_ERR("Failed to send conn req");
		goto failed;
	}

	atomic_set(&client->_state, BT_BIP_STATE_CONNECTING);
	sys_slist_append(&bip->_clients, &client->_node);
	return 0;

failed:
	if (allocated) {
		net_buf_unref(buf);
	}
	return err;
}

int bt_bip_primary_client_connect(struct bt_bip *bip, struct bt_bip_client *client,
				  enum bt_bip_conn_type type, struct bt_bip_client_cb *cb,
				  struct net_buf *buf)
{
	return bt_bip_client_connect(bip, client, type, cb, buf, NULL);
}

int bt_bip_secondary_client_connect(struct bt_bip *bip, struct bt_bip_client *client,
				    enum bt_bip_conn_type type, struct bt_bip_client_cb *cb,
				    struct net_buf *buf, struct bt_bip_server *primary_server)
{
	return bt_bip_client_connect(bip, client, type, cb, buf, primary_server);
}

int bt_bip_connect_rsp(struct bt_bip_server *server, uint8_t rsp_code, struct net_buf *buf)
{
	uint8_t val[BT_UUID_SIZE_128];
	int err;
	bool allocated = false;

	if (server == NULL || server->_bip == NULL) {
		return -EINVAL;
	}

	if (atomic_get(&server->_state) != BT_BIP_STATE_CONNECTING) {
		LOG_ERR("Invalid state %u", (uint8_t)atomic_get(&server->_state));
		return -EINVAL;
	}

	if (rsp_code == BT_OBEX_RSP_CODE_SUCCESS && !is_bip_primary_connect(server->_type) &&
	    server->_primary_client != NULL &&
	    atomic_get(&server->_primary_client->_state) != BT_BIP_STATE_CONNECTED) {
		LOG_ERR("Primary conn needs to be connected firstly");
		return -EINVAL;
	}

	if (buf == NULL) {
		buf = bt_goep_create_pdu(&server->_bip->goep, NULL);
		if (buf == NULL) {
			LOG_ERR("Failed to allocate buffer");
			return -ENOBUFS;
		}
		allocated = true;
	} else {
		if (bt_obex_has_header(buf, BT_OBEX_HEADER_ID_WHO) && server->_uuid != NULL) {
			err = bip_check_who(&server->_uuid->uuid, buf);
			if (err != 0) {
				return err;
			}
		}

		if (bt_obex_has_header(buf, BT_OBEX_HEADER_ID_CONN_ID)) {
			err = bip_check_conn_id(server->_conn_id, buf);
			if (err != 0) {
				return err;
			}
		}
	}

	if (rsp_code == BT_OBEX_RSP_CODE_SUCCESS) {
		if (!bt_obex_has_header(buf, BT_OBEX_HEADER_ID_WHO)) {
			sys_memcpy_swap(val, server->_uuid->val, sizeof(val));
			err = bt_obex_add_header_who(buf, sizeof(val), val);
			if (err != 0) {
				LOG_ERR("Failed to add header target %d", err);
				goto failed;
			}
		}

		if (!bt_obex_has_header(buf, BT_OBEX_HEADER_ID_CONN_ID)) {
			err = bt_obex_add_header_conn_id(buf, server->_conn_id);
			if (err != 0) {
				LOG_ERR("Failed to add header conn id %d", err);
				goto failed;
			}
		}
	}

	err = bt_obex_connect_rsp(&server->_server, rsp_code, server->_bip->goep.obex.rx.mtu, buf);
	if (err != 0) {
		LOG_ERR("Failed to send conn rsp %d", err);
		goto failed;
	}

	atomic_set(&server->_state, BT_BIP_STATE_CONNECTED);
	return 0;

failed:
	if (allocated) {
		net_buf_unref(buf);
	}
	return err;
}

int bt_bip_disconnect(struct bt_bip_client *client, struct net_buf *buf)
{
	int err;
	bool allocated = false;

	if (client == NULL || client->_bip == NULL) {
		return -EINVAL;
	}

	if (atomic_get(&client->_state) != BT_BIP_STATE_CONNECTED) {
		LOG_ERR("Invalid state %u", (uint8_t)atomic_get(&client->_state));
		return -EINVAL;
	}

	if (is_bip_primary_connect(client->_type) && client->_secondary_server != NULL &&
	    atomic_get(&client->_secondary_server->_state) == BT_BIP_STATE_CONNECTED) {
		LOG_ERR("Secondary conn needs to be disconnected firstly");
		return -EINVAL;
	}

	if (buf == NULL) {
		buf = bt_goep_create_pdu(&client->_bip->goep, NULL);
		if (buf == NULL) {
			LOG_ERR("Failed to allocate buffer");
			return -ENOBUFS;
		}
		allocated = true;
	}

	if (!bt_obex_has_header(buf, BT_OBEX_HEADER_ID_CONN_ID)) {
		err = bt_obex_add_header_conn_id(buf, client->_conn_id);
		if (err != 0) {
			LOG_ERR("Failed to add header conn id %d", err);
			goto failed;
		}
	} else {
		err = bip_check_conn_id(client->_conn_id, buf);
		if (err != 0) {
			goto failed;
		}
	}

	err = bt_obex_disconnect(&client->_client, buf);
	if (err != 0) {
		LOG_ERR("Failed to send conn rsp %d", err);
		goto failed;
	}

	atomic_set(&client->_state, BT_BIP_STATE_DISCONNECTING);
	return 0;
failed:
	if (allocated) {
		net_buf_unref(buf);
	}
	return err;
}

int bt_bip_disconnect_rsp(struct bt_bip_server *server, uint8_t rsp_code, struct net_buf *buf)
{
	int err;

	if (server == NULL || server->_bip == NULL) {
		return -EINVAL;
	}

	if (atomic_get(&server->_state) != BT_BIP_STATE_DISCONNECTING) {
		LOG_ERR("Invalid state %u", (uint8_t)atomic_get(&server->_state));
		return -EINVAL;
	}

	if (rsp_code == BT_OBEX_RSP_CODE_SUCCESS && is_bip_primary_connect(server->_type) &&
	    server->_secondary_client != NULL &&
	    atomic_get(&server->_secondary_client->_state) == BT_BIP_STATE_CONNECTED) {
		LOG_ERR("Secondary conn needs to be disconnected firstly");
		return -EINVAL;
	}

	err = bt_obex_disconnect_rsp(&server->_server, rsp_code, NULL);
	if (err != 0) {
		LOG_ERR("Failed to send conn rsp %d", err);
		return err;
	}

	if (rsp_code == BT_OBEX_RSP_CODE_SUCCESS) {
		atomic_set(&server->_state, BT_BIP_STATE_DISCONNECTED);
	} else {
		atomic_set(&server->_state, BT_BIP_STATE_CONNECTED);
	}
	return 0;
}

int bt_bip_abort(struct bt_bip_client *client, struct net_buf *buf)
{
	int err;
	bool allocated = false;

	if (client == NULL || client->_bip == NULL) {
		return -EINVAL;
	}

	if (atomic_get(&client->_state) != BT_BIP_STATE_CONNECTED) {
		LOG_ERR("Invalid state %u", (uint8_t)atomic_get(&client->_state));
		return -EINVAL;
	}

	if (client->_rsp_cb == NULL) {
		LOG_ERR("No operation is ongoing");
		return -EINVAL;
	}

	if (buf == NULL) {
		buf = bt_goep_create_pdu(&client->_bip->goep, NULL);
		if (buf == NULL) {
			LOG_ERR("Failed to allocate buffer");
			return -ENOBUFS;
		}
		allocated = true;
	}

	if (bt_obex_has_header(buf, BT_OBEX_HEADER_ID_CONN_ID)) {
		err = bip_check_conn_id(client->_conn_id, buf);
		if (err != 0) {
			goto failed;
		}
	} else {
		err = bt_obex_add_header_conn_id(buf, client->_conn_id);
		if (err != 0) {
			LOG_ERR("Failed to add header conn id %d", err);
			goto failed;
		}
	}

	err = bt_obex_abort(&client->_client, buf);
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

int bt_bip_abort_rsp(struct bt_bip_server *server, uint8_t rsp_code, struct net_buf *buf)
{
	int err;

	if (server == NULL) {
		return -EINVAL;
	}

	if (atomic_get(&server->_state) != BT_BIP_STATE_CONNECTED) {
		LOG_ERR("Invalid state %u", (uint8_t)atomic_get(&server->_state));
		return -EINVAL;
	}

	err = bt_obex_abort_rsp(&server->_server, rsp_code, buf);
	if (err != 0) {
		LOG_ERR("Failed to send abort rsp %d", err);
		return err;
	}

	if (rsp_code == BT_OBEX_RSP_CODE_SUCCESS) {
		server->_opcode = 0;
		server->_optype = NULL;
		server->_req_cb = NULL;
	}

	return 0;
}

static int bip_client_get_req_cb(struct bt_bip_client *client, const char *type,
				 struct net_buf *buf, bool is_get, bt_bip_client_cb_t *cb,
				 const char **req_type)
{
	const uint8_t *type_data;
	uint16_t len;
	int err;

	err = bt_obex_get_header_type(buf, &len, &type_data);
	if (err != 0) {
		LOG_WRN("Failed to get type header %d", err);
		return -EINVAL;
	}

	if (len <= strlen(type_data)) {
		LOG_WRN("Invalid type string len %u <= %u", len, strlen(type_data));
		return -EINVAL;
	}

	if (client->_bip == NULL) {
		LOG_ERR("Invalid BIP client context");
		return -EINVAL;
	}

	*cb = NULL;
	ARRAY_FOR_EACH(bip_functions, i) {
		if (strcmp(bip_functions[i].type, type) != 0) {
			continue;
		}

		if (strcmp(bip_functions[i].type, type_data) != 0) {
			continue;
		}

		if (bip_functions[i].op_get != is_get) {
			continue;
		}

		if ((bip_functions[i].supported_features & BIT(client->_type)) == 0) {
			continue;
		}

		if ((client->_bip->_supp_funcs & BIT(bip_functions[i].func_bit)) == 0) {
			continue;
		}

		if (!has_required_hdrs(buf, &bip_functions[i].hdr)) {
			continue;
		}

		/* Application parameter tag id is not checked. */

		if (bip_functions[i].get_client_cb == NULL) {
			continue;
		}

		*cb = bip_functions[i].get_client_cb(client);
		if (*cb == NULL) {
			continue;
		}

		if (req_type != NULL) {
			*req_type = bip_functions[i].type;
		}
		break;
	}

	if (*cb == NULL) {
		LOG_WRN("Unsupported request");
		return -EINVAL;
	}

	return 0;
}

static int bip_get_or_put(struct bt_bip_client *client, bool is_get, const char *type, bool final,
			  struct net_buf *buf)
{
	int err;
	bt_bip_client_cb_t cb;
	bt_bip_client_cb_t old_cb;
	const char *req_type;
	const char *old_req_type;

	if (client == NULL || buf == NULL) {
		return -EINVAL;
	}

	if (atomic_get(&client->_state) != BT_BIP_STATE_CONNECTED) {
		LOG_ERR("Invalid state %u", (uint8_t)atomic_get(&client->_state));
		return -EINVAL;
	}

	old_cb = client->_rsp_cb;
	old_req_type = client->_req_type;

	if (client->_rsp_cb == NULL || bt_obex_has_header(buf, BT_OBEX_HEADER_ID_TYPE)) {
		err = bip_client_get_req_cb(client, type, buf, is_get, &cb, &req_type);
		if (err != 0) {
			LOG_ERR("Invalid request %d", err);
			return err;
		}

		if (client->_rsp_cb != NULL && cb != client->_rsp_cb) {
			LOG_ERR("Previous operation is not completed");
			return -EINVAL;
		}

		client->_rsp_cb = cb;
		client->_req_type = req_type;
	}

	if (client->_req_type != NULL && strcmp(client->_req_type, type) != 0) {
		LOG_ERR("Invalid request type %s != %s", client->_req_type, type);
		err = -EINVAL;
		goto failed;
	}

	if (bt_obex_has_header(buf, BT_OBEX_HEADER_ID_CONN_ID)) {
		err = bip_check_conn_id(client->_conn_id, buf);
		if (err != 0) {
			goto failed;
		}
	}

	if (is_get) {
		err = bt_obex_get(&client->_client, final, buf);
	} else {
		err = bt_obex_put(&client->_client, final, buf);
	}

failed:
	if (err != 0) {
		client->_rsp_cb = old_cb;
		client->_req_type = old_req_type;
		LOG_ERR("Failed to send get/put req %d", err);
	}

	return err;
}

static int bip_get(struct bt_bip_client *client, const char *type, bool final, struct net_buf *buf)
{
	return bip_get_or_put(client, true, type, final, buf);
}

static int bip_put(struct bt_bip_client *client, const char *type, bool final, struct net_buf *buf)
{
	return bip_get_or_put(client, false, type, final, buf);
}

int bt_bip_get_capabilities(struct bt_bip_client *client, bool final, struct net_buf *buf)
{
	return bip_get(client, BT_BIP_HDR_TYPE_GET_CAPS, final, buf);
}

static int bip_get_or_put_rsp(struct bt_bip_server *server, bool is_get, const char *type,
			      uint8_t rsp_code, struct net_buf *buf)
{
	int err;

	if (server == NULL) {
		return -EINVAL;
	}

	if (atomic_get(&server->_state) != BT_BIP_STATE_CONNECTED) {
		LOG_ERR("Invalid state %u", (uint8_t)atomic_get(&server->_state));
		return -EINVAL;
	}

	if (server->_optype != NULL && strcmp(server->_optype, type) != 0) {
		LOG_ERR("Invalid operation type %s != %s", server->_optype, type);
		return -EINVAL;
	}

	if (is_get) {
		if (server->_opcode != BT_OBEX_OPCODE_GET) {
			LOG_ERR("Operation %u != %u", server->_opcode, BT_OBEX_OPCODE_GET);
			return -EINVAL;
		}

		err = bt_obex_get_rsp(&server->_server, rsp_code, buf);
	} else {
		if (server->_opcode != BT_OBEX_OPCODE_PUT) {
			LOG_ERR("Operation %u != %u", server->_opcode, BT_OBEX_OPCODE_PUT);
			return -EINVAL;
		}

		err = bt_obex_put_rsp(&server->_server, rsp_code, buf);
	}

	if (err != 0) {
		LOG_ERR("Failed to send get/put rsp %d", err);
		return err;
	}

	if (rsp_code != BT_OBEX_RSP_CODE_CONTINUE) {
		server->_opcode = 0;
		server->_optype = NULL;
		server->_req_cb = NULL;
	}

	return 0;
}

int bt_bip_get_capabilities_rsp(struct bt_bip_server *server, uint8_t rsp_code, struct net_buf *buf)
{
	return bip_get_or_put_rsp(server, true, BT_BIP_HDR_TYPE_GET_CAPS, rsp_code, buf);
}

int bt_bip_get_image_list(struct bt_bip_client *client, bool final, struct net_buf *buf)
{
	return bip_get(client, BT_BIP_HDR_TYPE_GET_IMAGE_LIST, final, buf);
}

int bt_bip_get_image_list_rsp(struct bt_bip_server *server, uint8_t rsp_code, struct net_buf *buf)
{
	return bip_get_or_put_rsp(server, true, BT_BIP_HDR_TYPE_GET_IMAGE_LIST, rsp_code, buf);
}

int bt_bip_get_image_properties(struct bt_bip_client *client, bool final, struct net_buf *buf)
{
	return bip_get(client, BT_BIP_HDR_TYPE_GET_IMAGE_PROPERTIES, final, buf);
}

int bt_bip_get_image_properties_rsp(struct bt_bip_server *server, uint8_t rsp_code,
				    struct net_buf *buf)
{
	return bip_get_or_put_rsp(server, true, BT_BIP_HDR_TYPE_GET_IMAGE_PROPERTIES, rsp_code,
				  buf);
}

int bt_bip_get_image(struct bt_bip_client *client, bool final, struct net_buf *buf)
{
	return bip_get(client, BT_BIP_HDR_TYPE_GET_IMAGE, final, buf);
}

int bt_bip_get_image_rsp(struct bt_bip_server *server, uint8_t rsp_code, struct net_buf *buf)
{
	return bip_get_or_put_rsp(server, true, BT_BIP_HDR_TYPE_GET_IMAGE, rsp_code, buf);
}

int bt_bip_get_linked_thumbnail(struct bt_bip_client *client, bool final, struct net_buf *buf)
{
	return bip_get(client, BT_BIP_HDR_TYPE_GET_LINKED_THUMBNAIL, final, buf);
}

int bt_bip_get_linked_thumbnail_rsp(struct bt_bip_server *server, uint8_t rsp_code,
				    struct net_buf *buf)
{
	return bip_get_or_put_rsp(server, true, BT_BIP_HDR_TYPE_GET_LINKED_THUMBNAIL, rsp_code,
				  buf);
}

int bt_bip_get_linked_attachment(struct bt_bip_client *client, bool final, struct net_buf *buf)
{
	return bip_get(client, BT_BIP_HDR_TYPE_GET_LINKED_ATTACHMENT, final, buf);
}

int bt_bip_get_linked_attachment_rsp(struct bt_bip_server *server, uint8_t rsp_code,
				     struct net_buf *buf)
{
	return bip_get_or_put_rsp(server, true, BT_BIP_HDR_TYPE_GET_LINKED_ATTACHMENT, rsp_code,
				  buf);
}

int bt_bip_get_partial_image(struct bt_bip_client *client, bool final, struct net_buf *buf)
{
	return bip_get(client, BT_BIP_HDR_TYPE_GET_PARTIAL_IMAGE, final, buf);
}

int bt_bip_get_partial_image_rsp(struct bt_bip_server *server, uint8_t rsp_code,
				 struct net_buf *buf)
{
	return bip_get_or_put_rsp(server, true, BT_BIP_HDR_TYPE_GET_PARTIAL_IMAGE, rsp_code, buf);
}

int bt_bip_get_monitoring_image(struct bt_bip_client *client, bool final, struct net_buf *buf)
{
	return bip_get(client, BT_BIP_HDR_TYPE_GET_MONITORING_IMAGE, final, buf);
}

int bt_bip_get_monitoring_image_rsp(struct bt_bip_server *server, uint8_t rsp_code,
				    struct net_buf *buf)
{
	return bip_get_or_put_rsp(server, true, BT_BIP_HDR_TYPE_GET_MONITORING_IMAGE, rsp_code,
				  buf);
}

int bt_bip_get_status(struct bt_bip_client *client, bool final, struct net_buf *buf)
{
	return bip_get(client, BT_BIP_HDR_TYPE_GET_STATUS, final, buf);
}

int bt_bip_get_status_rsp(struct bt_bip_server *server, uint8_t rsp_code, struct net_buf *buf)
{
	return bip_get_or_put_rsp(server, true, BT_BIP_HDR_TYPE_GET_STATUS, rsp_code, buf);
}

int bt_bip_put_image(struct bt_bip_client *client, bool final, struct net_buf *buf)
{
	return bip_put(client, BT_BIP_HDR_TYPE_PUT_IMAGE, final, buf);
}

int bt_bip_put_image_rsp(struct bt_bip_server *server, uint8_t rsp_code, struct net_buf *buf)
{
	return bip_get_or_put_rsp(server, false, BT_BIP_HDR_TYPE_PUT_IMAGE, rsp_code, buf);
}

int bt_bip_put_linked_thumbnail(struct bt_bip_client *client, bool final, struct net_buf *buf)
{
	return bip_put(client, BT_BIP_HDR_TYPE_PUT_LINKED_THUMBNAIL, final, buf);
}

int bt_bip_put_linked_thumbnail_rsp(struct bt_bip_server *server, uint8_t rsp_code,
				    struct net_buf *buf)
{
	return bip_get_or_put_rsp(server, false, BT_BIP_HDR_TYPE_PUT_LINKED_THUMBNAIL, rsp_code,
				  buf);
}

int bt_bip_put_linked_attachment(struct bt_bip_client *client, bool final, struct net_buf *buf)
{
	return bip_put(client, BT_BIP_HDR_TYPE_PUT_LINKED_ATTACHMENT, final, buf);
}

int bt_bip_put_linked_attachment_rsp(struct bt_bip_server *server, uint8_t rsp_code,
				     struct net_buf *buf)
{
	return bip_get_or_put_rsp(server, false, BT_BIP_HDR_TYPE_PUT_LINKED_ATTACHMENT, rsp_code,
				  buf);
}

int bt_bip_remote_display(struct bt_bip_client *client, bool final, struct net_buf *buf)
{
	return bip_put(client, BT_BIP_HDR_TYPE_REMOTE_DISPLAY, final, buf);
}

int bt_bip_remote_display_rsp(struct bt_bip_server *server, uint8_t rsp_code, struct net_buf *buf)
{
	return bip_get_or_put_rsp(server, false, BT_BIP_HDR_TYPE_REMOTE_DISPLAY, rsp_code, buf);
}

int bt_bip_delete_image(struct bt_bip_client *client, bool final, struct net_buf *buf)
{
	return bip_put(client, BT_BIP_HDR_TYPE_DELETE_IMAGE, final, buf);
}

int bt_bip_delete_image_rsp(struct bt_bip_server *server, uint8_t rsp_code, struct net_buf *buf)
{
	return bip_get_or_put_rsp(server, false, BT_BIP_HDR_TYPE_DELETE_IMAGE, rsp_code, buf);
}

int bt_bip_start_print(struct bt_bip_client *client, bool final, struct net_buf *buf)
{
	return bip_put(client, BT_BIP_HDR_TYPE_START_PRINT, final, buf);
}

int bt_bip_start_print_rsp(struct bt_bip_server *server, uint8_t rsp_code, struct net_buf *buf)
{
	return bip_get_or_put_rsp(server, false, BT_BIP_HDR_TYPE_START_PRINT, rsp_code, buf);
}

int bt_bip_start_archive(struct bt_bip_client *client, bool final, struct net_buf *buf)
{
	return bip_put(client, BT_BIP_HDR_TYPE_START_ARCHIVE, final, buf);
}

int bt_bip_start_archive_rsp(struct bt_bip_server *server, uint8_t rsp_code, struct net_buf *buf)
{
	return bip_get_or_put_rsp(server, false, BT_BIP_HDR_TYPE_START_ARCHIVE, rsp_code, buf);
}

int bt_bip_add_header_image_desc(struct net_buf *buf, uint16_t len, const uint8_t *desc)
{
	size_t total;

	if (buf == NULL || (len != 0 && desc == NULL)) {
		LOG_WRN("Invalid parameter");
		return -EINVAL;
	}

	total = sizeof(uint8_t) + sizeof(len) + len;
	if (net_buf_tailroom(buf) < total) {
		return -ENOMEM;
	}

	if (!bt_obex_string_is_valid(BT_BIP_HEADER_ID_IMG_DESC, len, desc)) {
		LOG_WRN("Invalid string");
		return -EINVAL;
	}

	net_buf_add_u8(buf, BT_BIP_HEADER_ID_IMG_DESC);
	net_buf_add_be16(buf, (uint16_t)total);
	net_buf_add_mem(buf, desc, len);
	return 0;
}

int bt_bip_add_header_image_handle(struct net_buf *buf, uint16_t len, const uint8_t *handle)
{
	size_t total;

	if (buf == NULL || handle == NULL || len == 0) {
		LOG_WRN("Invalid parameter");
		return -EINVAL;
	}

	total = sizeof(uint8_t) + sizeof(len) + len;
	if (net_buf_tailroom(buf) < total) {
		return -ENOMEM;
	}

	if (!bt_obex_string_is_valid(BT_BIP_HEADER_ID_IMG_HANDLE, len, handle)) {
		LOG_WRN("Invalid string");
		return -EINVAL;
	}

	net_buf_add_u8(buf, BT_BIP_HEADER_ID_IMG_HANDLE);
	net_buf_add_be16(buf, (uint16_t)total);
	net_buf_add_mem(buf, handle, len);
	return 0;
}
