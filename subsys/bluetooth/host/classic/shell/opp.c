/* opp.c - OPP shell commands */

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
#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/classic/sdp.h>
#include <zephyr/bluetooth/classic/opp.h>

#include <host/shell/bt.h>
#include <common/bt_shell_private.h>

#if defined(CONFIG_BT_OPP_CLIENT)

#define OPP_SDP_CLIENT_BUF_LEN 512

static struct bt_opp_client opp_client;
static uint8_t opp_rfcomm_channel;

/* Set while opp_client owns a transport, to keep the client from being reset
 * underneath a live connection.
 */
static bool opp_cli_transport_active;

/* Set by 'opp exchange' before its push is sent; cleared on the push's
 * final response, abort or disconnect.
 */
static bool opp_cli_exchange_pending;

/* Small static demo payloads for push commands. */
static const char opp_demo_vcard[] =
	"BEGIN:VCARD\r\nVERSION:2.1\r\nFN:Zephyr Shell\r\nEND:VCARD\r\n";
static const char opp_demo_vcal[] = "BEGIN:VCALENDAR\r\nVERSION:1.0\r\nBEGIN:VEVENT\r\n"
				    "SUMMARY:Shell Event\r\nEND:VEVENT\r\nEND:VCALENDAR\r\n";
static const char opp_demo_vnote[] =
	"BEGIN:VNOTE\r\nVERSION:1.1\r\nBODY:Hello from shell\r\nEND:VNOTE\r\n";
static const char opp_demo_vmsg[] = "BEGIN:VMSG\r\nVERSION:1.1\r\nBEGIN:VBODY\r\n"
				    "Body:Shell vMessage\r\nEND:VBODY\r\nEND:VMSG\r\n";

static uint8_t opp_sdp_cb(struct bt_conn *conn, struct bt_sdp_client_result *result,
			  const struct bt_sdp_discover_params *params)
{
	uint16_t rfcomm_ch;

	if (result == NULL || result->resp_buf == NULL) {
		/* Reached when no (further) record is resolved: the peer has no
		 * OPP service or the discovery failed. A discovery that resolves
		 * its records does not end with this call.
		 */
		if (opp_rfcomm_channel == 0) {
			bt_shell_print("OPP SDP: no OPP service found on remote device");
		}
		return BT_SDP_DISCOVER_UUID_STOP;
	}

	bt_shell_print("OPP SDP: got a service record (buf len=%u)", result->resp_buf->len);

	if (bt_sdp_get_proto_param(result->resp_buf, BT_SDP_PROTO_RFCOMM, &rfcomm_ch) == 0) {
		opp_rfcomm_channel = (uint8_t)rfcomm_ch;
		bt_shell_print("OPP SDP: RFCOMM channel = %u", opp_rfcomm_channel);
	} else {
		bt_shell_print("OPP SDP: no RFCOMM channel in this record");
	}

	return BT_SDP_DISCOVER_UUID_CONTINUE;
}

NET_BUF_POOL_FIXED_DEFINE(sdp_client_pool, CONFIG_BT_MAX_CONN, OPP_SDP_CLIENT_BUF_LEN, 8, NULL);

static struct bt_sdp_discover_params opp_sdp_params = {
	.type = BT_SDP_DISCOVER_SERVICE_SEARCH_ATTR,
	.uuid = BT_OPP_SDP_UUID,
	.func = opp_sdp_cb,
	.pool = &sdp_client_pool,
};

static void opp_cli_transport_connected(struct bt_conn *conn, struct bt_opp_client *client)
{
	bt_shell_print("OPP: transport connected");
}

static void opp_cli_transport_disconnected(struct bt_opp_client *client)
{
	opp_cli_transport_active = false;
	opp_cli_exchange_pending = false;

	bt_shell_print("OPP: transport disconnected");
}

static void opp_cli_connect(struct bt_opp_client *client, uint8_t rsp_code, uint8_t version,
			    uint16_t mopl, struct net_buf *buf)
{
	bt_shell_print("OPP: OBEX connect rsp=0x%02x version=0x%02x mopl=%u", rsp_code, version,
		       mopl);
}

static void opp_cli_disconnect(struct bt_opp_client *client, uint8_t rsp_code, struct net_buf *buf)
{
	bt_shell_print("OPP: OBEX disconnect rsp=0x%02x", rsp_code);
	opp_cli_exchange_pending = false;
}

