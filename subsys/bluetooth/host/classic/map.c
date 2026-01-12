/*
 * Copyright 2026 NXP
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
#include <zephyr/bluetooth/classic/map.h>

#include "host/hci_core.h"
#include "host/conn_internal.h"
#include "l2cap_br_internal.h"
#include "obex_internal.h"

#if defined(CONFIG_BT_MAP)
#define LOG_LEVEL CONFIG_BT_MAP_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_map);

#define MAP_TRANSPORT_TYPE_RFCOMM (0x00U)
#define MAP_TRANSPORT_TYPE_L2CAP  (0x01U)

typedef void (*mce_mas_cb_t)(struct bt_map_mce_mas *mce_mas, uint8_t rsp_code, struct net_buf *buf);
typedef void (*mce_mns_cb_t)(struct bt_map_mce_mns *mce_mns, bool final, struct net_buf *buf);
typedef void (*mse_mas_cb_t)(struct bt_map_mse_mas *mse_mns, bool final, struct net_buf *buf);
typedef void (*mse_mns_cb_t)(struct bt_map_mse_mns *mse_mas, uint8_t rsp_code, struct net_buf *buf);

struct map_required_app_param {
	uint8_t count;
	const uint8_t *tags;
};

struct map_required_hdr {
	uint8_t count;
	const uint8_t *hdrs;
};

#define MAP_REQUIRED_APP_PARAM_LIST(...)                                                           \
	COND_CODE_0(NUM_VA_ARGS_LESS_1(__VA_ARGS__), \
		    ({.count = 0, .tags = NULL}), \
		    (_MAP_REQUIRED_APP_PARAM_LIST(__VA_ARGS__)))

#define _MAP_REQUIRED_APP_PARAM_LIST(...)                                                          \
	MAP_REQUIRED_APP_PARAM(sizeof((uint8_t[]){__VA_ARGS__}), ((uint8_t[]){__VA_ARGS__}))
#define MAP_REQUIRED_APP_PARAM(_count, _tags)                                                      \
	{                                                                                          \
		.count = (_count), .tags = (const uint8_t *)(_tags)                                \
	}

#define MAP_REQUIRED_HDR_LIST(...)                                                                 \
	COND_CODE_0(NUM_VA_ARGS_LESS_1(__VA_ARGS__), \
		    ({.count = 0, .tags = NULL}), \
		    (_MAP_REQUIRED_HDR_LIST(__VA_ARGS__)))
#define _MAP_REQUIRED_HDR_LIST(...)                                                                \
	MAP_REQUIRED_HDR(sizeof((uint8_t[]){__VA_ARGS__}), ((uint8_t[]){__VA_ARGS__}))
#define MAP_REQUIRED_HDR(_count, _hdrs)                                                            \
	{                                                                                          \
		.count = (_count), .hdrs = (const uint8_t *)(_hdrs)                                \
	}

static bool has_required_hdrs(struct net_buf *buf, const struct map_required_hdr *hdr)
{
	for (uint8_t index = 0; index < hdr->count; index++) {
		if (!bt_obex_has_header(buf, hdr->hdrs[index])) {
			LOG_ERR("Not found required header %u", hdr->hdrs[index]);
			return false;
		}
	}
	return true;
}

static bool has_required_app_params(struct net_buf *buf, const struct map_required_app_param *ap)
{
	for (uint8_t index = 0; index < ap->count; index++) {
		if (!bt_obex_has_app_param(buf, ap->tags[index])) {
			LOG_ERR("Not found required app param %u", ap->tags[index]);
			return false;
		}
	}
	return true;
}

#define SEND_EVENT_REQUIRED_HDR BT_OBEX_HEADER_ID_CONN_ID, BT_OBEX_HEADER_ID_TYPE
#define SEND_EVENT_REQUIRED_AP

#define SET_NTF_REG_REQUIRED_HDR                                                                   \
	BT_OBEX_HEADER_ID_CONN_ID, BT_OBEX_HEADER_ID_TYPE, BT_OBEX_HEADER_ID_APP_PARAM
#define SET_NTF_REG_REQUIRED_AP BT_MAP_APPL_PARAM_TAG_ID_NOTIFICATION_STATUS

#define GET_FOLDER_LISTING_REQUIRED_HDR BT_OBEX_HEADER_ID_CONN_ID, BT_OBEX_HEADER_ID_TYPE
#define GET_FOLDER_LISTING_REQUIRED_AP

#define GET_MSG_LISTING_REQUIRED_HDR                                                               \
	BT_OBEX_HEADER_ID_CONN_ID, BT_OBEX_HEADER_ID_TYPE, BT_OBEX_HEADER_ID_NAME
#define GET_MSG_LISTING_REQUIRED_AP

#define GET_MSG_REQUIRED_HDR                                                                       \
	BT_OBEX_HEADER_ID_CONN_ID, BT_OBEX_HEADER_ID_TYPE, BT_OBEX_HEADER_ID_NAME
#define GET_MSG_REQUIRED_AP

#define SET_MSG_STATUS_REQUIRED_HDR                                                                \
	BT_OBEX_HEADER_ID_CONN_ID, BT_OBEX_HEADER_ID_TYPE, BT_OBEX_HEADER_ID_NAME,                 \
		BT_OBEX_HEADER_ID_APP_PARAM
#define SET_MSG_STATUS_REQUIRED_AP                                                                 \
	BT_MAP_APPL_PARAM_TAG_ID_STATUS_INDICATOR, BT_MAP_APPL_PARAM_TAG_ID_STATUS_VALUE

#define PUSH_MSG_REQUIRED_HDR                                                                      \
	BT_OBEX_HEADER_ID_CONN_ID, BT_OBEX_HEADER_ID_TYPE, BT_OBEX_HEADER_ID_NAME,                 \
		BT_OBEX_HEADER_ID_APP_PARAM
#define PUSH_MSG_REQUIRED_AP BT_MAP_APPL_PARAM_TAG_ID_CHARSET

#define UPDATE_INBOX_REQUIRED_HDR BT_OBEX_HEADER_ID_CONN_ID, BT_OBEX_HEADER_ID_TYPE
#define UPDATE_INBOX_REQUIRED_AP

#define GET_MAS_INST_INFO_REQUIRED_HDR                                                             \
	BT_OBEX_HEADER_ID_CONN_ID, BT_OBEX_HEADER_ID_TYPE, BT_OBEX_HEADER_ID_APP_PARAM
#define GET_MAS_INST_INFO_REQUIRED_AP BT_MAP_APPL_PARAM_TAG_ID_MAS_INSTANCE_ID

#define SET_OWNER_STATUS_REQUIRED_HDR                                                              \
	BT_OBEX_HEADER_ID_CONN_ID, BT_OBEX_HEADER_ID_TYPE, BT_OBEX_HEADER_ID_APP_PARAM
#define SET_OWNER_STATUS_REQUIRED_AP

#define GET_OWNER_STATUS_REQUIRED_HDR BT_OBEX_HEADER_ID_CONN_ID, BT_OBEX_HEADER_ID_TYPE
#define GET_OWNER_STATUS_REQUIRED_AP

#define GET_CONVO_LISTING_REQUIRED_HDR BT_OBEX_HEADER_ID_CONN_ID, BT_OBEX_HEADER_ID_TYPE
#define GET_CONVO_LISTING_REQUIRED_AP

#define SET_NTF_FILTER_REQUIRED_HDR                                                                \
	BT_OBEX_HEADER_ID_CONN_ID, BT_OBEX_HEADER_ID_TYPE, BT_OBEX_HEADER_ID_APP_PARAM
#define SET_NTF_FILTER_REQUIRED_AP BT_MAP_APPL_PARAM_TAG_ID_NOTIFICATION_FILTER_MASK

struct map_mas_function {
	const char *type;
	bool op_get;
	struct map_required_app_param ap;
	struct map_required_hdr hdr;
	mce_mas_cb_t (*get_mce_mas_cb)(struct bt_map_mce_mas *mce_mas);
	mse_mas_cb_t (*get_mse_mas_cb)(struct bt_map_mse_mas *mse_mas);
};

struct map_mns_function {
	const char *type;
	bool op_get;
	struct map_required_app_param ap;
	struct map_required_hdr hdr;
	mce_mns_cb_t (*get_mce_mns_cb)(struct bt_map_mce_mns *mce_mns);
	mse_mns_cb_t (*get_mse_mns_cb)(struct bt_map_mse_mns *mse_mns);
};

static mce_mns_cb_t mce_mns_get_cb_send_event(struct bt_map_mce_mns *mce_mns)
{
	return mce_mns->_cb->send_event;
}

static mce_mas_cb_t mce_mas_get_cb_set_ntf_reg(struct bt_map_mce_mas *mce_mas)
{
	return mce_mas->_cb->set_ntf_reg;
}

static mce_mas_cb_t mce_mas_get_cb_get_folder_listing(struct bt_map_mce_mas *mce_mas)
{
	return mce_mas->_cb->get_folder_listing;
}

static mce_mas_cb_t mce_mas_get_cb_get_msg_listing(struct bt_map_mce_mas *mce_mas)
{
	return mce_mas->_cb->get_msg_listing;
}

static mce_mas_cb_t mce_mas_get_cb_get_msg(struct bt_map_mce_mas *mce_mas)
{
	return mce_mas->_cb->get_msg;
}

static mce_mas_cb_t mce_mas_get_cb_set_msg_status(struct bt_map_mce_mas *mce_mas)
{
	return mce_mas->_cb->set_msg_status;
}

static mce_mas_cb_t mce_mas_get_cb_push_msg(struct bt_map_mce_mas *mce_mas)
{
	return mce_mas->_cb->push_msg;
}

static mce_mas_cb_t mce_mas_get_cb_update_inbox(struct bt_map_mce_mas *mce_mas)
{
	return mce_mas->_cb->update_inbox;
}

static mce_mas_cb_t mce_mas_get_cb_get_mas_inst_info(struct bt_map_mce_mas *mce_mas)
{
	return mce_mas->_cb->get_mas_inst_info;
}

static mce_mas_cb_t mce_mas_get_cb_set_owner_status(struct bt_map_mce_mas *mce_mas)
{
	return mce_mas->_cb->set_owner_status;
}

static mce_mas_cb_t mce_mas_get_cb_get_owner_status(struct bt_map_mce_mas *mce_mas)
{
	return mce_mas->_cb->get_owner_status;
}

static mce_mas_cb_t mce_mas_get_cb_get_convo_listing(struct bt_map_mce_mas *mce_mas)
{
	return mce_mas->_cb->get_convo_listing;
}

static mce_mas_cb_t mce_mas_get_cb_set_ntf_filter(struct bt_map_mce_mas *mce_mas)
{
	return mce_mas->_cb->set_ntf_filter;
}

static mse_mns_cb_t mse_mns_get_cb_send_event(struct bt_map_mse_mns *mse_mns)
{
	return mse_mns->_cb->send_event;
}

static mse_mas_cb_t mse_mas_get_cb_set_ntf_reg(struct bt_map_mse_mas *mse_mas)
{
	return mse_mas->_cb->set_ntf_reg;
}

static mse_mas_cb_t mse_mas_get_cb_get_folder_listing(struct bt_map_mse_mas *mse_mas)
{
	return mse_mas->_cb->get_folder_listing;
}

static mse_mas_cb_t mse_mas_get_cb_get_msg_listing(struct bt_map_mse_mas *mse_mas)
{
	return mse_mas->_cb->get_msg_listing;
}

static mse_mas_cb_t mse_mas_get_cb_get_msg(struct bt_map_mse_mas *mse_mas)
{
	return mse_mas->_cb->get_msg;
}

static mse_mas_cb_t mse_mas_get_cb_set_msg_status(struct bt_map_mse_mas *mse_mas)
{
	return mse_mas->_cb->set_msg_status;
}

static mse_mas_cb_t mse_mas_get_cb_push_msg(struct bt_map_mse_mas *mse_mas)
{
	return mse_mas->_cb->push_msg;
}

static mse_mas_cb_t mse_mas_get_cb_update_inbox(struct bt_map_mse_mas *mse_mas)
{
	return mse_mas->_cb->update_inbox;
}

static mse_mas_cb_t mse_mas_get_cb_get_mas_inst_info(struct bt_map_mse_mas *mse_mas)
{
	return mse_mas->_cb->get_mas_inst_info;
}

static mse_mas_cb_t mse_mas_get_cb_set_owner_status(struct bt_map_mse_mas *mse_mas)
{
	return mse_mas->_cb->set_owner_status;
}

static mse_mas_cb_t mse_mas_get_cb_get_owner_status(struct bt_map_mse_mas *mse_mas)
{
	return mse_mas->_cb->get_owner_status;
}

static mse_mas_cb_t mse_mas_get_cb_get_convo_listing(struct bt_map_mse_mas *mse_mas)
{
	return mse_mas->_cb->get_convo_listing;
}

static mse_mas_cb_t mse_mas_get_cb_set_ntf_filter(struct bt_map_mse_mas *mse_mas)
{
	return mse_mas->_cb->set_ntf_filter;
}

static const struct map_mas_function map_mas_functions[] = {
	{
		.type = BT_MAP_HDR_TYPE_SET_NTF_REG,
		.op_get = false,
		.ap = MAP_REQUIRED_APP_PARAM_LIST(SET_NTF_REG_REQUIRED_AP),
		.hdr = MAP_REQUIRED_HDR_LIST(SET_NTF_REG_REQUIRED_HDR),
		.get_mce_mas_cb = mce_mas_get_cb_set_ntf_reg,
		.get_mse_mas_cb = mse_mas_get_cb_set_ntf_reg,
	},
	{
		.type = BT_MAP_HDR_TYPE_GET_FOLDER_LISTING,
		.op_get = true,
		.ap = MAP_REQUIRED_APP_PARAM_LIST(GET_FOLDER_LISTING_REQUIRED_AP),
		.hdr = MAP_REQUIRED_HDR_LIST(GET_FOLDER_LISTING_REQUIRED_HDR),
		.get_mce_mas_cb = mce_mas_get_cb_get_folder_listing,
		.get_mse_mas_cb = mse_mas_get_cb_get_folder_listing,
	},
	{
		.type = BT_MAP_HDR_TYPE_GET_MSG_LISTING,
		.op_get = true,
		.ap = MAP_REQUIRED_APP_PARAM_LIST(GET_MSG_LISTING_REQUIRED_AP),
		.hdr = MAP_REQUIRED_HDR_LIST(GET_MSG_LISTING_REQUIRED_HDR),
		.get_mce_mas_cb = mce_mas_get_cb_get_msg_listing,
		.get_mse_mas_cb = mse_mas_get_cb_get_msg_listing,
	},
	{
		.type = BT_MAP_HDR_TYPE_GET_MSG,
		.op_get = true,
		.ap = MAP_REQUIRED_APP_PARAM_LIST(GET_MSG_REQUIRED_AP),
		.hdr = MAP_REQUIRED_HDR_LIST(GET_MSG_REQUIRED_HDR),
		.get_mce_mas_cb = mce_mas_get_cb_get_msg,
		.get_mse_mas_cb = mse_mas_get_cb_get_msg,
	},
	{
		.type = BT_MAP_HDR_TYPE_SET_MSG_STATUS,
		.op_get = false,
		.ap = MAP_REQUIRED_APP_PARAM_LIST(SET_MSG_STATUS_REQUIRED_AP),
		.hdr = MAP_REQUIRED_HDR_LIST(SET_MSG_STATUS_REQUIRED_HDR),
		.get_mce_mas_cb = mce_mas_get_cb_set_msg_status,
		.get_mse_mas_cb = mse_mas_get_cb_set_msg_status,
	},
	{
		.type = BT_MAP_HDR_TYPE_PUSH_MSG,
		.op_get = false,
		.ap = MAP_REQUIRED_APP_PARAM_LIST(PUSH_MSG_REQUIRED_AP),
		.hdr = MAP_REQUIRED_HDR_LIST(PUSH_MSG_REQUIRED_HDR),
		.get_mce_mas_cb = mce_mas_get_cb_push_msg,
		.get_mse_mas_cb = mse_mas_get_cb_push_msg,
	},
	{
		.type = BT_MAP_HDR_TYPE_UPDATE_INBOX,
		.op_get = false,
		.ap = MAP_REQUIRED_APP_PARAM_LIST(UPDATE_INBOX_REQUIRED_AP),
		.hdr = MAP_REQUIRED_HDR_LIST(UPDATE_INBOX_REQUIRED_HDR),
		.get_mce_mas_cb = mce_mas_get_cb_update_inbox,
		.get_mse_mas_cb = mse_mas_get_cb_update_inbox,
	},
	{
		.type = BT_MAP_HDR_TYPE_GET_MAS_INST_INFO,
		.op_get = true,
		.ap = MAP_REQUIRED_APP_PARAM_LIST(GET_MAS_INST_INFO_REQUIRED_AP),
		.hdr = MAP_REQUIRED_HDR_LIST(GET_MAS_INST_INFO_REQUIRED_HDR),
		.get_mce_mas_cb = mce_mas_get_cb_get_mas_inst_info,
		.get_mse_mas_cb = mse_mas_get_cb_get_mas_inst_info,
	},
	{
		.type = BT_MAP_HDR_TYPE_SET_OWNER_STATUS,
		.op_get = false,
		.ap = MAP_REQUIRED_APP_PARAM_LIST(SET_OWNER_STATUS_REQUIRED_AP),
		.hdr = MAP_REQUIRED_HDR_LIST(SET_OWNER_STATUS_REQUIRED_HDR),
		.get_mce_mas_cb = mce_mas_get_cb_set_owner_status,
		.get_mse_mas_cb = mse_mas_get_cb_set_owner_status,
	},
	{
		.type = BT_MAP_HDR_TYPE_GET_OWNER_STATUS,
		.op_get = true,
		.ap = MAP_REQUIRED_APP_PARAM_LIST(GET_OWNER_STATUS_REQUIRED_AP),
		.hdr = MAP_REQUIRED_HDR_LIST(GET_OWNER_STATUS_REQUIRED_HDR),
		.get_mce_mas_cb = mce_mas_get_cb_get_owner_status,
		.get_mse_mas_cb = mse_mas_get_cb_get_owner_status,
	},
	{
		.type = BT_MAP_HDR_TYPE_GET_CONVO_LISTING,
		.op_get = true,
		.ap = MAP_REQUIRED_APP_PARAM_LIST(GET_CONVO_LISTING_REQUIRED_AP),
		.hdr = MAP_REQUIRED_HDR_LIST(GET_CONVO_LISTING_REQUIRED_HDR),
		.get_mce_mas_cb = mce_mas_get_cb_get_convo_listing,
		.get_mse_mas_cb = mse_mas_get_cb_get_convo_listing,
	},
	{
		.type = BT_MAP_HDR_TYPE_SET_NTF_FILTER,
		.op_get = false,
		.ap = MAP_REQUIRED_APP_PARAM_LIST(SET_NTF_FILTER_REQUIRED_AP),
		.hdr = MAP_REQUIRED_HDR_LIST(SET_NTF_FILTER_REQUIRED_HDR),
		.get_mce_mas_cb = mce_mas_get_cb_set_ntf_filter,
		.get_mse_mas_cb = mse_mas_get_cb_set_ntf_filter,
	},
};

static const struct map_mns_function map_mns_functions[] = {
	{
		.type = BT_MAP_HDR_TYPE_SEND_EVENT,
		.op_get = false,
		.ap = MAP_REQUIRED_APP_PARAM_LIST(SEND_EVENT_REQUIRED_AP),
		.hdr = MAP_REQUIRED_HDR_LIST(SEND_EVENT_REQUIRED_HDR),
		.get_mce_mns_cb = mce_mns_get_cb_send_event,
		.get_mse_mns_cb = mse_mns_get_cb_send_event,
	},
};

static const struct bt_uuid_128 *map_mas_uuid = BT_MAP_UUID_MAS;
static const struct bt_uuid_128 *map_mns_uuid = BT_MAP_UUID_MNS;

static uint32_t map_get_connect_id(void)
{
	static uint32_t connect_id;

	connect_id++;

	if (connect_id == 0) {
		connect_id = 1;
	}

	return connect_id;
}

static int map_check_conn_id(uint32_t id, struct net_buf *buf)
{
	uint32_t conn_id;
	int err;

	err = bt_obex_get_header_conn_id(buf, &conn_id);
	if (err != 0) {
		LOG_ERR("Failed to get connection ID: %d (expected: %u)", err, id);
		return err;
	}

	if (conn_id != id) {
		LOG_ERR("Conn id is mismatched %u != %u", conn_id, id);
		return -EINVAL;
	}

	return 0;
}

static int map_check_who(const struct bt_uuid *uuid, struct net_buf *buf)
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

static int map_check_target(const struct bt_uuid *uuid, struct net_buf *buf)
{
	uint16_t len;
	const uint8_t *target;
	union bt_obex_uuid obex_uuid;
	int err;

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

	if (bt_uuid_cmp(uuid, &obex_uuid.u128.uuid) != 0) {
		LOG_ERR("Invalid UUID of target");
		return -EINVAL;
	}

	return 0;
}

#if defined(CONFIG_BT_MAP_MCE)
int bt_map_mce_mas_cb_register(struct bt_map_mce_mas *mce_mas, const struct bt_map_mce_mas_cb *cb)
{
	if (mce_mas == NULL || cb == NULL) {
		return -EINVAL;
	}

	mce_mas->_cb = cb;

	LOG_DBG("MCE MAS callbacks registered");
	return 0;
}

static void mce_mas_clear_pending_request(struct bt_map_mce_mas *mce_mas)
{
	mce_mas->_rsp_cb = NULL;
	mce_mas->_req_type = NULL;
}

static void mce_mas_transport_connected(struct bt_conn *conn, struct bt_goep *goep)
{
	struct bt_map_mce_mas *mce_mas = CONTAINER_OF(goep, struct bt_map_mce_mas, goep);

	LOG_DBG("MCE MAS transport connected");
	atomic_set(&mce_mas->_transport_state, BT_MAP_TRANSPORT_STATE_CONNECTED);

	if (mce_mas->_transport_type == MAP_TRANSPORT_TYPE_L2CAP) {
		if (mce_mas->_cb != NULL && mce_mas->_cb->l2cap_connected != NULL) {
			mce_mas->_cb->l2cap_connected(conn, mce_mas);
		}
	} else {
		if (mce_mas->_cb != NULL && mce_mas->_cb->rfcomm_connected != NULL) {
			mce_mas->_cb->rfcomm_connected(conn, mce_mas);
		}
	}
}

static void mce_mas_transport_disconnected(struct bt_goep *goep)
{
	struct bt_map_mce_mas *mce_mas = CONTAINER_OF(goep, struct bt_map_mce_mas, goep);

	LOG_DBG("MCE MAS transport disconnected");
	atomic_set(&mce_mas->_transport_state, BT_MAP_TRANSPORT_STATE_DISCONNECTED);
	atomic_set(&mce_mas->_state, BT_MAP_STATE_DISCONNECTED);
	mce_mas_clear_pending_request(mce_mas);

	if (mce_mas->_transport_type == MAP_TRANSPORT_TYPE_L2CAP) {
		if (mce_mas->_cb != NULL && mce_mas->_cb->l2cap_disconnected != NULL) {
			mce_mas->_cb->l2cap_disconnected(mce_mas);
		}
	} else {
		if (mce_mas->_cb != NULL && mce_mas->_cb->rfcomm_disconnected != NULL) {
			mce_mas->_cb->rfcomm_disconnected(mce_mas);
		}
	}
}

static struct bt_goep_transport_ops mce_mas_transport_ops = {
	.connected = mce_mas_transport_connected,
	.disconnected = mce_mas_transport_disconnected,
};

static int mce_mas_transport_connect(struct bt_conn *conn, struct bt_map_mce_mas *mce_mas,
				     uint8_t type, uint16_t psm, uint8_t channel)
{
	int err;

	if (conn == NULL || mce_mas == NULL) {
		return -EINVAL;
	}

	if (atomic_get(&mce_mas->_transport_state) != BT_MAP_TRANSPORT_STATE_DISCONNECTED) {
		LOG_DBG("MCE MAS transport connect in progress");
		return -EINPROGRESS;
	}

	LOG_DBG("MCE MAS transport connecting, type %u", type);
	mce_mas->_transport_type = type;
	mce_mas->goep.transport_ops = &mce_mas_transport_ops;
	atomic_set(&mce_mas->_transport_state, BT_MAP_TRANSPORT_STATE_CONNECTING);

	if (type == MAP_TRANSPORT_TYPE_L2CAP) {
		LOG_DBG("Connecting via L2CAP PSM 0x%04x", psm);
		err = bt_goep_transport_l2cap_connect(conn, &mce_mas->goep, psm);
	} else {
		LOG_DBG("Connecting via RFCOMM channel %u", channel);
		err = bt_goep_transport_rfcomm_connect(conn, &mce_mas->goep, channel);
	}

	if (err != 0) {
		LOG_ERR("Transport connect failed: %d", err);
		atomic_set(&mce_mas->_transport_state, BT_MAP_TRANSPORT_STATE_DISCONNECTED);
	}

	return err;
}

int bt_map_mce_mas_rfcomm_connect(struct bt_conn *conn, struct bt_map_mce_mas *mce_mas,
				  uint8_t channel)
{
	if (channel == 0) {
		return -EINVAL;
	}

	return mce_mas_transport_connect(conn, mce_mas, MAP_TRANSPORT_TYPE_RFCOMM, 0, channel);
}

int bt_map_mce_mas_l2cap_connect(struct bt_conn *conn, struct bt_map_mce_mas *mce_mas, uint16_t psm)
{
	if (psm == 0) {
		return -EINVAL;
	}

	return mce_mas_transport_connect(conn, mce_mas, MAP_TRANSPORT_TYPE_L2CAP, psm, 0);
}

static int mce_mas_transport_disconnect(struct bt_map_mce_mas *mce_mas, uint8_t type)
{
	int err;

	if (mce_mas == NULL) {
		return -EINVAL;
	}

	if (atomic_get(&mce_mas->_transport_state) != BT_MAP_TRANSPORT_STATE_CONNECTED) {
		LOG_DBG("MCE MAS transport disconnect in progress");
		return -EINPROGRESS;
	}

	LOG_DBG("MCE MAS transport disconnecting, type %u", type);
	atomic_set(&mce_mas->_transport_state, BT_MAP_TRANSPORT_STATE_DISCONNECTING);

	if (type == MAP_TRANSPORT_TYPE_L2CAP) {
		err = bt_goep_transport_l2cap_disconnect(&mce_mas->goep);
	} else {
		err = bt_goep_transport_rfcomm_disconnect(&mce_mas->goep);
	}

	if (err != 0) {
		LOG_ERR("Transport disconnect failed: %d", err);
		atomic_set(&mce_mas->_transport_state, BT_MAP_TRANSPORT_STATE_CONNECTED);
	}

	return err;
}

int bt_map_mce_mas_rfcomm_disconnect(struct bt_map_mce_mas *mce_mas)
{
	return mce_mas_transport_disconnect(mce_mas, MAP_TRANSPORT_TYPE_RFCOMM);
}

int bt_map_mce_mas_l2cap_disconnect(struct bt_map_mce_mas *mce_mas)
{
	return mce_mas_transport_disconnect(mce_mas, MAP_TRANSPORT_TYPE_L2CAP);
}

static void mce_mas_connected(struct bt_obex_client *client, uint8_t rsp_code, uint8_t version,
			      uint16_t mopl, struct net_buf *buf)
{
	struct bt_map_mce_mas *c = CONTAINER_OF(client, struct bt_map_mce_mas, _client);

	LOG_DBG("MCE MAS connected, rsp_code: 0x%02x, version: 0x%02x, mopl: %u", rsp_code, version,
		mopl);

	if (rsp_code == BT_OBEX_RSP_CODE_SUCCESS) {
		atomic_set(&c->_state, BT_MAP_STATE_CONNECTED);
		bt_obex_get_header_conn_id(buf, &c->_conn_id);
		LOG_DBG("Connection ID: %u", c->_conn_id);
	} else {
		atomic_set(&c->_state, BT_MAP_STATE_DISCONNECTED);
	}

	if ((c->_cb != NULL) && (c->_cb->connected)) {
		c->_cb->connected(c, rsp_code, version, mopl, buf);
	}
}

static void mce_mas_disconnected(struct bt_obex_client *client, uint8_t rsp_code,
				 struct net_buf *buf)
{
	struct bt_map_mce_mas *c = CONTAINER_OF(client, struct bt_map_mce_mas, _client);

	LOG_DBG("MCE MAS disconnected, rsp_code: 0x%02x", rsp_code);

	if (rsp_code == BT_OBEX_RSP_CODE_SUCCESS) {
		atomic_set(&c->_state, BT_MAP_STATE_DISCONNECTED);
		mce_mas_clear_pending_request(c);
	} else {
		atomic_set(&c->_state, BT_MAP_STATE_CONNECTED);
	}

	if ((c->_cb != NULL) && (c->_cb->disconnected)) {
		c->_cb->disconnected(c, rsp_code, buf);
	}
}

static void mce_mas_put(struct bt_obex_client *client, uint8_t rsp_code, struct net_buf *buf)
{
	struct bt_map_mce_mas *c = CONTAINER_OF(client, struct bt_map_mce_mas, _client);

	LOG_DBG("MCE MAS PUT response, rsp_code: 0x%02x", rsp_code);

	if (c->_rsp_cb != NULL) {
		c->_rsp_cb(c, rsp_code, buf);
	} else {
		LOG_WRN("No cb for rsp");
	}

	if (rsp_code != BT_OBEX_RSP_CODE_CONTINUE) {
		mce_mas_clear_pending_request(c);
	}
}

static void mce_mas_get(struct bt_obex_client *client, uint8_t rsp_code, struct net_buf *buf)
{
	struct bt_map_mce_mas *c = CONTAINER_OF(client, struct bt_map_mce_mas, _client);

	LOG_DBG("MCE MAS GET response, rsp_code: 0x%02x", rsp_code);

	if (c->_rsp_cb != NULL) {
		c->_rsp_cb(c, rsp_code, buf);
	} else {
		LOG_WRN("No cb for rsp");
	}

	if (rsp_code != BT_OBEX_RSP_CODE_CONTINUE) {
		mce_mas_clear_pending_request(c);
	}
}

static void mce_mas_abort(struct bt_obex_client *client, uint8_t rsp_code, struct net_buf *buf)
{
	struct bt_map_mce_mas *c = CONTAINER_OF(client, struct bt_map_mce_mas, _client);
	int err;

	LOG_DBG("MCE MAS abort response, rsp_code: 0x%02x", rsp_code);

	if ((c->_cb != NULL) && (c->_cb->abort != NULL)) {
		c->_cb->abort(c, rsp_code, buf);
	}

	if (rsp_code == BT_OBEX_RSP_CODE_SUCCESS) {
		mce_mas_clear_pending_request(c);
		return;
	}

	err = bt_map_mce_mas_disconnect(c, NULL);
	if (err != 0) {
		LOG_ERR("Failed to send MCE MAS disconnect");
	}
}

static void mce_mas_setpath(struct bt_obex_client *client, uint8_t rsp_code, struct net_buf *buf)
{
	struct bt_map_mce_mas *c = CONTAINER_OF(client, struct bt_map_mce_mas, _client);

	LOG_DBG("MCE MAS setpath response, rsp_code: 0x%02x", rsp_code);

	if ((c->_cb != NULL) && (c->_cb->set_folder)) {
		c->_cb->set_folder(c, rsp_code, buf);
	}
}

static const struct bt_obex_client_ops mce_mas_ops = {
	.connect = mce_mas_connected,
	.disconnect = mce_mas_disconnected,
	.put = mce_mas_put,
	.get = mce_mas_get,
	.abort = mce_mas_abort,
	.setpath = mce_mas_setpath,
	.action = NULL,
};

struct net_buf *bt_map_mce_mas_create_pdu(struct bt_map_mce_mas *mce_mas, struct net_buf_pool *pool)
{
	if (mce_mas == NULL) {
		LOG_ERR("Invalid parameter");
		return NULL;
	}

	return bt_goep_create_pdu(&mce_mas->goep, pool);
}

int bt_map_mce_mas_connect(struct bt_map_mce_mas *mce_mas, struct net_buf *buf)
{
	uint8_t val[BT_UUID_SIZE_128];
	int err;
	bool allocated = false;

	if (mce_mas == NULL) {
		return -EINVAL;
	}

	if (atomic_get(&mce_mas->_state) != BT_MAP_STATE_DISCONNECTED) {
		LOG_ERR("Invalid state %u", (uint8_t)atomic_get(&mce_mas->_state));
		return -EINVAL;
	}

	LOG_DBG("MCE MAS connecting");

	if (buf == NULL) {
		buf = bt_goep_create_pdu(&mce_mas->goep, NULL);
		if (buf == NULL) {
			LOG_ERR("Failed to allocate buffer");
			return -ENOBUFS;
		}
		allocated = true;
	}

	if (bt_obex_has_header(buf, BT_OBEX_HEADER_ID_TARGET)) {
		err = map_check_target(&map_mas_uuid->uuid, buf);
		if (err != 0) {
			goto failed;
		}
	} else {
		sys_memcpy_swap(val, map_mas_uuid->val, sizeof(val));
		err = bt_obex_add_header_target(buf, sizeof(val), val);
		if (err != 0) {
			LOG_ERR("Failed to add header target");
			goto failed;
		}
	}

	mce_mas->_client.ops = &mce_mas_ops;
	mce_mas->_client.obex = &mce_mas->goep.obex;

	err = bt_obex_connect(&mce_mas->_client, mce_mas->goep.obex.rx.mtu, buf);
	if (err != 0) {
		LOG_ERR("Failed to send conn req");
		goto failed;
	}

	atomic_set(&mce_mas->_state, BT_MAP_STATE_CONNECTING);
	return 0;

failed:
	if (allocated) {
		net_buf_unref(buf);
	}
	return err;
}

int bt_map_mce_mas_disconnect(struct bt_map_mce_mas *mce_mas, struct net_buf *buf)
{
	int err;
	bool allocated = false;

	if (mce_mas == NULL) {
		return -EINVAL;
	}

	if (atomic_get(&mce_mas->_state) != BT_MAP_STATE_CONNECTED) {
		LOG_ERR("Invalid state %u", (uint8_t)atomic_get(&mce_mas->_state));
		return -EINVAL;
	}

	LOG_DBG("MCE MAS disconnecting");

	if (buf == NULL) {
		buf = bt_goep_create_pdu(&mce_mas->goep, NULL);
		if (buf == NULL) {
			LOG_ERR("Failed to allocate buffer");
			return -ENOBUFS;
		}
		allocated = true;
	}

	if (!bt_obex_has_header(buf, BT_OBEX_HEADER_ID_CONN_ID)) {
		err = bt_obex_add_header_conn_id(buf, mce_mas->_conn_id);
		if (err != 0) {
			LOG_ERR("Failed to add header conn id %d", err);
			goto failed;
		}
	} else {
		err = map_check_conn_id(mce_mas->_conn_id, buf);
		if (err != 0) {
			goto failed;
		}
	}

	err = bt_obex_disconnect(&mce_mas->_client, buf);
	if (err != 0) {
		LOG_ERR("Failed to send conn rsp %d", err);
		goto failed;
	}

	atomic_set(&mce_mas->_state, BT_MAP_STATE_DISCONNECTING);
	return 0;

failed:
	if (allocated) {
		net_buf_unref(buf);
	}
	return err;
}

int bt_map_mce_mas_abort(struct bt_map_mce_mas *mce_mas, struct net_buf *buf)
{
	int err;
	bool allocated = false;

	if (mce_mas == NULL) {
		return -EINVAL;
	}

	if (atomic_get(&mce_mas->_state) != BT_MAP_STATE_CONNECTED) {
		LOG_ERR("Invalid state %u", (uint8_t)atomic_get(&mce_mas->_state));
		return -EINVAL;
	}

	if (mce_mas->_rsp_cb == NULL) {
		LOG_ERR("No operation is ongoing");
		return -EINVAL;
	}

	LOG_DBG("MCE MAS aborting");

	if (buf == NULL) {
		buf = bt_goep_create_pdu(&mce_mas->goep, NULL);
		if (buf == NULL) {
			LOG_ERR("Failed to allocate buffer");
			return -ENOBUFS;
		}
		allocated = true;
	}

	if (bt_obex_has_header(buf, BT_OBEX_HEADER_ID_CONN_ID)) {
		err = map_check_conn_id(mce_mas->_conn_id, buf);
		if (err != 0) {
			goto failed;
		}
	} else {
		err = bt_obex_add_header_conn_id(buf, mce_mas->_conn_id);
		if (err != 0) {
			LOG_ERR("Failed to add header conn id %d", err);
			goto failed;
		}
	}

	err = bt_obex_abort(&mce_mas->_client, buf);
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

int bt_map_mce_mas_set_folder(struct bt_map_mce_mas *mce_mas, uint8_t flags, struct net_buf *buf)
{
	int err;
	bool allocated = false;

	if (mce_mas == NULL) {
		return -EINVAL;
	}

	if (atomic_get(&mce_mas->_state) != BT_MAP_STATE_CONNECTED) {
		LOG_ERR("Invalid state %u", (uint8_t)atomic_get(&mce_mas->_state));
		return -EINVAL;
	}

	if (mce_mas->_rsp_cb != NULL) {
		LOG_ERR("Previous operation is not completed");
		return -EINVAL;
	}

	if ((flags == BT_MAP_SET_FOLDER_FLAGS_DOWN) &&
	    (!bt_obex_has_header(buf, BT_OBEX_HEADER_ID_NAME))) {
		LOG_ERR("Failed to get name when flags is root or down");
		return -EINVAL;
	}

	LOG_DBG("MCE MAS set folder, flags: 0x%02x", flags);

	if (buf == NULL) {
		buf = bt_goep_create_pdu(&mce_mas->goep, NULL);
		if (buf == NULL) {
			LOG_ERR("Failed to allocate buffer");
			return -ENOBUFS;
		}
		allocated = true;
	}

	if (!bt_obex_has_header(buf, BT_OBEX_HEADER_ID_CONN_ID)) {
		err = bt_obex_add_header_conn_id(buf, mce_mas->_conn_id);
		if (err != 0) {
			LOG_ERR("Failed to add header conn id %d", err);
			goto failed;
		}
	} else {
		err = map_check_conn_id(mce_mas->_conn_id, buf);
		if (err != 0) {
			goto failed;
		}
	}

	err = bt_obex_setpath(&mce_mas->_client, flags, buf);
	if (err != 0) {
		LOG_ERR("Failed to send conn rsp %d", err);
		goto failed;
	}
	return 0;

failed:
	if (allocated) {
		net_buf_unref(buf);
	}
	return err;
}

static int mce_mas_get_req_cb(struct bt_map_mce_mas *mce_mas, const char *type, struct net_buf *buf,
			      bool is_get, mce_mas_cb_t *cb, const char **req_type)
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

	*cb = NULL;
	ARRAY_FOR_EACH(map_mas_functions, i) {
		if (strcmp(map_mas_functions[i].type, type) != 0) {
			continue;
		}

		if (strcmp(map_mas_functions[i].type, type_data) != 0) {
			continue;
		}

		if (map_mas_functions[i].op_get != is_get) {
			continue;
		}

		if (!has_required_hdrs(buf, &map_mas_functions[i].hdr)) {
			continue;
		}

		if (!has_required_app_params(buf, &map_mas_functions[i].ap)) {
			continue;
		}

		if (map_mas_functions[i].get_mce_mas_cb == NULL) {
			continue;
		}

		*cb = map_mas_functions[i].get_mce_mas_cb(mce_mas);
		if (*cb == NULL) {
			continue;
		}

		if (req_type != NULL) {
			*req_type = map_mas_functions[i].type;
		}
		break;
	}

	if (*cb == NULL) {
		LOG_WRN("Unsupported request");
		return -EINVAL;
	}

	return 0;
}

static int mce_mas_get_or_put(struct bt_map_mce_mas *mce_mas, bool is_get, const char *type,
			      bool final, struct net_buf *buf)
{
	int err;
	mce_mas_cb_t cb;
	mce_mas_cb_t old_cb;
	const char *req_type;
	const char *old_req_type;

	if (mce_mas == NULL || buf == NULL) {
		return -EINVAL;
	}

	if (atomic_get(&mce_mas->_state) != BT_MAP_STATE_CONNECTED) {
		LOG_ERR("Invalid state %u", (uint8_t)atomic_get(&mce_mas->_state));
		return -EINVAL;
	}

	LOG_DBG("MCE MAS %s request, type: %s, final: %d", is_get ? "GET" : "PUT", type, final);

	old_cb = mce_mas->_rsp_cb;
	old_req_type = mce_mas->_req_type;

	if (mce_mas->_rsp_cb == NULL || bt_obex_has_header(buf, BT_OBEX_HEADER_ID_TYPE)) {
		if (is_get && mce_mas->goep._goep_v2 &&
		    !bt_obex_has_header(buf, BT_OBEX_HEADER_ID_SRM)) {
			LOG_ERR("Failed to get SRM");
			return -ENODATA;
		}

		err = mce_mas_get_req_cb(mce_mas, type, buf, is_get, &cb, &req_type);
		if (err != 0) {
			LOG_ERR("Invalid request %d", err);
			return err;
		}

		if (mce_mas->_rsp_cb != NULL && cb != mce_mas->_rsp_cb) {
			LOG_ERR("Previous operation is not completed");
			return -EINVAL;
		}

		mce_mas->_rsp_cb = cb;
		mce_mas->_req_type = req_type;
	}

	if (mce_mas->_req_type != NULL && strcmp(mce_mas->_req_type, type) != 0) {
		LOG_ERR("Invalid request type %s != %s", mce_mas->_req_type, type);
		err = -EINVAL;
		goto failed;
	}

	if (bt_obex_has_header(buf, BT_OBEX_HEADER_ID_CONN_ID)) {
		err = map_check_conn_id(mce_mas->_conn_id, buf);
		if (err != 0) {
			goto failed;
		}
	}

	if (!is_get && final && !bt_obex_has_header(buf, BT_OBEX_HEADER_ID_END_BODY)) {
		LOG_ERR("Failed to get end of body");
		goto failed;
	}

	if (is_get) {
		err = bt_obex_get(&mce_mas->_client, final, buf);
	} else {
		err = bt_obex_put(&mce_mas->_client, final, buf);
	}

failed:
	if (err != 0) {
		mce_mas->_rsp_cb = old_cb;
		mce_mas->_req_type = old_req_type;
		LOG_ERR("Failed to send get/put req %d", err);
	}

	return err;
}

int bt_map_mce_mas_set_ntf_reg(struct bt_map_mce_mas *mce_mas, bool final, struct net_buf *buf)
{
	return mce_mas_get_or_put(mce_mas, false, BT_MAP_HDR_TYPE_SET_NTF_REG, final, buf);
}

int bt_map_mce_mas_get_folder_listing(struct bt_map_mce_mas *mce_mas, bool final,
				      struct net_buf *buf)
{
	return mce_mas_get_or_put(mce_mas, true, BT_MAP_HDR_TYPE_GET_FOLDER_LISTING, final, buf);
}

int bt_map_mce_mas_get_msg_listing(struct bt_map_mce_mas *mce_mas, bool final, struct net_buf *buf)
{
	return mce_mas_get_or_put(mce_mas, true, BT_MAP_HDR_TYPE_GET_MSG_LISTING, final, buf);
}

int bt_map_mce_mas_get_msg(struct bt_map_mce_mas *mce_mas, bool final, struct net_buf *buf)
{
	return mce_mas_get_or_put(mce_mas, true, BT_MAP_HDR_TYPE_GET_MSG, final, buf);
}

int bt_map_mce_mas_set_msg_status(struct bt_map_mce_mas *mce_mas, bool final, struct net_buf *buf)
{
	return mce_mas_get_or_put(mce_mas, false, BT_MAP_HDR_TYPE_SET_MSG_STATUS, final, buf);
}

int bt_map_mce_mas_push_msg(struct bt_map_mce_mas *mce_mas, bool final, struct net_buf *buf)
{
	return mce_mas_get_or_put(mce_mas, false, BT_MAP_HDR_TYPE_PUSH_MSG, final, buf);
}

int bt_map_mce_mas_update_inbox(struct bt_map_mce_mas *mce_mas, bool final, struct net_buf *buf)
{
	return mce_mas_get_or_put(mce_mas, false, BT_MAP_HDR_TYPE_UPDATE_INBOX, final, buf);
}

int bt_map_mce_mas_get_mas_inst_info(struct bt_map_mce_mas *mce_mas, bool final,
				     struct net_buf *buf)
{
	return mce_mas_get_or_put(mce_mas, true, BT_MAP_HDR_TYPE_GET_MAS_INST_INFO, final, buf);
}

int bt_map_mce_mas_set_owner_status(struct bt_map_mce_mas *mce_mas, bool final, struct net_buf *buf)
{
	return mce_mas_get_or_put(mce_mas, false, BT_MAP_HDR_TYPE_SET_OWNER_STATUS, final, buf);
}

int bt_map_mce_mas_get_owner_status(struct bt_map_mce_mas *mce_mas, bool final, struct net_buf *buf)
{
	return mce_mas_get_or_put(mce_mas, true, BT_MAP_HDR_TYPE_GET_OWNER_STATUS, final, buf);
}

int bt_map_mce_mas_get_convo_listing(struct bt_map_mce_mas *mce_mas, bool final,
				     struct net_buf *buf)
{
	return mce_mas_get_or_put(mce_mas, true, BT_MAP_HDR_TYPE_GET_CONVO_LISTING, final, buf);
}

int bt_map_mce_mas_set_ntf_filter(struct bt_map_mce_mas *mce_mas, bool final, struct net_buf *buf)
{
	return mce_mas_get_or_put(mce_mas, false, BT_MAP_HDR_TYPE_SET_NTF_FILTER, final, buf);
}

/* MCE MNS Implementation */
int bt_map_mce_mns_cb_register(struct bt_map_mce_mns *mce_mns, const struct bt_map_mce_mns_cb *cb)
{
	if (mce_mns == NULL || cb == NULL) {
		return -EINVAL;
	}

	mce_mns->_cb = cb;

	LOG_DBG("MCE MNS callbacks registered");
	return 0;
}

