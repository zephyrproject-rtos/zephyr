/** @file
 * @brief Bluetooth GOEP shell module
 *
 * Provide some Bluetooth shell commands that can be useful to applications.
 */
/*
 * Copyright 2024-2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/types.h>
#include <stdlib.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/bluetooth/classic/rfcomm.h>
#include <zephyr/bluetooth/classic/sdp.h>
#include <zephyr/bluetooth/classic/goep.h>

#include <zephyr/shell/shell.h>

#include "host/shell/bt.h"
#include "common/bt_shell_private.h"

#define GOEP_MOPL CONFIG_BT_GOEP_RFCOMM_MTU

NET_BUF_POOL_FIXED_DEFINE(tx_pool, CONFIG_BT_MAX_CONN, BT_RFCOMM_BUF_SIZE(GOEP_MOPL),
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

static uint8_t add_head_buffer[GOEP_MOPL];

struct bt_goep_app {
	struct bt_goep goep;
	struct bt_conn *conn;
	struct net_buf *tx_buf;
};

static struct bt_goep_app goep_app;

static struct bt_goep_transport_rfcomm_server rfcomm_server;
static struct bt_goep_transport_l2cap_server l2cap_server;

#define TLV_COUNT       3
#define TLV_BUFFER_SIZE 64

static struct bt_obex_tlv tlvs[TLV_COUNT];
static uint8_t tlv_buffers[TLV_COUNT][TLV_BUFFER_SIZE];
static uint8_t tlv_count;

static struct bt_goep_app *goep_alloc(struct bt_conn *conn)
{
	if (goep_app.conn) {
		return NULL;
	}

	goep_app.conn = conn;
	return &goep_app;
}

static void goep_free(struct bt_goep_app *goep)
{
	goep->conn = NULL;
}

void goep_transport_connected(struct bt_conn *conn, struct bt_goep *goep)
{
	bt_shell_print("GOEP %p transport connected on %p", goep, conn);
}

void goep_transport_disconnected(struct bt_goep *goep)
{
	struct bt_goep_app *g_app = CONTAINER_OF(goep, struct bt_goep_app, goep);

	goep_free(g_app);
	bt_shell_print("GOEP %p transport disconnected", goep);
}

struct bt_goep_transport_ops goep_transport_ops = {
	.connected = goep_transport_connected,
	.disconnected = goep_transport_disconnected,
};

static bool goep_parse_tlvs_cb(struct bt_obex_tlv *tlv, void *user_data)
{
	bt_shell_print("T %02x L %d", tlv->type, tlv->data_len);
	bt_shell_hexdump(tlv->data, tlv->data_len);

	return true;
}

static bool goep_parse_headers_cb(struct bt_obex_hdr *hdr, void *user_data)
{
	bt_shell_print("HI %02x Len %d", hdr->id, hdr->len);

	switch (hdr->id) {
	case BT_OBEX_HEADER_ID_APP_PARAM:
	case BT_OBEX_HEADER_ID_AUTH_CHALLENGE:
	case BT_OBEX_HEADER_ID_AUTH_RSP:
		int err;

		err = bt_obex_tlv_parse(hdr->len, hdr->data, goep_parse_tlvs_cb, NULL);
		if (err) {
			bt_shell_error("Fail to parse OBEX TLV triplet");
		}
		break;
	default:
		bt_shell_hexdump(hdr->data, hdr->len);
		break;
	}

	return true;
}

static int goep_parse_headers(struct net_buf *buf)
{
	int err;

	if (!buf) {
		return 0;
	}

	err = bt_obex_header_parse(buf, goep_parse_headers_cb, NULL);
	if (err) {
		bt_shell_print("Fail to parse OBEX Headers");
	}

	return err;
}

static void goep_server_connect(struct bt_obex *obex, uint8_t version, uint16_t mopl,
				struct net_buf *buf)
{
	bt_shell_print("OBEX %p conn req, version %02x, mopl %04x", obex, version, mopl);
	goep_parse_headers(buf);
}

static void goep_server_disconnect(struct bt_obex *obex, struct net_buf *buf)
{
	bt_shell_print("OBEX %p disconn req", obex);
	goep_parse_headers(buf);
}

static void goep_server_put(struct bt_obex *obex, bool final, struct net_buf *buf)
{
	bt_shell_print("OBEX %p put req, final %s, data len %d", obex,
		    final ? "true" : "false", buf->len);
	goep_parse_headers(buf);
}

static void goep_server_get(struct bt_obex *obex, bool final, struct net_buf *buf)
{
	bt_shell_print("OBEX %p get req, final %s, data len %d", obex,
		    final ? "true" : "false", buf->len);
	goep_parse_headers(buf);
}

static void goep_server_abort(struct bt_obex *obex, struct net_buf *buf)
{
	bt_shell_print("OBEX %p abort req", obex);
	goep_parse_headers(buf);
}

static void goep_server_setpath(struct bt_obex *obex, uint8_t flags, struct net_buf *buf)
{
	bt_shell_print("OBEX %p setpath req, flags %02x, data len %d", obex, flags,
		    buf->len);
	goep_parse_headers(buf);
}

static void goep_server_action(struct bt_obex *obex, bool final, struct net_buf *buf)
{
	bt_shell_print("OBEX %p action req, final %s, data len %d", obex,
		    final ? "true" : "false", buf->len);
	goep_parse_headers(buf);
}

struct bt_obex_server_ops goep_server_ops = {
	.connect = goep_server_connect,
	.disconnect = goep_server_disconnect,
	.put = goep_server_put,
	.get = goep_server_get,
	.abort = goep_server_abort,
	.setpath = goep_server_setpath,
	.action = goep_server_action,
};

static void goep_client_connect(struct bt_obex *obex, uint8_t rsp_code, uint8_t version,
				uint16_t mopl, struct net_buf *buf)
{
	bt_shell_print("OBEX %p conn rsq, rsp_code %s, version %02x, mopl %04x", obex,
		    bt_obex_rsp_code_to_str(rsp_code), version, mopl);
	goep_parse_headers(buf);
}

static void goep_client_disconnect(struct bt_obex *obex, uint8_t rsp_code, struct net_buf *buf)
{
	bt_shell_print("OBEX %p disconn rsq, rsp_code %s", obex,
		    bt_obex_rsp_code_to_str(rsp_code));
	goep_parse_headers(buf);
}

static void goep_client_put(struct bt_obex *obex, uint8_t rsp_code, struct net_buf *buf)
{
	bt_shell_print("OBEX %p put rsq, rsp_code %s, data len %d", obex,
		    bt_obex_rsp_code_to_str(rsp_code), buf->len);
	goep_parse_headers(buf);
}

static void goep_client_get(struct bt_obex *obex, uint8_t rsp_code, struct net_buf *buf)
{
	bt_shell_print("OBEX %p get rsq, rsp_code %s, data len %d", obex,
		    bt_obex_rsp_code_to_str(rsp_code), buf->len);
	goep_parse_headers(buf);
}

static void goep_client_abort(struct bt_obex *obex, uint8_t rsp_code, struct net_buf *buf)
{
	bt_shell_print("OBEX %p abort rsq, rsp_code %s", obex,
		    bt_obex_rsp_code_to_str(rsp_code));
	goep_parse_headers(buf);
}

static void goep_client_setpath(struct bt_obex *obex, uint8_t rsp_code, struct net_buf *buf)
{
	bt_shell_print("OBEX %p setpath rsq, rsp_code %s", obex,
		    bt_obex_rsp_code_to_str(rsp_code));
	goep_parse_headers(buf);
}

static void goep_client_action(struct bt_obex *obex, uint8_t rsp_code, struct net_buf *buf)
{
	bt_shell_print("OBEX %p action rsq, rsp_code %s, data len %d", obex,
		    bt_obex_rsp_code_to_str(rsp_code), buf->len);
	goep_parse_headers(buf);
}

struct bt_obex_client_ops goep_client_ops = {
	.connect = goep_client_connect,
	.disconnect = goep_client_disconnect,
	.put = goep_client_put,
	.get = goep_client_get,
	.abort = goep_client_abort,
	.setpath = goep_client_setpath,
	.action = goep_client_action,
};

static int rfcomm_accept(struct bt_conn *conn, struct bt_goep_transport_rfcomm_server *server,
			 struct bt_goep **goep)
{
	struct bt_goep_app *g_app;

	g_app = goep_alloc(conn);
	if (!g_app) {
		bt_shell_print("Cannot allocate goep instance");
		return -ENOMEM;
	}

	g_app->goep.transport_ops = &goep_transport_ops;
	g_app->goep.obex.server_ops = &goep_server_ops;
	*goep = &g_app->goep;
	return 0;
}

static int cmd_register_rfcomm(const struct shell *sh, size_t argc, char *argv[])
{
	int err;
	uint8_t channel;

	if (rfcomm_server.rfcomm.channel) {
		shell_error(sh, "RFCOMM has been registered");
		return -EBUSY;
	}

	channel = (uint8_t)strtoul(argv[1], NULL, 16);

	rfcomm_server.rfcomm.channel = channel;
	rfcomm_server.accept = rfcomm_accept;
	err = bt_goep_transport_rfcomm_server_register(&rfcomm_server);
	if (err) {
		shell_error(sh, "Fail to register RFCOMM server (error %d)", err);
		rfcomm_server.rfcomm.channel = 0;
		return -ENOEXEC;
	}
	shell_print(sh, "RFCOMM server (channel %02x) is registered", rfcomm_server.rfcomm.channel);
	return 0;
}

static int cmd_connect_rfcomm(const struct shell *sh, size_t argc, char *argv[])
{
	int err;
	struct bt_goep_app *g_app;
	uint8_t channel;

	if (!default_conn) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	channel = (uint8_t)strtoul(argv[1], NULL, 16);
	if (!channel) {
		shell_error(sh, "Invalid channel");
		return -ENOEXEC;
	}

	g_app = goep_alloc(default_conn);
	if (!g_app) {
		shell_error(sh, "Cannot allocate goep instance");
		return -ENOMEM;
	}

	g_app->goep.transport_ops = &goep_transport_ops;
	g_app->goep.obex.client_ops = &goep_client_ops;

	err = bt_goep_transport_rfcomm_connect(default_conn, &g_app->goep, channel);
	if (err) {
		goep_free(g_app);
		shell_error(sh, "Fail to connect to channel %d (err %d)", channel, err);
	} else {
		shell_print(sh, "GOEP RFCOMM connection pending");
	}

	return err;
}

static int cmd_disconnect_rfcomm(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	if (!default_conn) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	if (!goep_app.conn) {
		shell_error(sh, "No goep transport connection");
		return -ENOEXEC;
	}

	err = bt_goep_transport_rfcomm_disconnect(&goep_app.goep);
	if (err) {
		shell_error(sh, "Fail to disconnect to channel (err %d)", err);
	} else {
		shell_print(sh, "GOEP RFCOMM disconnection pending");
	}
	return err;
}

static int l2cap_accept(struct bt_conn *conn, struct bt_goep_transport_l2cap_server *server,
			struct bt_goep **goep)
{
	struct bt_goep_app *g_app;

	g_app = goep_alloc(conn);
	if (!g_app) {
		bt_shell_print("Cannot allocate goep instance");
		return -ENOMEM;
	}

	g_app->goep.transport_ops = &goep_transport_ops;
	g_app->goep.obex.server_ops = &goep_server_ops;
	*goep = &g_app->goep;
	return 0;
}

static int cmd_register_l2cap(const struct shell *sh, size_t argc, char *argv[])
{
	int err;
	uint16_t psm;

	if (l2cap_server.l2cap.psm) {
		shell_error(sh, "L2CAP server has been registered");
		return -EBUSY;
	}

	psm = (uint16_t)strtoul(argv[1], NULL, 16);

	l2cap_server.l2cap.psm = psm;
	l2cap_server.accept = l2cap_accept;
	err = bt_goep_transport_l2cap_server_register(&l2cap_server);
	if (err) {
		shell_error(sh, "Fail to register L2CAP server (error %d)", err);
		l2cap_server.l2cap.psm = 0;
		return -ENOEXEC;
	}
	shell_print(sh, "L2CAP server (psm %04x) is registered", l2cap_server.l2cap.psm);
	return 0;
}

static int cmd_connect_l2cap(const struct shell *sh, size_t argc, char *argv[])
{
	int err;
	struct bt_goep_app *g_app;
	uint16_t psm;

	if (!default_conn) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	psm = (uint16_t)strtoul(argv[1], NULL, 16);
	if (!psm) {
		shell_error(sh, "Invalid psm");
		return -ENOEXEC;
	}

	g_app = goep_alloc(default_conn);
	if (!g_app) {
		shell_error(sh, "Cannot allocate goep instance");
		return -ENOMEM;
	}

	g_app->goep.transport_ops = &goep_transport_ops;
	g_app->goep.obex.client_ops = &goep_client_ops;

	err = bt_goep_transport_l2cap_connect(default_conn, &g_app->goep, psm);
	if (err) {
		goep_free(g_app);
		shell_error(sh, "Fail to connect to PSM %d (err %d)", psm, err);
	} else {
		shell_print(sh, "GOEP L2CAP connection pending");
	}

	return err;
}

static int cmd_disconnect_l2cap(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	if (!default_conn) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	if (!goep_app.conn) {
		shell_error(sh, "No goep transport connection");
		return -ENOEXEC;
	}

	err = bt_goep_transport_l2cap_disconnect(&goep_app.goep);
	if (err) {
		shell_error(sh, "Fail to disconnect L2CAP conn (err %d)", err);
	} else {
		shell_print(sh, "GOEP L2CAP disconnection pending");
	}
	return err;
}

static int cmd_add_header_count(const struct shell *sh, size_t argc, char *argv[])
{
	uint32_t count;
	int err;

	count = strtoul(argv[1], NULL, 16);

	err = bt_obex_add_header_count(goep_app.tx_buf, count);
	if (err) {
		shell_error(sh, "Fail to add header count");
	}
	return err;
}

static int cmd_add_header_name(const struct shell *sh, size_t argc, char *argv[])
{
	size_t len;
	int err;
	const char *hex_payload;
	size_t hex_payload_size;

	if (argc > 1) {
		hex_payload = argv[1];
		hex_payload_size = strlen(hex_payload);

		len = hex2bin(hex_payload, hex_payload_size, add_head_buffer,
			      sizeof(add_head_buffer));
		if (len > UINT16_MAX) {
			shell_error(sh, "Length exceeds max length (%x > %x)", len, UINT16_MAX);
			return -ENOEXEC;
		}
	} else {
		len = 0;
	}

	err = bt_obex_add_header_name(goep_app.tx_buf, (uint16_t)len, add_head_buffer);
	if (err) {
		shell_error(sh, "Fail to add header name");
	}
	return err;
}

static int cmd_add_header_type(const struct shell *sh, size_t argc, char *argv[])
{
	size_t len;
	int err;
	const char *hex_payload = argv[1];
	size_t hex_payload_size = strlen(hex_payload);

	len = hex2bin(hex_payload, hex_payload_size, add_head_buffer, sizeof(add_head_buffer));
	if (len > UINT16_MAX) {
		shell_error(sh, "Length exceeds max length (%x > %x)", len, UINT16_MAX);
		return -ENOEXEC;
	}

	err = bt_obex_add_header_type(goep_app.tx_buf, (uint16_t)len, add_head_buffer);
	if (err) {
		shell_error(sh, "Fail to add header type");
	}
	return err;
}

static int cmd_add_header_len(const struct shell *sh, size_t argc, char *argv[])
{
	uint32_t len;
	int err;

	len = strtoul(argv[1], NULL, 16);

	err = bt_obex_add_header_len(goep_app.tx_buf, len);
	if (err) {
		shell_error(sh, "Fail to add header len");
	}
	return err;
}

static int cmd_add_header_time_iso_8601(const struct shell *sh, size_t argc, char *argv[])
{
	size_t len;
	int err;
	const char *hex_payload = argv[1];
	size_t hex_payload_size = strlen(hex_payload);

	len = hex2bin(hex_payload, hex_payload_size, add_head_buffer, sizeof(add_head_buffer));
	if (len > UINT16_MAX) {
		shell_error(sh, "Length exceeds max length (%x > %x)", len, UINT16_MAX);
		return -ENOEXEC;
	}

	err = bt_obex_add_header_time_iso_8601(goep_app.tx_buf, (uint16_t)len, add_head_buffer);
	if (err) {
		shell_error(sh, "Fail to add header time_iso_8601");
	}
	return err;
}

static int cmd_add_header_time(const struct shell *sh, size_t argc, char *argv[])
{
	uint32_t t;
	int err;

	t = strtoul(argv[1], NULL, 16);

	err = bt_obex_add_header_time(goep_app.tx_buf, t);
	if (err) {
		shell_error(sh, "Fail to add header time");
	}
	return err;
}

static int cmd_add_header_description(const struct shell *sh, size_t argc, char *argv[])
{
	size_t len;
	int err;
	const char *hex_payload = argv[1];
	size_t hex_payload_size = strlen(hex_payload);

	len = hex2bin(hex_payload, hex_payload_size, add_head_buffer, sizeof(add_head_buffer));
	if (len > UINT16_MAX) {
		shell_error(sh, "Length exceeds max length (%x > %x)", len, UINT16_MAX);
		return -ENOEXEC;
	}

	err = bt_obex_add_header_description(goep_app.tx_buf, (uint16_t)len, add_head_buffer);
	if (err) {
		shell_error(sh, "Fail to add header description");
	}
	return err;
}

static int cmd_add_header_target(const struct shell *sh, size_t argc, char *argv[])
{
	size_t len;
	int err;
	const char *hex_payload = argv[1];
	size_t hex_payload_size = strlen(hex_payload);

	len = hex2bin(hex_payload, hex_payload_size, add_head_buffer, sizeof(add_head_buffer));
	if (len > UINT16_MAX) {
		shell_error(sh, "Length exceeds max length (%x > %x)", len, UINT16_MAX);
		return -ENOEXEC;
	}

	err = bt_obex_add_header_target(goep_app.tx_buf, (uint16_t)len, add_head_buffer);
	if (err) {
		shell_error(sh, "Fail to add header target");
	}
	return err;
}

static int cmd_add_header_http(const struct shell *sh, size_t argc, char *argv[])
{
	size_t len;
	int err;
	const char *hex_payload = argv[1];
	size_t hex_payload_size = strlen(hex_payload);

	len = hex2bin(hex_payload, hex_payload_size, add_head_buffer, sizeof(add_head_buffer));
	if (len > UINT16_MAX) {
		shell_error(sh, "Length exceeds max length (%x > %x)", len, UINT16_MAX);
		return -ENOEXEC;
	}

	err = bt_obex_add_header_http(goep_app.tx_buf, (uint16_t)len, add_head_buffer);
	if (err) {
		shell_error(sh, "Fail to add header http");
	}
	return err;
}

static int cmd_add_header_body(const struct shell *sh, size_t argc, char *argv[])
{
	size_t len;
	int err;
	const char *hex_payload = argv[1];
	size_t hex_payload_size = strlen(hex_payload);

	len = hex2bin(hex_payload, hex_payload_size, add_head_buffer, sizeof(add_head_buffer));
	if (len > UINT16_MAX) {
		shell_error(sh, "Length exceeds max length (%x > %x)", len, UINT16_MAX);
		return -ENOEXEC;
	}

	err = bt_obex_add_header_body(goep_app.tx_buf, (uint16_t)len, add_head_buffer);
	if (err) {
		shell_error(sh, "Fail to add header body");
	}
	return err;
}

static int cmd_add_header_end_body(const struct shell *sh, size_t argc, char *argv[])
{
	size_t len;
	int err;
	const char *hex_payload = argv[1];
	size_t hex_payload_size = strlen(hex_payload);

	len = hex2bin(hex_payload, hex_payload_size, add_head_buffer, sizeof(add_head_buffer));
	if (len > UINT16_MAX) {
		shell_error(sh, "Length exceeds max length (%x > %x)", len, UINT16_MAX);
		return -ENOEXEC;
	}

	err = bt_obex_add_header_end_body(goep_app.tx_buf, (uint16_t)len, add_head_buffer);
	if (err) {
		shell_error(sh, "Fail to add header end_body");
	}
	return err;
}

static int cmd_add_header_who(const struct shell *sh, size_t argc, char *argv[])
{
	size_t len;
	int err;
	const char *hex_payload = argv[1];
	size_t hex_payload_size = strlen(hex_payload);

	len = hex2bin(hex_payload, hex_payload_size, add_head_buffer, sizeof(add_head_buffer));
	if (len > UINT16_MAX) {
		shell_error(sh, "Length exceeds max length (%x > %x)", len, UINT16_MAX);
		return -ENOEXEC;
	}

	err = bt_obex_add_header_who(goep_app.tx_buf, (uint16_t)len, add_head_buffer);
	if (err) {
		shell_error(sh, "Fail to add header who");
	}
	return err;
}

static int cmd_add_header_conn_id(const struct shell *sh, size_t argc, char *argv[])
{
	uint32_t conn_id;
	int err;

	conn_id = strtoul(argv[1], NULL, 16);

	err = bt_obex_add_header_conn_id(goep_app.tx_buf, conn_id);
	if (err) {
		shell_error(sh, "Fail to add header conn_id");
	}
	return err;
}

static int cmd_add_header_app_param(const struct shell *sh, size_t argc, char *argv[])
{
	size_t len;
	int err;
	uint8_t tag;
	bool last = false;

	if (tlv_count >= TLV_COUNT) {
		shell_warn(sh, "No space of TLV array, add app_param and clear tlvs");
		goto add_header;
	}

	len = hex2bin(argv[1], strlen(argv[1]), &tag, sizeof(tag));
	if (len < 1) {
		shell_error(sh, "Length should not be zero");
		return -ENOEXEC;
	}

	len = hex2bin(argv[2], strlen(argv[2]), &tlv_buffers[tlv_count][0], TLV_BUFFER_SIZE);
	if (len > UINT16_MAX) {
		shell_error(sh, "Length exceeds max length (%x > %x)", len, UINT16_MAX);
		return -ENOEXEC;
	}

	if (argc > 3) {
		if (!strcmp(argv[3], "last")) {
			last = true;
		}
	}

	tlvs[tlv_count].type = tag;
	tlvs[tlv_count].data_len = len;
	tlvs[tlv_count].data = &tlv_buffers[tlv_count][0];

	tlv_count++;

	if (!last) {
		return 0;
	}

add_header:
	err = bt_obex_add_header_app_param(goep_app.tx_buf, (size_t)tlv_count, tlvs);
	if (err) {
		shell_error(sh, "Fail to add header app_param");
	}
	tlv_count = 0;
	return err;
}

static int cmd_add_header_auth_challenge(const struct shell *sh, size_t argc, char *argv[])
{
	size_t len;
	int err;
	uint8_t tag;
	bool last = false;

	if (tlv_count >= TLV_COUNT) {
		shell_warn(sh, "No space of TLV array, add auth_challenge and clear tlvs");
		goto add_header;
	}

	len = hex2bin(argv[1], strlen(argv[1]), &tag, sizeof(tag));
	if (len < 1) {
		shell_error(sh, "Length should not be zero");
		return -ENOEXEC;
	}

	len = hex2bin(argv[2], strlen(argv[2]), &tlv_buffers[tlv_count][0], TLV_BUFFER_SIZE);
	if (len > UINT16_MAX) {
		shell_error(sh, "Length exceeds max length (%x > %x)", len, UINT16_MAX);
		return -ENOEXEC;
	}

	if (argc > 3) {
		if (!strcmp(argv[3], "last")) {
			last = true;
		}
	}

	tlvs[tlv_count].type = tag;
	tlvs[tlv_count].data_len = len;
	tlvs[tlv_count].data = &tlv_buffers[tlv_count][0];

	tlv_count++;

	if (!last) {
		return 0;
	}

add_header:
	err = bt_obex_add_header_auth_challenge(goep_app.tx_buf, (size_t)tlv_count, tlvs);
	if (err) {
		shell_error(sh, "Fail to add header auth_challenge");
	}
	tlv_count = 0;
	return err;
}

static int cmd_add_header_auth_rsp(const struct shell *sh, size_t argc, char *argv[])
{
	size_t len;
	int err;
	uint8_t tag;
	bool last = false;

	if (tlv_count >= TLV_COUNT) {
		shell_warn(sh, "No space of TLV array, add auth_rsp and clear tlvs");
		goto add_header;
	}

	len = hex2bin(argv[1], strlen(argv[1]), &tag, sizeof(tag));
	if (len < 1) {
		shell_error(sh, "Length should not be zero");
		return -ENOEXEC;
	}

	len = hex2bin(argv[2], strlen(argv[2]), &tlv_buffers[tlv_count][0], TLV_BUFFER_SIZE);
	if (len > UINT16_MAX) {
		shell_error(sh, "Length exceeds max length (%x > %x)", len, UINT16_MAX);
		return -ENOEXEC;
	}

	if (argc > 3) {
		if (!strcmp(argv[3], "last")) {
			last = true;
		}
	}

	tlvs[tlv_count].type = tag;
	tlvs[tlv_count].data_len = len;
	tlvs[tlv_count].data = &tlv_buffers[tlv_count][0];

	tlv_count++;

	if (!last) {
		return 0;
	}

add_header:
	err = bt_obex_add_header_auth_rsp(goep_app.tx_buf, (size_t)tlv_count, tlvs);
	if (err) {
		shell_error(sh, "Fail to add header auth_rsp");
	}
	tlv_count = 0;
	return err;
}

static int cmd_add_header_creator_id(const struct shell *sh, size_t argc, char *argv[])
{
	uint32_t creator_id;
	int err;

	creator_id = strtoul(argv[1], NULL, 16);

	err = bt_obex_add_header_creator_id(goep_app.tx_buf, creator_id);
	if (err) {
		shell_error(sh, "Fail to add header creator_id");
	}
	return err;
}

static int cmd_add_header_wan_uuid(const struct shell *sh, size_t argc, char *argv[])
{
	size_t len;
	int err;
	const char *hex_payload = argv[1];
	size_t hex_payload_size = strlen(hex_payload);

	len = hex2bin(hex_payload, hex_payload_size, add_head_buffer, sizeof(add_head_buffer));
	if (len > UINT16_MAX) {
		shell_error(sh, "Length exceeds max length (%x > %x)", len, UINT16_MAX);
		return -ENOEXEC;
	}

	err = bt_obex_add_header_wan_uuid(goep_app.tx_buf, (uint16_t)len, add_head_buffer);
	if (err) {
		shell_error(sh, "Fail to add header wan_uuid");
	}
	return err;
}

static int cmd_add_header_obj_class(const struct shell *sh, size_t argc, char *argv[])
{
	size_t len;
	int err;
	const char *hex_payload = argv[1];
	size_t hex_payload_size = strlen(hex_payload);

	len = hex2bin(hex_payload, hex_payload_size, add_head_buffer, sizeof(add_head_buffer));
	if (len > UINT16_MAX) {
		shell_error(sh, "Length exceeds max length (%x > %x)", len, UINT16_MAX);
		return -ENOEXEC;
	}

	err = bt_obex_add_header_obj_class(goep_app.tx_buf, (uint16_t)len, add_head_buffer);
	if (err) {
		shell_error(sh, "Fail to add header obj_class");
	}
	return err;
}

static int cmd_add_header_session_param(const struct shell *sh, size_t argc, char *argv[])
{
	size_t len;
	int err;
	const char *hex_payload = argv[1];
	size_t hex_payload_size = strlen(hex_payload);

	len = hex2bin(hex_payload, hex_payload_size, add_head_buffer, sizeof(add_head_buffer));
	if (len > UINT16_MAX) {
		shell_error(sh, "Length exceeds max length (%x > %x)", len, UINT16_MAX);
		return -ENOEXEC;
	}

	err = bt_obex_add_header_session_param(goep_app.tx_buf, (uint16_t)len, add_head_buffer);
	if (err) {
		shell_error(sh, "Fail to add header session_param");
	}
	return err;
}

static int cmd_add_header_session_seq_number(const struct shell *sh, size_t argc, char *argv[])
{
	uint32_t session_seq_number;
	int err;

	session_seq_number = strtoul(argv[1], NULL, 16);

	err = bt_obex_add_header_session_seq_number(goep_app.tx_buf, session_seq_number);
	if (err) {
		shell_error(sh, "Fail to add header session_seq_number");
	}
	return err;
}

static int cmd_add_header_action_id(const struct shell *sh, size_t argc, char *argv[])
{
	uint32_t action_id;
	int err;

	action_id = strtoul(argv[1], NULL, 16);

	err = bt_obex_add_header_action_id(goep_app.tx_buf, action_id);
	if (err) {
		shell_error(sh, "Fail to add header action_id");
	}
	return err;
}

static int cmd_add_header_dest_name(const struct shell *sh, size_t argc, char *argv[])
{
	size_t len;
	int err;
	const char *hex_payload = argv[1];
	size_t hex_payload_size = strlen(hex_payload);

	len = hex2bin(hex_payload, hex_payload_size, add_head_buffer, sizeof(add_head_buffer));
	if (len > UINT16_MAX) {
		shell_error(sh, "Length exceeds max length (%x > %x)", len, UINT16_MAX);
		return -ENOEXEC;
	}

	err = bt_obex_add_header_dest_name(goep_app.tx_buf, (uint16_t)len, add_head_buffer);
	if (err) {
		shell_error(sh, "Fail to add header dest_name");
	}
	return err;
}

static int cmd_add_header_perm(const struct shell *sh, size_t argc, char *argv[])
{
	uint32_t perm;
	int err;

	perm = strtoul(argv[1], NULL, 16);

	err = bt_obex_add_header_perm(goep_app.tx_buf, perm);
	if (err) {
		shell_error(sh, "Fail to add header perm");
	}
	return err;
}

static int cmd_add_header_srm(const struct shell *sh, size_t argc, char *argv[])
{
	uint32_t srm;
	int err;

	srm = strtoul(argv[1], NULL, 16);

	if (srm > UINT8_MAX) {
		shell_error(sh, "Value exceeds max value (%x > %x)", srm, UINT8_MAX);
		return -ENOEXEC;
	}

	err = bt_obex_add_header_srm(goep_app.tx_buf, (uint8_t)srm);
	if (err) {
		shell_error(sh, "Fail to add header srm");
	}
	return err;
}

static int cmd_add_header_srm_param(const struct shell *sh, size_t argc, char *argv[])
{
	uint32_t srm_param;
	int err;

	srm_param = strtoul(argv[1], NULL, 16);

	if (srm_param > UINT8_MAX) {
		shell_error(sh, "Value exceeds max value (%x > %x)", srm_param, UINT8_MAX);
		return -ENOEXEC;
	}

	err = bt_obex_add_header_srm_param(goep_app.tx_buf, (uint8_t)srm_param);
	if (err) {
		shell_error(sh, "Fail to add header srm_param");
	}
	return err;
}

static int cmd_goep_client_conn(const struct shell *sh, size_t argc, char *argv[])
{
	uint16_t mopl;
	int err;

	if (!default_conn) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	if (!goep_app.conn) {
		shell_error(sh, "No goep transport connection");
		return -ENOEXEC;
	}

	mopl = (uint16_t)strtoul(argv[1], NULL, 16);

	err = bt_obex_connect(&goep_app.goep.obex, mopl, goep_app.tx_buf);
	if (err) {
		shell_error(sh, "Fail to send conn req %d", err);
	} else {
		goep_app.tx_buf = NULL;
	}
	return err;
}

static int cmd_goep_client_disconn(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	if (!default_conn) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	if (!goep_app.conn) {
		shell_error(sh, "No goep transport connection");
		return -ENOEXEC;
	}

	err = bt_obex_disconnect(&goep_app.goep.obex, goep_app.tx_buf);
	if (err) {
		shell_error(sh, "Fail to send disconn req %d", err);
	} else {
		goep_app.tx_buf = NULL;
	}
	return err;
}

static int cmd_goep_client_put(const struct shell *sh, size_t argc, char *argv[])
{
	bool final;
	int err;

	if (!default_conn) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	if (!goep_app.conn) {
		shell_error(sh, "No goep transport connection");
		return -ENOEXEC;
	}

	if (!strcmp(argv[1], "yes")) {
		final = true;
	} else if (!strcmp(argv[1], "no")) {
		final = false;
	} else {
		shell_help(sh);
		return SHELL_CMD_HELP_PRINTED;
	}

	err = bt_obex_put(&goep_app.goep.obex, final, goep_app.tx_buf);
	if (err) {
		shell_error(sh, "Fail to send put req %d", err);
	} else {
		goep_app.tx_buf = NULL;
	}
	return err;
}

static int cmd_goep_client_get(const struct shell *sh, size_t argc, char *argv[])
{
	bool final;
	int err;

	if (!default_conn) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	if (!goep_app.conn) {
		shell_error(sh, "No goep transport connection");
		return -ENOEXEC;
	}

	if (!strcmp(argv[1], "yes")) {
		final = true;
	} else if (!strcmp(argv[1], "no")) {
		final = false;
	} else {
		shell_help(sh);
		return SHELL_CMD_HELP_PRINTED;
	}

	err = bt_obex_get(&goep_app.goep.obex, final, goep_app.tx_buf);
	if (err) {
		shell_error(sh, "Fail to send get req %d", err);
	} else {
		goep_app.tx_buf = NULL;
	}
	return err;
}

static int cmd_goep_client_abort(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	if (!default_conn) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	if (!goep_app.conn) {
		shell_error(sh, "No goep transport connection");
		return -ENOEXEC;
	}

	err = bt_obex_abort(&goep_app.goep.obex, goep_app.tx_buf);
	if (err) {
		shell_error(sh, "Fail to send abort req %d", err);
	} else {
		goep_app.tx_buf = NULL;
	}
	return err;
}

static int cmd_goep_client_setpath(const struct shell *sh, size_t argc, char *argv[])
{
	int err;
	uint8_t flags = BIT(1);

	if (!default_conn) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	if (!goep_app.conn) {
		shell_error(sh, "No goep transport connection");
		return -ENOEXEC;
	}

	for (size_t index = 1; index < argc; index++) {
		if (!strcmp(argv[index], "parent")) {
			flags |= BIT(0);
		} else if (!strcmp(argv[index], "create")) {
			flags &= ~BIT(1);
		}
	}

	err = bt_obex_setpath(&goep_app.goep.obex, flags, goep_app.tx_buf);
	if (err) {
		shell_error(sh, "Fail to send setpath req %d", err);
	} else {
		goep_app.tx_buf = NULL;
	}
	return err;
}

static int cmd_goep_client_action(const struct shell *sh, size_t argc, char *argv[])
{
	bool final;
	int err;

	if (!default_conn) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	if (!goep_app.conn) {
		shell_error(sh, "No goep transport connection");
		return -ENOEXEC;
	}

	if (!strcmp(argv[1], "yes")) {
		final = true;
	} else if (!strcmp(argv[1], "no")) {
		final = false;
	} else {
		shell_help(sh);
		return SHELL_CMD_HELP_PRINTED;
	}

	err = bt_obex_action(&goep_app.goep.obex, final, goep_app.tx_buf);
	if (err) {
		shell_error(sh, "Fail to send action req %d", err);
	} else {
		goep_app.tx_buf = NULL;
	}
	return err;
}

static int cmd_goep_server_conn(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t rsp_code;
	uint16_t mopl;
	const char *rsp;
	int err;

	if (!default_conn) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	if (!goep_app.conn) {
		shell_error(sh, "No goep transport connection");
		return -ENOEXEC;
	}

	rsp = argv[1];
	if (!strcmp(rsp, "continue")) {
		rsp_code = BT_OBEX_RSP_CODE_CONTINUE;
	} else if (!strcmp(rsp, "success")) {
		rsp_code = BT_OBEX_RSP_CODE_SUCCESS;
	} else if (!strcmp(rsp, "error")) {
		if (argc < 4) {
			shell_error(sh, "[rsp_code] is needed if the rsp is %s", rsp);
			shell_help(sh);
			return SHELL_CMD_HELP_PRINTED;
		}
		rsp_code = (uint8_t)strtoul(argv[3], NULL, 16);
	} else {
		shell_help(sh);
		return SHELL_CMD_HELP_PRINTED;
	}

	mopl = (uint16_t)strtoul(argv[2], NULL, 16);

	err = bt_obex_connect_rsp(&goep_app.goep.obex, rsp_code, mopl, goep_app.tx_buf);
	if (err) {
		shell_error(sh, "Fail to send conn rsp %d", err);
	} else {
		goep_app.tx_buf = NULL;
	}
	return err;
}

static int cmd_goep_server_disconn(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t rsp_code;
	const char *rsp;
	int err;

	if (!default_conn) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	if (!goep_app.conn) {
		shell_error(sh, "No goep transport connection");
		return -ENOEXEC;
	}

	rsp = argv[1];
	if (!strcmp(rsp, "continue")) {
		rsp_code = BT_OBEX_RSP_CODE_CONTINUE;
	} else if (!strcmp(rsp, "success")) {
		rsp_code = BT_OBEX_RSP_CODE_SUCCESS;
	} else if (!strcmp(rsp, "error")) {
		if (argc < 3) {
			shell_error(sh, "[rsp_code] is needed if the rsp is %s", rsp);
			shell_help(sh);
			return SHELL_CMD_HELP_PRINTED;
		}
		rsp_code = (uint8_t)strtoul(argv[2], NULL, 16);
	} else {
		shell_help(sh);
		return SHELL_CMD_HELP_PRINTED;
	}

	err = bt_obex_disconnect_rsp(&goep_app.goep.obex, rsp_code, goep_app.tx_buf);
	if (err) {
		shell_error(sh, "Fail to send disconn rsp %d", err);
	} else {
		goep_app.tx_buf = NULL;
	}
	return err;
}

static int cmd_goep_server_put(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t rsp_code;
	const char *rsp;
	int err;

	if (!default_conn) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	if (!goep_app.conn) {
		shell_error(sh, "No goep transport connection");
		return -ENOEXEC;
	}

	rsp = argv[1];
	if (!strcmp(rsp, "continue")) {
		rsp_code = BT_OBEX_RSP_CODE_CONTINUE;
	} else if (!strcmp(rsp, "success")) {
		rsp_code = BT_OBEX_RSP_CODE_SUCCESS;
	} else if (!strcmp(rsp, "error")) {
		if (argc < 3) {
			shell_error(sh, "[rsp_code] is needed if the rsp is %s", rsp);
			shell_help(sh);
			return SHELL_CMD_HELP_PRINTED;
		}
		rsp_code = (uint8_t)strtoul(argv[2], NULL, 16);
	} else {
		shell_help(sh);
		return SHELL_CMD_HELP_PRINTED;
	}

	err = bt_obex_put_rsp(&goep_app.goep.obex, rsp_code, goep_app.tx_buf);
	if (err) {
		shell_error(sh, "Fail to send put rsp %d", err);
	} else {
		goep_app.tx_buf = NULL;
	}
	return err;
}

static int cmd_goep_server_get(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t rsp_code;
	const char *rsp;
	int err;

	if (!default_conn) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	if (!goep_app.conn) {
		shell_error(sh, "No goep transport connection");
		return -ENOEXEC;
	}

	rsp = argv[1];
	if (!strcmp(rsp, "continue")) {
		rsp_code = BT_OBEX_RSP_CODE_CONTINUE;
	} else if (!strcmp(rsp, "success")) {
		rsp_code = BT_OBEX_RSP_CODE_SUCCESS;
	} else if (!strcmp(rsp, "error")) {
		if (argc < 3) {
			shell_error(sh, "[rsp_code] is needed if the rsp is %s", rsp);
			shell_help(sh);
			return SHELL_CMD_HELP_PRINTED;
		}
		rsp_code = (uint8_t)strtoul(argv[2], NULL, 16);
	} else {
		shell_help(sh);
		return SHELL_CMD_HELP_PRINTED;
	}

	err = bt_obex_get_rsp(&goep_app.goep.obex, rsp_code, goep_app.tx_buf);
	if (err) {
		shell_error(sh, "Fail to send get rsp %d", err);
	} else {
		goep_app.tx_buf = NULL;
	}
	return err;
}

static int cmd_goep_server_abort(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t rsp_code;
	const char *rsp;
	int err;

	if (!default_conn) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	if (!goep_app.conn) {
		shell_error(sh, "No goep transport connection");
		return -ENOEXEC;
	}

	rsp = argv[1];
	if (!strcmp(rsp, "continue")) {
		rsp_code = BT_OBEX_RSP_CODE_CONTINUE;
	} else if (!strcmp(rsp, "success")) {
		rsp_code = BT_OBEX_RSP_CODE_SUCCESS;
	} else if (!strcmp(rsp, "error")) {
		if (argc < 3) {
			shell_error(sh, "[rsp_code] is needed if the rsp is %s", rsp);
			shell_help(sh);
			return SHELL_CMD_HELP_PRINTED;
		}
		rsp_code = (uint8_t)strtoul(argv[2], NULL, 16);
	} else {
		shell_help(sh);
		return SHELL_CMD_HELP_PRINTED;
	}

	err = bt_obex_abort_rsp(&goep_app.goep.obex, rsp_code, goep_app.tx_buf);
	if (err) {
		shell_error(sh, "Fail to send abort rsp %d", err);
	} else {
		goep_app.tx_buf = NULL;
	}
	return err;
}

static int cmd_goep_server_setpath(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t rsp_code;
	const char *rsp;
	int err;

	if (!default_conn) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	if (!goep_app.conn) {
		shell_error(sh, "No goep transport connection");
		return -ENOEXEC;
	}

	rsp = argv[1];
	if (!strcmp(rsp, "continue")) {
		rsp_code = BT_OBEX_RSP_CODE_CONTINUE;
	} else if (!strcmp(rsp, "success")) {
		rsp_code = BT_OBEX_RSP_CODE_SUCCESS;
	} else if (!strcmp(rsp, "error")) {
		if (argc < 3) {
			shell_error(sh, "[rsp_code] is needed if the rsp is %s", rsp);
			shell_help(sh);
			return SHELL_CMD_HELP_PRINTED;
		}
		rsp_code = (uint8_t)strtoul(argv[2], NULL, 16);
	} else {
		shell_help(sh);
		return SHELL_CMD_HELP_PRINTED;
	}

	err = bt_obex_setpath_rsp(&goep_app.goep.obex, rsp_code, goep_app.tx_buf);
	if (err) {
		shell_error(sh, "Fail to send setpath rsp %d", err);
	} else {
		goep_app.tx_buf = NULL;
	}
	return err;
}

static int cmd_goep_server_action(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t rsp_code;
	const char *rsp;
	int err;

	if (!default_conn) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	if (!goep_app.conn) {
		shell_error(sh, "No goep transport connection");
		return -ENOEXEC;
	}

	rsp = argv[1];
	if (!strcmp(rsp, "continue")) {
		rsp_code = BT_OBEX_RSP_CODE_CONTINUE;
	} else if (!strcmp(rsp, "success")) {
		rsp_code = BT_OBEX_RSP_CODE_SUCCESS;
	} else if (!strcmp(rsp, "error")) {
		if (argc < 3) {
			shell_error(sh, "[rsp_code] is needed if the rsp is %s", rsp);
			shell_help(sh);
			return SHELL_CMD_HELP_PRINTED;
		}
		rsp_code = (uint8_t)strtoul(argv[2], NULL, 16);
	} else {
		shell_help(sh);
		return SHELL_CMD_HELP_PRINTED;
	}

	err = bt_obex_action_rsp(&goep_app.goep.obex, rsp_code, goep_app.tx_buf);
	if (err) {
		shell_error(sh, "Fail to send action rsp %d", err);
	} else {
		goep_app.tx_buf = NULL;
	}
	return err;
}

#define HELP_NONE ""

SHELL_STATIC_SUBCMD_SET_CREATE(obex_add_header_cmds,
	SHELL_CMD_ARG(count, NULL, "<number of objects (used by Connect)>", cmd_add_header_count, 2,
		      0),
	SHELL_CMD_ARG(name, NULL, "[name of the object (often a file name)]", cmd_add_header_name,
		      1, 1),
	SHELL_CMD_ARG(type, NULL,
		      "<type of object - e.g. text, html, binary, manufacturer specific>",
		      cmd_add_header_type, 2, 0),
	SHELL_CMD_ARG(len, NULL, "<length of the object in bytes>", cmd_add_header_len, 2, 0),
	SHELL_CMD_ARG(time_iso_8601, NULL, "<date/time stamp - ISO 8601 version - preferred>",
		      cmd_add_header_time_iso_8601, 2, 0),
	SHELL_CMD_ARG(time, NULL, "<date/time stamp - 4 byte version (for compatibility only)>",
		      cmd_add_header_time, 2, 0),
	SHELL_CMD_ARG(description, NULL, "<text description of the object>",
		      cmd_add_header_description, 2, 0),
	SHELL_CMD_ARG(target, NULL, "<name of service that operation is targeted to>",
		      cmd_add_header_target, 2, 0),
	SHELL_CMD_ARG(http, NULL, "<an HTTP 1.x header>", cmd_add_header_http, 2, 0),
	SHELL_CMD_ARG(body, NULL, "<a chunk of the object body>", cmd_add_header_body, 2, 0),
	SHELL_CMD_ARG(end_body, NULL, "<the final chunk of the object body>",
		      cmd_add_header_end_body, 2, 0),
	SHELL_CMD_ARG(who, NULL,
		      "<identifies the OBEX application, used to tell if talking to a peer>",
		      cmd_add_header_who, 2, 0),
	SHELL_CMD_ARG(conn_id, NULL, "<an identifier used for OBEX connection multiplexing>",
		      cmd_add_header_conn_id, 2, 0),
	SHELL_CMD_ARG(app_param, NULL, "application parameter: <tag> <value> [last]",
		      cmd_add_header_app_param, 3, 1),
	SHELL_CMD_ARG(auth_challenge, NULL, "authentication digest-challenge: <tag> <value> [last]",
		      cmd_add_header_auth_challenge, 3, 1),
	SHELL_CMD_ARG(auth_rsp, NULL, "authentication digest-response: <tag> <value> [last]",
		      cmd_add_header_auth_rsp, 3, 1),
	SHELL_CMD_ARG(creator_id, NULL, "<indicates the creator of an object>",
		      cmd_add_header_creator_id, 2, 0),
	SHELL_CMD_ARG(wan_uuid, NULL, "<uniquely identifies the network client (OBEX server)>",
		      cmd_add_header_wan_uuid, 2, 0),
	SHELL_CMD_ARG(obj_class, NULL, "<OBEX Object class of object>", cmd_add_header_obj_class, 2,
		      0),
	SHELL_CMD_ARG(session_param, NULL, "<parameters used in session commands/responses>",
		      cmd_add_header_session_param, 2, 0),
	SHELL_CMD_ARG(session_seq_number, NULL,
		      "<sequence number used in each OBEX packet for reliability>",
		      cmd_add_header_session_seq_number, 2, 0),
	SHELL_CMD_ARG(action_id, NULL,
		      "<specifies the action to be performed (used in ACTION operation)>",
		      cmd_add_header_action_id, 2, 0),
	SHELL_CMD_ARG(dest_name, NULL,
		      "<the destination object name (used in certain ACTION operations)>",
		      cmd_add_header_dest_name, 2, 0),
	SHELL_CMD_ARG(perm, NULL, "<4-byte bit mask for setting permissions>", cmd_add_header_perm,
		      2, 0),
	SHELL_CMD_ARG(srm, NULL, "<1-byte value to setup Single Response Mode (SRM)>",
		      cmd_add_header_srm, 2, 0),
	SHELL_CMD_ARG(srm_param, NULL, "<Single Response Mode (SRM) Parameter>",
		      cmd_add_header_srm_param, 2, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(obex_client_cmds,
	SHELL_CMD_ARG(conn, NULL, "<mopl>", cmd_goep_client_conn, 2, 0),
	SHELL_CMD_ARG(disconn, NULL, HELP_NONE, cmd_goep_client_disconn, 1, 0),
	SHELL_CMD_ARG(put, NULL, "<final: yes, no>", cmd_goep_client_put, 2, 0),
	SHELL_CMD_ARG(get, NULL, "<final: yes, no>", cmd_goep_client_get, 2, 0),
	SHELL_CMD_ARG(abort, NULL, HELP_NONE, cmd_goep_client_abort, 1, 0),
	SHELL_CMD_ARG(setpath, NULL, "[parent] [create]", cmd_goep_client_setpath, 1, 2),
	SHELL_CMD_ARG(action, NULL, "<final: yes, no>", cmd_goep_client_action, 2, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(obex_server_cmds,
	SHELL_CMD_ARG(conn, NULL, "<rsp: continue, success, error> <mopl> [rsp_code]",
		      cmd_goep_server_conn, 3, 1),
	SHELL_CMD_ARG(disconn, NULL, "<rsp: continue, success, error> [rsp_code]",
		      cmd_goep_server_disconn, 2, 1),
	SHELL_CMD_ARG(put, NULL, "<rsp: continue, success, error> [rsp_code]", cmd_goep_server_put,
		      2, 1),
	SHELL_CMD_ARG(get, NULL, "<rsp: continue, success, error> [rsp_code]", cmd_goep_server_get,
		      2, 1),
	SHELL_CMD_ARG(abort, NULL, "<rsp: continue, success, error> [rsp_code]",
		      cmd_goep_server_abort, 2, 1),
	SHELL_CMD_ARG(setpath, NULL, "<rsp: continue, success, error> [rsp_code]",
		      cmd_goep_server_setpath, 2, 1),
	SHELL_CMD_ARG(action, NULL, "<rsp: continue, success, error> [rsp_code]",
		      cmd_goep_server_action, 2, 1),
	SHELL_SUBCMD_SET_END
);

static int cmd_alloc_buf(const struct shell *sh, size_t argc, char **argv)
{
	if (goep_app.tx_buf) {
		shell_error(sh, "Buf %p is using", goep_app.tx_buf);
		return -EBUSY;
	}

	goep_app.tx_buf = bt_goep_create_pdu(&goep_app.goep, &tx_pool);
	if (!goep_app.tx_buf) {
		shell_error(sh, "Fail to allocate tx buffer");
		return -ENOBUFS;
	}

	return 0;
}

static int cmd_release_buf(const struct shell *sh, size_t argc, char **argv)
{
	if (!goep_app.tx_buf) {
		shell_error(sh, "No buf is using");
		return -EINVAL;
	}

	net_buf_unref(goep_app.tx_buf);
	goep_app.tx_buf = NULL;

	return 0;
}

static int cmd_common(const struct shell *sh, size_t argc, char **argv)
{
	if (argc == 1) {
		shell_help(sh);
		return SHELL_CMD_HELP_PRINTED;
	}

	shell_error(sh, "%s unknown parameter: %s", argv[0], argv[1]);

	return -ENOEXEC;
}

SHELL_STATIC_SUBCMD_SET_CREATE(goep_cmds,
	SHELL_CMD_ARG(register-rfcomm, NULL, "<channel>", cmd_register_rfcomm, 2, 0),
	SHELL_CMD_ARG(connect-rfcomm, NULL, "<channel>", cmd_connect_rfcomm, 2, 0),
	SHELL_CMD_ARG(disconnect-rfcomm, NULL, HELP_NONE, cmd_disconnect_rfcomm, 1, 0),
	SHELL_CMD_ARG(register-l2cap, NULL, "<psm>", cmd_register_l2cap, 2, 0),
	SHELL_CMD_ARG(connect-l2cap, NULL, "<psm>", cmd_connect_l2cap, 2, 0),
	SHELL_CMD_ARG(disconnect-l2cap, NULL, HELP_NONE, cmd_disconnect_l2cap, 1, 0),
	SHELL_CMD_ARG(alloc-buf, NULL, "Alloc tx buffer", cmd_alloc_buf, 1, 0),
	SHELL_CMD_ARG(release-buf, NULL, "Free allocated tx buffer", cmd_release_buf, 1, 0),
	SHELL_CMD_ARG(add-header, &obex_add_header_cmds, "Adding header sets", cmd_common, 1, 0),
	SHELL_CMD_ARG(client, &obex_client_cmds, "Client sets", cmd_common, 1, 0),
	SHELL_CMD_ARG(server, &obex_server_cmds, "Server sets", cmd_common, 1, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_ARG_REGISTER(goep, &goep_cmds, "Bluetooth GOEP shell commands", cmd_common, 1, 1);
