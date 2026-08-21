/* opp_client.c - OPP Push Client role shell commands (board-to-board test) */

/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdlib.h>

#include <errno.h>
#include <zephyr/types.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/shell/shell.h>

#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/bluetooth/classic/rfcomm.h>
#include <zephyr/bluetooth/classic/sdp.h>
#include <zephyr/bluetooth/classic/goep.h>
#include <zephyr/bluetooth/classic/obex.h>
#include <zephyr/bluetooth/classic/opp.h>
#include <host/shell/bt.h>
#include <common/bt_shell_private.h>

#include "opp_common.h"

#define SDP_CLIENT_USER_BUF_LEN 512U

NET_BUF_POOL_FIXED_DEFINE(opp_sdp_client_pool, 1, SDP_CLIENT_USER_BUF_LEN,
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

static struct bt_opp_client opp_client;
static uint8_t opp_server_channel;

static struct bt_sdp_discover_params sdp_discover;
static struct bt_uuid_16 sdp_discover_uuid;

/* OPP client callbacks. */
static void opp_cli_rfcomm_connected(struct bt_conn *conn, struct bt_opp_client *client)
{
	bt_shell_print("OPP client RFCOMM connected client %p", client);
}

static void opp_cli_rfcomm_disconnected(struct bt_opp_client *client)
{
	bt_shell_print("OPP client RFCOMM disconnected client %p", client);
}

static void opp_cli_connect(struct bt_opp_client *client, enum bt_opp_rsp_code rsp_code,
			    uint8_t version, uint16_t mopl, struct net_buf *buf)
{
	bt_shell_print("OPP client OBEX connect rsp 0x%02x version 0x%02x mopl %u", rsp_code,
		       version, mopl);
	if (rsp_code == BT_OPP_RSP_CODE_SUCCESS) {
		bt_shell_print("OPP client OBEX connected");
	} else {
		bt_shell_print("OPP client OBEX connect error 0x%02x", rsp_code);
	}
}

static void opp_cli_disconnect(struct bt_opp_client *client, enum bt_opp_rsp_code rsp_code,
			       struct net_buf *buf)
{
	bt_shell_print("OPP client OBEX disconnect rsp 0x%02x", rsp_code);
	if (rsp_code == BT_OPP_RSP_CODE_SUCCESS) {
		bt_shell_print("OPP client OBEX disconnected");
	} else {
		bt_shell_print("OPP client OBEX disconnect error 0x%02x", rsp_code);
	}
}

static void opp_cli_push(struct bt_opp_client *client, enum bt_opp_rsp_code rsp_code,
			 struct net_buf *buf)
{
	bt_shell_print("OPP client push rsp 0x%02x", rsp_code);
	if (rsp_code == BT_OPP_RSP_CODE_SUCCESS) {
		bt_shell_print("OPP client push success");
	} else if (rsp_code == BT_OPP_RSP_CODE_CONTINUE) {
		bt_shell_print("OPP client push continue");
	} else if (rsp_code == BT_OPP_RSP_CODE_UNSUPP_MEDIA_TYPE) {
		bt_shell_print("OPP client push unsupported_media");
	} else if (rsp_code == BT_OPP_RSP_CODE_ENTITY_TOO_LARGE) {
		bt_shell_print("OPP client push entity_too_large");
	} else {
		bt_shell_print("OPP client push error 0x%02x", rsp_code);
	}
}

static void opp_cli_pull_bcard(struct bt_opp_client *client, enum bt_opp_rsp_code rsp_code,
			       struct net_buf *buf)
{
	bt_shell_print("OPP client pull_bcard rsp 0x%02x", rsp_code);
	if (rsp_code == BT_OPP_RSP_CODE_SUCCESS) {
		bt_shell_print("OPP client pull_bcard success");
	} else if (rsp_code == BT_OPP_RSP_CODE_CONTINUE) {
		bt_shell_print("OPP client pull_bcard continue");
	} else if (rsp_code == BT_OPP_RSP_CODE_NOT_FOUND) {
		bt_shell_print("OPP client pull_bcard not_found");
	} else if (rsp_code == BT_OPP_RSP_CODE_FORBIDDEN) {
		bt_shell_print("OPP client pull_bcard forbidden");
	} else {
		bt_shell_print("OPP client pull_bcard error 0x%02x", rsp_code);
	}
}

static void opp_cli_abort(struct bt_opp_client *client, enum bt_opp_rsp_code rsp_code,
			  struct net_buf *buf)
{
	bt_shell_print("OPP client abort rsp 0x%02x", rsp_code);
	if (rsp_code == BT_OPP_RSP_CODE_SUCCESS) {
		bt_shell_print("OPP client abort success");
	} else {
		bt_shell_print("OPP client abort error 0x%02x", rsp_code);
	}
}

static const struct bt_opp_client_cb opp_client_cb = {
	.rfcomm_connected = opp_cli_rfcomm_connected,
	.rfcomm_disconnected = opp_cli_rfcomm_disconnected,
	.connect = opp_cli_connect,
	.disconnect = opp_cli_disconnect,
	.push = opp_cli_push,
	.pull_bcard = opp_cli_pull_bcard,
	.abort = opp_cli_abort,
};

static uint8_t sdp_discover_func(struct bt_conn *conn, struct bt_sdp_client_result *result,
				 const struct bt_sdp_discover_params *params)
{
	int err;
	uint16_t channel;

	if ((result == NULL) || (result->resp_buf == NULL) || (result->resp_buf->len == 0)) {
		bt_shell_print("OPP SDP discovery done");
		return BT_SDP_DISCOVER_UUID_STOP;
	}

	err = bt_sdp_get_proto_param(result->resp_buf, BT_SDP_PROTO_RFCOMM, &channel);
	if (!err) {
		opp_server_channel = (uint8_t)channel;
		bt_shell_print("OPP server discovered on RFCOMM channel %u", opp_server_channel);
		return BT_SDP_DISCOVER_UUID_STOP;
	}

	return BT_SDP_DISCOVER_UUID_CONTINUE;
}

static int cmd_discover(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	if (!default_conn) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	sdp_discover_uuid.uuid.type = BT_UUID_TYPE_16;
	sdp_discover_uuid.val = BT_SDP_OBEX_OBJPUSH_SVCLASS;
	sdp_discover.uuid = &sdp_discover_uuid.uuid;
	sdp_discover.func = sdp_discover_func;
	sdp_discover.pool = &opp_sdp_client_pool;
	sdp_discover.type = BT_SDP_DISCOVER_SERVICE_SEARCH_ATTR;

	opp_server_channel = 0U;

	err = bt_sdp_discover(default_conn, &sdp_discover);
	if (err) {
		shell_error(sh, "Unable to start SDP discovery (err %d)", err);
		return -ENOEXEC;
	}

	shell_print(sh, "OPP SDP discovery started");

	return 0;
}

static int cmd_connect_rfcomm(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t channel;
	int err;

	if (!default_conn) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	if (argc > 1) {
		channel = strtoul(argv[1], NULL, 16);
	} else {
		channel = opp_server_channel;
	}

	if (channel == 0U) {
		shell_error(sh, "No RFCOMM channel (run 'opp_c discover' first)");
		return -ENOEXEC;
	}

	err = bt_opp_client_connect_rfcomm(default_conn, &opp_client, &opp_client_cb, channel);
	if (err) {
		shell_error(sh, "Unable to connect OPP RFCOMM (err %d)", err);
		return -ENOEXEC;
	}

	shell_print(sh, "OPP client RFCOMM connection pending");

	return 0;
}

static int cmd_obex_connect(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	err = bt_opp_client_connect(&opp_client, OPP_TEST_MOPL, NULL);
	if (err) {
		shell_error(sh, "Unable to send OBEX connect (err %d)", err);
		return -ENOEXEC;
	}

	shell_print(sh, "OPP client OBEX connect pending");

	return 0;
}

/*
 * Push a single-packet object. Optional args:
 *   argv[1]: Type header string (default BT_OPP_TYPE_VCARD). Use e.g.
 *            "application/x-unknown" to exercise the UNSUPP_MEDIA_TYPE path.
 */
static int cmd_push(const struct shell *sh, size_t argc, char *argv[])
{
	static const uint8_t name[] = OPP_TEST_OBJECT_NAME_UTF16;
	static const uint8_t vcard[] = OPP_TEST_VCARD;
	const char *type = (argc > 1) ? argv[1] : BT_OPP_TYPE_VCARD;
	struct net_buf *buf;
	int err;

	buf = bt_opp_client_create_pdu(&opp_client, NULL);
	if (!buf) {
		shell_error(sh, "Unable to allocate OPP PDU");
		return -ENOEXEC;
	}

	err = bt_obex_add_header_name(buf, sizeof(name), name);
	if (err) {
		shell_error(sh, "Unable to add Name header (err %d)", err);
		net_buf_unref(buf);
		return -ENOEXEC;
	}

	err = bt_obex_add_header_type(buf, strlen(type) + 1, (const uint8_t *)type);
	if (err) {
		shell_error(sh, "Unable to add Type header (err %d)", err);
		net_buf_unref(buf);
		return -ENOEXEC;
	}

	err = bt_obex_add_header_len(buf, sizeof(vcard) - 1);
	if (err) {
		shell_error(sh, "Unable to add Length header (err %d)", err);
		net_buf_unref(buf);
		return -ENOEXEC;
	}

	err = bt_obex_add_header_end_body(buf, sizeof(vcard) - 1, vcard);
	if (err) {
		shell_error(sh, "Unable to add End-of-Body header (err %d)", err);
		net_buf_unref(buf);
		return -ENOEXEC;
	}

	err = bt_opp_client_push(&opp_client, true, buf);
	if (err) {
		shell_error(sh, "Unable to push object (err %d)", err);
		net_buf_unref(buf);
		return -ENOEXEC;
	}

	shell_print(sh, "OPP client push pending");

	return 0;
}

/*
 * Start a multi-packet push: first PUT carries Name/Type/Length and a Body
 * header (non-final). Subsequent chunks are sent with 'push_next'.
 */
static int cmd_push_start(const struct shell *sh, size_t argc, char *argv[])
{
	static const uint8_t name[] = OPP_TEST_OBJECT_NAME_UTF16;
	static const uint8_t vcard[] = OPP_TEST_VCARD;
	struct net_buf *buf;
	int err;

	buf = bt_opp_client_create_pdu(&opp_client, NULL);
	if (!buf) {
		shell_error(sh, "Unable to allocate OPP PDU");
		return -ENOEXEC;
	}

	err = bt_obex_add_header_name(buf, sizeof(name), name);
	if (!err) {
		err = bt_obex_add_header_type(buf, sizeof(BT_OPP_TYPE_VCARD),
					      (const uint8_t *)BT_OPP_TYPE_VCARD);
	}
	if (!err) {
		err = bt_obex_add_header_len(buf, (sizeof(vcard) - 1) * 2);
	}
	if (!err) {
		err = bt_obex_add_header_body(buf, sizeof(vcard) - 1, vcard);
	}
	if (err) {
		shell_error(sh, "Unable to build first PUT packet (err %d)", err);
		net_buf_unref(buf);
		return -ENOEXEC;
	}

	err = bt_opp_client_push(&opp_client, false, buf);
	if (err) {
		shell_error(sh, "Unable to push (err %d)", err);
		net_buf_unref(buf);
		return -ENOEXEC;
	}

	shell_print(sh, "OPP client push_start pending");

	return 0;
}

/* Send the final PUT chunk (End-of-Body, final=true) of a multi-packet push. */
static int cmd_push_final(const struct shell *sh, size_t argc, char *argv[])
{
	static const uint8_t vcard[] = OPP_TEST_VCARD;
	struct net_buf *buf;
	int err;

	buf = bt_opp_client_create_pdu(&opp_client, NULL);
	if (!buf) {
		shell_error(sh, "Unable to allocate OPP PDU");
		return -ENOEXEC;
	}

	err = bt_obex_add_header_end_body(buf, sizeof(vcard) - 1, vcard);
	if (err) {
		shell_error(sh, "Unable to add End-of-Body header (err %d)", err);
		net_buf_unref(buf);
		return -ENOEXEC;
	}

	err = bt_opp_client_push(&opp_client, true, buf);
	if (err) {
		shell_error(sh, "Unable to push final (err %d)", err);
		net_buf_unref(buf);
		return -ENOEXEC;
	}

	shell_print(sh, "OPP client push_final pending");

	return 0;
}

/*
 * Pull the server's default business card (OBEX GET). Optional argv[1] adds a
 * non-empty Name header to exercise the FORBIDDEN path.
 */
static int cmd_pull_bcard(const struct shell *sh, size_t argc, char *argv[])
{
	static const uint8_t empty_name[] = {0x00, 0x00};
	static const uint8_t named[] = OPP_TEST_OBJECT_NAME_UTF16;
	struct net_buf *buf;
	int err;

	buf = bt_opp_client_create_pdu(&opp_client, NULL);
	if (!buf) {
		shell_error(sh, "Unable to allocate OPP PDU");
		return -ENOEXEC;
	}

	err = bt_obex_add_header_type(buf, sizeof(BT_OPP_TYPE_VCARD),
				      (const uint8_t *)BT_OPP_TYPE_VCARD);
	if (err) {
		shell_error(sh, "Unable to add Type header (err %d)", err);
		net_buf_unref(buf);
		return -ENOEXEC;
	}

	if (argc > 1) {
		/* Non-empty Name header -> server replies FORBIDDEN. */
		err = bt_obex_add_header_name(buf, sizeof(named), named);
	} else {
		err = bt_obex_add_header_name(buf, sizeof(empty_name), empty_name);
	}
	if (err) {
		shell_error(sh, "Unable to add Name header (err %d)", err);
		net_buf_unref(buf);
		return -ENOEXEC;
	}

	err = bt_opp_client_pull_bcard(&opp_client, buf);
	if (err) {
		shell_error(sh, "Unable to pull business card (err %d)", err);
		net_buf_unref(buf);
		return -ENOEXEC;
	}

	shell_print(sh, "OPP client pull_bcard pending");

	return 0;
}

/*
 * Issue a continuation GET for a multi-packet business-card pull. The client
 * stack does not auto-continue a GET: after the pull_bcard callback reports
 * BT_OPP_RSP_CODE_CONTINUE the application must send another GET to fetch the
 * next chunk. The continuation carries only the empty Name and Type headers.
 */
static int cmd_pull_continue(const struct shell *sh, size_t argc, char *argv[])
{
	static const uint8_t empty_name[] = {0x00, 0x00};
	struct net_buf *buf;
	int err;

	buf = bt_opp_client_create_pdu(&opp_client, NULL);
	if (!buf) {
		shell_error(sh, "Unable to allocate OPP PDU");
		return -ENOEXEC;
	}

	err = bt_obex_add_header_type(buf, sizeof(BT_OPP_TYPE_VCARD),
				      (const uint8_t *)BT_OPP_TYPE_VCARD);
	if (!err) {
		err = bt_obex_add_header_name(buf, sizeof(empty_name), empty_name);
	}
	if (err) {
		shell_error(sh, "Unable to build continuation GET (err %d)", err);
		net_buf_unref(buf);
		return -ENOEXEC;
	}

	err = bt_opp_client_pull_bcard(&opp_client, buf);
	if (err) {
		shell_error(sh, "Unable to send continuation GET (err %d)", err);
		net_buf_unref(buf);
		return -ENOEXEC;
	}

	shell_print(sh, "OPP client pull_continue pending");

	return 0;
}

static int cmd_abort(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	err = bt_opp_client_abort(&opp_client, NULL);
	if (err) {
		shell_error(sh, "Unable to send abort (err %d)", err);
		return -ENOEXEC;
	}

	shell_print(sh, "OPP client abort pending");

	return 0;
}

static int cmd_obex_disconnect(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	err = bt_opp_client_disconnect(&opp_client, NULL);
	if (err) {
		shell_error(sh, "Unable to send OBEX disconnect (err %d)", err);
		return -ENOEXEC;
	}

	shell_print(sh, "OPP client OBEX disconnect pending");

	return 0;
}

static int cmd_disconnect_rfcomm(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	err = bt_opp_client_disconnect_rfcomm(&opp_client);
	if (err) {
		shell_error(sh, "Unable to disconnect OPP RFCOMM (err %d)", err);
		return -ENOEXEC;
	}

	shell_print(sh, "OPP client RFCOMM disconnect pending");

	return 0;
}

static int cmd_create_pdu(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_buf *buf;

	buf = bt_opp_client_create_pdu(&opp_client, NULL);
	if (!buf) {
		shell_error(sh, "OPP client create_pdu failed");
		return -ENOEXEC;
	}

	shell_print(sh, "OPP client create_pdu ok headroom %u", net_buf_headroom(buf));
	net_buf_unref(buf);

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	opp_c_cmds,
	SHELL_CMD_ARG(discover, NULL, "discover OPP server RFCOMM channel via SDP", cmd_discover,
		      1, 0),
	SHELL_CMD_ARG(connect_rfcomm, NULL, "[channel] connect RFCOMM transport",
		      cmd_connect_rfcomm, 1, 1),
	SHELL_CMD_ARG(obex_connect, NULL, "establish OBEX session", cmd_obex_connect, 1, 0),
	SHELL_CMD_ARG(push, NULL, "[type] push a single-packet vCard object", cmd_push, 1, 1),
	SHELL_CMD_ARG(push_start, NULL, "start multi-packet push (non-final Body)",
		      cmd_push_start, 1, 0),
	SHELL_CMD_ARG(push_final, NULL, "send final PUT chunk (End-of-Body)", cmd_push_final,
		      1, 0),
	SHELL_CMD_ARG(pull_bcard, NULL, "[name] pull server business card (GET)", cmd_pull_bcard,
		      1, 1),
	SHELL_CMD_ARG(pull_continue, NULL, "issue a continuation GET for a multi-packet pull",
		      cmd_pull_continue, 1, 0),
	SHELL_CMD_ARG(abort, NULL, "abort the ongoing operation", cmd_abort, 1, 0),
	SHELL_CMD_ARG(obex_disconnect, NULL, "terminate OBEX session", cmd_obex_disconnect, 1, 0),
	SHELL_CMD_ARG(disconnect_rfcomm, NULL, "disconnect RFCOMM transport",
		      cmd_disconnect_rfcomm, 1, 0),
	SHELL_CMD_ARG(create_pdu, NULL, "allocate a client PDU (headroom test)", cmd_create_pdu,
		      1, 0),
	SHELL_SUBCMD_SET_END);

static int cmd_default_handler(const struct shell *sh, size_t argc, char **argv)
{
	if (argc == 1) {
		shell_help(sh);
		return SHELL_CMD_HELP_PRINTED;
	}

	shell_error(sh, "%s unknown parameter: %s", argv[0], argv[1]);

	return -EINVAL;
}

SHELL_CMD_ARG_REGISTER(opp_c, &opp_c_cmds, "Bluetooth classic OPP client shell commands",
		       cmd_default_handler, 1, 1);
