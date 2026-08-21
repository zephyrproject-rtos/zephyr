/* ftp.c - Bluetooth File Transfer Profile handling */

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

#include "psa/crypto.h"

#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/bluetooth/classic/rfcomm.h>
#include <zephyr/bluetooth/classic/sdp.h>
#include <zephyr/bluetooth/classic/goep.h>
#include <zephyr/bluetooth/classic/ftp.h>

#include "host/conn_internal.h"
#include "l2cap_br_internal.h"
#include "rfcomm_internal.h"
#include "obex_internal.h"

#define LOG_LEVEL CONFIG_BT_FTP_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_ftp);

typedef void (*ftp_client_cb)(struct bt_ftp_client *client, uint8_t final, struct net_buf *buf);
typedef void (*ftp_server_cb)(struct bt_ftp_server *server, bool final, struct net_buf *buf);

enum __packed bt_ftp_transport_state {
	BT_FTP_TRANSPORT_STATE_DISCONNECTED = 0,
	BT_FTP_TRANSPORT_STATE_CONNECTING = 1,
	BT_FTP_TRANSPORT_STATE_CONNECTED = 2,
	BT_FTP_TRANSPORT_STATE_DISCONNECTING = 3,
};

enum __packed bt_ftp_state {
	BT_FTP_STATE_DISCONNECTED = 0,
	BT_FTP_STATE_CONNECTING = 1,
	BT_FTP_STATE_CONNECTED = 2,
	BT_FTP_STATE_DISCONNECTING = 3,
};

enum __packed bt_ftp_op {
	BT_FTP_OP_NONE = 0,
	BT_FTP_OP_PULL_FOLDER_LISTING = 1,
	BT_FTP_OP_PUSH_FILE = 2,
	BT_FTP_OP_PULL_FILE = 3,
	BT_FTP_OP_DELETE = 4,
	BT_FTP_OP_RENAME = 5,
	BT_FTP_OP_COPY = 6,
	BT_FTP_OP_SET_PERMISSION = 7,
};

static const struct bt_uuid_128 *ftp_uuid = BT_FTP_UUID;

static int ftp_check_conn_id(struct net_buf *buf, uint32_t expected_id)
{
	uint32_t conn_id;
	int err;

	err = bt_obex_get_header_conn_id(buf, &conn_id);
	if (err != 0) {
		LOG_ERR("Failed to read CONN_ID header: %d", err);
		return err;
	}

	if (conn_id != expected_id) {
		LOG_ERR("CONN_ID mismatch: got %u, expected %u", conn_id, expected_id);
		return -EINVAL;
	}

	return 0;
}

static int ftp_check_type_header(struct net_buf *buf, const char *expected_type)
{
	uint16_t len;
	const uint8_t *type;
	int err;

	err = bt_obex_get_header_type(buf, &len, &type);
	if (err != 0) {
		return err;
	}

	if (len != strlen(expected_type) + 1U ||
	    strncmp((const char *)type, expected_type, len) != 0) {
		return -EINVAL;
	}

	return 0;
}

static int ftp_check_has_body(struct net_buf *buf)
{
	if (!bt_obex_has_header(buf, BT_OBEX_HEADER_ID_BODY) &&
	    !bt_obex_has_header(buf, BT_OBEX_HEADER_ID_END_BODY)) {
		return -ENODATA;
	}

	return 0;
}

#if defined(CONFIG_BT_FTP_CLIENT)
static void ftp_client_clear_pending(struct bt_ftp_client *client)
{
	client->_rsp_cb = NULL;
	atomic_set(&client->_optype, BT_FTP_OP_NONE);
}

static int ftp_check_action_id(struct net_buf *buf, uint8_t expected_id)
{
	uint8_t action_id;
	int err;

	err = bt_obex_get_header_action_id(buf, &action_id);
	if (err != 0) {
		LOG_ERR("Failed to read ACTION_ID header: %d", err);
		return err;
	}

	if (action_id != expected_id) {
		LOG_ERR("ACTION_ID mismatch: got %u, expected %u", action_id, expected_id);
		return -EINVAL;
	}

	return 0;
}

static int ftp_check_header_target(struct net_buf *buf)
{
	uint16_t len;
	const uint8_t *target;
	int err;
	union bt_obex_uuid uuid;

	err = bt_obex_get_header_target(buf, &len, &target);
	if (err != 0) {
		LOG_ERR("Failed to get TARGET header: %d", err);
		return err;
	}

	err = bt_obex_make_uuid(&uuid, target, len);
	if (err != 0) {
		LOG_ERR("Failed to construct UUID from TARGET header: %d", err);
		return err;
	}

	return bt_uuid_cmp(&uuid.uuid, &ftp_uuid->uuid);
}

/* Client transport callbacks */
static void ftp_client_transport_connected(struct bt_conn *conn, struct bt_goep *goep)
{
	struct bt_ftp_client *client = CONTAINER_OF(goep, struct bt_ftp_client, _goep);

	atomic_set(&client->_transport_state, BT_FTP_TRANSPORT_STATE_CONNECTED);

	if (client->_goep.v2 != NULL) {
		if (client->_cb != NULL && client->_cb->l2cap_connected != NULL) {
			client->_cb->l2cap_connected(conn, client);
		}
	} else {
		if (client->_cb != NULL && client->_cb->rfcomm_connected != NULL) {
			client->_cb->rfcomm_connected(conn, client);
		}
	}
}

static void ftp_client_transport_disconnected(struct bt_goep *goep)
{
	struct bt_ftp_client *client = CONTAINER_OF(goep, struct bt_ftp_client, _goep);

	atomic_set(&client->_transport_state, BT_FTP_TRANSPORT_STATE_DISCONNECTED);
	atomic_set(&client->_state, BT_FTP_STATE_DISCONNECTED);
	ftp_client_clear_pending(client);

	if (client->_goep.v2 != NULL) {
		if (client->_cb != NULL && client->_cb->l2cap_disconnected != NULL) {
			client->_cb->l2cap_disconnected(client);
		}
	} else {
		if (client->_cb != NULL && client->_cb->rfcomm_disconnected != NULL) {
			client->_cb->rfcomm_disconnected(client);
		}
	}
}

static struct bt_goep_transport_ops ftp_client_transport_ops = {
	.connected = ftp_client_transport_connected,
	.disconnected = ftp_client_transport_disconnected,
};

static int ftp_client_transport_connect(struct bt_conn *conn, struct bt_ftp_client *client,
					struct bt_ftp_client_cb *cb, uint8_t channel, uint16_t psm)
{
	int err;

	if (conn == NULL || client == NULL || cb == NULL || (channel == 0 && psm == 0)) {
		return -EINVAL;
	}

	if (atomic_get(&client->_transport_state) != BT_FTP_TRANSPORT_STATE_DISCONNECTED) {
		return -EINPROGRESS;
	}

	client->_cb = cb;
	client->_goep.transport_ops = &ftp_client_transport_ops;

	if (channel != 0) {
		BT_GOEP_INIT_V1(&client->_goep, &client->_goep_transport.v1);
		err = bt_goep_transport_rfcomm_connect(conn, &client->_goep, channel);
	} else {
		BT_GOEP_INIT_V2(&client->_goep, &client->_goep_transport.v2);
		err = bt_goep_transport_l2cap_connect(conn, &client->_goep, psm);
	}

	if (err == 0) {
		atomic_set(&client->_transport_state, BT_FTP_TRANSPORT_STATE_CONNECTING);
	}

	return err;
}

int bt_ftp_client_rfcomm_connect(struct bt_conn *conn, struct bt_ftp_client *client,
				 struct bt_ftp_client_cb *cb, uint8_t channel)
{
	return ftp_client_transport_connect(conn, client, cb, channel, 0);
}

int bt_ftp_client_l2cap_connect(struct bt_conn *conn, struct bt_ftp_client *client,
				struct bt_ftp_client_cb *cb, uint16_t psm)
{
	return ftp_client_transport_connect(conn, client, cb, 0, psm);
}

static int ftp_client_transport_disconnect(struct bt_ftp_client *client, bool is_rfcomm)
{
	int err;

	if (client == NULL) {
		return -EINVAL;
	}

	if (atomic_get(&client->_transport_state) != BT_FTP_TRANSPORT_STATE_CONNECTED) {
		return -EINPROGRESS;
	}

	atomic_set(&client->_transport_state, BT_FTP_TRANSPORT_STATE_DISCONNECTING);

	if (is_rfcomm) {
		err = bt_goep_transport_rfcomm_disconnect(&client->_goep);
	} else {
		err = bt_goep_transport_l2cap_disconnect(&client->_goep);
	}

	return err;
}