static void opp_cli_push(struct bt_opp_client *client, uint8_t rsp_code, struct net_buf *buf)
{
	struct net_buf *pull_buf;
	int err;

	bt_shell_print("OPP: push rsp=0x%02x", rsp_code);

	/* Only the final response to a Business Card Exchange push goes on. */
	if (!opp_cli_exchange_pending || rsp_code == BT_OPP_RSP_CODE_CONTINUE) {
		return;
	}

	/* The exchange push is over: consume the flag whatever the answer. */
	opp_cli_exchange_pending = false;

	if (rsp_code != BT_OPP_RSP_CODE_SUCCESS && rsp_code != BT_OPP_RSP_CODE_OK) {
		bt_shell_print("OPP: exchange push rejected (rsp 0x%02x), skip pull", rsp_code);
		return;
	}

	/* Auto-pull the server's business card. */
	pull_buf = bt_opp_client_create_pdu(client, NULL);
	if (pull_buf == NULL) {
		bt_shell_print("OPP: exchange pull - create PDU failed");
		return;
	}

	/* Type header: required by OPP Spec section 5.6. */
	err = bt_obex_add_header_type(pull_buf, (uint16_t)(strlen(BT_OPP_TYPE_VCARD) + 1),
				      (const uint8_t *)BT_OPP_TYPE_VCARD);
	if (err != 0) {
		bt_shell_print("OPP: exchange pull - add type header failed (err %d)", err);
		net_buf_unref(pull_buf);
		return;
	}

	/*
	 * IrOBEX 1.2 section 8.4: the Default Get Object is requested
	 * with the Type header and an empty Name header, which is
	 * treated the same as no Name header.
	 */
	err = bt_obex_add_header_name(pull_buf, 0, NULL);
	if (err != 0) {
		bt_shell_print("OPP: exchange pull - add name header failed (err %d)", err);
		net_buf_unref(pull_buf);
		return;
	}

	err = bt_opp_client_pull(client, pull_buf);
	if (err != 0) {
		net_buf_unref(pull_buf);
		bt_shell_print("OPP: exchange pull failed (err %d)", err);
		return;
	}

	bt_shell_print("OPP: exchange - pulling server business card");
}

static void opp_cli_pull(struct bt_opp_client *client, uint8_t rsp_code, struct net_buf *buf)
{
	const uint8_t *body;
	uint16_t body_len;
	int err;

	bt_shell_print("OPP: pull rsp=0x%02x", rsp_code);

	if ((rsp_code != BT_OPP_RSP_CODE_SUCCESS && rsp_code != BT_OPP_RSP_CODE_CONTINUE) ||
	    buf == NULL) {
		return;
	}

	if (bt_obex_has_header(buf, BT_OBEX_HEADER_ID_END_BODY)) {
		err = bt_obex_get_header_end_body(buf, &body_len, &body);
	} else {
		err = bt_obex_get_header_body(buf, &body_len, &body);
	}

	if (err != 0) {
		bt_shell_print("OPP: pull response carries no body");
		return;
	}

	if (body != NULL && body_len > 0) {
		bt_shell_print("OPP: bcard data (%u bytes):\n%.*s", body_len, (int)body_len, body);
	}
}

static void opp_cli_abort(struct bt_opp_client *client, uint8_t rsp_code, struct net_buf *buf)
{
	bt_shell_print("OPP: abort rsp=0x%02x", rsp_code);

	/* An abort cancels any in-progress exchange; clear the pending flag so
	 * a subsequent push does not mistakenly trigger the pull phase.
	 */
	opp_cli_exchange_pending = false;
}

static const struct bt_opp_client_cb opp_client_cb = {
	.transport_connected = opp_cli_transport_connected,
	.transport_disconnected = opp_cli_transport_disconnected,
	.connect = opp_cli_connect,
	.disconnect = opp_cli_disconnect,
	.push = opp_cli_push,
	.pull = opp_cli_pull,
	.abort = opp_cli_abort,
};

static uint16_t ascii_to_utf16be(const char *str, uint8_t *out, uint16_t out_max)
{
	uint16_t len = 0;

	if (out_max < 2U) {
		return 0;
	}

	/* Keep room for the 2-byte UTF-16BE null terminator written below. */
	while (*str != '\0' && (len + 4U) <= out_max) {
		out[len++] = 0x00;
		out[len++] = (uint8_t)*str++;
	}

	out[len++] = 0x00;
	out[len++] = 0x00;

	return len;
}

/* opp discover - run SDP discovery for OPP on the current ACL connection */
static int cmd_opp_discover(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	opp_rfcomm_channel = 0;

	err = bt_sdp_discover(default_conn, &opp_sdp_params);
	if (err != 0) {
		shell_error(sh, "SDP discover failed (err %d)", err);
		return err;
	}

	shell_print(sh, "SDP discover started");
	return 0;
}

