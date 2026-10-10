/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>

#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/bluetooth/classic/bip.h>
#include <zephyr/bluetooth/classic/obex.h>
#include <zephyr/bluetooth/classic/sdp.h>

#include "host/shell/bt.h"
#include "common/bt_shell_private.h"

struct bt_bip_app {
	struct bt_bip_client client;
	struct bt_bip bip;
	struct bt_bip_server server;
	struct bt_conn *conn;
	struct net_buf *tx_buf;
	uint16_t client_mopl;
	uint16_t server_mopl;
	uint32_t conn_id;
};

extern struct bt_bip_app bip_app;
extern struct bt_bip_server_cb bip_server_cb;
extern struct bt_bip_client_cb bip_client_cb;

static struct bt_bip_server secondary_server;
static struct bt_bip_client secondary_client;

/*
 * Per BIP spec (5.5.1/5.5.3, Table 6.2/6.3), a secondary connection is a
 * separate OBEX session over its own transport connection, discovered via
 * its own SDP service record (Referenced Objects / Archived Objects), not a
 * second OBEX session reusing the primary connection's transport. secondary_bip
 * is that independent transport instance.
 */
#define SEC_SDP_DISCOVER_BUF_LEN 512

static struct bt_bip secondary_bip;
static struct bt_bip_l2cap_server secondary_l2cap_server;
static bool secondary_transport_registered;
static bool secondary_transport_busy;

static uint16_t secondary_svclass_val;
static uint8_t secondary_rfcomm_channel_placeholder = 0x08;
static uint16_t secondary_l2cap_psm = 0x100b;

NET_BUF_POOL_FIXED_DEFINE(secondary_sdp_client_pool, CONFIG_BT_MAX_CONN, SEC_SDP_DISCOVER_BUF_LEN,
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

static struct bt_sdp_attribute secondary_sdp_attrs[] = {
	BT_SDP_NEW_SERVICE,
	BT_SDP_LIST(
		BT_SDP_ATTR_SVCLASS_ID_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
			&secondary_svclass_val,
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
				&secondary_rfcomm_channel_placeholder
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
				BT_SDP_ARRAY_16(BT_SDP_IMAGING_SVCLASS)
			},
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
				BT_SDP_ARRAY_16(0x0102)
			},
			)
		},
		)
	),
	{
		BT_SDP_ATTR_SUPPORTED_FUNCTIONS,
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UINT32),
			&secondary_bip._supp_funcs,
		},
	},
	{
		BT_SDP_ATTR_GOEP_L2CAP_PSM,
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
			&secondary_l2cap_psm,
		},
	},
};

static struct bt_sdp_record secondary_sdp_rec = BT_SDP_RECORD(secondary_sdp_attrs);

static void secondary_bip_connected(struct bt_conn *conn, struct bt_bip *bip)
{
	ARG_UNUSED(bip);

	bt_shell_print("BIP secondary %p transport connected on %p", bip, conn);
}

static void secondary_bip_disconnected(struct bt_bip *bip)
{
	ARG_UNUSED(bip);

	secondary_transport_busy = false;
}

static const struct bt_bip_transport_ops secondary_bip_transport_ops = {
	.connected = secondary_bip_connected,
	.disconnected = secondary_bip_disconnected,
};

static int secondary_l2cap_accept(struct bt_conn *conn, struct bt_bip_l2cap_server *server,
				  struct bt_bip **bip)
{
	ARG_UNUSED(conn);
	ARG_UNUSED(server);

	if (secondary_transport_busy) {
		return -EBUSY;
	}

	secondary_transport_busy = true;
	secondary_bip.ops = &secondary_bip_transport_ops;
	*bip = &secondary_bip;
	return 0;
}

static uint16_t secondary_svclass_for_type(uint8_t type)
{
	return (type == BT_BIP_2ND_CONN_TYPE_ARCHIVED_OBJECTS) ? BT_SDP_IMAGING_ARCHIVE_SVCLASS
							       : BT_SDP_IMAGING_REFOBJS_SVCLASS;
}

static int secondary_transport_register(uint8_t type)
{
	int err;

	if (secondary_transport_registered) {
		return 0;
	}

	secondary_svclass_val = secondary_svclass_for_type(type);

	secondary_l2cap_server.server.l2cap.psm = secondary_l2cap_psm;
	secondary_l2cap_server.accept = secondary_l2cap_accept;
	err = bt_bip_l2cap_register(&secondary_l2cap_server);
	if (err != 0) {
		return err;
	}

	err = bt_sdp_register_service(&secondary_sdp_rec);
	if (err != 0) {
		return err;
	}

	secondary_transport_registered = true;
	return 0;
}

