/* obex.c - IrDA Oject Exchange Protocol handling */

/*
 * Copyright 2024-2025 NXP
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

#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/bluetooth/classic/rfcomm.h>
#include <zephyr/bluetooth/classic/sdp.h>
#include <zephyr/bluetooth/classic/obex.h>

#include "obex_internal.h"

#define LOG_LEVEL CONFIG_BT_GOEP_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_obex);

#define OBEX_SERVER(node) CONTAINER_OF(node, struct bt_obex_server, _node)

static struct net_buf *obex_alloc_buf(struct bt_obex *obex)
{
	struct net_buf *buf;

	if (obex == NULL || obex->_transport_ops == NULL ||
	    obex->_transport_ops->alloc_buf == NULL) {
		LOG_WRN("Buffer allocation is unsupported");
		return NULL;
	}

	buf = obex->_transport_ops->alloc_buf(obex, NULL);
	if (!buf) {
		LOG_WRN("Fail to alloc buffer");
	}
	return buf;
}

static int obex_send(struct bt_obex *obex, uint16_t mopl, struct net_buf *buf)
{
	if (obex == NULL || obex->_transport_ops == NULL || obex->_transport_ops->send == NULL) {
		LOG_WRN("OBEX sending is unsupported");
		return -EIO;
	}

	if (buf->len > mopl) {
		LOG_WRN("Data length too long (%d > %d)", buf->len, mopl);
		return -EMSGSIZE;
	}

	return obex->_transport_ops->send(obex, buf);
}

static int obex_transport_disconn(struct bt_obex *obex)
{
	int err;

	if (obex == NULL || obex->_transport_ops == NULL ||
	    obex->_transport_ops->disconnect == NULL) {
		LOG_WRN("OBEX transport disconn is unsupported");
		return -EIO;
	}

	err = obex->_transport_ops->disconnect(obex);
	if (err) {
		LOG_ERR("Fail to disconnect transport (err %d)", err);
	}
	return -EINVAL;
}

struct bt_obex_has_header {
	uint8_t id;
	bool found;
};

static bool bt_obex_has_header_cb(struct bt_obex_hdr *hdr, void *user_data)
{
	struct bt_obex_has_header *data;

	data = user_data;

	if (hdr->id == data->id) {
		data->found = true;
		return false;
	}
	return true;
}

bool bt_obex_has_header(struct net_buf *buf, uint8_t id)
{
	struct bt_obex_has_header data;
	int err;

	if (buf == NULL) {
		LOG_WRN("Invalid parameter");
		return false;
	}

	data.id = id;
	data.found = false;

	err = bt_obex_header_parse(buf, bt_obex_has_header_cb, &data);
	if (err != 0) {
		return false;
	}

	return data.found;
}

int bt_obex_make_uuid(union bt_obex_uuid *uuid, const uint8_t *data, uint16_t len)
{
	switch (len) {
	case BT_UUID_SIZE_16:
		uuid->uuid.type = BT_UUID_TYPE_16;
		uuid->u16.val = sys_get_be16(data);
		break;
	case BT_UUID_SIZE_32:
		uuid->uuid.type = BT_UUID_TYPE_32;
		uuid->u32.val = sys_get_be32(data);
		break;
	case BT_UUID_SIZE_128:
		uuid->uuid.type = BT_UUID_TYPE_128;
		sys_memcpy_swap(uuid->u128.val, data, len);
		break;
	default:
		LOG_WRN("Unsupported UUID len %u", len);
		return -EINVAL;
	}

	return 0;
}

struct server_handler {
	uint8_t opcode;
	uint16_t min_len;
	int (*handler)(struct bt_obex_server *server, uint16_t len, struct net_buf *buf);
};

static int obex_server_connect(struct bt_obex_server *server, uint16_t len, struct net_buf *buf)
{
	struct bt_obex_conn_req_hdr *hdr;
	struct bt_obex_conn_rsp_hdr *rsp_conn_hdr;
	struct bt_obex_rsp_hdr *rsp_hdr;
	uint8_t version;
	__maybe_unused uint8_t flags;
	uint16_t mopl;
	uint8_t rsp_code;
	int err;
	uint16_t target_len;
	const uint8_t *target_data;

	LOG_DBG("");

	if ((len != buf->len) || (buf->len < sizeof(*hdr))) {
		LOG_WRN("Invalid packet size");
		rsp_code = BT_OBEX_RSP_CODE_PRECON_FAIL;
		goto failed;
	}

	if (atomic_get(&server->_state) != BT_OBEX_DISCONNECTED) {
		LOG_WRN("Invalid state, connect refused");
		rsp_code = BT_OBEX_RSP_CODE_FORBIDDEN;
		goto failed;
	}

	if (server->ops->connect == NULL) {
		LOG_WRN("Conn req handling not implemented");
		rsp_code = BT_OBEX_RSP_CODE_NOT_IMPL;
		goto failed;
	}

	if (!atomic_cas(&server->_opcode, 0, BT_OBEX_OPCODE_CONNECT)) {
		LOG_WRN("Unexpected conn request");
		rsp_code = BT_OBEX_RSP_CODE_FORBIDDEN;
		goto failed;
	}

	version = net_buf_pull_u8(buf);
	flags = net_buf_pull_u8(buf);
	mopl = net_buf_pull_be16(buf);

	atomic_set_bit_to(&server->_flags, BT_OBEX_HAS_TARGET,
			  bt_obex_has_header(buf, BT_OBEX_HEADER_ID_TARGET));

	err = bt_obex_get_header_target(buf, &target_len, &target_data);
	if (err != 0 && atomic_test_bit(&server->_flags, BT_OBEX_HAS_TARGET)) {
		LOG_WRN("Invalid target header");
		rsp_code = BT_OBEX_RSP_CODE_PRECON_FAIL;
		goto failed;
	}

	if (err == 0) {
		err = bt_obex_make_uuid(&server->_target, target_data, target_len);
		if (err != 0) {
			LOG_WRN("Invalid UUID header");
			rsp_code = BT_OBEX_RSP_CODE_PRECON_FAIL;
			goto failed;
		}
	}

	LOG_DBG("version %u, flags %u, mopl %u", version, flags, mopl);

	if (mopl < BT_OBEX_MIN_MTU) {
		LOG_WRN("Invalid MOPL (%d < %d)", mopl, BT_OBEX_MIN_MTU);
		rsp_code = BT_OBEX_RSP_CODE_PRECON_FAIL;
		goto failed;
	}

	if (mopl > server->obex->tx.mtu) {
		LOG_WRN("MOPL exceeds MTU (%d > %d)", mopl, server->obex->tx.mtu);
		rsp_code = BT_OBEX_RSP_CODE_PRECON_FAIL;
		goto failed;
	}

	server->tx.mopl = mopl;

	atomic_set(&server->_state, BT_OBEX_CONNECTING);
	server->ops->connect(server, version, mopl, buf);
	return 0;

failed:
	buf = obex_alloc_buf(server->obex);
	if (!buf) {
		LOG_WRN("Cannot allocate buffer");
		return -ENOBUFS;
	}

	rsp_hdr = net_buf_add(buf, sizeof(*rsp_hdr));
	rsp_hdr->code = rsp_code;
	rsp_conn_hdr = net_buf_add(buf, sizeof(*rsp_conn_hdr));
	rsp_conn_hdr->flags = 0;
	rsp_conn_hdr->mopl = 0;
	rsp_conn_hdr->version = BT_OBEX_VERSION;
	rsp_hdr->len = sys_cpu_to_be16(buf->len);

	err = obex_send(server->obex, server->tx.mopl, buf);
	if (err) {
		net_buf_unref(buf);
	}

	return err;
}

static int obex_server_disconn(struct bt_obex_server *server, uint16_t len, struct net_buf *buf)
{
	struct bt_obex_rsp_hdr *rsp_hdr;
	uint8_t rsp_code;
	int err;

	LOG_DBG("");

	if (len != buf->len) {
		LOG_WRN("Invalid packet size");
		rsp_code = BT_OBEX_RSP_CODE_PRECON_FAIL;
		goto failed;
	}

	if (atomic_get(&server->_state) != BT_OBEX_CONNECTED) {
		LOG_WRN("Invalid state, connect is not established");
		rsp_code = BT_OBEX_RSP_CODE_FORBIDDEN;
		goto failed;
	}

	if (server->ops->disconnect == NULL) {
		LOG_WRN("Conn req handling not implemented");
		rsp_code = BT_OBEX_RSP_CODE_NOT_IMPL;
		goto failed;
	}

	if (!atomic_cas(&server->_opcode, 0, BT_OBEX_OPCODE_DISCONN)) {
		LOG_WRN("Unexpected disconn request");
		rsp_code = BT_OBEX_RSP_CODE_FORBIDDEN;
		goto failed;
	}

	atomic_set(&server->_state, BT_OBEX_DISCONNECTING);
	server->ops->disconnect(server, buf);
	return 0;

failed:
	buf = obex_alloc_buf(server->obex);
	if (!buf) {
		LOG_WRN("Cannot allocate buffer");
		return -ENOBUFS;
	}

	rsp_hdr = net_buf_add(buf, sizeof(*rsp_hdr));
	rsp_hdr->code = rsp_code;
	rsp_hdr->len = sys_cpu_to_be16(buf->len);

	err = obex_send(server->obex, server->tx.mopl, buf);
	if (err) {
		net_buf_unref(buf);
	}

	return err;
}

static int obex_server_put_common(struct bt_obex_server *server, bool final, uint16_t len,
				  struct net_buf *buf)
{
	struct bt_obex_rsp_hdr *rsp_hdr;
	uint8_t rsp_code;
	uint8_t req_code;
	uint8_t opcode;
	int err;

	LOG_DBG("");

	if (len != buf->len) {
		LOG_WRN("Invalid packet size");
		rsp_code = BT_OBEX_RSP_CODE_PRECON_FAIL;
		goto failed;
	}

	if (atomic_get(&server->_state) != BT_OBEX_CONNECTED) {
		LOG_WRN("Invalid state, connect is not established");
		rsp_code = BT_OBEX_RSP_CODE_FORBIDDEN;
		goto failed;
	}

	if (server->ops->put == NULL) {
		LOG_WRN("Put req handling not implemented");
		rsp_code = BT_OBEX_RSP_CODE_NOT_IMPL;
		goto failed;
	}

	req_code = final ? BT_OBEX_OPCODE_PUT_F : BT_OBEX_OPCODE_PUT;
	if (!atomic_cas(&server->_opcode, 0, req_code)) {
		opcode = atomic_get(&server->_opcode);
		if ((opcode != BT_OBEX_OPCODE_PUT_F) && (opcode != BT_OBEX_OPCODE_PUT)) {
			LOG_WRN("Unexpected put request");
			rsp_code = BT_OBEX_RSP_CODE_FORBIDDEN;
			goto failed;
		}

		if (!final && (opcode == BT_OBEX_OPCODE_PUT_F)) {
			LOG_WRN("Unexpected put request without final bit");
			rsp_code = BT_OBEX_RSP_CODE_FORBIDDEN;
			goto failed;
		}

		if (opcode != req_code) {
			atomic_cas(&server->_opcode, opcode, req_code);
		}
	}

	server->ops->put(server, final, buf);
	return 0;

failed:
	buf = obex_alloc_buf(server->obex);
	if (!buf) {
		LOG_WRN("Cannot allocate buffer");
		return -ENOBUFS;
	}

	rsp_hdr = net_buf_add(buf, sizeof(*rsp_hdr));
	rsp_hdr->code = rsp_code;
	rsp_hdr->len = sys_cpu_to_be16(buf->len);

	err = obex_send(server->obex, server->tx.mopl, buf);
	if (err) {
		net_buf_unref(buf);
	}

	return err;
}

static int obex_server_put(struct bt_obex_server *server, uint16_t len, struct net_buf *buf)
{
	return obex_server_put_common(server, false, len, buf);
}

static int obex_server_put_final(struct bt_obex_server *server, uint16_t len, struct net_buf *buf)
{
	return obex_server_put_common(server, true, len, buf);
}

static int obex_server_get_common(struct bt_obex_server *server, bool final, uint16_t len,
				  struct net_buf *buf)
{
	struct bt_obex_rsp_hdr *rsp_hdr;
	uint8_t rsp_code;
	uint8_t req_code;
	uint8_t opcode;
	int err;

	LOG_DBG("");

	if (len != buf->len) {
		LOG_WRN("Invalid packet size");
		rsp_code = BT_OBEX_RSP_CODE_PRECON_FAIL;
		goto failed;
	}

	if (atomic_get(&server->_state) != BT_OBEX_CONNECTED) {
		LOG_WRN("Invalid state, connect is not established");
		rsp_code = BT_OBEX_RSP_CODE_FORBIDDEN;
		goto failed;
	}

	if (server->ops->get == NULL) {
		LOG_WRN("Get req handling not implemented");
		rsp_code = BT_OBEX_RSP_CODE_NOT_IMPL;
		goto failed;
	}

	req_code = final ? BT_OBEX_OPCODE_GET_F : BT_OBEX_OPCODE_GET;
	if (!atomic_cas(&server->_opcode, 0, req_code)) {
		opcode = atomic_get(&server->_opcode);
		if ((opcode != BT_OBEX_OPCODE_GET_F) && (opcode != BT_OBEX_OPCODE_GET)) {
			LOG_WRN("Unexpected get request");
			rsp_code = BT_OBEX_RSP_CODE_FORBIDDEN;
			goto failed;
		}

		if (!final && (opcode == BT_OBEX_OPCODE_GET_F)) {
			LOG_WRN("Unexpected get request without final bit");
			rsp_code = BT_OBEX_RSP_CODE_FORBIDDEN;
			goto failed;
		}

		if (opcode != req_code) {
			atomic_cas(&server->_opcode, opcode, req_code);
		}
	}

	server->ops->get(server, final, buf);
	return 0;

failed:
	buf = obex_alloc_buf(server->obex);
	if (!buf) {
		LOG_WRN("Cannot allocate buffer");
		return -ENOBUFS;
	}

	rsp_hdr = net_buf_add(buf, sizeof(*rsp_hdr));
	rsp_hdr->code = rsp_code;
	rsp_hdr->len = sys_cpu_to_be16(buf->len);

	err = obex_send(server->obex, server->tx.mopl, buf);
	if (err) {
		net_buf_unref(buf);
	}

	return err;
}

static int obex_server_get(struct bt_obex_server *server, uint16_t len, struct net_buf *buf)
{
	return obex_server_get_common(server, false, len, buf);
}

static int obex_server_get_final(struct bt_obex_server *server, uint16_t len, struct net_buf *buf)
{
	return obex_server_get_common(server, true, len, buf);
}

static int obex_server_setpath(struct bt_obex_server *server, uint16_t len, struct net_buf *buf)
{
	struct bt_obex_rsp_hdr *rsp_hdr;
	struct bt_obex_setpath_req_hdr *req_hdr;
	uint8_t rsp_code;
	int err;

	LOG_DBG("");

	if ((len != buf->len) || (buf->len < sizeof(*req_hdr))) {
		LOG_WRN("Invalid packet size");
		rsp_code = BT_OBEX_RSP_CODE_PRECON_FAIL;
		goto failed;
	}

	if (atomic_get(&server->_state) != BT_OBEX_CONNECTED) {
		LOG_WRN("Invalid state, connect is not established");
		rsp_code = BT_OBEX_RSP_CODE_FORBIDDEN;
		goto failed;
	}

	if (server->ops->setpath == NULL) {
		LOG_WRN("Setpath req handling not implemented");
		rsp_code = BT_OBEX_RSP_CODE_NOT_IMPL;
		goto failed;
	}

	if (!atomic_cas(&server->_opcode, 0, BT_OBEX_OPCODE_SETPATH)) {
		LOG_WRN("Unexpected setpath request");
		rsp_code = BT_OBEX_RSP_CODE_FORBIDDEN;
		goto failed;
	}

	req_hdr = net_buf_pull_mem(buf, sizeof(*req_hdr));
	server->ops->setpath(server, req_hdr->flags, buf);
	return 0;

failed:
	buf = obex_alloc_buf(server->obex);
	if (!buf) {
		LOG_WRN("Cannot allocate buffer");
		return -ENOBUFS;
	}

	rsp_hdr = net_buf_add(buf, sizeof(*rsp_hdr));
	rsp_hdr->code = rsp_code;
	rsp_hdr->len = sys_cpu_to_be16(buf->len);

	err = obex_send(server->obex, server->tx.mopl, buf);
	if (err) {
		net_buf_unref(buf);
	}

	return err;
}

static int obex_server_action_common(struct bt_obex_server *server, bool final, uint16_t len,
				     struct net_buf *buf)
{
	struct bt_obex_rsp_hdr *rsp_hdr;
	uint8_t rsp_code;
	uint8_t req_code;
	uint8_t opcode;
	int err;

	LOG_DBG("");

	if (len != buf->len) {
		LOG_WRN("Invalid packet size");
		rsp_code = BT_OBEX_RSP_CODE_PRECON_FAIL;
		goto failed;
	}

	if (atomic_get(&server->_state) != BT_OBEX_CONNECTED) {
		LOG_WRN("Invalid state, connect is not established");
		rsp_code = BT_OBEX_RSP_CODE_FORBIDDEN;
		goto failed;
	}

	if (server->ops->action == NULL) {
		LOG_WRN("Action req handling not implemented");
		rsp_code = BT_OBEX_RSP_CODE_NOT_IMPL;
		goto failed;
	}

	req_code = final ? BT_OBEX_OPCODE_ACTION_F : BT_OBEX_OPCODE_ACTION;
	if (!atomic_cas(&server->_opcode, 0, req_code)) {
		opcode = atomic_get(&server->_opcode);
		if ((opcode != BT_OBEX_OPCODE_ACTION_F) && (opcode != BT_OBEX_OPCODE_ACTION)) {
			LOG_WRN("Unexpected action request");
			rsp_code = BT_OBEX_RSP_CODE_FORBIDDEN;
			goto failed;
		}

		if (!final && (opcode == BT_OBEX_OPCODE_ACTION_F)) {
			LOG_WRN("Unexpected action request without final bit");
			rsp_code = BT_OBEX_RSP_CODE_FORBIDDEN;
			goto failed;
		}

		if (opcode != req_code) {
			atomic_cas(&server->_opcode, opcode, req_code);
		}
	}

	server->ops->action(server, final, buf);
	return 0;

failed:
	buf = obex_alloc_buf(server->obex);
	if (!buf) {
		LOG_WRN("Cannot allocate buffer");
		return -ENOBUFS;
	}

	rsp_hdr = net_buf_add(buf, sizeof(*rsp_hdr));
	rsp_hdr->code = rsp_code;
	rsp_hdr->len = sys_cpu_to_be16(buf->len);

	err = obex_send(server->obex, server->tx.mopl, buf);
	if (err) {
		net_buf_unref(buf);
	}

	return err;
}

static int obex_server_action(struct bt_obex_server *server, uint16_t len, struct net_buf *buf)
{
	return obex_server_action_common(server, false, len, buf);
}

static int obex_server_action_final(struct bt_obex_server *server, uint16_t len,
				    struct net_buf *buf)
{
	return obex_server_action_common(server, true, len, buf);
}

static int obex_server_session(struct bt_obex_server *server, uint16_t len, struct net_buf *buf)
{
	struct bt_obex_rsp_hdr *rsp_hdr;
	uint8_t rsp_code;
	int err;

	LOG_DBG("");

	if (len != buf->len) {
		LOG_WRN("Invalid packet size");
		rsp_code = BT_OBEX_RSP_CODE_PRECON_FAIL;
		goto failed;
	}

	if (atomic_get(&server->_state) != BT_OBEX_CONNECTED) {
		LOG_WRN("Invalid state, connect is not established");
		rsp_code = BT_OBEX_RSP_CODE_FORBIDDEN;
		goto failed;
	}

	LOG_WRN("Action req handling not implemented");
	rsp_code = BT_OBEX_RSP_CODE_NOT_IMPL;

failed:
	buf = obex_alloc_buf(server->obex);
	if (!buf) {
		LOG_WRN("Cannot allocate buffer");
		return -ENOBUFS;
	}

	rsp_hdr = net_buf_add(buf, sizeof(*rsp_hdr));
	rsp_hdr->code = rsp_code;
	rsp_hdr->len = sys_cpu_to_be16(buf->len);

	err = obex_send(server->obex, server->tx.mopl, buf);
	if (err) {
		net_buf_unref(buf);
	}

	return err;
}

static int obex_server_abort(struct bt_obex_server *server, uint16_t len, struct net_buf *buf)
{
	struct bt_obex_rsp_hdr *rsp_hdr;
	uint8_t rsp_code;
	int err;

	LOG_DBG("");

	if (len != buf->len) {
		LOG_WRN("Invalid packet size");
		rsp_code = BT_OBEX_RSP_CODE_PRECON_FAIL;
		goto failed;
	}

	if (atomic_get(&server->_state) != BT_OBEX_CONNECTED) {
		LOG_WRN("Invalid state, connect is not established");
		rsp_code = BT_OBEX_RSP_CODE_FORBIDDEN;
		goto failed;
	}

	if (server->ops->abort == NULL) {
		LOG_WRN("Abort req handling not implemented");
		rsp_code = BT_OBEX_RSP_CODE_NOT_IMPL;
		goto failed;
	}

	atomic_set(&server->_opcode, BT_OBEX_OPCODE_ABORT);

	server->ops->abort(server, buf);
	return 0;

failed:
	buf = obex_alloc_buf(server->obex);
	if (!buf) {
		LOG_WRN("Cannot allocate buffer");
		return -ENOBUFS;
	}

	rsp_hdr = net_buf_add(buf, sizeof(*rsp_hdr));
	rsp_hdr->code = rsp_code;
	rsp_hdr->len = sys_cpu_to_be16(buf->len);

	err = obex_send(server->obex, server->tx.mopl, buf);
	if (err) {
		net_buf_unref(buf);
	}

	return err;
}

struct server_handler server_handler[] = {
	{BT_OBEX_OPCODE_CONNECT, sizeof(struct bt_obex_conn_req_hdr), obex_server_connect},
	{BT_OBEX_OPCODE_DISCONN, 0, obex_server_disconn},
	{BT_OBEX_OPCODE_PUT, 0, obex_server_put},
	{BT_OBEX_OPCODE_PUT_F, 0, obex_server_put_final},
	{BT_OBEX_OPCODE_GET, 0, obex_server_get},
	{BT_OBEX_OPCODE_GET_F, 0, obex_server_get_final},
	{BT_OBEX_OPCODE_SETPATH, sizeof(struct bt_obex_setpath_req_hdr), obex_server_setpath},
	{BT_OBEX_OPCODE_ACTION, 0, obex_server_action},
	{BT_OBEX_OPCODE_ACTION_F, 0, obex_server_action_final},
	{BT_OBEX_OPCODE_SESSION, 0, obex_server_session},
	{BT_OBEX_OPCODE_ABORT, 0, obex_server_abort},
};

static struct bt_obex_server *obex_get_server_from_conn_id(struct bt_obex *obex, uint32_t conn_id)
{
	struct bt_obex_server *server, *next;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&obex->_servers, server, next, _node) {
		if (server->_conn_id == conn_id) {
			return server;
		}
	}
	return NULL;
}

static struct bt_obex_server *obex_get_server_from_uuid(struct bt_obex *obex, struct bt_uuid *uuid)
{
	struct bt_obex_server *server, *next;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&obex->_servers, server, next, _node) {
		if (server->uuid != NULL && bt_uuid_cmp(&server->uuid->uuid, uuid) == 0) {
			return server;
		}
	}
	return NULL;
}

static struct bt_obex_server *obex_get_server_with_state(struct bt_obex *obex,
							 enum bt_obex_state state)
{
	struct bt_obex_server *server, *next;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&obex->_servers, server, next, _node) {
		if (server->uuid == NULL && atomic_get(&server->_state) == state) {
			return server;
		}
	}
	return NULL;
}

static uint16_t obex_server_get_hdr_len(uint8_t opcode)
{
	ARRAY_FOR_EACH(server_handler, i) {
		if (server_handler[i].opcode == opcode) {
			return server_handler[i].min_len;
		}
	}

	return 0;
}

static int obex_server_recv(struct bt_obex *obex, struct net_buf *buf)
{
	struct bt_obex_req_hdr *hdr = NULL;
	struct bt_obex_rsp_hdr *rsp_hdr;
	struct bt_obex_server *server = NULL;
	uint32_t conn_id;
	uint16_t len;
	uint16_t target_len;
	const uint8_t *target_data;
	uint16_t min_hdr_len;
	int err;
	enum bt_obex_rsp_code rsp_code = BT_OBEX_RSP_CODE_BAD_REQ;

	if (buf->len < sizeof(*hdr)) {
		LOG_WRN("Too small header size (%d < %d)", buf->len, sizeof(*hdr));
		goto failed;
	}

	hdr = net_buf_pull_mem(buf, sizeof(*hdr));
	len = sys_be16_to_cpu(hdr->len);
	min_hdr_len = obex_server_get_hdr_len(hdr->code);

	if (min_hdr_len > buf->len) {
		LOG_WRN("Malformed request (%d > %d)", min_hdr_len, buf->len);
		goto failed;
	}

	if (hdr->code == BT_OBEX_OPCODE_CONNECT) {
		struct net_buf_simple_state state;

		net_buf_simple_save(&buf->b, &state);
		net_buf_pull(buf, min_hdr_len);
		err = bt_obex_get_header_target(buf, &target_len, &target_data);
		net_buf_simple_restore(&buf->b, &state);
		if (err == 0) {
			union bt_obex_uuid u;

			err = bt_obex_make_uuid(&u, target_data, target_len);
			if (err != 0) {
				LOG_WRN("Unsupported target header len %u", target_len);
				goto failed;
			}

			server = obex_get_server_from_uuid(obex, &u.uuid);
		}

		if (server == NULL) {
			server = obex_get_server_with_state(obex, BT_OBEX_DISCONNECTED);
		}
	} else {
		struct net_buf_simple_state state;

		net_buf_simple_save(&buf->b, &state);
		net_buf_pull(buf, min_hdr_len);
		err = bt_obex_get_header_conn_id(buf, &conn_id);
		net_buf_simple_restore(&buf->b, &state);
		if (err == 0) {
			server = obex_get_server_from_conn_id(obex, conn_id);
		} else if (atomic_ptr_get(&obex->_active_server) != NULL) {
			server = atomic_ptr_get(&obex->_active_server);
		} else if (sys_slist_len(&obex->_servers) == 1) {
			/* Only if the count of servers is 1, use it as default. */
			server = OBEX_SERVER(sys_slist_peek_head(&obex->_servers));
		}
	}

	if (server == NULL) {
		LOG_WRN("No server found from %p", obex);
		rsp_code = BT_OBEX_RSP_CODE_NOT_FOUND;
		goto failed;
	}

	ARRAY_FOR_EACH(server_handler, i) {
		if (server_handler[i].opcode == hdr->code) {
			err = server_handler[i].handler(server, len - sizeof(*hdr), buf);
			if (err != 0) {
				atomic_ptr_clear(&obex->_active_server);
				LOG_WRN("Handler err %d", err);
			} else {
				/* Save the server */
				atomic_ptr_set(&obex->_active_server, server);
			}
			return err;
		}
	}

