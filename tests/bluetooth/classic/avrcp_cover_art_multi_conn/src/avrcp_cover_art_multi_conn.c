/* avrcp_cover_art_multi_conn.c - Bluetooth classic AVRCP cover art multiple connections test */

/*
 * Copyright 2025 NXP
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

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/bluetooth/classic/avrcp.h>
#include <zephyr/bluetooth/classic/avrcp_cover_art.h>

#include <zephyr/shell/shell.h>

#include "host/shell/bt.h"
#include "common/bt_shell_private.h"

#define COVER_ART_MOPL CONFIG_BT_GOEP_L2CAP_MTU - 4

NET_BUF_POOL_FIXED_DEFINE(tx_pool, CONFIG_BT_MAX_CONN, BT_L2CAP_BUF_SIZE(COVER_ART_MOPL),
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

#if defined(CONFIG_BT_AVRCP_CONTROLLER)
static bool avrcp_ct_registered;
static struct bt_avrcp_ct *default_ct[CONFIG_BT_MAX_CONN];

uint8_t find_ct_index(struct bt_avrcp_ct *ct)
{
	for (uint8_t i = 0; i < ARRAY_SIZE(default_ct); i++) {
		if (default_ct[i] == ct) {
			return i;
		}
	}
	return UINT8_MAX;
}

static void avrcp_ct_connected(struct bt_conn *conn, struct bt_avrcp_ct *ct)
{
	uint8_t index;

	bt_shell_print("AVRCP CT connected");

	index = bt_conn_index(conn);
	if (index >= ARRAY_SIZE(default_ct)) {
		bt_shell_print("Conn index out of bounds");
		return;
	}

	default_ct[index] = ct;
}

static void avrcp_ct_disconnected(struct bt_avrcp_ct *ct)
{
	uint8_t index;

	bt_shell_print("AVRCP CT disconnected");

	index = find_ct_index(ct);
	if (index >= ARRAY_SIZE(default_ct)) {
		bt_shell_print("Conn index out of bounds");
		return;
	}

	default_ct[index] = NULL;
}

static struct bt_avrcp_ct_cb app_avrcp_ct_cb = {
	.connected = avrcp_ct_connected,
	.disconnected = avrcp_ct_disconnected,
};

static int register_ct_cb(const struct shell *sh)
{
	int err;

	if (avrcp_ct_registered) {
		return 0;
	}

	err = bt_avrcp_ct_register_cb(&app_avrcp_ct_cb);
	if (!err) {
		avrcp_ct_registered = true;
		shell_print(sh, "AVRCP CT callbacks registered");
	} else {
		shell_print(sh, "failed to register AVRCP CT callbacks");
	}

	return err;
}

static int cmd_register_ct_cb(const struct shell *sh, int32_t argc, char *argv[])
{
	if (avrcp_ct_registered) {
		shell_print(sh, "already registered");
		return 0;
	}

	register_ct_cb(sh);

	return 0;
}
#endif /* CONFIG_BT_AVRCP_CONTROLLER */

#if defined(CONFIG_BT_AVRCP_TARGET)
static bool avrcp_tg_registered;
static struct bt_avrcp_tg *default_tg[CONFIG_BT_MAX_CONN];

uint8_t find_tg_index(struct bt_avrcp_tg *tg)
{
	for (uint8_t i = 0; i < ARRAY_SIZE(default_tg); i++) {
		if (default_tg[i] == tg) {
			return i;
		}
	}
	return UINT8_MAX;
}

static void avrcp_tg_connected(struct bt_conn *conn, struct bt_avrcp_tg *tg)
{
	uint8_t index;

	bt_shell_print("AVRCP TG connected");

	index = bt_conn_index(conn);
	if (index >= ARRAY_SIZE(default_tg)) {
		bt_shell_print("Conn index out of bounds");
		return;
	}

	default_tg[index] = tg;
}

static void avrcp_tg_disconnected(struct bt_avrcp_tg *tg)
{
	uint8_t index;

	bt_shell_print("AVRCP TG disconnected");

	index = find_tg_index(tg);
	if (index >= ARRAY_SIZE(default_tg)) {
		bt_shell_print("Conn index out of bounds");
		return;
	}

	default_tg[index] = NULL;
}

