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
#include <zephyr/sys/byteorder.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/classic/sdp.h>
#include <zephyr/bluetooth/classic/opp.h>

#include "bredr.h"

#include <host/shell/bt.h>
#include <common/bt_shell_private.h>

/* The shell Bluetooth layer exposes the currently active ACL connection as
 * extern default_conn (defined in subsys/bluetooth/host/shell/bt.c).
 */
extern struct bt_conn *default_conn;

#if defined(CONFIG_BT_OPP_CLIENT)

static struct bt_opp_client opp_client;
static uint8_t opp_rfcomm_channel;

/* Flag set by 'opp exchange' to auto-pull bcard after push completes. */
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
	if (result == NULL || result->resp_buf == NULL) {
		/* Discovery finished (or empty record) - print a summary. */
		bt_shell_print("OPP SDP: discovery complete: RFCOMM ch=%u\n", opp_rfcomm_channel);
		if (opp_rfcomm_channel == 0) {
			bt_shell_print("OPP SDP: no OPP service found on remote device\n");
		}
		return BT_SDP_DISCOVER_UUID_STOP;
	}

	uint16_t rfcomm_ch;

	bt_shell_print("OPP SDP: got a service record (buf len=%u)\n", result->resp_buf->len);

	if (bt_sdp_get_proto_param(result->resp_buf, BT_SDP_PROTO_RFCOMM, &rfcomm_ch) == 0) {
		opp_rfcomm_channel = (uint8_t)rfcomm_ch;
		bt_shell_print("OPP SDP: RFCOMM channel = %u\n", opp_rfcomm_channel);
	} else {
		bt_shell_print("OPP SDP: no RFCOMM channel in this record\n");
	}

	return BT_SDP_DISCOVER_UUID_CONTINUE;
}