failed:
	LOG_WRN("Failed to process request");
	buf = obex_alloc_buf(obex);
	if (!buf) {
		LOG_ERR("Failed to allocate buf");
		return -ENOBUFS;
	}

	rsp_hdr = (void *)net_buf_add(buf, sizeof(*rsp_hdr));
	rsp_hdr->code = rsp_code;
	if (hdr != NULL && hdr->code == BT_OBEX_OPCODE_CONNECT) {
		struct bt_obex_conn_rsp_hdr *conn_rsp_hdr;

		conn_rsp_hdr = (void *)net_buf_add(buf, sizeof(*conn_rsp_hdr));
		conn_rsp_hdr->flags = 0;
		conn_rsp_hdr->mopl = sys_cpu_to_be16(obex->rx.mtu);
		conn_rsp_hdr->version = BT_OBEX_VERSION;
	}
	rsp_hdr->len = sys_cpu_to_be16(buf->len);

	err = obex_send(obex, BT_OBEX_MIN_MTU, buf);
	if (err) {
		LOG_ERR("Failed to send obex rep err %d", err);
		net_buf_unref(buf);
	}

	return err;
}

struct client_handler {
	uint8_t opcode;
	int (*handler)(struct bt_obex_client *client, uint8_t rsp_code, uint16_t len,
		       struct net_buf *buf);
};

static int obex_client_connect(struct bt_obex_client *client, uint8_t rsp_code, uint16_t len,
			       struct net_buf *buf)
{
	struct bt_obex_conn_rsp_hdr *rsp_conn_hdr;
	uint8_t version = 0;
	__maybe_unused uint8_t flags;
	uint16_t mopl = 0;

	LOG_DBG("");

	atomic_clear(&client->_opcode);
	atomic_ptr_clear(&client->obex->_active_client);

	if (atomic_get(&client->_state) != BT_OBEX_CONNECTING) {
		LOG_WRN("Invalid connection state %u", (uint8_t)atomic_get(&client->_state));
		goto failed;
	}

	if ((len != buf->len) || (buf->len < sizeof(*rsp_conn_hdr))) {
		LOG_WRN("Invalid packet size");
		goto failed;
	}

	version = net_buf_pull_u8(buf);
	flags = net_buf_pull_u8(buf);
	mopl = net_buf_pull_be16(buf);

	LOG_DBG("version %u, flags %u, mopl %u", version, flags, mopl);

	if (atomic_test_bit(&client->_flags, BT_OBEX_HAS_TARGET) &&
	    rsp_code == BT_OBEX_RSP_CODE_SUCCESS) {
		union bt_obex_uuid u;
		int err;
		uint16_t who_len;
		const uint8_t *who_data;

		if (bt_obex_has_header(buf, BT_OBEX_HEADER_ID_WHO) !=
		    bt_obex_has_header(buf, BT_OBEX_HEADER_ID_CONN_ID)) {
			LOG_WRN("Missing required who or connect id headers");
			goto failed;
		}

		if (atomic_test_bit(&client->_flags, BT_OBEX_HAS_TARGET) !=
		    bt_obex_has_header(buf, BT_OBEX_HEADER_ID_WHO)) {
			LOG_WRN("Missing required who header");
			goto failed;
		}

		err = bt_obex_get_header_conn_id(buf, &client->_conn_id);
		if (err != 0) {
			LOG_WRN("Invalid connection id header");
			goto failed;
		}

		err = bt_obex_get_header_who(buf, &who_len, &who_data);
		if (err != 0) {
			LOG_WRN("Invalid who header");
			goto failed;
		}

		err = bt_obex_make_uuid(&u, who_data, who_len);
		if (err != 0) {
			LOG_WRN("Unsupported who header len %u", who_len);
			goto failed;
		}

		if (bt_uuid_cmp(&u.uuid, &client->_target.uuid) != 0) {
			LOG_WRN("Who header is mismatched with target");
			goto failed;
		}
	}

	if (mopl < BT_OBEX_MIN_MTU) {
		LOG_WRN("Invalid MOPL (%d < %d)", mopl, BT_OBEX_MIN_MTU);
		goto failed;
	}

	if (mopl > client->obex->tx.mtu) {
		LOG_WRN("MOPL exceeds MTU (%d > %d)", mopl, client->obex->tx.mtu);
		goto failed;
	}

	client->tx.mopl = mopl;

