/* opp.c - Bluetooth Object Push Profile handling */

/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <strings.h>
#include <zephyr/kernel.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/bluetooth/classic/rfcomm.h>
#include <zephyr/bluetooth/classic/goep.h>
#include <zephyr/bluetooth/classic/opp.h>

#include "opp_internal.h"

#define LOG_LEVEL CONFIG_BT_OPP_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_opp);

/* Buffer pool: two buffers per ACL connection (one for operation, one for response). */
#define OPP_POOL_BUF_SIZE BT_RFCOMM_BUF_SIZE(CONFIG_BT_GOEP_RFCOMM_MTU)

NET_BUF_POOL_FIXED_DEFINE(bt_opp_pool, 2 * CONFIG_BT_MAX_CONN, OPP_POOL_BUF_SIZE,
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

/*
 * OPP spec Table 5.2 and Table 5.3 mark these headers as excluded (X): a unit
 * may support them but shall not use them. SRM and the reliable session
 * headers are IrOBEX 1.5 features that the OPP ICS allows only for v1.2, and
 * OPP has no operation for a reliable session.
 */
static const uint8_t opp_excluded_hdrs[] = {
	BT_OBEX_HEADER_ID_COUNT,
	BT_OBEX_HEADER_ID_TARGET,
	BT_OBEX_HEADER_ID_WHO,
	BT_OBEX_HEADER_ID_CONN_ID,
	BT_OBEX_HEADER_ID_APP_PARAM,
	BT_OBEX_HEADER_ID_AUTH_CHALLENGE,
	BT_OBEX_HEADER_ID_AUTH_RSP,
	BT_OBEX_HEADER_ID_OBJECT_CLASS,
	BT_OBEX_HEADER_ID_SRM,
	BT_OBEX_HEADER_ID_SRM_PARAM,
	BT_OBEX_HEADER_ID_SESSION_PARAM,
	BT_OBEX_HEADER_ID_SESSION_SEQ_NUM,
};

/* Reject a buffer the application built with a header OPP excludes. */
static bool opp_has_excluded_header(struct net_buf *buf)
{
	if (buf == NULL) {
		return false;
	}

	ARRAY_FOR_EACH(opp_excluded_hdrs, i) {
		if (bt_obex_has_header(buf, opp_excluded_hdrs[i])) {
			LOG_ERR("OBEX header (0x%02x) is not used by OPP", opp_excluded_hdrs[i]);
			return true;
		}
	}

	return false;
}

static bool opp_type_is_vcard(uint16_t len, const uint8_t *type)
{
	const uint8_t *sep;
	uint16_t media_len;

	if (len == 0U) {
		return false;
	}

	/*
	 * IrOBEX 1.2 section 2.2.3: compared case insensitively. The value may carry a
	 * terminator or MIME parameters, so only the media type is compared.
	 */
	sep = memchr(type, '\0', len);
	media_len = (sep != NULL) ? (uint16_t)(sep - type) : len;

	sep = memchr(type, ';', media_len);
	if (sep != NULL) {
		media_len = (uint16_t)(sep - type);
	}

	if (media_len != strlen(BT_OPP_TYPE_VCARD)) {
		return false;
	}

	return strncasecmp((const char *)type, BT_OPP_TYPE_VCARD, media_len) == 0;
}

/*
 * Check a GET request against the only object a Push Server serves, the
 * Default Get Object. Returns 0 when it addresses that object, or the response
 * code that rejects it.
 */
static uint8_t opp_pull_req_check(struct net_buf *buf)
{
	const uint8_t *name;
	const uint8_t *type;
	uint16_t name_len = 0;
	uint16_t type_len = 0;

	/*
	 * IrOBEX 1.2 section 8.4: the Default Get Object is requested with an
	 * empty Name header, which section 2.2.2 makes at most the 2-byte
	 * terminator. Only a longer name addresses an object, which OPP spec
	 * section 5.6 answers with FORBIDDEN.
	 */
	if (bt_obex_get_header_name(buf, &name_len, &name) == 0 && name_len > 2) {
		LOG_WRN("GET names an object, which OPP does not support");
		return BT_OPP_RSP_CODE_FORBIDDEN;
	}

	/*
	 * A continuation request carries no headers, and IrOBEX 1.2 section
	 * 3.3.4 lets a server with a single object serve a request that does not
	 * describe it; bt_opp_client_pull() requires the header on the first
	 * one. Section 8.4 answers a requested type the server does not have
	 * with NOT FOUND, which is every type but vCard here.
	 */
	if (bt_obex_get_header_type(buf, &type_len, &type) == 0 &&
	    !opp_type_is_vcard(type_len, type)) {
		LOG_WRN("GET asks for an object format other than vCard");
		return BT_OPP_RSP_CODE_NOT_FOUND;
	}

	return 0;
}

#if defined(CONFIG_BT_OPP_CLIENT)

/*
 * Check the first PUT request of a push for the Name and Length headers.
 * Returns 0 when both are present, or BT_OPP_RSP_CODE_BAD_REQ when one is
 * missing; their values are the Push Server's to validate.
 */
static uint8_t opp_push_first_req_check(struct net_buf *buf)
{
	/*
	 * OPP spec Table 5.2 makes Name and Length mandatory for the Push
	 * Client; GOEP section 5.5.1 puts Name in the first request, and
	 * IrOBEX 1.2 section 3.3.3.1 recommends the headers describing the
	 * object precede its body. Later requests do not repeat them.
	 */
	if (!bt_obex_has_header(buf, BT_OBEX_HEADER_ID_NAME)) {
		LOG_ERR("PUT is missing the OBEX header (Name)");
		return BT_OPP_RSP_CODE_BAD_REQ;
	}

	if (!bt_obex_has_header(buf, BT_OBEX_HEADER_ID_LEN)) {
		LOG_ERR("PUT is missing the OBEX header (Length)");
		return BT_OPP_RSP_CODE_BAD_REQ;
	}

	return 0;
}

/* GOEP transport ops for the client */
static void opp_client_transport_connected(struct bt_conn *conn, struct bt_goep *goep)
{
	struct bt_opp_client *client = CONTAINER_OF(goep, struct bt_opp_client, _goep);

	atomic_set(&client->_transport_state, BT_OPP_TRANSPORT_STATE_CONNECTED);

	LOG_DBG("client %p transport connected", client);

	client->cb->transport_connected(conn, client);
}

static void opp_client_transport_disconnected(struct bt_goep *goep)
{
	struct bt_opp_client *client = CONTAINER_OF(goep, struct bt_opp_client, _goep);

	atomic_set(&client->_transport_state, BT_OPP_TRANSPORT_STATE_DISCONNECTED);
	atomic_clear(&client->_push_state);
	atomic_clear(&client->_pull_state);

	LOG_DBG("client %p transport disconnected", client);

	client->cb->transport_disconnected(client);
}

static const struct bt_goep_transport_ops opp_client_goep_transport_ops = {
	.connected    = opp_client_transport_connected,
	.disconnected = opp_client_transport_disconnected,
};

/* OBEX client callback implementations */
static void opp_client_obex_connect_cb(struct bt_obex_client *obex_client, uint8_t rsp_code,
				       uint8_t version, uint16_t mopl, struct net_buf *buf)
{
	struct bt_opp_client *client = CONTAINER_OF(obex_client, struct bt_opp_client, _client);

	LOG_DBG("client %p OBEX connect rsp 0x%02x version 0x%02x mopl %u", client, rsp_code,
		version, mopl);

	client->cb->connect(client, rsp_code, version, mopl, buf);
}

static void opp_client_obex_disconnect_cb(struct bt_obex_client *obex_client, uint8_t rsp_code,
					  struct net_buf *buf)
{
	struct bt_opp_client *client = CONTAINER_OF(obex_client, struct bt_opp_client, _client);
	int err;

	if (rsp_code == BT_OBEX_RSP_CODE_SUCCESS) {
		atomic_clear(&client->_push_state);
		atomic_clear(&client->_pull_state);
	} else {
		/*
		 * IrOBEX 1.2 section 3.3.2: a disconnect may not be refused. The
		 * OBEX layer keeps its connection on any other response code, so
		 * close the transport below instead.
		 */
		LOG_WRN("client %p disconnect refused with rsp 0x%02x", client, rsp_code);
	}

	LOG_DBG("client %p OBEX disconnect rsp 0x%02x", client, rsp_code);

	client->cb->disconnect(client, rsp_code, buf);

	if (rsp_code != BT_OBEX_RSP_CODE_SUCCESS) {
		err = bt_opp_client_transport_disconnect(client);
		if (err != 0) {
			LOG_WRN("client %p failed to close the transport (err %d)", client, err);
		}
	}
}

static void opp_client_obex_put_cb(struct bt_obex_client *obex_client, uint8_t rsp_code,
				   struct net_buf *buf)
{
	struct bt_opp_client *client = CONTAINER_OF(obex_client, struct bt_opp_client, _client);

	LOG_DBG("client %p PUT rsp 0x%02x", client, rsp_code);

	/* CONTINUE continues the push; any other response code ends it. */
	if (rsp_code != BT_OBEX_RSP_CODE_CONTINUE) {
		atomic_clear(&client->_push_state);
	}

	client->cb->push(client, rsp_code, buf);
}

static void opp_client_obex_get_cb(struct bt_obex_client *obex_client, uint8_t rsp_code,
				   struct net_buf *buf)
{
	struct bt_opp_client *client = CONTAINER_OF(obex_client, struct bt_opp_client, _client);

	LOG_DBG("client %p GET rsp 0x%02x", client, rsp_code);

	/* CONTINUE continues the pull; any other response code ends it. */
	if (rsp_code != BT_OBEX_RSP_CODE_CONTINUE) {
		atomic_clear(&client->_pull_state);
	}

	if (client->cb->pull != NULL) {
		client->cb->pull(client, rsp_code, buf);
	}
}

static void opp_client_obex_abort_cb(struct bt_obex_client *obex_client, uint8_t rsp_code,
				     struct net_buf *buf)
{
	struct bt_opp_client *client = CONTAINER_OF(obex_client, struct bt_opp_client, _client);

	LOG_DBG("client %p ABORT rsp 0x%02x", client, rsp_code);

	/*
	 * Only a SUCCESS abort response reaches this callback; any other code
	 * makes the OBEX layer close the transport and reset the state.
	 */
	atomic_clear(&client->_push_state);
	atomic_clear(&client->_pull_state);

	client->cb->abort(client, buf);
}

static const struct bt_obex_client_ops opp_client_obex_ops = {
	.connect    = opp_client_obex_connect_cb,
	.disconnect = opp_client_obex_disconnect_cb,
	.put        = opp_client_obex_put_cb,
	.get        = opp_client_obex_get_cb,
	.abort      = opp_client_obex_abort_cb,
};

int bt_opp_client_transport_connect(struct bt_conn *conn, struct bt_opp_client *client,
				    const struct bt_opp_client_cb *cb, uint8_t channel)
{
	int err;

	if (conn == NULL || client == NULL || cb == NULL) {
		return -EINVAL;
	}

	/*
	 * The transport callbacks report the transport coming up or going away,
	 * so they are required. Get is optional, so its callback is checked in
	 * bt_opp_client_pull().
	 */
	if (cb->transport_connected == NULL || cb->transport_disconnected == NULL ||
	    cb->connect == NULL || cb->disconnect == NULL || cb->push == NULL ||
	    cb->abort == NULL) {
		LOG_ERR("Mandatory callback is missing");
		return -EINVAL;
	}

	if (!atomic_cas(&client->_transport_state, BT_OPP_TRANSPORT_STATE_DISCONNECTED,
			BT_OPP_TRANSPORT_STATE_CONNECTING)) {
		return -EALREADY;
	}

	client->cb = cb;
	atomic_clear(&client->_push_state);
	atomic_clear(&client->_pull_state);

	BT_GOEP_INIT_V1(&client->_goep, &client->_transport);
	client->_goep.transport_ops = &opp_client_goep_transport_ops;
	client->_client.ops = &opp_client_obex_ops;
	client->_client.obex = &client->_goep.obex;

	err = bt_goep_transport_rfcomm_connect(conn, &client->_goep, channel);
	if (err != 0) {
		atomic_set(&client->_transport_state, BT_OPP_TRANSPORT_STATE_DISCONNECTED);
	}

	return err;
}

int bt_opp_client_transport_disconnect(struct bt_opp_client *client)
{
	int err;

	if (client == NULL) {
		return -EINVAL;
	}

	if (!atomic_cas(&client->_transport_state, BT_OPP_TRANSPORT_STATE_CONNECTED,
			BT_OPP_TRANSPORT_STATE_DISCONNECTING)) {
		return -ENOTCONN;
	}

	err = bt_goep_transport_rfcomm_disconnect(&client->_goep);
	if (err != 0) {
		/* The peer may have torn the transport down already. */
		(void)atomic_cas(&client->_transport_state, BT_OPP_TRANSPORT_STATE_DISCONNECTING,
				 BT_OPP_TRANSPORT_STATE_CONNECTED);
	}

	return err;
}

struct net_buf *bt_opp_client_create_pdu(struct bt_opp_client *client, struct net_buf_pool *pool)
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

	/* OPP spec section 5.4 and Table 5.2: the Target header is not used. */
	if (opp_has_excluded_header(buf)) {
		return -EINVAL;
	}

	return bt_obex_connect(&client->_client, mopl, buf);
}

