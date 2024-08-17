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

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/classic/sdp.h>
#include <zephyr/bluetooth/classic/goep_client.h>
#include <zephyr/bluetooth/l2cap.h>

#define LOG_LEVEL CONFIG_BT_GOEP_CLIENT_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_goep_client);

#include "host/hci_core.h"
#include "host/conn_internal.h"
#include "goep_client_internal.h"

#define GOEP_CLIENT_VERSION 0x10 /* V1.0 */
#define GEOP_RFCOMM_MTU     672
#define GEOP_MAX_PKT_LEN    (GEOP_RFCOMM_MTU - 6)

enum {
	GOEP_CLIENT_STATE_IDLE,
	GOEP_CLIENT_STATE_CONNECTING_RFCOMM,
	GOEP_CLIENT_STATE_RFCOMM_CONNECTED,
	GOEP_CLIENT_STATE_CONNECTING_PROFILE,
	GOEP_CLIENT_STATE_PROFILE_CONNECTED,
	GOEP_CLIENT_STATE_GETTING,
	GOEP_CLIENT_STATE_ABORTING,
	GOEP_CLIENT_STATE_DISCONNECTING,
	GOEP_CLIENT_STATE_DISCONNECTED,
	GOEP_CLIENT_STATE_RFCOMM_DISCONNECTING,
	GOEP_CLIENT_STATE_RFCOMM_DISCONNECTED,
	GOEP_CLIENT_STATE_SETPATH,
};

static K_MUTEX_DEFINE(goep_client_lock);
static struct bt_goep_client goep_clients[CONFIG_BT_MAX_CONN];

NET_BUF_POOL_FIXED_DEFINE(goep_tx_pool, CONFIG_BT_L2CAP_TX_BUF_COUNT,
			  BT_L2CAP_BUF_SIZE(CONFIG_BT_L2CAP_TX_MTU), 8, NULL);

static void goep_client_cleanup(struct bt_goep_client *client)
{
	(void)memset(client, 0, sizeof(*client));
}

static struct bt_goep_client *goep_find_client_by_dlci(struct bt_rfcomm_dlc *dlci)
{
	uint8_t i;

	for (i = 0; i < CONFIG_BT_MAX_CONN; i++) {
		if (&goep_clients[i].rfcomm_dlc == dlci) {
			return &goep_clients[i];
		}
	}

	return NULL;
}

static uint16_t goep_header_append_encoding_0(struct net_buf *buf, uint8_t flag, uint8_t *data,
					      uint16_t len)
{
	uint16_t size, i;
	uint8_t header = GOEP_CREATE_HEADER_ID(GOEP_HEADER_ENCODING_UNICODE_TEXT, flag);

	if (len > 0) {
		size = 1 + 2 + (len * 2 + 2);
	} else {
		size = 1 + 2;
	}

	net_buf_add_u8(buf, header);
	net_buf_add_be16(buf, size);
	for (i = 0; i < len; i++) {
		net_buf_add_u8(buf, 0x00); /* string to Unicode */
		net_buf_add_u8(buf, data[i]);
	}

	/* End */
	if (len > 0) {
		net_buf_add_u8(buf, 0x00);
		net_buf_add_u8(buf, 0x00);
	}
	return size;
}

static uint16_t goep_header_append_encoding_1(struct net_buf *buf, uint8_t flag, uint8_t *data,
					      uint16_t len)
{
	uint16_t size;
	uint8_t header = GOEP_CREATE_HEADER_ID(GOEP_HEADER_ENCODING_BYTE_SEQUENCE, flag);

	size = 1 + 2 + len;
	net_buf_add_u8(buf, header);
	net_buf_add_be16(buf, size);
	net_buf_add_mem(buf, data, len);

	return size;
}

static uint16_t goep_header_append_encoding_3(struct net_buf *buf, uint8_t flag, uint32_t data)
{
	uint8_t header = GOEP_CREATE_HEADER_ID(GOEP_HEADER_ENCODING_4_BYTE_QUANTITY, flag);

	net_buf_add_u8(buf, header);
	net_buf_add_be32(buf, data);

	return (1 + sizeof(data));
}

static void goep_set_next_state(struct bt_goep_client *client, uint8_t state)
{
	client->state = state;
}

static int goep_client_send_to_rfcomm(struct bt_goep_client *client, struct net_buf *buf,
				      uint8_t state)
{
	int ret;

	if (!buf) {
		return -EINVAL;
	}

	ret = bt_rfcomm_dlc_send(&client->rfcomm_dlc, buf);
	if (ret < 0) {
		LOG_ERR("goep state %d send failed: %d", state, ret);
		net_buf_unref(buf);
		return ret;
	}

	goep_set_next_state(client, state);
	return ret;
}