static struct bt_avrcp_tg_cb app_avrcp_tg_cb = {
	.connected = avrcp_tg_connected,
	.disconnected = avrcp_tg_disconnected,
};

static int register_tg_cb(const struct shell *sh)
{
	int err;

	if (avrcp_tg_registered) {
		return 0;
	}

	err = bt_avrcp_tg_register_cb(&app_avrcp_tg_cb);
	if (!err) {
		avrcp_tg_registered = true;
		shell_print(sh, "AVRCP TG callbacks registered");
	} else {
		shell_print(sh, "failed to register AVRCP TG callbacks");
	}

	return err;
}

static int cmd_register_tg_cb(const struct shell *sh, int32_t argc, char *argv[])
{
	if (avrcp_tg_registered) {
		shell_print(sh, "already registered");
		return 0;
	}

	register_tg_cb(sh);

	return 0;
}
#endif /* CONFIG_BT_AVRCP_TARGET */

static int cmd_connect(const struct shell *sh, int32_t argc, char *argv[])
{
	int err;

	if (!avrcp_ct_registered && register_ct_cb(sh) != 0) {
		return -ENOEXEC;
	}

	if (!avrcp_tg_registered && register_tg_cb(sh) != 0) {
		return -ENOEXEC;
	}

	if (!default_conn) {
		shell_error(sh, "BR/EDR not connected");
		return -ENOEXEC;
	}

	err = bt_avrcp_connect(default_conn);
	if (err < 0) {
		shell_error(sh, "fail to connect AVRCP");
	}

	return 0;
}

static int cmd_disconnect(const struct shell *sh, int32_t argc, char *argv[])
{
	uint8_t index;
	bool index_valid;

	if ((!avrcp_ct_registered) && (!avrcp_tg_registered)) {
		shell_error(sh, "Neither CT nor TG callbacks are registered.");
		return -ENOEXEC;
	}

	if (!default_conn) {
		shell_print(sh, "Not connected");
		return -ENOEXEC;
	}

	index = bt_conn_index(default_conn);

	if (((index < ARRAY_SIZE(default_ct)) && (default_ct[index] != NULL)) ||
	    ((index < ARRAY_SIZE(default_tg)) && (default_tg[index] != NULL))) {
		bt_avrcp_disconnect(default_conn);
		return 0;
	} else {
		index_valid = index < ARRAY_SIZE(default_ct);
		index_valid = index_valid && (index < ARRAY_SIZE(default_tg));

		if (index_valid) {
			shell_error(sh, "AVRCP is not connected");
		} else {
			shell_error(sh, "Conn index out of bounds");
		}
		return -ENOEXEC;
	}
}

static bool cover_art_parse_tlvs_cb(struct bt_obex_tlv *tlv, void *user_data)
{
	bt_shell_print("T %02x L %d", tlv->type, tlv->data_len);
	return true;
}

static bool cover_art_parse_headers_cb(struct bt_obex_hdr *hdr, void *user_data)
{
	bt_shell_print("HI %02x Len %d", hdr->id, hdr->len);

	switch (hdr->id) {
	case BT_OBEX_HEADER_ID_APP_PARAM:
	case BT_OBEX_HEADER_ID_AUTH_CHALLENGE:
	case BT_OBEX_HEADER_ID_AUTH_RSP: {
		int err;

		err = bt_obex_tlv_parse(hdr->len, hdr->data, cover_art_parse_tlvs_cb, NULL);
		if (err != 0) {
			bt_shell_error("Fail to parse BIP TLV triplet (err %d)", err);
		}
	} break;
	default:
		break;
	}

	return true;
}

static int cover_art_parse_headers(struct net_buf *buf)
{
	int err;

	if (buf == NULL) {
		return 0;
	}

	err = bt_obex_header_parse(buf, cover_art_parse_headers_cb, NULL);
	if (err != 0) {
		bt_shell_print("Fail to parse BIP Headers (err %d)", err);
	}

	return err;
}