int bt_ftp_client_rfcomm_disconnect(struct bt_ftp_client *client)
{
	return ftp_client_transport_disconnect(client, true);
}

int bt_ftp_client_l2cap_disconnect(struct bt_ftp_client *client)
{
	return ftp_client_transport_disconnect(client, false);
}

/* Client OBEX response callbacks */
static void ftp_client_connect(struct bt_obex_client *client, uint8_t rsp_code, uint8_t version,
			       uint16_t mopl, struct net_buf *buf)
{
	struct bt_ftp_client *c = CONTAINER_OF(client, struct bt_ftp_client, _client);
	int err;

	if (rsp_code == BT_FTP_RSP_CODE_SUCCESS) {
		atomic_set(&c->_state, BT_FTP_STATE_CONNECTED);
		err = bt_obex_get_header_conn_id(buf, &c->_conn_id);
		if (err != 0) {
			LOG_ERR("Failed to get connection ID: %d", err);
			ftp_client_transport_disconnect(c, c->_goep.v2 == NULL);
		} else {
			LOG_DBG("Connection ID: %u", c->_conn_id);
		}
	} else {
		atomic_set(&c->_state, BT_FTP_STATE_DISCONNECTED);
	}

	if (c->_cb->connect != NULL) {
		c->_cb->connect(c, rsp_code, version, mopl, buf);
	}
}

static void ftp_client_disconnect(struct bt_obex_client *client, uint8_t rsp_code,
				  struct net_buf *buf)
{
	struct bt_ftp_client *c = CONTAINER_OF(client, struct bt_ftp_client, _client);

	if (rsp_code == BT_OBEX_RSP_CODE_SUCCESS) {
		atomic_set(&c->_state, BT_FTP_STATE_DISCONNECTED);
		ftp_client_clear_pending(c);
	} else {
		atomic_set(&c->_state, BT_FTP_STATE_CONNECTED);
	}

	if (c->_cb->disconnect != NULL) {
		c->_cb->disconnect(c, rsp_code, buf);
	}
}

static void ftp_client_put(struct bt_obex_client *client, uint8_t rsp_code, struct net_buf *buf)
{
	struct bt_ftp_client *c = CONTAINER_OF(client, struct bt_ftp_client, _client);
	ftp_client_cb cb = c->_rsp_cb;

	if (rsp_code != BT_FTP_RSP_CODE_CONTINUE) {
		ftp_client_clear_pending(c);
	}

	if (cb != NULL) {
		cb(c, rsp_code, buf);
	} else {
		LOG_WRN("No PUT response callback registered");
	}
}

static void ftp_client_get(struct bt_obex_client *client, uint8_t rsp_code, struct net_buf *buf)
{
	struct bt_ftp_client *c = CONTAINER_OF(client, struct bt_ftp_client, _client);
	ftp_client_cb cb = c->_rsp_cb;

	if (rsp_code != BT_FTP_RSP_CODE_CONTINUE) {
		ftp_client_clear_pending(c);
	}

	if (cb != NULL) {
		cb(c, rsp_code, buf);
	} else {
		LOG_WRN("No GET response callback registered");
	}
}

static void ftp_client_abort(struct bt_obex_client *client, uint8_t rsp_code, struct net_buf *buf)
{
	struct bt_ftp_client *c = CONTAINER_OF(client, struct bt_ftp_client, _client);
	int err;

	if (rsp_code == BT_OBEX_RSP_CODE_SUCCESS) {
		ftp_client_clear_pending(c);
	}

	if (c->_cb->abort != NULL) {
		c->_cb->abort(c, rsp_code, buf);
	}

	if (rsp_code != BT_OBEX_RSP_CODE_SUCCESS) {
		err = bt_ftp_client_disconnect(c, NULL);
		if (err != 0) {
			LOG_ERR("Failed to send DISCONNECT request: %d", err);
		}
	}
}

static void ftp_client_setpath(struct bt_obex_client *client, uint8_t rsp_code, struct net_buf *buf)
{
	struct bt_ftp_client *c = CONTAINER_OF(client, struct bt_ftp_client, _client);

	if (c->_cb->set_folder != NULL) {
		c->_cb->set_folder(c, rsp_code, buf);
	}
}

static void ftp_client_action(struct bt_obex_client *client, uint8_t rsp_code, struct net_buf *buf)
{
	struct bt_ftp_client *c = CONTAINER_OF(client, struct bt_ftp_client, _client);
	ftp_client_cb cb = c->_rsp_cb;

	if (rsp_code != BT_FTP_RSP_CODE_CONTINUE) {
		ftp_client_clear_pending(c);
	}

	if (cb != NULL) {
		cb(c, rsp_code, buf);
	} else {
		LOG_WRN("No ACTION response callback registered");
	}
}

static struct bt_obex_client_ops ftp_client_ops = {
	.connect = ftp_client_connect,
	.disconnect = ftp_client_disconnect,
	.put = ftp_client_put,
	.get = ftp_client_get,
	.abort = ftp_client_abort,
	.setpath = ftp_client_setpath,
	.action = ftp_client_action,
};

int bt_ftp_client_connect(struct bt_ftp_client *client, struct net_buf *buf)
{
	uint8_t val[BT_UUID_SIZE_128];
	int err;
	bool allocated = false;

	if (client == NULL) {
		return -EINVAL;
	}

	if (atomic_get(&client->_transport_state) != BT_FTP_TRANSPORT_STATE_CONNECTED) {
		LOG_ERR("Transport not connected");
		return -EINVAL;
	}

	if (atomic_get(&client->_state) != BT_FTP_STATE_DISCONNECTED) {
		LOG_ERR("Invalid OBEX state %u", (uint8_t)atomic_get(&client->_state));
		return -EINVAL;
	}

	if (buf == NULL) {
		buf = bt_goep_create_pdu(&client->_goep, NULL);
		if (buf == NULL) {
			LOG_ERR("Failed to allocate buffer");
			return -ENOBUFS;
		}
		allocated = true;
	}

	if (bt_obex_has_header(buf, BT_OBEX_HEADER_ID_TARGET)) {
		err = ftp_check_header_target(buf);
		if (err != 0) {
			LOG_ERR("Invalid TARGET header content");
			goto failed;
		}
	} else {
		sys_memcpy_swap(val, ftp_uuid->val, sizeof(val));
		err = bt_obex_add_header_target(buf, sizeof(val), val);
		if (err != 0) {
			LOG_ERR("Failed to add header target: %d", err);
			goto failed;
		}
	}

	client->_client.ops = &ftp_client_ops;
	client->_client.obex = &client->_goep.obex;

	err = bt_obex_connect(&client->_client, client->_goep.obex.rx.mtu, buf);
	if (err != 0) {
		LOG_ERR("Failed to send CONNECT request: %d", err);
		goto failed;
	}

	atomic_set(&client->_state, BT_FTP_STATE_CONNECTING);
	return 0;

failed:
	if (allocated) {
		net_buf_unref(buf);
	}
	return err;
}

int bt_ftp_client_abort(struct bt_ftp_client *client, struct net_buf *buf)
{
	int err;
	bool allocated = false;

	if (client == NULL) {
		return -EINVAL;
	}

	if (atomic_get(&client->_state) != BT_FTP_STATE_CONNECTED) {
		LOG_ERR("Invalid OBEX state %u", (uint8_t)atomic_get(&client->_state));
		return -EINVAL;
	}

	if (buf == NULL) {
		buf = bt_goep_create_pdu(&client->_goep, NULL);
		if (buf == NULL) {
			LOG_ERR("Failed to allocate buffer");
			return -ENOBUFS;
		}
		allocated = true;
	}

	if (bt_obex_has_header(buf, BT_OBEX_HEADER_ID_CONN_ID)) {
		err = ftp_check_conn_id(buf, client->_conn_id);
		if (err != 0) {
			goto failed;
		}
	} else {
		err = bt_obex_add_header_conn_id(buf, client->_conn_id);
		if (err != 0) {
			LOG_ERR("Failed to add header conn id: %d", err);
			goto failed;
		}
	}

	err = bt_obex_abort(&client->_client, buf);
	if (err != 0) {
		LOG_ERR("Failed to send ABORT request: %d", err);
		goto failed;
	}

	return 0;

failed:
	if (allocated) {
		net_buf_unref(buf);
	}
	return err;
}