static int secondary_sdp_get_l2cap_psm(const struct net_buf *buf, uint16_t *psm)
{
	int err;
	struct bt_sdp_attribute attr;
	struct bt_sdp_attr_value value;

	err = bt_sdp_get_attr(buf, BT_SDP_ATTR_GOEP_L2CAP_PSM, &attr);
	if (err != 0) {
		return err;
	}

	err = bt_sdp_attr_read(&attr, NULL, &value);
	if (err != 0) {
		return err;
	}

	if ((value.type != BT_SDP_ATTR_VALUE_TYPE_UINT) || (value.uint.size != sizeof(*psm))) {
		return -EINVAL;
	}

	*psm = value.uint.u16;
	return 0;
}

static uint8_t secondary_sdp_discover_func(struct bt_conn *conn,
					   struct bt_sdp_client_result *result,
					   const struct bt_sdp_discover_params *params)
{
	int err;
	uint16_t psm = 0;

	ARG_UNUSED(conn);
	ARG_UNUSED(params);

	if (result == NULL || result->resp_buf == NULL) {
		bt_shell_info("No secondary record found");
		return BT_SDP_DISCOVER_UUID_CONTINUE;
	}

	err = secondary_sdp_get_l2cap_psm(result->resp_buf, &psm);
	if (err != 0) {
		bt_shell_error("Failed to get secondary GOEP L2CAP PSM: %d", err);
	} else {
		bt_shell_info("Found secondary GOEP L2CAP PSM %u", psm);
	}

	return BT_SDP_DISCOVER_UUID_CONTINUE;
}

/* UTF-16BE "1000001" + NUL terminator */
#define IMAGE_PARTIAL_FILE_NAME "\x00\x31\x00\x30\x00\x30\x00\x30\x00\x30\x00\x30\x00\x31\x00\x00"

static int cmd_sec_set_feats_funcs(const struct shell *sh, size_t argc, char *argv[])
{
	int err = 0;
	uint16_t features;
	uint32_t functions;

	features = shell_strtoul(argv[1], 0, &err);
	if (err != 0) {
		shell_error(sh, "Invalid features %s", argv[1]);
		return -ENOEXEC;
	}

	functions = shell_strtoul(argv[2], 0, &err);
	if (err != 0) {
		shell_error(sh, "Invalid functions %s", argv[2]);
		return -ENOEXEC;
	}

	secondary_bip._supp_feats = features;
	secondary_bip._supp_funcs = functions;

	return 0;
}

static int cmd_sec_reg(const struct shell *sh, size_t argc, char *argv[])
{
	int err;
	uint8_t type;

	err = 0;
	type = shell_strtoul(argv[1], 0, &err);
	if (err != 0) {
		shell_error(sh, "Invalid type %s", argv[1]);
		return -ENOEXEC;
	}

	err = secondary_transport_register(type);
	if (err != 0) {
		shell_error(sh, "Failed to register secondary transport %d", err);
		return err;
	}

	err = bt_bip_secondary_server_register(&secondary_bip, &secondary_server, type, NULL,
					       &bip_server_cb, &bip_app.client);
	if (err != 0) {
		shell_error(sh, "Failed to register secondary server %d", err);
		return err;
	}

	return 0;
}

static int cmd_sec_unreg(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	err = bt_bip_server_unregister(&secondary_server);
	if (err != 0) {
		shell_error(sh, "Failed to unregister secondary server %d", err);
		return err;
	}

	return 0;
}

static int cmd_sec_sdp_discover(const struct shell *sh, size_t argc, char *argv[])
{
	int err = 0;
	uint8_t type;
	static struct bt_uuid_16 uuid;
	static struct bt_sdp_discover_params discover_params;

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	type = shell_strtoul(argv[1], 0, &err);
	if (err != 0) {
		shell_error(sh, "Invalid type %s", argv[1]);
		return -ENOEXEC;
	}

	uuid.uuid.type = BT_UUID_TYPE_16;
	uuid.val = secondary_svclass_for_type(type);

	discover_params.uuid = &uuid.uuid;
	discover_params.func = secondary_sdp_discover_func;
	discover_params.pool = &secondary_sdp_client_pool;
	discover_params.type = BT_SDP_DISCOVER_SERVICE_SEARCH_ATTR;

	err = bt_sdp_discover(default_conn, &discover_params);
	if (err != 0) {
		shell_error(sh, "Failed to start secondary SDP discovery %d", err);
	}

	return err;
}