#if defined(CONFIG_BT_AVRCP_TG_COVER_ART)
static struct bt_avrcp_cover_art_tg *ca_tg[CONFIG_BT_MAX_CONN];
static uint16_t ca_tg_peer_mopl[CONFIG_BT_MAX_CONN];
static uint16_t ca_tg_offset[CONFIG_BT_MAX_CONN];

uint8_t find_ca_tg_index(struct bt_avrcp_cover_art_tg *cover_art_tg)
{
	for (uint8_t i = 0; i < ARRAY_SIZE(ca_tg); i++) {
		if (ca_tg[i] == cover_art_tg) {
			return i;
		}
	}
	return UINT8_MAX;
}

static void tg_l2cap_connected(struct bt_avrcp_tg *tg, struct bt_avrcp_cover_art_tg *cover_art_tg)
{
	uint8_t index;

	bt_shell_print("Cover Art TG L2CAP Connected: %p", cover_art_tg);

	index = find_tg_index(tg);
	if (index >= ARRAY_SIZE(ca_tg)) {
		bt_shell_print("Conn index out of bounds");
		return;
	}
	ca_tg[index] = cover_art_tg;
}

static void tg_l2cap_disconnected(struct bt_avrcp_cover_art_tg *tg)
{
	uint8_t index;

	bt_shell_print("Cover Art TG L2CAP Disconnected: %p", tg);

	index = find_ca_tg_index(tg);
	if (index >= ARRAY_SIZE(ca_tg)) {
		bt_shell_print("Conn index out of bounds");
		return;
	}
	ca_tg[index] = NULL;
}

static void tg_connect(struct bt_avrcp_cover_art_tg *tg, uint8_t version, uint16_t mopl,
		       struct net_buf *buf)
{
	uint8_t index;
	bt_shell_print("Cover Art TG %p OBEX connect req: version %02x, mopl %04x", tg, version,
		       mopl);

	index = find_ca_tg_index(tg);
	if (index >= ARRAY_SIZE(ca_tg)) {
		bt_shell_print("Conn index out of bounds");
		return;
	}
	ca_tg_peer_mopl[index] = mopl;
	cover_art_parse_headers(buf);
}

static void tg_disconnect(struct bt_avrcp_cover_art_tg *tg, struct net_buf *buf)
{
	bt_shell_print("Cover Art TG %p disconn req", tg);
	cover_art_parse_headers(buf);
}

static void tg_get_image_properties(struct bt_avrcp_cover_art_tg *tg, bool final,
				    struct net_buf *buf)
{
	bt_shell_print("Cover Art TG %p get_image_properties req, final %s, data len %u", tg,
		       final ? "true" : "false", buf->len);
	cover_art_parse_headers(buf);
}

static struct bt_avrcp_cover_art_tg_cb cover_art_tg_cb = {
	.l2cap_connected = tg_l2cap_connected,
	.l2cap_disconnected = tg_l2cap_disconnected,
	.connect = tg_connect,
	.disconnect = tg_disconnect,
	.abort = NULL,
	.get_image_properties = tg_get_image_properties,
	.get_image = NULL,
	.get_linked_thumbnail = NULL,
};

static int cmd_tg_reg(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	err = bt_avrcp_cover_art_tg_cb_register(&cover_art_tg_cb);
	if (err != 0) {
		shell_error(sh, "Failed to reg cover art TG cb (err %d)", err);
	}

	return err;
}

static int cmd_tg_l2cap_disconnect(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t index;
	int err;

	index = bt_conn_index(default_conn);
	if (index >= ARRAY_SIZE(ca_tg)) {
		shell_error(sh, "Conn index out of bounds");
		return -ENOEXEC;
	}

	err = bt_avrcp_cover_art_tg_l2cap_disconnect(ca_tg[index]);
	if (err != 0) {
		shell_error(sh, "L2CAP disconnect failed (err %d)", err);
	}

	return err;
}