	atomic_set(&client->_state,
		   rsp_code == BT_OBEX_RSP_CODE_SUCCESS ? BT_OBEX_CONNECTED : BT_OBEX_DISCONNECTED);

	if (atomic_get(&client->_state) == BT_OBEX_DISCONNECTED) {
		sys_slist_find_and_remove(&client->obex->_clients, &client->_node);
	}

	if (client->ops->connect) {
		client->ops->connect(client, rsp_code, version, mopl, buf);
	}
	return 0;

failed:
	LOG_WRN("Disconnect transport");

	return obex_transport_disconn(client->obex);
}

static int obex_client_disconn(struct bt_obex_client *client, uint8_t rsp_code, uint16_t len,
			       struct net_buf *buf)
{
	LOG_DBG("");

	atomic_clear(&client->_opcode);
	atomic_ptr_clear(&client->obex->_active_client);

	if (atomic_get(&client->_state) != BT_OBEX_DISCONNECTING) {
		return -EINVAL;
	}

	atomic_set(&client->_state,
		   rsp_code == BT_OBEX_RSP_CODE_SUCCESS ? BT_OBEX_DISCONNECTED : BT_OBEX_CONNECTED);

	if (atomic_get(&client->_state) == BT_OBEX_DISCONNECTED) {
		sys_slist_find_and_remove(&client->obex->_clients, &client->_node);
	}

	if (client->ops->disconnect) {
		client->ops->disconnect(client, rsp_code, buf);
	}
	return 0;
}

static int obex_client_put_common(struct bt_obex_client *client, uint8_t rsp_code, uint16_t len,
				  struct net_buf *buf)
{
	LOG_DBG("");

	if (len != buf->len) {
		LOG_WRN("Invalid packet size");
		return -EINVAL;
	}

	if (atomic_get(&client->_state) != BT_OBEX_CONNECTED) {
		LOG_WRN("Invalid state, connect is not established");
		return -EINVAL;
	}

	if (client->ops->put == NULL) {
		LOG_WRN("Put rsp handling not implemented");
		return -ENOTSUP;
	}

	if (rsp_code != BT_OBEX_RSP_CODE_CONTINUE) {
		atomic_clear(&client->_opcode);
		atomic_ptr_clear(&client->obex->_active_client);
	}

	client->ops->put(client, rsp_code, buf);
	return 0;
}

static int obex_client_put(struct bt_obex_client *client, uint8_t rsp_code, uint16_t len,
			   struct net_buf *buf)
{
	return obex_client_put_common(client, rsp_code, len, buf);
}

static int obex_client_put_final(struct bt_obex_client *client, uint8_t rsp_code, uint16_t len,
				 struct net_buf *buf)
{
	return obex_client_put_common(client, rsp_code, len, buf);
}

static int obex_client_get_common(struct bt_obex_client *client, uint8_t rsp_code, uint16_t len,
				  struct net_buf *buf)
{
	LOG_DBG("");

	if (len != buf->len) {
		LOG_WRN("Invalid packet size");
		return -EINVAL;
	}

	if (atomic_get(&client->_state) != BT_OBEX_CONNECTED) {
		LOG_WRN("Invalid state, connect is not established");
		return -EINVAL;
	}

	if (client->ops->get == NULL) {
		LOG_WRN("Get rsp handling not implemented");
		return -ENOTSUP;
	}

	if (rsp_code != BT_OBEX_RSP_CODE_CONTINUE) {
		atomic_clear(&client->_opcode);
		atomic_ptr_clear(&client->obex->_active_client);
	}

	client->ops->get(client, rsp_code, buf);
	return 0;
}

static int obex_client_get(struct bt_obex_client *client, uint8_t rsp_code, uint16_t len,
			   struct net_buf *buf)
{
	return obex_client_get_common(client, rsp_code, len, buf);
}

static int obex_client_get_final(struct bt_obex_client *client, uint8_t rsp_code, uint16_t len,
				 struct net_buf *buf)
{
	return obex_client_get_common(client, rsp_code, len, buf);
}

static int obex_client_setpath(struct bt_obex_client *client, uint8_t rsp_code, uint16_t len,
			       struct net_buf *buf)
{
	LOG_DBG("");

	if (len != buf->len) {
		LOG_WRN("Invalid packet size");
		return -EINVAL;
	}

	if (atomic_get(&client->_state) != BT_OBEX_CONNECTED) {
		LOG_WRN("Invalid state, connect is not established");
		return -EINVAL;
	}

	if (client->ops->setpath == NULL) {
		LOG_WRN("Setpath rsp handling not implemented");
		return -ENOTSUP;
	}

	if (rsp_code != BT_OBEX_RSP_CODE_CONTINUE) {
		atomic_clear(&client->_opcode);
		atomic_ptr_clear(&client->obex->_active_client);
	}

	client->ops->setpath(client, rsp_code, buf);
	return 0;
}

static int obex_client_action_common(struct bt_obex_client *client, uint8_t rsp_code, uint16_t len,
				     struct net_buf *buf)
{
	LOG_DBG("");

	if (len != buf->len) {
		LOG_WRN("Invalid packet size");
		return -EINVAL;
	}

	if (atomic_get(&client->_state) != BT_OBEX_CONNECTED) {
		LOG_WRN("Invalid state, connect is not established");
		return -EINVAL;
	}

	if (client->ops->action == NULL) {
		LOG_WRN("Setpath rsp handling not implemented");
		return -ENOTSUP;
	}

	if (rsp_code != BT_OBEX_RSP_CODE_CONTINUE) {
		atomic_clear(&client->_opcode);
		atomic_ptr_clear(&client->obex->_active_client);
	}

	client->ops->action(client, rsp_code, buf);
	return 0;
}

static int obex_client_action(struct bt_obex_client *client, uint8_t rsp_code, uint16_t len,
			      struct net_buf *buf)
{
	return obex_client_action_common(client, rsp_code, len, buf);
}

static int obex_client_action_final(struct bt_obex_client *client, uint8_t rsp_code, uint16_t len,
				    struct net_buf *buf)
{
	return obex_client_action_common(client, rsp_code, len, buf);
}

static int obex_client_session(struct bt_obex_client *client, uint8_t rsp_code, uint16_t len,
			       struct net_buf *buf)
{
	return -ENOTSUP;
}

static struct client_handler *obex_client_find_handler(uint8_t opcode);

static int obex_client_abort(struct bt_obex_client *client, uint8_t rsp_code, uint16_t len,
			     struct net_buf *buf)
{
	LOG_DBG("");

	if (len != buf->len) {
		LOG_WRN("Invalid packet size");
		return -EINVAL;
	}

	if (atomic_get(&client->_state) != BT_OBEX_CONNECTED) {
		LOG_WRN("Invalid state, connect is not established");
		return -EINVAL;
	}

	if (len != 0) {
		struct client_handler *handler;
		int err = -EINVAL;

		handler = obex_client_find_handler(atomic_get(&client->_pre_opcode));
		if (handler) {
			err = handler->handler(client, rsp_code, len, buf);
			if (err) {
				LOG_WRN("Handler err %d", err);
			}
		}
		return err;
	}

	if (client->ops->abort == NULL) {
		LOG_WRN("Abort rsp handling not implemented");
		return -ENOTSUP;
	}

	atomic_clear(&client->_opcode);
	atomic_ptr_clear(&client->obex->_active_client);

	if (rsp_code != BT_OBEX_RSP_CODE_SUCCESS) {
		LOG_WRN("Disconnect transport");

		return obex_transport_disconn(client->obex);
	}

	client->ops->abort(client, rsp_code, buf);
	return 0;
}

#define BT_OBEX_REQUEST_CODE_START 0x00
#define BT_OBEX_REQUEST_CODE_END   0x0f
#define BT_OBEX_REQUEST_CODE_F_BIT BIT(7)
#define IN_OBEX_REQUEST_RANGE(code)                                                                \
	((code) >= BT_OBEX_REQUEST_CODE_START && (code) <= BT_OBEX_REQUEST_CODE_END)
#define IS_OBEX_REQUEST(code) (IN_OBEX_REQUEST_RANGE((code) & ~(BT_OBEX_REQUEST_CODE_F_BIT)) ||    \
	((code) == BT_OBEX_OPCODE_ABORT))

struct client_handler client_handler[] = {
	{BT_OBEX_OPCODE_CONNECT, obex_client_connect},
	{BT_OBEX_OPCODE_DISCONN, obex_client_disconn},
	{BT_OBEX_OPCODE_PUT, obex_client_put},
	{BT_OBEX_OPCODE_PUT_F, obex_client_put_final},
	{BT_OBEX_OPCODE_GET, obex_client_get},
	{BT_OBEX_OPCODE_GET_F, obex_client_get_final},
	{BT_OBEX_OPCODE_SETPATH, obex_client_setpath},
	{BT_OBEX_OPCODE_ACTION, obex_client_action},
	{BT_OBEX_OPCODE_ACTION_F, obex_client_action_final},
	{BT_OBEX_OPCODE_SESSION, obex_client_session},
	{BT_OBEX_OPCODE_ABORT, obex_client_abort},
};

static struct client_handler *obex_client_find_handler(uint8_t opcode)
{
	ARRAY_FOR_EACH(client_handler, i) {
		if (client_handler[i].opcode == opcode) {
			return &client_handler[i];
		}
	}
	return NULL;
}

static int obex_client_recv(struct bt_obex *obex, struct net_buf *buf)
{
	struct client_handler *handler;
	struct bt_obex_rsp_hdr *hdr;
	struct bt_obex_client *client = atomic_ptr_get(&obex->_active_client);
	uint16_t len;
	int err;

	if (buf->len < sizeof(*hdr)) {
		LOG_WRN("Too small header size (%d < %d)", buf->len, sizeof(*hdr));
		return -EINVAL;
	}

	if (client == NULL) {
		LOG_WRN("No executing OBEX request");
		return -EINVAL;
	}

	hdr = net_buf_pull_mem(buf, sizeof(*hdr));
	len = sys_be16_to_cpu(hdr->len);
	handler = obex_client_find_handler(atomic_get(&client->_opcode));
	if (handler) {
		err = handler->handler(client, hdr->code, len - sizeof(*hdr), buf);
		if (err) {
			LOG_WRN("Handler err %d", err);
		}
		return err;
	}

	LOG_WRN("Unknown OBEX rsp (rsp_code %02x, len %d)", hdr->code, len);
	return -EINVAL;
}

int bt_obex_transport_connected(struct bt_obex *obex)
{
	if (obex == NULL) {
		return -EINVAL;
	}

	if (obex->rx.mtu < BT_OBEX_MIN_MTU) {
		LOG_WRN("Invalid MTU (%d < %d)", obex->rx.mtu, BT_OBEX_MIN_MTU);
		return -EINVAL;
	}

	if (obex->tx.mtu < BT_OBEX_MIN_MTU) {
		LOG_WRN("Invalid MTU (%d < %d)", obex->tx.mtu, BT_OBEX_MIN_MTU);
		return -EINVAL;
	}

	atomic_ptr_clear(&obex->_active_client);

	return 0;
}

int bt_obex_transport_disconnected(struct bt_obex *obex)
{
	struct bt_obex_client *client, *cnext;
	struct bt_obex_server *server, *snext;

	obex->_transport_ops = NULL;
	atomic_ptr_clear(&obex->_active_client);

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&obex->_clients, client, cnext, _node) {
		atomic_clear(&client->_opcode);
		atomic_clear(&client->_pre_opcode);
		atomic_set(&client->_state, BT_OBEX_DISCONNECTED);
	}

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&obex->_servers, server, snext, _node) {
		atomic_clear(&server->_opcode);
		atomic_set(&server->_state, BT_OBEX_DISCONNECTED);
	}

	sys_slist_init(&obex->_clients);
	return 0;
}

int bt_obex_reg_transport(struct bt_obex *obex, const struct bt_obex_transport_ops *ops)
{
	if (!obex || !ops) {
		LOG_WRN("Invalid parameter");
		return -EINVAL;
	}

	obex->_transport_ops = ops;
	return 0;
}

int bt_obex_recv(struct bt_obex *obex, struct net_buf *buf)
{
	struct bt_obex_comm_hdr *hdr;

	if (obex == NULL || buf == NULL) {
		LOG_WRN("Invalid parameter");
		return -EINVAL;
	}

	if (buf->len < sizeof(*hdr)) {
		LOG_WRN("Malformed packet");
		return -EINVAL;
	}

	hdr = (struct bt_obex_comm_hdr *)buf->data;
	if (!IS_OBEX_REQUEST(hdr->code)) {
		return obex_client_recv(obex, buf);
	}

	return obex_server_recv(obex, buf);
}

int bt_obex_server_register(struct bt_obex_server *server, const struct bt_uuid_128 *uuid)
{
	if (server == NULL || server->obex == NULL) {
		LOG_ERR("Invalid parameter");
		return -EINVAL;
	}

	if (server->ops == NULL || server->ops->connect == NULL ||
	    server->ops->disconnect == NULL) {
		LOG_ERR("Invalid OBEX role");
		return -EINVAL;
	}

	if (atomic_get(&server->_state) != BT_OBEX_DISCONNECTED) {
		LOG_ERR("Invalid state, connect is %u", (uint8_t)atomic_get(&server->_state));
		return -EINPROGRESS;
	}

	if (sys_slist_find(&server->obex->_servers, &server->_node, NULL)) {
		LOG_ERR("server %p has been registered", server);
		return -EALREADY;
	}

	if (!sys_slist_is_empty(&server->obex->_servers) && server->uuid == NULL) {
		LOG_ERR("UUID of server %p should not be NULL", server);
		return -EINVAL;
	}

	server->rx.mopl = BT_OBEX_MIN_MTU;
	/* Set MOPL of TX to MTU by default to avoid the OBEX connect rsp cannot be sent. */
	server->tx.mopl = BT_OBEX_MIN_MTU;

	server->uuid = uuid;

	sys_slist_append(&server->obex->_servers, &server->_node);

	return 0;
}

int bt_obex_server_unregister(struct bt_obex_server *server)
{
	if (server == NULL || server->obex == NULL) {
		LOG_ERR("Invalid parameter");
		return -EINVAL;
	}

	if (!sys_slist_find(&server->obex->_servers, &server->_node, NULL)) {
		LOG_ERR("server %p has not been registered", server);
		return -EALREADY;
	}

	if (atomic_get(&server->_state) != BT_OBEX_DISCONNECTED) {
		LOG_ERR("Invalid state, connect is %u", (uint8_t)atomic_get(&server->_state));
		return -EINPROGRESS;
	}

	sys_slist_find_and_remove(&server->obex->_servers, &server->_node);

	return 0;
}