static int goep_client_find_header_param(uint16_t flag, struct bt_goep_header_param *header,
					 uint8_t size)
{
	int i;

	for (i = 0; i < size; i++) {
		if (header[i].flag == flag) {
			return i;
		}
	}

	return -EINVAL;
}

struct net_buf *goep_client_create_pdu(uint8_t opcode, struct bt_goep_header_param *header,
				       uint8_t size, uint32_t connection_id, uint16_t flag)
{
	struct net_buf *buf;
	struct bt_goep_field_head *field;
	uint16_t len = 0;
	int index;

	buf = bt_rfcomm_create_pdu(&goep_tx_pool);
	if (!buf) {
		LOG_ERR("can't alloc buf");
		return NULL;
	}

	field = net_buf_add(buf, sizeof(*field));
	memset(field, 0, sizeof(*field));
	field->opcode = opcode;
	field->final_flag = 1;

	len = sizeof(*field);

	if (flag & GOEP_SEND_HEADER_FLAGS) {
		net_buf_add_u8(buf, 2);
		net_buf_add_u8(buf, 0);
		len += 2;
	}

	if (flag & GOEP_SEND_HEADER_CONNECTING_ID) {
		len += goep_header_append_encoding_3(buf, GOEP_HEADER_FLAG_CONNECTION_ID,
						     connection_id);
	}

	if (flag & GOEP_SEND_HEADER_NAME) {
		index = goep_client_find_header_param(GOEP_SEND_HEADER_NAME, header, size);
		if (index >= 0) {
			len += goep_header_append_encoding_0(
				buf, GOEP_HEADER_FLAG_NAME, header[index].value, header[index].len);
		}
	}

	if (flag & GOEP_SEND_HEADER_TYPE) {
		index = goep_client_find_header_param(GOEP_SEND_HEADER_TYPE, header, size);
		if (index >= 0) {
			len += goep_header_append_encoding_1(
				buf, GOEP_HEADER_FLAG_TYPE, header[index].value, header[index].len);
		}
	}

	if (flag & GOEP_SEND_HEADER_APP_PARAM) {
		index = goep_client_find_header_param(GOEP_SEND_HEADER_APP_PARAM, header, size);
		if (index >= 0) {
			len += goep_header_append_encoding_1(buf, GOEP_HEADER_FLAG_APP_PARAM,
							     header[index].value,
							     header[index].len);
		}
	}

	GOEP_PUT_VALUE_16(len, field->pkt_len);
	return buf;
}

static int goep_client_connect_profile(struct bt_goep_client *client)
{
	int ret;
	uint16_t size;
	struct net_buf *buf;
	struct bt_goep_connect_field *field;

	buf = bt_rfcomm_create_pdu(&goep_tx_pool);
	if (!buf) {
		LOG_ERR("can't alloc buf");
		return -ENOMEM;
	}

	field = net_buf_add(buf, sizeof(*field));
	memset(field, 0, sizeof(*field));
	field->opcode = GOEP_OPCODE_REQ_CONNECT;
	field->final_flag = 1;
	field->version = GOEP_CLIENT_VERSION;
	field->flags = 0;
	GOEP_PUT_VALUE_16(GEOP_MAX_PKT_LEN, field->max_pkt_len);

	size = sizeof(*field);
	size += goep_header_append_encoding_1(buf, GOEP_HEADER_FLAG_TARGET, client->target_uuid,
					      client->uuid_len);

	GOEP_PUT_VALUE_16(size, field->pkt_len);
	ret = goep_client_send_to_rfcomm(client, buf, GOEP_CLIENT_STATE_CONNECTING_PROFILE);
	if (ret < 0) {
		LOG_ERR("geop send rfcomm fail, err:%d", ret);
		return ret;
	}

	return 0;
}

static void goep_header_body_handle(struct bt_goep_client *client, struct net_buf *buf)
{
	uint16_t size;
	struct bt_goep_header_body *header;

	header = (void *)buf->data;
	size = GOEP_GET_VALUE_16(header->len);
	size -= sizeof(*header);

	if (client->cb && client->cb->recv) {
		client->cb->recv(client, GOEP_RECV_HEADER_BODY, header->data, size);
	}
}

static void goep_header_end_of_body_handle(struct bt_goep_client *client, struct net_buf *buf)
{
	uint16_t size;
	struct bt_goep_header_body *header;

	header = (void *)buf->data;
	size = GOEP_GET_VALUE_16(header->len);
	size -= sizeof(*header);

	if (client->cb && client->cb->recv) {
		client->cb->recv(client, GOEP_RECV_HEADER_END_OF_BODY, (size ? header->data : NULL),
				 size);
	}
}