int bt_opp_client_disconnect(struct bt_opp_client *client, struct net_buf *buf)
{
	if (client == NULL) {
		return -EINVAL;
	}

	if (opp_has_excluded_header(buf)) {
		return -EINVAL;
	}

	return bt_obex_disconnect(&client->_client, buf);
}

int bt_opp_client_push(struct bt_opp_client *client, bool final, struct net_buf *buf)
{
	bool first;
	int err;

	if (client == NULL || buf == NULL || client->cb == NULL) {
		return -EINVAL;
	}

	if (opp_has_excluded_header(buf)) {
		return -EINVAL;
	}

	/*
	 * Publish the state before sending: the RX thread may handle the response
	 * before bt_obex_put() returns. This also tells the first PUT of a push
	 * from a continuation.
	 */
	first = !atomic_test_and_set_bit(&client->_push_state, BT_OPP_PUSH_ONGOING);

	if (first && opp_push_first_req_check(buf) != 0) {
		atomic_clear_bit(&client->_push_state, BT_OPP_PUSH_ONGOING);
		return -EINVAL;
	}

	/*
	 * OPP spec Table 5.2 makes End-of-Body mandatory; without it the server
	 * cannot tell the object is complete, and IrOBEX 1.2 section 3.3.3.6
	 * treats a PUT without a body as a delete request.
	 */
	if (final && !bt_obex_has_header(buf, BT_OBEX_HEADER_ID_END_BODY)) {
		LOG_ERR("OBEX header (End of Body) is missing");
		if (first) {
			atomic_clear_bit(&client->_push_state, BT_OPP_PUSH_ONGOING);
		}

		return -ENODATA;
	}

	err = bt_obex_put(&client->_client, final, buf);
	if (err != 0 && first) {
		/* Undo the published operation if nothing was sent. */
		atomic_clear_bit(&client->_push_state, BT_OPP_PUSH_ONGOING);
	}

	return err;
}