static int cmd_sec_connect_l2cap(const struct shell *sh, size_t argc, char *argv[])
{
	int err;
	uint16_t psm;

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	psm = (uint16_t)strtoul(argv[1], NULL, 16);
	if (psm == 0) {
		shell_error(sh, "Invalid psm");
		return -ENOEXEC;
	}

	if (secondary_transport_busy) {
		shell_error(sh, "Secondary transport is busy");
		return -EBUSY;
	}

	secondary_transport_busy = true;
	secondary_bip.ops = &secondary_bip_transport_ops;

	err = bt_bip_l2cap_connect(default_conn, &secondary_bip, psm);
	if (err != 0) {
		secondary_transport_busy = false;
		shell_error(sh, "Fail to connect to secondary PSM %d (err %d)", psm, err);
	} else {
		shell_print(sh, "BIP secondary L2CAP connection pending");
	}

	return err;
}

static int cmd_sec_conn(const struct shell *sh, size_t argc, char *argv[])
{
	int err;
	uint8_t type;

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	if (bip_app.conn == NULL) {
		shell_error(sh, "No bip transport connection");
		return -ENOEXEC;
	}

	err = 0;
	type = shell_strtoul(argv[1], 0, &err);
	if (err != 0) {
		shell_error(sh, "Invalid type %s", argv[1]);
		return -ENOEXEC;
	}

	err = bt_bip_secondary_client_connect(&secondary_bip, &secondary_client, type,
					      &bip_client_cb, bip_app.tx_buf, &bip_app.server);
	if (err != 0) {
		shell_error(sh, "Fail to send secondary conn req %d", err);
	} else {
		bip_app.tx_buf = NULL;
	}

	return err;
}

static int cmd_sec_server_conn(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t rsp_code;
	const char *rsp;
	int err;

	rsp = argv[1];
	if (!strcmp(rsp, "success")) {
		rsp_code = BT_OBEX_RSP_CODE_SUCCESS;
	} else if (!strcmp(rsp, "error")) {
		if (argc < 3) {
			shell_error(sh, "[rsp_code] is needed");
			return -ENOEXEC;
		}
		rsp_code = (uint8_t)strtoul(argv[2], NULL, 16);
	} else {
		shell_help(sh);
		return SHELL_CMD_HELP_PRINTED;
	}

	err = bt_bip_connect_rsp(&secondary_server, rsp_code, bip_app.tx_buf);
	if (err != 0) {
		shell_error(sh, "Fail to send sec conn rsp %d", err);
	} else {
		bip_app.tx_buf = NULL;
	}
	return err;
}

static int cmd_sec_server_abort(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t rsp_code;
	const char *rsp;
	int err;

	rsp = argv[1];
	if (!strcmp(rsp, "success")) {
		rsp_code = BT_OBEX_RSP_CODE_SUCCESS;
	} else if (!strcmp(rsp, "error")) {
		if (argc < 3) {
			shell_error(sh, "[rsp_code] is needed");
			return -ENOEXEC;
		}
		rsp_code = (uint8_t)strtoul(argv[2], NULL, 16);
	} else {
		shell_help(sh);
		return SHELL_CMD_HELP_PRINTED;
	}

	err = bt_bip_abort_rsp(&secondary_server, rsp_code, bip_app.tx_buf);
	if (err != 0) {
		shell_error(sh, "Fail to send sec abort rsp %d", err);
	} else {
		bip_app.tx_buf = NULL;
	}
	return err;
}

static int cmd_sec_server_get_partial_image(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t rsp_code;
	const char *rsp;
	int err;

	rsp = argv[1];
	if (!strcmp(rsp, "error")) {
		if (argc < 3) {
			shell_error(sh, "[rsp_code] is needed");
			return -ENOEXEC;
		}
		rsp_code = (uint8_t)strtoul(argv[2], NULL, 16);
	} else if (!strcmp(rsp, "noerror")) {
		rsp_code = BT_OBEX_RSP_CODE_SUCCESS;
	} else {
		shell_help(sh);
		return SHELL_CMD_HELP_PRINTED;
	}

	err = bt_bip_get_partial_image_rsp(&secondary_server, rsp_code, bip_app.tx_buf);
	if (err != 0) {
		shell_error(sh, "Fail to send sec get_partial_image rsp %d", err);
	} else {
		bip_app.tx_buf = NULL;
	}
	return err;
}