static void goep_header_who_handle(struct bt_goep_client *client, struct net_buf *buf)
{
	uint16_t size;
	struct bt_goep_header_who *header;

	header = (void *)buf->data;
	size = GOEP_GET_VALUE_16(header->len);
	size -= sizeof(*header);
}

static void goep_header_connection_id_handle(struct bt_goep_client *client, struct net_buf *buf)
{
	uint32_t id;
	struct bt_goep_header_connection_id *header;

	header = (void *)buf->data;
	id = GOEP_GET_VALUE_32(header->id);

	client->connection_id = id;
}

static void goep_header_app_param_handle(struct bt_goep_client *client, struct net_buf *buf)
{
	uint16_t size;
	struct bt_goep_header_app_param *header;

	header = (void *)buf->data;
	size = GOEP_GET_VALUE_16(header->len);
	size -= sizeof(*header);

	if (client->cb && client->cb->recv) {
		client->cb->recv(client, GOEP_RECV_HEADER_APP_PARAM, header->data, size);
	}
}

struct goep_parse_header_handle {
	uint8_t flag;
	void (*func)(struct bt_goep_client *client, struct net_buf *buf);
};

static const struct goep_parse_header_handle parse_handler[] = {
	{GOEP_HEADER_FLAG_BODY, goep_header_body_handle},
	{GOEP_HEADER_FLAG_END_OF_BODY, goep_header_end_of_body_handle},
	{GOEP_HEADER_FLAG_WHO, goep_header_who_handle},
	{GOEP_HEADER_FLAG_CONNECTION_ID, goep_header_connection_id_handle},
	{GOEP_HEADER_FLAG_APP_PARAM, goep_header_app_param_handle},
};

static uint16_t goep_client_pull_one_header(struct net_buf *buf)
{
	uint16_t header_len = 0;
	struct bt_goep_header_len *header = (void *)buf->data;

	switch (header->encoding) {
	case GOEP_HEADER_ENCODING_UNICODE_TEXT:
	case GOEP_HEADER_ENCODING_BYTE_SEQUENCE:
		header_len = GOEP_GET_VALUE_16(header->len);
		break;
	case GOEP_HEADER_ENCODING_1_BYTE_QUANTITY:
		header_len = 1 + 1;
		break;
	case GOEP_HEADER_ENCODING_4_BYTE_QUANTITY:
		header_len = 1 + 4;
		break;
	}

	net_buf_pull(buf, header_len);
	return header_len;
}

static uint8_t goep_client_parse_header(struct bt_goep_client *client, struct net_buf *buf,
					uint16_t headers_len)
{
	struct bt_goep_header *header;
	uint16_t parse_len, i;
	uint8_t flag = 0;

	while (headers_len > 0) {
		header = (void *)buf->data;

		for (i = 0; i < ARRAY_SIZE(parse_handler); i++) {
			if (header->flag == parse_handler[i].flag) {
				if (header->flag == GOEP_HEADER_FLAG_BODY) {
					flag = 1;
				}
				parse_handler[i].func(client, buf);
				break;
			}
		}

		parse_len = goep_client_pull_one_header(buf);
		headers_len -= parse_len;
	}

	return flag;
}

static void goep_connecting_rsp_success_handle(struct bt_goep_client *client, struct net_buf *buf)
{
	uint16_t size;
	struct bt_goep_connect_field *field = (void *)buf->data;

	net_buf_pull(buf, sizeof(*field));

	size = GOEP_GET_VALUE_16(field->pkt_len);
	size -= sizeof(*field);
	goep_client_parse_header(client, buf, size);

	goep_set_next_state(client, GOEP_CLIENT_STATE_PROFILE_CONNECTED);
	if (client->cb && client->cb->connected) {
		client->cb->connected(client);
	}
}

static void goep_getting_rsp_handle(struct bt_goep_client *client, struct net_buf *buf)
{
	uint8_t flag;
	uint16_t size;
	struct bt_goep_field_head *field = (void *)buf->data;

	net_buf_pull(buf, sizeof(*field));
	size = GOEP_GET_VALUE_16(field->pkt_len);
	size -= sizeof(*field);
	flag = goep_client_parse_header(client, buf, size);

	goep_set_next_state(client, GOEP_CLIENT_STATE_PROFILE_CONNECTED);
	if (!flag && field->opcode == GOEP_OPCODE_RSP_CONTINUE && client->cb && client->cb->recv) {
		client->cb->recv(client, GOEP_RECV_CONTINUE_WITHOUT_BODY, NULL, 0);
	}
}

