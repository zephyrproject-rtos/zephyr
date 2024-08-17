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
#include <zephyr/bluetooth/classic/map_client.h>
#include <zephyr/bluetooth/l2cap.h>

#define LOG_LEVEL CONFIG_BT_MAP_CLIENT_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_map_client);

#include "host/hci_core.h"
#include "host/conn_internal.h"

#define MAP_GET_MESSAGES_LIST_TYPE_VALUE  "x-bt/MAP-msg-listing"
#define MAP_GET_MESSAGE_TYPE_VALUE        "x-bt/message"
#define MAP_GET_FOLDER_LISTING_TYPE_VALUE "x-obex/folder-listing"

#define MAP_CLINET_PATH_FLAGS_ROOT   0x02
#define MAP_CLINET_PATH_FLAGS_CHILD  0x02
#define MAP_CLINET_PATH_FLAGS_PARENT 0x03

#define MAP_TARGET_ID_LEN 16
#define SDP_MTU           256

enum {
	MAP_CLIENT_STATE_IDLE,
	MAP_CLIENT_STATE_DISCOVER,
	MAP_CLIENT_STATE_CONNECTING,
	MAP_CLIENT_STATE_CONNECTED,
	MAP_CLIENT_STATE_SETPATH,
	MAP_CLIENT_STATE_GET_MAX_LIST,
	MAP_CLIENT_STATE_GET_MESSAGE,
	MAP_CLIENT_STATE_ABORTING,
	MAP_CLIENT_STATE_DISCONNECTING,
	MAP_CLIENT_STATE_DISCONNECTED,
};

NET_BUF_POOL_FIXED_DEFINE(sdp_pool, CONFIG_BT_MAX_CONN, BT_L2CAP_BUF_SIZE(SDP_MTU),
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

static K_MUTEX_DEFINE(map_lock);
const static uint8_t map_uuid[MAP_TARGET_ID_LEN] = {0xbb, 0x58, 0x2b, 0x40, 0x42, 0x0c, 0x11, 0xdb,
						    0xb0, 0xde, 0x08, 0x00, 0x20, 0x0c, 0x9a, 0x66};
static struct bt_map_client map_clients[CONFIG_BT_MAX_CONN];

static void map_client_cleanup(struct bt_map_client *client)
{
	memset(client, 0, sizeof(*client));
}

static struct bt_map_client *map_find_client_by_goep(struct bt_goep_client *goep)
{
	int i;

	if (!goep) {
		return NULL;
	}

	for (i = 0; i < CONFIG_BT_MAX_CONN; i++) {
		if (map_clients[i].goep_client == goep) {
			return &map_clients[i];
		}
	}

	return NULL;
}

static uint8_t map_check_server_path(uint8_t *path)
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

static int map_get_server_path(uint8_t *path, uint8_t level, uint8_t *child, uint8_t size)
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
				LOG_ERR("map server patch error");
			}
		}

		i++;
	}

	child[j] = 0;
	return j;
}

static uint16_t map_client_create_folder_listing_param(uint8_t *param, uint16_t size)
{
	struct map_common_param *value;
	uint16_t len = 0;

	value = (void *)&param[len];
	value->id = MAP_PARAM_ID_MAX_LIST_COUNT;
	value->len = 2;
	MAP_PUT_VALUE_16(1024, value->data);
	len += 4;

	if (size < len) {
		LOG_ERR("map create folder size:%d < param:%d", len, size);
	}

	return len;
}

static uint16_t map_client_create_max_list_cnt_param(uint16_t max_list_cn, uint32_t param_mask,
						     uint8_t *param, uint16_t size)
{
	struct map_common_param *value;
	uint16_t len = 0;

	value = (void *)&param[len];
	value->id = MAP_PARAM_ID_MAX_LIST_COUNT;
	value->len = 2;
	MAP_PUT_VALUE_16(max_list_cn, value->data);
	len += 4;

	value = (void *)&param[len];
	value->id = MAP_PARAM_ID_PARAMETER_MASK;
	value->len = 4;
	MAP_PUT_VALUE_32(param_mask, &value->data[0]);
	len += 6;

	if (size < len) {
		LOG_ERR("map create list size:%d < param:%d", len, size);
	}

	return len;
}