NET_BUF_POOL_DEFINE(opp_sdp_pool, 10, BT_L2CAP_BUF_SIZE(CONFIG_BT_L2CAP_TX_MTU),
		    CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

static struct bt_sdp_discover_params opp_sdp_params = {
	.type = BT_SDP_DISCOVER_SERVICE_SEARCH_ATTR,
	.uuid = BT_OPP_SDP_UUID,
	.func = opp_sdp_cb,
	.pool = &opp_sdp_pool,
};

static void opp_cli_rfcomm_connected(struct bt_conn *conn, struct bt_opp_client *client)
{
	bt_shell_print("OPP: RFCOMM connected\n");
}

static void opp_cli_rfcomm_disconnected(struct bt_opp_client *client)
{
	bt_shell_print("OPP: RFCOMM disconnected\n");
}

static void opp_cli_connect(struct bt_opp_client *client, enum bt_opp_rsp_code rsp_code,
			    uint8_t version, uint16_t mopl, struct net_buf *buf)
{
	bt_shell_print("OPP: OBEX connect rsp=0x%02x version=0x%02x mopl=%u\n", rsp_code, version,
		       mopl);
}

static void opp_cli_disconnect(struct bt_opp_client *client, enum bt_opp_rsp_code rsp_code,
			       struct net_buf *buf)
{
	bt_shell_print("OPP: OBEX disconnect rsp=0x%02x\n", rsp_code);
}

static void opp_cli_push(struct bt_opp_client *client, enum bt_opp_rsp_code rsp_code,
			 struct net_buf *buf)
{
	bt_shell_print("OPP: push rsp=0x%02x\n", rsp_code);

	/* If this push was part of a Business Card Exchange, auto-pull server's bcard. */
	if (opp_cli_exchange_pending &&
	    (rsp_code == BT_OPP_RSP_CODE_SUCCESS || rsp_code == BT_OPP_RSP_CODE_OK)) {
		struct net_buf *pull_buf;

		opp_cli_exchange_pending = false;
		pull_buf = bt_opp_client_create_pdu(client, NULL);
		if (pull_buf == NULL) {
			bt_shell_print("OPP: exchange pull - create PDU failed\n");
			return;
		}
		/* Type header: required by spec section 5.6. */
		(void)bt_obex_add_header_type(pull_buf, (uint16_t)(strlen(BT_OPP_TYPE_VCARD) + 1),
					      (const uint8_t *)BT_OPP_TYPE_VCARD);
		/*
		 * Empty Name header (2-byte UTF-16BE null terminator).
		 * OPP Spec Table 5.3 marks Name as Mandatory for GET; section 5.6
		 * states the Name is not used.  Sending an empty Name header
		 * satisfies both requirements and improves interoperability.
		 */
		{
			static const uint8_t empty_name[2] = {0x00, 0x00};

			(void)bt_obex_add_header_name(pull_buf, sizeof(empty_name), empty_name);
		}
		if (bt_opp_client_pull_bcard(client, pull_buf) == 0) {
			bt_shell_print("OPP: exchange - pulling server business card\n");
		} else {
			net_buf_unref(pull_buf);
			bt_shell_print("OPP: exchange pull_bcard failed\n");
		}
	}
}

static void opp_cli_pull_bcard(struct bt_opp_client *client, enum bt_opp_rsp_code rsp_code,
			       struct net_buf *buf)
{
	const uint8_t *body;
	uint16_t body_len;

	bt_shell_print("OPP: pull_bcard rsp=0x%02x\n", rsp_code);

	if ((rsp_code == BT_OPP_RSP_CODE_SUCCESS || rsp_code == BT_OPP_RSP_CODE_CONTINUE) &&
	    buf != NULL) {
		bool eob = bt_obex_has_header(buf, BT_OBEX_HEADER_ID_END_BODY);

		if (eob) {
			(void)bt_obex_get_header_end_body(buf, &body_len, &body);
		} else {
			(void)bt_obex_get_header_body(buf, &body_len, &body);
		}

		if (body != NULL && body_len > 0) {
			bt_shell_print("OPP: bcard data (%u bytes):\n%.*s\n", body_len, body_len,
				       body);
		}
	}
}

static void opp_cli_abort(struct bt_opp_client *client, enum bt_opp_rsp_code rsp_code,
			  struct net_buf *buf)
{
	bt_shell_print("OPP: abort rsp=0x%02x\n", rsp_code);

	/* An abort cancels any in-progress exchange; clear the pending flag so
	 * a subsequent push does not mistakenly trigger the pull phase.
	 */
	opp_cli_exchange_pending = false;
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

static uint16_t ascii_to_utf16be(const char *str, uint8_t *out, uint16_t out_max)
{
	uint16_t len = 0;

	while (*str != '\0' && len + 2 < out_max) {
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

	if (err) {
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
		shell_error(sh, "No RFCOMM channel - run sdp_discover first");
		return -ENOENT;
	}

	memset(&opp_client, 0, sizeof(opp_client));

	err = bt_opp_client_connect_rfcomm(default_conn, &opp_client, &opp_client_cb,
					   opp_rfcomm_channel);
	if (err) {
		shell_error(sh, "RFCOMM connect failed (err %d)", err);
		return err;
	}

	shell_print(sh, "OPP RFCOMM connecting channel=%u", opp_rfcomm_channel);
	return 0;
}

/* opp transport_disconnect */
static int cmd_opp_transport_disconnect(const struct shell *sh, size_t argc, char *argv[])
{
	int err = bt_opp_client_disconnect_rfcomm(&opp_client);

	if (err) {
		shell_error(sh, "RFCOMM disconnect failed (err %d)", err);
		return err;
	}

	shell_print(sh, "OPP RFCOMM disconnecting");
	return 0;
}

/* opp connect [mopl] */
static int cmd_opp_connect(const struct shell *sh, size_t argc, char *argv[])
{
	uint16_t mopl = 0xFFFFU;
	int err;

	if (argc >= 2) {
		mopl = (uint16_t)strtoul(argv[1], NULL, 0);
	}

	err = bt_opp_client_connect(&opp_client, mopl, NULL);
	if (err) {
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

	if (err) {
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
	(void)bt_obex_add_header_name(buf, name_len, name_utf16);
	(void)bt_obex_add_header_type(buf, (uint16_t)(strlen(type_str) + 1),
				      (const uint8_t *)type_str);
	(void)bt_obex_add_header_len(buf, (uint32_t)data_len);
	(void)bt_obex_add_header_end_body(buf, (uint16_t)data_len, (const uint8_t *)data);

	/* Single-packet push: End-of-Body is already in buf, so final=true. */
	err = bt_opp_client_push(&opp_client, true, buf);
	if (err) {
		net_buf_unref(buf);
		shell_error(sh, "push failed (err %d)", err);
		return err;
	}

	shell_print(sh, "OPP pushing %s (%s)", argv[1], name_str);
	return 0;
}

/* opp pull_bcard - pull server's default business card */
static int cmd_opp_pull_bcard(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_buf *buf;
	int err;

	buf = bt_opp_client_create_pdu(&opp_client, NULL);
	if (buf == NULL) {
		shell_error(sh, "create PDU failed");
		return -ENOMEM;
	}

	(void)bt_obex_add_header_type(buf, (uint16_t)(strlen(BT_OPP_TYPE_VCARD) + 1),
				      (const uint8_t *)BT_OPP_TYPE_VCARD);
	/*
	 * Empty Name header (2-byte UTF-16BE null terminator).
	 * OPP Spec Table 5.3 marks Name as Mandatory for GET; section 5.6
	 * states the Name is not used.  Sending an empty Name header
	 * satisfies both requirements and improves interoperability.
	 */
	{
		static const uint8_t empty_name[2] = {0x00, 0x00};

		(void)bt_obex_add_header_name(buf, sizeof(empty_name), empty_name);
	}

	err = bt_opp_client_pull_bcard(&opp_client, buf);
	if (err) {
		net_buf_unref(buf);
		shell_error(sh, "pull_bcard failed (err %d)", err);
		return err;
	}

	shell_print(sh, "OPP pulling business card");
	return 0;
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

	buf = bt_opp_client_create_pdu(&opp_client, NULL);
	if (buf == NULL) {
		shell_error(sh, "create PDU failed");
		return -ENOMEM;
	}

	name_len = ascii_to_utf16be("my_card.vcf", name_utf16, sizeof(name_utf16));
	(void)bt_obex_add_header_name(buf, name_len, name_utf16);
	(void)bt_obex_add_header_type(buf, (uint16_t)(strlen(BT_OPP_TYPE_VCARD) + 1),
				      (const uint8_t *)BT_OPP_TYPE_VCARD);
	(void)bt_obex_add_header_len(buf, (uint32_t)data_len);
	(void)bt_obex_add_header_end_body(buf, (uint16_t)data_len, (const uint8_t *)opp_demo_vcard);

	/* Single-packet push: End-of-Body is already in buf, so final=true. */
	err = bt_opp_client_push(&opp_client, true, buf);
	if (err) {
		net_buf_unref(buf);
		shell_error(sh, "exchange push failed (err %d)", err);
		return err;
	}

	opp_cli_exchange_pending = true;
	shell_print(sh, "OPP Business Card Exchange: push started (pull will auto-trigger)");
	return 0;
}

/* opp abort - abort current operation */
static int cmd_opp_abort(const struct shell *sh, size_t argc, char *argv[])
{
	int err = bt_opp_client_abort(&opp_client, NULL);

	if (err) {
		shell_error(sh, "abort failed (err %d)", err);
		return err;
	}

	shell_print(sh, "OPP abort sent");
	return 0;
}

#endif /* CONFIG_BT_OPP_CLIENT */

#if defined(CONFIG_BT_OPP_SERVER)

/* Pre-assign a fixed RFCOMM channel for the shell OPP server so the SDP
 * record can reference it at compile time.  Channel 5 is used by the
 * opp_push_server sample and is a reasonable default for the shell.
 */
#define OPP_SHELL_RFCOMM_CHANNEL 5U

static struct bt_sdp_attribute opp_shell_attrs[] = {
	BT_SDP_NEW_SERVICE,
	/* Service Class: OBEXObjectPush (0x1105) */
	BT_SDP_LIST(
		BT_SDP_ATTR_SVCLASS_ID_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
			BT_SDP_ARRAY_16(0x1105)
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
				BT_SDP_ARRAY_8(OPP_SHELL_RFCOMM_CHANNEL)
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
				BT_SDP_ARRAY_16(0x1105)
			},
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
				BT_SDP_ARRAY_16(0x0100) /* v1.1 */
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

/* Single server session - the shell supports one concurrent OPP server connection. */
struct opp_shell_session {
	struct bt_opp_server server;
	bool in_use;
};

static struct opp_shell_session g_shell_session;

static void opp_srv_sh_rfcomm_connected(struct bt_conn *conn, struct bt_opp_server *server)
{
	bt_shell_print("OPP Server: RFCOMM connected\n");
}

static void opp_srv_sh_rfcomm_disconnected(struct bt_opp_server *server)
{
	bt_shell_print("OPP Server: RFCOMM disconnected\n");

	/* Release the single session slot for reuse on the next connection. */
	if (&g_shell_session.server == server) {
		g_shell_session.in_use = false;
	}
}

static void opp_srv_sh_connect(struct bt_opp_server *server, uint8_t version, uint16_t mopl,
			       struct net_buf *buf)
{
	bt_shell_print("OPP Server: OBEX connect req version=0x%02x mopl=%u\n", version, mopl);
	bt_shell_print("OPP Server: run 'opp server_connect_rsp success' to accept, or 'reject' to "
		       "refuse\n");
	/* Do NOT auto-reply here; the user drives the response via shell commands. */
}

static void opp_srv_sh_disconnect(struct bt_opp_server *server, struct net_buf *buf)
{
	bt_shell_print("OPP Server: OBEX disconnect req\n");
	bt_shell_print("OPP Server: run 'opp server_disconnect_rsp' to respond\n");
	/* Do NOT auto-reply; user drives the response via shell commands. */
}

static void opp_srv_sh_push(struct bt_opp_server *server, bool final, struct net_buf *buf)
{
	const uint8_t *name;
	uint16_t name_len;
	const uint8_t *type;
	uint16_t type_len;
	bool is_eob = bt_obex_has_header(buf, BT_OBEX_HEADER_ID_END_BODY);

	if (bt_obex_get_header_name(buf, &name_len, &name) == 0 && name_len > 0) {
		/* Name is UTF-16BE; print byte count only (conversion out of scope). */
		bt_shell_print("OPP Server: push name (UTF-16BE, %u bytes)\n", name_len);
	}

	if (bt_obex_get_header_type(buf, &type_len, &type) == 0 && type != NULL && type_len > 0) {
		bt_shell_print("OPP Server: push type: %.*s\n", (int)(type_len - 1), type);
	}

	if (is_eob) {
		const uint8_t *body;
		uint16_t body_len;

		if (bt_obex_get_header_end_body(buf, &body_len, &body) == 0 && body != NULL &&
		    body_len > 0) {
			bt_shell_print("OPP Server: push body (%u bytes):\n%.*s\n", body_len,
				       (int)body_len, body);
		}
		bt_shell_print("OPP Server: push complete (EOB)\n");
	} else {
		const uint8_t *body;
		uint16_t body_len;

		if (bt_obex_get_header_body(buf, &body_len, &body) == 0 && body != NULL &&
		    body_len > 0) {
			bt_shell_print("OPP Server: push body chunk (%u bytes):\n%.*s\n", body_len,
				       (int)body_len, body);
		}
	}

	/* Do NOT auto-reply; let the user drive the response via shell command.
	 * Use: opp server_push_rsp [continue|success|reject|toobig|unsupported]
	 */
	bt_shell_print("OPP Server: run 'opp server_push_rsp "
		       "[continue|success|reject|toobig|unsupported]' to respond\n");
}

static void opp_srv_sh_pull_bcard(struct bt_opp_server *server, struct net_buf *buf)
{
	bt_shell_print("OPP Server: pull_bcard request\n");
	bt_shell_print("OPP Server: run 'opp server_pull_bcard_rsp [success|reject|notfound]' to "
		       "respond\n");
	/* Do NOT auto-reply; user drives the response via shell commands. */
}

static void opp_srv_sh_abort(struct bt_opp_server *server, struct net_buf *buf)
{
	bt_shell_print("OPP Server: ABORT\n");
	bt_shell_print("OPP Server: run 'opp server_abort_rsp' to respond\n");
	/* Do NOT auto-reply; user drives the response via shell command. */
}

static struct bt_opp_server_cb opp_server_cb = {
	.rfcomm_connected = opp_srv_sh_rfcomm_connected,
	.rfcomm_disconnected = opp_srv_sh_rfcomm_disconnected,
	.connect = opp_srv_sh_connect,
	.disconnect = opp_srv_sh_disconnect,
	.push = opp_srv_sh_push,
	.pull_bcard = opp_srv_sh_pull_bcard,
	.abort = opp_srv_sh_abort,
};

static int opp_shell_rfcomm_accept(struct bt_conn *conn, struct bt_opp_server_rfcomm *rfcomm_server,
				   struct bt_opp_server **opp_server)
{
	if (g_shell_session.in_use) {
		return -ENOMEM;
	}

	memset(&g_shell_session, 0, sizeof(g_shell_session));
	g_shell_session.in_use = true;
	*opp_server = &g_shell_session.server;
	return bt_opp_server_register(*opp_server, &opp_server_cb);
}

static struct bt_opp_server_rfcomm g_shell_rfcomm_server = {
	.server = {.rfcomm = {.channel = OPP_SHELL_RFCOMM_CHANNEL}},
	.accept = opp_shell_rfcomm_accept,
};

static bool g_server_registered;

/* opp server_register - register OPP RFCOMM server (OPP 1.1) */
static int cmd_opp_server_register(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	if (g_server_registered) {
		shell_warn(sh, "OPP server already registered");
		return 0;
	}

	err = bt_sdp_register_service(&opp_shell_sdp_rec);
	if (err && err != -EEXIST) {
		shell_error(sh, "SDP register failed (err %d)", err);
		return err;
	}

	err = bt_opp_server_rfcomm_register(&g_shell_rfcomm_server);
	if (err) {
		shell_error(sh, "RFCOMM register failed (err %d)", err);
		return err;
	}

	g_server_registered = true;
	shell_print(sh, "OPP server registered: RFCOMM channel=%u",
		    g_shell_rfcomm_server.server.rfcomm.channel);
	return 0;
}

/* opp server_disconnect_rsp - respond to OBEX DISCONNECT request */
static int cmd_opp_server_disconnect_rsp(const struct shell *sh, size_t argc, char *argv[])
{
	if (g_shell_session.server.cb == NULL) {
		shell_error(sh, "No active server session");
		return -ENODEV;
	}

	int err = bt_opp_server_disconnect_rsp(&g_shell_session.server, BT_OPP_RSP_CODE_SUCCESS,
					       NULL);
	if (err) {
		shell_error(sh, "disconnect_rsp failed (err %d)", err);
		return err;
	}

	shell_print(sh, "OPP server disconnect rsp sent");
	return 0;
}

/* opp server_connect_rsp <success|reject> [mopl] - respond to incoming connect */
static int cmd_opp_server_connect_rsp(const struct shell *sh, size_t argc, char *argv[])
{
	if (g_shell_session.server.cb == NULL) {
		shell_error(sh, "No active server session");
		return -ENODEV;
	}

	uint8_t rsp = BT_OPP_RSP_CODE_SUCCESS;
	uint16_t mopl = 0xFFFFU;

	if (argc >= 2 && strcmp(argv[1], "reject") == 0) {
		rsp = BT_OPP_RSP_CODE_FORBIDDEN;
	}
	if (argc >= 3) {
		mopl = (uint16_t)strtoul(argv[2], NULL, 0);
	}

	int err = bt_opp_server_connect_rsp(&g_shell_session.server, mopl, rsp, NULL);

	if (err) {
		shell_error(sh, "connect_rsp failed (err %d)", err);
		return err;
	}

	shell_print(sh, "OPP server connect rsp sent (0x%02x)", rsp);
	return 0;
}

/* opp server_push_rsp <continue|success|reject> - respond to push */
static int cmd_opp_server_push_rsp(const struct shell *sh, size_t argc, char *argv[])
{
	if (g_shell_session.server.cb == NULL) {
		shell_error(sh, "No active server session");
		return -ENODEV;
	}

	uint8_t rsp = BT_OPP_RSP_CODE_SUCCESS;

	if (argc >= 2) {
		if (strcmp(argv[1], "continue") == 0) {
			rsp = BT_OPP_RSP_CODE_CONTINUE;
		} else if (strcmp(argv[1], "reject") == 0) {
			rsp = BT_OPP_RSP_CODE_FORBIDDEN;
		} else if (strcmp(argv[1], "toobig") == 0) {
			rsp = BT_OPP_RSP_CODE_ENTITY_TOO_LARGE;
		} else if (strcmp(argv[1], "unsupported") == 0) {
			rsp = BT_OPP_RSP_CODE_UNSUPP_MEDIA_TYPE;
		}
	}

	int err = bt_opp_server_push_rsp(&g_shell_session.server, rsp, NULL);

	if (err) {
		shell_error(sh, "push_rsp failed (err %d)", err);
		return err;
	}

	shell_print(sh, "OPP server push rsp sent (0x%02x)", rsp);
	return 0;
}

/* opp server_pull_bcard_rsp [success|reject|notfound] - respond to business card pull */
static int cmd_opp_server_pull_bcard_rsp(const struct shell *sh, size_t argc, char *argv[])
{
	static const char shell_vcard[] =
		"BEGIN:VCARD\r\nVERSION:2.1\r\nFN:Zephyr Shell Server\r\nEND:VCARD\r\n";

	if (g_shell_session.server.cb == NULL) {
		shell_error(sh, "No active server session");
		return -ENODEV;
	}

	uint8_t rsp_code = BT_OPP_RSP_CODE_SUCCESS;

	if (argc >= 2) {
		if (strcmp(argv[1], "reject") == 0) {
			rsp_code = BT_OPP_RSP_CODE_FORBIDDEN;
		} else if (strcmp(argv[1], "notfound") == 0) {
			rsp_code = BT_OPP_RSP_CODE_NOT_FOUND;
		}
	}

	int err;

	if (rsp_code == BT_OPP_RSP_CODE_SUCCESS) {
		/* Send the built-in shell vCard as the default business card. */
		size_t len = strlen(shell_vcard);
		struct net_buf *buf = bt_opp_server_create_pdu(&g_shell_session.server, NULL);

		if (buf == NULL) {
			shell_error(sh, "create PDU failed");
			return -ENOMEM;
		}

		(void)bt_obex_add_header_len(buf, (uint32_t)len);
		(void)bt_obex_add_header_end_body(buf, (uint16_t)len, (const uint8_t *)shell_vcard);
		err = bt_opp_server_pull_bcard_rsp(&g_shell_session.server, BT_OPP_RSP_CODE_SUCCESS,
						   buf);
	} else {
		err = bt_opp_server_pull_bcard_rsp(&g_shell_session.server, rsp_code, NULL);
	}

	if (err) {
		shell_error(sh, "pull_bcard_rsp failed (err %d)", err);
		return err;
	}

	shell_print(sh, "OPP server pull_bcard rsp sent (0x%02x)", rsp_code);
	return 0;
}

/* opp server_abort_rsp - respond to abort */
static int cmd_opp_server_abort_rsp(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	if (g_shell_session.server.cb == NULL) {
		shell_error(sh, "No active server session");
		return -ENODEV;
	}

	err = bt_opp_server_abort_rsp(&g_shell_session.server, BT_OPP_RSP_CODE_SUCCESS, NULL);
	if (err) {
		shell_error(sh, "abort_rsp failed (err %d)", err);
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
	SHELL_CMD_ARG(connect, NULL, "[mopl]  Establish OBEX session", cmd_opp_connect, 1, 1),
	SHELL_CMD_ARG(disconnect, NULL, "Terminate OBEX session", cmd_opp_disconnect, 1, 0),
	SHELL_CMD_ARG(push, NULL, "<vcard|vcal|vnote|vmsg>  Push a demo object to the server",
		      cmd_opp_push, 2, 0),
	SHELL_CMD_ARG(pull_bcard, NULL, "Pull the server's default business card",
		      cmd_opp_pull_bcard, 1, 0),
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
		      "[continue|success|reject|toobig|unsupported]  Send PUT response",
		      cmd_opp_server_push_rsp, 1, 1),
	SHELL_CMD_ARG(server_pull_bcard_rsp, NULL,
		      "[success|reject|notfound]  Send GET response for business card pull",
		      cmd_opp_server_pull_bcard_rsp, 1, 1),
	SHELL_CMD_ARG(server_abort_rsp, NULL, "Send ABORT response", cmd_opp_server_abort_rsp, 1,
		      0),
#endif /* CONFIG_BT_OPP_SERVER */
	SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(opp, &opp_cmds, "Bluetooth OPP shell commands", NULL);