int bt_obex_connect(struct bt_obex_client *client, uint16_t mopl, struct net_buf *buf)
{
	struct bt_obex_conn_req_hdr *req_hdr;
	struct bt_obex_req_hdr *hdr;
	int err;
	bool allocated = false;
	uint16_t target_len;
	const uint8_t *target_data;

	if (client == NULL || client->obex == NULL) {
		LOG_WRN("Invalid parameter");
		return -EINVAL;
	}

	if (client->ops == NULL || client->ops->connect == NULL ||
	    client->ops->disconnect == NULL) {
		LOG_WRN("Invalid OBEX role");
		return -EINVAL;
	}

	if (mopl > client->obex->rx.mtu) {
		mopl = client->obex->rx.mtu;
	}

	if (mopl < BT_OBEX_MIN_MTU) {
		LOG_WRN("Invalid MOPL");
		return -EINVAL;
	}

	if (atomic_get(&client->_state) != BT_OBEX_DISCONNECTED) {
		LOG_WRN("Invalid state, connect is %u", (uint8_t)atomic_get(&client->_state));
		return -EINPROGRESS;
	}

	if (!sys_slist_is_empty(&client->obex->_clients) &&
	    !bt_obex_has_header(buf, BT_OBEX_HEADER_ID_TARGET)) {
		LOG_ERR("The Header target should be added");
		return -EINVAL;
	}

	if (!atomic_ptr_cas(&client->obex->_active_client, NULL, client)) {
		LOG_WRN("One OBEX request is executing");
		return -EBUSY;
	}

	if (!atomic_cas(&client->_opcode, 0, BT_OBEX_OPCODE_CONNECT)) {
		LOG_WRN("Operation inprogress");
		return -EBUSY;
	}

	atomic_set_bit_to(&client->_flags, BT_OBEX_HAS_TARGET,
			  bt_obex_has_header(buf, BT_OBEX_HEADER_ID_TARGET));

	if (atomic_test_bit(&client->_flags, BT_OBEX_HAS_TARGET)) {
		err = bt_obex_get_header_target(buf, &target_len, &target_data);
		if (err != 0) {
			LOG_ERR("Invalid target header");
			return err;
		}

		err = bt_obex_make_uuid(&client->_target, target_data, target_len);
		if (err != 0) {
			LOG_WRN("Unsupported target header len %u", target_len);
			return err;
		}
	}

	if (buf == NULL) {
		buf = obex_alloc_buf(client->obex);
		if (!buf) {
			LOG_WRN("No buffers");
			return -ENOBUFS;
		}
		allocated = true;
	}

	client->rx.mopl = mopl;
	/* Set MOPL of TX to MTU by default to avoid the OBEX connect req cannot be sent. */
	client->tx.mopl = BT_OBEX_MIN_MTU;
	atomic_set(&client->_state, BT_OBEX_CONNECTING);

	if (!sys_slist_find(&client->obex->_clients, &client->_node, NULL)) {
		sys_slist_append(&client->obex->_clients, &client->_node);
	}

	req_hdr = net_buf_push(buf, sizeof(*req_hdr));
	req_hdr->flags = 0;
	req_hdr->mopl = sys_cpu_to_be16(mopl);
	req_hdr->version = BT_OBEX_VERSION;
	hdr = net_buf_push(buf, sizeof(*hdr));
	hdr->code = BT_OBEX_OPCODE_CONNECT;
	hdr->len = sys_cpu_to_be16(buf->len);

	err = obex_send(client->obex, client->tx.mopl, buf);
	if (err) {
		atomic_set(&client->_state, BT_OBEX_DISCONNECTED);
		atomic_clear(&client->_opcode);
		atomic_ptr_clear(&client->obex->_active_client);
		sys_slist_find_and_remove(&client->obex->_clients, &client->_node);

		if (allocated) {
			net_buf_unref(buf);
		}
	}
	return err;
}

int bt_obex_connect_rsp(struct bt_obex_server *server, uint8_t rsp_code, uint16_t mopl,
			struct net_buf *buf)
{
	struct bt_obex_conn_rsp_hdr *rsp_hdr;
	struct bt_obex_rsp_hdr *hdr;
	int err;
	atomic_val_t old_state;
	uint16_t who_len;
	const uint8_t *who_data;

	bool allocated = false;

	if (server == NULL || server->obex == NULL || (rsp_code == BT_OBEX_RSP_CODE_CONTINUE)) {
		LOG_WRN("Invalid parameter");
		return -EINVAL;
	}

	if (server->ops == NULL) {
		LOG_WRN("Invalid OBEX role");
		return -EINVAL;
	}

	if (mopl > server->obex->rx.mtu) {
		mopl = server->obex->rx.mtu;
	}

	if (mopl < BT_OBEX_MIN_MTU) {
		LOG_WRN("Invalid MOPL");
		return -EINVAL;
	}

	if (atomic_get(&server->_state) != BT_OBEX_CONNECTING) {
		LOG_WRN("Invalid state");
		return -ENOTCONN;
	}

	if (atomic_get(&server->_opcode) != BT_OBEX_OPCODE_CONNECT) {
		LOG_WRN("Invalid response");
		return -EINVAL;
	}

	if (rsp_code == BT_OBEX_RSP_CODE_SUCCESS) {
		if (bt_obex_has_header(buf, BT_OBEX_HEADER_ID_WHO) !=
		    bt_obex_has_header(buf, BT_OBEX_HEADER_ID_CONN_ID)) {
			LOG_ERR("Missing required who or connect id headers");
			return -EINVAL;
		}

		if (atomic_test_bit(&server->_flags, BT_OBEX_HAS_TARGET) !=
		    bt_obex_has_header(buf, BT_OBEX_HEADER_ID_WHO)) {
			LOG_ERR("Missing required who header");
			return -EINVAL;
		}

		if (atomic_test_bit(&server->_flags, BT_OBEX_HAS_TARGET)) {
			union bt_obex_uuid u;

			err = bt_obex_get_header_conn_id(buf, &server->_conn_id);
			if (err != 0) {
				LOG_ERR("Invalid connection id header");
				return err;
			}

			err = bt_obex_get_header_who(buf, &who_len, &who_data);
			if (err != 0) {
				LOG_ERR("Invalid who header");
				return err;
			}

			err = bt_obex_make_uuid(&u, who_data, who_len);
			if (err != 0) {
				LOG_WRN("Unsupported who header len %u", who_len);
				return err;
			}

			if (bt_uuid_cmp(&u.uuid, &server->_target.uuid) != 0) {
				LOG_WRN("Who header is mismatched with target");
				return -EINVAL;
			}
		}
	}

	if (!buf) {
		buf = obex_alloc_buf(server->obex);
		if (!buf) {
			LOG_WRN("No buffers");
			return -ENOBUFS;
		}
		allocated = true;
	}

	server->rx.mopl = mopl;
	if (rsp_code == BT_OBEX_RSP_CODE_SUCCESS) {
		old_state = atomic_set(&server->_state, BT_OBEX_CONNECTED);
	} else {
		old_state = atomic_set(&server->_state, BT_OBEX_DISCONNECTED);
	}

	rsp_hdr = net_buf_push(buf, sizeof(*rsp_hdr));
	rsp_hdr->flags = 0;
	rsp_hdr->mopl = sys_cpu_to_be16(mopl);
	rsp_hdr->version = BT_OBEX_VERSION;
	hdr = net_buf_push(buf, sizeof(*hdr));
	hdr->code = rsp_code;
	hdr->len = sys_cpu_to_be16(buf->len);

	err = obex_send(server->obex, server->tx.mopl, buf);
	if (err) {
		atomic_set(&server->_state, old_state);

		if (allocated) {
			net_buf_unref(buf);
		}
	} else {
		atomic_clear(&server->_opcode);
		atomic_ptr_clear(&server->obex->_active_server);
	}
	return err;
}

int bt_obex_disconnect(struct bt_obex_client *client, struct net_buf *buf)
{
	struct bt_obex_req_hdr *hdr;
	int err;
	bool allocated = false;

	if (client == NULL || client->obex == NULL) {
		LOG_WRN("Invalid parameter");
		return -EINVAL;
	}

	if (client->ops == NULL) {
		LOG_WRN("Invalid OBEX role");
		return -EINVAL;
	}

	if (atomic_get(&client->_state) != BT_OBEX_CONNECTED) {
		LOG_WRN("Invalid state, connect is not established");
		return -ENOTCONN;
	}

	if (!atomic_ptr_cas(&client->obex->_active_client, NULL, client)) {
		LOG_WRN("One OBEX request is executing");
		return -EBUSY;
	}

	if (!atomic_cas(&client->_opcode, 0, BT_OBEX_OPCODE_DISCONN)) {
		LOG_WRN("Operation inprogress");
		return -EBUSY;
	}

	if (!buf) {
		buf = obex_alloc_buf(client->obex);
		if (!buf) {
			LOG_WRN("No buffers");
			return -ENOBUFS;
		}
		allocated = true;
	}

	hdr = net_buf_push(buf, sizeof(*hdr));
	hdr->code = BT_OBEX_OPCODE_DISCONN;
	hdr->len = sys_cpu_to_be16(buf->len);

	err = obex_send(client->obex, client->tx.mopl, buf);
	if (err) {
		atomic_clear(&client->_opcode);
		atomic_ptr_clear(&client->obex->_active_client);

		if (allocated) {
			net_buf_unref(buf);
		}
	} else {
		atomic_set(&client->_state, BT_OBEX_DISCONNECTING);
	}
	return err;
}

int bt_obex_disconnect_rsp(struct bt_obex_server *server, uint8_t rsp_code, struct net_buf *buf)
{
	struct bt_obex_rsp_hdr *hdr;
	int err;
	atomic_val_t old_state;
	bool allocated = false;

	if (server == NULL || server->obex == NULL || (rsp_code == BT_OBEX_RSP_CODE_CONTINUE)) {
		LOG_WRN("Invalid parameter");
		return -EINVAL;
	}

	if (server->ops == NULL) {
		LOG_WRN("Invalid OBEX role");
		return -EINVAL;
	}

	if (atomic_get(&server->_state) != BT_OBEX_DISCONNECTING) {
		LOG_WRN("Invalid state");
		return -EINVAL;
	}

	if (atomic_get(&server->_opcode) != BT_OBEX_OPCODE_DISCONN) {
		LOG_WRN("Invalid response");
		return -EINVAL;
	}

	if (!buf) {
		buf = obex_alloc_buf(server->obex);
		if (!buf) {
			LOG_WRN("No buffers");
			return -ENOBUFS;
		}
		allocated = true;
	}

	if (rsp_code == BT_OBEX_RSP_CODE_SUCCESS) {
		old_state = atomic_set(&server->_state, BT_OBEX_DISCONNECTED);
	} else {
		old_state = atomic_set(&server->_state, BT_OBEX_CONNECTED);
	}

	hdr = net_buf_push(buf, sizeof(*hdr));
	hdr->code = rsp_code;
	hdr->len = sys_cpu_to_be16(buf->len);

	err = obex_send(server->obex, server->tx.mopl, buf);
	if (err) {
		atomic_set(&server->_state, old_state);

		if (allocated) {
			net_buf_unref(buf);
		}
	} else {
		atomic_clear(&server->_opcode);
		atomic_ptr_clear(&server->obex->_active_server);
	}
	return err;
}

int bt_obex_put(struct bt_obex_client *client, bool final, struct net_buf *buf)
{
	struct bt_obex_req_hdr *hdr;
	struct bt_obex_client *active_client;
	int err;
	uint8_t req_code;
	uint8_t opcode;
	bool allocated = false;

	if (client == NULL || client->obex == NULL) {
		LOG_WRN("Invalid parameter");
		return -EINVAL;
	}

	if (client->ops == NULL) {
		LOG_WRN("Invalid OBEX role");
		return -EINVAL;
	}

	if (atomic_get(&client->_state) != BT_OBEX_CONNECTED) {
		LOG_WRN("Invalid state, connect is not established");
		return -ENOTCONN;
	}

	active_client = atomic_ptr_get(&client->obex->_active_client);
	if (!atomic_ptr_cas(&client->obex->_active_client, NULL, client) &&
	    (active_client != client)) {
		LOG_WRN("One OBEX request is executing");
		return -EBUSY;
	}

	if (!buf) {
		buf = obex_alloc_buf(client->obex);
		if (!buf) {
			LOG_WRN("No buffers");
			return -ENOBUFS;
		}
		allocated = true;
	}

	opcode = atomic_get(&client->_opcode);

	req_code = final ? BT_OBEX_OPCODE_PUT_F : BT_OBEX_OPCODE_PUT;
	if (!atomic_cas(&client->_opcode, 0, req_code)) {
		if ((opcode != BT_OBEX_OPCODE_PUT_F) && (opcode != BT_OBEX_OPCODE_PUT)) {
			LOG_WRN("Operation inprogress");
			return -EBUSY;
		}

		if (!final && (opcode == BT_OBEX_OPCODE_PUT_F)) {
			LOG_WRN("Unexpected put request without final bit");
			return -EBUSY;
		}

		if (opcode != req_code) {
			atomic_cas(&client->_opcode, opcode, req_code);
		}
	}

	hdr = net_buf_push(buf, sizeof(*hdr));
	hdr->code = req_code;
	hdr->len = sys_cpu_to_be16(buf->len);

	err = obex_send(client->obex, client->tx.mopl, buf);
	if (err) {
		atomic_set(&client->_opcode, opcode);
		atomic_ptr_set(&client->obex->_active_client, active_client);

		if (allocated) {
			net_buf_unref(buf);
		}
	}
	return err;
}

int bt_obex_put_rsp(struct bt_obex_server *server, uint8_t rsp_code, struct net_buf *buf)
{
	struct bt_obex_rsp_hdr *hdr;
	int err;
	uint8_t opcode;
	bool allocated = false;

	if (server == NULL || server->obex == NULL) {
		LOG_WRN("Invalid parameter");
		return -EINVAL;
	}

	if (server->ops == NULL) {
		LOG_WRN("Invalid OBEX role");
		return -EINVAL;
	}

	if (atomic_get(&server->_state) != BT_OBEX_CONNECTED) {
		LOG_WRN("Invalid state, connect is not established");
		return -EINVAL;
	}

	if (!buf) {
		buf = obex_alloc_buf(server->obex);
		if (!buf) {
			LOG_WRN("No buffers");
			return -ENOBUFS;
		}
		allocated = true;
	}

	opcode = atomic_get(&server->_opcode);
	if ((opcode != BT_OBEX_OPCODE_PUT_F) && (opcode != BT_OBEX_OPCODE_PUT)) {
		LOG_WRN("Invalid response");
		return -EINVAL;
	}

	if ((opcode != BT_OBEX_OPCODE_PUT_F) && (rsp_code == BT_OBEX_RSP_CODE_SUCCESS)) {
		LOG_WRN("Success cannot be responded without final");
		return -EINVAL;
	}

	hdr = net_buf_push(buf, sizeof(*hdr));
	hdr->code = rsp_code;
	hdr->len = sys_cpu_to_be16(buf->len);

	err = obex_send(server->obex, server->tx.mopl, buf);
	if (!err) {
		if (rsp_code != BT_OBEX_RSP_CODE_CONTINUE) {
			atomic_clear(&server->_opcode);
			atomic_ptr_clear(&server->obex->_active_server);
		}
	} else {
		if (allocated) {
			net_buf_unref(buf);
		}
	}
	return err;
}

int bt_obex_get(struct bt_obex_client *client, bool final, struct net_buf *buf)
{
	struct bt_obex_req_hdr *hdr;
	struct bt_obex_client *active_client;
	int err;
	uint8_t req_code;
	uint8_t opcode;
	bool allocated = false;

	if (client == NULL || client->obex == NULL) {
		LOG_WRN("Invalid parameter");
		return -EINVAL;
	}

	if (client->ops == NULL) {
		LOG_WRN("Invalid OBEX role");
		return -EINVAL;
	}

	if (atomic_get(&client->_state) != BT_OBEX_CONNECTED) {
		LOG_WRN("Invalid state, connect is not established");
		return -ENOTCONN;
	}

	active_client = atomic_ptr_get(&client->obex->_active_client);
	if (!atomic_ptr_cas(&client->obex->_active_client, NULL, client) &&
	    (active_client != client)) {
		LOG_WRN("One OBEX request is executing");
		return -EBUSY;
	}

	if (!buf) {
		buf = obex_alloc_buf(client->obex);
		if (!buf) {
			LOG_WRN("No buffers");
			return -ENOBUFS;
		}
		allocated = true;
	}

	opcode = atomic_get(&client->_opcode);

	req_code = final ? BT_OBEX_OPCODE_GET_F : BT_OBEX_OPCODE_GET;
	if (!atomic_cas(&client->_opcode, 0, req_code)) {
		if ((opcode != BT_OBEX_OPCODE_GET_F) && (opcode != BT_OBEX_OPCODE_GET)) {
			LOG_WRN("Operation inprogress");
			return -EBUSY;
		}

		if (!final && (opcode == BT_OBEX_OPCODE_GET_F)) {
			LOG_WRN("Unexpected get request without final bit");
			return -EBUSY;
		}

		if (opcode != req_code) {
			atomic_cas(&client->_opcode, opcode, req_code);
		}
	}

	hdr = net_buf_push(buf, sizeof(*hdr));
	hdr->code = req_code;
	hdr->len = sys_cpu_to_be16(buf->len);

	err = obex_send(client->obex, client->tx.mopl, buf);
	if (err) {
		atomic_set(&client->_opcode, opcode);
		atomic_ptr_set(&client->obex->_active_client, active_client);

		if (allocated) {
			net_buf_unref(buf);
		}
	}
	return err;
}