static void mce_mns_clear_pending_request(struct bt_map_mce_mns *mce_mns)
{
	mce_mns->_opcode = 0;
	mce_mns->_optype = NULL;
	mce_mns->_req_cb = NULL;
}

static void mce_mns_transport_connected(struct bt_conn *conn, struct bt_goep *goep)
{
	struct bt_map_mce_mns *mce_mns = CONTAINER_OF(goep, struct bt_map_mce_mns, goep);

	LOG_DBG("MCE MNS transport connected");
	atomic_set(&mce_mns->_transport_state, BT_MAP_TRANSPORT_STATE_CONNECTED);

	if (mce_mns->_transport_type == MAP_TRANSPORT_TYPE_L2CAP) {
		if (mce_mns->_cb != NULL && mce_mns->_cb->l2cap_connected != NULL) {
			mce_mns->_cb->l2cap_connected(conn, mce_mns);
		}
	} else {
		if (mce_mns->_cb != NULL && mce_mns->_cb->rfcomm_connected != NULL) {
			mce_mns->_cb->rfcomm_connected(conn, mce_mns);
		}
	}
}

static void mce_mns_transport_disconnected(struct bt_goep *goep)
{
	struct bt_map_mce_mns *mce_mns = CONTAINER_OF(goep, struct bt_map_mce_mns, goep);
	int err;

	LOG_DBG("MCE MNS transport disconnected");
	atomic_set(&mce_mns->_transport_state, BT_MAP_TRANSPORT_STATE_DISCONNECTED);
	atomic_set(&mce_mns->_state, BT_MAP_STATE_DISCONNECTED);
	mce_mns_clear_pending_request(mce_mns);

	err = bt_obex_server_unregister(&mce_mns->_server);
	if (err != 0) {
		LOG_ERR("Failed to unregister obex server %d", err);
	}

	if (mce_mns->_transport_type == MAP_TRANSPORT_TYPE_L2CAP) {
		if (mce_mns->_cb != NULL && mce_mns->_cb->l2cap_disconnected != NULL) {
			mce_mns->_cb->l2cap_disconnected(mce_mns);
		}
	} else {
		if (mce_mns->_cb != NULL && mce_mns->_cb->rfcomm_disconnected != NULL) {
			mce_mns->_cb->rfcomm_disconnected(mce_mns);
		}
	}
}