static int parse_rsp_code(const struct shell *sh, size_t argc, char *argv[], uint8_t *rsp_code)
{
	const char *rsp;

	rsp = argv[1];
	if (!strcmp(rsp, "continue")) {
		*rsp_code = BT_OBEX_RSP_CODE_CONTINUE;
	} else if (!strcmp(rsp, "success")) {
		*rsp_code = BT_OBEX_RSP_CODE_SUCCESS;
	} else if (!strcmp(rsp, "error")) {
		if (argc < 3) {
			shell_error(sh, "[rsp_code] is needed if the rsp is %s", rsp);
			shell_help(sh);
			return SHELL_CMD_HELP_PRINTED;
		}
		*rsp_code = (uint8_t)strtoul(argv[2], NULL, 16);
	} else {
		shell_help(sh);
		return SHELL_CMD_HELP_PRINTED;
	}

	return 0;
}

static int cmd_tg_connect(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t index;
	uint8_t rsp_code;
	int err;

	err = parse_rsp_code(sh, argc, argv, &rsp_code);
	if (err != 0) {
		return err;
	}

	index = bt_conn_index(default_conn);
	if (index >= ARRAY_SIZE(ca_tg)) {
		shell_error(sh, "Conn index out of bounds");
		return -ENOEXEC;
	}

	err = bt_avrcp_cover_art_tg_connect(ca_tg[index], rsp_code);
	if (err != 0) {
		shell_error(sh, "Cover art TG connect rsp failed (err %d)", err);
	}

	return err;
}

static int cmd_tg_disconnect(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t index;
	uint8_t rsp_code;
	int err;

	err = parse_rsp_code(sh, argc, argv, &rsp_code);
	if (err != 0) {
		return err;
	}

	index = bt_conn_index(default_conn);
	if (index >= ARRAY_SIZE(ca_tg)) {
		shell_error(sh, "Conn index out of bounds");
		return -ENOEXEC;
	}

	err = bt_avrcp_cover_art_tg_disconnect(ca_tg[index], rsp_code);
	if (err != 0) {
		shell_error(sh, "Cover art TG disconnect rsp failed (err %d)", err);
	}

	return err;
}

#define IMAGE_PROPERTIES_BODY                                                                      \
	"<image-properties version=\"1.0\" handle=\"1000001\">"                                    \
	"<native encoding=\"JPEG\" pixel=\"1280*1024\" size=\"1048576\"/>"                         \
	"<variant encoding=\"JPEG\" pixel=\"640*480\" />"                                          \
	"<variant encoding=\"JPEG\" pixel=\"160*120\" />"                                          \
	"<variant encoding=\"GIF\" pixel=\"80*60-640*480\"/>"                                      \
	"<attachment content-type=\"text/plain\" name=\"ABCD0001.txt\" size=\"5120\"/>"            \
	"<attachment content-type=\"audio/basic\" name=\"ABCD0001.wav\" size=\"102400\"/>"         \
	"</image-properties>"

static int cmd_tg_get_image_properties(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t index;
	uint8_t rsp_code;
	const char *rsp;
	int err;
	uint16_t len = 0;
	struct net_buf *buf = NULL;
	const uint8_t *image_properties_body = (const uint8_t *)IMAGE_PROPERTIES_BODY;

	index = bt_conn_index(default_conn);
	if (index >= ARRAY_SIZE(ca_tg)) {
		shell_error(sh, "Conn index out of bounds");
		return -ENOEXEC;
	}

	if (ca_tg[index] == NULL) {
		shell_error(sh, "Invalid Cover Art Target");
		return -ENOEXEC;
	}

	rsp = argv[1];
	if (!strcmp(rsp, "error")) {
		if (argc < 3) {
			shell_error(sh, "[rsp_code] is needed if the rsp is %s", rsp);
			shell_help(sh);
			return SHELL_CMD_HELP_PRINTED;
		}
		rsp_code = (uint8_t)strtoul(argv[2], NULL, 16);

		goto error_rsp;
	}

	buf = bt_avrcp_cover_art_tg_create_pdu(ca_tg[index], &tx_pool);
	if (buf == NULL) {
		shell_error(sh, "Fail to allocate tx buffer");
		return -ENOBUFS;
	}

	err = bt_obex_add_header_body_or_end_body(
		buf, ca_tg_peer_mopl[index], sizeof(IMAGE_PROPERTIES_BODY) - ca_tg_offset[index],
		image_properties_body + ca_tg_offset[index], &len);
	if (err != 0) {
		shell_error(sh, "Fail to add body");
	}

	if (bt_obex_has_header(buf, BT_OBEX_HEADER_ID_END_BODY)) {
		rsp_code = BT_OBEX_RSP_CODE_SUCCESS;
	} else {
		rsp_code = BT_OBEX_RSP_CODE_CONTINUE;
	}

error_rsp:
	err = bt_avrcp_cover_art_tg_get_image_properties(ca_tg[index], rsp_code, buf);
	if (err != 0) {
		if (buf != NULL) {
			net_buf_unref(buf);
		}
		shell_error(sh, "Fail to send image properties rsp %d", err);
	} else {
		if (rsp_code == BT_OBEX_RSP_CODE_CONTINUE) {
			ca_tg_offset[index] += len;
		} else {
			ca_tg_offset[index] = 0;
		}
	}
	return err;
}