int bt_ftp_client_disconnect(struct bt_ftp_client *client, struct net_buf *buf)
{
	int err;
	bool allocated = false;

	if (client == NULL) {
		return -EINVAL;
	}

	if (atomic_get(&client->_state) != BT_FTP_STATE_CONNECTED) {
		LOG_ERR("Invalid OBEX state %u", (uint8_t)atomic_get(&client->_state));
		return -EINVAL;
	}

	if (buf == NULL) {
		buf = bt_goep_create_pdu(&client->_goep, NULL);
		if (buf == NULL) {
			LOG_ERR("Failed to allocate buffer");
			return -ENOBUFS;
		}
		allocated = true;
	}

	if (!bt_obex_has_header(buf, BT_OBEX_HEADER_ID_CONN_ID)) {
		err = bt_obex_add_header_conn_id(buf, client->_conn_id);
		if (err != 0) {
			LOG_ERR("Failed to add header conn id: %d", err);
			goto failed;
		}
	} else {
		err = ftp_check_conn_id(buf, client->_conn_id);
		if (err != 0) {
			goto failed;
		}
	}

	err = bt_obex_disconnect(&client->_client, buf);
	if (err != 0) {
		LOG_ERR("Failed to send DISCONNECT request: %d", err);
		goto failed;
	}

	atomic_set(&client->_state, BT_FTP_STATE_DISCONNECTING);
	return 0;

failed:
	if (allocated) {
		net_buf_unref(buf);
	}
	return err;
}

int bt_ftp_client_set_folder(struct bt_ftp_client *client, uint8_t flags, struct net_buf *buf)
{
	int err;
	bool allocated = false;

	if (client == NULL) {
		return -EINVAL;
	}

	if (atomic_get(&client->_state) != BT_FTP_STATE_CONNECTED) {
		LOG_ERR("Invalid OBEX state %u", (uint8_t)atomic_get(&client->_state));
		return -EINVAL;
	}

	if (buf == NULL) {
		buf = bt_goep_create_pdu(&client->_goep, NULL);
		if (buf == NULL) {
			LOG_ERR("Failed to allocate buffer");
			return -ENOBUFS;
		}
		allocated = true;
	}

	if ((flags == BT_FTP_SET_FOLDER_FLAGS_DOWN || flags == BT_FTP_SET_FOLDER_FLAGS_ROOT ||
	     flags == BT_FTP_SET_FOLDER_FLAGS_NEW) &&
	    (!bt_obex_has_header(buf, BT_OBEX_HEADER_ID_NAME))) {
		LOG_ERR("Failed to get name when flags is root/down or creating a new folder");
		err = -EINVAL;
		goto failed;
	}

	if (!bt_obex_has_header(buf, BT_OBEX_HEADER_ID_CONN_ID)) {
		err = bt_obex_add_header_conn_id(buf, client->_conn_id);
		if (err != 0) {
			LOG_ERR("Failed to add header conn id: %d", err);
			goto failed;
		}
	} else {
		err = ftp_check_conn_id(buf, client->_conn_id);
		if (err != 0) {
			goto failed;
		}
	}

	err = bt_obex_setpath(&client->_client, flags, buf);
	if (err != 0) {
		LOG_ERR("Failed to send SETPATH request: %d", err);
		goto failed;
	}

	return 0;

failed:
	if (allocated) {
		net_buf_unref(buf);
	}
	return err;
}

int bt_ftp_client_pull_folder_listing(struct bt_ftp_client *client, bool final, struct net_buf *buf)
{
	int err;
	atomic_val_t old_optype;

	if (client == NULL || buf == NULL) {
		return -EINVAL;
	}

	if (atomic_get(&client->_state) != BT_FTP_STATE_CONNECTED) {
		LOG_ERR("Invalid state %u", (uint8_t)atomic_get(&client->_state));
		return -EINVAL;
	}

	old_optype = atomic_get(&client->_optype);

	/*
	 * On the first fragment validate mandatory headers:
	 * - CONN_ID: must be present and match.
	 * - TYPE: must be present and equal "x-obex/folder-listing".
	 */
	if (atomic_cas(&client->_optype, BT_FTP_OP_NONE, BT_FTP_OP_PULL_FOLDER_LISTING)) {
		err = ftp_check_conn_id(buf, client->_conn_id);
		if (err != 0) {
			goto failed;
		}

		err = ftp_check_type_header(buf, BT_FTP_FOLDER_LISTING_TYPE);
		if (err != 0) {
			LOG_ERR("TYPE header missing or mismatch");
			goto failed;
		}

		client->_rsp_cb = client->_cb->pull_folder_listing;
	} else {
		if (atomic_get(&client->_optype) != BT_FTP_OP_PULL_FOLDER_LISTING) {
			LOG_ERR("Previous operation is not completed");
			err = -EBUSY;
			goto failed;
		}
	}

	err = bt_obex_get(&client->_client, final, buf);
	if (err == 0) {
		return 0;
	}

failed:
	LOG_ERR("Failed to send GET (folder listing) request: %d", err);
	if (old_optype == BT_FTP_OP_NONE) {
		ftp_client_clear_pending(client);
	}

	return err;
}

int bt_ftp_client_push_file(struct bt_ftp_client *client, bool final, struct net_buf *buf)
{
	int err;
	atomic_val_t old_optype;

	if (client == NULL || buf == NULL) {
		return -EINVAL;
	}

	if (atomic_get(&client->_state) != BT_FTP_STATE_CONNECTED) {
		LOG_ERR("Invalid state %u", (uint8_t)atomic_get(&client->_state));
		return -EINVAL;
	}

	old_optype = atomic_get(&client->_optype);

	/*
	 * First fragment: CONN_ID and NAME are mandatory.
	 */
	if (atomic_cas(&client->_optype, BT_FTP_OP_NONE, BT_FTP_OP_PUSH_FILE)) {
		err = ftp_check_conn_id(buf, client->_conn_id);
		if (err != 0) {
			goto failed;
		}

		if (!bt_obex_has_header(buf, BT_OBEX_HEADER_ID_NAME)) {
			LOG_ERR("Missing required NAME header for push file");
			err = -EINVAL;
			goto failed;
		}

		client->_rsp_cb = client->_cb->push_file;
	} else {
		if (atomic_get(&client->_optype) != BT_FTP_OP_PUSH_FILE) {
			LOG_ERR("Previous operation is not completed");
			err = -EBUSY;
			goto failed;
		}
	}

	/* BODY or END_BODY must be present; absence would make this a
	 * delete rather than a file upload.
	 */
	err = ftp_check_has_body(buf);
	if (err != 0) {
		LOG_ERR("Missing required BODY or END_BODY header");
		goto failed;
	}

	if (final && !bt_obex_has_header(buf, BT_OBEX_HEADER_ID_END_BODY)) {
		LOG_ERR("OBEX header (End of Body) is missing");
		err = -EINVAL;
		goto failed;
	}

	err = bt_obex_put(&client->_client, final, buf);
	if (err == 0) {
		return 0;
	}

failed:
	LOG_ERR("Failed to send PUT (push file) request: %d", err);
	if (old_optype == BT_FTP_OP_NONE) {
		ftp_client_clear_pending(client);
	}
	return err;
}

int bt_ftp_client_pull_file(struct bt_ftp_client *client, bool final, struct net_buf *buf)
{
	int err;
	atomic_val_t old_optype;

	if (client == NULL || buf == NULL) {
		return -EINVAL;
	}

	if (atomic_get(&client->_state) != BT_FTP_STATE_CONNECTED) {
		LOG_ERR("Invalid state %u", (uint8_t)atomic_get(&client->_state));
		return -EINVAL;
	}

	old_optype = atomic_get(&client->_optype);

	/*
	 * First fragment: CONN_ID and NAME are mandatory.
	 */
	if (atomic_cas(&client->_optype, BT_FTP_OP_NONE, BT_FTP_OP_PULL_FILE)) {
		err = ftp_check_conn_id(buf, client->_conn_id);
		if (err != 0) {
			goto failed;
		}

		if (!bt_obex_has_header(buf, BT_OBEX_HEADER_ID_NAME)) {
			LOG_ERR("Missing required NAME header for pull file");
			err = -EINVAL;
			goto failed;
		}

		client->_rsp_cb = client->_cb->pull_file;
	} else {
		if (atomic_get(&client->_optype) != BT_FTP_OP_PULL_FILE) {
			LOG_ERR("Previous operation is not completed");
			err = -EBUSY;
			goto failed;
		}
	}

	err = bt_obex_get(&client->_client, final, buf);
	if (err == 0) {
		return 0;
	}

failed:
	LOG_ERR("Failed to send GET (pull file) request: %d", err);
	if (old_optype == BT_FTP_OP_NONE) {
		ftp_client_clear_pending(client);
	}

	return err;
}