/* opp transport_connect - connect RFCOMM using SDP-discovered channel */
static int cmd_opp_transport_connect(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	if (opp_rfcomm_channel == 0) {
		shell_error(sh, "No RFCOMM channel - run 'opp discover' first");
		return -ENOENT;
	}

	/* Resetting the instance while a transport is up would defeat the
	 * -EALREADY check inside bt_opp_client_transport_connect().
	 */
	if (opp_cli_transport_active) {
		shell_error(sh, "OPP client transport already in use");
		return -EALREADY;
	}

	memset(&opp_client, 0, sizeof(opp_client));

	err = bt_opp_client_transport_connect(default_conn, &opp_client, &opp_client_cb,
					      opp_rfcomm_channel);
	if (err != 0) {
		shell_error(sh, "transport connect failed (err %d)", err);
		return err;
	}

	opp_cli_transport_active = true;

	shell_print(sh, "OPP transport connecting channel=%u", opp_rfcomm_channel);
	return 0;
}

/* opp transport_disconnect */
static int cmd_opp_transport_disconnect(const struct shell *sh, size_t argc, char *argv[])
{
	int err = bt_opp_client_transport_disconnect(&opp_client);

	if (err != 0) {
		shell_error(sh, "transport disconnect failed (err %d)", err);
		return err;
	}

	shell_print(sh, "OPP transport disconnecting");
	return 0;
}

/* opp connect [mopl] */
static int cmd_opp_connect(const struct shell *sh, size_t argc, char *argv[])
{
	uint16_t mopl = 0xffffU;
	int err;

	if (argc >= 2) {
		mopl = (uint16_t)strtoul(argv[1], NULL, 0);
	}

	err = bt_opp_client_connect(&opp_client, mopl, NULL);
	if (err != 0) {
		shell_error(sh, "OBEX connect failed (err %d)", err);
		return err;
	}

	shell_print(sh, "OPP OBEX connecting mopl=%u", mopl);
	return 0;
}

/* opp disconnect */
static int cmd_opp_disconnect(const struct shell *sh, size_t argc, char *argv[])
{
	int err = bt_opp_client_disconnect(&opp_client, NULL);

	if (err != 0) {
		shell_error(sh, "OBEX disconnect failed (err %d)", err);
		return err;
	}

	shell_print(sh, "OPP OBEX disconnecting");
	return 0;
}

/* opp push <vcard|vcal|vnote|vmsg> - push a demo object of the specified type */
static int cmd_opp_push(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_buf *buf;
	uint8_t name_utf16[32];
	uint16_t name_len;
	const char *type_str;
	const char *name_str;
	const char *data;
	size_t data_len;
	int err;

	if (argc < 2) {
		shell_error(sh, "Usage: push <vcard|vcal|vnote|vmsg>");
		return -EINVAL;
	}

	if (strcmp(argv[1], "vcard") == 0) {
		type_str = BT_OPP_TYPE_VCARD;
		name_str = "contact.vcf";
		data = opp_demo_vcard;
	} else if (strcmp(argv[1], "vcal") == 0) {
		type_str = BT_OPP_TYPE_VCAL;
		name_str = "event.vcs";
		data = opp_demo_vcal;
	} else if (strcmp(argv[1], "vnote") == 0) {
		type_str = BT_OPP_TYPE_VNOTE;
		name_str = "note.vnt";
		data = opp_demo_vnote;
	} else if (strcmp(argv[1], "vmsg") == 0) {
		type_str = BT_OPP_TYPE_VMESSAGE;
		name_str = "msg.vmg";
		data = opp_demo_vmsg;
	} else {
		shell_error(sh, "Unknown type '%s'. Use: vcard | vcal | vnote | vmsg", argv[1]);
		return -EINVAL;
	}

	data_len = strlen(data);

	buf = bt_opp_client_create_pdu(&opp_client, NULL);
	if (buf == NULL) {
		shell_error(sh, "create PDU failed");
		return -ENOMEM;
	}

	name_len = ascii_to_utf16be(name_str, name_utf16, sizeof(name_utf16));

	err = bt_obex_add_header_name(buf, name_len, name_utf16);
	if (err != 0) {
		shell_error(sh, "Fail to add name header %d", err);
		goto failed;
	}

	err = bt_obex_add_header_type(buf, (uint16_t)(strlen(type_str) + 1),
				      (const uint8_t *)type_str);
	if (err != 0) {
		shell_error(sh, "Fail to add type header %d", err);
		goto failed;
	}

	err = bt_obex_add_header_len(buf, (uint32_t)data_len);
	if (err != 0) {
		shell_error(sh, "Fail to add length header %d", err);
		goto failed;
	}

	err = bt_obex_add_header_end_body(buf, (uint16_t)data_len, (const uint8_t *)data);
	if (err != 0) {
		shell_error(sh, "Fail to add end-of-body header %d", err);
		goto failed;
	}

	/* Single-packet push: End-of-Body is already in buf, so final=true. */
	err = bt_opp_client_push(&opp_client, true, buf);
	if (err != 0) {
		shell_error(sh, "push failed (err %d)", err);
		goto failed;
	}

	shell_print(sh, "OPP pushing %s (%s)", argv[1], name_str);
	return 0;

failed:
	net_buf_unref(buf);
	return err;
}