int bt_opp_client_pull(struct bt_opp_client *client, struct net_buf *buf)
{
	bool first;
	int err;

	if (client == NULL || buf == NULL || client->cb == NULL) {
		return -EINVAL;
	}

	if (client->cb->pull == NULL) {
		LOG_ERR("client %p cannot handle the pull response", client);
		return -ENOTSUP;
	}

	if (opp_has_excluded_header(buf)) {
		return -EINVAL;
	}

	/* Publish the state before sending, see bt_opp_client_push(). */
	first = !atomic_test_and_set_bit(&client->_pull_state, BT_OPP_PULL_ONGOING);

	/*
	 * OPP spec section 5.6 and IrOBEX 1.2 section 8.4: the first request of
	 * a pull carries the Type header; a continuation carries no headers.
	 */
	if (first && !bt_obex_has_header(buf, BT_OBEX_HEADER_ID_TYPE)) {
		LOG_ERR("OBEX header (Type) is missing");
		atomic_clear_bit(&client->_pull_state, BT_OPP_PULL_ONGOING);
		return -ENODATA;
	}

	/* The Default Get Object is the only object a Push Server serves. */
	if (opp_pull_req_check(buf) != 0) {
		if (first) {
			atomic_clear_bit(&client->_pull_state, BT_OPP_PULL_ONGOING);
		}

		return -EINVAL;
	}

	/*
	 * IrOBEX 1.2 section 3.3.4 requires the Final bit on a continuation
	 * request; the first request completes its headers and sets it too.
	 */
	err = bt_obex_get(&client->_client, true, buf);
	if (err != 0 && first) {
		atomic_clear_bit(&client->_pull_state, BT_OPP_PULL_ONGOING);
	}

	return err;
}