#endif /* CONFIG_BT_AVRCP_TG_COVER_ART */

#if defined(CONFIG_BT_AVRCP_CT_COVER_ART)
static struct bt_avrcp_cover_art_ct *ca_ct[CONFIG_BT_MAX_CONN];
static uint32_t ca_ct_conn_id[CONFIG_BT_MAX_CONN];
static uint8_t ca_ct_rsp_code[CONFIG_BT_MAX_CONN];

uint8_t find_ca_ct_index(struct bt_avrcp_cover_art_ct *cover_art_ct)
{
	for (uint8_t i = 0; i < ARRAY_SIZE(ca_ct); i++) {
		if (ca_ct[i] == cover_art_ct) {
			return i;
		}
	}
	return UINT8_MAX;
}

static void ct_l2cap_connected(struct bt_avrcp_ct *ct, struct bt_avrcp_cover_art_ct *cover_art_ct)
{
	uint8_t index;

	bt_shell_print("Cover Art CT L2CAP Connected: %p", cover_art_ct);

	index = find_ct_index(ct);
	if (index >= ARRAY_SIZE(ca_ct)) {
		bt_shell_error("Conn index out of bounds");
	}
}

static void ct_l2cap_disconnected(struct bt_avrcp_cover_art_ct *ct)
{
	uint8_t index;

	bt_shell_print("Cover Art CT L2CAP Disconnected: %p", ct);

	index = find_ca_ct_index(ct);
	if (index >= ARRAY_SIZE(ca_ct)) {
		bt_shell_error("Conn index out of bounds");
		return;
	}

	ca_ct[index] = NULL;
	ca_ct_rsp_code[index] = BT_OBEX_RSP_CODE_SUCCESS;
}

static void ct_connect(struct bt_avrcp_cover_art_ct *ct, uint8_t rsp_code, uint8_t version,
		       uint16_t mopl, struct net_buf *buf)
{
	uint8_t index;
	int err;

	bt_shell_print("Cover Art CT %p conn rsp, rsp_code %s, version %02x, mopl %04x", ct,
		       bt_obex_rsp_code_to_str(rsp_code), version, mopl);
	cover_art_parse_headers(buf);

	index = find_ca_ct_index(ct);
	if (index >= ARRAY_SIZE(ca_ct)) {
		bt_shell_error("Conn index out of bounds");
		return;
	}

	err = bt_obex_get_header_conn_id(buf, &ca_ct_conn_id[index]);
	if (err != 0) {
		bt_shell_error("Failed to get connection id");
	} else {
		bt_shell_print("Connection id %u", ca_ct_conn_id[index]);
	}
}

static void ct_disconnect(struct bt_avrcp_cover_art_ct *ct, uint8_t rsp_code, struct net_buf *buf)
{
	bt_shell_print("Cover Art CT %p disconn rsp, rsp_code %s", ct,
		       bt_obex_rsp_code_to_str(rsp_code));
	cover_art_parse_headers(buf);
}