static uint16_t map_client_create_get_message_param(uint8_t *param, uint16_t size)
{
	uint16_t len = 0;

	struct map_common_param *value;

	value = (void *)&param[len];
	value->id = MAP_PARAM_ID_ATTACHMENT;
	value->len = 1;
	value->data[0] = 0;
	len += 3;

	if (size < len) {
		LOG_ERR("map create msg size:%d < param:%d", len, size);
	}

	return len;
}

static void map_client_get_folder_listing(struct bt_map_client *client)
{
	uint16_t flag = GOEP_SEND_HEADER_CONNECTING_ID;
	struct bt_goep_header_param header[2] = {0};
	uint8_t value[8];

	flag |= GOEP_SEND_HEADER_TYPE;
	header[0].flag = GOEP_SEND_HEADER_TYPE;
	header[0].len = sizeof(MAP_GET_FOLDER_LISTING_TYPE_VALUE);
	header[0].value = MAP_GET_FOLDER_LISTING_TYPE_VALUE;

	flag |= GOEP_SEND_HEADER_APP_PARAM;
	header[1].flag = GOEP_SEND_HEADER_APP_PARAM;
	header[1].len = map_client_create_folder_listing_param(value, sizeof(value));
	header[1].value = (void *)value;

	bt_goep_client_get(client->goep_client, flag, header, ARRAY_SIZE(header));
}

static void map_client_get_messages_listing(struct bt_map_client *client)
{
	uint16_t flag = GOEP_SEND_HEADER_CONNECTING_ID;
	struct bt_goep_header_param header[3] = {0};
	uint8_t value[10];

	flag |= GOEP_SEND_HEADER_NAME;
	header[0].flag = GOEP_SEND_HEADER_NAME;
	header[0].len = 0;
	header[0].value = 0;

	flag |= GOEP_SEND_HEADER_TYPE;
	header[1].flag = GOEP_SEND_HEADER_TYPE;
	header[1].len = sizeof(MAP_GET_MESSAGES_LIST_TYPE_VALUE);
	header[1].value = MAP_GET_MESSAGES_LIST_TYPE_VALUE;

	flag |= GOEP_SEND_HEADER_APP_PARAM;
	header[2].flag = GOEP_SEND_HEADER_APP_PARAM;
	header[2].len = map_client_create_max_list_cnt_param(client->max_list_cnt, client->mask,
							     value, sizeof(value));
	header[2].value = (void *)value;

	bt_goep_client_get(client->goep_client, flag, header, ARRAY_SIZE(header));
}

static void map_client_set_path(struct bt_map_client *client, char *path)
{
	uint16_t flag = GOEP_SEND_HEADER_CONNECTING_ID;
	struct bt_goep_header_param header[2] = {0};
	uint8_t flags = MAP_CLINET_PATH_FLAGS_CHILD;

	flag |= GOEP_SEND_HEADER_FLAGS;
	header[0].flag = GOEP_SEND_HEADER_FLAGS;
	header[0].len = 1;
	header[0].value = &flags;

	flag |= GOEP_SEND_HEADER_NAME;
	header[1].flag = GOEP_SEND_HEADER_NAME;
	if (!path) {
		header[1].len = 0;
		header[1].value = 0;
	} else {
		header[1].len = strlen(path);
		header[1].value = path;
	}

	bt_goep_client_setpath(client->goep_client, flag, header, ARRAY_SIZE(header));
}