int bt_opp_client_abort(struct bt_opp_client *client, struct net_buf *buf)
{
	if (client == NULL || client->cb == NULL) {
		return -EINVAL;
	}

	if (opp_has_excluded_header(buf)) {
		return -EINVAL;
	}

	return bt_obex_abort(&client->_client, buf);
}

#endif /* CONFIG_BT_OPP_CLIENT */

#if defined(CONFIG_BT_OPP_SERVER)

/*
 * Check the Name header of a PUT request and report whether it names the
 * object. Returns 0 when the header is absent or valid, or the response code
 * that rejects the request.
 */
static uint8_t opp_push_name_check(struct net_buf *buf, bool *named)
{
	static const char path_chars[] = ":/\\";
	const uint8_t *name;
	uint16_t len = 0;

	*named = false;

	if (bt_obex_get_header_name(buf, &len, &name) != 0) {
		return 0;
	}

	/*
	 * OPP spec Table 5.2: a Name header has to hold a name, and IrOBEX 1.2
	 * section 2.2.2 makes an empty one carry at most the 2-byte terminator.
	 * Section 10.6.2 (HC5, HS5) requires a received Unicode header to be
	 * verified, which bt_obex_string_is_valid() does.
	 */
	if (len <= 2U || !bt_obex_string_is_valid(BT_OBEX_HEADER_ID_NAME, len, name)) {
		LOG_WRN("PUT carries an empty or malformed OBEX header (Name)");
		return BT_OPP_RSP_CODE_BAD_REQ;
	}

	/*
	 * IrOBEX 1.2 section 4.3: reject a name that holds a path character. The
	 * name is UTF-16BE (section 2.2.2), so a path character is a code unit
	 * 0x00 followed by ':', '/' or '\'.
	 */
	for (uint16_t i = 0; (i + 1U) < len; i += 2U) {
		if (name[i] != 0U) {
			continue;
		}

		if (memchr(path_chars, name[i + 1U], sizeof(path_chars) - 1U) != NULL) {
			LOG_WRN("PUT names a path, which the inbox does not accept");
			return BT_OPP_RSP_CODE_BAD_REQ;
		}
	}

	*named = true;

	return 0;
}