/* opp pull - pull server's default business card */
static int cmd_opp_pull(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_buf *buf;
	int err;

	buf = bt_opp_client_create_pdu(&opp_client, NULL);
	if (buf == NULL) {
		shell_error(sh, "create PDU failed");
		return -ENOMEM;
	}

	err = bt_obex_add_header_type(buf, (uint16_t)(strlen(BT_OPP_TYPE_VCARD) + 1),
				      (const uint8_t *)BT_OPP_TYPE_VCARD);
	if (err != 0) {
		shell_error(sh, "Fail to add type header %d", err);
		goto failed;
	}

	/*
	 * IrOBEX 1.2 section 8.4: the Default Get Object is requested with the
	 * Type header and an empty Name header, which is treated the same as no
	 * Name header.
	 */
	err = bt_obex_add_header_name(buf, 0, NULL);
	if (err != 0) {
		shell_error(sh, "Fail to add name header %d", err);
		goto failed;
	}

	err = bt_opp_client_pull(&opp_client, buf);
	if (err != 0) {
		shell_error(sh, "pull failed (err %d)", err);
		goto failed;
	}

	shell_print(sh, "OPP pulling business card");
	return 0;

failed:
	net_buf_unref(buf);
	return err;
}

/* opp exchange - Business Card Exchange: push own vCard then pull server's */
static int cmd_opp_exchange(const struct shell *sh, size_t argc, char *argv[])
{
	/* Push phase (pull phase triggered from push callback in application). */
	struct net_buf *buf;
	uint8_t name_utf16[32];
	uint16_t name_len;
	size_t data_len = strlen(opp_demo_vcard);
	int err;

	if (opp_cli_exchange_pending) {
		shell_error(sh, "exchange already in progress");
		return -EBUSY;
	}

	buf = bt_opp_client_create_pdu(&opp_client, NULL);
	if (buf == NULL) {
		shell_error(sh, "create PDU failed");
		return -ENOMEM;
	}

	name_len = ascii_to_utf16be("my_card.vcf", name_utf16, sizeof(name_utf16));

	err = bt_obex_add_header_name(buf, name_len, name_utf16);
	if (err != 0) {
		shell_error(sh, "Fail to add name header %d", err);
		goto failed;
	}

	err = bt_obex_add_header_type(buf, (uint16_t)(strlen(BT_OPP_TYPE_VCARD) + 1),
				      (const uint8_t *)BT_OPP_TYPE_VCARD);
	if (err != 0) {
		shell_error(sh, "Fail to add type header %d", err);
		goto failed;
	}

	err = bt_obex_add_header_len(buf, (uint32_t)data_len);
	if (err != 0) {
		shell_error(sh, "Fail to add length header %d", err);
		goto failed;
	}

	err = bt_obex_add_header_end_body(buf, (uint16_t)data_len, (const uint8_t *)opp_demo_vcard);
	if (err != 0) {
		shell_error(sh, "Fail to add end-of-body header %d", err);
		goto failed;
	}

	/* Set before sending: the response is handled in the RX thread and may
	 * arrive before bt_opp_client_push() returns.
	 */
	opp_cli_exchange_pending = true;

	/* Single-packet push: End-of-Body is already in buf, so final=true. */
	err = bt_opp_client_push(&opp_client, true, buf);
	if (err != 0) {
		opp_cli_exchange_pending = false;
		shell_error(sh, "exchange push failed (err %d)", err);
		goto failed;
	}

	shell_print(sh, "OPP Business Card Exchange: push started (pull will auto-trigger)");
	return 0;

failed:
	net_buf_unref(buf);
	return err;
}

/* opp abort - abort current operation */
static int cmd_opp_abort(const struct shell *sh, size_t argc, char *argv[])
{
	int err = bt_opp_client_abort(&opp_client, NULL);

	if (err != 0) {
		shell_error(sh, "abort failed (err %d)", err);
		return err;
	}

	shell_print(sh, "OPP abort sent");
	return 0;
}

#endif /* CONFIG_BT_OPP_CLIENT */

#if defined(CONFIG_BT_OPP_SERVER)

