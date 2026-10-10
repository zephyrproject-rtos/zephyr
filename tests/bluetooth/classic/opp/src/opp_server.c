/* opp_server.c - OPP Push Server role shell commands (board-to-board test) */

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
#include <common/bt_shell_private.h>

#include "opp_common.h"

/* Default RFCOMM channel used to advertise the OPP server. */
#define OPP_SERVER_RFCOMM_CHANNEL 5U

/*
 * OPP SDP service record: service class OBEXObjectPush (0x1105), an RFCOMM/OBEX
 * protocol descriptor list carrying the server channel, the OPP profile
 * descriptor, and the mandatory Supported Formats List attribute (0x0303).
 */
static struct bt_sdp_attribute opp_attrs[] = {
	BT_SDP_NEW_SERVICE,
	BT_SDP_LIST(
		BT_SDP_ATTR_SVCLASS_ID_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
			BT_SDP_ARRAY_16(BT_SDP_OBEX_OBJPUSH_SVCLASS)
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
				BT_SDP_ARRAY_8(OPP_SERVER_RFCOMM_CHANNEL)
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
	BT_SDP_LIST(
		BT_SDP_ATTR_PROFILE_DESC_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 8),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
				BT_SDP_ARRAY_16(BT_SDP_OBEX_OBJPUSH_SVCLASS)
			},
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
				BT_SDP_ARRAY_16(0x0101)
			},
			)
		},
		)
	),
	/* Supported Formats List (attribute 0x0303): advertise vCard 2.1. */
	BT_SDP_LIST(
		BT_SDP_ATTR_SUPPORTED_FORMATS_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 2),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UINT8),
			BT_SDP_ARRAY_8(BT_OPP_FORMAT_VCARD_2_1)
		},
		)
	),

	BT_SDP_SERVICE_NAME("OBEX Object Push"),
};

static struct bt_sdp_record opp_rec = BT_SDP_RECORD(opp_attrs);

static struct bt_opp_server opp_server;
static bool opp_server_registered;

/*
 * Runtime-configurable response codes so the pytest cases can drive the server
 * down the SUCCESS / CONTINUE / error paths without rebuilding the firmware.
 * Defaults reflect the normal happy-path behavior.
 */
static uint8_t rsp_connect = BT_OPP_RSP_CODE_SUCCESS;
static uint8_t rsp_push = BT_OPP_RSP_CODE_SUCCESS;
static uint8_t rsp_pull = BT_OPP_RSP_CODE_SUCCESS;
static uint8_t rsp_disconnect = BT_OPP_RSP_CODE_SUCCESS;

/*
 * Multi-packet business-card pull support. When 'pull_multi' is enabled the
 * server answers the first GET with a CONTINUE carrying a non-final Body
 * chunk, then answers the continuation GET with a SUCCESS carrying the final
 * End-of-Body chunk, so the GET CONTINUE_NON_FINAL path runs on the wire.
 */
static bool pull_multi;
static uint8_t pull_stage;

/* OPP server callbacks. */

static void opp_srv_rfcomm_connected(struct bt_conn *conn, struct bt_opp_server *server)
{
	bt_shell_print("OPP server RFCOMM connected server %p", server);
}

static void opp_srv_rfcomm_disconnected(struct bt_opp_server *server)
{
	bt_shell_print("OPP server RFCOMM disconnected server %p", server);
}

static void opp_srv_connect(struct bt_opp_server *server, uint8_t version, uint16_t mopl,
			    struct net_buf *buf)
{
	int err;

	bt_shell_print("OPP server OBEX connect req version 0x%02x mopl %u", version, mopl);

	/* Per OPP Spec 5.4 the client must not send a Target header. */
	if (buf != NULL) {
		uint16_t tgt_len;
		const uint8_t *tgt;

		if (bt_obex_get_header_target(buf, &tgt_len, &tgt) == 0) {
			bt_shell_print("OPP server OBEX connect target_header present");
		} else {
			bt_shell_print("OPP server OBEX connect no_target_header");
		}
	} else {
		bt_shell_print("OPP server OBEX connect no_target_header");
	}

	err = bt_opp_server_connect_rsp(server, OPP_TEST_MOPL,
					(enum bt_opp_rsp_code)rsp_connect, NULL);
	if (err) {
		bt_shell_error("OPP server connect rsp failed (err %d)", err);
	} else if (rsp_connect == BT_OPP_RSP_CODE_SUCCESS) {
		bt_shell_print("OPP server OBEX connected");
	} else {
		bt_shell_print("OPP server OBEX connect rejected 0x%02x", rsp_connect);
	}
}

static void opp_srv_disconnect(struct bt_opp_server *server, struct net_buf *buf)
{
	int err;

	bt_shell_print("OPP server OBEX disconnect req");

	err = bt_opp_server_disconnect_rsp(server, (enum bt_opp_rsp_code)rsp_disconnect, NULL);
	if (err) {
		bt_shell_error("OPP server disconnect rsp failed (err %d)", err);
	} else if (rsp_disconnect == BT_OPP_RSP_CODE_SUCCESS) {
		bt_shell_print("OPP server OBEX disconnected");
	} else {
		bt_shell_print("OPP server OBEX disconnect error 0x%02x", rsp_disconnect);
	}
}

