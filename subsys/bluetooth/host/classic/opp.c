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
 * OPP spec Table 5.2 and Table 5.3 mark these headers as excluded (X), a
 * capability that the unit may support but shall never use in the profile.
 *
 * Single Response Mode and its parameters are IrOBEX 1.5 features. The OPP ICS
 * only allows those for OPP v1.2 (with v1.2.1) and excludes them for OPP v1.1,
 * so this OPP v1.1 implementation does not use them either. The same holds for
 * the Session headers of a reliable session, which the profile has no
 * operation for.
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

/*
 * Check a buffer this device is about to send for a header the profile does not
 * use. An application builds the headers of a request or a response itself, so
 * this is what keeps it from putting OPP on the air with headers that OPP
 * excludes.
 */
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
	 * IrOBEX 1.2 section 2.2.3: the Type header holds null terminated ASCII
	 * text and its value is matched case insensitively, which is why
	 * section 8.4.1 can spell the same type "text/x-vCard". An OBEX byte
	 * sequence is not implicitly terminated though, and the value may carry
	 * MIME parameters, so only the media type is compared: the bytes up to
	 * the terminator, a ';' or the end of the header value, whichever comes
	 * first. That also bounds the comparison below to the header.
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
 * Check the headers of a GET request against the only object the Push Server
 * serves, the Default Get Object (owner's business card). Both roles share this
 * so that a request this client sends is one this server would accept.
 *
 * Returns 0 when the request addresses the Default Get Object, or the response
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
	 * empty Name header, and an empty Name header is treated the same as no
	 * Name header. Per section 2.2.2 an empty Name header carries no value,
	 * so name_len is 0. A peer that sends the UTF-16BE null terminator on
	 * its own gives name_len 2 instead. Both mean empty and are treated the
	 * same as absent, so only a longer value counts as a real name. Naming
	 * an object is answered with FORBIDDEN by both section 8.4 and OPP spec
	 * section 5.6.
	 */
	if (bt_obex_get_header_name(buf, &name_len, &name) == 0 && name_len > 2) {
		LOG_WRN("GET names an object, which OPP does not support");
		return BT_OPP_RSP_CODE_FORBIDDEN;
	}

	/*
	 * An absent Type header is accepted here for two reasons: a continuation
	 * request of a pull operation carries no headers at all, and per IrOBEX
	 * 1.2 section 3.3.4 a server with a single object may serve it to a
	 * request that does not describe it. The first request this client sends
	 * does have to carry the header, which bt_opp_client_pull() enforces.
	 *
	 * IrOBEX 1.2 section 8.4: a default object of the requested type that
	 * the server does not have is answered with NOT FOUND, which is what
	 * any object format other than vCard amounts to for a Push Server.
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
 * Check the headers of the first PUT request this client sends in a push
 * operation.
 *
 * Returns 0 when the request carries the Name and Length headers, or
 * BT_OPP_RSP_CODE_BAD_REQ when one of them is missing. Only their presence is
 * checked, not their value: an empty name, or one holding a path character
 * that IrOBEX 1.2 section 4.3 does not accept for an inbox object, passes and
 * is left to the Push Server to reject.
 */
static uint8_t opp_push_first_req_check(struct net_buf *buf)
{
	/*
	 * OPP spec Table 5.2: the Name and Length headers of the Object Push
	 * feature are mandatory for the Push Client. GOEP section 5.5.1,
	 * Table 5.8, which OPP spec section 5.5 refers to, puts Name in the
	 * first request of the operation. This client requires Length there
	 * too, following the IrOBEX 1.2 section 3.3.3.1 recommendation that
	 * the headers describing the object precede its Body. Later requests
	 * do not repeat them.
	 */
	if (!bt_obex_has_header(buf, BT_OBEX_HEADER_ID_NAME)) {
		LOG_WRN("PUT is missing the OBEX header (Name)");
		return BT_OPP_RSP_CODE_BAD_REQ;
	}

	if (!bt_obex_has_header(buf, BT_OBEX_HEADER_ID_LEN)) {
		LOG_WRN("PUT is missing the OBEX header (Length)");
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

	if (client->cb->transport_connected != NULL) {
		client->cb->transport_connected(conn, client);
	}
}

static void opp_client_transport_disconnected(struct bt_goep *goep)
{
	struct bt_opp_client *client = CONTAINER_OF(goep, struct bt_opp_client, _goep);

	atomic_set(&client->_transport_state, BT_OPP_TRANSPORT_STATE_DISCONNECTED);
	atomic_set(&client->_state, BT_OPP_STATE_DISCONNECTED);
	atomic_set(&client->_push_state, BT_OPP_PUSH_IDLE);
	atomic_set(&client->_pull_state, BT_OPP_PULL_IDLE);

	LOG_DBG("client %p transport disconnected", client);

	if (client->cb->transport_disconnected != NULL) {
		client->cb->transport_disconnected(client);
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
	struct bt_opp_client *client = CONTAINER_OF(obex_client, struct bt_opp_client, _client);

	/*
	 * BT_OBEX_RSP_CODE_OK and BT_OBEX_RSP_CODE_SUCCESS share the same
	 * numeric value; a single check is sufficient.
	 */
	if (rsp_code == BT_OBEX_RSP_CODE_SUCCESS) {
		atomic_set(&client->_state, BT_OPP_STATE_CONNECTED);
	} else {
		atomic_set(&client->_state, BT_OPP_STATE_DISCONNECTED);
	}

	LOG_DBG("client %p OBEX connect rsp 0x%02x version 0x%02x mopl %u", client, rsp_code,
		version, mopl);

	if (client->cb->connect != NULL) {
		client->cb->connect(client, rsp_code, version, mopl, buf);
	}
}

static void opp_client_obex_disconnect_cb(struct bt_obex_client *obex_client, uint8_t rsp_code,
					  struct net_buf *buf)
{
	struct bt_opp_client *client = CONTAINER_OF(obex_client, struct bt_opp_client, _client);
	int err;

	if (rsp_code == BT_OBEX_RSP_CODE_SUCCESS) {
		atomic_set(&client->_state, BT_OPP_STATE_DISCONNECTED);
		atomic_set(&client->_push_state, BT_OPP_PUSH_IDLE);
		atomic_set(&client->_pull_state, BT_OPP_PULL_IDLE);
	} else {
		/*
		 * IrOBEX 1.2 section 3.3.2: a disconnect may not be refused.
		 * The OBEX layer leaves its own connection established on any
		 * response code other than success, so follow it here and close
		 * the transport below instead of carrying on with a peer that
		 * does not follow the protocol.
		 */
		LOG_WRN("client %p disconnect refused with rsp 0x%02x", client, rsp_code);
		atomic_set(&client->_state, BT_OPP_STATE_CONNECTED);
	}

	LOG_DBG("client %p OBEX disconnect rsp 0x%02x", client, rsp_code);

	if (client->cb->disconnect != NULL) {
		client->cb->disconnect(client, rsp_code, buf);
	}

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

	/*
	 * CONTINUE means the server expects the next packet of the same push
	 * operation. Any other response code ends the operation, so the next
	 * request has to describe a new object again.
	 */
	if (rsp_code != BT_OBEX_RSP_CODE_CONTINUE) {
		atomic_set(&client->_push_state, BT_OPP_PUSH_IDLE);
	}

	if (client->cb->push != NULL) {
		client->cb->push(client, rsp_code, buf);
	}
}

static void opp_client_obex_get_cb(struct bt_obex_client *obex_client, uint8_t rsp_code,
				   struct net_buf *buf)
{
	struct bt_opp_client *client = CONTAINER_OF(obex_client, struct bt_opp_client, _client);

	LOG_DBG("client %p GET rsp 0x%02x", client, rsp_code);

	/*
	 * CONTINUE means the server has more of the object to send. Any other
	 * response code ends the operation, so the next request has to address
	 * the Default Get Object again.
	 */
	if (rsp_code != BT_OBEX_RSP_CODE_CONTINUE) {
		atomic_set(&client->_pull_state, BT_OPP_PULL_IDLE);
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
	 * Only a SUCCESS abort response reaches this callback. On any other
	 * response code the OBEX layer closes the transport, as IrOBEX 1.2
	 * section 3.3.5 asks, and opp_client_transport_disconnected() resets
	 * the state instead.
	 */
	atomic_set(&client->_push_state, BT_OPP_PUSH_IDLE);
	atomic_set(&client->_pull_state, BT_OPP_PULL_IDLE);

	if (client->cb->abort != NULL) {
		client->cb->abort(client, rsp_code, buf);
	}
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
	 * The transport callbacks are the only way the application learns that
	 * the transport came up or went away, so they are required.
	 *
	 * Per the OPP Spec the Push Client shall support the Connect, Disconnect,
	 * Put and Abort operations, while Get is optional. The response callback
	 * of an optional operation is checked by the operation itself, see
	 * bt_opp_client_pull().
	 */
	if (cb->transport_connected == NULL || cb->transport_disconnected == NULL ||
	    cb->connect == NULL || cb->disconnect == NULL || cb->push == NULL ||
	    cb->abort == NULL) {
		LOG_WRN("Mandatory callback is missing");
		return -EINVAL;
	}

	if (!atomic_cas(&client->_transport_state, BT_OPP_TRANSPORT_STATE_DISCONNECTED,
			BT_OPP_TRANSPORT_STATE_CONNECTING)) {
		return -EALREADY;
	}

	client->cb = cb;
	atomic_set(&client->_state, BT_OPP_STATE_DISCONNECTED);
	atomic_set(&client->_push_state, BT_OPP_PUSH_IDLE);
	atomic_set(&client->_pull_state, BT_OPP_PULL_IDLE);

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
		/*
		 * Only undo the transition made above. The transport may have
		 * been torn down by the peer in the meantime, in which case the
		 * disconnected callback already published the final state.
		 */
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
	int err;

	if (client == NULL) {
		return -EINVAL;
	}

	if (atomic_get(&client->_transport_state) != BT_OPP_TRANSPORT_STATE_CONNECTED) {
		return -ENOTCONN;
	}

	/*
	 * OPP spec section 5.4 and Table 5.2: the Target header is not used, and
	 * neither is any of the other headers the profile excludes.
	 */
	if (opp_has_excluded_header(buf)) {
		return -EINVAL;
	}

	if (!atomic_cas(&client->_state, BT_OPP_STATE_DISCONNECTED, BT_OPP_STATE_CONNECTING)) {
		return -EALREADY;
	}

	err = bt_obex_connect(&client->_client, mopl, buf);
	if (err != 0) {
		(void)atomic_cas(&client->_state, BT_OPP_STATE_CONNECTING,
				 BT_OPP_STATE_DISCONNECTED);
	}

	return err;
}

int bt_opp_client_disconnect(struct bt_opp_client *client, struct net_buf *buf)
{
	int err;

	if (client == NULL) {
		return -EINVAL;
	}

	if (opp_has_excluded_header(buf)) {
		return -EINVAL;
	}

	if (!atomic_cas(&client->_state, BT_OPP_STATE_CONNECTED, BT_OPP_STATE_DISCONNECTING)) {
		return -ENOTCONN;
	}

	err = bt_obex_disconnect(&client->_client, buf);
	if (err != 0) {
		(void)atomic_cas(&client->_state, BT_OPP_STATE_DISCONNECTING,
				 BT_OPP_STATE_CONNECTED);
	}

	return err;
}

int bt_opp_client_push(struct bt_opp_client *client, bool final, struct net_buf *buf)
{
	atomic_val_t prev;
	atomic_val_t next;
	int err;

	if (client == NULL || buf == NULL || client->cb == NULL) {
		return -EINVAL;
	}

	if (atomic_get(&client->_state) != BT_OPP_STATE_CONNECTED) {
		return -ENOTCONN;
	}

	if (opp_has_excluded_header(buf)) {
		return -EINVAL;
	}

	prev = atomic_get(&client->_push_state);

	if ((prev & BIT(BT_OPP_PUSH_ONGOING)) == 0 && opp_push_first_req_check(buf) != 0) {
		return -ENODATA;
	}

	/*
	 * OPP spec Table 5.2 makes the End-of-Body header mandatory, and per
	 * IrOBEX 1.2 section 2.2.9 it is what signals the end of the object.
	 * Without it the server cannot tell that the object is complete, and
	 * per section 3.3.3.6 a push that carries no object body at all is a
	 * delete request instead.
	 */
	if (final && !bt_obex_has_header(buf, BT_OBEX_HEADER_ID_END_BODY)) {
		LOG_ERR("OBEX header (End of Body) is missing");
		return -ENODATA;
	}

	/*
	 * Publish the state before the request is sent. The response is handled
	 * in the RX thread, which may run before bt_obex_put() has returned to
	 * this thread, and it ends the operation by setting IDLE. Setting
	 * ONGOING afterwards would overwrite that and leave the operation
	 * ongoing forever, so that the first PUT of the next push would not be
	 * checked for the Name and Length headers.
	 */
	next = prev | BIT(BT_OPP_PUSH_ONGOING);
	atomic_set(&client->_push_state, next);

	err = bt_obex_put(&client->_client, final, buf);
	if (err != 0) {
		/*
		 * Nothing has been sent, so restore the state the operation was
		 * in. Only undo the transition made above: the transport may
		 * have been torn down in the meantime, in which case the
		 * disconnected callback already published the final state.
		 */
		(void)atomic_cas(&client->_push_state, next, prev);
	}

	return err;
}

int bt_opp_client_pull(struct bt_opp_client *client, struct net_buf *buf)
{
	atomic_val_t prev;
	atomic_val_t next;
	int err;

	if (client == NULL || buf == NULL || client->cb == NULL) {
		return -EINVAL;
	}

	if (atomic_get(&client->_state) != BT_OPP_STATE_CONNECTED) {
		return -ENOTCONN;
	}

	if (client->cb->pull == NULL) {
		LOG_WRN("client %p cannot handle the pull response", client);
		return -ENOTSUP;
	}

	if (opp_has_excluded_header(buf)) {
		return -EINVAL;
	}

	prev = atomic_get(&client->_pull_state);

	/*
	 * OPP spec section 5.6: the Push Client must use the Type header when
	 * getting the Default Get Object. IrOBEX 1.2 section 8.4 spells out the
	 * same procedure. Only the first request of a pull operation carries it,
	 * a continuation request carries no headers at all.
	 */
	if ((prev & BIT(BT_OPP_PULL_ONGOING)) == 0 &&
	    !bt_obex_has_header(buf, BT_OBEX_HEADER_ID_TYPE)) {
		LOG_ERR("OBEX header (Type) is missing");
		return -ENODATA;
	}

	/*
	 * The Default Get Object is the only object a Push Server serves, so a
	 * request addressing anything else would just be refused by the peer.
	 */
	if (opp_pull_req_check(buf) != 0) {
		return -EINVAL;
	}

	/* Publish the state before the request is sent, see bt_opp_client_push(). */
	next = prev | BIT(BT_OPP_PULL_ONGOING);
	atomic_set(&client->_pull_state, next);

	/*
	 * Every GET of a pull operation sets the Final bit: the first request
	 * carries all the request headers, which completes the request phase,
	 * and IrOBEX 1.2 section 3.3.4 requires the bit on every continuation
	 * request that follows.
	 */
	err = bt_obex_get(&client->_client, true, buf);
	if (err != 0) {
		(void)atomic_cas(&client->_pull_state, next, prev);
	}

	return err;
}

int bt_opp_client_abort(struct bt_opp_client *client, struct net_buf *buf)
{
	if (client == NULL || client->cb == NULL) {
		return -EINVAL;
	}

	if (atomic_get(&client->_state) != BT_OPP_STATE_CONNECTED) {
		return -ENOTCONN;
	}

	if (opp_has_excluded_header(buf)) {
		return -EINVAL;
	}

	return bt_obex_abort(&client->_client, buf);
}

#endif /* CONFIG_BT_OPP_CLIENT */

#if defined(CONFIG_BT_OPP_SERVER)

/*
 * IrOBEX 1.2 section 2.2.11: a Connection Id is only handed out in the response
 * to a connect request that carries a Target header, which OPP does not use
 * (OPP spec section 5.4). A Connection Id this server never assigned is
 * therefore invalid whatever its value, and the section recommends answering
 * such a request with "Service Unavailable". Section 3.3.2 names the same
 * response code for a disconnect request, and section 2.2.11 forbids the header
 * in a connect request outright.
 *
 * The OBEX layer routes a request other than CONNECT by its Connection Id alone
 * and answers "Not Found" itself when no server owns that id. It only sets the
 * _conn_id of a server that accepts a connect request with a Target header,
 * which OPP never does, so a PUT, GET or DISCONNECT only reaches this check
 * when its Connection Id happens to equal the initial _conn_id, 0 for a
 * zero-initialized instance. Any other value, 0xFFFFFFFF included, is answered
 * "Not Found" without OPP seeing it. CONNECT is not routed by Connection Id and
 * reaches this check with any value.
 */
static bool opp_req_has_conn_id(struct net_buf *buf)
{
	if (!bt_obex_has_header(buf, BT_OBEX_HEADER_ID_CONN_ID)) {
		return false;
	}

	LOG_WRN("Request carries a Connection Id, which OPP does not assign");

	return true;
}

/*
 * Check the Name header of a PUT request, and report whether the request names
 * the object.
 *
 * Returns 0 when the header is absent or names the object, or the response code
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
	 * OPP spec Table 5.2: the Name header names the object, so it has to
	 * hold a name. Per IrOBEX 1.2 section 2.2.2 an empty Name header carries
	 * no value, so len is 0; a peer that sends the UTF-16BE null terminator
	 * on its own gives len 2 instead. Both name no object.
	 *
	 * IrOBEX 1.2 section 10.6.2 (HC5, HS5) requires a received Unicode
	 * header to be verified null terminated, which bt_obex_string_is_valid()
	 * does along with the length and the surrogate pairs.
	 */
	if (len <= 2U || !bt_obex_string_is_valid(BT_OBEX_HEADER_ID_NAME, len, name)) {
		LOG_WRN("PUT carries an empty or malformed OBEX header (Name)");
		return BT_OPP_RSP_CODE_BAD_REQ;
	}

	/*
	 * IrOBEX 1.2 section 4.3: the name of an object pushed into the inbox
	 * should not contain a path character such as ':', '/' or '\', and an
	 * object with an improperly formed name should be rejected. The name is
	 * UTF-16BE (section 2.2.2), so a path character is a code unit whose
	 * high byte is zero and whose low byte is one of the three.
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
	atomic_set(&server->_state, BT_OPP_STATE_DISCONNECTED);
	atomic_set(&server->_push_state, BT_OPP_PUSH_IDLE);

	LOG_DBG("server %p transport disconnected", server);

	if (server->cb->transport_disconnected != NULL) {
		server->cb->transport_disconnected(server);
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
	struct bt_opp_server *server = CONTAINER_OF(obex_server, struct bt_opp_server, _server);
	int err;

	LOG_DBG("server %p OBEX connect req version 0x%02x mopl %u", server, version, mopl);

	/*
	 * IrOBEX 1.2 section 2.2.11: a Connection Id header in a connect
	 * request does not make sense and is forbidden.
	 */
	if (opp_req_has_conn_id(buf)) {
		err = bt_opp_server_connect_rsp(server, mopl, BT_OPP_RSP_CODE_FORBIDDEN, NULL);
		if (err != 0) {
			LOG_WRN("server %p failed to reject the connect request (err %d)", server,
				err);
		}

		return;
	}

	/*
	 * A connect request carrying a Target header asks for a service this
	 * server does not provide: OPP spec section 5.4 states that the header
	 * is not used, so the server is registered without a target UUID. Per
	 * IrOBEX 1.2 section 4.6 the server is encouraged to accept such a
	 * request anyway and to leave the decision to continue or abort with
	 * the client, which section 3.3.1.6 describes as a connection to the
	 * inbox service. The request is therefore reported to the application
	 * like any other.
	 *
	 * Note that accepting it with a success response is not possible as
	 * long as the OBEX layer requires that response to carry the Who and
	 * Connection ID headers, both of which OPP excludes (Table 5.2), so
	 * such a request can only be answered with an error response.
	 */
	if (bt_obex_has_header(buf, BT_OBEX_HEADER_ID_TARGET)) {
		LOG_WRN("server %p connect req carries a Target header", server);
	}

	/*
	 * Set CONNECTING to indicate the connection is pending application
	 * approval. The final state (CONNECTED or DISCONNECTED) is set by
	 * bt_opp_server_connect_rsp() once the application calls it.
	 */
	atomic_set(&server->_state, BT_OPP_STATE_CONNECTING);

	if (server->cb->connect != NULL) {
		server->cb->connect(server, version, mopl, buf);
	}
}

static void opp_server_obex_disconnect_cb(struct bt_obex_server *obex_server, struct net_buf *buf)
{
	struct bt_opp_server *server = CONTAINER_OF(obex_server, struct bt_opp_server, _server);
	int err;

	LOG_DBG("server %p OBEX disconnect req", server);

	/*
	 * IrOBEX 1.2 section 3.3.2: a disconnect may not be refused, but a
	 * disconnect request that carries invalid information such as an
	 * invalid Connection Id may be answered with "Service Unavailable".
	 * Only a Connection Id that the OBEX layer matched to this server gets
	 * here, see opp_req_has_conn_id().
	 */
	if (opp_req_has_conn_id(buf)) {
		err = bt_opp_server_disconnect_rsp(server, BT_OPP_RSP_CODE_UNAVAIL, NULL);
		if (err != 0) {
			LOG_WRN("server %p failed to reject the disconnect req (err %d)", server,
				err);
		}

		return;
	}

	if (server->cb->disconnect != NULL) {
		server->cb->disconnect(server, buf);
	}
}

/*
 * Track the headers of the push operation the Push Server is receiving and
 * check the request against the profile.
 *
 * Returns 0 when the request is acceptable, or the response code that rejects
 * it.
 */
static uint8_t opp_push_req_check(struct bt_opp_server *server, bool final, struct net_buf *buf)
{
	uint8_t rsp_code;
	bool named;

	if (opp_req_has_conn_id(buf)) {
		return BT_OPP_RSP_CODE_UNAVAIL;
	}

	atomic_set_bit(&server->_push_state, BT_OPP_PUSH_ONGOING);

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
		 * OPP spec Table 5.2: the Name header that names the object is
		 * mandatory. Per IrOBEX 1.2 section 3.3.3.1 it only has to
		 * precede the object body, so it may arrive in any request of
		 * the operation up to the one that starts the body. The Length
		 * header is not enforced: section 2.2.4 makes it optional for
		 * the sender that does not know the size in advance, and the
		 * End-of-Body header is what marks the end of the object.
		 */
		if (!atomic_test_bit(&server->_push_state, BT_OPP_PUSH_NAME)) {
			LOG_WRN("PUT sends the object body before the OBEX header (Name)");
			return BT_OPP_RSP_CODE_BAD_REQ;
		}

		atomic_set_bit(&server->_push_state, BT_OPP_PUSH_BODY);
	}

	/*
	 * IrOBEX 1.2 section 3.3.3.6: a PUT operation with no Body and no
	 * End-of-Body header whatsoever is a delete request. The Object Push
	 * profile has no delete feature (OPP spec Table 4.1), so refuse it
	 * instead of reporting an object the client never pushed.
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
	uint8_t rsp_code;
	int err;

	LOG_DBG("server %p PUT req (final=%d)", server, final);

	/*
	 * A push that carries object data before any of its requests has named
	 * the object, or that ends without any object data, is rejected here
	 * without invoking the application callback.
	 */
	rsp_code = opp_push_req_check(server, final, buf);
	if (rsp_code != 0) {
		err = bt_opp_server_push_rsp(server, rsp_code, NULL);
		if (err != 0) {
			LOG_WRN("server %p failed to reject the PUT request (err %d)", server, err);
		}

		return;
	}

	if (server->cb->push != NULL) {
		server->cb->push(server, final, buf);
	}
}

static void opp_server_obex_get_cb(struct bt_obex_server *obex_server, bool final,
				   struct net_buf *buf)
{
	struct bt_opp_server *server = CONTAINER_OF(obex_server, struct bt_opp_server, _server);
	uint8_t rsp_code;
	int err;

	LOG_DBG("server %p GET req (final=%d)", server, final);

	/*
	 * The Default Get Object is the only object the Push Server serves, so a
	 * request addressing anything else is rejected here without invoking the
	 * application callback.
	 */
	if (opp_req_has_conn_id(buf)) {
		rsp_code = BT_OPP_RSP_CODE_UNAVAIL;
	} else {
		rsp_code = opp_pull_req_check(buf);
	}

	if (rsp_code != 0) {
		err = bt_opp_server_pull_rsp(server, rsp_code, NULL);
		if (err != 0) {
			LOG_WRN("server %p failed to reject the GET request (err %d)", server, err);
		}

		return;
	}

	if (server->cb->pull != NULL) {
		server->cb->pull(server, final, buf);
	}
}

static void opp_server_obex_abort_cb(struct bt_obex_server *obex_server, struct net_buf *buf)
{
	struct bt_opp_server *server = CONTAINER_OF(obex_server, struct bt_opp_server, _server);

	LOG_DBG("server %p ABORT req", server);

	/* The aborted operation is over, whatever the application answers. */
	atomic_set(&server->_push_state, BT_OPP_PUSH_IDLE);

	if (server->cb->abort != NULL) {
		server->cb->abort(server, buf);
	}
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
	 * The accept callback is required to call bt_opp_server_register() on
	 * the instance it hands back, which is what installs the callbacks the
	 * request handling relies on. Rejecting the transport request here
	 * instead would leave the instance the application just allocated
	 * unused and most likely leaked, so treat a missing registration as the
	 * coding error it is.
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
	 * transport_disconnected() is the only signal the application gets that
	 * the instance is free again, so it is required. transport_connected()
	 * is not: accept() already told the application about the connection.
	 */
	if (cb->transport_disconnected == NULL || cb->connect == NULL ||
	    cb->disconnect == NULL || cb->push == NULL || cb->pull == NULL ||
	    cb->abort == NULL) {
		LOG_WRN("Mandatory callback is missing");
		return -EINVAL;
	}

	if (atomic_get(&server->_state) != BT_OPP_STATE_DISCONNECTED) {
		LOG_WRN("server %p is in use", server);
		return -EINPROGRESS;
	}

	if (server->cb != NULL) {
		LOG_WRN("server %p is already registered", server);
		return -EALREADY;
	}

	server->cb = cb;
	atomic_set(&server->_push_state, BT_OPP_PUSH_IDLE);
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

	if (atomic_get(&server->_state) != BT_OPP_STATE_DISCONNECTED) {
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

int bt_opp_server_connect_rsp(struct bt_opp_server *server, uint16_t mopl, uint8_t rsp_code,
			      struct net_buf *buf)
{
	int err;

	if (server == NULL) {
		return -EINVAL;
	}

	if (opp_has_excluded_header(buf)) {
		return -EINVAL;
	}

	err = bt_obex_connect_rsp(&server->_server, rsp_code, mopl, buf);
	if (err != 0) {
		return err;
	}

	/*
	 * BT_OPP_RSP_CODE_OK and BT_OPP_RSP_CODE_SUCCESS share the same
	 * numeric value; a single check is sufficient.
	 */
	if (rsp_code == BT_OPP_RSP_CODE_SUCCESS) {
		atomic_set(&server->_state, BT_OPP_STATE_CONNECTED);
	} else {
		atomic_set(&server->_state, BT_OPP_STATE_DISCONNECTED);
	}

	return 0;
}

int bt_opp_server_disconnect_rsp(struct bt_opp_server *server, uint8_t rsp_code,
				 struct net_buf *buf)
{
	int err;

	if (server == NULL) {
		return -EINVAL;
	}

	/*
	 * IrOBEX 1.2 section 3.3.2: a disconnect may not be refused. The only
	 * other response code the section allows is "Service Unavailable", for
	 * a request that carries invalid information. The application may send
	 * it, and opp_server_obex_disconnect_cb() sends it on its own for a
	 * Connection Id that the OBEX layer did not already answer with
	 * "Not Found", see opp_req_has_conn_id().
	 */
	if (rsp_code != BT_OPP_RSP_CODE_SUCCESS && rsp_code != BT_OPP_RSP_CODE_UNAVAIL) {
		LOG_ERR("A disconnect request cannot be refused with rsp 0x%02x", rsp_code);
		return -EINVAL;
	}

	if (opp_has_excluded_header(buf)) {
		return -EINVAL;
	}

	err = bt_obex_disconnect_rsp(&server->_server, rsp_code, buf);
	if (err != 0) {
		return err;
	}

	/*
	 * The OBEX connection is torn down when the request is accepted. A
	 * "Service Unavailable" response rejects a malformed request, which
	 * leaves the connection as it was.
	 */
	if (rsp_code == BT_OPP_RSP_CODE_SUCCESS) {
		atomic_set(&server->_state, BT_OPP_STATE_DISCONNECTED);
		atomic_set(&server->_push_state, BT_OPP_PUSH_IDLE);
	}

	return 0;
}

int bt_opp_server_push_rsp(struct bt_opp_server *server, uint8_t rsp_code, struct net_buf *buf)
{
	atomic_val_t prev = BT_OPP_PUSH_IDLE;
	bool ended;
	int err;

	if (server == NULL) {
		return -EINVAL;
	}

	if (opp_has_excluded_header(buf)) {
		return -EINVAL;
	}

	/*
	 * CONTINUE asks the client for the next packet of the same push
	 * operation. Any other response code ends it, so the next push has to
	 * name its object again before the object body.
	 *
	 * Publish that before the response is sent. The next request is handled
	 * in the RX thread, which may run before bt_obex_put_rsp() has returned
	 * to this thread, and it has to be checked as part of a new push
	 * operation, not against the Name header and body of the ended one.
	 */
	ended = rsp_code != BT_OPP_RSP_CODE_CONTINUE;
	if (ended) {
		prev = atomic_set(&server->_push_state, BT_OPP_PUSH_IDLE);
	}

	err = bt_obex_put_rsp(&server->_server, rsp_code, buf);
	if (err != 0) {
		/*
		 * Nothing has been sent, so the operation is still ongoing.
		 * Only undo the transition made above: the transport may have
		 * been torn down in the meantime, in which case the
		 * disconnected callback already published the final state.
		 */
		if (ended) {
			(void)atomic_cas(&server->_push_state, BT_OPP_PUSH_IDLE, prev);
		}

		return err;
	}

	return 0;
}

int bt_opp_server_pull_rsp(struct bt_opp_server *server, uint8_t rsp_code, struct net_buf *buf)
{
	if (server == NULL) {
		return -EINVAL;
	}

	if (opp_has_excluded_header(buf)) {
		return -EINVAL;
	}

	/*
	 * OPP spec Table 5.3 makes the End-of-Body header mandatory, and per
	 * IrOBEX 1.2 section 2.2.9 it carries the last chunk of the object,
	 * which is what a success response concludes the pull operation with.
	 * Any other response code either asks for another chunk or rejects the
	 * request.
	 */
	if (rsp_code == BT_OPP_RSP_CODE_SUCCESS &&
	    !bt_obex_has_header(buf, BT_OBEX_HEADER_ID_END_BODY)) {
		LOG_ERR("OBEX header (End of Body) is missing");
		return -ENODATA;
	}

	return bt_obex_get_rsp(&server->_server, rsp_code, buf);
}

int bt_opp_server_abort_rsp(struct bt_opp_server *server, uint8_t rsp_code, struct net_buf *buf)
{
	if (server == NULL) {
		return -EINVAL;
	}

	/*
	 * IrOBEX 1.2 section 3.3.5: the response to an abort is success,
	 * indicating that the abort was received and that the server is
	 * resynchronized with the client. A client that gets anything else
	 * should disconnect, so there is nothing to gain from sending it.
	 */
	if (rsp_code != BT_OPP_RSP_CODE_SUCCESS) {
		LOG_ERR("An abort request cannot be answered with rsp 0x%02x", rsp_code);
		return -EINVAL;
	}

	if (opp_has_excluded_header(buf)) {
		return -EINVAL;
	}

	return bt_obex_abort_rsp(&server->_server, rsp_code, buf);
}

#endif /* CONFIG_BT_OPP_SERVER */