/* The RFCOMM channel is left at 0 so that bt_opp_server_transport_register()
 * allocates a free one. The SDP record below points at this field and is
 * registered after the transport, so it advertises the allocated channel.
 */
static struct bt_opp_server_transport g_shell_transport_server;

static struct bt_sdp_attribute opp_shell_attrs[] = {
	BT_SDP_NEW_SERVICE,
	/* Service Class: OBEXObjectPush (0x1105) */
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
	/* Protocol Descriptor List: L2CAP + RFCOMM(channel) + OBEX */
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
				&g_shell_transport_server.server.rfcomm.channel
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
	/* Profile Descriptor: OBEXObjectPush v1.1 */
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
				/* OPP v1.1 Table 6.1 lists 0x0100 for v1.1, not 0x0101 */
				BT_SDP_ARRAY_16(0x0100)
			},
			)
		},
		)
	),
	/* Service Name: optional, default value "OBEX Object Push" per OPP Spec Table 6.1. */
	BT_SDP_SERVICE_NAME("OBEX Object Push"),
	/* Supported Formats List: advertise the concrete formats this demo server handles. */
	BT_SDP_LIST(
		BT_OPP_SDP_ATTR_SUPPORTED_FORMATS,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 12),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UINT8),
			BT_SDP_ARRAY_8(BT_OPP_FORMAT_VCARD_2_1)
		},
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UINT8),
			BT_SDP_ARRAY_8(BT_OPP_FORMAT_VCARD_3_0)
		},
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UINT8),
			BT_SDP_ARRAY_8(BT_OPP_FORMAT_VCAL_1_0)
		},
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UINT8),
			BT_SDP_ARRAY_8(BT_OPP_FORMAT_ICAL_2_0)
		},
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UINT8),
			BT_SDP_ARRAY_8(BT_OPP_FORMAT_VNOTE)
		},
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UINT8),
			BT_SDP_ARRAY_8(BT_OPP_FORMAT_VMESSAGE)
		},
		)
	),
};

static struct bt_sdp_record opp_shell_sdp_rec = BT_SDP_RECORD(opp_shell_attrs);

/* Single server instance - the shell supports one concurrent OPP server connection. */
struct opp_shell_server_inst {
	struct bt_opp_server server;
	bool in_use;
	bool push_final;
};

static struct opp_shell_server_inst g_shell_server_inst;

static void opp_srv_sh_transport_connected(struct bt_conn *conn, struct bt_opp_server *server)
{
	bt_shell_print("OPP Server: transport connected");
}

static void opp_srv_sh_transport_disconnected(struct bt_opp_server *server)
{
	int err;

	bt_shell_print("OPP Server: transport disconnected");

	/* Release the single instance slot for reuse on the next connection. */
	if (&g_shell_server_inst.server == server) {
		err = bt_opp_server_unregister(server);
		if (err != 0) {
			bt_shell_error("OPP Server: unregister failed (err %d)", err);
		}

		g_shell_server_inst.in_use = false;
	}
}

static void opp_srv_sh_connect(struct bt_opp_server *server, uint8_t version, uint16_t mopl,
			       struct net_buf *buf)
{
	bt_shell_print("OPP Server: OBEX connect req version=0x%02x mopl=%u", version, mopl);
	bt_shell_print("OPP Server: run 'opp server_connect_rsp success' to accept, or 'reject' to "
		       "refuse");
	/* Do NOT auto-reply here; the user drives the response via shell commands. */
}

static void opp_srv_sh_disconnect(struct bt_opp_server *server, struct net_buf *buf)
{
	bt_shell_print("OPP Server: OBEX disconnect req");
	bt_shell_print("OPP Server: run 'opp server_disconnect_rsp' to respond");
	/* Do NOT auto-reply; user drives the response via shell commands. */
}