int bt_obex_get_rsp(struct bt_obex_server *server, uint8_t rsp_code, struct net_buf *buf)
{
	struct bt_obex_rsp_hdr *hdr;
	int err;
	uint8_t opcode;
	bool allocated = false;

	if (server == NULL || server->obex == NULL) {
		LOG_WRN("Invalid parameter");
		return -EINVAL;
	}

	if (server->ops == NULL) {
		LOG_WRN("Invalid OBEX role");
		return -EINVAL;
	}

	if (atomic_get(&server->_state) != BT_OBEX_CONNECTED) {
		LOG_WRN("Invalid state, connect is not established");
		return -EINVAL;
	}

	if (!buf) {
		buf = obex_alloc_buf(server->obex);
		if (!buf) {
			LOG_WRN("No buffers");
			return -ENOBUFS;
		}
		allocated = true;
	}

	opcode = atomic_get(&server->_opcode);
	if ((opcode != BT_OBEX_OPCODE_GET_F) && (opcode != BT_OBEX_OPCODE_GET)) {
		LOG_WRN("Invalid response");
		return -EINVAL;
	}

	if ((opcode != BT_OBEX_OPCODE_GET_F) && (rsp_code == BT_OBEX_RSP_CODE_SUCCESS)) {
		LOG_WRN("Success cannot be responded without final");
		return -EINVAL;
	}

	hdr = net_buf_push(buf, sizeof(*hdr));
	hdr->code = rsp_code;
	hdr->len = sys_cpu_to_be16(buf->len);

	err = obex_send(server->obex, server->tx.mopl, buf);
	if (!err) {
		if (rsp_code != BT_OBEX_RSP_CODE_CONTINUE) {
			atomic_clear(&server->_opcode);
			atomic_ptr_clear(&server->obex->_active_server);
		}
	} else {
		if (allocated) {
			net_buf_unref(buf);
		}
	}
	return err;
}

static const uint8_t abort_opcode[] = {
	BT_OBEX_OPCODE_PUT,
	BT_OBEX_OPCODE_PUT_F,
	BT_OBEX_OPCODE_GET,
	BT_OBEX_OPCODE_GET_F,
	BT_OBEX_OPCODE_ACTION,
	BT_OBEX_OPCODE_ACTION_F
};

static bool obex_op_support_abort(uint8_t opcode)
{
	for (size_t index = 0; index < ARRAY_SIZE(abort_opcode); index++) {
		if (opcode == abort_opcode[index]) {
			return true;
		}
	}
	return false;
}

int bt_obex_abort(struct bt_obex_client *client, struct net_buf *buf)
{
	struct bt_obex_req_hdr *hdr;
	struct bt_obex_client *active_client;
	int err;
	uint8_t opcode;
	bool allocated = false;

	if (client == NULL || client->obex == NULL) {
		LOG_WRN("Invalid parameter");
		return -EINVAL;
	}

	if (client->ops == NULL) {
		LOG_WRN("Invalid OBEX role");
		return -EINVAL;
	}

	if (atomic_get(&client->_state) != BT_OBEX_CONNECTED) {
		LOG_WRN("Invalid state, connect is not established");
		return -ENOTCONN;
	}

	active_client = atomic_ptr_get(&client->obex->_active_client);
	if (active_client != client) {
		LOG_WRN("One OBEX request is executing");
		return -EBUSY;
	}

	if (!atomic_get(&client->_opcode)) {
		LOG_WRN("No operation is inprogress");
		return -EINVAL;
	} else if (atomic_get(&client->_opcode) == BT_OBEX_OPCODE_ABORT) {
		LOG_WRN("Abort is inprogress");
		return -EINPROGRESS;
	} else if (!obex_op_support_abort((uint8_t)atomic_get(&client->_opcode))) {
		LOG_WRN("Opcode %02x cannot be aborted", (uint8_t)atomic_get(&client->_opcode));
		return -ENOTSUP;
	}

	if (!buf) {
		buf = obex_alloc_buf(client->obex);
		if (!buf) {
			LOG_WRN("No buffers");
			return -ENOBUFS;
		}
		allocated = true;
	}

	opcode = atomic_set(&client->_opcode, BT_OBEX_OPCODE_ABORT);
	atomic_clear(&client->_pre_opcode);

	hdr = net_buf_push(buf, sizeof(*hdr));
	hdr->code = BT_OBEX_OPCODE_ABORT;
	hdr->len = sys_cpu_to_be16(buf->len);

	err = obex_send(client->obex, client->tx.mopl, buf);
	if (err) {
		atomic_set(&client->_opcode, opcode);
		atomic_ptr_set(&client->obex->_active_client, active_client);

		if (allocated) {
			net_buf_unref(buf);
		}
	} else {
		atomic_set(&client->_pre_opcode, opcode);
	}
	return err;
}

int bt_obex_abort_rsp(struct bt_obex_server *server, uint8_t rsp_code, struct net_buf *buf)
{
	struct bt_obex_rsp_hdr *hdr;
	int err;
	bool allocated = false;

	if (server == NULL || server->obex == NULL) {
		LOG_WRN("Invalid parameter");
		return -EINVAL;
	}

	if (server->ops == NULL) {
		LOG_WRN("Invalid OBEX role");
		return -EINVAL;
	}

	if (atomic_get(&server->_state) != BT_OBEX_CONNECTED) {
		LOG_WRN("Invalid state, connect is not established");
		return -EINVAL;
	}

	if (atomic_get(&server->_opcode) != BT_OBEX_OPCODE_ABORT) {
		LOG_WRN("Invalid response");
		return -EINVAL;
	}

	if (!buf) {
		buf = obex_alloc_buf(server->obex);
		if (!buf) {
			LOG_WRN("No buffers");
			return -ENOBUFS;
		}
		allocated = true;
	}

	hdr = net_buf_push(buf, sizeof(*hdr));
	hdr->code = rsp_code;
	hdr->len = sys_cpu_to_be16(buf->len);

	err = obex_send(server->obex, server->tx.mopl, buf);
	if (!err) {
		atomic_clear(&server->_opcode);
		atomic_ptr_clear(&server->obex->_active_server);
	} else {
		if (allocated) {
			net_buf_unref(buf);
		}
	}
	return err;
}

int bt_obex_setpath(struct bt_obex_client *client, uint8_t flags, struct net_buf *buf)
{
	struct bt_obex_req_hdr *hdr;
	struct bt_obex_setpath_req_hdr *req_hdr;
	int err;
	bool allocated = false;

	if (client == NULL || client->obex == NULL) {
		LOG_WRN("Invalid parameter");
		return -EINVAL;
	}

	if (client->ops == NULL) {
		LOG_WRN("Invalid OBEX role");
		return -EINVAL;
	}

	if (atomic_get(&client->_state) != BT_OBEX_CONNECTED) {
		LOG_WRN("Invalid state, connect is not established");
		return -ENOTCONN;
	}

	if (!atomic_ptr_cas(&client->obex->_active_client, NULL, client)) {
		LOG_WRN("One OBEX request is executing");
		return -EBUSY;
	}

	if (!atomic_cas(&client->_opcode, 0, BT_OBEX_OPCODE_SETPATH)) {
		LOG_WRN("Operation inprogress");
		return -EINPROGRESS;
	}

	if (!buf) {
		buf = obex_alloc_buf(client->obex);
		if (!buf) {
			LOG_WRN("No buffers");
			return -ENOBUFS;
		}
		allocated = true;
	}

	req_hdr = net_buf_push(buf, sizeof(*req_hdr));
	req_hdr->flags = flags;
	req_hdr->constants = 0;
	hdr = net_buf_push(buf, sizeof(*hdr));
	hdr->code = BT_OBEX_OPCODE_SETPATH;
	hdr->len = sys_cpu_to_be16(buf->len);

	err = obex_send(client->obex, client->tx.mopl, buf);
	if (err) {
		atomic_clear(&client->_opcode);
		atomic_ptr_clear(&client->obex->_active_client);

		if (allocated) {
			net_buf_unref(buf);
		}
	}
	return err;
}

int bt_obex_setpath_rsp(struct bt_obex_server *server, uint8_t rsp_code, struct net_buf *buf)
{
	struct bt_obex_rsp_hdr *hdr;
	int err;
	bool allocated = false;

	if (server == NULL || server->obex == NULL) {
		LOG_WRN("Invalid parameter");
		return -EINVAL;
	}

	if (server->ops == NULL) {
		LOG_WRN("Invalid OBEX role");
		return -EINVAL;
	}

	if (atomic_get(&server->_state) != BT_OBEX_CONNECTED) {
		LOG_WRN("Invalid state, connect is not established");
		return -EINVAL;
	}

	if (atomic_get(&server->_opcode) != BT_OBEX_OPCODE_SETPATH) {
		LOG_WRN("Invalid response");
		return -EINVAL;
	}

	if (!buf) {
		buf = obex_alloc_buf(server->obex);
		if (!buf) {
			LOG_WRN("No buffers");
			return -ENOBUFS;
		}
		allocated = true;
	}

	hdr = net_buf_push(buf, sizeof(*hdr));
	hdr->code = rsp_code;
	hdr->len = sys_cpu_to_be16(buf->len);

	err = obex_send(server->obex, server->tx.mopl, buf);
	if (!err) {
		atomic_clear(&server->_opcode);
		atomic_ptr_clear(&server->obex->_active_server);
	} else {
		if (allocated) {
			net_buf_unref(buf);
		}
	}
	return err;
}

int bt_obex_action(struct bt_obex_client *client, bool final, struct net_buf *buf)
{
	struct bt_obex_req_hdr *hdr;
	struct bt_obex_client *active_client;
	int err;
	uint8_t req_code;
	uint8_t opcode;
	bool allocated = false;

	if (client == NULL || client->obex == NULL) {
		LOG_WRN("Invalid parameter");
		return -EINVAL;
	}

	if (client->ops == NULL) {
		LOG_WRN("Invalid OBEX role");
		return -EINVAL;
	}

	if (atomic_get(&client->_state) != BT_OBEX_CONNECTED) {
		LOG_WRN("Invalid state, connect is not established");
		return -ENOTCONN;
	}

	active_client = atomic_ptr_get(&client->obex->_active_client);
	if (!atomic_ptr_cas(&client->obex->_active_client, NULL, client) &&
	    (active_client != client)) {
		LOG_WRN("One OBEX request is executing");
		return -EBUSY;
	}

	if (!buf) {
		buf = obex_alloc_buf(client->obex);
		if (!buf) {
			LOG_WRN("No buffers");
			return -ENOBUFS;
		}
		allocated = true;
	}

	opcode = atomic_get(&client->_opcode);

	req_code = final ? BT_OBEX_OPCODE_ACTION_F : BT_OBEX_OPCODE_ACTION;
	if (!atomic_cas(&client->_opcode, 0, req_code)) {
		if ((opcode != BT_OBEX_OPCODE_ACTION_F) && (opcode != BT_OBEX_OPCODE_ACTION)) {
			LOG_WRN("Operation inprogress");
			return -EBUSY;
		}

		if (!final && (opcode == BT_OBEX_OPCODE_ACTION_F)) {
			LOG_WRN("Unexpected get request without final bit");
			return -EBUSY;
		}

		if (opcode != req_code) {
			atomic_cas(&client->_opcode, opcode, req_code);
		}
	}

	hdr = net_buf_push(buf, sizeof(*hdr));
	hdr->code = req_code;
	hdr->len = sys_cpu_to_be16(buf->len);

	err = obex_send(client->obex, client->tx.mopl, buf);
	if (err) {
		atomic_set(&client->_opcode, opcode);
		atomic_ptr_set(&client->obex->_active_client, active_client);

		if (allocated) {
			net_buf_unref(buf);
		}
	}
	return err;
}

int bt_obex_action_rsp(struct bt_obex_server *server, uint8_t rsp_code, struct net_buf *buf)
{
	struct bt_obex_rsp_hdr *hdr;
	int err;
	uint8_t opcode;
	bool allocated = false;

	if (server == NULL || server->obex == NULL) {
		LOG_WRN("Invalid parameter");
		return -EINVAL;
	}

	if (server->ops == NULL) {
		LOG_WRN("Invalid OBEX role");
		return -EINVAL;
	}

	if (atomic_get(&server->_state) != BT_OBEX_CONNECTED) {
		LOG_WRN("Invalid state, connect is not established");
		return -EINVAL;
	}

	if (!buf) {
		buf = obex_alloc_buf(server->obex);
		if (!buf) {
			LOG_WRN("No buffers");
			return -ENOBUFS;
		}
		allocated = true;
	}

	opcode = atomic_get(&server->_opcode);
	if ((opcode != BT_OBEX_OPCODE_ACTION_F) && (opcode != BT_OBEX_OPCODE_ACTION)) {
		LOG_WRN("Invalid response");
		return -EINVAL;
	}

	if ((opcode != BT_OBEX_OPCODE_ACTION_F) && (rsp_code == BT_OBEX_RSP_CODE_SUCCESS)) {
		LOG_WRN("Success cannot be responded without final");
		return -EINVAL;
	}

	hdr = net_buf_push(buf, sizeof(*hdr));
	hdr->code = rsp_code;
	hdr->len = sys_cpu_to_be16(buf->len);

	err = obex_send(server->obex, server->tx.mopl, buf);
	if (!err) {
		if (rsp_code != BT_OBEX_RSP_CODE_CONTINUE) {
			atomic_clear(&server->_opcode);
			atomic_ptr_clear(&server->obex->_active_server);
		}
	} else {
		if (allocated) {
			net_buf_unref(buf);
		}
	}
	return err;
}

#define BT_OBEX_HEADER_ENCODING(header_id) (0xc0 & (header_id))
#define BT_OBEX_HEADER_ENCODING_UNICODE    0x00
#define BT_OBEX_HEADER_ENCODING_BYTE_SEQ   0x40
#define BT_OBEX_HEADER_ENCODING_1_BYTE     0x80
#define BT_OBEX_HEADER_ENCODING_4_BYTES    0xc0

#define BT_OBEX_UFT16_LEAD_SURROGATE_START  0xd800
#define BT_OBEX_UFT16_LEAD_SURROGATE_END    0xdbff
#define BT_OBEX_UFT16_TRAIL_SURROGATE_START 0xdc00
#define BT_OBEX_UFT16_TRAIL_SURROGATE_END   0xdfff

#define BT_OBEX_UFT16_NULL_TERMINATED 0x0000

#define BT_OBEX_UFT16_CODE_OCTETS 0x02

static bool bt_obex_unicode_is_valid(uint16_t len, const uint8_t *str)
{
	uint16_t index = 0;
	uint16_t code;

	if ((len < BT_OBEX_UFT16_CODE_OCTETS) || (len % BT_OBEX_UFT16_CODE_OCTETS)) {
		LOG_WRN("Invalid string length %d", len);
		return false;
	}

	code = sys_get_be16(&str[len - BT_OBEX_UFT16_CODE_OCTETS]);
	if (code != BT_OBEX_UFT16_NULL_TERMINATED) {
		LOG_WRN("Invalid terminated unicode %04x", code);
		return false;
	}

	while ((index + 1) < len) {
		code = sys_get_be16(&str[index]);
		if ((code >= BT_OBEX_UFT16_LEAD_SURROGATE_START) &&
		    (code <= BT_OBEX_UFT16_LEAD_SURROGATE_END)) {
			/* Find the trail surrogate */
			index += sizeof(code);
			if ((index + 1) >= len) {
				LOG_WRN("Invalid length, trail surrogate missing");
				return false;
			}

			code = sys_get_be16(&str[index]);
			if ((code < BT_OBEX_UFT16_TRAIL_SURROGATE_START) ||
			    (code > BT_OBEX_UFT16_TRAIL_SURROGATE_END)) {
				LOG_WRN("Invalid trail surrogate %04x at %d", code, index);
				return false;
			}
		} else if ((code >= BT_OBEX_UFT16_TRAIL_SURROGATE_START) &&
			   (code <= BT_OBEX_UFT16_TRAIL_SURROGATE_END)) {
			LOG_WRN("Abnormal trail surrogate %04x at %d", code, index);
			return false;
		}
		index += sizeof(code);
	}

	return true;
}