static struct bt_goep_transport_ops mce_mns_transport_ops = {
	.connected = mce_mns_transport_connected,
	.disconnected = mce_mns_transport_disconnected,
};

static int mce_mns_transport_disconnect(struct bt_map_mce_mns *mce_mns, uint8_t type)
{
	int err;

	if (mce_mns == NULL) {
		return -EINVAL;
	}

	if (atomic_get(&mce_mns->_transport_state) != BT_MAP_TRANSPORT_STATE_CONNECTED) {
		LOG_DBG("MCE MNS transport disconnect in progress");
		return -EINPROGRESS;
	}

	LOG_DBG("MCE MNS transport disconnecting, type %u", type);
	atomic_set(&mce_mns->_transport_state, BT_MAP_TRANSPORT_STATE_DISCONNECTING);

	if (type == MAP_TRANSPORT_TYPE_L2CAP) {
		err = bt_goep_transport_l2cap_disconnect(&mce_mns->goep);
	} else {
		err = bt_goep_transport_rfcomm_disconnect(&mce_mns->goep);
	}

	if (err != 0) {
		LOG_ERR("Transport disconnect failed: %d", err);
		atomic_set(&mce_mns->_transport_state, BT_MAP_TRANSPORT_STATE_CONNECTED);
	}

	return err;
}