static void opp_srv_sh_push(struct bt_opp_server *server, bool final, struct net_buf *buf)
{
	const uint8_t *name;
	uint16_t name_len;
	const uint8_t *type;
	uint16_t type_len;
	bool is_eob = bt_obex_has_header(buf, BT_OBEX_HEADER_ID_END_BODY);

	g_shell_server_inst.push_final = final;
	bt_shell_print("OPP Server: push req, final %s, data len %u", final ? "true" : "false",
		       buf->len);

	if (bt_obex_get_header_name(buf, &name_len, &name) == 0 && name_len > 0) {
		/* Name is UTF-16BE; print byte count only (conversion out of scope). */
		bt_shell_print("OPP Server: push name (UTF-16BE, %u bytes)", name_len);
	}

	if (bt_obex_get_header_type(buf, &type_len, &type) == 0 && type != NULL && type_len > 0) {
		bt_shell_print("OPP Server: push type: %.*s", (int)(type_len - 1), type);
	}

	if (is_eob) {
		const uint8_t *body;
		uint16_t body_len;

		if (bt_obex_get_header_end_body(buf, &body_len, &body) == 0 && body != NULL &&
		    body_len > 0) {
			bt_shell_print("OPP Server: push body (%u bytes):\n%.*s", body_len,
				       (int)body_len, body);
		}
	} else {
		const uint8_t *body;
		uint16_t body_len;

		if (bt_obex_get_header_body(buf, &body_len, &body) == 0 && body != NULL &&
		    body_len > 0) {
			bt_shell_print("OPP Server: push body chunk (%u bytes):\n%.*s", body_len,
				       (int)body_len, body);
		}
	}

	/* Do NOT auto-reply; let the user drive the response via shell command. */
	if (final) {
		bt_shell_print("OPP Server: push complete, run 'opp server_push_rsp "
			       "[success|reject|toobig|unsupported]' to respond");
	} else {
		bt_shell_print("OPP Server: run 'opp server_push_rsp "
			       "[continue|reject|toobig|unsupported]' to respond");
	}
}

static void opp_srv_sh_pull(struct bt_opp_server *server, bool final, struct net_buf *buf)
{
	bt_shell_print("OPP Server: business card pull request (final %d)", final);
	bt_shell_print("OPP Server: run 'opp server_pull_rsp [success|reject|notfound]' to "
		       "respond");
	/* Do NOT auto-reply; user drives the response via shell commands. */
}

static void opp_srv_sh_abort(struct bt_opp_server *server, struct net_buf *buf)
{
	bt_shell_print("OPP Server: ABORT");
	bt_shell_print("OPP Server: run 'opp server_abort_rsp' to respond");
	/* Do NOT auto-reply; user drives the response via shell command. */
}

static const struct bt_opp_server_cb opp_server_cb = {
	.transport_connected = opp_srv_sh_transport_connected,
	.transport_disconnected = opp_srv_sh_transport_disconnected,
	.connect = opp_srv_sh_connect,
	.disconnect = opp_srv_sh_disconnect,
	.push = opp_srv_sh_push,
	.pull = opp_srv_sh_pull,
	.abort = opp_srv_sh_abort,
};

static int opp_shell_transport_accept(struct bt_conn *conn,
				      struct bt_opp_server_transport *transport_server,
				      struct bt_opp_server **opp_server)
{
	int err;

	if (g_shell_server_inst.in_use) {
		return -ENOMEM;
	}

	memset(&g_shell_server_inst, 0, sizeof(g_shell_server_inst));

	err = bt_opp_server_register(&g_shell_server_inst.server, &opp_server_cb);
	if (err != 0) {
		return err;
	}

	/* Claim the slot only once the instance is registered. Rejecting the
	 * connection here means the transport is never established, so
	 * opp_srv_sh_transport_disconnected(), which is what releases the slot
	 * again, is never called.
	 */
	g_shell_server_inst.in_use = true;
	*opp_server = &g_shell_server_inst.server;

	return 0;
}

static bool g_server_registered;

/* opp server_register - register OPP RFCOMM server (OPP 1.1) */
static int cmd_opp_server_register(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	if (g_server_registered) {
		shell_warn(sh, "OPP server already registered");
		return 0;
	}

	/* The channel is only set once the transport is registered. */
	if (g_shell_transport_server.server.rfcomm.channel == 0U) {
		g_shell_transport_server.accept = opp_shell_transport_accept;
		err = bt_opp_server_transport_register(&g_shell_transport_server);
		if (err != 0) {
			g_shell_transport_server.server.rfcomm.channel = 0U;
			shell_error(sh, "transport register failed (err %d)", err);
			return err;
		}
	}

	err = bt_sdp_register_service(&opp_shell_sdp_rec);
	if (err != 0 && err != -EEXIST) {
		shell_error(sh, "SDP register failed (err %d)", err);
		return err;
	}

	g_server_registered = true;
	shell_print(sh, "OPP server registered: RFCOMM channel=%u",
		    g_shell_transport_server.server.rfcomm.channel);
	return 0;
}

/* opp server_disconnect_rsp - respond to OBEX DISCONNECT request */
static int cmd_opp_server_disconnect_rsp(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	if (!g_shell_server_inst.in_use) {
		shell_error(sh, "No active server connection");
		return -ENODEV;
	}

	err = bt_opp_server_disconnect_rsp(&g_shell_server_inst.server, BT_OPP_RSP_CODE_SUCCESS,
					   NULL);
	if (err != 0) {
		shell_error(sh, "disconnect rsp failed (err %d)", err);
		return err;
	}

	shell_print(sh, "OPP server disconnect rsp sent");
	return 0;
}