static void opp_srv_push(struct bt_opp_server *server, bool final, struct net_buf *buf)
{
	enum bt_opp_rsp_code code;
	int err;

	bt_shell_print("OPP server push req final %d len %u", (int)final,
		       buf != NULL ? buf->len : 0U);

	if (rsp_push != BT_OPP_RSP_CODE_SUCCESS) {
		/* Forced error path for the negative test cases. */
		code = (enum bt_opp_rsp_code)rsp_push;
	} else if (final) {
		code = BT_OPP_RSP_CODE_SUCCESS;
	} else {
		code = BT_OPP_RSP_CODE_CONTINUE;
	}

	err = bt_opp_server_push_rsp(server, code, NULL);
	if (err) {
		bt_shell_error("OPP server push rsp failed (err %d)", err);
		return;
	}

	if (code == BT_OPP_RSP_CODE_SUCCESS) {
		bt_shell_print("OPP server push complete");
	} else if (code == BT_OPP_RSP_CODE_CONTINUE) {
		bt_shell_print("OPP server push continue");
	} else {
		bt_shell_print("OPP server push error 0x%02x", code);
	}
}

static void opp_srv_pull_bcard(struct bt_opp_server *server, struct net_buf *buf)
{
	static const uint8_t vcard[] = OPP_TEST_VCARD;
	struct net_buf *rsp;
	int err;

	bt_shell_print("OPP server pull_bcard req");

	if (rsp_pull == BT_OPP_RSP_CODE_NOT_FOUND) {
		err = bt_opp_server_pull_bcard_rsp(server, BT_OPP_RSP_CODE_NOT_FOUND, NULL);
		if (err) {
			bt_shell_error("OPP server pull_bcard rsp failed (err %d)", err);
		} else {
			bt_shell_print("OPP server pull_bcard not_found");
		}
		return;
	}

	rsp = bt_opp_server_create_pdu(server, NULL);
	if (!rsp) {
		bt_shell_error("OPP server unable to allocate pull_bcard PDU");
		return;
	}

	if (pull_multi && pull_stage == 0U) {
		/*
		 * First half of a multi-packet GET: send a non-final Body chunk
		 * with CONTINUE so the client must issue a continuation GET.
		 */
		err = bt_obex_add_header_body(rsp, sizeof(vcard) - 1, vcard);
		if (err) {
			bt_shell_error("OPP server add Body failed (err %d)", err);
			net_buf_unref(rsp);
			return;
		}

		err = bt_opp_server_pull_bcard_rsp(server, BT_OPP_RSP_CODE_CONTINUE, rsp);
		if (err) {
			bt_shell_error("OPP server pull_bcard rsp failed (err %d)", err);
			net_buf_unref(rsp);
		} else {
			pull_stage = 1U;
			bt_shell_print("OPP server pull_bcard continue");
		}
		return;
	}

	err = bt_obex_add_header_end_body(rsp, sizeof(vcard) - 1, vcard);
	if (err) {
		bt_shell_error("OPP server add End-of-Body failed (err %d)", err);
		net_buf_unref(rsp);
		return;
	}

	err = bt_opp_server_pull_bcard_rsp(server, BT_OPP_RSP_CODE_SUCCESS, rsp);
	if (err) {
		bt_shell_error("OPP server pull_bcard rsp failed (err %d)", err);
		net_buf_unref(rsp);
	} else {
		pull_stage = 0U;
		bt_shell_print("OPP server pull_bcard success");
	}
}

static void opp_srv_abort(struct bt_opp_server *server, struct net_buf *buf)
{
	int err;

	bt_shell_print("OPP server abort req");

	err = bt_opp_server_abort_rsp(server, BT_OPP_RSP_CODE_SUCCESS, NULL);
	if (err) {
		bt_shell_error("OPP server abort rsp failed (err %d)", err);
	} else {
		bt_shell_print("OPP server abort handled");
	}
}

static const struct bt_opp_server_cb opp_server_cb = {
	.rfcomm_connected = opp_srv_rfcomm_connected,
	.rfcomm_disconnected = opp_srv_rfcomm_disconnected,
	.connect = opp_srv_connect,
	.disconnect = opp_srv_disconnect,
	.push = opp_srv_push,
	.pull_bcard = opp_srv_pull_bcard,
	.abort = opp_srv_abort,
};

static int opp_rfcomm_accept(struct bt_conn *conn, struct bt_opp_server_rfcomm *rfcomm_server,
			     struct bt_opp_server **out)
{
	int err;

	bt_shell_print("OPP server accept conn %p", conn);

	err = bt_opp_server_register(&opp_server, &opp_server_cb);
	if (err) {
		bt_shell_error("OPP server register failed (err %d)", err);
		return err;
	}