int bt_map_mce_mns_rfcomm_disconnect(struct bt_map_mce_mns *mce_mns)
{
	return mce_mns_transport_disconnect(mce_mns, MAP_TRANSPORT_TYPE_RFCOMM);
}

int bt_map_mce_mns_l2cap_disconnect(struct bt_map_mce_mns *mce_mns)
{
	return mce_mns_transport_disconnect(mce_mns, MAP_TRANSPORT_TYPE_L2CAP);
}

static void mce_mns_connected(struct bt_obex_server *server, uint8_t version, uint16_t mopl,
			      struct net_buf *buf)
{
	struct bt_map_mce_mns *s = CONTAINER_OF(server, struct bt_map_mce_mns, _server);

	LOG_DBG("MCE MNS connected, version: 0x%02x, mopl: %u", version, mopl);
	atomic_set(&s->_state, BT_MAP_STATE_CONNECTING);

	if ((s->_cb != NULL) && (s->_cb->connected)) {
		s->_cb->connected(s, version, mopl, buf);
	}
}

static void mce_mns_disconnected(struct bt_obex_server *server, struct net_buf *buf)
{
	struct bt_map_mce_mns *s = CONTAINER_OF(server, struct bt_map_mce_mns, _server);

	LOG_DBG("MCE MNS disconnected");
	atomic_set(&s->_state, BT_MAP_STATE_DISCONNECTING);

	if ((s->_cb != NULL) && (s->_cb->disconnected)) {
		s->_cb->disconnected(s, buf);
	}
}

static enum bt_obex_rsp_code mce_mns_get_req_cb(struct bt_map_mce_mns *mce_mns, struct net_buf *buf,
						bool is_get, mce_mns_cb_t *cb)
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
	ARRAY_FOR_EACH(map_mns_functions, i) {
		if (mce_mns->_optype != NULL && map_mns_functions[i].type != mce_mns->_optype) {
			continue;
		}

		if (strcmp(map_mns_functions[i].type, type) != 0) {
			continue;
		}

		if (map_mns_functions[i].op_get != is_get) {
			continue;
		}

		if (!has_required_hdrs(buf, &map_mns_functions[i].hdr)) {
			continue;
		}

		if (!has_required_app_params(buf, &map_mas_functions[i].ap)) {
			continue;
		}

		if (map_mns_functions[i].get_mce_mns_cb == NULL) {
			continue;
		}

		*cb = map_mns_functions[i].get_mce_mns_cb(mce_mns);
		if (*cb == NULL) {
			continue;
		}

		mce_mns->_optype = map_mns_functions[i].type;
		mce_mns->_opcode = is_get ? BT_OBEX_OPCODE_GET : BT_OBEX_OPCODE_PUT;
		break;
	}

	if (*cb == NULL) {
		LOG_WRN("Unsupported request");
		return BT_OBEX_RSP_CODE_NOT_IMPL;
	}

	return BT_OBEX_RSP_CODE_SUCCESS;
}

