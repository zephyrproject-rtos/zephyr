/** @file
 *  @brief Audio Video Remote Control Cover Art Profile shell functions.
 */

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

#define COVER_ART_MOPL CONFIG_BT_GOEP_L2CAP_MTU

NET_BUF_POOL_FIXED_DEFINE(tx_pool, CONFIG_BT_MAX_CONN, BT_L2CAP_BUF_SIZE(COVER_ART_MOPL),
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

static bool cover_art_parse_tlvs_cb(struct bt_obex_tlv *tlv, void *user_data)
{
	bt_shell_print("T %02x L %d", tlv->type, tlv->data_len);
	bt_shell_hexdump(tlv->data, tlv->data_len);

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
		bt_shell_hexdump(hdr->data, hdr->len);
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
static struct bt_avrcp_cover_art_tg *ca_tg;
static uint16_t ca_tg_peer_mopl;

static void tg_l2cap_connected(struct bt_avrcp_tg *tg, struct bt_avrcp_cover_art_tg *cover_art_tg)
{
	ca_tg = cover_art_tg;
	bt_shell_print("Cover Art TG L2CAP Connected: %p", cover_art_tg);
}

static void tg_l2cap_disconnected(struct bt_avrcp_cover_art_tg *tg)
{
	ca_tg = NULL;
	bt_shell_print("Cover Art TG L2CAP Disconnected: %p", tg);
}

static void tg_connect(struct bt_avrcp_cover_art_tg *tg, uint8_t version, uint16_t mopl,
		       struct net_buf *buf)
{
	bt_shell_print("Cover Art TG %p OBEX connect req: version %u, mopl %u", tg, version, mopl);
	ca_tg_peer_mopl = mopl;
	cover_art_parse_headers(buf);
}

static void tg_disconnect(struct bt_avrcp_cover_art_tg *tg, struct net_buf *buf)
{
	bt_shell_print("Cover Art TG %p disconn req", tg);
	cover_art_parse_headers(buf);
}

static void tg_abort(struct bt_avrcp_cover_art_tg *tg, struct net_buf *buf)
{
	bt_shell_print("Cover Art TG %p abort req", tg);
	cover_art_parse_headers(buf);
}

static void tg_get_image_properties(struct bt_avrcp_cover_art_tg *tg, bool final,
				    struct net_buf *buf)
{
	bt_shell_print("Cover Art TG %p get_image_properties req, final %s, data len %u", tg,
		       final ? "true" : "false", buf->len);
	cover_art_parse_headers(buf);
}

static void tg_get_image(struct bt_avrcp_cover_art_tg *tg, bool final, struct net_buf *buf)
{
	bt_shell_print("Cover Art TG %p get_image req, final %s, data len %u", tg,
		       final ? "true" : "false", buf->len);
	cover_art_parse_headers(buf);
}

static void tg_get_linked_thumbnail(struct bt_avrcp_cover_art_tg *tg, bool final,
				    struct net_buf *buf)
{
	bt_shell_print("Cover Art TG %p get_linked_thumbnail req, final %s, data len %u", tg,
		       final ? "true" : "false", buf->len);
	cover_art_parse_headers(buf);
}

struct bt_avrcp_cover_art_tg_cb cover_art_tg_cb = {
	.l2cap_connected = tg_l2cap_connected,
	.l2cap_disconnected = tg_l2cap_disconnected,
	.connect = tg_connect,
	.disconnect = tg_disconnect,
	.abort = tg_abort,
	.get_image_properties = tg_get_image_properties,
	.get_image = tg_get_image,
	.get_linked_thumbnail = tg_get_linked_thumbnail,
};

static int cmd_tg_reg(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	err = bt_avrcp_cover_art_tg_cb_register(&cover_art_tg_cb);
	if (err != 0) {
		shell_error(sh, "Failed to reg cover art cb (err %d)", err);
	}

	return err;
}

static int cmd_tg_l2cap_disconnect(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	err = bt_avrcp_cover_art_tg_l2cap_disconnect(ca_tg);
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
	uint8_t rsp_code;
	int err;

	err = parse_rsp_code(sh, argc, argv, &rsp_code);
	if (err != 0) {
		return err;
	}

	err = bt_avrcp_cover_art_tg_connect(ca_tg, rsp_code);
	if (err != 0) {
		shell_error(sh, "Cover art TG connect rsp failed (err %d)", err);
	}

	return err;
}

static int cmd_tg_disconnect(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t rsp_code;
	int err;

	err = parse_rsp_code(sh, argc, argv, &rsp_code);
	if (err != 0) {
		return err;
	}

	err = bt_avrcp_cover_art_tg_disconnect(ca_tg, rsp_code);
	if (err != 0) {
		shell_error(sh, "Cover art TG disconnect rsp failed (err %d)", err);
	}

	return err;
}

static int cmd_tg_abort(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t rsp_code;
	int err;

	err = parse_rsp_code(sh, argc, argv, &rsp_code);
	if (err != 0) {
		return err;
	}

	err = bt_avrcp_cover_art_tg_abort(ca_tg, rsp_code, NULL);
	if (err != 0) {
		shell_error(sh, "Cover art TG abort rsp failed (err %d)", err);
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
	uint8_t rsp_code;
	const char *rsp;
	int err;
	uint16_t len = 0;
	struct net_buf *buf = NULL;
	const uint8_t *image_properties_body = (const uint8_t *)IMAGE_PROPERTIES_BODY;

	static uint16_t offset;

	if (ca_tg == NULL) {
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

	buf = bt_avrcp_cover_art_tg_create_pdu(ca_tg, &tx_pool);
	if (buf == NULL) {
		shell_error(sh, "Fail to allocate tx buffer");
		return -ENOBUFS;
	}

	err = bt_obex_add_header_body_or_end_body(buf, ca_tg_peer_mopl,
						  sizeof(IMAGE_PROPERTIES_BODY) - offset,
						  image_properties_body + offset, &len);
	if (err != 0) {
		shell_error(sh, "Fail to add body");
	}

	if (bt_obex_has_header(buf, BT_OBEX_HEADER_ID_END_BODY)) {
		rsp_code = BT_OBEX_RSP_CODE_SUCCESS;
	} else {
		rsp_code = BT_OBEX_RSP_CODE_CONTINUE;
	}

error_rsp:
	err = bt_avrcp_cover_art_tg_get_image_properties(ca_tg, rsp_code, buf);
	if (err != 0) {
		if (buf != NULL) {
			net_buf_unref(buf);
		}
		shell_error(sh, "Fail to send image properties rsp %d", err);
	} else {
		if (rsp_code == BT_OBEX_RSP_CODE_CONTINUE) {
			offset += len;
		} else {
			offset = 0;
		}
	}
	return err;
}

/* Minimal 1x1 pixel white JPEG image data */
static const uint8_t jpeg_1000001[] = {
	/* SOI (Start of Image) */
	0xFF, 0xD8,

	/* APP0 segment */
	0xFF, 0xE0,                   /* APP0 marker */
	0x00, 0x10,                   /* Length (16 bytes) */
	0x4A, 0x46, 0x49, 0x46, 0x00, /* "JFIF\0" */
	0x01, 0x01,                   /* Version 1.1 */
	0x01,                         /* Density units (1 = pixels per inch) */
	0x00, 0x48,                   /* X density (72) */
	0x00, 0x48,                   /* Y density (72) */
	0x00, 0x00,                   /* Thumbnail width and height (0,0 = no thumbnail) */

	/* DQT (Define Quantization Table) - Luminance quantization table */
	0xFF, 0xDB, /* DQT marker */
	0x00, 0x43, /* Length (67 bytes) */
	0x00,       /* Table ID (0) and precision (8bit) */
	/* 8x8 quantization table (simplified version) */
	0x10, 0x0B, 0x0C, 0x0E, 0x0C, 0x0A, 0x10, 0x0E, 0x0D, 0x0E, 0x12, 0x11, 0x10, 0x13, 0x18,
	0x28, 0x1A, 0x18, 0x16, 0x16, 0x18, 0x31, 0x23, 0x25, 0x1D, 0x28, 0x3A, 0x33, 0x3D, 0x3C,
	0x39, 0x33, 0x38, 0x37, 0x40, 0x48, 0x5C, 0x4E, 0x40, 0x44, 0x57, 0x45, 0x37, 0x38, 0x50,
	0x6D, 0x51, 0x57, 0x5F, 0x62, 0x67, 0x68, 0x67, 0x3E, 0x4D, 0x71, 0x79, 0x70, 0x64, 0x78,
	0x5C, 0x65, 0x67, 0x63,

	/* SOF0 (Start of Frame - Baseline DCT) */
	0xFF, 0xC0, /* SOF0 marker */
	0x00, 0x11, /* Length (17 bytes) */
	0x08,       /* Data precision (8 bits) */
	0x00, 0x01, /* Image height (1) */
	0x00, 0x01, /* Image width (1) */
	0x01,       /* Number of color components (1 = grayscale) */
	0x01,       /* Component ID (1) */
	0x11,       /* Sampling factors (1x1) */
	0x00,       /* Quantization table selector (0) */

	/* DHT (Define Huffman Table) - DC table */
	0xFF, 0xC4, /* DHT marker */
	0x00, 0x1F, /* Length (31 bytes) */
	0x00,       /* Table class (0=DC) and table ID (0) */
	/* Symbol length counts (16 bytes) */
	0x00, 0x01, 0x05, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00,
	/* Symbol values (12 bytes) */
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B,

	/* DHT (Define Huffman Table) - AC table */
	0xFF, 0xC4, /* DHT marker */
	0x00, 0xB5, /* Length (181 bytes) */
	0x10,       /* Table class (1=AC) and table ID (0) */
	/* AC Huffman table data (simplified version) */
	0x00, 0x02, 0x01, 0x03, 0x03, 0x02, 0x04, 0x03, 0x05, 0x05, 0x04, 0x04, 0x00, 0x00, 0x01,
	0x7D, 0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12, 0x21, 0x31, 0x41, 0x06, 0x13, 0x51,
	0x61, 0x07, 0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xA1, 0x08, 0x23, 0x42, 0xB1, 0xC1, 0x15,
	0x52, 0xD1, 0xF0, 0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0A, 0x16, 0x17, 0x18, 0x19, 0x1A,
	0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x43, 0x44,
	0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x63,
	0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A,
	0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98,
	0x99, 0x9A, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xB2, 0xB3, 0xB4, 0xB5,
	0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xD2,
	0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7,
	0xE8, 0xE9, 0xEA, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA,

	/* SOS (Start of Scan) */
	0xFF, 0xDA, /* SOS marker */
	0x00, 0x08, /* Length (8 bytes) */
	0x01,       /* Number of components (1) */
	0x01,       /* Component ID (1) */
	0x00,       /* DC and AC Huffman table selectors */
	0x00,       /* Spectral selection start */
	0x3F,       /* Spectral selection end */
	0x00,       /* Successive approximation bit position */

	/* Image data (encoded data for 1 white pixel) */
	0xFF, 0x00, /* Stuffed DC coefficient (white) */

	/* EOI (End of Image) */
	0xFF, 0xD9};

static int cmd_tg_get_image(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t rsp_code;
	const char *rsp;
	int err;
	uint16_t len = 0;
	struct net_buf *buf = NULL;

	static uint16_t offset;

	if (ca_tg == NULL) {
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

	buf = bt_avrcp_cover_art_tg_create_pdu(ca_tg, &tx_pool);
	if (buf == NULL) {
		shell_error(sh, "Fail to allocate tx buffer");
		return -ENOBUFS;
	}

	if (offset == 0) {
		err = bt_obex_add_header_len(buf, sizeof(jpeg_1000001));
		if (err != 0) {
			shell_error(sh, "Fail to add len header %d", err);
			net_buf_unref(buf);
			return err;
		}
	}

	err = bt_obex_add_header_body_or_end_body(buf, ca_tg_peer_mopl,
						  sizeof(jpeg_1000001) - offset,
						  jpeg_1000001 + offset, &len);
	if (err != 0) {
		shell_error(sh, "Fail to add body");
	}

	if (bt_obex_has_header(buf, BT_OBEX_HEADER_ID_END_BODY)) {
		rsp_code = BT_OBEX_RSP_CODE_SUCCESS;
	} else {
		rsp_code = BT_OBEX_RSP_CODE_CONTINUE;
	}

error_rsp:
	err = bt_avrcp_cover_art_tg_get_image(ca_tg, rsp_code, buf);
	if (err != 0) {
		if (buf != NULL) {
			net_buf_unref(buf);
		}
		shell_error(sh, "Fail to send get image rsp %d", err);
	} else {
		if (rsp_code == BT_OBEX_RSP_CODE_CONTINUE) {
			offset += len;
		} else {
			offset = 0;
		}
	}
	return err;
}

static int cmd_tg_get_linked_thumbnail(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t rsp_code;
	const char *rsp;
	int err;
	uint16_t len = 0;
	struct net_buf *buf = NULL;

	static uint16_t offset;

	if (ca_tg == NULL) {
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

	buf = bt_avrcp_cover_art_tg_create_pdu(ca_tg, &tx_pool);
	if (buf == NULL) {
		shell_error(sh, "Fail to allocate tx buffer");
		return -ENOBUFS;
	}

	err = bt_obex_add_header_body_or_end_body(buf, ca_tg_peer_mopl,
						  sizeof(jpeg_1000001) - offset,
						  jpeg_1000001 + offset, &len);
	if (err != 0) {
		shell_error(sh, "Fail to add body");
	}

	if (bt_obex_has_header(buf, BT_OBEX_HEADER_ID_END_BODY)) {
		rsp_code = BT_OBEX_RSP_CODE_SUCCESS;
	} else {
		rsp_code = BT_OBEX_RSP_CODE_CONTINUE;
	}

error_rsp:
	err = bt_avrcp_cover_art_tg_get_linked_thumbnail(ca_tg, rsp_code, buf);
	if (err != 0) {
		if (buf != NULL) {
			net_buf_unref(buf);
		}
		shell_error(sh, "Fail to send get linked thumbnail rsp %d", err);
	} else {
		if (rsp_code == BT_OBEX_RSP_CODE_CONTINUE) {
			offset += len;
		} else {
			offset = 0;
		}
	}
	return err;
}

#endif /* CONFIG_BT_AVRCP_TG_COVER_ART */

#if defined(CONFIG_BT_AVRCP_CT_COVER_ART)
static struct bt_avrcp_cover_art_ct *ca_ct;
static uint32_t ca_ct_conn_id;

static void ct_l2cap_connected(struct bt_avrcp_ct *ct, struct bt_avrcp_cover_art_ct *cover_art_ct)
{
	bt_shell_print("Cover Art CT L2CAP Connected: %p", cover_art_ct);
}

static void ct_l2cap_disconnected(struct bt_avrcp_cover_art_ct *ct)
{
	ca_ct = NULL;
	bt_shell_print("Cover Art CT L2CAP Disconnected: %p", ct);
}

static void ct_connect(struct bt_avrcp_cover_art_ct *ct, uint8_t rsp_code, uint8_t version,
		       uint16_t mopl, struct net_buf *buf)
{
	int err;

	bt_shell_print("Cover Art CT %p conn rsp, rsp_code %s, version %02x, mopl %04x", ct,
		       bt_obex_rsp_code_to_str(rsp_code), version, mopl);
	cover_art_parse_headers(buf);
	err = bt_obex_get_header_conn_id(buf, &ca_ct_conn_id);
	if (err != 0) {
		bt_shell_error("Failed to get connection id");
	} else {
		bt_shell_print("Connection id %u", ca_ct_conn_id);
	}
}

static void ct_disconnect(struct bt_avrcp_cover_art_ct *ct, uint8_t rsp_code, struct net_buf *buf)
{
	bt_shell_print("Cover Art CT %p disconn rsp, rsp_code %s", ct,
		       bt_obex_rsp_code_to_str(rsp_code));
	cover_art_parse_headers(buf);
}

static uint8_t ct_get_image_properties_rsp_code;
static void ct_get_image_properties(struct bt_avrcp_cover_art_ct *ct, uint8_t rsp_code,
				    struct net_buf *buf)
{
	ct_get_image_properties_rsp_code = rsp_code;
	bt_shell_print("Cover Art CT %p get_image_properties rsp, rsp_code %s, data len %u", ct,
		       bt_obex_rsp_code_to_str(rsp_code), buf->len);
	cover_art_parse_headers(buf);
}

static uint8_t ct_get_image_rsp_code;
static void ct_get_image(struct bt_avrcp_cover_art_ct *ct, uint8_t rsp_code, struct net_buf *buf)
{
	ct_get_image_rsp_code = rsp_code;
	bt_shell_print("Cover Art CT %p get_image rsp, rsp_code %s, data len %u", ct,
		       bt_obex_rsp_code_to_str(rsp_code), buf->len);
	cover_art_parse_headers(buf);
}

static uint8_t ct_get_linked_thumbnail_rsp_code;
static void ct_get_linked_thumbnail(struct bt_avrcp_cover_art_ct *ct, uint8_t rsp_code,
				    struct net_buf *buf)
{
	ct_get_linked_thumbnail_rsp_code = rsp_code;
	bt_shell_print("Cover Art CT %p get_linked_thumbnail rsp, rsp_code %s, data len %u", ct,
		       bt_obex_rsp_code_to_str(rsp_code), buf->len);
	cover_art_parse_headers(buf);
}

static void ct_abort(struct bt_avrcp_cover_art_ct *ct, uint8_t rsp_code, struct net_buf *buf)
{
	ct_get_image_properties_rsp_code = BT_OBEX_RSP_CODE_SUCCESS;
	ct_get_image_rsp_code = BT_OBEX_RSP_CODE_SUCCESS;
	ct_get_linked_thumbnail_rsp_code = BT_OBEX_RSP_CODE_SUCCESS;
	bt_shell_print("Cover Art CT %p abort rsp, rsp_code %s", ct,
		       bt_obex_rsp_code_to_str(rsp_code));
	cover_art_parse_headers(buf);
}

static struct bt_avrcp_cover_art_ct_cb cover_art_ct_cb = {
	.l2cap_connected = ct_l2cap_connected,
	.l2cap_disconnected = ct_l2cap_disconnected,
	.connect = ct_connect,
	.disconnect = ct_disconnect,
	.abort = ct_abort,
	.get_image_properties = ct_get_image_properties,
	.get_image = ct_get_image,
	.get_linked_thumbnail = ct_get_linked_thumbnail,
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

extern struct bt_avrcp_ct *default_ct;

static int cmd_ct_l2cap_connect(const struct shell *sh, size_t argc, char *argv[])
{
	uint16_t psm;
	int err = 0;

	psm = shell_strtoul(argv[1], 16, &err);
	if (err != 0) {
		shell_error(sh, "Invalid PSM %s", argv[1]);
		return -ENOEXEC;
	}

	if (default_ct == NULL) {
		shell_error(sh, "Invalid CT");
		return -ENOEXEC;
	}

	err = bt_avrcp_cover_art_ct_l2cap_connect(default_ct, &ca_ct, psm);
	if (err != 0) {
		shell_error(sh, "L2CAP connect failed (err %d)", err);
	}

	return err;
}

static int cmd_ct_l2cap_disconnect(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	err = bt_avrcp_cover_art_ct_l2cap_disconnect(ca_ct);
	if (err != 0) {
		shell_error(sh, "L2CAP disconnect failed (err %d)", err);
	}

	return err;
}

static int cmd_ct_connect(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	err = bt_avrcp_cover_art_ct_connect(ca_ct);
	if (err != 0) {
		shell_error(sh, "OBEX connect failed (err %d)", err);
	}

	return err;
}

static int cmd_ct_disconnect(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	err = bt_avrcp_cover_art_ct_disconnect(ca_ct);
	if (err != 0) {
		shell_error(sh, "OBEX disconnect failed (err %d)", err);
	}

	return err;
}

static int cmd_ct_abort(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	err = bt_avrcp_cover_art_ct_abort(ca_ct, NULL);
	if (err != 0) {
		shell_error(sh, "OBEX abort failed (err %d)", err);
	}

	return err;
}

#define IMAGE_HANDLE "\x00\x31\x00\x30\x00\x30\x00\x30\x00\x30\x00\x30\x00\x31\x00\x00"

#if defined(CONFIG_BT_AVRCP_CT_COVER_ART_GET_IMAGE_PROPERTIES)
static int cmd_ct_get_image_properties(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_buf *buf;
	int err;

	if (ca_ct == NULL) {
		shell_error(sh, "Invalid Cover Art CT");
		return -ENOEXEC;
	}

	buf = bt_avrcp_cover_art_ct_create_pdu(ca_ct, &tx_pool);
	if (buf == NULL) {
		shell_error(sh, "Fail to allocate tx buffer");
		return -ENOBUFS;
	}

	if (ct_get_image_properties_rsp_code == BT_OBEX_RSP_CODE_CONTINUE) {
		goto continue_req;
	}

	err = bt_obex_add_header_conn_id(buf, ca_ct->client._conn_id);
	if (err != 0) {
		shell_error(sh, "Fail to add conn id header %d", err);
		goto failed;
	}

	err = bt_obex_add_header_type(buf, sizeof(BT_BIP_HDR_TYPE_GET_IMAGE_PROPERTIES),
				      BT_BIP_HDR_TYPE_GET_IMAGE_PROPERTIES);
	if (err != 0) {
		shell_error(sh, "Fail to add type header %d", err);
		goto failed;
	}

	err = bt_bip_add_header_image_handle(buf, sizeof(IMAGE_HANDLE) - 1, IMAGE_HANDLE);
	if (err != 0) {
		shell_error(sh, "Fail to add image handle header %d", err);
		goto failed;
	}

continue_req:
	err = bt_avrcp_cover_art_ct_get_image_properties(ca_ct, true, buf);
	if (err == 0) {
		return 0;
	}

	shell_error(sh, "Fail to send get_image_properties req %d", err);

failed:
	net_buf_unref(buf);
	return err;
}
#endif /* CONFIG_BT_AVRCP_CT_COVER_ART_GET_IMAGE_PROPERTIES */

#if defined(CONFIG_BT_AVRCP_CT_COVER_ART_GET_IMAGE)

#define IMAGE_DESC                                                                                 \
	"<image-descriptor version=\"1.0\">"                                                       \
	"<image encoding=\"JPEG\" pixel=\"640*480\" size=\"100000\"/>"                             \
	"</image-descriptor>"

static int cmd_ct_get_image(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_buf *buf;
	int err;

	if (ca_ct == NULL) {
		shell_error(sh, "Invalid Cover Art CT");
		return -ENOEXEC;
	}

	buf = bt_avrcp_cover_art_ct_create_pdu(ca_ct, &tx_pool);
	if (buf == NULL) {
		shell_error(sh, "Fail to allocate tx buffer");
		return -ENOBUFS;
	}

	if (ct_get_image_rsp_code == BT_OBEX_RSP_CODE_CONTINUE) {
		goto continue_req;
	}

	err = bt_obex_add_header_conn_id(buf, ca_ct->client._conn_id);
	if (err != 0) {
		shell_error(sh, "Fail to add conn id header %d", err);
		goto failed;
	}

	err = bt_obex_add_header_type(buf, sizeof(BT_BIP_HDR_TYPE_GET_IMAGE),
				      BT_BIP_HDR_TYPE_GET_IMAGE);
	if (err != 0) {
		shell_error(sh, "Fail to add type header %d", err);
		goto failed;
	}

	err = bt_bip_add_header_image_handle(buf, sizeof(IMAGE_HANDLE) - 1, IMAGE_HANDLE);
	if (err != 0) {
		shell_error(sh, "Fail to add image handle header %d", err);
		goto failed;
	}

	err = bt_bip_add_header_image_desc(buf, sizeof(IMAGE_DESC), IMAGE_DESC);
	if (err != 0) {
		shell_error(sh, "Fail to add image descriptor header %d", err);
		goto failed;
	}

continue_req:
	err = bt_avrcp_cover_art_ct_get_image(ca_ct, true, buf);
	if (err == 0) {
		return 0;
	}

	shell_error(sh, "Fail to send get_image req %d", err);

failed:
	net_buf_unref(buf);
	return err;
}
#endif /* CONFIG_BT_AVRCP_CT_COVER_ART_GET_IMAGE */

#if defined(CONFIG_BT_AVRCP_CT_COVER_ART_GET_LINKED_THUMBNAIL)
static int cmd_ct_get_linked_thumbnail(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_buf *buf;
	int err;

	if (ca_ct == NULL) {
		shell_error(sh, "Invalid Cover Art CT");
		return -ENOEXEC;
	}

	buf = bt_avrcp_cover_art_ct_create_pdu(ca_ct, &tx_pool);
	if (buf == NULL) {
		shell_error(sh, "Fail to allocate tx buffer");
		return -ENOBUFS;
	}

	if (ct_get_linked_thumbnail_rsp_code == BT_OBEX_RSP_CODE_CONTINUE) {
		goto continue_req;
	}

	err = bt_obex_add_header_conn_id(buf, ca_ct->client._conn_id);
	if (err != 0) {
		shell_error(sh, "Fail to add conn id header %d", err);
		goto failed;
	}

	err = bt_obex_add_header_type(buf, sizeof(BT_BIP_HDR_TYPE_GET_LINKED_THUMBNAIL),
				      BT_BIP_HDR_TYPE_GET_LINKED_THUMBNAIL);
	if (err != 0) {
		shell_error(sh, "Fail to add type header %d", err);
		goto failed;
	}

	err = bt_bip_add_header_image_handle(buf, sizeof(IMAGE_HANDLE) - 1, IMAGE_HANDLE);
	if (err != 0) {
		shell_error(sh, "Fail to add image handle header %d", err);
		goto failed;
	}

continue_req:
	err = bt_avrcp_cover_art_ct_get_linked_thumbnail(ca_ct, true, buf);
	if (err == 0) {
		return 0;
	}

	shell_error(sh, "Fail to send get_linked_thumbnail req %d", err);

failed:
	net_buf_unref(buf);
	return err;
}
#endif /* CONFIG_BT_AVRCP_CT_COVER_ART_GET_LINKED_THUMBNAIL */
#endif /* CONFIG_BT_AVRCP_CT_COVER_ART */

#define HELP_NONE "[none]"

#if defined(CONFIG_BT_AVRCP_TG_COVER_ART)
#define RSP_HELP_1 "<rsp: continue, success, error> [rsp_code]"
#define RSP_HELP_2 "<rsp: noerror, error> [rsp_code]"

SHELL_STATIC_SUBCMD_SET_CREATE(
	tg_cmds, SHELL_CMD_ARG(reg, NULL, HELP_NONE, cmd_tg_reg, 1, 0),
	SHELL_CMD_ARG(l2cap_disconnect, NULL, HELP_NONE, cmd_tg_l2cap_disconnect, 1, 0),
	SHELL_CMD_ARG(connect, NULL, RSP_HELP_1, cmd_tg_connect, 2, 1),
	SHELL_CMD_ARG(disconnect, NULL, RSP_HELP_1, cmd_tg_disconnect, 2, 1),
	SHELL_CMD_ARG(abort, NULL, RSP_HELP_1, cmd_tg_abort, 2, 1),
	SHELL_CMD_ARG(get_image_properties, NULL, RSP_HELP_2, cmd_tg_get_image_properties, 2, 1),
	SHELL_CMD_ARG(get_image, NULL, RSP_HELP_2, cmd_tg_get_image, 2, 1),
	SHELL_CMD_ARG(get_linked_thumbnail, NULL, RSP_HELP_2, cmd_tg_get_linked_thumbnail, 2, 1),
	SHELL_SUBCMD_SET_END);
#endif /* CONFIG_BT_AVRCP_TG_COVER_ART */

#if defined(CONFIG_BT_AVRCP_CT_COVER_ART)
SHELL_STATIC_SUBCMD_SET_CREATE(
	ct_cmds, SHELL_CMD_ARG(reg, NULL, HELP_NONE, cmd_ct_reg, 1, 0),
	SHELL_CMD_ARG(l2cap_connect, NULL, "<psm>", cmd_ct_l2cap_connect, 2, 0),
	SHELL_CMD_ARG(l2cap_disconnect, NULL, HELP_NONE, cmd_ct_l2cap_disconnect, 1, 0),
	SHELL_CMD_ARG(connect, NULL, HELP_NONE, cmd_ct_connect, 1, 0),
	SHELL_CMD_ARG(disconnect, NULL, HELP_NONE, cmd_ct_disconnect, 1, 0),
	SHELL_CMD_ARG(abort, NULL, HELP_NONE, cmd_ct_abort, 1, 0),
#if defined(CONFIG_BT_AVRCP_CT_COVER_ART_GET_IMAGE_PROPERTIES)
	SHELL_CMD_ARG(get_image_properties, NULL, HELP_NONE, cmd_ct_get_image_properties, 1, 0),
#endif /* CONFIG_BT_AVRCP_CT_COVER_ART_GET_IMAGE_PROPERTIES */
#if defined(CONFIG_BT_AVRCP_CT_COVER_ART_GET_IMAGE)
	SHELL_CMD_ARG(get_image, NULL, HELP_NONE, cmd_ct_get_image, 1, 0),
#endif /* CONFIG_BT_AVRCP_CT_COVER_ART_GET_IMAGE */
#if defined(CONFIG_BT_AVRCP_CT_COVER_ART_GET_LINKED_THUMBNAIL)
	SHELL_CMD_ARG(get_linked_thumbnail, NULL, HELP_NONE, cmd_ct_get_linked_thumbnail, 1, 0),
#endif /* CONFIG_BT_AVRCP_CT_COVER_ART_GET_LINKED_THUMBNAIL */
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

SHELL_STATIC_SUBCMD_SET_CREATE(avrcp_cover_art_cmds,
#if defined(CONFIG_BT_AVRCP_CT_COVER_ART)
			       SHELL_CMD(ct, &ct_cmds, "AVRCP Cover Art CT shell commands",
					 cmd_avrcp_cover_art),
#endif /* CONFIG_BT_AVRCP_CT_COVER_ART*/
#if defined(CONFIG_BT_AVRCP_TG_COVER_ART)
			       SHELL_CMD(tg, &tg_cmds, "AVRCP Cover Art TG shell commands",
					 cmd_avrcp_cover_art),
#endif /* CONFIG_BT_AVRCP_TG_COVER_ART */
			       SHELL_SUBCMD_SET_END);

SHELL_CMD_ARG_REGISTER(avrcp_cover_art, &avrcp_cover_art_cmds,
		       "Bluetooth AVRCP Cover Art shell commands", cmd_avrcp_cover_art, 1, 1);