static void map_client_get_messge(struct bt_map_client *client)
{
	uint16_t flag = GOEP_SEND_HEADER_CONNECTING_ID;
	struct bt_goep_header_param header[3] = {0};
	uint8_t value[17];

	flag |= GOEP_SEND_HEADER_NAME;
	header[0].flag = GOEP_SEND_HEADER_NAME;
	header[0].len = strlen(client->path);
	header[0].value = client->path;

	flag |= GOEP_SEND_HEADER_TYPE;
	header[1].flag = GOEP_SEND_HEADER_TYPE;
	header[1].len = sizeof(MAP_GET_MESSAGE_TYPE_VALUE);
	header[1].value = MAP_GET_MESSAGE_TYPE_VALUE;

	flag |= GOEP_SEND_HEADER_APP_PARAM;
	header[2].flag = GOEP_SEND_HEADER_APP_PARAM;
	header[2].len = map_client_create_get_message_param(value, sizeof(value));
	header[2].value = (void *)value;

	bt_goep_client_get(client->goep_client, flag, header, ARRAY_SIZE(header));
}

static void map_client_connected_cb(struct bt_goep_client *goep)
{
	struct bt_map_client *client;

	k_mutex_lock(&map_lock, K_FOREVER);

	client = map_find_client_by_goep(goep);
	if (!client) {
		LOG_WRN("Can't find map client");
		goto exit;
	}

	client->state = MAP_CLIENT_STATE_CONNECTED;
	client->goep_connect = 1;
	client->folder_level = -1;

	if (client->cb && client->cb->connected) {
		client->cb->connected(client);
	}

	map_client_set_path(client, NULL);
	client->state = MAP_CLIENT_STATE_SETPATH;

exit:
	k_mutex_unlock(&map_lock);
}

static void map_client_aborted_cb(struct bt_goep_client *goep)
{
	struct bt_map_client *client;

	k_mutex_lock(&map_lock, K_FOREVER);

	client = map_find_client_by_goep(goep);
	if (!client) {
		LOG_WRN("Can't find map client");
		goto exit;
	}

	bt_goep_client_disconnect(client->goep_client, GOEP_SEND_HEADER_CONNECTING_ID);
	client->state = MAP_CLIENT_STATE_DISCONNECTING;

exit:
	k_mutex_unlock(&map_lock);
}

static void map_client_disconnected_cb(struct bt_goep_client *goep)
{
	struct bt_map_client *client;

	k_mutex_lock(&map_lock, K_FOREVER);

	client = map_find_client_by_goep(goep);
	if (!client) {
		LOG_WRN("Can't find client");
		goto exit;
	}

	if (client->cb && client->cb->disconnected) {
		client->cb->disconnected(client);
	}

	map_client_cleanup(client);

exit:
	k_mutex_unlock(&map_lock);
}

static void map_client_parse_app_param(struct bt_map_client *client, uint8_t *data, uint16_t size)
{
	struct map_common_param *value = (void *)data;
	struct bt_map_result datetime;

	uint16_t parse_len = 0;
	uint8_t *p = data;

	while (parse_len < size) {
		switch (value->id) {
		case MAP_PARAM_ID_NEW_MESSAGE:
			parse_len += 3;
			value = (void *)(p + parse_len);
			break;
		case MAP_PARAM_ID_MESSAGES_LISTING_SIZE:
			parse_len += 4;
			value = (void *)(p + parse_len);
			break;
		case MAP_PARAM_ID_MSE_TIME:
			if (client->cb && client->cb->recv) {
				datetime.data = value->data;
				datetime.len = value->len;
				datetime.type = MAP_MESSAGE_RESULT_TYPE_DATETIME;
				client->cb->recv(client, &datetime, 1);
			}
			parse_len += (value->len + 2);
			value = (void *)(p + parse_len);
			break;
		default:
			parse_len += (value->len + 2);
			value = (void *)(p + parse_len);
			break;
		}
	}
}

static void map_client_parse_message(struct bt_map_client *client, uint8_t *data, uint16_t size)
{
	struct bt_map_result result = {0};

	LOG_INF("message %d", size);

	result.data = data;
	result.len = size;
	result.type = MAP_MESSAGE_RESULT_TYPE_BODY;

	if (size && client->cb && client->cb->recv) {
		client->cb->recv(client, &result, 1);
	}

	bt_goep_client_get(client->goep_client, 0, NULL, 0);
}