static void mce_mns_put(struct bt_obex_server *server, bool final, struct net_buf *buf)
{
	struct bt_map_mce_mns *s = CONTAINER_OF(server, struct bt_map_mce_mns, _server);
	int err;
	enum bt_obex_rsp_code rsp_code;

	LOG_DBG("MCE MNS PUT request, final: %d", final);

	if (s->_optype == NULL || bt_obex_has_header(buf, BT_OBEX_HEADER_ID_TYPE)) {
		rsp_code = mce_mns_get_req_cb(s, buf, false, &s->_req_cb);
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
	mce_mns_clear_pending_request(s);
	err = bt_obex_put_rsp(server, rsp_code, NULL);
	if (err != 0) {
		LOG_ERR("Failed to send put rsp %d", err);
	}
}

static void mce_mns_abort(struct bt_obex_server *server, struct net_buf *buf)
{
	struct bt_map_mce_mns *s = CONTAINER_OF(server, struct bt_map_mce_mns, _server);
	int err;

	LOG_DBG("MCE MNS abort request");

	if (s->_cb->abort != NULL) {
		s->_cb->abort(s, buf);
	}

	err = bt_obex_abort_rsp(server, BT_OBEX_RSP_CODE_NOT_IMPL, NULL);
	if (err != 0) {
		LOG_ERR("Failed to send abort rsp %d", err);
	}
}

static const struct bt_obex_server_ops mce_mns_ops = {
	.connect = mce_mns_connected,
	.disconnect = mce_mns_disconnected,
	.put = mce_mns_put,
	.get = NULL,
	.abort = mce_mns_abort,
	.setpath = NULL,
	.action = NULL,
};

#define MNS_RFCOMM_SERVER(server) CONTAINER_OF(server, struct bt_map_mce_mns_rfcomm_server, server);
static int mce_mns_rfcomm_accept(struct bt_conn *conn,
				 struct bt_goep_transport_rfcomm_server *server,
				 struct bt_goep **goep)
{
	struct bt_map_mce_mns_rfcomm_server *rfcomm_server = MNS_RFCOMM_SERVER(server);
	struct bt_map_mce_mns *mce_mns;
	int err;

	LOG_DBG("MCE MNS RFCOMM accept");

	if (rfcomm_server->accept == NULL) {
		return -ENOTSUP;
	}

	err = rfcomm_server->accept(conn, rfcomm_server, &mce_mns);
	if (err != 0) {
		LOG_WRN("Incoming connection rejected");
		return err;
	}

	if (mce_mns == NULL || mce_mns->_cb == NULL) {
		LOG_WRN("Invalid parameter");
		return -EINVAL;
	}

	mce_mns->_transport_type = MAP_TRANSPORT_TYPE_RFCOMM;
	mce_mns->goep.transport_ops = &mce_mns_transport_ops;
	mce_mns->_server.ops = &mce_mns_ops;
	mce_mns->_server.obex = &mce_mns->goep.obex;
	mce_mns->_conn_id = map_get_connect_id();
	err = bt_obex_server_register(&mce_mns->_server, map_mns_uuid);
	if (err != 0) {
		LOG_ERR("Failed to register obex server %d", err);
		return err;
	}
	*goep = &mce_mns->goep;
	atomic_set(&mce_mns->_transport_state, BT_MAP_TRANSPORT_STATE_CONNECTING);

	return 0;
}

int bt_map_mce_mns_rfcomm_register(struct bt_map_mce_mns_rfcomm_server *server)
{
	if (server == NULL || server->accept == NULL) {
		return -EINVAL;
	}

	LOG_DBG("Registering MCE MNS RFCOMM server");
	server->server.accept = mce_mns_rfcomm_accept;

	return bt_goep_transport_rfcomm_server_register(&server->server);
}

#define MNS_L2CAP_SERVER(server) CONTAINER_OF(server, struct bt_map_mce_mns_l2cap_server, server);
static int mce_mns_l2cap_accept(struct bt_conn *conn, struct bt_goep_transport_l2cap_server *server,
				struct bt_goep **goep)
{
	struct bt_map_mce_mns_l2cap_server *l2cap_server = MNS_L2CAP_SERVER(server);
	struct bt_map_mce_mns *mce_mns;
	int err;

	LOG_DBG("MCE MNS L2CAP accept");

	if (l2cap_server->accept == NULL) {
		return -ENOTSUP;
	}

	err = l2cap_server->accept(conn, l2cap_server, &mce_mns);
	if (err != 0) {
		LOG_WRN("Incoming connection rejected");
		return err;
	}

	if (mce_mns == NULL || mce_mns->_cb == NULL) {
		LOG_WRN("Invalid parameter");
		return -EINVAL;
	}

	mce_mns->_transport_type = MAP_TRANSPORT_TYPE_L2CAP;
	mce_mns->goep.transport_ops = &mce_mns_transport_ops;
	mce_mns->_server.ops = &mce_mns_ops;
	mce_mns->_server.obex = &mce_mns->goep.obex;
	mce_mns->_conn_id = map_get_connect_id();
	err = bt_obex_server_register(&mce_mns->_server, map_mns_uuid);
	if (err != 0) {
		LOG_ERR("Failed to register obex server %d", err);
		return err;
	}
	*goep = &mce_mns->goep;
	atomic_set(&mce_mns->_transport_state, BT_MAP_TRANSPORT_STATE_CONNECTING);

	return 0;
}

int bt_map_mce_mns_l2cap_register(struct bt_map_mce_mns_l2cap_server *server)
{
	if (server == NULL || server->accept == NULL) {
		return -EINVAL;
	}

	LOG_DBG("Registering MCE MNS L2CAP server");
	server->server.accept = mce_mns_l2cap_accept;

	return bt_goep_transport_l2cap_server_register(&server->server);
}

struct net_buf *bt_map_mce_mns_create_pdu(struct bt_map_mce_mns *mce_mns, struct net_buf_pool *pool)
{
	if (mce_mns == NULL) {
		LOG_ERR("Invalid parameter");
		return NULL;
	}

	return bt_goep_create_pdu(&mce_mns->goep, pool);
}

int bt_map_mce_mns_connect(struct bt_map_mce_mns *mce_mns, uint8_t rsp_code, struct net_buf *buf)
{
	uint8_t val[BT_UUID_SIZE_128];
	int err;
	bool allocated = false;

	if (mce_mns == NULL) {
		return -EINVAL;
	}

	if (atomic_get(&mce_mns->_state) != BT_MAP_STATE_CONNECTING) {
		LOG_ERR("Invalid state %u", (uint8_t)atomic_get(&mce_mns->_state));
		return -EINVAL;
	}

	LOG_DBG("MCE MNS connect response, rsp_code: 0x%02x", rsp_code);

	if (buf == NULL) {
		buf = bt_goep_create_pdu(&mce_mns->goep, NULL);
		if (buf == NULL) {
			LOG_ERR("Failed to allocate buffer");
			return -ENOBUFS;
		}
		allocated = true;
	} else {
		if (bt_obex_has_header(buf, BT_OBEX_HEADER_ID_WHO)) {
			err = map_check_who(&map_mns_uuid->uuid, buf);
			if (err != 0) {
				return err;
			}
		}

		if (bt_obex_has_header(buf, BT_OBEX_HEADER_ID_CONN_ID)) {
			err = map_check_conn_id(mce_mns->_conn_id, buf);
			if (err != 0) {
				return err;
			}
		}
	}

	if (rsp_code == BT_OBEX_RSP_CODE_SUCCESS) {
		if (!bt_obex_has_header(buf, BT_OBEX_HEADER_ID_WHO)) {
			sys_memcpy_swap(val, map_mns_uuid->val, sizeof(val));
			err = bt_obex_add_header_who(buf, sizeof(val), val);
			if (err != 0) {
				LOG_ERR("Failed to add header target %d", err);
				goto failed;
			}
		}

		if (!bt_obex_has_header(buf, BT_OBEX_HEADER_ID_CONN_ID)) {
			err = bt_obex_add_header_conn_id(buf, mce_mns->_conn_id);
			if (err != 0) {
				LOG_ERR("Failed to add header conn id %d", err);
				goto failed;
			}
		}
	}

	err = bt_obex_connect_rsp(&mce_mns->_server, rsp_code, mce_mns->goep.obex.rx.mtu, buf);
	if (err != 0) {
		LOG_ERR("Failed to send conn rsp %d", err);
		goto failed;
	}

	atomic_set(&mce_mns->_state, BT_MAP_STATE_CONNECTED);
	return 0;

failed:
	if (allocated) {
		net_buf_unref(buf);
	}
	return err;
}

int bt_map_mce_mns_disconnect(struct bt_map_mce_mns *mce_mns, uint8_t rsp_code, struct net_buf *buf)
{
	int err;

	if (mce_mns == NULL) {
		return -EINVAL;
	}

	if (atomic_get(&mce_mns->_state) != BT_MAP_STATE_DISCONNECTING) {
		LOG_ERR("Invalid state %u", (uint8_t)atomic_get(&mce_mns->_state));
		return -EINVAL;
	}

	LOG_DBG("MCE MNS disconnect response, rsp_code: 0x%02x", rsp_code);

	err = bt_obex_disconnect_rsp(&mce_mns->_server, rsp_code, buf);
	if (err != 0) {
		LOG_ERR("Failed to send conn rsp %d", err);
		return err;
	}

	if (rsp_code == BT_OBEX_RSP_CODE_SUCCESS) {
		atomic_set(&mce_mns->_state, BT_MAP_STATE_DISCONNECTED);
	} else {
		atomic_set(&mce_mns->_state, BT_MAP_STATE_CONNECTED);
	}
	return 0;
}

int bt_map_mce_mns_abort(struct bt_map_mce_mns *mce_mns, uint8_t rsp_code, struct net_buf *buf)
{
	int err;

	if (mce_mns == NULL) {
		return -EINVAL;
	}

	if (atomic_get(&mce_mns->_state) != BT_MAP_STATE_CONNECTED) {
		LOG_ERR("Invalid state %u", (uint8_t)atomic_get(&mce_mns->_state));
		return -EINVAL;
	}

	LOG_DBG("MCE MNS abort response, rsp_code: 0x%02x", rsp_code);

	err = bt_obex_abort_rsp(&mce_mns->_server, rsp_code, buf);
	if (err != 0) {
		LOG_ERR("Failed to send abort rsp %d", err);
		return err;
	}

	if (rsp_code == BT_OBEX_RSP_CODE_SUCCESS) {
		mce_mns_clear_pending_request(mce_mns);
	}

	return 0;
}

static int mce_mns_get_or_put_rsp(struct bt_map_mce_mns *mce_mns, bool is_get, const char *type,
				  uint8_t rsp_code, struct net_buf *buf)
{
	int err;

	if (mce_mns == NULL) {
		return -EINVAL;
	}

	if (atomic_get(&mce_mns->_state) != BT_MAP_STATE_CONNECTED) {
		LOG_ERR("Invalid state %u", (uint8_t)atomic_get(&mce_mns->_state));
		return -EINVAL;
	}

	if (mce_mns->_optype != NULL && strcmp(mce_mns->_optype, type) != 0) {
		LOG_ERR("Invalid operation type %s != %s", mce_mns->_optype, type);
		return -EINVAL;
	}

	LOG_DBG("MCE MNS %s response, type: %s, rsp_code: 0x%02x", is_get ? "GET" : "PUT", type,
		rsp_code);

	if (is_get) {
		if (mce_mns->_opcode != BT_OBEX_OPCODE_GET) {
			LOG_ERR("Operation %u != %u", mce_mns->_opcode, BT_OBEX_OPCODE_GET);
			return -EINVAL;
		}

		err = bt_obex_get_rsp(&mce_mns->_server, rsp_code, buf);
	} else {
		if (mce_mns->_opcode != BT_OBEX_OPCODE_PUT) {
			LOG_ERR("Operation %u != %u", mce_mns->_opcode, BT_OBEX_OPCODE_PUT);
			return -EINVAL;
		}

		err = bt_obex_put_rsp(&mce_mns->_server, rsp_code, buf);
	}

	if (err != 0) {
		LOG_ERR("Failed to send get/put rsp %d", err);
		return err;
	}

	if (rsp_code != BT_OBEX_RSP_CODE_CONTINUE) {
		mce_mns_clear_pending_request(mce_mns);
	}

	return 0;
}

int bt_map_mce_mns_send_event(struct bt_map_mce_mns *mce_mns, uint8_t rsp_code, struct net_buf *buf)
{
	return mce_mns_get_or_put_rsp(mce_mns, false, BT_MAP_HDR_TYPE_SEND_EVENT, rsp_code, buf);
}
#endif /* CONFIG_BT_MAP_MCE */

#if defined(CONFIG_BT_MAP_MSE)
int bt_map_mse_mas_cb_register(struct bt_map_mse_mas *mse_mas, const struct bt_map_mse_mas_cb *cb)
{
	if (mse_mas == NULL || cb == NULL) {
		return -EINVAL;
	}

	mse_mas->_cb = cb;

	LOG_DBG("MSE MAS callbacks registered");
	return 0;
}

static void mse_mas_clear_pending_request(struct bt_map_mse_mas *mse_mas)
{
	mse_mas->_opcode = 0;
	mse_mas->_optype = NULL;
	mse_mas->_req_cb = NULL;
}

static void mse_mas_transport_connected(struct bt_conn *conn, struct bt_goep *goep)
{
	struct bt_map_mse_mas *mse_mas = CONTAINER_OF(goep, struct bt_map_mse_mas, goep);

	LOG_DBG("MSE MAS transport connected");
	atomic_set(&mse_mas->_transport_state, BT_MAP_TRANSPORT_STATE_CONNECTED);

	if (mse_mas->_transport_type == MAP_TRANSPORT_TYPE_L2CAP) {
		if (mse_mas->_cb != NULL && mse_mas->_cb->l2cap_connected != NULL) {
			mse_mas->_cb->l2cap_connected(conn, mse_mas);
		}
	} else {
		if (mse_mas->_cb != NULL && mse_mas->_cb->rfcomm_connected != NULL) {
			mse_mas->_cb->rfcomm_connected(conn, mse_mas);
		}
	}
}

static void mse_mas_transport_disconnected(struct bt_goep *goep)
{
	struct bt_map_mse_mas *mse_mas = CONTAINER_OF(goep, struct bt_map_mse_mas, goep);
	int err;

	LOG_DBG("MSE MAS transport disconnected");
	atomic_set(&mse_mas->_transport_state, BT_MAP_TRANSPORT_STATE_DISCONNECTED);
	atomic_set(&mse_mas->_state, BT_MAP_STATE_DISCONNECTED);
	mse_mas_clear_pending_request(mse_mas);

	err = bt_obex_server_unregister(&mse_mas->_server);
	if (err != 0) {
		LOG_ERR("Failed to unregister obex server %d", err);
	}

	if (mse_mas->_transport_type == MAP_TRANSPORT_TYPE_L2CAP) {
		if (mse_mas->_cb != NULL && mse_mas->_cb->l2cap_disconnected != NULL) {
			mse_mas->_cb->l2cap_disconnected(mse_mas);
		}
	} else {
		if (mse_mas->_cb != NULL && mse_mas->_cb->rfcomm_disconnected != NULL) {
			mse_mas->_cb->rfcomm_disconnected(mse_mas);
		}
	}
}

static struct bt_goep_transport_ops mse_mas_transport_ops = {
	.connected = mse_mas_transport_connected,
	.disconnected = mse_mas_transport_disconnected,
};

static int mse_mas_transport_disconnect(struct bt_map_mse_mas *mse_mas, uint8_t type)
{
	int err;

	if (mse_mas == NULL) {
		return -EINVAL;
	}

	if (atomic_get(&mse_mas->_transport_state) != BT_MAP_TRANSPORT_STATE_CONNECTED) {
		LOG_DBG("MSE MAS transport disconnect in progress");
		return -EINPROGRESS;
	}

	LOG_DBG("MSE MAS transport disconnecting, type %u", type);
	atomic_set(&mse_mas->_transport_state, BT_MAP_TRANSPORT_STATE_DISCONNECTING);

	if (type == MAP_TRANSPORT_TYPE_L2CAP) {
		err = bt_goep_transport_l2cap_disconnect(&mse_mas->goep);
	} else {
		err = bt_goep_transport_rfcomm_disconnect(&mse_mas->goep);
	}

	if (err != 0) {
		LOG_ERR("Transport disconnect failed: %d", err);
		atomic_set(&mse_mas->_transport_state, BT_MAP_TRANSPORT_STATE_CONNECTED);
	}

	return err;
}

int bt_map_mse_mas_rfcomm_disconnect(struct bt_map_mse_mas *mse_mas)
{
	return mse_mas_transport_disconnect(mse_mas, MAP_TRANSPORT_TYPE_RFCOMM);
}

int bt_map_mse_mas_l2cap_disconnect(struct bt_map_mse_mas *mse_mas)
{
	return mse_mas_transport_disconnect(mse_mas, MAP_TRANSPORT_TYPE_L2CAP);
}

static void mse_mas_connected(struct bt_obex_server *server, uint8_t version, uint16_t mopl,
			      struct net_buf *buf)
{
	struct bt_map_mse_mas *s = CONTAINER_OF(server, struct bt_map_mse_mas, _server);

	LOG_DBG("MSE MAS connected, version: 0x%02x, mopl: %u", version, mopl);
	atomic_set(&s->_state, BT_MAP_STATE_CONNECTING);

	if ((s->_cb != NULL) && (s->_cb->connected)) {
		s->_cb->connected(s, version, mopl, buf);
	}
}

static void mse_mas_disconnected(struct bt_obex_server *server, struct net_buf *buf)
{
	struct bt_map_mse_mas *s = CONTAINER_OF(server, struct bt_map_mse_mas, _server);

	LOG_DBG("MSE MAS disconnected");
	atomic_set(&s->_state, BT_MAP_STATE_DISCONNECTING);

	if ((s->_cb != NULL) && (s->_cb->disconnected)) {
		s->_cb->disconnected(s, buf);
	}
}

static enum bt_obex_rsp_code mse_mas_get_req_cb(struct bt_map_mse_mas *mse_mas, struct net_buf *buf,
						bool is_get, mse_mas_cb_t *cb)
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
	ARRAY_FOR_EACH(map_mas_functions, i) {
		if (mse_mas->_optype != NULL && map_mas_functions[i].type != mse_mas->_optype) {
			continue;
		}

		if (strcmp(map_mas_functions[i].type, type) != 0) {
			continue;
		}

		if (map_mas_functions[i].op_get != is_get) {
			continue;
		}

		if (!has_required_hdrs(buf, &map_mas_functions[i].hdr)) {
			continue;
		}

		if (!has_required_app_params(buf, &map_mas_functions[i].ap)) {
			continue;
		}

		if (map_mas_functions[i].get_mse_mas_cb == NULL) {
			continue;
		}

		*cb = map_mas_functions[i].get_mse_mas_cb(mse_mas);
		if (*cb == NULL) {
			continue;
		}

		mse_mas->_optype = map_mas_functions[i].type;
		mse_mas->_opcode = is_get ? BT_OBEX_OPCODE_GET : BT_OBEX_OPCODE_PUT;
		break;
	}

	if (*cb == NULL) {
		LOG_WRN("Unsupported request");
		return BT_OBEX_RSP_CODE_NOT_IMPL;
	}

	return BT_OBEX_RSP_CODE_SUCCESS;
}