int bt_ftp_client_delete(struct bt_ftp_client *client, bool final, struct net_buf *buf)
{
	int err;
	atomic_val_t old_optype;

	if (client == NULL || buf == NULL) {
		return -EINVAL;
	}

	if (atomic_get(&client->_state) != BT_FTP_STATE_CONNECTED) {
		LOG_ERR("Invalid state %u", (uint8_t)atomic_get(&client->_state));
		return -EINVAL;
	}

	old_optype = atomic_get(&client->_optype);

	/*
	 * First fragment: CONN_ID and NAME are mandatory.
	 */
	if (atomic_cas(&client->_optype, BT_FTP_OP_NONE, BT_FTP_OP_DELETE)) {
		err = ftp_check_conn_id(buf, client->_conn_id);
		if (err != 0) {
			goto failed;
		}

		if (!bt_obex_has_header(buf, BT_OBEX_HEADER_ID_NAME)) {
			LOG_ERR("Missing required NAME header for delete");
			err = -EINVAL;
			goto failed;
		}

		client->_rsp_cb = client->_cb->delete;
	} else {
		if (atomic_get(&client->_optype) != BT_FTP_OP_DELETE) {
			LOG_ERR("Previous operation is not completed");
			err = -EBUSY;
			goto failed;
		}
	}

	/* BODY/END_BODY must NOT be present. */
	err = ftp_check_has_body(buf);
	if (err == 0) {
		LOG_ERR("BODY or END_BODY header must not be present for this operation");
		err = -ENODATA;
		goto failed;
	}

	err = bt_obex_put(&client->_client, final, buf);
	if (err == 0) {
		return 0;
	}

failed:
	LOG_ERR("Failed to send PUT (delete) request: %d", err);
	if (old_optype == BT_FTP_OP_NONE) {
		ftp_client_clear_pending(client);
	}
	return err;
}

int bt_ftp_client_rename(struct bt_ftp_client *client, bool final, struct net_buf *buf)
{
	int err;
	atomic_val_t old_optype;

	if (client == NULL || buf == NULL) {
		return -EINVAL;
	}

	if (atomic_get(&client->_state) != BT_FTP_STATE_CONNECTED) {
		LOG_ERR("Invalid state %u", (uint8_t)atomic_get(&client->_state));
		return -EINVAL;
	}

	old_optype = atomic_get(&client->_optype);

	if (atomic_cas(&client->_optype, BT_FTP_OP_NONE, BT_FTP_OP_RENAME)) {
		/*
		 * First fragment:
		 * - CONN_ID must match.
		 * - NAME (source) must be present.
		 * - DEST_NAME (destination) must be present.
		 * - ACTION_ID must equal BT_OBEX_ACTION_MOVE_RENAME (0x01).
		 */
		err = ftp_check_conn_id(buf, client->_conn_id);
		if (err != 0) {
			goto failed;
		}

		if (!bt_obex_has_header(buf, BT_OBEX_HEADER_ID_NAME)) {
			LOG_ERR("Missing required NAME header for rename");
			err = -EINVAL;
			goto failed;
		}

		if (!bt_obex_has_header(buf, BT_OBEX_HEADER_ID_DEST_NAME)) {
			LOG_ERR("Missing required DEST_NAME header for rename");
			err = -EINVAL;
			goto failed;
		}

		err = ftp_check_action_id(buf, BT_OBEX_ACTION_MOVE_RENAME);
		if (err != 0) {
			goto failed;
		}

		client->_rsp_cb = client->_cb->rename;
	} else {
		if (atomic_get(&client->_optype) != BT_FTP_OP_RENAME) {
			LOG_ERR("Previous operation is not completed");
			err = -EBUSY;
			goto failed;
		}
	}

	err = bt_obex_action(&client->_client, final, buf);
	if (err == 0) {
		return 0;
	}

failed:
	LOG_ERR("Failed to send ACTION (rename) request: %d", err);
	if (old_optype == BT_FTP_OP_NONE) {
		ftp_client_clear_pending(client);
	}

	return err;
}

int bt_ftp_client_copy(struct bt_ftp_client *client, bool final, struct net_buf *buf)
{
	int err;
	atomic_val_t old_optype;

	if (client == NULL || buf == NULL) {
		return -EINVAL;
	}

	if (atomic_get(&client->_state) != BT_FTP_STATE_CONNECTED) {
		LOG_ERR("Invalid state %u", (uint8_t)atomic_get(&client->_state));
		return -EINVAL;
	}

	old_optype = atomic_get(&client->_optype);

	if (atomic_cas(&client->_optype, BT_FTP_OP_NONE, BT_FTP_OP_COPY)) {
		/*
		 * First fragment:
		 * - CONN_ID must match.
		 * - NAME (source) must be present.
		 * - DEST_NAME (destination) must be present.
		 * - ACTION_ID must equal BT_OBEX_ACTION_COPY (0x00).
		 */
		err = ftp_check_conn_id(buf, client->_conn_id);
		if (err != 0) {
			goto failed;
		}

		if (!bt_obex_has_header(buf, BT_OBEX_HEADER_ID_NAME)) {
			LOG_ERR("Missing required NAME header for copy");
			err = -EINVAL;
			goto failed;
		}

		if (!bt_obex_has_header(buf, BT_OBEX_HEADER_ID_DEST_NAME)) {
			LOG_ERR("Missing required DEST_NAME header for copy");
			err = -EINVAL;
			goto failed;
		}

		err = ftp_check_action_id(buf, BT_OBEX_ACTION_COPY);
		if (err != 0) {
			goto failed;
		}

		client->_rsp_cb = client->_cb->copy;
	} else {
		if (atomic_get(&client->_optype) != BT_FTP_OP_COPY) {
			LOG_ERR("Previous operation is not completed");
			err = -EBUSY;
			goto failed;
		}
	}

	err = bt_obex_action(&client->_client, final, buf);
	if (err == 0) {
		return 0;
	}

failed:
	LOG_ERR("Failed to send ACTION (copy) request: %d", err);
	if (old_optype == BT_FTP_OP_NONE) {
		ftp_client_clear_pending(client);
	}

	return err;
}

int bt_ftp_client_set_permission(struct bt_ftp_client *client, bool final, struct net_buf *buf)
{
	int err;
	atomic_val_t old_optype;

	if (client == NULL || buf == NULL) {
		return -EINVAL;
	}

	if (atomic_get(&client->_state) != BT_FTP_STATE_CONNECTED) {
		LOG_ERR("Invalid state %u", (uint8_t)atomic_get(&client->_state));
		return -EINVAL;
	}

	old_optype = atomic_get(&client->_optype);

	if (atomic_cas(&client->_optype, BT_FTP_OP_NONE, BT_FTP_OP_SET_PERMISSION)) {
		/*
		 * First fragment:
		 * - CONN_ID must match.
		 * - NAME must identify the target object.
		 * - PERM (permissions bitmask) must be present.
		 * - ACTION_ID must equal BT_OBEX_ACTION_SET_PERM (0x02).
		 */
		err = ftp_check_conn_id(buf, client->_conn_id);
		if (err != 0) {
			goto failed;
		}

		if (!bt_obex_has_header(buf, BT_OBEX_HEADER_ID_NAME)) {
			LOG_ERR("Missing required NAME header for set permission");
			err = -EINVAL;
			goto failed;
		}

		if (!bt_obex_has_header(buf, BT_OBEX_HEADER_ID_PERM)) {
			LOG_ERR("Missing required PERM header for set permission");
			err = -EINVAL;
			goto failed;
		}

		err = ftp_check_action_id(buf, BT_OBEX_ACTION_SET_PERM);
		if (err != 0) {
			goto failed;
		}

		client->_rsp_cb = client->_cb->set_permission;
	} else {
		if (atomic_get(&client->_optype) != BT_FTP_OP_SET_PERMISSION) {
			LOG_ERR("Previous operation is not completed");
			err = -EBUSY;
			goto failed;
		}
	}

	err = bt_obex_action(&client->_client, final, buf);
	if (err == 0) {
		return 0;
	}

failed:
	LOG_ERR("Failed to send ACTION (set permission) request: %d", err);
	if (old_optype == BT_FTP_OP_NONE) {
		ftp_client_clear_pending(client);
	}

	return err;
}

struct net_buf *bt_ftp_client_create_pdu(struct bt_ftp_client *client, struct net_buf_pool *pool)
{
	if (client == NULL) {
		LOG_ERR("Invalid parameter");
		return NULL;
	}