bool bt_obex_string_is_valid(uint8_t id, uint16_t len, const uint8_t *str)
{
	if (BT_OBEX_HEADER_ENCODING(id) == BT_OBEX_HEADER_ENCODING_UNICODE) {
		return bt_obex_unicode_is_valid(len, str);
	}

	return true;
}

int bt_obex_add_header_count(struct net_buf *buf, uint32_t count)
{
	size_t total;

	if (!buf) {
		LOG_WRN("Invalid buf");
		return -EINVAL;
	}

	total = sizeof(uint8_t) + sizeof(count);
	if (net_buf_tailroom(buf) < total) {
		return -ENOMEM;
	}

	net_buf_add_u8(buf, BT_OBEX_HEADER_ID_COUNT);
	net_buf_add_be32(buf, count);
	return 0;
}

int bt_obex_add_header_name(struct net_buf *buf, uint16_t len, const uint8_t *name)
{
	size_t total;

	/*
	 * OBEX Version 1.5, section 2.2.2 Name
	 * The `name` could be a NULL, so the `len` of the name could 0.
	 * In some cases an empty Name header is used to specify a particular behavior;
	 * see the GET and SETPATH Operations. An empty Name header is defined as a Name
	 * header of length 3 (one byte opcode + two byte length).
	 */
	if (!buf || (len && !name)) {
		LOG_WRN("Invalid parameter");
		return -EINVAL;
	}

	total = sizeof(uint8_t) + sizeof(len) + len;
	if (net_buf_tailroom(buf) < total) {
		return -ENOMEM;
	}

	if (len && !bt_obex_string_is_valid(BT_OBEX_HEADER_ID_NAME, len, name)) {
		LOG_WRN("Invalid string");
		return -EINVAL;
	}

	net_buf_add_u8(buf, BT_OBEX_HEADER_ID_NAME);
	net_buf_add_be16(buf, (uint16_t)total);
	if (len) {
		net_buf_add_mem(buf, name, len);
	}
	return 0;
}

int bt_obex_add_header_type(struct net_buf *buf, uint16_t len, const uint8_t *type)
{
	size_t total;

	if (!buf || !type || !len) {
		LOG_WRN("Invalid parameter");
		return -EINVAL;
	}

	total = sizeof(uint8_t) + sizeof(len) + len;
	if (net_buf_tailroom(buf) < total) {
		return -ENOMEM;
	}

	if (!bt_obex_string_is_valid(BT_OBEX_HEADER_ID_TYPE, len, type)) {
		LOG_WRN("Invalid string");
		return -EINVAL;
	}

	net_buf_add_u8(buf, BT_OBEX_HEADER_ID_TYPE);
	net_buf_add_be16(buf, (uint16_t)total);
	net_buf_add_mem(buf, type, len);
	return 0;
}

int bt_obex_add_header_len(struct net_buf *buf, uint32_t len)
{
	size_t total;

	if (!buf) {
		LOG_WRN("Invalid buf");
		return -EINVAL;
	}

	total = sizeof(uint8_t) + sizeof(len);
	if (net_buf_tailroom(buf) < total) {
		return -ENOMEM;
	}

	net_buf_add_u8(buf, BT_OBEX_HEADER_ID_LEN);
	net_buf_add_be32(buf, len);
	return 0;
}

int bt_obex_add_header_time_iso_8601(struct net_buf *buf, uint16_t len, const uint8_t *t)
{
	size_t total;

	if (!buf || !t || !len) {
		LOG_WRN("Invalid parameter");
		return -EINVAL;
	}

	total = sizeof(uint8_t) + sizeof(len) + len;
	if (net_buf_tailroom(buf) < total) {
		return -ENOMEM;
	}

	if (!bt_obex_string_is_valid(BT_OBEX_HEADER_ID_TIME_ISO_8601, len, t)) {
		LOG_WRN("Invalid string");
		return -EINVAL;
	}

	net_buf_add_u8(buf, BT_OBEX_HEADER_ID_TIME_ISO_8601);
	net_buf_add_be16(buf, (uint16_t)total);
	net_buf_add_mem(buf, t, len);
	return 0;
}

int bt_obex_add_header_time(struct net_buf *buf, uint32_t t)
{
	size_t total;

	if (!buf) {
		LOG_WRN("Invalid buf");
		return -EINVAL;
	}

	total = sizeof(uint8_t) + sizeof(t);
	if (net_buf_tailroom(buf) < total) {
		return -ENOMEM;
	}

	net_buf_add_u8(buf, BT_OBEX_HEADER_ID_TIME);
	net_buf_add_be32(buf, t);
	return 0;
}

int bt_obex_add_header_description(struct net_buf *buf, uint16_t len, const uint8_t *dec)
{
	size_t total;

	if (!buf || !dec || !len) {
		LOG_WRN("Invalid parameter");
		return -EINVAL;
	}

	total = sizeof(uint8_t) + sizeof(len) + len;
	if (net_buf_tailroom(buf) < total) {
		return -ENOMEM;
	}

	if (!bt_obex_string_is_valid(BT_OBEX_HEADER_ID_DES, len, dec)) {
		LOG_WRN("Invalid string");
		return -EINVAL;
	}

	net_buf_add_u8(buf, BT_OBEX_HEADER_ID_DES);
	net_buf_add_be16(buf, (uint16_t)total);
	net_buf_add_mem(buf, dec, len);
	return 0;
}

int bt_obex_add_header_target(struct net_buf *buf, uint16_t len, const uint8_t *target)
{
	size_t total;

	if (!buf || !target || !len) {
		LOG_WRN("Invalid parameter");
		return -EINVAL;
	}

	total = sizeof(uint8_t) + sizeof(len) + len;
	if (net_buf_tailroom(buf) < total) {
		return -ENOMEM;
	}

	if (!bt_obex_string_is_valid(BT_OBEX_HEADER_ID_TARGET, len, target)) {
		LOG_WRN("Invalid string");
		return -EINVAL;
	}

	net_buf_add_u8(buf, BT_OBEX_HEADER_ID_TARGET);
	net_buf_add_be16(buf, (uint16_t)total);
	net_buf_add_mem(buf, target, len);
	return 0;
}

int bt_obex_add_header_http(struct net_buf *buf, uint16_t len, const uint8_t *http)
{
	size_t total;

	if (!buf || !http || !len) {
		LOG_WRN("Invalid parameter");
		return -EINVAL;
	}

	total = sizeof(uint8_t) + sizeof(len) + len;
	if (net_buf_tailroom(buf) < total) {
		return -ENOMEM;
	}

	if (!bt_obex_string_is_valid(BT_OBEX_HEADER_ID_HTTP, len, http)) {
		LOG_WRN("Invalid string");
		return -EINVAL;
	}

	net_buf_add_u8(buf, BT_OBEX_HEADER_ID_HTTP);
	net_buf_add_be16(buf, (uint16_t)total);
	net_buf_add_mem(buf, http, len);
	return 0;
}

int bt_obex_add_header_body(struct net_buf *buf, uint16_t len, const uint8_t *body)
{
	size_t total;

	if (!buf || !body || !len) {
		LOG_WRN("Invalid parameter");
		return -EINVAL;
	}

	total = sizeof(uint8_t) + sizeof(len) + len;
	if (net_buf_tailroom(buf) < total) {
		return -ENOMEM;
	}

	if (!bt_obex_string_is_valid(BT_OBEX_HEADER_ID_BODY, len, body)) {
		LOG_WRN("Invalid string");
		return -EINVAL;
	}

	net_buf_add_u8(buf, BT_OBEX_HEADER_ID_BODY);
	net_buf_add_be16(buf, (uint16_t)total);
	net_buf_add_mem(buf, body, len);
	return 0;
}

int bt_obex_add_header_end_body(struct net_buf *buf, uint16_t len, const uint8_t *body)
{
	size_t total;

	if (!buf || !body || !len) {
		LOG_WRN("Invalid parameter");
		return -EINVAL;
	}

	total = sizeof(uint8_t) + sizeof(len) + len;
	if (net_buf_tailroom(buf) < total) {
		return -ENOMEM;
	}

	if (!bt_obex_string_is_valid(BT_OBEX_HEADER_ID_END_BODY, len, body)) {
		LOG_WRN("Invalid string");
		return -EINVAL;
	}

	net_buf_add_u8(buf, BT_OBEX_HEADER_ID_END_BODY);
	net_buf_add_be16(buf, (uint16_t)total);
	net_buf_add_mem(buf, body, len);
	return 0;
}

int bt_obex_add_header_who(struct net_buf *buf, uint16_t len, const uint8_t *who)
{
	size_t total;

	if (!buf || !who || !len) {
		LOG_WRN("Invalid parameter");
		return -EINVAL;
	}

	total = sizeof(uint8_t) + sizeof(len) + len;
	if (net_buf_tailroom(buf) < total) {
		return -ENOMEM;
	}

	if (!bt_obex_string_is_valid(BT_OBEX_HEADER_ID_WHO, len, who)) {
		LOG_WRN("Invalid string");
		return -EINVAL;
	}

	net_buf_add_u8(buf, BT_OBEX_HEADER_ID_WHO);
	net_buf_add_be16(buf, (uint16_t)total);
	net_buf_add_mem(buf, who, len);
	return 0;
}

int bt_obex_add_header_conn_id(struct net_buf *buf, uint32_t conn_id)
{
	size_t total;

	if (!buf) {
		LOG_WRN("Invalid buf");
		return -EINVAL;
	}

	total = sizeof(uint8_t) + sizeof(conn_id);
	if (net_buf_tailroom(buf) < total) {
		return -ENOMEM;
	}

	net_buf_add_u8(buf, BT_OBEX_HEADER_ID_CONN_ID);
	net_buf_add_be32(buf, conn_id);
	return 0;
}

int bt_obex_add_header_app_param(struct net_buf *buf, size_t count, const struct bt_obex_tlv data[])
{
	size_t total;
	uint16_t len = 0;

	if (!buf || !data || !count) {
		LOG_WRN("Invalid parameter");
		return -EINVAL;
	}

	for (size_t i = 0; i < count; i++) {
		if (data[i].data_len && !data[i].data) {
			LOG_WRN("Invalid parameter");
			return -EINVAL;
		}
		len += data[i].data_len + sizeof(data[i].type) + sizeof(data[i].data_len);
	}

	total = sizeof(uint8_t) + sizeof(len) + len;
	if (net_buf_tailroom(buf) < total) {
		return -ENOMEM;
	}

	net_buf_add_u8(buf, BT_OBEX_HEADER_ID_APP_PARAM);
	net_buf_add_be16(buf, (uint16_t)total);
	for (size_t i = 0; i < count; i++) {
		len += data[i].data_len + sizeof(data[i].type) + sizeof(data[i].data_len);
		net_buf_add_u8(buf, data[i].type);
		net_buf_add_u8(buf, data[i].data_len);
		net_buf_add_mem(buf, data[i].data, (size_t)data[i].data_len);
	}
	return 0;
}

int bt_obex_add_header_auth_challenge(struct net_buf *buf, size_t count,
				      const struct bt_obex_tlv data[])
{
	size_t total;
	uint16_t len = 0;

	if (!buf || !data || !count) {
		LOG_WRN("Invalid parameter");
		return -EINVAL;
	}

	for (size_t i = 0; i < count; i++) {
		if (data[i].data_len && !data[i].data) {
			LOG_WRN("Invalid parameter");
			return -EINVAL;
		}
		len += data[i].data_len + sizeof(data[i].type) + sizeof(data[i].data_len);
	}

	total = sizeof(uint8_t) + sizeof(len) + len;
	if (net_buf_tailroom(buf) < total) {
		return -ENOMEM;
	}

	net_buf_add_u8(buf, BT_OBEX_HEADER_ID_AUTH_CHALLENGE);
	net_buf_add_be16(buf, (uint16_t)total);
	for (size_t i = 0; i < count; i++) {
		len += data[i].data_len + sizeof(data[i].type) + sizeof(data[i].data_len);
		net_buf_add_u8(buf, data[i].type);
		net_buf_add_u8(buf, data[i].data_len);
		net_buf_add_mem(buf, data[i].data, (size_t)data[i].data_len);
	}
	return 0;
}

int bt_obex_add_header_auth_rsp(struct net_buf *buf, size_t count, const struct bt_obex_tlv data[])
{
	size_t total;
	uint16_t len = 0;

	if (!buf || !data || !count) {
		LOG_WRN("Invalid parameter");
		return -EINVAL;
	}

	for (size_t i = 0; i < count; i++) {
		if (data[i].data_len && !data[i].data) {
			LOG_WRN("Invalid parameter");
			return -EINVAL;
		}
		len += data[i].data_len + sizeof(data[i].type) + sizeof(data[i].data_len);
	}

	total = sizeof(uint8_t) + sizeof(len) + len;
	if (net_buf_tailroom(buf) < total) {
		return -ENOMEM;
	}

	net_buf_add_u8(buf, BT_OBEX_HEADER_ID_AUTH_RSP);
	net_buf_add_be16(buf, (uint16_t)total);
	for (size_t i = 0; i < count; i++) {
		len += data[i].data_len + sizeof(data[i].type) + sizeof(data[i].data_len);
		net_buf_add_u8(buf, data[i].type);
		net_buf_add_u8(buf, data[i].data_len);
		net_buf_add_mem(buf, data[i].data, (size_t)data[i].data_len);
	}
	return 0;
}

int bt_obex_add_header_creator_id(struct net_buf *buf, uint32_t creator_id)
{
	size_t total;

	if (!buf) {
		LOG_WRN("Invalid buf");
		return -EINVAL;
	}

	total = sizeof(uint8_t) + sizeof(creator_id);
	if (net_buf_tailroom(buf) < total) {
		return -ENOMEM;
	}

	net_buf_add_u8(buf, BT_OBEX_HEADER_ID_CREATE_ID);
	net_buf_add_be32(buf, creator_id);
	return 0;
}

int bt_obex_add_header_wan_uuid(struct net_buf *buf, uint16_t len, const uint8_t *uuid)
{
	size_t total;

	if (!buf || !uuid || !len) {
		LOG_WRN("Invalid parameter");
		return -EINVAL;
	}

	total = sizeof(uint8_t) + sizeof(len) + len;
	if (net_buf_tailroom(buf) < total) {
		return -ENOMEM;
	}

	if (!bt_obex_string_is_valid(BT_OBEX_HEADER_ID_WAN_UUID, len, uuid)) {
		LOG_WRN("Invalid string");
		return -EINVAL;
	}

	net_buf_add_u8(buf, BT_OBEX_HEADER_ID_WAN_UUID);
	net_buf_add_be16(buf, (uint16_t)total);
	net_buf_add_mem(buf, uuid, len);
	return 0;
}