static void mse_mas_put(struct bt_obex_server *server, bool final, struct net_buf *buf)
{
	struct bt_map_mse_mas *s = CONTAINER_OF(server, struct bt_map_mse_mas, _server);
	int err;
	enum bt_obex_rsp_code rsp_code;

	LOG_DBG("MSE MAS PUT request, final: %d", final);

	if (s->_optype == NULL || bt_obex_has_header(buf, BT_OBEX_HEADER_ID_TYPE)) {
		rsp_code = mse_mas_get_req_cb(s, buf, false, &s->_req_cb);
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
	mse_mas_clear_pending_request(s);
	err = bt_obex_put_rsp(server, rsp_code, NULL);
	if (err != 0) {
		LOG_ERR("Failed to send put rsp %d", err);
	}
}

static void mse_mas_get(struct bt_obex_server *server, bool final, struct net_buf *buf)
{
	struct bt_map_mse_mas *s = CONTAINER_OF(server, struct bt_map_mse_mas, _server);
	int err;
	enum bt_obex_rsp_code rsp_code;

	LOG_DBG("MSE MAS GET request, final: %d", final);

	if (s->_optype == NULL || bt_obex_has_header(buf, BT_OBEX_HEADER_ID_TYPE)) {
		rsp_code = mse_mas_get_req_cb(s, buf, true, &s->_req_cb);
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
	mse_mas_clear_pending_request(s);
	err = bt_obex_get_rsp(server, rsp_code, NULL);
	if (err != 0) {
		LOG_ERR("Failed to send get rsp %d", err);
	}
}

static void mse_mas_abort(struct bt_obex_server *server, struct net_buf *buf)
{
	struct bt_map_mse_mas *s = CONTAINER_OF(server, struct bt_map_mse_mas, _server);
	int err;

	LOG_DBG("MSE MAS abort request");

	if (s->_cb->abort != NULL) {
		s->_cb->abort(s, buf);
	} else {
		LOG_WRN("No cb for abort req");
		err = bt_obex_abort_rsp(server, BT_OBEX_RSP_CODE_NOT_IMPL, NULL);
		if (err != 0) {
			LOG_ERR("Failed to send abort rsp %d", err);
		}
	}
}

static void mse_mas_setpath(struct bt_obex_server *server, uint8_t flags, struct net_buf *buf)
{
	struct bt_map_mse_mas *s = CONTAINER_OF(server, struct bt_map_mse_mas, _server);
	int err;

	LOG_DBG("MSE MAS setpath request, flags: 0x%02x", flags);

	if ((s->_cb != NULL) && (s->_cb->set_folder)) {
		s->_cb->set_folder(s, flags, buf);
	} else {
		LOG_WRN("No cb for set_folder req");
		err = bt_obex_setpath_rsp(server, BT_OBEX_RSP_CODE_NOT_IMPL, NULL);
		if (err != 0) {
			LOG_ERR("Failed to send set_folder rsp %d", err);
		}
	}
}

static const struct bt_obex_server_ops mse_mas_ops = {
	.connect = mse_mas_connected,
	.disconnect = mse_mas_disconnected,
	.put = mse_mas_put,
	.get = mse_mas_get,
	.abort = mse_mas_abort,
	.setpath = mse_mas_setpath,
	.action = NULL,
};

#define MAS_RFCOMM_SERVER(server) CONTAINER_OF(server, struct bt_map_mse_mas_rfcomm_server, server);
static int mse_mas_rfcomm_accept(struct bt_conn *conn,
				 struct bt_goep_transport_rfcomm_server *server,
				 struct bt_goep **goep)
{
	struct bt_map_mse_mas_rfcomm_server *rfcomm_server = MAS_RFCOMM_SERVER(server);
	struct bt_map_mse_mas *mse_mas;
	int err;

	LOG_DBG("MSE MAS RFCOMM accept");

	if (rfcomm_server->accept == NULL) {
		return -ENOTSUP;
	}

	err = rfcomm_server->accept(conn, rfcomm_server, &mse_mas);
	if (err != 0) {
		LOG_WRN("Incoming connection rejected");
		return err;
	}

	if (mse_mas == NULL || mse_mas->_cb == NULL) {
		LOG_WRN("Invalid parameter");
		return -EINVAL;
	}

	mse_mas->_transport_type = MAP_TRANSPORT_TYPE_RFCOMM;
	mse_mas->goep.transport_ops = &mse_mas_transport_ops;
	mse_mas->_server.ops = &mse_mas_ops;
	mse_mas->_server.obex = &mse_mas->goep.obex;
	mse_mas->_conn_id = map_get_connect_id();
	err = bt_obex_server_register(&mse_mas->_server, map_mas_uuid);
	if (err != 0) {
		LOG_ERR("Failed to register obex server %d", err);
		return err;
	}
	*goep = &mse_mas->goep;
	atomic_set(&mse_mas->_transport_state, BT_MAP_TRANSPORT_STATE_CONNECTING);

	return 0;
}

int bt_map_mse_mas_rfcomm_register(struct bt_map_mse_mas_rfcomm_server *server)
{
	if (server == NULL || server->accept == NULL) {
		return -EINVAL;
	}

	LOG_DBG("Registering MSE MAS RFCOMM server");
	server->server.accept = mse_mas_rfcomm_accept;

	return bt_goep_transport_rfcomm_server_register(&server->server);
}

#define MAS_L2CAP_SERVER(server) CONTAINER_OF(server, struct bt_map_mse_mas_l2cap_server, server);
static int mse_mas_l2cap_accept(struct bt_conn *conn, struct bt_goep_transport_l2cap_server *server,
				struct bt_goep **goep)
{
	struct bt_map_mse_mas_l2cap_server *l2cap_server = MAS_L2CAP_SERVER(server);
	struct bt_map_mse_mas *mse_mas;
	int err;

	LOG_DBG("MSE MAS L2CAP accept");

	if (l2cap_server->accept == NULL) {
		return -ENOTSUP;
	}

	err = l2cap_server->accept(conn, l2cap_server, &mse_mas);
	if (err != 0) {
		LOG_WRN("Incoming connection rejected");
		return err;
	}

	if (mse_mas == NULL || mse_mas->_cb == NULL) {
		LOG_WRN("Invalid parameter");
		return -EINVAL;
	}

	mse_mas->_transport_type = MAP_TRANSPORT_TYPE_L2CAP;
	mse_mas->goep.transport_ops = &mse_mas_transport_ops;
	mse_mas->_server.ops = &mse_mas_ops;
	mse_mas->_server.obex = &mse_mas->goep.obex;
	mse_mas->_conn_id = map_get_connect_id();
	err = bt_obex_server_register(&mse_mas->_server, map_mas_uuid);
	if (err != 0) {
		LOG_ERR("Failed to register obex server %d", err);
		return err;
	}
	*goep = &mse_mas->goep;
	atomic_set(&mse_mas->_transport_state, BT_MAP_TRANSPORT_STATE_CONNECTING);

	return 0;
}

int bt_map_mse_mas_l2cap_register(struct bt_map_mse_mas_l2cap_server *server)
{
	if (server == NULL || server->accept == NULL) {
		return -EINVAL;
	}

	LOG_DBG("Registering MSE MAS L2CAP server");
	server->server.accept = mse_mas_l2cap_accept;

	return bt_goep_transport_l2cap_server_register(&server->server);
}

struct net_buf *bt_map_mse_mas_create_pdu(struct bt_map_mse_mas *mse_mas, struct net_buf_pool *pool)
{
	if (mse_mas == NULL) {
		LOG_ERR("Invalid parameter");
		return NULL;
	}

	return bt_goep_create_pdu(&mse_mas->goep, pool);
}

int bt_map_mse_mas_connect(struct bt_map_mse_mas *mse_mas, uint8_t rsp_code, struct net_buf *buf)
{
	uint8_t val[BT_UUID_SIZE_128];
	int err;
	bool allocated = false;

	if (mse_mas == NULL) {
		return -EINVAL;
	}

	if (atomic_get(&mse_mas->_state) != BT_MAP_STATE_CONNECTING) {
		LOG_ERR("Invalid state %u", (uint8_t)atomic_get(&mse_mas->_state));
		return -EINVAL;
	}

	LOG_DBG("MSE MAS connect response, rsp_code: 0x%02x", rsp_code);

	if (buf == NULL) {
		buf = bt_goep_create_pdu(&mse_mas->goep, NULL);
		if (buf == NULL) {
			LOG_ERR("Failed to allocate buffer");
			return -ENOBUFS;
		}
		allocated = true;
	} else {
		if (bt_obex_has_header(buf, BT_OBEX_HEADER_ID_WHO)) {
			err = map_check_who(&map_mas_uuid->uuid, buf);
			if (err != 0) {
				return err;
			}
		}

		if (bt_obex_has_header(buf, BT_OBEX_HEADER_ID_CONN_ID)) {
			err = map_check_conn_id(mse_mas->_conn_id, buf);
			if (err != 0) {
				return err;
			}
		}
	}

	if (rsp_code == BT_OBEX_RSP_CODE_SUCCESS) {
		if (!bt_obex_has_header(buf, BT_OBEX_HEADER_ID_WHO)) {
			sys_memcpy_swap(val, map_mas_uuid->val, sizeof(val));
			err = bt_obex_add_header_who(buf, sizeof(val), val);
			if (err != 0) {
				LOG_ERR("Failed to add header who %d", err);
				goto failed;
			}
		}

		if (!bt_obex_has_header(buf, BT_OBEX_HEADER_ID_CONN_ID)) {
			err = bt_obex_add_header_conn_id(buf, mse_mas->_conn_id);
			if (err != 0) {
				LOG_ERR("Failed to add header conn id %d", err);
				goto failed;
			}
		}
	}

	err = bt_obex_connect_rsp(&mse_mas->_server, rsp_code, mse_mas->goep.obex.rx.mtu, buf);
	if (err != 0) {
		LOG_ERR("Failed to send conn rsp %d", err);
		goto failed;
	}

	atomic_set(&mse_mas->_state, BT_MAP_STATE_CONNECTED);
	return 0;

failed:
	if (allocated) {
		net_buf_unref(buf);
	}
	return err;
}

int bt_map_mse_mas_disconnect(struct bt_map_mse_mas *mse_mas, uint8_t rsp_code, struct net_buf *buf)
{
	int err;

	if (mse_mas == NULL) {
		return -EINVAL;
	}

	if (atomic_get(&mse_mas->_state) != BT_MAP_STATE_DISCONNECTING) {
		LOG_ERR("Invalid state %u", (uint8_t)atomic_get(&mse_mas->_state));
		return -EINVAL;
	}

	LOG_DBG("MSE MAS disconnect response, rsp_code: 0x%02x", rsp_code);

	err = bt_obex_disconnect_rsp(&mse_mas->_server, rsp_code, buf);
	if (err != 0) {
		LOG_ERR("Failed to send disconnect rsp %d", err);
		return err;
	}

	if (rsp_code == BT_OBEX_RSP_CODE_SUCCESS) {
		atomic_set(&mse_mas->_state, BT_MAP_STATE_DISCONNECTED);
	} else {
		atomic_set(&mse_mas->_state, BT_MAP_STATE_CONNECTED);
	}
	return 0;
}

int bt_map_mse_mas_abort(struct bt_map_mse_mas *mse_mas, uint8_t rsp_code, struct net_buf *buf)
{
	int err;

	if (mse_mas == NULL) {
		return -EINVAL;
	}

	if (atomic_get(&mse_mas->_state) != BT_MAP_STATE_CONNECTED) {
		LOG_ERR("Invalid state %u", (uint8_t)atomic_get(&mse_mas->_state));
		return -EINVAL;
	}

	LOG_DBG("MSE MAS abort response, rsp_code: 0x%02x", rsp_code);

	err = bt_obex_abort_rsp(&mse_mas->_server, rsp_code, buf);
	if (err != 0) {
		LOG_ERR("Failed to send abort rsp %d", err);
		return err;
	}

	if (rsp_code == BT_OBEX_RSP_CODE_SUCCESS) {
		mse_mas_clear_pending_request(mse_mas);
	}

	return 0;
}

int bt_map_mse_mas_set_folder(struct bt_map_mse_mas *mse_mas, uint8_t rsp_code, struct net_buf *buf)
{
	int err;

	if (mse_mas == NULL) {
		return -EINVAL;
	}

	if (atomic_get(&mse_mas->_state) != BT_MAP_STATE_CONNECTED) {
		LOG_ERR("Invalid state %u", (uint8_t)atomic_get(&mse_mas->_state));
		return -EINVAL;
	}

	LOG_DBG("MSE MAS set folder response, rsp_code: 0x%02x", rsp_code);

	err = bt_obex_setpath_rsp(&mse_mas->_server, rsp_code, buf);
	if (err != 0) {
		LOG_ERR("Failed to send setpath rsp %d", err);
		return err;
	}

	return 0;
}

static int mse_mas_get_or_put_rsp(struct bt_map_mse_mas *mse_mas, bool is_get, const char *type,
				  uint8_t rsp_code, struct net_buf *buf)
{
	int err;

	if (mse_mas == NULL) {
		return -EINVAL;
	}

	if (atomic_get(&mse_mas->_state) != BT_MAP_STATE_CONNECTED) {
		LOG_ERR("Invalid state %u", (uint8_t)atomic_get(&mse_mas->_state));
		return -EINVAL;
	}

	if (mse_mas->_optype != NULL && strcmp(mse_mas->_optype, type) != 0) {
		LOG_ERR("Invalid operation type %s != %s", mse_mas->_optype, type);
		return -EINVAL;
	}

	LOG_DBG("MSE MAS %s response, type: %s, rsp_code: 0x%02x", is_get ? "GET" : "PUT", type,
		rsp_code);

	if (is_get) {
		if (mse_mas->_opcode != BT_OBEX_OPCODE_GET) {
			LOG_ERR("Operation %u != %u", mse_mas->_opcode, BT_OBEX_OPCODE_GET);
			return -EINVAL;
		}

		err = bt_obex_get_rsp(&mse_mas->_server, rsp_code, buf);
	} else {
		if (mse_mas->_opcode != BT_OBEX_OPCODE_PUT) {
			LOG_ERR("Operation %u != %u", mse_mas->_opcode, BT_OBEX_OPCODE_PUT);
			return -EINVAL;
		}

		err = bt_obex_put_rsp(&mse_mas->_server, rsp_code, buf);
	}

	if (err != 0) {
		LOG_ERR("Failed to send get/put rsp %d", err);
		return err;
	}

	if (rsp_code != BT_OBEX_RSP_CODE_CONTINUE) {
		mse_mas_clear_pending_request(mse_mas);
	}

	return 0;
}

int bt_map_mse_mas_set_ntf_reg(struct bt_map_mse_mas *mse_mas, uint8_t rsp_code,
			       struct net_buf *buf)
{
	return mse_mas_get_or_put_rsp(mse_mas, false, BT_MAP_HDR_TYPE_SET_NTF_REG, rsp_code, buf);
}

int bt_map_mse_mas_get_folder_listing(struct bt_map_mse_mas *mse_mas, uint8_t rsp_code,
				      struct net_buf *buf)
{
	return mse_mas_get_or_put_rsp(mse_mas, true, BT_MAP_HDR_TYPE_GET_FOLDER_LISTING, rsp_code,
				      buf);
}

int bt_map_mse_mas_get_msg_listing(struct bt_map_mse_mas *mse_mas, uint8_t rsp_code,
				   struct net_buf *buf)
{
	return mse_mas_get_or_put_rsp(mse_mas, true, BT_MAP_HDR_TYPE_GET_MSG_LISTING, rsp_code,
				      buf);
}

int bt_map_mse_mas_get_msg(struct bt_map_mse_mas *mse_mas, uint8_t rsp_code, struct net_buf *buf)
{
	return mse_mas_get_or_put_rsp(mse_mas, true, BT_MAP_HDR_TYPE_GET_MSG, rsp_code, buf);
}

int bt_map_mse_mas_set_msg_status(struct bt_map_mse_mas *mse_mas, uint8_t rsp_code,
				  struct net_buf *buf)
{
	return mse_mas_get_or_put_rsp(mse_mas, false, BT_MAP_HDR_TYPE_SET_MSG_STATUS, rsp_code,
				      buf);
}

int bt_map_mse_mas_push_msg(struct bt_map_mse_mas *mse_mas, uint8_t rsp_code, struct net_buf *buf)
{
	return mse_mas_get_or_put_rsp(mse_mas, false, BT_MAP_HDR_TYPE_PUSH_MSG, rsp_code, buf);
}

int bt_map_mse_mas_update_inbox(struct bt_map_mse_mas *mse_mas, uint8_t rsp_code,
				struct net_buf *buf)
{
	return mse_mas_get_or_put_rsp(mse_mas, false, BT_MAP_HDR_TYPE_UPDATE_INBOX, rsp_code, buf);
}

int bt_map_mse_mas_get_mas_inst_info(struct bt_map_mse_mas *mse_mas, uint8_t rsp_code,
				     struct net_buf *buf)
{
	return mse_mas_get_or_put_rsp(mse_mas, true, BT_MAP_HDR_TYPE_GET_MAS_INST_INFO, rsp_code,
				      buf);
}

int bt_map_mse_mas_set_owner_status(struct bt_map_mse_mas *mse_mas, uint8_t rsp_code,
				    struct net_buf *buf)
{
	return mse_mas_get_or_put_rsp(mse_mas, false, BT_MAP_HDR_TYPE_SET_OWNER_STATUS, rsp_code,
				      buf);
}

int bt_map_mse_mas_get_owner_status(struct bt_map_mse_mas *mse_mas, uint8_t rsp_code,
				    struct net_buf *buf)
{
	return mse_mas_get_or_put_rsp(mse_mas, true, BT_MAP_HDR_TYPE_GET_OWNER_STATUS, rsp_code,
				      buf);
}

int bt_map_mse_mas_get_convo_listing(struct bt_map_mse_mas *mse_mas, uint8_t rsp_code,
				     struct net_buf *buf)
{
	return mse_mas_get_or_put_rsp(mse_mas, true, BT_MAP_HDR_TYPE_GET_CONVO_LISTING, rsp_code,
				      buf);
}

int bt_map_mse_mas_set_ntf_filter(struct bt_map_mse_mas *mse_mas, uint8_t rsp_code,
				  struct net_buf *buf)
{
	return mse_mas_get_or_put_rsp(mse_mas, false, BT_MAP_HDR_TYPE_SET_NTF_FILTER, rsp_code,
				      buf);
}

/* MSE MNS Implementation */
int bt_map_mse_mns_cb_register(struct bt_map_mse_mns *mse_mns, const struct bt_map_mse_mns_cb *cb)
{
	if (mse_mns == NULL || cb == NULL) {
		return -EINVAL;
	}

	mse_mns->_cb = cb;

	LOG_DBG("MSE MNS callbacks registered");
	return 0;
}

static void mse_mns_clear_pending_request(struct bt_map_mse_mns *mse_mns)
{
	mse_mns->_rsp_cb = NULL;
	mse_mns->_req_type = NULL;
}

static void mse_mns_transport_connected(struct bt_conn *conn, struct bt_goep *goep)
{
	struct bt_map_mse_mns *mse_mns = CONTAINER_OF(goep, struct bt_map_mse_mns, goep);

	LOG_DBG("MSE MNS transport connected");
	atomic_set(&mse_mns->_transport_state, BT_MAP_TRANSPORT_STATE_CONNECTED);

	if (mse_mns->_transport_type == MAP_TRANSPORT_TYPE_L2CAP) {
		if (mse_mns->_cb != NULL && mse_mns->_cb->l2cap_connected != NULL) {
			mse_mns->_cb->l2cap_connected(conn, mse_mns);
		}
	} else {
		if (mse_mns->_cb != NULL && mse_mns->_cb->rfcomm_connected != NULL) {
			mse_mns->_cb->rfcomm_connected(conn, mse_mns);
		}
	}
}

static void mse_mns_transport_disconnected(struct bt_goep *goep)
{
	struct bt_map_mse_mns *mse_mns = CONTAINER_OF(goep, struct bt_map_mse_mns, goep);

	LOG_DBG("MSE MNS transport disconnected");
	atomic_set(&mse_mns->_transport_state, BT_MAP_TRANSPORT_STATE_DISCONNECTED);
	atomic_set(&mse_mns->_state, BT_MAP_STATE_DISCONNECTED);
	mse_mns_clear_pending_request(mse_mns);

	if (mse_mns->_transport_type == MAP_TRANSPORT_TYPE_L2CAP) {
		if (mse_mns->_cb != NULL && mse_mns->_cb->l2cap_disconnected != NULL) {
			mse_mns->_cb->l2cap_disconnected(mse_mns);
		}
	} else {
		if (mse_mns->_cb != NULL && mse_mns->_cb->rfcomm_disconnected != NULL) {
			mse_mns->_cb->rfcomm_disconnected(mse_mns);
		}
	}
}

static struct bt_goep_transport_ops mse_mns_transport_ops = {
	.connected = mse_mns_transport_connected,
	.disconnected = mse_mns_transport_disconnected,
};

static int mse_mns_transport_connect(struct bt_conn *conn, struct bt_map_mse_mns *mse_mns,
				     uint8_t type, uint16_t psm, uint8_t channel)
{
	int err;

	if (conn == NULL || mse_mns == NULL) {
		return -EINVAL;
	}

	if (atomic_get(&mse_mns->_transport_state) != BT_MAP_TRANSPORT_STATE_DISCONNECTED) {
		LOG_DBG("MSE MNS transport connect in progress");
		return -EINPROGRESS;
	}

	LOG_DBG("MSE MNS transport connecting, type %u", type);
	mse_mns->_transport_type = type;
	mse_mns->goep.transport_ops = &mse_mns_transport_ops;
	atomic_set(&mse_mns->_transport_state, BT_MAP_TRANSPORT_STATE_CONNECTING);

	if (type == MAP_TRANSPORT_TYPE_L2CAP) {
		LOG_DBG("Connecting via L2CAP PSM 0x%04x", psm);
		err = bt_goep_transport_l2cap_connect(conn, &mse_mns->goep, psm);
	} else {
		LOG_DBG("Connecting via RFCOMM channel %u", channel);
		err = bt_goep_transport_rfcomm_connect(conn, &mse_mns->goep, channel);
	}

	if (err != 0) {
		LOG_ERR("Transport connect failed: %d", err);
		atomic_set(&mse_mns->_transport_state, BT_MAP_TRANSPORT_STATE_DISCONNECTED);
	}

	return err;
}

int bt_map_mse_mns_rfcomm_connect(struct bt_conn *conn, struct bt_map_mse_mns *mse_mns,
				  uint8_t channel)
{
	if (channel == 0) {
		return -EINVAL;
	}

	return mse_mns_transport_connect(conn, mse_mns, MAP_TRANSPORT_TYPE_RFCOMM, 0, channel);
}

int bt_map_mse_mns_l2cap_connect(struct bt_conn *conn, struct bt_map_mse_mns *mse_mns, uint16_t psm)
{
	if (psm == 0) {
		return -EINVAL;
	}

	return mse_mns_transport_connect(conn, mse_mns, MAP_TRANSPORT_TYPE_L2CAP, psm, 0);
}

static int mse_mns_transport_disconnect(struct bt_map_mse_mns *mse_mns, uint8_t type)
{
	int err;

	if (mse_mns == NULL) {
		return -EINVAL;
	}

	if (atomic_get(&mse_mns->_transport_state) != BT_MAP_TRANSPORT_STATE_CONNECTED) {
		LOG_DBG("MSE MNS transport disconnect in progress");
		return -EINPROGRESS;
	}

	LOG_DBG("MSE MNS transport disconnecting, type %u", type);
	atomic_set(&mse_mns->_transport_state, BT_MAP_TRANSPORT_STATE_DISCONNECTING);

	if (type == MAP_TRANSPORT_TYPE_L2CAP) {
		err = bt_goep_transport_l2cap_disconnect(&mse_mns->goep);
	} else {
		err = bt_goep_transport_rfcomm_disconnect(&mse_mns->goep);
	}

	if (err != 0) {
		LOG_ERR("Transport disconnect failed: %d", err);
		atomic_set(&mse_mns->_transport_state, BT_MAP_TRANSPORT_STATE_CONNECTED);
	}

	return err;
}

int bt_map_mse_mns_rfcomm_disconnect(struct bt_map_mse_mns *mse_mns)
{
	return mse_mns_transport_disconnect(mse_mns, MAP_TRANSPORT_TYPE_RFCOMM);
}

int bt_map_mse_mns_l2cap_disconnect(struct bt_map_mse_mns *mse_mns)
{
	return mse_mns_transport_disconnect(mse_mns, MAP_TRANSPORT_TYPE_L2CAP);
}

static void mse_mns_connected(struct bt_obex_client *client, uint8_t rsp_code, uint8_t version,
			      uint16_t mopl, struct net_buf *buf)
{
	struct bt_map_mse_mns *c = CONTAINER_OF(client, struct bt_map_mse_mns, _client);

	LOG_DBG("MSE MNS connected, rsp_code: 0x%02x, version: 0x%02x, mopl: %u", rsp_code, version,
		mopl);

	if (rsp_code == BT_OBEX_RSP_CODE_SUCCESS) {
		atomic_set(&c->_state, BT_MAP_STATE_CONNECTED);
		bt_obex_get_header_conn_id(buf, &c->_conn_id);
		LOG_DBG("Connection ID: %u", c->_conn_id);
	} else {
		atomic_set(&c->_state, BT_MAP_STATE_DISCONNECTED);
	}

	if ((c->_cb != NULL) && (c->_cb->connected)) {
		c->_cb->connected(c, rsp_code, version, mopl, buf);
	}
}

static void mse_mns_disconnected(struct bt_obex_client *client, uint8_t rsp_code,
				 struct net_buf *buf)
{
	struct bt_map_mse_mns *c = CONTAINER_OF(client, struct bt_map_mse_mns, _client);

	LOG_DBG("MSE MNS disconnected, rsp_code: 0x%02x", rsp_code);

	if (rsp_code == BT_OBEX_RSP_CODE_SUCCESS) {
		atomic_set(&c->_state, BT_MAP_STATE_DISCONNECTED);
		mse_mns_clear_pending_request(c);
	} else {
		atomic_set(&c->_state, BT_MAP_STATE_CONNECTED);
	}

	if ((c->_cb != NULL) && (c->_cb->disconnected)) {
		c->_cb->disconnected(c, rsp_code, buf);
	}
}

static void mse_mns_put(struct bt_obex_client *client, uint8_t rsp_code, struct net_buf *buf)
{
	struct bt_map_mse_mns *c = CONTAINER_OF(client, struct bt_map_mse_mns, _client);

	LOG_DBG("MSE MNS PUT response, rsp_code: 0x%02x", rsp_code);

	if (c->_rsp_cb != NULL) {
		c->_rsp_cb(c, rsp_code, buf);
	} else {
		LOG_WRN("No cb for rsp");
	}

	if (rsp_code != BT_OBEX_RSP_CODE_CONTINUE) {
		mse_mns_clear_pending_request(c);
	}
}

static void mse_mns_abort(struct bt_obex_client *client, uint8_t rsp_code, struct net_buf *buf)
{
	struct bt_map_mse_mns *c = CONTAINER_OF(client, struct bt_map_mse_mns, _client);
	int err;

	LOG_DBG("MSE MNS abort response, rsp_code: 0x%02x", rsp_code);

	if ((c->_cb != NULL) && (c->_cb->abort != NULL)) {
		c->_cb->abort(c, rsp_code, buf);
	}

	if (rsp_code == BT_OBEX_RSP_CODE_SUCCESS) {
		mse_mns_clear_pending_request(c);
		return;
	}

	err = bt_map_mse_mns_disconnect(c, NULL);
	if (err != 0) {
		LOG_ERR("Failed to send MSE MNS disconnect");
	}
}

static const struct bt_obex_client_ops mse_mns_ops = {
	.connect = mse_mns_connected,
	.disconnect = mse_mns_disconnected,
	.put = mse_mns_put,
	.get = NULL,
	.abort = mse_mns_abort,
	.setpath = NULL,
	.action = NULL,
};

struct net_buf *bt_map_mse_mns_create_pdu(struct bt_map_mse_mns *mse_mns, struct net_buf_pool *pool)
{
	if (mse_mns == NULL) {
		LOG_ERR("Invalid parameter");
		return NULL;
	}

	return bt_goep_create_pdu(&mse_mns->goep, pool);
}

int bt_map_mse_mns_connect(struct bt_map_mse_mns *mse_mns, struct net_buf *buf)
{
	uint8_t val[BT_UUID_SIZE_128];
	int err;
	bool allocated = false;

	if (mse_mns == NULL) {
		return -EINVAL;
	}

	if (atomic_get(&mse_mns->_state) != BT_MAP_STATE_DISCONNECTED) {
		LOG_ERR("Invalid state %u", (uint8_t)atomic_get(&mse_mns->_state));
		return -EINVAL;
	}

	LOG_DBG("MSE MNS connecting");

	if (buf == NULL) {
		buf = bt_goep_create_pdu(&mse_mns->goep, NULL);
		if (buf == NULL) {
			LOG_ERR("Failed to allocate buffer");
			return -ENOBUFS;
		}
		allocated = true;
	}

	if (bt_obex_has_header(buf, BT_OBEX_HEADER_ID_TARGET)) {
		err = map_check_target(&map_mns_uuid->uuid, buf);
		if (err != 0) {
			goto failed;
		}
	} else {
		sys_memcpy_swap(val, map_mns_uuid->val, sizeof(val));
		err = bt_obex_add_header_target(buf, sizeof(val), val);
		if (err != 0) {
			LOG_ERR("Failed to add header target");
			goto failed;
		}
	}

	mse_mns->_client.ops = &mse_mns_ops;
	mse_mns->_client.obex = &mse_mns->goep.obex;

	err = bt_obex_connect(&mse_mns->_client, mse_mns->goep.obex.rx.mtu, buf);
	if (err != 0) {
		LOG_ERR("Failed to send conn req: %d", err);
		goto failed;
	}

	atomic_set(&mse_mns->_state, BT_MAP_STATE_CONNECTING);
	return 0;

failed:
	if (allocated) {
		net_buf_unref(buf);
	}
	return err;
}

int bt_map_mse_mns_disconnect(struct bt_map_mse_mns *mse_mns, struct net_buf *buf)
{
	int err;
	bool allocated = false;

	if (mse_mns == NULL) {
		return -EINVAL;
	}

	if (atomic_get(&mse_mns->_state) != BT_MAP_STATE_CONNECTED) {
		LOG_ERR("Invalid state %u", (uint8_t)atomic_get(&mse_mns->_state));
		return -EINVAL;
	}

	LOG_DBG("MSE MNS disconnecting");

	if (buf == NULL) {
		buf = bt_goep_create_pdu(&mse_mns->goep, NULL);
		if (buf == NULL) {
			LOG_ERR("Failed to allocate buffer");
			return -ENOBUFS;
		}
		allocated = true;
	}

	if (!bt_obex_has_header(buf, BT_OBEX_HEADER_ID_CONN_ID)) {
		err = bt_obex_add_header_conn_id(buf, mse_mns->_conn_id);
		if (err != 0) {
			LOG_ERR("Failed to add header conn id %d", err);
			goto failed;
		}
	} else {
		err = map_check_conn_id(mse_mns->_conn_id, buf);
		if (err != 0) {
			goto failed;
		}
	}

	err = bt_obex_disconnect(&mse_mns->_client, buf);
	if (err != 0) {
		LOG_ERR("Failed to send conn rsp %d", err);
		goto failed;
	}

	atomic_set(&mse_mns->_state, BT_MAP_STATE_DISCONNECTING);
	return 0;
failed:
	if (allocated) {
		net_buf_unref(buf);
	}
	return err;
}

int bt_map_mse_mns_abort(struct bt_map_mse_mns *mse_mns, struct net_buf *buf)
{
	int err;
	bool allocated = false;

	if (mse_mns == NULL) {
		return -EINVAL;
	}

	if (atomic_get(&mse_mns->_state) != BT_MAP_STATE_CONNECTED) {
		LOG_ERR("Invalid state %u", (uint8_t)atomic_get(&mse_mns->_state));
		return -EINVAL;
	}

	if (mse_mns->_rsp_cb == NULL) {
		LOG_ERR("No operation is ongoing");
		return -EINVAL;
	}

	LOG_DBG("MSE MNS aborting");

	if (buf == NULL) {
		buf = bt_goep_create_pdu(&mse_mns->goep, NULL);
		if (buf == NULL) {
			LOG_ERR("Failed to allocate buffer");
			return -ENOBUFS;
		}
		allocated = true;
	}

	if (bt_obex_has_header(buf, BT_OBEX_HEADER_ID_CONN_ID)) {
		err = map_check_conn_id(mse_mns->_conn_id, buf);
		if (err != 0) {
			goto failed;
		}
	} else {
		err = bt_obex_add_header_conn_id(buf, mse_mns->_conn_id);
		if (err != 0) {
			LOG_ERR("Failed to add header conn id %d", err);
			goto failed;
		}
	}

	err = bt_obex_abort(&mse_mns->_client, buf);
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

static int mse_mns_get_req_cb(struct bt_map_mse_mns *mse_mns, const char *type, struct net_buf *buf,
			      bool is_get, mse_mns_cb_t *cb, const char **req_type)
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

	*cb = NULL;
	ARRAY_FOR_EACH(map_mns_functions, i) {
		if (strcmp(map_mns_functions[i].type, type) != 0) {
			continue;
		}

		if (strcmp(map_mns_functions[i].type, type_data) != 0) {
			continue;
		}

		if (map_mns_functions[i].op_get != is_get) {
			continue;
		}

		if (!has_required_hdrs(buf, &map_mns_functions[i].hdr)) {
			continue;
		}

		if (!has_required_app_params(buf, &map_mns_functions[i].ap)) {
			continue;
		}

		if (map_mns_functions[i].get_mse_mns_cb == NULL) {
			continue;
		}

		*cb = map_mns_functions[i].get_mse_mns_cb(mse_mns);
		if (*cb == NULL) {
			continue;
		}

		if (req_type != NULL) {
			*req_type = map_mns_functions[i].type;
		}
		break;
	}

	if (*cb == NULL) {
		LOG_WRN("Unsupported request");
		return -EINVAL;
	}

	return 0;
}

static int mse_mns_get_or_put(struct bt_map_mse_mns *mse_mns, bool is_get, const char *type,
			      bool final, struct net_buf *buf)
{
	int err;
	mse_mns_cb_t cb;
	mse_mns_cb_t old_cb;
	const char *req_type;
	const char *old_req_type;

	if (mse_mns == NULL || buf == NULL) {
		return -EINVAL;
	}

	if (atomic_get(&mse_mns->_state) != BT_MAP_STATE_CONNECTED) {
		LOG_ERR("Invalid state %u", (uint8_t)atomic_get(&mse_mns->_state));
		return -EINVAL;
	}

	LOG_DBG("MSE MNS %s request, type: %s, final: %d", is_get ? "GET" : "PUT", type, final);

	old_cb = mse_mns->_rsp_cb;
	old_req_type = mse_mns->_req_type;

	if (mse_mns->_rsp_cb == NULL || bt_obex_has_header(buf, BT_OBEX_HEADER_ID_TYPE)) {
		if (is_get && mse_mns->goep._goep_v2 &&
		    !bt_obex_has_header(buf, BT_OBEX_HEADER_ID_SRM)) {
			LOG_ERR("Failed to get SRM");
			return -ENODATA;
		}

		err = mse_mns_get_req_cb(mse_mns, type, buf, is_get, &cb, &req_type);
		if (err != 0) {
			LOG_ERR("Invalid request %d", err);
			return err;
		}

		if (mse_mns->_rsp_cb != NULL && cb != mse_mns->_rsp_cb) {
			LOG_ERR("Previous operation is not completed");
			return -EINVAL;
		}

		mse_mns->_rsp_cb = cb;
		mse_mns->_req_type = req_type;
	}

	if (mse_mns->_req_type != NULL && strcmp(mse_mns->_req_type, type) != 0) {
		LOG_ERR("Invalid request type %s != %s", mse_mns->_req_type, type);
		err = -EINVAL;
		goto failed;
	}

	if (bt_obex_has_header(buf, BT_OBEX_HEADER_ID_CONN_ID)) {
		err = map_check_conn_id(mse_mns->_conn_id, buf);
		if (err != 0) {
			goto failed;
		}
	}

	if (!is_get && final && !bt_obex_has_header(buf, BT_OBEX_HEADER_ID_END_BODY)) {
		LOG_ERR("Failed to get end of body");
		goto failed;
	}

	if (is_get) {
		err = bt_obex_get(&mse_mns->_client, final, buf);
	} else {
		err = bt_obex_put(&mse_mns->_client, final, buf);
	}

failed:
	if (err != 0) {
		mse_mns->_rsp_cb = old_cb;
		mse_mns->_req_type = old_req_type;
		LOG_ERR("Failed to send get/put req %d", err);
	}

	return err;
}

int bt_map_mse_mns_send_event(struct bt_map_mse_mns *mse_mns, bool final, struct net_buf *buf)
{
	return mse_mns_get_or_put(mse_mns, false, BT_MAP_HDR_TYPE_SEND_EVENT, final, buf);
}
#endif /* CONFIG_BT_MAP_MSE */
#endif /* CONFIG_BT_MAP */