static void goep_aborting_rsp_success_handle(struct bt_goep_client *client, struct net_buf *buf)
{
	goep_set_next_state(client, GOEP_CLIENT_STATE_PROFILE_CONNECTED);
	if (client->cb && client->cb->aborted) {
		client->cb->aborted(client);
	}
}

static void goep_disconnecting_rsp_success_handle(struct bt_goep_client *client,
						  struct net_buf *buf)
{
	client->connection_id = 0;

	goep_client_send_to_rfcomm(client, NULL, GOEP_CLIENT_STATE_RFCOMM_DISCONNECTING);
	bt_rfcomm_dlc_disconnect(&client->rfcomm_dlc);
}

static void goep_setpath_rsp_success_handle(struct bt_goep_client *client, struct net_buf *buf)
{
	goep_set_next_state(client, GOEP_CLIENT_STATE_PROFILE_CONNECTED);
	if (client->cb && client->cb->setpath) {
		client->cb->setpath(client);
	}
}

struct goep_client_handler {
	uint8_t state;
	uint8_t code;
	void (*func)(struct bt_goep_client *client, struct net_buf *buf);
};

static const struct goep_client_handler client_handler[] = {
	{GOEP_CLIENT_STATE_CONNECTING_PROFILE, GOEP_OPCODE_RSP_SUCCESS,
	 goep_connecting_rsp_success_handle},
	{GOEP_CLIENT_STATE_GETTING, GOEP_OPCODE_RSP_CONTINUE, goep_getting_rsp_handle},
	{GOEP_CLIENT_STATE_GETTING, GOEP_OPCODE_RSP_SUCCESS, goep_getting_rsp_handle},
	{GOEP_CLIENT_STATE_ABORTING, GOEP_OPCODE_RSP_SUCCESS, goep_aborting_rsp_success_handle},
	{GOEP_CLIENT_STATE_DISCONNECTING, GOEP_OPCODE_RSP_SUCCESS,
	 goep_disconnecting_rsp_success_handle},
	{GOEP_CLIENT_STATE_SETPATH, GOEP_OPCODE_RSP_SUCCESS, goep_setpath_rsp_success_handle}};

static void goep_client_rfcomm_recv(struct bt_rfcomm_dlc *dlci, struct net_buf *buf)
{
	int i;
	struct bt_goep_client *client;
	struct bt_goep_field_head *field_head;

	k_mutex_lock(&goep_client_lock, K_FOREVER);
	client = goep_find_client_by_dlci(dlci);
	if (!client) {
		goto exit;
	}

	field_head = (struct bt_goep_field_head *)buf->data;
	LOG_DBG("goep rx %d, 0x%x %d", client->state, field_head->opcode,
		GOEP_GET_VALUE_16(field_head->pkt_len));

	for (i = 0; i < ARRAY_SIZE(client_handler); i++) {
		if (client->state == client_handler[i].state &&
		    field_head->opcode == client_handler[i].code) {
			client_handler[i].func(client, buf);
			break;
		}
	}

exit:
	k_mutex_unlock(&goep_client_lock);
}

static void goep_client_rfcomm_connected(struct bt_rfcomm_dlc *dlci)
{
	struct bt_goep_client *client;

	k_mutex_lock(&goep_client_lock, K_FOREVER);
	client = goep_find_client_by_dlci(dlci);
	if (!client) {
		LOG_WRN("Can't find client");
		goto exit;
	}

	goep_set_next_state(client, GOEP_CLIENT_STATE_RFCOMM_CONNECTED);
	goep_client_connect_profile(client);

exit:
	k_mutex_unlock(&goep_client_lock);
}

static void goep_client_rfcomm_disconnected(struct bt_rfcomm_dlc *dlci)
{
	struct bt_goep_client *client;

	k_mutex_lock(&goep_client_lock, K_FOREVER);
	client = goep_find_client_by_dlci(dlci);
	if (!client) {
		LOG_WRN("Can't find client");
		goto exit;
	}

	goep_set_next_state(client, GOEP_CLIENT_STATE_RFCOMM_DISCONNECTED);
	if (client->cb && client->cb->disconnected) {
		client->cb->disconnected(client);
	}

	goep_client_cleanup(client);

exit:
	k_mutex_unlock(&goep_client_lock);
}

static const struct bt_rfcomm_dlc_ops goep_client_rfcomm_ops = {
	.connected = goep_client_rfcomm_connected,
	.disconnected = goep_client_rfcomm_disconnected,
	.recv = goep_client_rfcomm_recv,
};