	return bt_goep_create_pdu(&client->_goep, pool);
}
#endif /* CONFIG_BT_FTP_CLIENT */

#if defined(CONFIG_BT_FTP_SERVER)
static uint8_t ftp_rfcomm_channel;
static uint16_t ftp_l2cap_psm;
static struct bt_sdp_attribute ftp_server_attrs[] = {
	BT_SDP_NEW_SERVICE,
	BT_SDP_LIST(
		BT_SDP_ATTR_SVCLASS_ID_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
			BT_SDP_ARRAY_16(BT_SDP_OBEX_FILETRANS_SVCLASS)
		},
		)
	),
	BT_SDP_LIST(
		BT_SDP_ATTR_PROTO_DESC_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 17),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3),
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
				BT_SDP_ARRAY_16(BT_SDP_PROTO_L2CAP)
			},
			)
		},
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 5),
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
				BT_SDP_ARRAY_16(BT_SDP_PROTO_RFCOMM)
			},
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UINT8),
				&ftp_rfcomm_channel
			},
			)
		},
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3),
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
				BT_SDP_ARRAY_16(BT_SDP_PROTO_OBEX)
			},
			)
		},
		)
	),
	BT_SDP_SERVICE_NAME(CONFIG_BT_FTP_SERVICE_NAME),
	BT_SDP_LIST(
		BT_SDP_ATTR_PROFILE_DESC_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 8),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
				BT_SDP_ARRAY_16(BT_SDP_GENERIC_FILETRANS_SVCLASS)
			},
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
				BT_SDP_ARRAY_16(0x0103)
			},
			)
		},
		)
	),
	{
		BT_SDP_ATTR_GOEP_L2CAP_PSM,
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
			&ftp_l2cap_psm
		}
	},
};

static struct bt_sdp_record ftp_server_rec = BT_SDP_RECORD(ftp_server_attrs);

static void ftp_server_clear_pending(struct bt_ftp_server *server)
{
	server->_req_cb = NULL;
	atomic_set(&server->_optype, BT_FTP_OP_NONE);
}

static uint32_t ftp_server_get_connect_id(void)
{
	static uint32_t connect_id;

	connect_id++;

	if (connect_id == 0U) {
		connect_id = 1;
	}

	return connect_id;
}

static int ftp_check_header_who(struct net_buf *buf)
{
	uint16_t len;
	const uint8_t *who;
	int err;
	union bt_obex_uuid uuid;

	err = bt_obex_get_header_who(buf, &len, &who);
	if (err != 0) {
		LOG_ERR("Failed to get WHO header: %d", err);
		return err;
	}

	err = bt_obex_make_uuid(&uuid, who, len);
	if (err != 0) {
		LOG_ERR("Failed to construct UUID from WHO header: %d", err);
		return err;
	}

	return bt_uuid_cmp(&uuid.uuid, &ftp_uuid->uuid);
}

/* Server transport callbacks */
static void ftp_server_transport_connected(struct bt_conn *conn, struct bt_goep *goep)
{
	struct bt_ftp_server *server = CONTAINER_OF(goep, struct bt_ftp_server, _goep);

	atomic_set(&server->_transport_state, BT_FTP_TRANSPORT_STATE_CONNECTED);

	if (server->_goep.v2 != NULL) {
		if (server->_cb != NULL && server->_cb->l2cap_connected != NULL) {
			server->_cb->l2cap_connected(conn, server);
		}
	} else {
		if (server->_cb != NULL && server->_cb->rfcomm_connected != NULL) {
			server->_cb->rfcomm_connected(conn, server);
		}
	}
}

static void ftp_server_transport_disconnected(struct bt_goep *goep)
{
	struct bt_ftp_server *server = CONTAINER_OF(goep, struct bt_ftp_server, _goep);

	atomic_set(&server->_transport_state, BT_FTP_TRANSPORT_STATE_DISCONNECTED);
	atomic_set(&server->_state, BT_FTP_STATE_DISCONNECTED);
	ftp_server_clear_pending(server);

	if (server->_goep.v2 != NULL) {
		if (server->_cb != NULL && server->_cb->l2cap_disconnected != NULL) {
			server->_cb->l2cap_disconnected(server);
		}
	} else {
		if (server->_cb != NULL && server->_cb->rfcomm_disconnected != NULL) {
			server->_cb->rfcomm_disconnected(server);
		}
	}
}

static struct bt_goep_transport_ops ftp_server_transport_ops = {
	.connected = ftp_server_transport_connected,
	.disconnected = ftp_server_transport_disconnected,
};

/* Server OBEX operation callbacks */
static void ftp_server_connect(struct bt_obex_server *server, uint8_t version, uint16_t mopl,
			       struct net_buf *buf)
{
	struct bt_ftp_server *s = CONTAINER_OF(server, struct bt_ftp_server, _server);

	atomic_set(&s->_state, BT_FTP_STATE_CONNECTING);

	if (s->_cb->connect != NULL) {
		s->_cb->connect(s, version, mopl, buf);
	}
}

static void ftp_server_disconnect(struct bt_obex_server *server, struct net_buf *buf)
{
	struct bt_ftp_server *s = CONTAINER_OF(server, struct bt_ftp_server, _server);

	atomic_set(&s->_state, BT_FTP_STATE_DISCONNECTING);

	if (s->_cb->disconnect != NULL) {
		s->_cb->disconnect(s, buf);
	}
}

static void ftp_server_put(struct bt_obex_server *server, bool final, struct net_buf *buf)
{
	struct bt_ftp_server *s = CONTAINER_OF(server, struct bt_ftp_server, _server);
	enum bt_obex_rsp_code rsp_code;
	int err;

	/*
	 * On the first packet determine which callback to use and cache it in
	 * s->_req_cb so subsequent packets of the same multi-packet
	 * operation are dispatched to the same handler without re-inspecting
	 * headers that may be absent in continuation packets.
	 *
	 * A PUT without BODY / END_BODY is a delete; a PUT with body is a
	 * file upload.
	 */
	if (s->_req_cb == NULL) {
		err = ftp_check_conn_id(buf, s->_conn_id);
		if (err != 0) {
			rsp_code = BT_OBEX_RSP_CODE_BAD_REQ;
			goto failed;
		}

		if (!bt_obex_has_header(buf, BT_OBEX_HEADER_ID_NAME)) {
			rsp_code = BT_OBEX_RSP_CODE_BAD_REQ;
			goto failed;
		}

		err = ftp_check_has_body(buf);
		if (err != 0) {
			s->_req_cb = s->_cb->delete;
			atomic_set(&s->_optype, BT_FTP_OP_DELETE);
		} else {
			s->_req_cb = s->_cb->push_file;
			atomic_set(&s->_optype, BT_FTP_OP_PUSH_FILE);
		}
	}

	if (s->_req_cb == NULL) {
		LOG_WRN("No handler registered for PUT operation");
		rsp_code = BT_OBEX_RSP_CODE_NOT_IMPL;
		goto failed;
	}

	s->_req_cb(s, final, buf);

	return;

failed:
	ftp_server_clear_pending(s);
	err = bt_obex_put_rsp(server, rsp_code, NULL);
	if (err != 0) {
		LOG_ERR("Failed to send PUT error response: %d", err);
	}
}

static void ftp_server_get(struct bt_obex_server *server, bool final, struct net_buf *buf)
{
	struct bt_ftp_server *s = CONTAINER_OF(server, struct bt_ftp_server, _server);
	enum bt_obex_rsp_code rsp_code;
	int err;

	/*
	 * On the first packet decide pull_folder_listing vs pull_file using
	 * the TYPE header, then cache the chosen callback so continuation
	 * packets (which carry no TYPE header) are handled consistently.
	 */
	if (s->_req_cb == NULL) {
		err = ftp_check_conn_id(buf, s->_conn_id);
		if (err != 0) {
			rsp_code = BT_OBEX_RSP_CODE_BAD_REQ;
			goto failed;
		}

		if (!bt_obex_has_header(buf, BT_OBEX_HEADER_ID_NAME)) {
			rsp_code = BT_OBEX_RSP_CODE_BAD_REQ;
			goto failed;
		}

		err = ftp_check_type_header(buf, BT_FTP_FOLDER_LISTING_TYPE);
		if (err != 0) {
			s->_req_cb = s->_cb->pull_file;
			atomic_set(&s->_optype, BT_FTP_OP_PULL_FILE);
		} else {
			s->_req_cb = s->_cb->pull_folder_listing;
			atomic_set(&s->_optype, BT_FTP_OP_PULL_FOLDER_LISTING);
		}
	}

	if (s->_req_cb == NULL) {
		LOG_WRN("No handler registered for GET operation");
		rsp_code = BT_OBEX_RSP_CODE_NOT_IMPL;
		goto failed;
	}

	s->_req_cb(s, final, buf);

	return;

failed:
	ftp_server_clear_pending(s);
	err = bt_obex_get_rsp(server, rsp_code, NULL);
	if (err != 0) {
		LOG_ERR("Failed to send GET error response: %d", err);
	}
}