int bt_obex_add_header_obj_class(struct net_buf *buf, uint16_t len, const uint8_t *obj_class)
{
	size_t total;

	if (!buf || !obj_class || !len) {
		LOG_WRN("Invalid parameter");
		return -EINVAL;
	}

	total = sizeof(uint8_t) + sizeof(len) + len;
	if (net_buf_tailroom(buf) < total) {
		return -ENOMEM;
	}

	if (!bt_obex_string_is_valid(BT_OBEX_HEADER_ID_OBJECT_CLASS, len, obj_class)) {
		LOG_WRN("Invalid string");
		return -EINVAL;
	}

	net_buf_add_u8(buf, BT_OBEX_HEADER_ID_OBJECT_CLASS);
	net_buf_add_be16(buf, (uint16_t)total);
	net_buf_add_mem(buf, obj_class, len);
	return 0;
}

int bt_obex_add_header_session_param(struct net_buf *buf, uint16_t len,
				     const uint8_t *session_param)
{
	size_t total;

	if (!buf || !session_param || !len) {
		LOG_WRN("Invalid parameter");
		return -EINVAL;
	}

	total = sizeof(uint8_t) + sizeof(len) + len;
	if (net_buf_tailroom(buf) < total) {
		return -ENOMEM;
	}

	if (!bt_obex_string_is_valid(BT_OBEX_HEADER_ID_SESSION_PARAM, len, session_param)) {
		LOG_WRN("Invalid string");
		return -EINVAL;
	}

	net_buf_add_u8(buf, BT_OBEX_HEADER_ID_SESSION_PARAM);
	net_buf_add_be16(buf, (uint16_t)total);
	net_buf_add_mem(buf, session_param, len);
	return 0;
}

int bt_obex_add_header_session_seq_number(struct net_buf *buf, uint32_t session_seq_number)
{
	size_t total;

	if (!buf) {
		LOG_WRN("Invalid buf");
		return -EINVAL;
	}

	total = sizeof(uint8_t) + sizeof(session_seq_number);
	if (net_buf_tailroom(buf) < total) {
		return -ENOMEM;
	}

	net_buf_add_u8(buf, BT_OBEX_HEADER_ID_SESSION_SEQ_NUM);
	net_buf_add_be32(buf, session_seq_number);
	return 0;
}

int bt_obex_add_header_action_id(struct net_buf *buf, uint8_t action_id)
{
	size_t total;

	if (!buf) {
		LOG_WRN("Invalid buf");
		return -EINVAL;
	}

	total = sizeof(uint8_t) + sizeof(action_id);
	if (net_buf_tailroom(buf) < total) {
		return -ENOMEM;
	}

	net_buf_add_u8(buf, BT_OBEX_HEADER_ID_ACTION_ID);
	net_buf_add_u8(buf, action_id);
	return 0;
}

int bt_obex_add_header_dest_name(struct net_buf *buf, uint16_t len, const uint8_t *dest_name)
{
	size_t total;

	if (!buf || !dest_name || !len) {
		LOG_WRN("Invalid parameter");
		return -EINVAL;
	}

	total = sizeof(uint8_t) + sizeof(len) + len;
	if (net_buf_tailroom(buf) < total) {
		return -ENOMEM;
	}

	if (!bt_obex_string_is_valid(BT_OBEX_HEADER_ID_DEST_NAME, len, dest_name)) {
		LOG_WRN("Invalid string");
		return -EINVAL;
	}

	net_buf_add_u8(buf, BT_OBEX_HEADER_ID_DEST_NAME);
	net_buf_add_be16(buf, (uint16_t)total);
	net_buf_add_mem(buf, dest_name, len);
	return 0;
}

int bt_obex_add_header_perm(struct net_buf *buf, uint32_t perm)
{
	size_t total;

	if (!buf) {
		LOG_WRN("Invalid buf");
		return -EINVAL;
	}

	total = sizeof(uint8_t) + sizeof(perm);
	if (net_buf_tailroom(buf) < total) {
		return -ENOMEM;
	}

	net_buf_add_u8(buf, BT_OBEX_HEADER_ID_PERM);
	net_buf_add_be32(buf, perm);
	return 0;
}

int bt_obex_add_header_srm(struct net_buf *buf, uint8_t srm)
{
	size_t total;

	if (!buf) {
		LOG_WRN("Invalid buf");
		return -EINVAL;
	}

	total = sizeof(uint8_t) + sizeof(srm);
	if (net_buf_tailroom(buf) < total) {
		return -ENOMEM;
	}

	net_buf_add_u8(buf, BT_OBEX_HEADER_ID_SRM);
	net_buf_add_u8(buf, srm);
	return 0;
}

int bt_obex_add_header_srm_param(struct net_buf *buf, uint8_t srm_param)
{
	size_t total;

	if (!buf) {
		LOG_WRN("Invalid buf");
		return -EINVAL;
	}

	total = sizeof(uint8_t) + sizeof(srm_param);
	if (net_buf_tailroom(buf) < total) {
		return -ENOMEM;
	}

	net_buf_add_u8(buf, BT_OBEX_HEADER_ID_SRM_PARAM);
	net_buf_add_u8(buf, srm_param);
	return 0;
}

int bt_obex_header_parse(struct net_buf *buf,
			 bool (*func)(struct bt_obex_hdr *hdr, void *user_data), void *user_data)
{
	uint16_t len = 0;
	struct bt_obex_hdr hdr;
	uint8_t header_id;
	uint16_t header_value_len;

	if (!buf || !func) {
		LOG_WRN("Invalid parameter");
		return -EINVAL;
	}

	while (len < buf->len) {
		header_id = buf->data[len];
		len++;

		switch (BT_OBEX_HEADER_ENCODING(header_id)) {
		case BT_OBEX_HEADER_ENCODING_1_BYTE:
			header_value_len = sizeof(header_id);
			break;
		case BT_OBEX_HEADER_ENCODING_4_BYTES:
			header_value_len = 4;
			break;
		case BT_OBEX_HEADER_ENCODING_UNICODE:
		case BT_OBEX_HEADER_ENCODING_BYTE_SEQ:
		default:
			if ((len + sizeof(header_value_len)) > buf->len) {
				return -EINVAL;
			}

			header_value_len = sys_get_be16(&buf->data[len]);
			if (header_value_len < (sizeof(header_id) + sizeof(header_value_len))) {
				return -EINVAL;
			}

			header_value_len -= sizeof(header_id) + sizeof(header_value_len);
			len += sizeof(header_value_len);
			break;
		}

		if ((len + header_value_len) > buf->len) {
			return -EINVAL;
		}

		hdr.id = header_id;
		hdr.data = &buf->data[len];
		hdr.len = header_value_len;
		len += header_value_len;

		if (!func(&hdr, user_data)) {
			return 0;
		}
	}
	return 0;
}

struct bt_obex_find_header_data {
	struct bt_obex_hdr hdr;
	bool found;
};

static bool bt_obex_find_header_cb(struct bt_obex_hdr *hdr, void *user_data)
{
	struct bt_obex_find_header_data *data;

	data = user_data;

	if (hdr->id == data->hdr.id) {
		data->hdr.data = hdr->data;
		data->hdr.len = hdr->len;
		data->found = true;
		return false;
	}
	return true;
}

int bt_obex_get_header_count(struct net_buf *buf, uint32_t *count)
{
	struct bt_obex_find_header_data data;
	int err;

	if (!buf || !count) {
		LOG_WRN("Invalid parameter");
		return -EINVAL;
	}

	data.hdr.id = BT_OBEX_HEADER_ID_COUNT;
	data.hdr.len = 0;
	data.hdr.data = NULL;
	data.found = false;

	err = bt_obex_header_parse(buf, bt_obex_find_header_cb, &data);
	if (err) {
		return err;
	}

	if ((data.hdr.len != sizeof(*count)) || !data.hdr.data) {
		return -ENODATA;
	}

	*count = sys_get_be32(data.hdr.data);
	return 0;
}

int bt_obex_get_header_name(struct net_buf *buf, uint16_t *len, const uint8_t **name)
{
	struct bt_obex_find_header_data data;
	int err;

	if (!buf || !len || !name) {
		LOG_WRN("Invalid parameter");
		return -EINVAL;
	}

	data.hdr.id = BT_OBEX_HEADER_ID_NAME;
	data.hdr.len = 0;
	data.hdr.data = NULL;
	data.found = false;

	err = bt_obex_header_parse(buf, bt_obex_find_header_cb, &data);
	if (err) {
		return err;
	}

	if (!data.found) {
		return -ENODATA;
	}

	*len = data.hdr.len;
	*name = data.hdr.data;
	return 0;
}

int bt_obex_get_header_type(struct net_buf *buf, uint16_t *len, const uint8_t **type)
{
	struct bt_obex_find_header_data data;
	int err;

	if (!buf || !len || !type) {
		LOG_WRN("Invalid parameter");
		return -EINVAL;
	}

	data.hdr.id = BT_OBEX_HEADER_ID_TYPE;
	data.hdr.len = 0;
	data.hdr.data = NULL;
	data.found = false;

	err = bt_obex_header_parse(buf, bt_obex_find_header_cb, &data);
	if (err) {
		return err;
	}

	if (!data.hdr.len || !data.hdr.data) {
		return -ENODATA;
	}

	*len = data.hdr.len;
	*type = data.hdr.data;
	return 0;
}

int bt_obex_get_header_len(struct net_buf *buf, uint32_t *len)
{
	struct bt_obex_find_header_data data;
	int err;

	if (!buf || !len) {
		LOG_WRN("Invalid parameter");
		return -EINVAL;
	}

	data.hdr.id = BT_OBEX_HEADER_ID_LEN;
	data.hdr.len = 0;
	data.hdr.data = NULL;
	data.found = false;

	err = bt_obex_header_parse(buf, bt_obex_find_header_cb, &data);
	if (err) {
		return err;
	}

	if ((data.hdr.len != sizeof(*len)) || !data.hdr.data) {
		return -ENODATA;
	}

	*len = sys_get_be32(data.hdr.data);
	return 0;
}

int bt_obex_get_header_time_iso_8601(struct net_buf *buf, uint16_t *len, const uint8_t **t)
{
	struct bt_obex_find_header_data data;
	int err;

	if (!buf || !len || !t) {
		LOG_WRN("Invalid parameter");
		return -EINVAL;
	}

	data.hdr.id = BT_OBEX_HEADER_ID_TIME_ISO_8601;
	data.hdr.len = 0;
	data.hdr.data = NULL;
	data.found = false;

	err = bt_obex_header_parse(buf, bt_obex_find_header_cb, &data);
	if (err) {
		return err;
	}

	if (!data.hdr.len || !data.hdr.data) {
		return -ENODATA;
	}

	*len = data.hdr.len;
	*t = data.hdr.data;
	return 0;
}

int bt_obex_get_header_time(struct net_buf *buf, uint32_t *t)
{
	struct bt_obex_find_header_data data;
	int err;

	if (!buf || !t) {
		LOG_WRN("Invalid parameter");
		return -EINVAL;
	}

	data.hdr.id = BT_OBEX_HEADER_ID_TIME;
	data.hdr.len = 0;
	data.hdr.data = NULL;
	data.found = false;

	err = bt_obex_header_parse(buf, bt_obex_find_header_cb, &data);
	if (err) {
		return err;
	}

	if ((data.hdr.len != sizeof(*t)) || !data.hdr.data) {
		return -ENODATA;
	}

	*t = sys_get_be32(data.hdr.data);
	return 0;
}

int bt_obex_get_header_description(struct net_buf *buf, uint16_t *len, const uint8_t **dec)
{
	struct bt_obex_find_header_data data;
	int err;

	if (!buf || !len || !dec) {
		LOG_WRN("Invalid parameter");
		return -EINVAL;
	}

	data.hdr.id = BT_OBEX_HEADER_ID_DES;
	data.hdr.len = 0;
	data.hdr.data = NULL;
	data.found = false;

	err = bt_obex_header_parse(buf, bt_obex_find_header_cb, &data);
	if (err) {
		return err;
	}

	if (!data.hdr.len || !data.hdr.data) {
		return -ENODATA;
	}

	*len = data.hdr.len;
	*dec = data.hdr.data;
	return 0;
}

int bt_obex_get_header_target(struct net_buf *buf, uint16_t *len, const uint8_t **target)
{
	struct bt_obex_find_header_data data;
	int err;

	if (!buf || !len || !target) {
		LOG_WRN("Invalid parameter");
		return -EINVAL;
	}

	data.hdr.id = BT_OBEX_HEADER_ID_TARGET;
	data.hdr.len = 0;
	data.hdr.data = NULL;
	data.found = false;

	err = bt_obex_header_parse(buf, bt_obex_find_header_cb, &data);
	if (err) {
		return err;
	}

	if (!data.hdr.len || !data.hdr.data) {
		return -ENODATA;
	}

	*len = data.hdr.len;
	*target = data.hdr.data;
	return 0;
}

int bt_obex_get_header_http(struct net_buf *buf, uint16_t *len, const uint8_t **http)
{
	struct bt_obex_find_header_data data;
	int err;

	if (!buf || !len || !http) {
		LOG_WRN("Invalid parameter");
		return -EINVAL;
	}

	data.hdr.id = BT_OBEX_HEADER_ID_HTTP;
	data.hdr.len = 0;
	data.hdr.data = NULL;
	data.found = false;

	err = bt_obex_header_parse(buf, bt_obex_find_header_cb, &data);
	if (err) {
		return err;
	}

	if (!data.hdr.len || !data.hdr.data) {
		return -ENODATA;
	}

	*len = data.hdr.len;
	*http = data.hdr.data;
	return 0;
}

int bt_obex_get_header_body(struct net_buf *buf, uint16_t *len, const uint8_t **body)
{
	struct bt_obex_find_header_data data;
	int err;

	if (!buf || !len || !body) {
		LOG_WRN("Invalid parameter");
		return -EINVAL;
	}

	data.hdr.id = BT_OBEX_HEADER_ID_BODY;
	data.hdr.len = 0;
	data.hdr.data = NULL;
	data.found = false;

	err = bt_obex_header_parse(buf, bt_obex_find_header_cb, &data);
	if (err) {
		return err;
	}

	if (!data.hdr.len || !data.hdr.data) {
		return -ENODATA;
	}

	*len = data.hdr.len;
	*body = data.hdr.data;
	return 0;
}

int bt_obex_get_header_end_body(struct net_buf *buf, uint16_t *len, const uint8_t **body)
{
	struct bt_obex_find_header_data data;
	int err;

	if (!buf || !len || !body) {
		LOG_WRN("Invalid parameter");
		return -EINVAL;
	}

	data.hdr.id = BT_OBEX_HEADER_ID_END_BODY;
	data.hdr.len = 0;
	data.hdr.data = NULL;
	data.found = false;

	err = bt_obex_header_parse(buf, bt_obex_find_header_cb, &data);
	if (err) {
		return err;
	}

	if (!data.hdr.len || !data.hdr.data) {
		return -ENODATA;
	}

	*len = data.hdr.len;
	*body = data.hdr.data;
	return 0;
}

int bt_obex_get_header_who(struct net_buf *buf, uint16_t *len, const uint8_t **who)
{
	struct bt_obex_find_header_data data;
	int err;

	if (!buf || !len || !who) {
		LOG_WRN("Invalid parameter");
		return -EINVAL;
	}

	data.hdr.id = BT_OBEX_HEADER_ID_WHO;
	data.hdr.len = 0;
	data.hdr.data = NULL;
	data.found = false;

	err = bt_obex_header_parse(buf, bt_obex_find_header_cb, &data);
	if (err) {
		return err;
	}

	if (!data.hdr.len || !data.hdr.data) {
		return -ENODATA;
	}

	*len = data.hdr.len;
	*who = data.hdr.data;
	return 0;
}

int bt_obex_get_header_conn_id(struct net_buf *buf, uint32_t *conn_id)
{
	struct bt_obex_find_header_data data;
	int err;

	if (!buf || !conn_id) {
		LOG_WRN("Invalid parameter");
		return -EINVAL;
	}

	data.hdr.id = BT_OBEX_HEADER_ID_CONN_ID;
	data.hdr.len = 0;
	data.hdr.data = NULL;
	data.found = false;

	err = bt_obex_header_parse(buf, bt_obex_find_header_cb, &data);
	if (err) {
		return err;
	}

	if ((data.hdr.len != sizeof(*conn_id)) || !data.hdr.data) {
		return -ENODATA;
	}

	*conn_id = sys_get_be32(data.hdr.data);
	return 0;
}