/* GOEP transport ops for the server */
static void opp_server_transport_connected(struct bt_conn *conn, struct bt_goep *goep)
{
	struct bt_opp_server *server = CONTAINER_OF(goep, struct bt_opp_server, _goep);

	atomic_set(&server->_transport_state, BT_OPP_TRANSPORT_STATE_CONNECTED);

	LOG_DBG("server %p transport connected", server);

	if (server->cb->transport_connected != NULL) {
		server->cb->transport_connected(conn, server);
	}
}

static void opp_server_transport_disconnected(struct bt_goep *goep)
{
	struct bt_opp_server *server = CONTAINER_OF(goep, struct bt_opp_server, _goep);

	atomic_set(&server->_transport_state, BT_OPP_TRANSPORT_STATE_DISCONNECTED);
	atomic_clear(&server->_push_state);
	atomic_clear(&server->_pull_state);

	LOG_DBG("server %p transport disconnected", server);

	server->cb->transport_disconnected(server);
}

static const struct bt_goep_transport_ops opp_server_goep_transport_ops = {
	.connected    = opp_server_transport_connected,
	.disconnected = opp_server_transport_disconnected,
};

/* OBEX server callback implementations */
static void opp_server_obex_connect_cb(struct bt_obex_server *obex_server, uint8_t version,
				       uint16_t mopl, struct net_buf *buf)
{
	struct bt_opp_server *server = CONTAINER_OF(obex_server, struct bt_opp_server, _server);
	int err;

	LOG_DBG("server %p OBEX connect req version 0x%02x mopl %u", server, version, mopl);

	/*
	 * OPP spec section 5.4 and Table 5.2: the Target header is not used, and
	 * accepting it would make the response carry the Who and Connection ID
	 * headers, which OPP excludes as well.
	 */
	if (bt_obex_has_header(buf, BT_OBEX_HEADER_ID_TARGET)) {
		LOG_WRN("server %p connect req carries a Target header", server);
		err = bt_opp_server_connect(server, mopl, BT_OPP_RSP_CODE_FORBIDDEN, NULL);
		if (err != 0) {
			LOG_WRN("server %p failed to reject the connect request (err %d)", server,
				err);
		}

		return;
	}

	server->cb->connect(server, version, mopl, buf);
}