/* opp server_connect_rsp [success|reject] [mopl] - respond to incoming connect */
static int cmd_opp_server_connect_rsp(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t rsp = BT_OPP_RSP_CODE_SUCCESS;
	uint16_t mopl = 0xffffU;
	int err;

	if (!g_shell_server_inst.in_use) {
		shell_error(sh, "No active server connection");
		return -ENODEV;
	}

	if (argc >= 2) {
		if (strcmp(argv[1], "success") == 0) {
			rsp = BT_OPP_RSP_CODE_SUCCESS;
		} else if (strcmp(argv[1], "reject") == 0) {
			rsp = BT_OPP_RSP_CODE_FORBIDDEN;
		} else {
			shell_error(sh, "Unknown response '%s'. Use: success | reject", argv[1]);
			return -EINVAL;
		}
	}

	if (argc >= 3) {
		mopl = (uint16_t)strtoul(argv[2], NULL, 0);
	}

	err = bt_opp_server_connect_rsp(&g_shell_server_inst.server, mopl, rsp, NULL);
	if (err != 0) {
		shell_error(sh, "connect rsp failed (err %d)", err);
		return err;
	}

	shell_print(sh, "OPP server connect rsp sent (0x%02x)", rsp);
	return 0;
}

/* opp server_push_rsp [continue|success|reject|toobig|unsupported] - respond to push.
 * Default: success for a final PUT, continue otherwise.
 */
static int cmd_opp_server_push_rsp(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t rsp;
	int err;

	if (!g_shell_server_inst.in_use) {
		shell_error(sh, "No active server connection");
		return -ENODEV;
	}

	rsp = g_shell_server_inst.push_final ? BT_OPP_RSP_CODE_SUCCESS : BT_OPP_RSP_CODE_CONTINUE;

	if (argc >= 2) {
		if (strcmp(argv[1], "success") == 0) {
			rsp = BT_OPP_RSP_CODE_SUCCESS;
		} else if (strcmp(argv[1], "continue") == 0) {
			rsp = BT_OPP_RSP_CODE_CONTINUE;
		} else if (strcmp(argv[1], "reject") == 0) {
			rsp = BT_OPP_RSP_CODE_FORBIDDEN;
		} else if (strcmp(argv[1], "toobig") == 0) {
			rsp = BT_OPP_RSP_CODE_ENTITY_TOO_LARGE;
		} else if (strcmp(argv[1], "unsupported") == 0) {
			rsp = BT_OPP_RSP_CODE_UNSUPP_MEDIA_TYPE;
		} else {
			shell_error(sh, "Unknown response '%s'. Use: "
				    "success | continue | reject | toobig | unsupported", argv[1]);
			return -EINVAL;
		}
	}

	err = bt_opp_server_push_rsp(&g_shell_server_inst.server, rsp, NULL);
	if (err != 0) {
		shell_error(sh, "push rsp failed (err %d)", err);
		return err;
	}

	shell_print(sh, "OPP server push rsp sent (0x%02x)", rsp);
	return 0;
}

/* opp server_pull_rsp [success|reject|notfound] - respond to business card pull */
static int cmd_opp_server_pull_rsp(const struct shell *sh, size_t argc, char *argv[])
{
	static const char shell_vcard[] =
		"BEGIN:VCARD\r\nVERSION:2.1\r\nFN:Zephyr Shell Server\r\nEND:VCARD\r\n";
	uint8_t rsp_code = BT_OPP_RSP_CODE_SUCCESS;
	int err;

	if (!g_shell_server_inst.in_use) {
		shell_error(sh, "No active server connection");
		return -ENODEV;
	}

	if (argc >= 2) {
		if (strcmp(argv[1], "success") == 0) {
			rsp_code = BT_OPP_RSP_CODE_SUCCESS;
		} else if (strcmp(argv[1], "reject") == 0) {
			rsp_code = BT_OPP_RSP_CODE_FORBIDDEN;
		} else if (strcmp(argv[1], "notfound") == 0) {
			rsp_code = BT_OPP_RSP_CODE_NOT_FOUND;
		} else {
			shell_error(sh, "Unknown response '%s'. Use: success | reject | notfound",
				    argv[1]);
			return -EINVAL;
		}
	}

	if (rsp_code == BT_OPP_RSP_CODE_SUCCESS) {
		/* Send the built-in shell vCard as the default business card. */
		size_t len = strlen(shell_vcard);
		struct net_buf *buf = bt_opp_server_create_pdu(&g_shell_server_inst.server, NULL);

		if (buf == NULL) {
			shell_error(sh, "create PDU failed");
			return -ENOMEM;
		}

		err = bt_obex_add_header_len(buf, (uint32_t)len);
		if (err != 0) {
			shell_error(sh, "Fail to add length header %d", err);
			net_buf_unref(buf);
			return err;
		}

		err = bt_obex_add_header_end_body(buf, (uint16_t)len, (const uint8_t *)shell_vcard);
		if (err != 0) {
			shell_error(sh, "Fail to add end-of-body header %d", err);
			net_buf_unref(buf);
			return err;
		}

		err = bt_opp_server_pull_rsp(&g_shell_server_inst.server, BT_OPP_RSP_CODE_SUCCESS,
					     buf);
		if (err != 0) {
			net_buf_unref(buf);
		}
	} else {
		err = bt_opp_server_pull_rsp(&g_shell_server_inst.server, rsp_code, NULL);
	}

	if (err != 0) {
		shell_error(sh, "pull rsp failed (err %d)", err);
		return err;
	}

	shell_print(sh, "OPP server pull rsp sent (0x%02x)", rsp_code);
	return 0;
}