	*out = &opp_server;

	return 0;
}

static struct bt_opp_server_rfcomm opp_rfcomm = {
	.accept = opp_rfcomm_accept,
};

static int cmd_register(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	if (opp_server_registered) {
		shell_print(sh, "OPP server already registered");
		return 0;
	}

	opp_rfcomm.server.rfcomm.channel = OPP_SERVER_RFCOMM_CHANNEL;

	err = bt_opp_server_rfcomm_register(&opp_rfcomm);
	if (err) {
		shell_error(sh, "Unable to register OPP server (err %d)", err);
		return -ENOEXEC;
	}

	err = bt_sdp_register_service(&opp_rec);
	if (err) {
		shell_error(sh, "Unable to register OPP SDP record (err %d)", err);
		return -ENOEXEC;
	}

	opp_server_registered = true;
	shell_print(sh, "OPP server registered on RFCOMM channel %u",
		    opp_rfcomm.server.rfcomm.channel);

	return 0;
}

static int cmd_set_connect_rsp(const struct shell *sh, size_t argc, char *argv[])
{
	rsp_connect = (uint8_t)strtoul(argv[1], NULL, 16);
	shell_print(sh, "OPP server connect rsp set 0x%02x", rsp_connect);
	return 0;
}

static int cmd_set_push_rsp(const struct shell *sh, size_t argc, char *argv[])
{
	rsp_push = (uint8_t)strtoul(argv[1], NULL, 16);
	shell_print(sh, "OPP server push rsp set 0x%02x", rsp_push);
	return 0;
}

static int cmd_set_pull_rsp(const struct shell *sh, size_t argc, char *argv[])
{
	rsp_pull = (uint8_t)strtoul(argv[1], NULL, 16);
	shell_print(sh, "OPP server pull rsp set 0x%02x", rsp_pull);
	return 0;
}

static int cmd_set_disconnect_rsp(const struct shell *sh, size_t argc, char *argv[])
{
	rsp_disconnect = (uint8_t)strtoul(argv[1], NULL, 16);
	shell_print(sh, "OPP server disconnect rsp set 0x%02x", rsp_disconnect);
	return 0;
}

static int cmd_set_pull_multi(const struct shell *sh, size_t argc, char *argv[])
{
	pull_multi = (strtoul(argv[1], NULL, 10) != 0U);
	pull_stage = 0U;
	shell_print(sh, "OPP server pull multi %s", pull_multi ? "on" : "off");
	return 0;
}

static int cmd_reset_rsp(const struct shell *sh, size_t argc, char *argv[])
{
	rsp_connect = BT_OPP_RSP_CODE_SUCCESS;
	rsp_push = BT_OPP_RSP_CODE_SUCCESS;
	rsp_pull = BT_OPP_RSP_CODE_SUCCESS;
	rsp_disconnect = BT_OPP_RSP_CODE_SUCCESS;
	pull_multi = false;
	pull_stage = 0U;
	shell_print(sh, "OPP server rsp codes reset");
	return 0;
}

static int cmd_create_pdu(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_buf *buf;

	if (!opp_server_registered) {
		shell_error(sh, "OPP server not registered");
		return -ENOEXEC;
	}

	buf = bt_opp_server_create_pdu(&opp_server, NULL);
	if (!buf) {
		shell_error(sh, "OPP server create_pdu failed");
		return -ENOEXEC;
	}

	shell_print(sh, "OPP server create_pdu ok headroom %u", net_buf_headroom(buf));
	net_buf_unref(buf);

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	opp_s_cmds,
	SHELL_CMD_ARG(register, NULL, "register OPP server + SDP record", cmd_register, 1, 0),
	SHELL_CMD_ARG(set_connect_rsp, NULL, "<hex> set OBEX connect response code",
		      cmd_set_connect_rsp, 2, 0),
	SHELL_CMD_ARG(set_push_rsp, NULL, "<hex> set PUT response code", cmd_set_push_rsp, 2, 0),
	SHELL_CMD_ARG(set_pull_rsp, NULL, "<hex> set GET (pull_bcard) response code",
		      cmd_set_pull_rsp, 2, 0),
	SHELL_CMD_ARG(set_disconnect_rsp, NULL, "<hex> set OBEX disconnect response code",
		      cmd_set_disconnect_rsp, 2, 0),
	SHELL_CMD_ARG(set_pull_multi, NULL, "<0|1> enable multi-packet business-card pull",
		      cmd_set_pull_multi, 2, 0),
	SHELL_CMD_ARG(reset_rsp, NULL, "reset all response codes to SUCCESS", cmd_reset_rsp, 1, 0),
	SHELL_CMD_ARG(create_pdu, NULL, "allocate a server PDU (headroom test)", cmd_create_pdu,
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

SHELL_CMD_ARG_REGISTER(opp_s, &opp_s_cmds, "Bluetooth classic OPP server shell commands",
		       cmd_default_handler, 1, 1);