static void opp_server_obex_disconnect_cb(struct bt_obex_server *obex_server, struct net_buf *buf)
{
	struct bt_opp_server *server = CONTAINER_OF(obex_server, struct bt_opp_server, _server);

	LOG_DBG("server %p OBEX disconnect req", server);

	server->cb->disconnect(server, buf);
}

/*
 * Track the headers of the push operation and check the request against the
 * profile. Returns 0 when acceptable, or the response code that rejects it.
 */
static uint8_t opp_push_req_check(struct bt_opp_server *server, bool final, struct net_buf *buf)
{
	uint8_t rsp_code;
	bool named;

	rsp_code = opp_push_name_check(buf, &named);
	if (rsp_code != 0) {
		return rsp_code;
	}

	if (named) {
		atomic_set_bit(&server->_push_state, BT_OPP_PUSH_NAME);
	}

	if (bt_obex_has_header(buf, BT_OBEX_HEADER_ID_BODY) ||
	    bt_obex_has_header(buf, BT_OBEX_HEADER_ID_END_BODY)) {
		/*
		 * OPP spec Table 5.2 makes Name mandatory; IrOBEX 1.2 section
		 * 3.3.3.1 only requires it before the object body. Length is
		 * not enforced: section 2.2.4 makes it optional for a sender
		 * that does not know the size, and End-of-Body marks the end.
		 */
		if (!atomic_test_bit(&server->_push_state, BT_OPP_PUSH_NAME)) {
			LOG_WRN("PUT sends the object body before the OBEX header (Name)");
			return BT_OPP_RSP_CODE_BAD_REQ;
		}

		atomic_set_bit(&server->_push_state, BT_OPP_PUSH_BODY);
	}

	/*
	 * IrOBEX 1.2 section 3.3.3.6: a final PUT without a body is a delete
	 * request, which OPP has no feature for (OPP spec Table 4.1).
	 */
	if (final && !atomic_test_bit(&server->_push_state, BT_OPP_PUSH_BODY)) {
		LOG_WRN("PUT carries no object body, which OPP does not support");
		return BT_OPP_RSP_CODE_FORBIDDEN;
	}

	return 0;
}

static void opp_server_obex_put_cb(struct bt_obex_server *obex_server, bool final,
				   struct net_buf *buf)
{
	struct bt_opp_server *server = CONTAINER_OF(obex_server, struct bt_opp_server, _server);
	bool ongoing = atomic_test_and_set_bit(&server->_push_state, BT_OPP_PUSH_ONGOING);
	uint8_t rsp_code;
	int err;

	LOG_DBG("server %p PUT req (final=%d)", server, final);

	/* A malformed push is rejected without invoking the application. */
	rsp_code = opp_push_req_check(server, final, buf);
	if (rsp_code != 0) {
		err = bt_opp_server_push(server, rsp_code, NULL);
		if (err != 0) {
			LOG_WRN("server %p failed to reject the PUT request (err %d)", server, err);
		}

		/*
		 * bt_opp_server_push() already cleared the state when it sent
		 * the response.
		 */
		if (!ongoing) {
			atomic_clear(&server->_push_state);
		}

		if (ongoing) {
			err = bt_goep_transport_rfcomm_disconnect(&server->_goep);
			if (err != 0) {
				LOG_WRN("server %p failed to disconnect after rejecting "
					"PUT (err %d)",
					server, err);
			}
		}

		return;
	}

	server->cb->push(server, final, buf);
}

static void opp_server_obex_get_cb(struct bt_obex_server *obex_server, bool final,
				   struct net_buf *buf)
{
	struct bt_opp_server *server = CONTAINER_OF(obex_server, struct bt_opp_server, _server);
	bool ongoing = atomic_test_and_set_bit(&server->_pull_state, BT_OPP_PULL_ONGOING);
	uint8_t rsp_code;
	int err;

	LOG_DBG("server %p GET req (final=%d)", server, final);

	/* Only the Default Get Object is served, so reject anything else. */
	rsp_code = opp_pull_req_check(buf);

	if (rsp_code != 0) {
		err = bt_opp_server_pull(server, rsp_code, NULL);
		if (err != 0) {
			LOG_WRN("server %p failed to reject the GET request (err %d)", server, err);
		}

		/*
		 * bt_opp_server_pull() already cleared the state when it sent
		 * the response.
		 */
		if (!ongoing) {
			atomic_clear(&server->_pull_state);
		}

		if (ongoing) {
			err = bt_goep_transport_rfcomm_disconnect(&server->_goep);
			if (err != 0) {
				LOG_WRN("server %p failed to disconnect after rejecting "
					"GET (err %d)",
					server, err);
			}
		}

		return;
	}