/* opp server_abort_rsp - respond to abort */
static int cmd_opp_server_abort_rsp(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	if (!g_shell_server_inst.in_use) {
		shell_error(sh, "No active server connection");
		return -ENODEV;
	}

	err = bt_opp_server_abort_rsp(&g_shell_server_inst.server, BT_OPP_RSP_CODE_SUCCESS, NULL);
	if (err != 0) {
		shell_error(sh, "abort rsp failed (err %d)", err);
		return err;
	}

	shell_print(sh, "OPP server abort rsp sent");
	return 0;
}

#endif /* CONFIG_BT_OPP_SERVER */

SHELL_STATIC_SUBCMD_SET_CREATE(
	opp_cmds,
#if defined(CONFIG_BT_OPP_CLIENT)
	SHELL_CMD_ARG(discover, NULL, "Discover OPP service RFCOMM channel via SDP",
		      cmd_opp_discover, 1, 0),
	SHELL_CMD_ARG(transport_connect, NULL, "Connect RFCOMM using SDP-discovered channel",
		      cmd_opp_transport_connect, 1, 0),
	SHELL_CMD_ARG(transport_disconnect, NULL, "Disconnect RFCOMM", cmd_opp_transport_disconnect,
		      1, 0),
	SHELL_CMD_ARG(connect, NULL, "[mopl]  Establish OBEX connection", cmd_opp_connect, 1, 1),
	SHELL_CMD_ARG(disconnect, NULL, "Terminate OBEX connection", cmd_opp_disconnect, 1, 0),
	SHELL_CMD_ARG(push, NULL, "<vcard|vcal|vnote|vmsg>  Push a demo object to the server",
		      cmd_opp_push, 2, 0),
	SHELL_CMD_ARG(pull, NULL, "Pull the server's default business card", cmd_opp_pull, 1, 0),
	SHELL_CMD_ARG(exchange, NULL, "Business Card Exchange: push own vCard then pull server's",
		      cmd_opp_exchange, 1, 0),
	SHELL_CMD_ARG(abort, NULL, "Abort the current operation", cmd_opp_abort, 1, 0),
#endif /* CONFIG_BT_OPP_CLIENT */
#if defined(CONFIG_BT_OPP_SERVER)
	SHELL_CMD_ARG(server_register, NULL, "Register OPP Push Server (RFCOMM, OPP 1.1)",
		      cmd_opp_server_register, 1, 0),
	SHELL_CMD_ARG(server_connect_rsp, NULL,
		      "[success|reject] [mopl]  Send OBEX CONNECT response",
		      cmd_opp_server_connect_rsp, 1, 2),
	SHELL_CMD_ARG(server_disconnect_rsp, NULL, "Send OBEX DISCONNECT response",
		      cmd_opp_server_disconnect_rsp, 1, 0),
	SHELL_CMD_ARG(server_push_rsp, NULL,
		      "[continue|success|reject|toobig|unsupported]  Send PUT response "
		      "(default: success if final, else continue)",
		      cmd_opp_server_push_rsp, 1, 1),
	SHELL_CMD_ARG(server_pull_rsp, NULL,
		      "[success|reject|notfound]  Send GET response for business card pull",
		      cmd_opp_server_pull_rsp, 1, 1),
	SHELL_CMD_ARG(server_abort_rsp, NULL, "Send ABORT response", cmd_opp_server_abort_rsp, 1,
		      0),
#endif /* CONFIG_BT_OPP_SERVER */
	SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(opp, &opp_cmds, "Bluetooth OPP shell commands", NULL);