int bt_obex_tlv_parse(uint16_t len, const uint8_t *data,
		      bool (*func)(struct bt_obex_tlv *tlv, void *user_data), void *user_data)
{
	uint16_t index = 0;
	struct bt_obex_tlv tlv;

	if (!len || !data || !func) {
		LOG_WRN("Invalid parameter");
		return -EINVAL;
	}

	while ((index + 2) <= len) {
		tlv.type = data[index];
		tlv.data_len = data[index + 1];
		index += 2;
		if ((index + tlv.data_len) > len) {
			break;
		}

		tlv.data = &data[index];
		index += tlv.data_len;
		if (!func(&tlv, user_data)) {
			return 0;
		}
	}
	return 0;
}

int bt_obex_get_header_app_param(struct net_buf *buf, uint16_t *len, const uint8_t **app_param)
{
	struct bt_obex_find_header_data data;
	int err;

	if (!buf || !len || !app_param) {
		LOG_WRN("Invalid parameter");
		return -EINVAL;
	}

	data.hdr.id = BT_OBEX_HEADER_ID_APP_PARAM;
	data.hdr.len = 0;
	data.hdr.data = NULL;
	data.found = false;

	err = bt_obex_header_parse(buf, bt_obex_find_header_cb, &data);
	if (err) {
		return err;
	}

	if (!data.hdr.len || !data.hdr.data) {
		return -ENODATA;
	}

	*len = data.hdr.len;
	*app_param = data.hdr.data;
	return 0;
}

int bt_obex_get_header_auth_challenge(struct net_buf *buf, uint16_t *len, const uint8_t **auth)
{
	struct bt_obex_find_header_data data;
	int err;

	if (!buf || !len || !auth) {
		LOG_WRN("Invalid parameter");
		return -EINVAL;
	}

	data.hdr.id = BT_OBEX_HEADER_ID_AUTH_CHALLENGE;
	data.hdr.len = 0;
	data.hdr.data = NULL;
	data.found = false;

	err = bt_obex_header_parse(buf, bt_obex_find_header_cb, &data);
	if (err) {
		return err;
	}

	if (!data.hdr.len || !data.hdr.data) {
		return -ENODATA;
	}

	*len = data.hdr.len;
	*auth = data.hdr.data;
	return 0;
}

int bt_obex_get_header_auth_rsp(struct net_buf *buf, uint16_t *len, const uint8_t **auth)
{
	struct bt_obex_find_header_data data;
	int err;

	if (!buf || !len || !auth) {
		LOG_WRN("Invalid parameter");
		return -EINVAL;
	}

	data.hdr.id = BT_OBEX_HEADER_ID_AUTH_RSP;
	data.hdr.len = 0;
	data.hdr.data = NULL;
	data.found = false;

	err = bt_obex_header_parse(buf, bt_obex_find_header_cb, &data);
	if (err) {
		return err;
	}

	if (!data.hdr.len || !data.hdr.data) {
		return -ENODATA;
	}

	*len = data.hdr.len;
	*auth = data.hdr.data;
	return 0;
}

int bt_obex_get_header_creator_id(struct net_buf *buf, uint32_t *creator_id)
{
	struct bt_obex_find_header_data data;
	int err;

	if (!buf || !creator_id) {
		LOG_WRN("Invalid parameter");
		return -EINVAL;
	}

	data.hdr.id = BT_OBEX_HEADER_ID_CREATE_ID;
	data.hdr.len = 0;
	data.hdr.data = NULL;
	data.found = false;

	err = bt_obex_header_parse(buf, bt_obex_find_header_cb, &data);
	if (err) {
		return err;
	}

	if ((data.hdr.len != sizeof(*creator_id)) || !data.hdr.data) {
		return -ENODATA;
	}

	*creator_id = sys_get_be32(data.hdr.data);
	return 0;
}

int bt_obex_get_header_wan_uuid(struct net_buf *buf, uint16_t *len, const uint8_t **uuid)
{
	struct bt_obex_find_header_data data;
	int err;

	if (!buf || !len || !uuid) {
		LOG_WRN("Invalid parameter");
		return -EINVAL;
	}

	data.hdr.id = BT_OBEX_HEADER_ID_WAN_UUID;
	data.hdr.len = 0;
	data.hdr.data = NULL;
	data.found = false;

	err = bt_obex_header_parse(buf, bt_obex_find_header_cb, &data);
	if (err) {
		return err;
	}

	if (!data.hdr.len || !data.hdr.data) {
		return -ENODATA;
	}

	*len = data.hdr.len;
	*uuid = data.hdr.data;
	return 0;
}

int bt_obex_get_header_obj_class(struct net_buf *buf, uint16_t *len, const uint8_t **obj_class)
{
	struct bt_obex_find_header_data data;
	int err;

	if (!buf || !len || !obj_class) {
		LOG_WRN("Invalid parameter");
		return -EINVAL;
	}

	data.hdr.id = BT_OBEX_HEADER_ID_OBJECT_CLASS;
	data.hdr.len = 0;
	data.hdr.data = NULL;
	data.found = false;

	err = bt_obex_header_parse(buf, bt_obex_find_header_cb, &data);
	if (err) {
		return err;
	}

	if (!data.hdr.len || !data.hdr.data) {
		return -ENODATA;
	}

	*len = data.hdr.len;
	*obj_class = data.hdr.data;
	return 0;
}

int bt_obex_get_header_session_param(struct net_buf *buf, uint16_t *len,
				     const uint8_t **session_param)
{
	struct bt_obex_find_header_data data;
	int err;

	if (!buf || !len || !session_param) {
		LOG_WRN("Invalid parameter");
		return -EINVAL;
	}

	data.hdr.id = BT_OBEX_HEADER_ID_SESSION_PARAM;
	data.hdr.len = 0;
	data.hdr.data = NULL;
	data.found = false;

	err = bt_obex_header_parse(buf, bt_obex_find_header_cb, &data);
	if (err) {
		return err;
	}

	if (!data.hdr.len || !data.hdr.data) {
		return -ENODATA;
	}

	*len = data.hdr.len;
	*session_param = data.hdr.data;
	return 0;
}

int bt_obex_get_header_session_seq_number(struct net_buf *buf, uint32_t *session_seq_number)
{
	struct bt_obex_find_header_data data;
	int err;

	if (!buf || !session_seq_number) {
		LOG_WRN("Invalid parameter");
		return -EINVAL;
	}

	data.hdr.id = BT_OBEX_HEADER_ID_SESSION_SEQ_NUM;
	data.hdr.len = 0;
	data.hdr.data = NULL;
	data.found = false;

	err = bt_obex_header_parse(buf, bt_obex_find_header_cb, &data);
	if (err) {
		return err;
	}

	if ((data.hdr.len != sizeof(*session_seq_number)) || !data.hdr.data) {
		return -ENODATA;
	}

	*session_seq_number = sys_get_be32(data.hdr.data);
	return 0;
}

int bt_obex_get_header_action_id(struct net_buf *buf, uint8_t *action_id)
{
	struct bt_obex_find_header_data data;
	int err;

	if (!buf || !action_id) {
		LOG_WRN("Invalid parameter");
		return -EINVAL;
	}

	data.hdr.id = BT_OBEX_HEADER_ID_ACTION_ID;
	data.hdr.len = 0;
	data.hdr.data = NULL;
	data.found = false;

	err = bt_obex_header_parse(buf, bt_obex_find_header_cb, &data);
	if (err) {
		return err;
	}

	if ((data.hdr.len != sizeof(*action_id)) || !data.hdr.data) {
		return -ENODATA;
	}

	*action_id = data.hdr.data[0];
	return 0;
}

int bt_obex_get_header_dest_name(struct net_buf *buf, uint16_t *len, const uint8_t **dest_name)
{
	struct bt_obex_find_header_data data;
	int err;

	if (!buf || !len || !dest_name) {
		LOG_WRN("Invalid parameter");
		return -EINVAL;
	}

	data.hdr.id = BT_OBEX_HEADER_ID_DEST_NAME;
	data.hdr.len = 0;
	data.hdr.data = NULL;
	data.found = false;

	err = bt_obex_header_parse(buf, bt_obex_find_header_cb, &data);
	if (err) {
		return err;
	}

	if (!data.hdr.len || !data.hdr.data) {
		return -ENODATA;
	}

	*len = data.hdr.len;
	*dest_name = data.hdr.data;
	return 0;
}

int bt_obex_get_header_perm(struct net_buf *buf, uint32_t *perm)
{
	struct bt_obex_find_header_data data;
	int err;

	if (!buf || !perm) {
		LOG_WRN("Invalid parameter");
		return -EINVAL;
	}

	data.hdr.id = BT_OBEX_HEADER_ID_PERM;
	data.hdr.len = 0;
	data.hdr.data = NULL;
	data.found = false;

	err = bt_obex_header_parse(buf, bt_obex_find_header_cb, &data);
	if (err) {
		return err;
	}

	if ((data.hdr.len != sizeof(*perm)) || !data.hdr.data) {
		return -ENODATA;
	}

	*perm = sys_get_be32(data.hdr.data);
	return 0;
}

int bt_obex_get_header_srm(struct net_buf *buf, uint8_t *srm)
{
	struct bt_obex_find_header_data data;
	int err;

	if (!buf || !srm) {
		LOG_WRN("Invalid parameter");
		return -EINVAL;
	}

	data.hdr.id = BT_OBEX_HEADER_ID_SRM;
	data.hdr.len = 0;
	data.hdr.data = NULL;
	data.found = false;

	err = bt_obex_header_parse(buf, bt_obex_find_header_cb, &data);
	if (err) {
		return err;
	}

	if ((data.hdr.len != sizeof(*srm)) || !data.hdr.data) {
		return -ENODATA;
	}

	*srm = data.hdr.data[0];
	return 0;
}

int bt_obex_get_header_srm_param(struct net_buf *buf, uint8_t *srm_param)
{
	struct bt_obex_find_header_data data;
	int err;

	if (!buf || !srm_param) {
		LOG_WRN("Invalid parameter");
		return -EINVAL;
	}

	data.hdr.id = BT_OBEX_HEADER_ID_SRM_PARAM;
	data.hdr.len = 0;
	data.hdr.data = NULL;
	data.found = false;

	err = bt_obex_header_parse(buf, bt_obex_find_header_cb, &data);
	if (err) {
		return err;
	}

	if ((data.hdr.len != sizeof(*srm_param)) || !data.hdr.data) {
		return -ENODATA;
	}

	*srm_param = data.hdr.data[0];
	return 0;
}

#if defined(CONFIG_BT_OEBX_RSP_CODE_TO_STR)
const char *bt_obex_rsp_code_to_str(enum bt_obex_rsp_code rsp_code)
{
	const char *rsp_code_str;

	switch (rsp_code) {
	case BT_OBEX_RSP_CODE_CONTINUE:
		rsp_code_str = "Continue";
		break;
	case BT_OBEX_RSP_CODE_SUCCESS:
		rsp_code_str = "Success";
		break;
	case BT_OBEX_RSP_CODE_CREATED:
		rsp_code_str = "Created";
		break;
	case BT_OBEX_RSP_CODE_ACCEPTED:
		rsp_code_str = "Accepted";
		break;
	case BT_OBEX_RSP_CODE_NON_AUTH_INFO:
		rsp_code_str = "Non-Authoritative Information";
		break;
	case BT_OBEX_RSP_CODE_NO_CONTENT:
		rsp_code_str = "No Content";
		break;
	case BT_OBEX_RSP_CODE_RESET_CONTENT:
		rsp_code_str = "Reset Content";
		break;
	case BT_OBEX_RSP_CODE_PARTIAL_CONTENT:
		rsp_code_str = "Partial Content";
		break;
	case BT_OBEX_RSP_CODE_MULTI_CHOICES:
		rsp_code_str = "Multiple Choices";
		break;
	case BT_OBEX_RSP_CODE_MOVED_PERM:
		rsp_code_str = "Moved Permanently";
		break;
	case BT_OBEX_RSP_CODE_MOVED_TEMP:
		rsp_code_str = "Moved temporarily";
		break;
	case BT_OBEX_RSP_CODE_SEE_OTHER:
		rsp_code_str = "See Other";
		break;
	case BT_OBEX_RSP_CODE_NOT_MODIFIED:
		rsp_code_str = "Not modified";
		break;
	case BT_OBEX_RSP_CODE_USE_PROXY:
		rsp_code_str = "Use Proxy";
		break;
	case BT_OBEX_RSP_CODE_BAD_REQ:
		rsp_code_str = "Bad Request - server couldn't understand request";
		break;
	case BT_OBEX_RSP_CODE_UNAUTH:
		rsp_code_str = "Unauthorized";
		break;
	case BT_OBEX_RSP_CODE_PAY_REQ:
		rsp_code_str = "Payment Required";
		break;
	case BT_OBEX_RSP_CODE_FORBIDDEN:
		rsp_code_str = "Forbidden - operation is understood but refused";
		break;
	case BT_OBEX_RSP_CODE_NOT_FOUND:
		rsp_code_str = "Not Found";
		break;
	case BT_OBEX_RSP_CODE_NOT_ALLOW:
		rsp_code_str = "Method Not Allowed";
		break;
	case BT_OBEX_RSP_CODE_NOT_ACCEPT:
		rsp_code_str = "Not Acceptable";
		break;
	case BT_OBEX_RSP_CODE_PROXY_AUTH_REQ:
		rsp_code_str = "Proxy Authentication Required";
		break;
	case BT_OBEX_RSP_CODE_REQ_TIMEOUT:
		rsp_code_str = "Request Time Out";
		break;
	case BT_OBEX_RSP_CODE_CONFLICT:
		rsp_code_str = "Conflict";
		break;
	case BT_OBEX_RSP_CODE_GONE:
		rsp_code_str = "Gone";
		break;
	case BT_OBEX_RSP_CODE_LEN_REQ:
		rsp_code_str = "Length Required";
		break;
	case BT_OBEX_RSP_CODE_PRECON_FAIL:
		rsp_code_str = "Precondition Failed";
		break;
	case BT_OBEX_RSP_CODE_ENTITY_TOO_LARGE:
		rsp_code_str = "Requested Entity Too Large";
		break;
	case BT_OBEX_RSP_CODE_URL_TOO_LARGE:
		rsp_code_str = "Requested URL Too Large";
		break;
	case BT_OBEX_RSP_CODE_UNSUPP_MEDIA_TYPE:
		rsp_code_str = "Unsupported media type";
		break;
	case BT_OBEX_RSP_CODE_INTER_ERROR:
		rsp_code_str = "Internal serve Error";
		break;
	case BT_OBEX_RSP_CODE_NOT_IMPL:
		rsp_code_str = "Not Implemented";
		break;
	case BT_OBEX_RSP_CODE_BAD_GATEWAY:
		rsp_code_str = "Bad Gateway";
		break;
	case BT_OBEX_RSP_CODE_UNAVAIL:
		rsp_code_str = "Service Unavailable";
		break;
	case BT_OBEX_RSP_CODE_GATEWAY_TIMEOUT:
		rsp_code_str = "Gateway Timeout";
		break;
	case BT_OBEX_RSP_CODE_VER_UNSUPP:
		rsp_code_str = "HTTP Version not supported";
		break;
	case BT_OBEX_RSP_CODE_DB_FULL:
		rsp_code_str = "Database Full";
		break;
	case BT_OBEX_RSP_CODE_DB_LOCK:
		rsp_code_str = "Database Locked";
		break;
	default:
		rsp_code_str = "Unknown";
		break;
	}

	return rsp_code_str;
}
#endif /* CONFIG_BT_OEBX_RSP_CODE_TO_STR */