	server->cb->pull(server, final, buf);
}

static void opp_server_obex_abort_cb(struct bt_obex_server *obex_server, struct net_buf *buf)
{
	struct bt_opp_server *server = CONTAINER_OF(obex_server, struct bt_opp_server, _server);

	LOG_DBG("server %p ABORT req", server);

	/* The aborted operation is over, whatever the application answers. */
	atomic_clear(&server->_push_state);
	atomic_clear(&server->_pull_state);

	server->cb->abort(server, buf);
}

static const struct bt_obex_server_ops opp_server_obex_ops = {
	.connect    = opp_server_obex_connect_cb,
	.disconnect = opp_server_obex_disconnect_cb,
	.put        = opp_server_obex_put_cb,
	.get        = opp_server_obex_get_cb,
	.abort      = opp_server_obex_abort_cb,
};

static int opp_transport_goep_accept(struct bt_conn *conn,
				     struct bt_goep_transport_rfcomm_server *rfcomm_srv,
				     struct bt_goep **goep)
{
	struct bt_opp_server_transport *opp_transport =
		CONTAINER_OF(rfcomm_srv, struct bt_opp_server_transport, server);
	struct bt_opp_server *opp_server = NULL;
	int err;

	err = opp_transport->accept(conn, opp_transport, &opp_server);
	if (err != 0) {
		return err;
	}

	if (opp_server == NULL) {
		return -ENOMEM;
	}

	/*
	 * accept() must register the instance it hands back; rejecting the
	 * transport instead would leave it unused and likely leaked, so a
	 * missing registration is a coding error.
	 */
	__ASSERT(opp_server->cb != NULL, "OPP server %p is not registered", opp_server);

	/* Initialize GOEP v1 transport for this connection. */
	BT_GOEP_INIT_V1(&opp_server->_goep, &opp_server->_transport);
	opp_server->_goep.transport_ops = &opp_server_goep_transport_ops;
	atomic_set(&opp_server->_transport_state, BT_OPP_TRANSPORT_STATE_CONNECTING);

	*goep = &opp_server->_goep;

	return 0;
}

int bt_opp_server_transport_register(struct bt_opp_server_transport *server)
{
	if (server == NULL || server->accept == NULL) {
		return -EINVAL;
	}

	server->server.accept = opp_transport_goep_accept;

	return bt_goep_transport_rfcomm_server_register(&server->server);
}

int bt_opp_server_register(struct bt_opp_server *server, const struct bt_opp_server_cb *cb)
{
	int err;

	if (server == NULL || cb == NULL) {
		return -EINVAL;
	}

	/*
	 * transport_disconnected() is the only signal that the instance is free
	 * again, so it is required; accept() already reported the connection.
	 */
	if (cb->transport_disconnected == NULL || cb->connect == NULL ||
	    cb->disconnect == NULL || cb->push == NULL || cb->pull == NULL ||
	    cb->abort == NULL) {
		LOG_ERR("Mandatory callback is missing");
		return -EINVAL;
	}

	if (server->cb != NULL) {
		LOG_WRN("server %p is already registered", server);
		return -EALREADY;
	}

	server->cb = cb;
	atomic_clear(&server->_push_state);
	atomic_clear(&server->_pull_state);
	server->_server.obex = &server->_goep.obex;
	server->_server.ops = &opp_server_obex_ops;

	/* OPP does not use a Target UUID (spec section 5.4). */
	err = bt_obex_server_register(&server->_server, NULL);
	if (err != 0) {
		server->cb = NULL;
	}

	return err;
}

int bt_opp_server_unregister(struct bt_opp_server *server)
{
	int err;

	if (server == NULL) {
		return -EINVAL;
	}

	if (server->cb == NULL) {
		return -EALREADY;
	}

	if (atomic_get(&server->_transport_state) != BT_OPP_TRANSPORT_STATE_DISCONNECTED) {
		LOG_WRN("server %p is in use", server);
		return -EINPROGRESS;
	}

	err = bt_obex_server_unregister(&server->_server);
	if (err != 0) {
		return err;
	}

	server->cb = NULL;

	return 0;
}