static void ct_get_image_properties(struct bt_avrcp_cover_art_ct *ct, uint8_t rsp_code,
				    struct net_buf *buf)
{
	uint8_t index;

	bt_shell_print("Cover Art CT %p get_image_properties rsp, rsp_code %s, data len %u", ct,
		       bt_obex_rsp_code_to_str(rsp_code), buf->len);
	cover_art_parse_headers(buf);

	index = find_ca_ct_index(ct);
	if (index >= ARRAY_SIZE(ca_ct)) {
		bt_shell_error("Conn index out of bounds");
		return;
	}

	ca_ct_rsp_code[index] = rsp_code;
}

static struct bt_avrcp_cover_art_ct_cb cover_art_ct_cb = {
	.l2cap_connected = ct_l2cap_connected,
	.l2cap_disconnected = ct_l2cap_disconnected,
	.connect = ct_connect,
	.disconnect = ct_disconnect,
	.abort = NULL,
	.get_image_properties = ct_get_image_properties,
	.get_image = NULL,
	.get_linked_thumbnail = NULL,
};

static int cmd_ct_reg(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	err = bt_avrcp_cover_art_ct_cb_register(&cover_art_ct_cb);
	if (err != 0) {
		shell_error(sh, "Failed to reg cover art CT cb (err %d)", err);
	}

	return err;
}

static int cmd_ct_l2cap_connect(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t index;
	uint16_t psm;
	int err = 0;

	psm = shell_strtoul(argv[1], 16, &err);
	if (err != 0) {
		shell_error(sh, "Invalid PSM %s", argv[1]);
		return -ENOEXEC;
	}

	index = bt_conn_index(default_conn);
	if (index >= ARRAY_SIZE(default_ct)) {
		shell_error(sh, "Conn index out of bounds");
		return -ENOEXEC;
	}

	if (default_ct[index] == NULL) {
		shell_error(sh, "Invalid CT");
		return -ENOEXEC;
	}

	err = bt_avrcp_cover_art_ct_l2cap_connect(default_ct[index], &ca_ct[index], psm);
	if (err != 0) {
		shell_error(sh, "L2CAP connect failed (err %d)", err);
	}

	return err;
}

static int cmd_ct_l2cap_disconnect(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t index;
	int err;

	index = bt_conn_index(default_conn);
	if (index >= ARRAY_SIZE(ca_ct)) {
		shell_error(sh, "Conn index out of bounds");
		return -ENOEXEC;
	}

	err = bt_avrcp_cover_art_ct_l2cap_disconnect(ca_ct[index]);
	if (err != 0) {
		shell_error(sh, "L2CAP disconnect failed (err %d)", err);
	}

	return err;
}

static int cmd_ct_connect(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t index;
	int err;

	index = bt_conn_index(default_conn);
	if (index >= ARRAY_SIZE(ca_ct)) {
		shell_error(sh, "Conn index out of bounds");
		return -ENOEXEC;
	}

	err = bt_avrcp_cover_art_ct_connect(ca_ct[index]);
	if (err != 0) {
		shell_error(sh, "OBEX connect failed (err %d)", err);
	}

	return err;
}

static int cmd_ct_disconnect(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t index;
	int err;

	index = bt_conn_index(default_conn);
	if (index >= ARRAY_SIZE(ca_ct)) {
		shell_error(sh, "Conn index out of bounds");
		return -ENOEXEC;
	}

	err = bt_avrcp_cover_art_ct_disconnect(ca_ct[index]);
	if (err != 0) {
		shell_error(sh, "OBEX disconnect failed (err %d)", err);
	}

	return err;
}

#define IMAGE_HANDLE "\x00\x31\x00\x30\x00\x30\x00\x30\x00\x30\x00\x30\x00\x31\x00\x00"