static void map_client_recv_cb(struct bt_goep_client *goep, uint16_t flag, uint8_t *data,
			       uint16_t size)
{
	struct bt_map_client *client;

	k_mutex_lock(&map_lock, K_FOREVER);

	client = map_find_client_by_goep(goep);

	LOG_INF("header:%d", flag);

	if (!client) {
		LOG_WRN("Can't find client");
		goto exit;
	}

	if (client->state >= MAP_CLIENT_STATE_ABORTING) {
		goto exit;
	}

	if (flag == GOEP_RECV_HEADER_APP_PARAM) {
		map_client_parse_app_param(client, data, size);
	} else if (flag == GOEP_RECV_HEADER_BODY) {
		map_client_parse_message(client, data, size);
	} else if (flag == GOEP_RECV_HEADER_END_OF_BODY) {
	} else {
		LOG_WRN("Wait todo flag 0x%x", flag);
	}

exit:
	k_mutex_unlock(&map_lock);
}

static void map_client_setpath_cb(struct bt_goep_client *goep)
{
	struct bt_map_client *client;
	uint8_t path_param[16];
	k_mutex_lock(&map_lock, K_FOREVER);

	client = map_find_client_by_goep(goep);
	if (!client) {
		LOG_WRN("Can't find map client");
		goto exit;
	}

	client->folder_level += 1;
	if (client->folder_level < map_check_server_path(client->path)) {
		map_get_server_path(client->path, client->folder_level + 1, path_param, 16);
		map_client_set_path(client, path_param);
		client->state = MAP_CLIENT_STATE_SETPATH;
	} else {
		client->state = MAP_CLIENT_STATE_CONNECTED;
		client->path_valid = 1;
		if (client->cb && client->cb->set_path_finished) {
			client->cb->set_path_finished(client);
		}
	}

exit:
	k_mutex_unlock(&map_lock);
}

static struct bt_goep_client_cb goep_client_cb = {
	.connected = map_client_connected_cb,
	.aborted = map_client_aborted_cb,
	.disconnected = map_client_disconnected_cb,
	.recv = map_client_recv_cb,
	.setpath = map_client_setpath_cb,
};

static uint8_t map_client_sdp_handle(struct bt_conn *conn, struct bt_sdp_client_result *result)
{
	struct bt_map_client *client;
	int ret;
	uint16_t param;

	LOG_INF("map client sdp:%p", result);

	if (!result) {
		LOG_ERR("No SDP result from remote");
		return BT_SDP_DISCOVER_UUID_CONTINUE;
	}

	client = &map_clients[bt_conn_index(conn)];

	ret = bt_sdp_get_proto_param(result->resp_buf, BT_SDP_PROTO_RFCOMM, &param);
	if (ret < 0) {
		LOG_ERR("map client sdp fail, err:%d", ret);
		goto exit;
	}

	k_mutex_lock(&map_lock, K_FOREVER);
	client->goep_client = bt_goep_client_connect(conn, (uint8_t *)map_uuid, MAP_TARGET_ID_LEN,
						     param, &goep_client_cb);
	if (!client->goep_client) {
		LOG_WRN("Failed to connect goep client");
		map_client_cleanup(client);
		k_mutex_unlock(&map_lock);
		goto exit;
	}

	client->state = MAP_CLIENT_STATE_CONNECTING;
	k_mutex_unlock(&map_lock);

exit:
	return BT_SDP_DISCOVER_UUID_CONTINUE;
}

static struct bt_sdp_discover_params map_sdp_params = {
	.uuid = BT_UUID_DECLARE_16(BT_SDP_MAP_MSE_SVCLASS),
	.func = map_client_sdp_handle,
	.pool = &sdp_pool,
};

struct bt_map_client *bt_map_client_connect(struct bt_conn *conn, char *path,
					    struct bt_map_client_cb *cb)
{
	int ret;
	struct bt_map_client *client;

	if (!conn) {
		return NULL;
	}