struct net_buf *bt_opp_server_create_pdu(struct bt_opp_server *server, struct net_buf_pool *pool)
{
	if (server == NULL) {
		return NULL;
	}

	return bt_goep_create_pdu(&server->_goep, pool != NULL ? pool : &bt_opp_pool);
}

int bt_opp_server_connect(struct bt_opp_server *server, uint16_t mopl, uint8_t rsp_code,
			      struct net_buf *buf)
{
	if (server == NULL) {
		return -EINVAL;
	}

	if (opp_has_excluded_header(buf)) {
		return -EINVAL;
	}

	return bt_obex_connect_rsp(&server->_server, rsp_code, mopl, buf);
}

int bt_opp_server_disconnect(struct bt_opp_server *server, struct net_buf *buf)
{
	atomic_val_t prev_push_state;
	atomic_val_t prev_pull_state;
	int err;

	if (server == NULL) {
		return -EINVAL;
	}

	if (opp_has_excluded_header(buf)) {
		return -EINVAL;
	}

	/*
	 * IrDA Interoperability 2.0, section 2.2.2: the request cannot be
	 * refused, and Success is its only response.
	 *
	 * End the push or pull operation before sending the response. Restore it
	 * only if the response was not sent and the state is still cleared.
	 */
	prev_push_state = atomic_clear(&server->_push_state);
	prev_pull_state = atomic_clear(&server->_pull_state);

	err = bt_obex_disconnect_rsp(&server->_server, BT_OPP_RSP_CODE_SUCCESS, buf);
	if (err != 0) {
		(void)atomic_cas(&server->_push_state, 0, prev_push_state);
		(void)atomic_cas(&server->_pull_state, 0, prev_pull_state);
	}

	return err;
}

int bt_opp_server_push(struct bt_opp_server *server, uint8_t rsp_code, struct net_buf *buf)
{
	atomic_val_t prev = 0;
	bool ended;
	int err;

	if (server == NULL) {
		return -EINVAL;
	}

	if (opp_has_excluded_header(buf)) {
		return -EINVAL;
	}

	/*
	 * CONTINUE continues the push and any other code ends it. Publish that
	 * before sending: the RX thread may handle the next request before
	 * bt_obex_put_rsp() returns, and it belongs to a new operation.
	 */
	ended = rsp_code != BT_OPP_RSP_CODE_CONTINUE;
	if (ended) {
		prev = atomic_clear(&server->_push_state);
	}

	err = bt_obex_put_rsp(&server->_server, rsp_code, buf);
	if (err != 0) {
		/* Nothing was sent, so the operation is still ongoing. */
		if (ended) {
			(void)atomic_cas(&server->_push_state, 0, prev);
		}

		return err;
	}

	return 0;
}

int bt_opp_server_pull(struct bt_opp_server *server, uint8_t rsp_code, struct net_buf *buf)
{
	atomic_val_t prev = 0;
	bool ended;
	int err;

	if (server == NULL) {
		return -EINVAL;
	}

	if (opp_has_excluded_header(buf)) {
		return -EINVAL;
	}

	/*
	 * OPP spec Table 5.3 makes End-of-Body mandatory; per IrOBEX 1.2 section
	 * 2.2.9 it carries the last chunk of the object.
	 */
	if (rsp_code == BT_OPP_RSP_CODE_SUCCESS &&
	    !bt_obex_has_header(buf, BT_OBEX_HEADER_ID_END_BODY)) {
		LOG_ERR("OBEX header (End of Body) is missing");
		return -ENODATA;
	}

	ended = rsp_code != BT_OPP_RSP_CODE_CONTINUE;
	if (ended) {
		prev = atomic_clear(&server->_pull_state);
	}

	err = bt_obex_get_rsp(&server->_server, rsp_code, buf);
	if (err != 0 && ended) {
		(void)atomic_cas(&server->_pull_state, 0, prev);
	}

	return err;
}

int bt_opp_server_abort(struct bt_opp_server *server, struct net_buf *buf)
{
	if (server == NULL) {
		return -EINVAL;
	}

	if (opp_has_excluded_header(buf)) {
		return -EINVAL;
	}

	/*
	 * IrOBEX 1.2 section 3.3.5: the response to an abort is success; a client
	 * that gets anything else disconnects.
	 */
	return bt_obex_abort_rsp(&server->_server, BT_OPP_RSP_CODE_SUCCESS, buf);
}

#endif /* CONFIG_BT_OPP_SERVER */