#if defined(CONFIG_BT_AVRCP_CT_COVER_ART_GET_IMAGE_PROPERTIES)
static int cmd_ct_get_image_properties(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t index;
	struct net_buf *buf;
	int err;

	index = bt_conn_index(default_conn);
	if (index >= ARRAY_SIZE(ca_ct)) {
		shell_error(sh, "Conn index out of bounds");
		return -ENOEXEC;
	}

	if (ca_ct[index] == NULL) {
		shell_error(sh, "Invalid Cover Art CT");
		return -ENOEXEC;
	}

	buf = bt_avrcp_cover_art_ct_create_pdu(ca_ct[index], &tx_pool);
	if (buf == NULL) {
		shell_error(sh, "Fail to allocate tx buffer");
		return -ENOBUFS;
	}

	if (ca_ct_rsp_code[index] == BT_OBEX_RSP_CODE_CONTINUE) {
		goto continue_req;
	}

	err = bt_obex_add_header_conn_id(buf, ca_ct[index]->client._conn_id);
	if (err != 0) {
		shell_error(sh, "Fail to add conn id header %d", err);
		goto failed;
	}

	err = bt_obex_add_header_type(buf, sizeof(BT_BIP_HDR_TYPE_GET_IMAGE_PROPERTIES),
				      (uint8_t *)BT_BIP_HDR_TYPE_GET_IMAGE_PROPERTIES);
	if (err != 0) {
		shell_error(sh, "Fail to add type header %d", err);
		goto failed;
	}

	err = bt_bip_add_header_image_handle(buf, sizeof(IMAGE_HANDLE) - 1, (uint8_t *)IMAGE_HANDLE);
	if (err != 0) {
		shell_error(sh, "Fail to add image handle header %d", err);
		goto failed;
	}

continue_req:
	err = bt_avrcp_cover_art_ct_get_image_properties(ca_ct[index], true, buf);
	if (err == 0) {
		return 0;
	}

	shell_error(sh, "Fail to send get_image_properties req %d", err);

failed:
	net_buf_unref(buf);
	return err;
}
#endif /* CONFIG_BT_AVRCP_CT_COVER_ART_GET_IMAGE_PROPERTIES */
#endif /* CONFIG_BT_AVRCP_CT_COVER_ART */

#define HELP_NONE "[none]"

#if defined(CONFIG_BT_AVRCP_CONTROLLER)
SHELL_STATIC_SUBCMD_SET_CREATE(avrcp_ct_cmds,
			       SHELL_CMD_ARG(register_cb, NULL, "register avrcp ct callbacks",
					     cmd_register_ct_cb, 1, 0),
			       SHELL_SUBCMD_SET_END);
#endif /* CONFIG_BT_AVRCP_CONTROLLER */

#if defined(CONFIG_BT_AVRCP_TARGET)
SHELL_STATIC_SUBCMD_SET_CREATE(avrcp_tg_cmds,
			       SHELL_CMD_ARG(register_cb, NULL, "register avrcp tg callbacks",
					     cmd_register_tg_cb, 1, 0),
			       SHELL_SUBCMD_SET_END);
#endif /* CONFIG_BT_AVRCP_TARGET */

static int cmd_avrcp(const struct shell *sh, size_t argc, char **argv)
{
	if (argc == 1) {
		shell_help(sh);
		return SHELL_CMD_HELP_PRINTED;
	}

	shell_error(sh, "%s unknown parameter: %s", argv[0], argv[1]);

	return -ENOEXEC;
}

SHELL_STATIC_SUBCMD_SET_CREATE(test_avrcp_cmds,
			       SHELL_CMD_ARG(connect, NULL, "connect AVRCP", cmd_connect, 1, 0),
			       SHELL_CMD_ARG(disconnect, NULL, "disconnect AVRCP", cmd_disconnect,
					     1, 0),
#if defined(CONFIG_BT_AVRCP_CONTROLLER)
			       SHELL_CMD(ct, &avrcp_ct_cmds, "AVRCP CT shell commands", cmd_avrcp),
#endif /* CONFIG_BT_AVRCP_CONTROLLER */
#if defined(CONFIG_BT_AVRCP_TARGET)
			       SHELL_CMD(tg, &avrcp_tg_cmds, "AVRCP TG shell commands", cmd_avrcp),
#endif /* CONFIG_BT_AVRCP_TARGET */
			       SHELL_SUBCMD_SET_END);

