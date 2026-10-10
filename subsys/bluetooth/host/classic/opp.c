/* opp.c - Bluetooth Object Push Profile handling */

/*
 * Copyright 2026 NXP
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
#include <zephyr/bluetooth/classic/goep.h>
#include <zephyr/bluetooth/classic/opp.h>

#include <host/conn_internal.h>
#include "l2cap_br_internal.h"
#include "rfcomm_internal.h"
#include "obex_internal.h"
#include "opp_internal.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_opp, CONFIG_BT_OPP_LOG_LEVEL);

/* Buffer pool: two buffers per ACL connection (one for operation, one for response). */
#define OPP_POOL_BUF_SIZE  BT_RFCOMM_BUF_SIZE(CONFIG_BT_GOEP_RFCOMM_MTU)

NET_BUF_POOL_FIXED_DEFINE(bt_opp_pool, 2 * CONFIG_BT_MAX_CONN, OPP_POOL_BUF_SIZE,
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

#if defined(CONFIG_BT_OPP_CLIENT)

/* Forward declarations of OBEX client ops */
static void opp_client_obex_connect_cb(struct bt_obex_client *client, uint8_t rsp_code,
					uint8_t version, uint16_t mopl, struct net_buf *buf);
static void opp_client_obex_disconnect_cb(struct bt_obex_client *client, uint8_t rsp_code,
					   struct net_buf *buf);
static void opp_client_obex_put_cb(struct bt_obex_client *client, uint8_t rsp_code,
				    struct net_buf *buf);
static void opp_client_obex_get_cb(struct bt_obex_client *client, uint8_t rsp_code,
				    struct net_buf *buf);
static void opp_client_obex_abort_cb(struct bt_obex_client *client, uint8_t rsp_code,
				      struct net_buf *buf);

static const struct bt_obex_client_ops opp_client_obex_ops = {
	.connect    = opp_client_obex_connect_cb,
	.disconnect = opp_client_obex_disconnect_cb,
	.put        = opp_client_obex_put_cb,
	.get        = opp_client_obex_get_cb,
	.abort      = opp_client_obex_abort_cb,
};

/* GOEP transport ops for the client */
static void opp_client_transport_connected(struct bt_conn *conn, struct bt_goep *goep)
{
	struct bt_opp_client *client =
		CONTAINER_OF(goep, struct bt_opp_client, _goep);

	atomic_set(&client->_transport_state, BT_OPP_TRANSPORT_STATE_CONNECTED);

	LOG_DBG("client %p RFCOMM connected", client);

	if (client->cb && client->cb->rfcomm_connected) {
		client->cb->rfcomm_connected(conn, client);
	}
}

static void opp_client_transport_disconnected(struct bt_goep *goep)
{
	struct bt_opp_client *client =
		CONTAINER_OF(goep, struct bt_opp_client, _goep);

	atomic_set(&client->_transport_state, BT_OPP_TRANSPORT_STATE_DISCONNECTED);
	atomic_set(&client->_state, BT_OPP_STATE_DISCONNECTED);

	LOG_DBG("client %p RFCOMM disconnected", client);

	if (client->cb && client->cb->rfcomm_disconnected) {
		client->cb->rfcomm_disconnected(client);
	}
}

static const struct bt_goep_transport_ops opp_client_goep_transport_ops = {
	.connected    = opp_client_transport_connected,
	.disconnected = opp_client_transport_disconnected,
};

/* OBEX client callback implementations */
static void opp_client_obex_connect_cb(struct bt_obex_client *obex_client, uint8_t rsp_code,
					uint8_t version, uint16_t mopl, struct net_buf *buf)
{
	struct bt_opp_client *client =
		CONTAINER_OF(obex_client, struct bt_opp_client, _client);

	/*
	 * BT_OBEX_RSP_CODE_OK and BT_OBEX_RSP_CODE_SUCCESS share the same
	 * numeric value; a single check is sufficient.
	 */
	if (rsp_code == BT_OBEX_RSP_CODE_SUCCESS) {
		atomic_set(&client->_state, BT_OPP_STATE_CONNECTED);
	} else {
		atomic_set(&client->_state, BT_OPP_STATE_DISCONNECTED);
	}

	LOG_DBG("client %p OBEX connect rsp 0x%02x version 0x%02x mopl %u",
		client, rsp_code, version, mopl);

	if (client->cb && client->cb->connect) {
		client->cb->connect(client, rsp_code, version, mopl, buf);
	}
}

static void opp_client_obex_disconnect_cb(struct bt_obex_client *obex_client, uint8_t rsp_code,
					   struct net_buf *buf)
{
	struct bt_opp_client *client =
		CONTAINER_OF(obex_client, struct bt_opp_client, _client);

	atomic_set(&client->_state, BT_OPP_STATE_DISCONNECTED);

	LOG_DBG("client %p OBEX disconnect rsp 0x%02x", client, rsp_code);

	if (client->cb && client->cb->disconnect) {
		client->cb->disconnect(client, rsp_code, buf);
	}
}

static void opp_client_obex_put_cb(struct bt_obex_client *obex_client, uint8_t rsp_code,
				    struct net_buf *buf)
{
	struct bt_opp_client *client =
		CONTAINER_OF(obex_client, struct bt_opp_client, _client);

	LOG_DBG("client %p PUT rsp 0x%02x", client, rsp_code);

	if (client->cb && client->cb->push) {
		client->cb->push(client, rsp_code, buf);
	}
}

static void opp_client_obex_get_cb(struct bt_obex_client *obex_client, uint8_t rsp_code,
				    struct net_buf *buf)
{
	struct bt_opp_client *client =
		CONTAINER_OF(obex_client, struct bt_opp_client, _client);

	LOG_DBG("client %p GET rsp 0x%02x", client, rsp_code);

	if (client->cb && client->cb->pull_bcard) {
		client->cb->pull_bcard(client, rsp_code, buf);
	}
}

static void opp_client_obex_abort_cb(struct bt_obex_client *obex_client, uint8_t rsp_code,
				      struct net_buf *buf)
{
	struct bt_opp_client *client =
		CONTAINER_OF(obex_client, struct bt_opp_client, _client);

	LOG_DBG("client %p ABORT rsp 0x%02x", client, rsp_code);

	if (client->cb && client->cb->abort) {
		client->cb->abort(client, rsp_code, buf);
	}
}

int bt_opp_client_connect_rfcomm(struct bt_conn *conn, struct bt_opp_client *client,
				  const struct bt_opp_client_cb *cb, uint8_t channel)
{
	int err;

	if (conn == NULL || client == NULL || cb == NULL) {
		return -EINVAL;
	}

	if (atomic_get(&client->_transport_state) != BT_OPP_TRANSPORT_STATE_DISCONNECTED) {
		return -EALREADY;
	}

	client->cb = cb;
	atomic_set(&client->_transport_state, BT_OPP_TRANSPORT_STATE_CONNECTING);
	atomic_set(&client->_state, BT_OPP_STATE_DISCONNECTED);

	BT_GOEP_INIT_V1(&client->_goep, &client->_goep_transport.v1);
	client->_goep.transport_ops = &opp_client_goep_transport_ops;
	client->_client.ops = &opp_client_obex_ops;
	client->_client.obex = &client->_goep.obex;

	err = bt_goep_transport_rfcomm_connect(conn, &client->_goep, channel);

	if (err != 0) {
		atomic_set(&client->_transport_state, BT_OPP_TRANSPORT_STATE_DISCONNECTED);
	}

	return err;
}

int bt_opp_client_disconnect_rfcomm(struct bt_opp_client *client)
{
	int err;

	if (client == NULL) {
		return -EINVAL;
	}

	atomic_set(&client->_transport_state, BT_OPP_TRANSPORT_STATE_DISCONNECTING);

	err = bt_goep_transport_rfcomm_disconnect(&client->_goep);

	if (err != 0) {
		atomic_set(&client->_transport_state, BT_OPP_TRANSPORT_STATE_CONNECTED);
	}

	return err;
}

struct net_buf *bt_opp_client_create_pdu(struct bt_opp_client *client,
					 struct net_buf_pool *pool)
{
	if (client == NULL) {
		return NULL;
	}

	return bt_goep_create_pdu(&client->_goep, pool != NULL ? pool : &bt_opp_pool);
}

int bt_opp_client_connect(struct bt_opp_client *client, uint16_t mopl, struct net_buf *buf)
{
	if (client == NULL) {
		return -EINVAL;
	}

	if (atomic_get(&client->_transport_state) != BT_OPP_TRANSPORT_STATE_CONNECTED) {
		return -ENOTCONN;
	}

	if (atomic_get(&client->_state) != BT_OPP_STATE_DISCONNECTED) {
		return -EALREADY;
	}

	atomic_set(&client->_state, BT_OPP_STATE_CONNECTING);

	/* OPP spec section 5.4: no Target header. */
	return bt_obex_connect(&client->_client, mopl, buf);
}

int bt_opp_client_disconnect(struct bt_opp_client *client, struct net_buf *buf)
{
	if (client == NULL) {
		return -EINVAL;
	}

	if (atomic_get(&client->_state) != BT_OPP_STATE_CONNECTED) {
		return -ENOTCONN;
	}

	atomic_set(&client->_state, BT_OPP_STATE_DISCONNECTING);

	return bt_obex_disconnect(&client->_client, buf);
}

int bt_opp_client_push(struct bt_opp_client *client, bool final, struct net_buf *buf)
{
	if (client == NULL || buf == NULL) {
		return -EINVAL;
	}

	if (atomic_get(&client->_state) != BT_OPP_STATE_CONNECTED) {
		return -ENOTCONN;
	}

	return bt_obex_put(&client->_client, final, buf);
}

int bt_opp_client_pull_bcard(struct bt_opp_client *client, struct net_buf *buf)
{
	if (client == NULL || buf == NULL) {
		return -EINVAL;
	}

	if (atomic_get(&client->_state) != BT_OPP_STATE_CONNECTED) {
		return -ENOTCONN;
	}

	/* Final bit set for the first (and often only) GET request. */
	return bt_obex_get(&client->_client, true, buf);
}

int bt_opp_client_abort(struct bt_opp_client *client, struct net_buf *buf)
{
	if (client == NULL) {
		return -EINVAL;
	}

	if (atomic_get(&client->_state) != BT_OPP_STATE_CONNECTED) {
		return -ENOTCONN;
	}

	return bt_obex_abort(&client->_client, buf);
}

#endif /* CONFIG_BT_OPP_CLIENT */

#if defined(CONFIG_BT_OPP_SERVER)

/* Forward declarations of OBEX server ops */
static void opp_server_obex_connect_cb(struct bt_obex_server *server, uint8_t version,
					uint16_t mopl, struct net_buf *buf);
static void opp_server_obex_disconnect_cb(struct bt_obex_server *server, struct net_buf *buf);
static void opp_server_obex_put_cb(struct bt_obex_server *server, bool final,
				    struct net_buf *buf);
static void opp_server_obex_get_cb(struct bt_obex_server *server, bool final,
				    struct net_buf *buf);
static void opp_server_obex_abort_cb(struct bt_obex_server *server, struct net_buf *buf);

static const struct bt_obex_server_ops opp_server_obex_ops = {
	.connect    = opp_server_obex_connect_cb,
	.disconnect = opp_server_obex_disconnect_cb,
	.put        = opp_server_obex_put_cb,
	.get        = opp_server_obex_get_cb,
	.abort      = opp_server_obex_abort_cb,
};

/* GOEP transport ops for the server */
static void opp_server_transport_connected(struct bt_conn *conn, struct bt_goep *goep)
{
	struct bt_opp_server *server =
		CONTAINER_OF(goep, struct bt_opp_server, _goep);

	atomic_set(&server->_transport_state, BT_OPP_TRANSPORT_STATE_CONNECTED);

	LOG_DBG("server %p RFCOMM connected", server);

	if (server->cb && server->cb->rfcomm_connected) {
		server->cb->rfcomm_connected(conn, server);
	}
}

static void opp_server_transport_disconnected(struct bt_goep *goep)
{
	struct bt_opp_server *server =
		CONTAINER_OF(goep, struct bt_opp_server, _goep);

	atomic_set(&server->_transport_state, BT_OPP_TRANSPORT_STATE_DISCONNECTED);
	atomic_set(&server->_state, BT_OPP_STATE_DISCONNECTED);
	atomic_clear_bit(&server->_flags, BT_OPP_FLAG_RSP_ONGOING_BIT);

	LOG_DBG("server %p RFCOMM disconnected", server);

	if (server->cb && server->cb->rfcomm_disconnected) {
		server->cb->rfcomm_disconnected(server);
	}
}

static const struct bt_goep_transport_ops opp_server_goep_transport_ops = {
	.connected    = opp_server_transport_connected,
	.disconnected = opp_server_transport_disconnected,
};

/* OBEX server callback implementations */
static void opp_server_obex_connect_cb(struct bt_obex_server *obex_server, uint8_t version,
					uint16_t mopl, struct net_buf *buf)
{
	struct bt_opp_server *server =
		CONTAINER_OF(obex_server, struct bt_opp_server, _server);

	/*
	 * Set CONNECTING to indicate the session is pending application approval.
	 * The final state (CONNECTED or DISCONNECTED) is set by
	 * bt_opp_server_connect_rsp() once the application calls it.
	 */
	atomic_set(&server->_state, BT_OPP_STATE_CONNECTING);

	LOG_DBG("server %p OBEX connect req version 0x%02x mopl %u", server, version, mopl);

	if (server->cb && server->cb->connect) {
		server->cb->connect(server, version, mopl, buf);
	} else {
		/* Auto-respond SUCCESS if no application handler. */
		(void)bt_opp_server_connect_rsp(server, mopl, BT_OPP_RSP_CODE_SUCCESS, NULL);
	}
}

static void opp_server_obex_disconnect_cb(struct bt_obex_server *obex_server, struct net_buf *buf)
{
	struct bt_opp_server *server =
		CONTAINER_OF(obex_server, struct bt_opp_server, _server);

	LOG_DBG("server %p OBEX disconnect req", server);

	if (server->cb && server->cb->disconnect) {
		server->cb->disconnect(server, buf);
	} else {
		(void)bt_opp_server_disconnect_rsp(server, BT_OPP_RSP_CODE_SUCCESS, NULL);
	}
}

static void opp_server_obex_put_cb(struct bt_obex_server *obex_server, bool final,
				    struct net_buf *buf)
{
	struct bt_opp_server *server =
		CONTAINER_OF(obex_server, struct bt_opp_server, _server);

	LOG_DBG("server %p PUT req (final=%d)", server, final);

	if (server->cb && server->cb->push) {
		server->cb->push(server, final, buf);
	} else {
		(void)bt_opp_server_push_rsp(server, BT_OPP_RSP_CODE_NOT_IMPL, NULL);
	}
}

static void opp_server_obex_get_cb(struct bt_obex_server *obex_server, bool final,
				    struct net_buf *buf)
{
	struct bt_opp_server *server =
		CONTAINER_OF(obex_server, struct bt_opp_server, _server);
	const uint8_t *name;
	uint16_t name_len = 0;

	LOG_DBG("server %p GET req (final=%d)", server, final);

	/*
	 * OPP Spec section 5.6: the Name header must be absent or empty when
	 * getting the Default Get Object (owner's business card).  If the
	 * client sends a non-empty Name header, respond with FORBIDDEN and do
	 * not invoke the application callback.
	 *
	 * An empty Name header is encoded as a 2-byte UTF-16BE null terminator
	 * (0x00 0x00).  A name_len of 2 therefore means empty and must be
	 * treated the same as absent.  Only reject if actual name content
	 * exists beyond the null terminator (name_len > 2).
	 */
	if (bt_obex_get_header_name(buf, &name_len, &name) == 0 && name_len > 2) {
		LOG_DBG("server %p GET with non-empty Name header, responding FORBIDDEN", server);
		(void)bt_opp_server_pull_bcard_rsp(server, BT_OPP_RSP_CODE_FORBIDDEN, NULL);
		return;
	}

	if (server->cb && server->cb->pull_bcard) {
		server->cb->pull_bcard(server, buf);
	} else {
		(void)bt_opp_server_pull_bcard_rsp(server, BT_OPP_RSP_CODE_NOT_IMPL, NULL);
	}
}

static void opp_server_obex_abort_cb(struct bt_obex_server *obex_server, struct net_buf *buf)
{
	struct bt_opp_server *server =
		CONTAINER_OF(obex_server, struct bt_opp_server, _server);

	LOG_DBG("server %p ABORT req", server);

	if (server->cb && server->cb->abort) {
		server->cb->abort(server, buf);
	} else {
		(void)bt_opp_server_abort_rsp(server, BT_OPP_RSP_CODE_SUCCESS, NULL);
	}
}

static int opp_rfcomm_goep_accept(struct bt_conn *conn,
				  struct bt_goep_transport_rfcomm_server *rfcomm_srv,
				  struct bt_goep **goep)
{
	struct bt_opp_server_rfcomm *opp_rfcomm =
		CONTAINER_OF(rfcomm_srv, struct bt_opp_server_rfcomm, server);
	struct bt_opp_server *opp_server = NULL;
	int err;

	err = opp_rfcomm->accept(conn, opp_rfcomm, &opp_server);
	if (err != 0 || opp_server == NULL) {
		return err != 0 ? err : -ENOMEM;
	}

	/* Initialize GOEP v1 transport for this connection. */
	BT_GOEP_INIT_V1(&opp_server->_goep, &opp_server->_goep_transport.v1);
	opp_server->_goep.transport_ops = &opp_server_goep_transport_ops;
	atomic_set(&opp_server->_transport_state, BT_OPP_TRANSPORT_STATE_CONNECTING);

	*goep = &opp_server->_goep;

	return 0;
}

int bt_opp_server_rfcomm_register(struct bt_opp_server_rfcomm *server)
{
	if (server == NULL || server->accept == NULL) {
		return -EINVAL;
	}

	server->server.rfcomm.accept = NULL; /* replaced by GOEP accept bridge */
	server->server.accept = opp_rfcomm_goep_accept;

	return bt_goep_transport_rfcomm_server_register(&server->server);
}

int bt_opp_server_register(struct bt_opp_server *server, const struct bt_opp_server_cb *cb)
{
	if (server == NULL || cb == NULL) {
		return -EINVAL;
	}

	server->cb = cb;
	atomic_set(&server->_state, BT_OPP_STATE_DISCONNECTED);
	atomic_set(&server->_flags, 0);

	server->_server.obex = &server->_goep.obex;
	server->_server.ops = &opp_server_obex_ops;

	/* OPP does not use a Target UUID (spec section 5.4). */
	return bt_obex_server_register(&server->_server, NULL);
}

struct net_buf *bt_opp_server_create_pdu(struct bt_opp_server *server,
					 struct net_buf_pool *pool)
{
	if (server == NULL) {
		return NULL;
	}

	return bt_goep_create_pdu(&server->_goep, pool != NULL ? pool : &bt_opp_pool);
}

int bt_opp_server_connect_rsp(struct bt_opp_server *server, uint16_t mopl,
			      enum bt_opp_rsp_code rsp_code, struct net_buf *buf)
{
	int err;

	if (server == NULL) {
		return -EINVAL;
	}

	err = bt_obex_connect_rsp(&server->_server, (uint8_t)rsp_code, mopl, buf);
	if (err == 0) {
		/*
		 * BT_OPP_RSP_CODE_OK and BT_OPP_RSP_CODE_SUCCESS share the same
		 * numeric value; a single check is sufficient.
		 */
		if (rsp_code == BT_OPP_RSP_CODE_SUCCESS) {
			atomic_set(&server->_state, BT_OPP_STATE_CONNECTED);
		} else {
			atomic_set(&server->_state, BT_OPP_STATE_DISCONNECTED);
		}
	}

	return err;
}

int bt_opp_server_disconnect_rsp(struct bt_opp_server *server, enum bt_opp_rsp_code rsp_code,
				 struct net_buf *buf)
{
	int err;

	if (server == NULL) {
		return -EINVAL;
	}

	err = bt_obex_disconnect_rsp(&server->_server, (uint8_t)rsp_code, buf);
	if (err == 0) {
		atomic_set(&server->_state, BT_OPP_STATE_DISCONNECTED);
	}

	return err;
}

int bt_opp_server_push_rsp(struct bt_opp_server *server, enum bt_opp_rsp_code rsp_code,
			   struct net_buf *buf)
{
	if (server == NULL) {
		return -EINVAL;
	}

	return bt_obex_put_rsp(&server->_server, (uint8_t)rsp_code, buf);
}

int bt_opp_server_pull_bcard_rsp(struct bt_opp_server *server, enum bt_opp_rsp_code rsp_code,
				 struct net_buf *buf)
{
	if (server == NULL) {
		return -EINVAL;
	}

	return bt_obex_get_rsp(&server->_server, (uint8_t)rsp_code, buf);
}

int bt_opp_server_abort_rsp(struct bt_opp_server *server, enum bt_opp_rsp_code rsp_code,
			    struct net_buf *buf)
{
	if (server == NULL) {
		return -EINVAL;
	}

	return bt_obex_abort_rsp(&server->_server, (uint8_t)rsp_code, buf);
}

#endif /* CONFIG_BT_OPP_SERVER */