static void ftp_server_abort(struct bt_obex_server *server, struct net_buf *buf)
{
	struct bt_ftp_server *s = CONTAINER_OF(server, struct bt_ftp_server, _server);
	int err;

	if (s->_cb->abort != NULL) {
		s->_cb->abort(s, buf);
		return;
	}

	LOG_WRN("No callback registered for abort request");
	err = bt_obex_abort_rsp(server, BT_OBEX_RSP_CODE_NOT_IMPL, NULL);
	if (err != 0) {
		LOG_ERR("Failed to send abort rsp: %d", err);
	}
}

static void ftp_server_setpath(struct bt_obex_server *server, uint8_t flags, struct net_buf *buf)
{
	struct bt_ftp_server *s = CONTAINER_OF(server, struct bt_ftp_server, _server);
	int err;

	if (s->_cb != NULL && s->_cb->set_folder != NULL) {
		s->_cb->set_folder(s, flags, buf);
		return;
	}

	LOG_WRN("No callback registered for set_folder request");
	err = bt_obex_setpath_rsp(server, BT_OBEX_RSP_CODE_NOT_IMPL, NULL);
	if (err != 0) {
		LOG_ERR("Failed to send set_folder rsp: %d", err);
	}
}

static void ftp_server_action(struct bt_obex_server *server, bool final, struct net_buf *buf)
{
	struct bt_ftp_server *s = CONTAINER_OF(server, struct bt_ftp_server, _server);
	enum bt_obex_rsp_code rsp_code;
	uint8_t action_id;
	int err;

	/*
	 * On the first packet decode ACTION_ID and cache the matching
	 * callback; continuation packets reuse the cached callback.
	 */
	if (s->_req_cb == NULL) {
		rsp_code = BT_OBEX_RSP_CODE_BAD_REQ;

		err = ftp_check_conn_id(buf, s->_conn_id);
		if (err != 0) {
			goto failed;
		}

		err = bt_obex_get_header_action_id(buf, &action_id);
		if (err != 0) {
			goto failed;
		}

		if (!bt_obex_has_header(buf, BT_OBEX_HEADER_ID_NAME)) {
			goto failed;
		}

		switch (action_id) {
		case BT_OBEX_ACTION_COPY:
			if (!bt_obex_has_header(buf, BT_OBEX_HEADER_ID_DEST_NAME)) {
				goto failed;
			}

			s->_req_cb = s->_cb->copy;
			atomic_set(&s->_optype, BT_FTP_OP_COPY);
			break;
		case BT_OBEX_ACTION_MOVE_RENAME:
			if (!bt_obex_has_header(buf, BT_OBEX_HEADER_ID_DEST_NAME)) {
				goto failed;
			}

			s->_req_cb = s->_cb->rename;
			atomic_set(&s->_optype, BT_FTP_OP_RENAME);
			break;
		case BT_OBEX_ACTION_SET_PERM:
			if (!bt_obex_has_header(buf, BT_OBEX_HEADER_ID_PERM)) {
				goto failed;
			}

			s->_req_cb = s->_cb->set_permission;
			atomic_set(&s->_optype, BT_FTP_OP_SET_PERMISSION);
			break;
		default:
			break;
		}
	}

	if (s->_req_cb == NULL) {
		LOG_WRN("No handler registered for ACTION operation");
		rsp_code = BT_OBEX_RSP_CODE_NOT_IMPL;
		goto failed;
	}

	s->_req_cb(s, final, buf);

	return;

failed:
	ftp_server_clear_pending(s);
	err = bt_obex_put_rsp(server, rsp_code, NULL);
	if (err != 0) {
		LOG_ERR("Failed to send ACTION error response: %d", err);
	}
}

static struct bt_obex_server_ops ftp_server_ops = {
	.connect = ftp_server_connect,
	.disconnect = ftp_server_disconnect,
	.put = ftp_server_put,
	.get = ftp_server_get,
	.abort = ftp_server_abort,
	.setpath = ftp_server_setpath,
	.action = ftp_server_action,
};

/* Server transport accept callbacks */
static int ftp_server_rfcomm_accept(struct bt_conn *conn,
				    struct bt_goep_transport_rfcomm_server *goep_server,
				    struct bt_goep **goep)
{
	struct bt_ftp_server_rfcomm *ftp_rfcomm =
		CONTAINER_OF(goep_server, struct bt_ftp_server_rfcomm, server);
	struct bt_ftp_server *server;
	int err;

	if (ftp_rfcomm->accept == NULL) {
		return -ENOTSUP;
	}

	err = ftp_rfcomm->accept(conn, ftp_rfcomm, &server);
	if (err != 0) {
		LOG_WRN("Incoming connection rejected");
		return err;
	}

	if (server == NULL || server->_cb == NULL) {
		LOG_ERR("Invalid parameter");
		return -EINVAL;
	}

	server->_goep.transport_ops = &ftp_server_transport_ops;
	BT_GOEP_INIT_V1(&server->_goep, &server->_goep_transport.v1);
	*goep = &server->_goep;

	atomic_set(&server->_transport_state, BT_FTP_TRANSPORT_STATE_CONNECTING);
	return 0;
}

static int ftp_server_l2cap_accept(struct bt_conn *conn,
				   struct bt_goep_transport_l2cap_server *goep_server,
				   struct bt_goep **goep)
{
	struct bt_ftp_server_l2cap *ftp_l2cap =
		CONTAINER_OF(goep_server, struct bt_ftp_server_l2cap, server);
	struct bt_ftp_server *server;
	int err;

	if (ftp_l2cap->accept == NULL) {
		return -ENOTSUP;
	}

	err = ftp_l2cap->accept(conn, ftp_l2cap, &server);
	if (err != 0) {
		LOG_WRN("Incoming connection rejected");
		return err;
	}

	if (server == NULL || server->_cb == NULL) {
		LOG_ERR("Invalid parameter");
		return -EINVAL;
	}

	server->_goep.transport_ops = &ftp_server_transport_ops;
	BT_GOEP_INIT_V2(&server->_goep, &server->_goep_transport.v2);
	*goep = &server->_goep;

	atomic_set(&server->_transport_state, BT_FTP_TRANSPORT_STATE_CONNECTING);
	return 0;
}

int bt_ftp_server_register(struct bt_ftp_server *server, struct bt_ftp_server_cb *cb)
{
	if (server == NULL || cb == NULL) {
		return -EINVAL;
	}

	server->_cb = cb;
	server->_server.ops = &ftp_server_ops;
	server->_server.obex = &server->_goep.obex;
	server->_conn_id = ftp_server_get_connect_id();

	return bt_obex_server_register(&server->_server, ftp_uuid);
}

int bt_ftp_server_rfcomm_register(struct bt_ftp_server_rfcomm *server)
{
	int err;

	if (server == NULL || server->accept == NULL) {
		return -EINVAL;
	}

	server->server.accept = ftp_server_rfcomm_accept;

	err = bt_goep_transport_rfcomm_server_register(&server->server);
	if (err != 0) {
		LOG_ERR("Failed to register RFCOMM server: %d", err);
		return err;
	}

	ftp_rfcomm_channel = server->server.rfcomm.channel;

	if (ftp_l2cap_psm != 0U) {
		return 0;
	}

	err = bt_sdp_register_service(&ftp_server_rec);
	if (err != 0) {
		LOG_ERR("Failed to register SDP record: %d", err);
		return err;
	}

	return 0;
}

int bt_ftp_server_l2cap_register(struct bt_ftp_server_l2cap *server)
{
	int err;

	if (server == NULL || server->accept == NULL) {
		return -EINVAL;
	}

	server->server.accept = ftp_server_l2cap_accept;

	err = bt_goep_transport_l2cap_server_register(&server->server);
	if (err != 0) {
		LOG_ERR("Failed to register L2CAP server: %d", err);
		return err;
	}

	ftp_l2cap_psm = server->server.l2cap.psm;

	if (ftp_rfcomm_channel != 0U) {
		return 0;
	}

	err = bt_sdp_register_service(&ftp_server_rec);
	if (err != 0) {
		LOG_ERR("Failed to register SDP record: %d", err);
		return err;
	}

	return 0;
}