SHELL_CMD_ARG_REGISTER(test_avrcp, &test_avrcp_cmds, "Bluetooth AVRCP shell commands", cmd_avrcp, 1,
		       1);

#if defined(CONFIG_BT_AVRCP_TG_COVER_ART)
#define RSP_HELP_1 "<rsp: continue, success, error> [rsp_code]"
#define RSP_HELP_2 "<rsp: noerror, error> [rsp_code]"

SHELL_STATIC_SUBCMD_SET_CREATE(tg_cmds, SHELL_CMD_ARG(reg, NULL, HELP_NONE, cmd_tg_reg, 1, 0),
			       SHELL_CMD_ARG(l2cap_disconnect, NULL, HELP_NONE,
					     cmd_tg_l2cap_disconnect, 1, 0),
			       SHELL_CMD_ARG(connect, NULL, RSP_HELP_1, cmd_tg_connect, 2, 1),
			       SHELL_CMD_ARG(disconnect, NULL, RSP_HELP_1, cmd_tg_disconnect, 2, 1),
			       SHELL_CMD_ARG(get_image_properties, NULL, RSP_HELP_2,
					     cmd_tg_get_image_properties, 2, 1),
			       SHELL_SUBCMD_SET_END);
#endif /* CONFIG_BT_AVRCP_TG_COVER_ART */

#if defined(CONFIG_BT_AVRCP_CT_COVER_ART)
SHELL_STATIC_SUBCMD_SET_CREATE(
	ct_cmds, SHELL_CMD_ARG(reg, NULL, HELP_NONE, cmd_ct_reg, 1, 0),
	SHELL_CMD_ARG(l2cap_connect, NULL, "<psm>", cmd_ct_l2cap_connect, 2, 0),
	SHELL_CMD_ARG(l2cap_disconnect, NULL, HELP_NONE, cmd_ct_l2cap_disconnect, 1, 0),
	SHELL_CMD_ARG(connect, NULL, HELP_NONE, cmd_ct_connect, 1, 0),
	SHELL_CMD_ARG(disconnect, NULL, HELP_NONE, cmd_ct_disconnect, 1, 0),
#if defined(CONFIG_BT_AVRCP_CT_COVER_ART_GET_IMAGE_PROPERTIES)
	SHELL_CMD_ARG(get_image_properties, NULL, HELP_NONE, cmd_ct_get_image_properties, 1, 0),
#endif /* CONFIG_BT_AVRCP_CT_COVER_ART_GET_IMAGE_PROPERTIES */
	SHELL_SUBCMD_SET_END);
#endif /* CONFIG_BT_AVRCP_CT_COVER_ART */

static int cmd_avrcp_cover_art(const struct shell *sh, size_t argc, char **argv)
{
	if (argc == 1) {
		shell_help(sh);
		return SHELL_CMD_HELP_PRINTED;
	}

	shell_error(sh, "%s unknown parameter: %s", argv[0], argv[1]);

	return -ENOEXEC;
}

SHELL_STATIC_SUBCMD_SET_CREATE(test_avrcp_cover_art_cmds,
#if defined(CONFIG_BT_AVRCP_CT_COVER_ART)
			       SHELL_CMD(ct, &ct_cmds, "AVRCP Cover Art CT shell commands",
					 cmd_avrcp_cover_art),
#endif /* CONFIG_BT_AVRCP_CT_COVER_ART*/
#if defined(CONFIG_BT_AVRCP_TG_COVER_ART)
			       SHELL_CMD(tg, &tg_cmds, "AVRCP Cover Art TG shell commands",
					 cmd_avrcp_cover_art),
#endif /* CONFIG_BT_AVRCP_TG_COVER_ART */
			       SHELL_SUBCMD_SET_END);

SHELL_CMD_ARG_REGISTER(test_avrcp_cover_art, &test_avrcp_cover_art_cmds,
		       "Bluetooth AVRCP Cover Art shell commands", cmd_avrcp_cover_art, 1, 1);
