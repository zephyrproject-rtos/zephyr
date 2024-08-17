/*
 * Copyright 2024 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <string.h>
#include <errno.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include <zephyr/bluetooth/buf.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/classic/sdp.h>
#include <zephyr/bluetooth/classic/goep_client.h>
#include <zephyr/bluetooth/classic/pbap_client.h>
#include <zephyr/bluetooth/l2cap.h>

#define LOG_LEVEL CONFIG_BT_PBAP_CLIENT_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_pbap_client);

#include "host/hci_core.h"
#include "host/conn_internal.h"

#define PBAP_GET_TYPE_PB      "x-bt/phonebook"
#define PBAP_GET_TYPE_LISTING "x-bt/vcard-listing"
#define PBAP_GET_TYPE_VCARD   "x-bt/vcard"

#define PBAP_CLINET_PATH_FLAGS_ROOT   0x02
#define PBAP_CLINET_PATH_FLAGS_CHILD  0x02
#define PBAP_CLINET_PATH_FLAGS_PARENT 0x03

#define PBAP_TARGET_ID_LEN       16
#define PBAP_VCARD_MAX_LINE_SIZE 128
#define VCARD_MAX_RESULT_SIZE    10
#define SDP_MTU                  256

enum {
	PBAP_CLIENT_STATE_IDLE,
	PBAP_CLIENT_STATE_DISCOVER,
	PBAP_CLIENT_STATE_CONNECTING,
	PBAP_CLIENT_STATE_CONNECTED,
	PBAP_CLIENT_STATE_GET_MAX_LIST,
	PBAP_CLIENT_STATE_GET_PB,
	PBAP_CLIENT_STATE_SET_PATH,
	PBAP_CLIENT_STATE_GET_VCARD,
	PBAP_CLIENT_STATE_LISTING,
	PBAP_CLIENT_STATE_ABORTING,
	PBAP_CLIENT_STATE_DISCONNECTING,
	PBAP_CLIENT_STATE_DISCONNECTED,
};

NET_BUF_POOL_FIXED_DEFINE(sdp_pool, CONFIG_BT_MAX_CONN, BT_L2CAP_BUF_SIZE(SDP_MTU),
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

static K_MUTEX_DEFINE(pbap_lock);
static struct bt_pbap_client pbap_clients[CONFIG_BT_MAX_CONN];

static struct parse_vcard_result vcard_result[VCARD_MAX_RESULT_SIZE];
const static uint8_t pbap_target_id[PBAP_TARGET_ID_LEN] = {0x79, 0x61, 0x35, 0xf0, 0xf0, 0xc5,
							   0x11, 0xd8, 0x09, 0x66, 0x08, 0x00,
							   0x20, 0x0c, 0x9a, 0x66};

static void pbap_client_cleanup(struct bt_pbap_client *client)
{
	memset(client, 0, sizeof(*client));
}

static struct bt_pbap_client *pbap_find_client_by_goep(struct bt_goep_client *client)
{
	int i;

	for (i = 0; i < CONFIG_BT_MAX_CONN; i++) {
		if (pbap_clients[i].goep_client == client) {
			return &pbap_clients[i];
		}
	}

	return NULL;
}

static void pbap_reset_vcard_cached(struct bt_pbap_client *client)
{
	client->vcached.cached_len = 0;
	client->vcached.parse_pos = 0;
}

static uint16_t pbap_client_create_max_list_cnt_param(uint8_t *param, uint16_t len)
{
	struct pbap_common_param *common_param;

	common_param = (void *)param;
	common_param->id = PBAP_PARAM_ID_MAX_LIST_COUNT;
	common_param->len = 2;
	PBAP_PUT_VALUE_16(0, common_param->data);

	if (len < 4) {
		LOG_ERR("Must fix %d!!!!", len);
	}
	return 4;
}

static void pbap_client_get_max_list_count(struct bt_pbap_client *client)
{
	uint16_t flag = GOEP_SEND_HEADER_CONNECTING_ID;
	struct bt_goep_header_param header[3];
	uint8_t value[4];

	header[0].flag = GOEP_SEND_HEADER_NAME;
	header[0].len = strlen(client->path);
	header[0].value = client->path;
	flag |= GOEP_SEND_HEADER_NAME;

	header[1].flag = GOEP_SEND_HEADER_TYPE;
	header[1].len = sizeof(PBAP_GET_TYPE_PB);
	header[1].value = PBAP_GET_TYPE_PB;
	flag |= GOEP_SEND_HEADER_TYPE;

	header[2].flag = GOEP_SEND_HEADER_APP_PARAM;
	header[2].len = pbap_client_create_max_list_cnt_param(value, sizeof(value));
	header[2].value = (void *)value;
	flag |= GOEP_SEND_HEADER_APP_PARAM;

	bt_goep_client_get(client->goep_client, flag, header, ARRAY_SIZE(header));
}

static uint16_t pbap_client_create_get_pb_param(uint8_t *param, uint16_t len, uint32_t filter)
{
	uint16_t param_len = 0;
	struct pbap_common_param *common_param;

	common_param = (void *)&param[param_len];
	common_param->id = PBAP_PARAM_ID_MAX_LIST_COUNT;
	common_param->len = 2;
	PBAP_PUT_VALUE_16(0xFFFF, common_param->data);
	param_len += 4;

	common_param = (void *)&param[param_len];
	common_param->id = PBAP_PARAM_ID_FORMAT;
	common_param->len = 1;
	common_param->data[0] = PBAP_VCARD_VERSION_2_1;
	param_len += 3;

	common_param = (void *)&param[param_len];
	common_param->id = PBAP_PARAM_ID_FILTER;
	common_param->len = 8;
	PBAP_PUT_VALUE_32(0x00, &common_param->data[0]);
	PBAP_PUT_VALUE_32(filter, &common_param->data[4]);
	param_len += 10;

	if (len < param_len) {
		LOG_ERR("Must fix %d(%d)!!!!", param_len, len);
	}
	return param_len;
}

static void pbap_client_get_pb(struct bt_pbap_client *client, uint32_t filter)
{
	uint16_t flag = GOEP_SEND_HEADER_CONNECTING_ID;
	struct bt_goep_header_param header[3];
	uint8_t value[17];

	header[0].flag = GOEP_SEND_HEADER_NAME;
	header[0].len = strlen(client->path);
	header[0].value = client->path;
	flag |= GOEP_SEND_HEADER_NAME;

	header[1].flag = GOEP_SEND_HEADER_TYPE;
	header[1].len = sizeof(PBAP_GET_TYPE_PB);
	header[1].value = PBAP_GET_TYPE_PB;
	flag |= GOEP_SEND_HEADER_TYPE;

	header[2].flag = GOEP_SEND_HEADER_APP_PARAM;
	header[2].len = pbap_client_create_get_pb_param(value, sizeof(value), filter);
	header[2].value = (void *)value;
	flag |= GOEP_SEND_HEADER_APP_PARAM;

	bt_goep_client_get(client->goep_client, flag, header, ARRAY_SIZE(header));
}

static void pbap_client_set_path(struct bt_pbap_client *client, char *path)
{
	struct bt_goep_header_param header[2];
	uint16_t flag = GOEP_SEND_HEADER_CONNECTING_ID;
	uint8_t param = PBAP_CLINET_PATH_FLAGS_ROOT;

	header[0].flag = GOEP_SEND_HEADER_FLAGS;
	header[0].len = 1;
	header[0].value = &param;
	flag |= GOEP_SEND_HEADER_FLAGS;

	flag |= GOEP_SEND_HEADER_NAME;
	header[1].flag = GOEP_SEND_HEADER_NAME;
	if (path) {
		header[1].len = strlen(path);
		header[1].value = path;
	} else {
		header[1].len = 0;
		header[1].value = 0;
	}

	LOG_INF("level %d path %d %s", client->folder_level, header[1].len,
		(char *)(header[1].value));
	bt_goep_client_setpath(client->goep_client, flag, header, ARRAY_SIZE(header));
}

static uint16_t pbap_client_create_get_vcard_param(uint8_t *param, uint16_t len, uint32_t filter)
{
	uint16_t param_len = 0;
	struct pbap_common_param *common_param;

	common_param = (void *)&param[param_len];
	common_param->id = PBAP_PARAM_ID_FORMAT;
	common_param->len = 1;
	common_param->data[0] = PBAP_VCARD_VERSION_2_1;
	param_len += 3;

	common_param = (void *)&param[param_len];
	common_param->id = PBAP_PARAM_ID_FILTER;
	common_param->len = 8;
	PBAP_PUT_VALUE_32(0x00, &common_param->data[0]);
	PBAP_PUT_VALUE_32(filter, &common_param->data[4]);
	param_len += 10;

	if (len < param_len) {
		LOG_ERR("Must fix %d(%d)!!!!", param_len, len);
	}
	return param_len;
}

static void pbap_client_get_vcard(struct bt_pbap_client *client, uint8_t *name, uint8_t len,
				  uint32_t filter)
{
	uint16_t flag = GOEP_SEND_HEADER_CONNECTING_ID;
	struct bt_goep_header_param header[3];
	uint8_t value[16];

	header[0].flag = GOEP_SEND_HEADER_NAME;
	header[0].len = len;
	header[0].value = name;
	flag |= GOEP_SEND_HEADER_NAME;

	header[1].flag = GOEP_SEND_HEADER_TYPE;
	header[1].len = sizeof(PBAP_GET_TYPE_VCARD);
	header[1].value = PBAP_GET_TYPE_VCARD;
	flag |= GOEP_SEND_HEADER_TYPE;

	header[2].flag = GOEP_SEND_HEADER_APP_PARAM;
	header[2].len = pbap_client_create_get_vcard_param(value, sizeof(value), filter);
	header[2].value = (void *)value;
	flag |= GOEP_SEND_HEADER_APP_PARAM;

	bt_goep_client_get(client->goep_client, flag, header, ARRAY_SIZE(header));
}

static uint16_t pbap_client_create_listing_param(uint8_t *value, uint16_t len,
						 struct pbap_client_param *param)
{
	uint16_t param_len = 0;
	struct pbap_common_param *common_param;

	common_param = (void *)&value[param_len];
	common_param->id = PBAP_PARAM_ID_ORDER;
	common_param->len = 1;
	common_param->data[0] = param->order;
	param_len += 3;

	if (param->search_len) {
		common_param = (void *)&value[param_len];
		common_param->id = PBAP_PARAM_ID_SEARCH_VALUE;
		common_param->len = param->search_len;
		memcpy(common_param->data, param->search_value, param->search_len);
		param_len += param->search_len + 2;

		common_param = (void *)&value[param_len];
		common_param->id = PBAP_PARAM_ID_SEARCH_ATTR;
		common_param->len = 1;
		common_param->data[0] = param->search_attr;
		param_len += 3;
	}

	common_param = (void *)&value[param_len];
	common_param->id = PBAP_PARAM_ID_MAX_LIST_COUNT;
	common_param->len = 2;
	PBAP_PUT_VALUE_16(0xFFFF, common_param->data);
	param_len += 4;

	if (len < param_len) {
		LOG_ERR("Must fix %d(%d)!!!!", param_len, len);
	}
	return param_len;
}

static void pbap_client_listing(struct bt_pbap_client *client, struct pbap_client_param *param)
{
	uint16_t flag = GOEP_SEND_HEADER_CONNECTING_ID;
	struct bt_goep_header_param header[3] = {0};
	uint8_t value[42] = {0};

	header[0].flag = GOEP_SEND_HEADER_NAME;
	header[0].len = 0;
	header[0].value = NULL;
	flag |= GOEP_SEND_HEADER_NAME;

	header[1].flag = GOEP_SEND_HEADER_TYPE;
	header[1].len = sizeof(PBAP_GET_TYPE_LISTING);
	header[1].value = PBAP_GET_TYPE_LISTING;
	flag |= GOEP_SEND_HEADER_TYPE;

	header[2].flag = GOEP_SEND_HEADER_APP_PARAM;
	header[2].len = pbap_client_create_listing_param(value, sizeof(value), param);
	header[2].value = (void *)value;
	flag |= GOEP_SEND_HEADER_APP_PARAM;

	bt_goep_client_get(client->goep_client, flag, header, ARRAY_SIZE(header));
}

static void pbap_client_connected_cb(struct bt_goep_client *goep)
{
	struct bt_pbap_client *client;

	k_mutex_lock(&pbap_lock, K_FOREVER);

	client = pbap_find_client_by_goep(goep);
	if (!client) {
		LOG_WRN("Can't find client client");
		goto exit;
	}

	client->state = PBAP_CLIENT_STATE_CONNECTED;
	if (client->cb && client->cb->connected) {
		client->cb->connected(client);
	}

exit:
	k_mutex_unlock(&pbap_lock);
}

static void pbap_client_cb_recv(struct bt_pbap_client *client, uint8_t event)
{
	struct bt_pbap_result cb_result;

	if (client->cb && client->cb->recv) {
		cb_result.event = event;
		client->cb->recv(client, &cb_result);
	}
}

static void pbap_client_aborted_cb(struct bt_goep_client *goep)
{
	struct bt_pbap_client *client;

	k_mutex_lock(&pbap_lock, K_FOREVER);

	client = pbap_find_client_by_goep(goep);
	if (!client) {
		LOG_WRN("Can't find client client");
		goto exit;
	}

	pbap_reset_vcard_cached(client);
	pbap_client_cb_recv(client, PBAP_CLIENT_CB_EVENT_ABORT_FINISH);

exit:
	k_mutex_unlock(&pbap_lock);
}

static void pbap_client_disconnected_cb(struct bt_goep_client *goep)
{
	struct bt_pbap_client *client;

	k_mutex_lock(&pbap_lock, K_FOREVER);

	client = pbap_find_client_by_goep(goep);
	if (!client) {
		LOG_WRN("Can't find client client");
		goto exit;
	}

	if (client->cb && client->cb->disconnected) {
		client->cb->disconnected(client);
	}

	pbap_client_cleanup(client);

exit:
	k_mutex_unlock(&pbap_lock);
}

static void pbap_client_parse_app_param(struct bt_pbap_client *client, uint8_t *data, uint16_t len)
{
	struct pbap_common_param *common_param = (void *)data;
	struct bt_pbap_result cb_result;

	if (common_param->id != PBAP_PARAM_ID_PHONEBOOK_SIZE) {
		LOG_INF("Wait todo 0x%x", common_param->id);
		return;
	}

	if (common_param->len != 2) {
		LOG_INF("Wait todo parse len %d", common_param->len);
		return;
	}

	client->max_list_cnt = PBAP_GET_VALUE_16(common_param->data);
	LOG_DBG("pbap max_list_cnt %d", client->max_list_cnt);

	if (client->cb && client->cb->recv) {
		cb_result.event = PBAP_CLIENT_CB_EVENT_MAX_SIZE;
		cb_result.max_size = client->max_list_cnt;
		client->cb->recv(client, &cb_result);
	}
}

static void pbap_parse_vcached_cut_short(struct parse_cached_data *vcached)
{
	uint16_t i, j, line_len = 0, del_start_pos = 0, del_end_pos = 0;

	for (i = 0; i < PBAP_VCARD_MAX_CACHED; i++) {
		if ((vcached->cached[i] == 0x3d) && ((i + 2) < PBAP_VCARD_MAX_CACHED) &&
		    (((vcached->cached[i + 1] == 0x0d) && (vcached->cached[i + 2] == 0x0a)) ||
		     ((vcached->cached[i + 1] == 0x0a) && (vcached->cached[i + 2] == 0x0d)))) {
			line_len += 3;
			if (line_len > PBAP_VCARD_MAX_LINE_SIZE) {
				del_start_pos = i;
				break;
			}
			i += 2;
		} else if (vcached->cached[i] == 0x0d || vcached->cached[i] == 0x0a) {
			line_len = 0;
		} else {
			if ((vcached->cached[i] == 0x3d) && ((i + 2) < PBAP_VCARD_MAX_CACHED)) {
				line_len += 2;
			}

			line_len++;
			if (line_len > PBAP_VCARD_MAX_LINE_SIZE) {
				del_start_pos = i;
				break;
			}
		}
	}

	if (del_start_pos == 0) {
		return;
	}

	for (; i < PBAP_VCARD_MAX_CACHED; i++) {
		if ((vcached->cached[i - 1] == 0x3d) &&
		    (vcached->cached[i] == 0x0d || vcached->cached[i] == 0x0a)) {
		} else if ((vcached->cached[i - 2] == 0x3d) &&
			   (vcached->cached[i - 1] == 0x0d || vcached->cached[i - 1] == 0x0a) &&
			   (vcached->cached[i] == 0x0d || vcached->cached[i] == 0x0a)) {
		} else if (vcached->cached[i] == 0x0d || vcached->cached[i] == 0x0a) {
			del_end_pos = i;
			break;
		}
	}

	if (i == PBAP_VCARD_MAX_CACHED) {
		del_end_pos = PBAP_VCARD_MAX_CACHED;
	}

	j = 0;
	for (i = del_end_pos; i < PBAP_VCARD_MAX_CACHED; i++) {
		vcached->cached[del_start_pos + j] = vcached->cached[del_end_pos + j];
		j++;
	}

	vcached->cached_len -= (del_end_pos - del_start_pos);
	LOG_WRN("Vcard do cut short %d", (del_end_pos - del_start_pos));
}

static void pbap_client_parse_pb(struct bt_pbap_client *client, uint8_t *data, uint16_t len)
{
	struct bt_pbap_result cb_result;
	uint16_t parse_len = 0;
	uint8_t *p = data;
	uint8_t size;

	LOG_DBG("pbap vcard %d", len);
	while (parse_len < len) {
		parse_len += pbap_parse_copy_to_cached(&p[parse_len], (len - parse_len),
						       &client->vcached);

		do {
			size = VCARD_MAX_RESULT_SIZE;
			pbap_parse_vcached_vcard(&client->vcached, vcard_result, &size);
			if (size && client->cb && client->cb->recv) {
				cb_result.event = PBAP_CLIENT_CB_EVENT_VCARD;
				cb_result.vcard_result = vcard_result;
				cb_result.array_size = size;
				client->cb->recv(client, &cb_result);
			}

			if ((size == 0) && ((client->vcached.cached_len -
					     client->vcached.parse_pos) == PBAP_VCARD_MAX_CACHED)) {
				pbap_parse_vcached_cut_short(&client->vcached);
				if ((client->vcached.cached_len - client->vcached.parse_pos) ==
				    PBAP_VCARD_MAX_CACHED) {
					pbap_reset_vcard_cached(client);
					LOG_ERR("Vcard too larger");
				}
			}
		} while (size);

		pbap_parse_vcached_move(&client->vcached);
	}
}

static void pbap_client_parse_list(struct bt_pbap_client *client, uint8_t *data, uint16_t len)
{
	struct bt_pbap_result cb_result;
	uint8_t size;
	uint8_t *p = data;
	uint16_t parse_len = 0;

	LOG_DBG("pbap search %d", len);

	while (parse_len < len) {
		parse_len += pbap_parse_copy_to_cached(&p[parse_len], (len - parse_len),
						       &client->vcached);

		do {
			size = VCARD_MAX_RESULT_SIZE;
			pbap_parse_vcached_list(&client->vcached, vcard_result, &size);
			if (size && client->cb && client->cb->recv) {
				cb_result.event = PBAP_CLIENT_CB_EVENT_SEARCH_RESULT;
				cb_result.vcard_result = vcard_result;
				cb_result.array_size = size;
				client->cb->recv(client, &cb_result);
			}

			if ((size == 0) && ((client->vcached.cached_len -
					     client->vcached.parse_pos) == PBAP_VCARD_MAX_CACHED)) {
				pbap_reset_vcard_cached(client);
				LOG_ERR("List too larger");
			}
		} while (size);

		pbap_parse_vcached_move(&client->vcached);
	}
}

static void pbap_client_parse_vcard(struct bt_pbap_client *client, uint8_t *data, uint16_t len)
{
	if (client->state == PBAP_CLIENT_STATE_GET_PB ||
	    client->state == PBAP_CLIENT_STATE_GET_VCARD) {
		pbap_client_parse_pb(client, data, len);
	} else if (client->state == PBAP_CLIENT_STATE_LISTING) {
		pbap_client_parse_list(client, data, len);
	} else {
		LOG_ERR("Parse vcard err state %d", client->state);
	}
}

static void pbap_client_recv_cb(struct bt_goep_client *goep, uint16_t flag, uint8_t *data,
				uint16_t len)
{
	struct bt_pbap_client *client;

	k_mutex_lock(&pbap_lock, K_FOREVER);

	client = pbap_find_client_by_goep(goep);
	if (!client) {
		LOG_ERR("Can't find client client");
		goto exit;
	}

	switch (flag) {
	case GOEP_RECV_HEADER_APP_PARAM:
		pbap_client_parse_app_param(client, data, len);
		break;
	case GOEP_RECV_HEADER_BODY:
		pbap_client_parse_vcard(client, data, len);
		pbap_client_cb_recv(client, PBAP_CLIENT_CB_EVENT_GET_VCARD_FINISH);
		break;
	case GOEP_RECV_HEADER_END_OF_BODY:
		if (data) {
			pbap_client_parse_vcard(client, data, len);
		}
		pbap_client_cb_recv(client, PBAP_CLIENT_CB_EVENT_END_OF_BODY);
		pbap_client_cb_recv(client, PBAP_CLIENT_CB_EVENT_GET_VCARD_FINISH);
		break;
	case GOEP_RECV_CONTINUE_WITHOUT_BODY:
		pbap_client_cb_recv(client, PBAP_CLIENT_CB_EVENT_GET_VCARD_FINISH);
		break;
	default:
		LOG_WRN("pbap client unhandle flag 0x%x", flag);
		break;
	}

exit:
	k_mutex_unlock(&pbap_lock);
}

static int pbap_client_get_server_path(uint8_t *path, uint8_t level, uint8_t *child, uint8_t size)
{
	uint8_t *pt = path;
	uint8_t current = 0;
	uint8_t i = 0;
	uint8_t j = 0;

	if (!path || !child) {
		return -EINVAL;
	}

	while ((pt[i] != 0) && (level > 0)) {
		if ((pt[i] == '/') && (i != 0)) {
			current += 1;
		}
		if ((pt[i] != '/') && (i == 0)) {
			current += 1;
		}

		if ((current == level) && (pt[i] != '/')) {
			if (j < size - 1) {
				child[j] = pt[i];
				j++;
			} else {
				LOG_ERR("Get server patch error");
			}
		}
		i++;
	}

	child[j] = 0;
	return j;
}

static uint8_t pbap_client_check_server_path(uint8_t *path)
{
	uint8_t *pt = path;
	uint8_t level = 0;
	uint8_t i = 0;

	if (!path) {
		return 0;
	}

	while (pt[i] != 0) {
		if ((pt[i] == '/') && (i != 0)) {
			level += 1;
		}
		if ((pt[i] != '/') && (i == 0)) {
			level += 1;
		}
		i += 1;
	}

	return level;
}

static void pbap_client_setpath_cb(struct bt_goep_client *goep)
{
	struct bt_pbap_client *client;
	uint8_t path_param[16];

	k_mutex_lock(&pbap_lock, K_FOREVER);

	client = pbap_find_client_by_goep(goep);
	if (!client) {
		LOG_WRN("Can't find pbap client client");
		goto exit;
	}

	client->folder_level += 1;
	if (client->folder_level <= pbap_client_check_server_path(client->path)) {
		pbap_client_get_server_path(client->path, client->folder_level, path_param, 16);
		pbap_client_set_path(client, path_param);
	} else {
		client->path_valid = 1;
		pbap_client_cb_recv(client, PBAP_CLIENT_CB_EVENT_SETPATH_FINISH);
	}

exit:
	k_mutex_unlock(&pbap_lock);
}

static struct bt_goep_client_cb goep_client_cb = {
	.connected = pbap_client_connected_cb,
	.aborted = pbap_client_aborted_cb,
	.disconnected = pbap_client_disconnected_cb,
	.recv = pbap_client_recv_cb,
	.setpath = pbap_client_setpath_cb,
};

static uint8_t pbap_client_sdp_handle(struct bt_conn *conn, struct bt_sdp_client_result *result)
{
	struct bt_pbap_client *client;
	uint16_t param;
	int ret;

	if (!result) {
		LOG_DBG("Failed to find discover client");
		return BT_SDP_DISCOVER_UUID_CONTINUE;
	}

	client = &pbap_clients[bt_conn_index(conn)];

	ret = bt_sdp_get_proto_param(result->resp_buf, BT_SDP_PROTO_RFCOMM, &param);
	if (ret < 0) {
		LOG_ERR("pbap client sdp fail, err:%d", ret);
		goto exit;
	}

	k_mutex_lock(&pbap_lock, K_FOREVER);
	client->goep_client = bt_goep_client_connect(conn, (uint8_t *)pbap_target_id,
						     PBAP_TARGET_ID_LEN, param, &goep_client_cb);

	if (!client->goep_client) {
		pbap_client_cleanup(client);
		LOG_DBG("Failed to connect goep client");
		k_mutex_unlock(&pbap_lock);
		goto exit;
	}

	client->state = PBAP_CLIENT_STATE_CONNECTING;
	k_mutex_unlock(&pbap_lock);

exit:
	return BT_SDP_DISCOVER_UUID_CONTINUE;
}

static struct bt_sdp_discover_params pbap_sdp_params = {
	.uuid = BT_UUID_DECLARE_16(BT_SDP_PBAP_PSE_SVCLASS),
	.func = pbap_client_sdp_handle,
	.pool = &sdp_pool,
};

struct bt_pbap_client *bt_pbap_client_connect(struct bt_conn *conn, struct bt_pbap_client_cb *cb)
{
	int ret;
	struct bt_pbap_client *client;

	if (!conn) {
		return NULL;
	}

	k_mutex_lock(&pbap_lock, K_FOREVER);
	client = &pbap_clients[bt_conn_index(conn)];
	client->conn = conn;
	client->cb = cb;
	client->state = PBAP_CLIENT_STATE_DISCOVER;

	ret = bt_sdp_discover(conn, &pbap_sdp_params);
	if (ret) {
		pbap_client_cleanup(client);
		client = NULL;
		goto exit;
	}

exit:
	k_mutex_unlock(&pbap_lock);
	return client;
}

int bt_pbap_client_disconnect(struct bt_pbap_client *client)
{
	int ret;

	k_mutex_lock(&pbap_lock, K_FOREVER);
	if (!client) {
		ret = -EINVAL;
		LOG_ERR("client is null");
		goto exit;
	}

	ret = bt_goep_client_disconnect(client->goep_client, GOEP_SEND_HEADER_CONNECTING_ID);
	if (ret < 0) {
		LOG_ERR("client disconnect fail, err:%d", ret);
		goto exit;
	}

	client->state = PBAP_CLIENT_STATE_DISCONNECTING;

exit:
	k_mutex_unlock(&pbap_lock);
	return ret;
}

int bt_pbap_client_request(struct bt_pbap_client *client, struct pbap_client_param *param)
{
	int ret = 0;

	if (!client || !client->conn) {
		LOG_ERR("client is not connected");
		return -EINVAL;
	}

	k_mutex_lock(&pbap_lock, K_FOREVER);

	switch (param->op_cmd) {
	case PBAP_CLIENT_OP_CMD_GET_SIZE:
		client->state = PBAP_CLIENT_STATE_GET_MAX_LIST;
		client->path = param->path;
		pbap_client_get_max_list_count(client);
		break;
	case PBAP_CLIENT_OP_CMD_GET_PB:
		client->state = PBAP_CLIENT_STATE_GET_PB;
		client->path = param->path;
		pbap_reset_vcard_cached(client);
		pbap_client_get_pb(client, param->filter);
		break;
	case PBAP_CLIENT_OP_CMD_SET_PATH:
		client->state = PBAP_CLIENT_STATE_SET_PATH;
		client->path = param->path;
		client->folder_level = 0;
		pbap_client_set_path(client, NULL);
		break;
	case PBAP_CLIENT_OP_CMD_GET_VCARD:
		client->state = PBAP_CLIENT_STATE_GET_VCARD;
		pbap_reset_vcard_cached(client);
		pbap_client_get_vcard(client, param->vcard_name, param->vcard_name_len,
				      param->filter);
		break;
	case PBAP_CLIENT_OP_CMD_GET_CONTINUE:
		bt_goep_client_get(client->goep_client, 0, NULL, 0);
		break;
	case PBAP_CLIENT_OP_CMD_LISTING:
		client->state = PBAP_CLIENT_STATE_LISTING;
		pbap_reset_vcard_cached(client);
		pbap_client_listing(client, param);
		break;
	case PBAP_CLIENT_OP_CMD_ABORT:
		client->state = PBAP_CLIENT_STATE_ABORTING;
		bt_goep_client_abort(client->goep_client, 0);
		break;
	default:
		break;
	}

	k_mutex_unlock(&pbap_lock);
	return ret;
}