static int cmd_sec_get_partial_image(const struct shell *sh, size_t argc, char *argv[])
{
	int err;
	uint32_t partial_file_len = 0xffffffff;
	uint32_t partial_file_start_offset = 0;
	struct bt_obex_tlv appl_params[] = {
		{BT_BIP_APPL_PARAM_TAG_ID_PARTIAL_FILE_LEN, sizeof(partial_file_len),
		 (const uint8_t *)&partial_file_len},
		{BT_BIP_APPL_PARAM_TAG_ID_PARTIAL_FILE_START_OFFSET,
		 sizeof(partial_file_start_offset), (const uint8_t *)&partial_file_start_offset},
	};

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	if (bip_app.conn == NULL) {
		shell_error(sh, "No bip transport connection");
		return -ENOEXEC;
	}

	if (bip_app.tx_buf == NULL) {
		shell_error(sh, "No tx buffer, call 'bip alloc-buf' first");
		return -ENOBUFS;
	}

	err = bt_obex_add_header_conn_id(bip_app.tx_buf, bip_app.conn_id);
	if (err != 0) {
		shell_error(sh, "Fail to add conn id header %d", err);
		return err;
	}

	err = bt_obex_add_header_type(bip_app.tx_buf, sizeof(BT_BIP_HDR_TYPE_GET_PARTIAL_IMAGE),
				      BT_BIP_HDR_TYPE_GET_PARTIAL_IMAGE);
	if (err != 0) {
		shell_error(sh, "Fail to add type header %d", err);
		return err;
	}

	err = bt_obex_add_header_name(bip_app.tx_buf, sizeof(IMAGE_PARTIAL_FILE_NAME) - 1,
				      IMAGE_PARTIAL_FILE_NAME);
	if (err != 0) {
		shell_error(sh, "Fail to add name header %d", err);
		return err;
	}

	err = bt_obex_add_header_app_param(bip_app.tx_buf, ARRAY_SIZE(appl_params), appl_params);
	if (err != 0) {
		shell_error(sh, "Fail to add app param header %d", err);
		return err;
	}

	err = bt_bip_get_partial_image(&secondary_client, true, bip_app.tx_buf);
	if (err != 0) {
		shell_error(sh, "Fail to send sec get_partial_image req %d", err);
	} else {
		bip_app.tx_buf = NULL;
	}
	return err;
}

static int cmd_sec_abort(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	if (bip_app.conn == NULL) {
		shell_error(sh, "No bip transport connection");
		return -ENOEXEC;
	}

	err = bt_bip_abort(&secondary_client, bip_app.tx_buf);
	if (err != 0) {
		shell_error(sh, "Fail to send sec abort req %d", err);
	} else {
		bip_app.tx_buf = NULL;
	}
	return err;
}

SHELL_STATIC_SUBCMD_SET_CREATE(bip_server_cmds,
		SHELL_CMD_ARG(sec_set_feats_funcs, NULL,
			      "<features> <functions>",
			      cmd_sec_set_feats_funcs, 3, 0),
		SHELL_CMD_ARG(sec_reg, NULL, "<type> Register secondary server",
			      cmd_sec_reg, 2, 0),
		SHELL_CMD_ARG(sec_unreg, NULL, "Unregister secondary server",
			      cmd_sec_unreg, 1, 0),
		SHELL_CMD_ARG(sec_sdp_discover, NULL, "<type> Secondary SDP discover",
			      cmd_sec_sdp_discover, 2, 0),
		SHELL_CMD_ARG(sec_connect_l2cap, NULL, "<psm> Secondary L2CAP transport connect",
			      cmd_sec_connect_l2cap, 2, 0),
		SHELL_CMD_ARG(sec_conn, NULL, "<type> Secondary client connect",
			      cmd_sec_conn, 2, 0),
		SHELL_CMD_ARG(sec_server_conn, NULL, "<success|error> [rsp_code]",
			      cmd_sec_server_conn, 2, 1),
		SHELL_CMD_ARG(sec_server_abort, NULL, "<success|error> [rsp_code]",
			      cmd_sec_server_abort, 2, 1),
		SHELL_CMD_ARG(sec_server_get_partial_image, NULL,
			      "<noerror|error> [rsp_code]",
			      cmd_sec_server_get_partial_image, 2, 1),
		SHELL_CMD_ARG(sec_get_partial_image, NULL,
			      "Secondary client get_partial_image",
			      cmd_sec_get_partial_image, 1, 0),
		SHELL_CMD_ARG(sec_abort, NULL, "Secondary client abort",
			      cmd_sec_abort, 1, 0),
		SHELL_SUBCMD_SET_END
);

static int cmd_bip_server(const struct shell *sh, size_t argc, char **argv)
{
	if (argc == 1) {
		shell_help(sh);
		return 1;
	}

	shell_error(sh, "%s unknown parameter: %s", argv[0], argv[1]);

	return -ENOEXEC;
}

SHELL_CMD_ARG_REGISTER(bip_server_test, &bip_server_cmds,
		       "Bluetooth test bip server test sh commands", cmd_bip_server, 1, 1);