	k_mutex_lock(&map_lock, K_FOREVER);

	client = &map_clients[bt_conn_index(conn)];
	client->state = MAP_CLIENT_STATE_DISCOVER;
	client->conn = conn;
	client->path = path;
	client->cb = cb;
	client->path_valid = 0;

	ret = bt_sdp_discover(conn, &map_sdp_params);
	if (ret) {
		map_client_cleanup(client);
		client = NULL;
		goto exit;
	}

exit:
	k_mutex_unlock(&map_lock);
	return client;
}

int bt_map_client_disconnect(struct bt_map_client *client)
{
	int ret = 0;

	if (!client) {
		LOG_ERR("client is null");
		return -EINVAL;
	}

	k_mutex_lock(&map_lock, K_FOREVER);

	ret = bt_goep_client_disconnect(client->goep_client, GOEP_SEND_HEADER_CONNECTING_ID);
	if (ret < 0) {
		LOG_ERR("map goep client disconnect, err:%d", ret);
		goto exit;
	}

	client->state = MAP_CLIENT_STATE_DISCONNECTING;

exit:
	k_mutex_unlock(&map_lock);
	return ret;
}

int bt_map_client_get_message(struct bt_map_client *client, char *path)
{
	if (!client) {
		LOG_ERR("client is null");
		return -EINVAL;
	}

	k_mutex_lock(&map_lock, K_FOREVER);
	client->state = MAP_CLIENT_STATE_GET_MESSAGE;
	client->path = path;

	map_client_get_messge(client);
	k_mutex_unlock(&map_lock);
	return 0;
}

int bt_map_client_setpath(struct bt_map_client *client, char *path)
{
	if (!client) {
		LOG_ERR("client is null");
		return -EINVAL;
	}

	k_mutex_lock(&map_lock, K_FOREVER);

	client->state = MAP_CLIENT_STATE_SETPATH;
	client->path = path;
	client->path_valid = 0;

	map_client_set_path(client, path);
	k_mutex_unlock(&map_lock);
	return 0;
}

int bt_map_client_set_folder(struct bt_map_client *client, char *path, uint8_t flags)
{
	if (!client) {
		LOG_ERR("client is null");
		return -EINVAL;
	}

	k_mutex_lock(&map_lock, K_FOREVER);

	client->state = MAP_CLIENT_STATE_SETPATH;
	client->folder_level = -1;
	client->path_valid = 0;
	client->path = path;
	map_client_set_path(client, path);

	k_mutex_unlock(&map_lock);
	return 0;
}

int bt_map_client_get_folder_listing(struct bt_map_client *client)
{
	int ret = 0;

	if (!client) {
		LOG_ERR("client is null");
		return -EINVAL;
	}

	k_mutex_lock(&map_lock, K_FOREVER);

	if (client->path_valid == 1) {
		map_client_get_folder_listing(client);
	} else {
		LOG_WRN("Not set path");
	}

	k_mutex_unlock(&map_lock);
	return ret;
}

int bt_map_client_get_messages_listing(struct bt_map_client *client, uint16_t max_list_count,
				       uint32_t mask)
{
	int ret = 0;

	LOG_INF("map client get msg list, count:%d, mask:0%x", max_list_count, mask);

	if (!client) {
		LOG_ERR("client is null");
		return -EINVAL;
	}

	k_mutex_lock(&map_lock, K_FOREVER);

	if (!client->path_valid) {
		LOG_WRN("Not set path");
		ret = -EINVAL;
		goto exit;
	}

	if (max_list_count > 0 && max_list_count < 1024) {
		client->max_list_cnt = max_list_count;
	} else {
		client->max_list_cnt = 1024;
	}

	if (mask != 0) {
		client->mask = mask;
	} else {
		client->mask = MAP_PARAMETER_MASK_SENDER_ADDRESS | MAP_PARAMETER_MASK_DATETIME;
	}

	map_client_get_messages_listing(client);

exit:
	k_mutex_unlock(&map_lock);
	return ret;
}