static int ftp_server_transport_disconnect(struct bt_ftp_server *server, bool is_rfcomm)
{
	int err;

	if (server == NULL) {
		return -EINVAL;
	}

	if (atomic_get(&server->_transport_state) != BT_FTP_TRANSPORT_STATE_CONNECTED) {
		return -EINPROGRESS;
	}

	atomic_set(&server->_transport_state, BT_FTP_TRANSPORT_STATE_DISCONNECTING);

	if (is_rfcomm) {
		err = bt_goep_transport_rfcomm_disconnect(&server->_goep);
	} else {
		err = bt_goep_transport_l2cap_disconnect(&server->_goep);
	}

	return err;
}

int bt_ftp_server_rfcomm_disconnect(struct bt_ftp_server *server)
{
	return ftp_server_transport_disconnect(server, true);
}

int bt_ftp_server_l2cap_disconnect(struct bt_ftp_server *server)
{
	return ftp_server_transport_disconnect(server, false);
}

int bt_ftp_server_connect(struct bt_ftp_server *server, uint8_t rsp_code, struct net_buf *buf)
{
	uint8_t val[BT_UUID_SIZE_128];
	int err;
	bool allocated = false;

	if (server == NULL) {
		return -EINVAL;
	}

	if (atomic_get(&server->_state) != BT_FTP_STATE_CONNECTING) {
		LOG_ERR("Invalid OBEX state %u", (uint8_t)atomic_get(&server->_state));
		return -EINVAL;
	}

	if (buf == NULL) {
		buf = bt_goep_create_pdu(&server->_goep, NULL);
		if (buf == NULL) {
			LOG_ERR("Failed to allocate buffer");
			return -ENOBUFS;
		}
		allocated = true;
	} else {
		if (bt_obex_has_header(buf, BT_OBEX_HEADER_ID_CONN_ID)) {
			err = ftp_check_conn_id(buf, server->_conn_id);
			if (err != 0) {
				return err;
			}
		}

		if (bt_obex_has_header(buf, BT_OBEX_HEADER_ID_WHO)) {
			err = ftp_check_header_who(buf);
			if (err != 0) {
				return err;
			}
		}
	}

	if (rsp_code == BT_OBEX_RSP_CODE_SUCCESS) {
		if (!bt_obex_has_header(buf, BT_OBEX_HEADER_ID_CONN_ID)) {
			err = bt_obex_add_header_conn_id(buf, server->_conn_id);
			if (err != 0) {
				LOG_ERR("Failed to add header conn id: %d", err);
				goto failed;
			}
		}

		if (!bt_obex_has_header(buf, BT_OBEX_HEADER_ID_WHO)) {
			sys_memcpy_swap(val, ftp_uuid->val, sizeof(val));
			err = bt_obex_add_header_who(buf, sizeof(val), val);
			if (err != 0) {
				LOG_ERR("Failed to add header who: %d", err);
				goto failed;
			}
		}
	}

	err = bt_obex_connect_rsp(&server->_server, rsp_code, server->_goep.obex.rx.mtu, buf);
	if (err != 0) {
		LOG_ERR("Failed to send CONNECT response: %d", err);
		goto failed;
	}

	if (rsp_code == BT_OBEX_RSP_CODE_SUCCESS) {
		atomic_set(&server->_state, BT_FTP_STATE_CONNECTED);
	} else {
		atomic_set(&server->_state, BT_FTP_STATE_DISCONNECTED);
	}
	return 0;

failed:
	if (allocated) {
		net_buf_unref(buf);
	}
	return err;
}

int bt_ftp_server_disconnect(struct bt_ftp_server *server, uint8_t rsp_code, struct net_buf *buf)
{
	int err;

	if (server == NULL) {
		return -EINVAL;
	}

	if (atomic_get(&server->_state) != BT_FTP_STATE_DISCONNECTING) {
		LOG_ERR("Invalid OBEX state %u", (uint8_t)atomic_get(&server->_state));
		return -EINVAL;
	}

	err = bt_obex_disconnect_rsp(&server->_server, rsp_code, buf);
	if (err != 0) {
		LOG_ERR("Failed to send DISCONNECT response: %d", err);
		return err;
	}

	if (rsp_code == BT_OBEX_RSP_CODE_SUCCESS) {
		atomic_set(&server->_state, BT_FTP_STATE_DISCONNECTED);
	} else {
		atomic_set(&server->_state, BT_FTP_STATE_CONNECTED);
	}
	return 0;
}

int bt_ftp_server_abort(struct bt_ftp_server *server, uint8_t rsp_code, struct net_buf *buf)
{
	int err;

	if (server == NULL) {
		return -EINVAL;
	}

	if (atomic_get(&server->_state) != BT_FTP_STATE_CONNECTED) {
		LOG_ERR("Invalid OBEX state %u", (uint8_t)atomic_get(&server->_state));
		return -EINVAL;
	}

	err = bt_obex_abort_rsp(&server->_server, rsp_code, buf);
	if (err != 0) {
		LOG_ERR("Failed to send ABORT response: %d", err);
	}

	if (rsp_code == BT_OBEX_RSP_CODE_SUCCESS) {
		ftp_server_clear_pending(server);
	}

	return err;
}

int bt_ftp_server_set_folder(struct bt_ftp_server *server, uint8_t rsp_code, struct net_buf *buf)
{
	int err;

	if (server == NULL) {
		return -EINVAL;
	}

	if (atomic_get(&server->_state) != BT_FTP_STATE_CONNECTED) {
		LOG_ERR("Invalid OBEX state %u", (uint8_t)atomic_get(&server->_state));
		return -EINVAL;
	}

	err = bt_obex_setpath_rsp(&server->_server, rsp_code, buf);
	if (err != 0) {
		LOG_ERR("Failed to send SETPATH response: %d", err);
	}

	return err;
}

int bt_ftp_server_pull_folder_listing(struct bt_ftp_server *server, uint8_t rsp_code,
				      struct net_buf *buf)
{
	int err;

	if (server == NULL) {
		return -EINVAL;
	}

	if (atomic_get(&server->_state) != BT_FTP_STATE_CONNECTED) {
		LOG_ERR("Invalid OBEX state %u", (uint8_t)atomic_get(&server->_state));
		return -EINVAL;
	}

	if (atomic_get(&server->_optype) != BT_FTP_OP_PULL_FOLDER_LISTING) {
		LOG_ERR("Invalid operation type");
		return -EINVAL;
	}

	err = bt_obex_get_rsp(&server->_server, rsp_code, buf);
	if (err != 0) {
		LOG_ERR("Failed to send GET (folder listing) response: %d", err);
		return err;
	}

	if (rsp_code != BT_OBEX_RSP_CODE_CONTINUE) {
		ftp_server_clear_pending(server);
	}

	return 0;
}

int bt_ftp_server_push_file(struct bt_ftp_server *server, uint8_t rsp_code, struct net_buf *buf)
{
	int err;

	if (server == NULL) {
		return -EINVAL;
	}

	if (atomic_get(&server->_state) != BT_FTP_STATE_CONNECTED) {
		LOG_ERR("Invalid OBEX state %u", (uint8_t)atomic_get(&server->_state));
		return -EINVAL;
	}

	if (atomic_get(&server->_optype) != BT_FTP_OP_PUSH_FILE) {
		LOG_ERR("Invalid operation type");
		return -EINVAL;
	}

	err = bt_obex_put_rsp(&server->_server, rsp_code, buf);
	if (err != 0) {
		LOG_ERR("Failed to send PUT (push file) response: %d", err);
		return err;
	}

	if (rsp_code != BT_OBEX_RSP_CODE_CONTINUE) {
		ftp_server_clear_pending(server);
	}

	return 0;
}

int bt_ftp_server_pull_file(struct bt_ftp_server *server, uint8_t rsp_code, struct net_buf *buf)
{
	int err;

	if (server == NULL) {
		return -EINVAL;
	}

	if (atomic_get(&server->_state) != BT_FTP_STATE_CONNECTED) {
		LOG_ERR("Invalid OBEX state %u", (uint8_t)atomic_get(&server->_state));
		return -EINVAL;
	}

	if (atomic_get(&server->_optype) != BT_FTP_OP_PULL_FILE) {
		LOG_ERR("Invalid operation type");
		return -EINVAL;
	}

	err = bt_obex_get_rsp(&server->_server, rsp_code, buf);
	if (err != 0) {
		LOG_ERR("Failed to send GET (pull file) response: %d", err);
		return err;
	}

	if (rsp_code != BT_OBEX_RSP_CODE_CONTINUE) {
		ftp_server_clear_pending(server);
	}

	return 0;
}