struct bt_goep_client *bt_goep_client_connect(struct bt_conn *conn, uint8_t *target_uuid,
					      uint8_t uuid_len, uint8_t server_channel,
					      struct bt_goep_client_cb *cb)
{
	int ret;
	struct bt_goep_client *client;

	k_mutex_lock(&goep_client_lock, K_FOREVER);

	client = &goep_clients[bt_conn_index(conn)];
	client->target_uuid = target_uuid;
	client->uuid_len = uuid_len;
	client->cb = cb;
	client->conn = conn;
	client->rfcomm_dlc.ops = (void *)&goep_client_rfcomm_ops;
	client->rfcomm_dlc.mtu = GEOP_RFCOMM_MTU;

	ret = bt_rfcomm_dlc_connect(conn, &client->rfcomm_dlc, server_channel);
	if (ret) {
		goep_client_cleanup(client);
		client = NULL;
		goto exit;
	}

	goep_set_next_state(client, GOEP_CLIENT_STATE_CONNECTING_RFCOMM);

exit:
	k_mutex_unlock(&goep_client_lock);
	return client;
}

int bt_goep_client_get(struct bt_goep_client *client, uint16_t flag,
		       struct bt_goep_header_param *header, uint8_t size)
{
	struct net_buf *buf;
	int ret;

	if (!client) {
		return -EINVAL;
	}

	k_mutex_lock(&goep_client_lock, K_FOREVER);

	buf = goep_client_create_pdu(GOEP_OPCODE_REQ_GET, header, size, client->connection_id,
				     flag);
	if (!buf) {
		LOG_ERR("build packet fai");
		ret = -ENOMEM;
		goto exit;
	}

	ret = goep_client_send_to_rfcomm(client, buf, GOEP_CLIENT_STATE_GETTING);
	if (ret < 0) {
		LOG_ERR("send rfcomm fail, err:%d", ret);
		goto exit;
	}

exit:
	k_mutex_unlock(&goep_client_lock);
	return ret;
}

int bt_goep_client_setpath(struct bt_goep_client *client, uint16_t flag,
			   struct bt_goep_header_param *header, uint8_t size)
{
	struct net_buf *buf;
	int ret;

	if (!client) {
		return -EINVAL;
	}

	k_mutex_lock(&goep_client_lock, K_FOREVER);

	buf = goep_client_create_pdu(GOEP_OPCODE_REQ_SETPATH, header, size, client->connection_id,
				     flag);
	if (!buf) {
		LOG_ERR("build packet fai");
		ret = -ENOMEM;
		goto exit;
	}

	ret = goep_client_send_to_rfcomm(client, buf, GOEP_CLIENT_STATE_SETPATH);
	if (ret < 0) {
		LOG_ERR("send rfcomm fail, err:%d", ret);
		goto exit;
	}

exit:
	k_mutex_unlock(&goep_client_lock);
	return ret;
}

int bt_goep_client_abort(struct bt_goep_client *client, uint16_t flag)
{
	struct net_buf *buf;
	int ret;

	if (!client) {
		return -EINVAL;
	}

	k_mutex_lock(&goep_client_lock, K_FOREVER);

	buf = goep_client_create_pdu(GOEP_OPCODE_REQ_ABORT, NULL, 0, client->connection_id, flag);
	if (!buf) {
		LOG_ERR("build packet fai");
		ret = -ENOMEM;
		goto exit;
	}

	ret = goep_client_send_to_rfcomm(client, buf, GOEP_CLIENT_STATE_ABORTING);
	if (ret < 0) {
		LOG_ERR("send rfcomm fail, err:%d", ret);
		goto exit;
	}

exit:
	k_mutex_unlock(&goep_client_lock);
	return ret;
}

int bt_goep_client_disconnect(struct bt_goep_client *client, uint16_t flag)
{
	struct net_buf *buf;
	int ret;

	if (!client) {
		return -EINVAL;
	}

	k_mutex_lock(&goep_client_lock, K_FOREVER);

	buf = goep_client_create_pdu(GOEP_OPCODE_REQ_DISCONNECT, NULL, 0, client->connection_id,
				     flag);
	if (!buf) {
		LOG_ERR("build packet fai");
		ret = -ENOMEM;
		goto exit;
	}

	ret = goep_client_send_to_rfcomm(client, buf, GOEP_CLIENT_STATE_DISCONNECTING);
	if (ret < 0) {
		LOG_ERR("send rfcomm fail, err:%d", ret);
		goto exit;
	}

exit:
	k_mutex_unlock(&goep_client_lock);
	return ret;
}