int bt_ftp_server_delete(struct bt_ftp_server *server, uint8_t rsp_code, struct net_buf *buf)
{
	int err;

	if (server == NULL) {
		return -EINVAL;
	}

	if (atomic_get(&server->_state) != BT_FTP_STATE_CONNECTED) {
		LOG_ERR("Invalid OBEX state %u", (uint8_t)atomic_get(&server->_state));
		return -EINVAL;
	}

	if (atomic_get(&server->_optype) != BT_FTP_OP_DELETE) {
		LOG_ERR("Invalid operation type");
		return -EINVAL;
	}

	err = bt_obex_put_rsp(&server->_server, rsp_code, buf);
	if (err != 0) {
		LOG_ERR("Failed to send PUT (delete) response: %d", err);
		return err;
	}

	if (rsp_code != BT_OBEX_RSP_CODE_CONTINUE) {
		ftp_server_clear_pending(server);
	}

	return 0;
}

int bt_ftp_server_rename(struct bt_ftp_server *server, uint8_t rsp_code, struct net_buf *buf)
{
	int err;

	if (server == NULL) {
		return -EINVAL;
	}

	if (atomic_get(&server->_state) != BT_FTP_STATE_CONNECTED) {
		LOG_ERR("Invalid OBEX state %u", (uint8_t)atomic_get(&server->_state));
		return -EINVAL;
	}

	if (atomic_get(&server->_optype) != BT_FTP_OP_RENAME) {
		LOG_ERR("Invalid operation type");
		return -EINVAL;
	}

	err = bt_obex_action_rsp(&server->_server, rsp_code, buf);
	if (err != 0) {
		LOG_ERR("Failed to send ACTION (rename) response: %d", err);
		return err;
	}

	if (rsp_code != BT_OBEX_RSP_CODE_CONTINUE) {
		ftp_server_clear_pending(server);
	}

	return 0;
}

int bt_ftp_server_copy(struct bt_ftp_server *server, uint8_t rsp_code, struct net_buf *buf)
{
	int err;

	if (server == NULL) {
		return -EINVAL;
	}

	if (atomic_get(&server->_state) != BT_FTP_STATE_CONNECTED) {
		LOG_ERR("Invalid OBEX state %u", (uint8_t)atomic_get(&server->_state));
		return -EINVAL;
	}

	if (atomic_get(&server->_optype) != BT_FTP_OP_COPY) {
		LOG_ERR("Invalid operation type");
		return -EINVAL;
	}

	err = bt_obex_action_rsp(&server->_server, rsp_code, buf);
	if (err != 0) {
		LOG_ERR("Failed to send ACTION (copy) response: %d", err);
		return err;
	}

	if (rsp_code != BT_OBEX_RSP_CODE_CONTINUE) {
		ftp_server_clear_pending(server);
	}

	return 0;
}

int bt_ftp_server_set_permission(struct bt_ftp_server *server, uint8_t rsp_code,
				 struct net_buf *buf)
{
	int err;

	if (server == NULL) {
		return -EINVAL;
	}

	if (atomic_get(&server->_state) != BT_FTP_STATE_CONNECTED) {
		LOG_ERR("Invalid OBEX state %u", (uint8_t)atomic_get(&server->_state));
		return -EINVAL;
	}

	if (atomic_get(&server->_optype) != BT_FTP_OP_SET_PERMISSION) {
		LOG_ERR("Invalid operation type");
		return -EINVAL;
	}

	err = bt_obex_action_rsp(&server->_server, rsp_code, buf);
	if (err != 0) {
		LOG_ERR("Failed to send ACTION (set permission) response: %d", err);
		return err;
	}

	if (rsp_code != BT_OBEX_RSP_CODE_CONTINUE) {
		ftp_server_clear_pending(server);
	}

	return 0;
}

struct net_buf *bt_ftp_server_create_pdu(struct bt_ftp_server *server, struct net_buf_pool *pool)
{
	if (server == NULL) {
		LOG_ERR("Invalid parameter");
		return NULL;
	}

	return bt_goep_create_pdu(&server->_goep, pool);
}
#endif /* CONFIG_BT_FTP_SERVER */

int bt_ftp_calculate_nonce(const uint8_t *pwd, uint8_t nonce[BT_OBEX_CHALLENGE_TAG_NONCE_LEN])
{
	int64_t timestamp = k_uptime_get();
	uint8_t hash_input[CONFIG_BT_FTP_PWD_MAX_LEN + 1U + sizeof(timestamp)];
	size_t len;
	uint16_t pwd_len;
	int err;

	if (pwd == NULL) {
		LOG_ERR("Password is NULL");
		return -EINVAL;
	}

	if (nonce == NULL) {
		LOG_ERR("Nonce is NULL");
		return -EINVAL;
	}

	pwd_len = strlen(pwd);
	if (pwd_len == 0 || pwd_len > CONFIG_BT_FTP_PWD_MAX_LEN) {
		LOG_ERR("Password length is invalid");
		return -EINVAL;
	}

	memcpy(hash_input, &timestamp, sizeof(timestamp));
	hash_input[sizeof(timestamp)] = ':';
	memcpy(hash_input + sizeof(timestamp) + 1U, pwd, pwd_len);
	err = psa_hash_compute(PSA_ALG_MD5, (const unsigned char *)hash_input,
			       sizeof(timestamp) + 1U + pwd_len, nonce,
			       BT_OBEX_CHALLENGE_TAG_NONCE_LEN, &len);
	if (err != 0) {
		LOG_ERR("Generate nonce failed: %d", err);
		return err;
	}
	return 0;
}

int bt_ftp_calculate_rsp_digest(const uint8_t *pwd,
				const uint8_t nonce[BT_OBEX_CHALLENGE_TAG_NONCE_LEN],
				uint8_t rsp_digest[BT_OBEX_RESPONSE_TAG_REQ_DIGEST_LEN])
{
	uint8_t hash_input[CONFIG_BT_FTP_PWD_MAX_LEN + BT_OBEX_CHALLENGE_TAG_NONCE_LEN + 1U];
	size_t len;
	uint16_t pwd_len;
	int err;

	if (pwd == NULL) {
		LOG_ERR("Password is NULL");
		return -EINVAL;
	}

	if (nonce == NULL) {
		LOG_ERR("Nonce is NULL");
		return -EINVAL;
	}

	if (rsp_digest == NULL) {
		LOG_ERR("Response digest is NULL");
		return -EINVAL;
	}

	pwd_len = strlen(pwd);
	if (pwd_len == 0 || pwd_len > CONFIG_BT_FTP_PWD_MAX_LEN) {
		LOG_ERR("Password length is invalid");
		return -EINVAL;
	}

	memcpy(hash_input, nonce, BT_OBEX_CHALLENGE_TAG_NONCE_LEN);
	hash_input[BT_OBEX_CHALLENGE_TAG_NONCE_LEN] = ':';
	memcpy(hash_input + BT_OBEX_CHALLENGE_TAG_NONCE_LEN + 1U, pwd, pwd_len);

	err = psa_hash_compute(PSA_ALG_MD5, (const unsigned char *)hash_input,
			       BT_OBEX_CHALLENGE_TAG_NONCE_LEN + 1U + pwd_len, rsp_digest,
			       BT_OBEX_RESPONSE_TAG_REQ_DIGEST_LEN, &len);
	if (err != 0) {
		LOG_ERR("Generate response digest failed: %d", err);
		return err;
	}
	return 0;
}

int bt_ftp_verify_authentication(uint8_t nonce[BT_OBEX_CHALLENGE_TAG_NONCE_LEN],
				 uint8_t rsp_digest[BT_OBEX_RESPONSE_TAG_REQ_DIGEST_LEN],
				 const uint8_t *pwd)
{
	uint8_t result[BT_OBEX_RESPONSE_TAG_REQ_DIGEST_LEN];
	int err;

	err = bt_ftp_calculate_rsp_digest(pwd, nonce, result);
	if (err == 0) {
		err = memcmp(result, rsp_digest, BT_OBEX_RESPONSE_TAG_REQ_DIGEST_LEN);
		if (err != 0) {
			LOG_ERR("rsp_digest is invalid");
			return -EINVAL;
		}
	}

	return err;
}
