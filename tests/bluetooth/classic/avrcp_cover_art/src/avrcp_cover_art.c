/* avrcp_cover_art.c - Bluetooth classic AVRCP cover art test */

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

#define BT_OBEX_SRM_DISABLE 0x00
#define BT_OBEX_SRM_ENABLE  0x01
#define BT_OBEX_SRMP_WAIT   0x01

#define LOCAL_SRM_ENABLED   0x01
#define REMOTE_SRM_ENABLED  0x02
#define LOCAL_SRMP_ENABLED  0x04
#define REMOTE_SRMP_ENABLED 0x08
#define SRM_NO_WAIT_ENABLED 0x80

#define USER_DEFINED_APP_PARAM_HDR 0xFF
#define APP_PARAM_USED_IN_ABORT    "abort"

#define COVER_ART_MOPL CONFIG_BT_GOEP_L2CAP_MTU - 4

NET_BUF_POOL_FIXED_DEFINE(tx_pool, CONFIG_BT_MAX_CONN, BT_L2CAP_BUF_SIZE(COVER_ART_MOPL),
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

static bool is_local_srm_enabled(uint8_t srm)
{
	return ((srm & LOCAL_SRM_ENABLED) != 0U);
}

static bool is_remote_srm_enabled(uint8_t srm)
{
	return ((srm & REMOTE_SRM_ENABLED) != 0U);
}

static bool is_srm_enabled(uint8_t srm)
{
	return (is_local_srm_enabled(srm) && is_remote_srm_enabled(srm));
}

static bool is_srm_no_wait_enabled(uint8_t srm)
{
	return (is_local_srm_enabled(srm) && is_remote_srm_enabled(srm) &&
		((srm & SRM_NO_WAIT_ENABLED) != 0U));
}

static bool is_local_srmp_enabled(uint8_t srm)
{
	return ((srm & LOCAL_SRMP_ENABLED) != 0U);
}

static bool is_remote_srmp_enabled(uint8_t srm)
{
	return ((srm & REMOTE_SRMP_ENABLED) != 0U);
}

static bool is_srmp_enabled(uint8_t srm)
{
	return (is_local_srmp_enabled(srm) || is_remote_srmp_enabled(srm));
}

static void set_srm_no_wait(uint8_t *srm_state)
{
	/* If SRM is enabled and SRMP is not enabled, set SRM to enabled without waiting */
	if (is_srm_enabled(*srm_state) && !is_srmp_enabled(*srm_state)) {
		*srm_state |= SRM_NO_WAIT_ENABLED;
	}
}

static bool ca_find_header_cb(struct bt_obex_hdr *hdr, void *user_data)
{
	struct bt_obex_hdr *data = user_data;

	if (hdr->id == data->id) {
		data->data = hdr->data;
		data->len = hdr->len;
		return false;
	}
	return true;
}

static bool cover_art_parse_tlvs_cb(struct bt_obex_tlv *tlv, void *user_data)
{
	if (tlv->type == USER_DEFINED_APP_PARAM_HDR) {
		bt_shell_print("T %02x L %d V %.*s", tlv->type, tlv->data_len, tlv->data_len,
			       tlv->data);
	} else {
		bt_shell_print("T %02x L %d", tlv->type, tlv->data_len);
	}

	return true;
}

static bool cover_art_parse_headers_cb(struct bt_obex_hdr *hdr, void *user_data)
{
	bt_shell_print("HI %02x Len %d", hdr->id, hdr->len);

	switch (hdr->id) {
	case BT_OBEX_HEADER_ID_SRM:
		bt_shell_print("SRM value: %02x", hdr->data[0]);
		break;
	case BT_OBEX_HEADER_ID_SRM_PARAM:
		bt_shell_print("SRM Parameter value: %02x", hdr->data[0]);
		break;
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
static struct bt_avrcp_cover_art_tg *ca_tg;
static uint16_t ca_tg_peer_mopl;
static uint8_t ca_tg_srm;
static bool ca_tg_chunked_req;
static uint16_t ca_tg_offest;

static void tg_l2cap_connected(struct bt_avrcp_tg *tg, struct bt_avrcp_cover_art_tg *cover_art_tg)
{
	ca_tg = cover_art_tg;
	bt_shell_print("Cover Art TG L2CAP Connected: %p", cover_art_tg);
}

static void tg_l2cap_disconnected(struct bt_avrcp_cover_art_tg *tg)
{
	ca_tg = NULL;
	ca_tg_srm = 0;
	ca_tg_chunked_req = false;
	ca_tg_offest = 0;
	bt_shell_print("Cover Art TG L2CAP Disconnected: %p", tg);
}

static void tg_connect(struct bt_avrcp_cover_art_tg *tg, uint8_t version, uint16_t mopl,
		       struct net_buf *buf)
{
	bt_shell_print("Cover Art TG %p OBEX connect req: version %02x, mopl %04x", tg, version,
		       mopl);
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

static int tg_parse_header_srm(struct net_buf *buf, uint8_t *srm_state)
{
	struct bt_obex_hdr hdr;
	uint8_t srm = 0xFF;
	int err;

	/* Skip if remote SRM is enabled */
	if (is_remote_srm_enabled(*srm_state)) {
		return 0;
	}

	hdr.id = BT_OBEX_HEADER_ID_SRM;
	hdr.len = 0;
	hdr.data = NULL;
	err = bt_obex_header_parse(buf, ca_find_header_cb, &hdr);
	if (err != 0) {
		return err;
	}

	if ((hdr.data != NULL) && (hdr.len == 1)) {
		srm = hdr.data[0];
	}

	if (srm == BT_OBEX_SRM_ENABLE) {
		*srm_state |= REMOTE_SRM_ENABLED;
	}

	return 0;
}

static int tg_parse_header_srm_param(struct net_buf *buf, uint8_t *srm_state)
{
	struct bt_obex_hdr hdr;
	uint8_t srmp = 0xFF;
	int err;

	/* Skip if remote SRM is not enabled or already in SRM no-wait mode */
	if (!is_remote_srm_enabled(*srm_state) || is_srm_no_wait_enabled(*srm_state)) {
		return 0;
	}

	hdr.id = BT_OBEX_HEADER_ID_SRM_PARAM;
	hdr.len = 0;
	hdr.data = NULL;
	err = bt_obex_header_parse(buf, ca_find_header_cb, &hdr);
	if (err != 0) {
		return err;
	}

	if ((hdr.data != NULL) && (hdr.len == 1)) {
		srmp = hdr.data[0];
	}

	if (srmp == BT_OBEX_SRMP_WAIT) {
		*srm_state |= REMOTE_SRMP_ENABLED;
	} else {
		*srm_state &= ~REMOTE_SRMP_ENABLED;
	}

	set_srm_no_wait(srm_state);

	return 0;
}

static void tg_get_image_properties(struct bt_avrcp_cover_art_tg *tg, bool final,
				    struct net_buf *buf)
{
	bt_shell_print("Cover Art TG %p get_image_properties req, final %s, data len %u", tg,
		       final ? "true" : "false", buf->len);
	cover_art_parse_headers(buf);

	ca_tg_chunked_req = !final;
	tg_parse_header_srm(buf, &ca_tg_srm);
	tg_parse_header_srm_param(buf, &ca_tg_srm);
}

static void tg_get_image(struct bt_avrcp_cover_art_tg *tg, bool final, struct net_buf *buf)
{
	bt_shell_print("Cover Art TG %p get_image req, final %s, data len %u", tg,
		       final ? "true" : "false", buf->len);
	cover_art_parse_headers(buf);

	ca_tg_chunked_req = !final;
	tg_parse_header_srm(buf, &ca_tg_srm);
	tg_parse_header_srm_param(buf, &ca_tg_srm);
}

static void tg_get_linked_thumbnail(struct bt_avrcp_cover_art_tg *tg, bool final,
				    struct net_buf *buf)
{
	bt_shell_print("Cover Art TG %p get_linked_thumbnail req, final %s, data len %u", tg,
		       final ? "true" : "false", buf->len);
	cover_art_parse_headers(buf);

	ca_tg_chunked_req = !final;
	tg_parse_header_srm(buf, &ca_tg_srm);
	tg_parse_header_srm_param(buf, &ca_tg_srm);
}

static struct bt_avrcp_cover_art_tg_cb cover_art_tg_cb = {
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
	struct bt_avrcp_cover_art_tg_cb *cb;
	int err;

	if ((argc > 1) && (!strcmp(argv[1], "NULL"))) {
		cb = NULL;
	} else {
		cb = &cover_art_tg_cb;
	}

	err = bt_avrcp_cover_art_tg_cb_register(cb);
	if (err != 0) {
		shell_error(sh, "Failed to reg cover art TG cb (err %d)", err);
	}

	return err;
}

static int cmd_tg_alloc_buf(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_buf_pool *pool;
	struct net_buf *buf;

	if ((argc > 1) && (!strcmp(argv[1], "NULL"))) {
		pool = NULL;
	} else {
		pool = &tx_pool;
	}

	buf = bt_avrcp_cover_art_tg_create_pdu(ca_tg, pool);
	if (buf == NULL) {
		shell_error(sh, "Fail to allocate tx buffer");
		return -ENOBUFS;
	} else {
		shell_print(sh, "Allocate tx buffer successfully: %p", buf);
		net_buf_unref(buf);
		return 0;
	}
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
	bool add_header = false;
	struct net_buf *buf = NULL;
	int err;

	err = parse_rsp_code(sh, argc, argv, &rsp_code);
	if (err != 0) {
		return err;
	}

	if (rsp_code == BT_OBEX_RSP_CODE_SUCCESS) {
		if ((argc > 2) && (!strcmp(argv[2], "true"))) {
			add_header = true;
		}
	}

	if (add_header) {
		struct bt_obex_tlv tlv[1];

		buf = bt_avrcp_cover_art_tg_create_pdu(ca_tg, &tx_pool);
		if (buf == NULL) {
			shell_error(sh, "Fail to allocate tx buffer");
			return -ENOBUFS;
		}

		tlv[0].type = USER_DEFINED_APP_PARAM_HDR;
		tlv[0].data_len = strlen(APP_PARAM_USED_IN_ABORT);
		tlv[0].data = (const uint8_t *)APP_PARAM_USED_IN_ABORT;
		err = bt_obex_add_header_app_param(buf, ARRAY_SIZE(tlv), tlv);
		if (err != 0) {
			net_buf_unref(buf);
			bt_shell_error("Failed to add application parameter header");
			return err;
		}
	}

	err = bt_avrcp_cover_art_tg_abort(ca_tg, rsp_code, buf);
	if (err != 0) {
		shell_error(sh, "Cover art TG abort rsp failed (err %d)", err);
		if (buf != NULL) {
			net_buf_unref(buf);
		}
	} else {
		ca_tg_chunked_req = false;
		ca_tg_srm = 0;
		ca_tg_offest = 0;
	}

	return err;
}

static int tg_parse_params(const struct shell *sh, size_t argc, char *argv[], uint8_t *rsp_code,
			   bool *enable_srm, bool *enable_srmp)
{
	int arg_idx = 1;

	/* Initialize output parameters */
	if (rsp_code) {
		*rsp_code = 0;
	}
	if (enable_srm) {
		*enable_srm = false;
	}
	if (enable_srmp) {
		*enable_srmp = false;
	}

	/* Parse optional parameters */
	while (arg_idx < argc) {
		if (!strcmp(argv[arg_idx], "noerror")) {
			/* nothing to do */
		} else if (!strcmp(argv[arg_idx], "error")) {
			if (arg_idx + 1 >= argc) {
				shell_error(sh, "[rsp_code] is needed if the rsp is %s", argv[1]);
				return -EINVAL;
			}
			arg_idx++;
			*rsp_code = (uint8_t)strtoul(argv[arg_idx], NULL, 16);
		} else if (!strcmp(argv[arg_idx], "srm")) {
			if (arg_idx + 1 >= argc) {
				shell_error(sh, "[true or false] is needed after 'srm'");
				return -EINVAL;
			}
			arg_idx++;
			if (!strcmp(argv[arg_idx], "true")) {
				*enable_srm = true;
			} else if (!strcmp(argv[arg_idx], "false")) {
				*enable_srm = false;
			} else {
				shell_error(sh, "Invalid srm value: %s (expected true/false)",
					    argv[arg_idx]);
				return -EINVAL;
			}
			bt_shell_print("SRM mode: %s", enable_srm ? "enabled" : "disabled");
		} else if (!strcmp(argv[arg_idx], "srmp")) {
			if (arg_idx + 1 >= argc) {
				shell_error(sh, "[true or false] is needed after 'srmp'");
				return -EINVAL;
			}
			arg_idx++;
			if (!strcmp(argv[arg_idx], "true")) {
				*enable_srmp = true;
			} else if (!strcmp(argv[arg_idx], "false")) {
				*enable_srmp = false;
			} else {
				shell_error(sh, "Invalid srmp value: %s (expected true/false)",
					    argv[arg_idx]);
				return -EINVAL;
			}
			bt_shell_print("SRMP wait: %s", enable_srmp ? "enabled" : "disabled");
		} else {
			shell_error(sh, "Unknown parameter: %s", argv[arg_idx]);
			return -EINVAL;
		}
		arg_idx++;
	}

	if (*enable_srmp && !*enable_srm) {
		shell_error(sh, "SRMP requires SRM to be enabled");
		return -EINVAL;
	}

	return 0;
}

static int tg_add_header_srm(struct net_buf *buf, uint8_t *srm_state, bool enable_srm)
{
	int err;

	/* Skip if remote SRM is not enabled or SRM Enabled*/
	if (!is_remote_srm_enabled(*srm_state) || is_srm_enabled(*srm_state)) {
		return 0;
	}

	/* If remote SRM is enabled and local SRM is not enabled, add SRM header if enabled */
	if (enable_srm) {
		err = bt_obex_add_header_srm(buf, BT_OBEX_SRM_ENABLE);
		if (err != 0) {
			return err;
		}
		*srm_state |= LOCAL_SRM_ENABLED;
	}

	return 0;
}

static int tg_add_header_srm_param(struct net_buf *buf, uint8_t *srm_state, bool enable_srmp)
{
	int err;

	/* Skip if SRM is not enabled or already in no-wait state */
	if (!is_srm_enabled(*srm_state) || is_srm_no_wait_enabled(*srm_state)) {
		return 0;
	}

	/* If SRM Enabled but waiting, add SRMP header if enabled */
	if (enable_srmp) {
		err = bt_obex_add_header_srm_param(buf, BT_OBEX_SRMP_WAIT);
		if (err != 0) {
			return err;
		}
		*srm_state |= LOCAL_SRMP_ENABLED;
	} else {
		*srm_state &= ~LOCAL_SRMP_ENABLED;
	}

	return 0;
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
	uint8_t rsp_code = 0;
	bool enable_srm = false;
	bool enable_srmp = false;
	int err;
	uint16_t len = 0;
	struct net_buf *buf = NULL;
	const uint8_t *image_properties_body = (const uint8_t *)IMAGE_PROPERTIES_BODY;

	if (ca_tg == NULL) {
		shell_error(sh, "Invalid Cover Art Target");
		return -ENOEXEC;
	}

	buf = bt_avrcp_cover_art_tg_create_pdu(ca_tg, &tx_pool);
	if (buf == NULL) {
		shell_error(sh, "Fail to allocate tx buffer");
		return -ENOBUFS;
	}

	err = tg_parse_params(sh, argc, argv, &rsp_code, &enable_srm, &enable_srmp);
	if (err != 0) {
		net_buf_unref(buf);
		shell_help(sh);
		return SHELL_CMD_HELP_PRINTED;
	}

	if (rsp_code != 0U) {
		goto error_rsp;
	}

	err = tg_add_header_srm(buf, &ca_tg_srm, enable_srm);
	if (err != 0) {
		shell_error(sh, "Fail to add SRM header %d", err);
		net_buf_unref(buf);
		return err;
	}

	err = tg_add_header_srm_param(buf, &ca_tg_srm, enable_srmp);
	if (err != 0) {
		shell_error(sh, "Fail to add SRMP header %d", err);
		net_buf_unref(buf);
		return err;
	}

	if (ca_tg_chunked_req) {
		if (is_srm_no_wait_enabled(ca_tg_srm)) {
			net_buf_unref(buf);
			return 0;
		}
		rsp_code = BT_OBEX_RSP_CODE_CONTINUE;
		goto error_rsp;
	}

	err = bt_obex_add_header_body_or_end_body(buf, ca_tg_peer_mopl,
						  sizeof(IMAGE_PROPERTIES_BODY) - ca_tg_offest,
						  image_properties_body + ca_tg_offest, &len);
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
			set_srm_no_wait(&ca_tg_srm);
			ca_tg_offest += len;
		} else {
			ca_tg_srm = 0;
			ca_tg_chunked_req = false;
			ca_tg_offest = 0;
		}
	}
	return err;
}

/*
 * White JPEG original image (300x300 pixels)
 * JPEG baseline-compliant, sRGB, YCC422 sampling
 */
static const uint8_t jpeg_1000001[] = {
	/* SOI (Start of Image) marker */
	0xFF, 0xD8,

	/* APP0 (JFIF) marker segment */
	0xFF, 0xE0, 0x00, 0x10,       /* Length: 16 bytes */
	0x4A, 0x46, 0x49, 0x46, 0x00, /* Identifier: "JFIF\0" */
	0x01, 0x01,                   /* Version: 1.1 */
	0x01,                         /* Units: dots per inch */
	0x00, 0x48,                   /* X density: 72 dpi */
	0x00, 0x48,                   /* Y density: 72 dpi */
	0x00,                         /* Thumbnail width: 0 */
	0x00,                         /* Thumbnail height: 0 */

	/* APP14 (Adobe) marker for sRGB color space */
	0xFF, 0xEE, 0x00, 0x0E,       /* Length: 14 bytes */
	0x41, 0x64, 0x6F, 0x62, 0x65, /* Identifier: "Adobe" */
	0x00, 0x64,                   /* Version: 100 */
	0x00, 0x00,                   /* Flags0: 0 */
	0x00, 0x00,                   /* Flags1: 0 */
	0x01,                         /* Color transform: YCbCr */

	/* DQT (Define Quantization Table) marker - Luminance */
	0xFF, 0xDB, 0x00, 0x43, /* Length: 67 bytes */
	0x00,                   /* Table ID: 0, 8-bit precision */
	/* Quantization table for Y component (optimized for white, higher compression) */
	0x10, 0x0B, 0x0C, 0x0E, 0x0C, 0x0A, 0x10, 0x0E, 0x0D, 0x0E, 0x12, 0x11, 0x10, 0x13, 0x18,
	0x28, 0x1A, 0x18, 0x16, 0x16, 0x18, 0x31, 0x23, 0x25, 0x1D, 0x28, 0x3A, 0x33, 0x3D, 0x3C,
	0x39, 0x33, 0x38, 0x37, 0x40, 0x48, 0x5C, 0x4E, 0x40, 0x44, 0x57, 0x45, 0x37, 0x38, 0x50,
	0x6D, 0x51, 0x57, 0x5F, 0x62, 0x67, 0x68, 0x67, 0x3E, 0x4D, 0x71, 0x79, 0x70, 0x64, 0x78,
	0x5C, 0x65, 0x67, 0x63,

	/* DQT (Define Quantization Table) marker - Chrominance */
	0xFF, 0xDB, 0x00, 0x43, /* Length: 67 bytes */
	0x01,                   /* Table ID: 1, 8-bit precision */
	/* Quantization table for Cb/Cr components (optimized for white, higher compression) */
	0x11, 0x12, 0x12, 0x18, 0x15, 0x18, 0x2F, 0x1A, 0x1A, 0x2F, 0x63, 0x42, 0x38, 0x42, 0x63,
	0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63,
	0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63,
	0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63,
	0x63, 0x63, 0x63, 0x63,

	/* SOF0 (Start of Frame - Baseline DCT) marker */
	0xFF, 0xC0, 0x00, 0x11, /* Length: 17 bytes */
	0x08,                   /* Sample precision: 8 bits */
	0x01, 0x2C,             /* Image height: 300 pixels */
	0x01, 0x2C,             /* Image width: 300 pixels */
	0x03,                   /* Number of components: 3 (Y, Cb, Cr) */
	/* Component 1 (Y - Luminance) */
	0x01, /* Component ID: 1 */
	0x22, /* Sampling factors: H=2, V=2 (4:2:2) */
	0x00, /* Quantization table ID: 0 */
	/* Component 2 (Cb - Chrominance blue) */
	0x02, /* Component ID: 2 */
	0x11, /* Sampling factors: H=1, V=1 */
	0x01, /* Quantization table ID: 1 */
	/* Component 3 (Cr - Chrominance red) */
	0x03, /* Component ID: 3 */
	0x11, /* Sampling factors: H=1, V=1 */
	0x01, /* Quantization table ID: 1 */

	/* DHT (Define Huffman Table) marker - DC Luminance */
	0xFF, 0xC4, 0x00, 0x1F, /* Length: 31 bytes */
	0x00,                   /* Table class: DC, Table ID: 0 */
	/* Number of codes of each length (1-16) */
	0x00, 0x01, 0x05, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00,
	/* Huffman values */
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B,

	/* DHT (Define Huffman Table) marker - AC Luminance */
	0xFF, 0xC4, 0x00, 0xB5, /* Length: 181 bytes */
	0x10,                   /* Table class: AC, Table ID: 0 */
	/* Number of codes of each length (1-16) */
	0x00, 0x02, 0x01, 0x03, 0x03, 0x02, 0x04, 0x03, 0x05, 0x05, 0x04, 0x04, 0x00, 0x00, 0x01,
	0x7D,
	/* Huffman values */
	0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12, 0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61,
	0x07, 0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xA1, 0x08, 0x23, 0x42, 0xB1, 0xC1, 0x15, 0x52,
	0xD1, 0xF0, 0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0A, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x25,
	0x26, 0x27, 0x28, 0x29, 0x2A, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x43, 0x44, 0x45,
	0x46, 0x47, 0x48, 0x49, 0x4A, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x63, 0x64,
	0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x83,
	0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99,
	0x9A, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6,
	0xB7, 0xB8, 0xB9, 0xBA, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xD2, 0xD3,
	0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8,
	0xE9, 0xEA, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA,

	/* DHT (Define Huffman Table) marker - DC Chrominance */
	0xFF, 0xC4, 0x00, 0x1F, /* Length: 31 bytes */
	0x01,                   /* Table class: DC, Table ID: 1 */
	/* Number of codes of each length (1-16) */
	0x00, 0x03, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00,
	0x00,
	/* Huffman values */
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B,

	/* DHT (Define Huffman Table) marker - AC Chrominance */
	0xFF, 0xC4, 0x00, 0xB5, /* Length: 181 bytes */
	0x11,                   /* Table class: AC, Table ID: 1 */
	/* Number of codes of each length (1-16) */
	0x00, 0x02, 0x01, 0x02, 0x04, 0x04, 0x03, 0x04, 0x07, 0x05, 0x04, 0x04, 0x00, 0x01, 0x02,
	0x77,
	/* Huffman values */
	0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21, 0x31, 0x06, 0x12, 0x41, 0x51, 0x07, 0x61,
	0x71, 0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91, 0xA1, 0xB1, 0xC1, 0x09, 0x23, 0x33,
	0x52, 0xF0, 0x15, 0x62, 0x72, 0xD1, 0x0A, 0x16, 0x24, 0x34, 0xE1, 0x25, 0xF1, 0x17, 0x18,
	0x19, 0x1A, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x43, 0x44,
	0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x63,
	0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A,
	0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97,
	0x98, 0x99, 0x9A, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xB2, 0xB3, 0xB4,
	0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA,
	0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7,
	0xE8, 0xE9, 0xEA, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA,

	/* SOS (Start of Scan) marker */
	0xFF, 0xDA, 0x00, 0x0C, /* Length: 12 bytes */
	0x03,                   /* Number of components: 3 */
	/* Component 1 (Y - Luminance) */
	0x01, /* Component ID: 1 */
	0x00, /* DC table: 0, AC table: 0 */
	/* Component 2 (Cb - Chrominance blue) */
	0x02, /* Component ID: 2 */
	0x11, /* DC table: 1, AC table: 1 */
	/* Component 3 (Cr - Chrominance red) */
	0x03, /* Component ID: 3 */
	0x11, /* DC table: 1, AC table: 1 */
	0x00, /* Start of spectral selection: 0 */
	0x3F, /* End of spectral selection: 63 */
	0x00, /* Successive approximation: 0 */

	/* Compressed image data (minimal white image data for 300x300) */
	/* Optimized entropy-coded data for pure white image */
	0xFF, 0xC0, 0x00, 0x3F, 0xFF, 0xC0, 0x00, 0x3F, 0xFF, 0xC0, 0x00, 0x3F, 0xFF, 0xC0, 0x00,
	0x3F, 0xFF, 0xC0, 0x00, 0x3F, 0xFF, 0xC0, 0x00, 0x3F, 0xFF, 0xC0, 0x00, 0x3F, 0xFF, 0xC0,
	0x00, 0x3F, 0xFF, 0xC0, 0x00, 0x3F, 0xFF, 0xC0, 0x00, 0x3F, 0xFF, 0xC0, 0x00, 0x3F, 0xFF,
	0xC0, 0x00, 0x3F, 0xFF, 0xC0, 0x00, 0x3F, 0xFF, 0xC0, 0x00, 0x3F, 0xFF, 0xC0, 0x00, 0x3F,
	0xFF, 0xC0, 0x00, 0x3F, 0xFF, 0xC0, 0x00, 0x3F, 0xFF, 0xC0, 0x00, 0x3F, 0xFF, 0xC0, 0x00,
	0x3F, 0xFF, 0xC0, 0x00, 0x3F, 0xFF, 0xC0, 0x00, 0x3F, 0xFF, 0xC0, 0x00, 0x3F, 0xFF, 0xC0,
	0x00, 0x3F, 0xFF, 0xC0, 0x00, 0x3F, 0xFF, 0xC0, 0x00, 0x3F, 0xFF, 0xC0, 0x00, 0x3F, 0xFF,
	0xC0, 0x00, 0x3F, 0xFF, 0xC0, 0x00, 0x3F, 0xFF, 0xC0, 0x00, 0x3F, 0xFF, 0xC0, 0x00, 0x3F,
	0xFF, 0xC0, 0x00, 0x3F, 0xFF, 0xC0, 0x00, 0x3F, 0xFF, 0xC0, 0x00, 0x3F, 0xFF, 0xC0, 0x00,
	0x3F, 0xFF, 0xC0, 0x00, 0x3F, 0xFF, 0xC0, 0x00, 0x3F, 0xFF, 0xC0, 0x00, 0x3F, 0xFF, 0xC0,
	0x00, 0x3F, 0xFF, 0xC0, 0x00, 0x3F, 0xFF, 0xC0, 0x00, 0x3F, 0xFF, 0xC0, 0x00, 0x3F, 0xFF,
	0xC0, 0x00, 0x3F, 0xFF, 0xC0, 0x00, 0x3F, 0xFF, 0xC0, 0x00, 0x3F, 0xFF, 0xC0, 0x00, 0x3F,
	0xFF, 0xC0, 0x00, 0x3F, 0xFF, 0xC0, 0x00, 0x3F, 0xFF, 0xC0, 0x00, 0x3F, 0xFF, 0xC0, 0x00,
	0x3F, 0xFF, 0xC0, 0x00, 0x3F, 0xFF, 0xC0, 0x00, 0x3F, 0xFF, 0xC0, 0x00, 0x3F, 0xFF, 0xC0,
	0x00, 0x3F, 0xFF, 0xC0, 0x00, 0x3F, 0xFF, 0xC0, 0x00, 0x3F, 0xFF, 0xC0, 0x00, 0x3F, 0xFF,
	0xC0, 0x00, 0x3F, 0xFF, 0xC0, 0x00, 0x3F, 0xFF, 0xC0, 0x00, 0x3F, 0xFF, 0xC0, 0x00, 0x3F,
	0xFF, 0xC0, 0x00, 0x3F, 0xFF, 0xC0, 0x00, 0x3F, 0xFF, 0xC0, 0x00, 0x3F, 0xFF, 0xC0, 0x00,
	0x3F,

	/* EOI (End of Image) marker */
	0xFF, 0xD9};

static int cmd_tg_get_image(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t rsp_code = 0;
	bool enable_srm = false;
	bool enable_srmp = false;
	int err;
	uint16_t len = 0;
	struct net_buf *buf = NULL;

	if (ca_tg == NULL) {
		shell_error(sh, "Invalid Cover Art Target");
		return -ENOEXEC;
	}

	buf = bt_avrcp_cover_art_tg_create_pdu(ca_tg, &tx_pool);
	if (buf == NULL) {
		shell_error(sh, "Fail to allocate tx buffer");
		return -ENOBUFS;
	}

	err = tg_parse_params(sh, argc, argv, &rsp_code, &enable_srm, &enable_srmp);
	if (err != 0) {
		net_buf_unref(buf);
		shell_help(sh);
		return SHELL_CMD_HELP_PRINTED;
	}

	if (rsp_code != 0U) {
		goto error_rsp;
	}

	err = tg_add_header_srm(buf, &ca_tg_srm, enable_srm);
	if (err != 0) {
		shell_error(sh, "Fail to add SRM header %d", err);
		net_buf_unref(buf);
		return err;
	}

	err = tg_add_header_srm_param(buf, &ca_tg_srm, enable_srmp);
	if (err != 0) {
		shell_error(sh, "Fail to add SRMP header %d", err);
		net_buf_unref(buf);
		return err;
	}

	if (ca_tg_chunked_req) {
		if (is_srm_no_wait_enabled(ca_tg_srm)) {
			net_buf_unref(buf);
			return 0;
		}
		rsp_code = BT_OBEX_RSP_CODE_CONTINUE;
		goto error_rsp;
	}

	if (ca_tg_offest == 0) {
		err = bt_obex_add_header_len(buf, sizeof(jpeg_1000001));
		if (err != 0) {
			shell_error(sh, "Fail to add len header %d", err);
			net_buf_unref(buf);
			return err;
		}
	}

	err = bt_obex_add_header_body_or_end_body(buf, ca_tg_peer_mopl,
						  sizeof(jpeg_1000001) - ca_tg_offest,
						  jpeg_1000001 + ca_tg_offest, &len);
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
			set_srm_no_wait(&ca_tg_srm);
			ca_tg_offest += len;
		} else {
			ca_tg_srm = 0;
			ca_tg_chunked_req = false;
			ca_tg_offest = 0;
		}
	}
	return err;
}

static int cmd_tg_get_linked_thumbnail(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t rsp_code = 0;
	bool enable_srm = false;
	bool enable_srmp = false;
	int err;
	uint16_t len = 0;
	struct net_buf *buf = NULL;

	if (ca_tg == NULL) {
		shell_error(sh, "Invalid Cover Art Target");
		return -ENOEXEC;
	}

	buf = bt_avrcp_cover_art_tg_create_pdu(ca_tg, &tx_pool);
	if (buf == NULL) {
		shell_error(sh, "Fail to allocate tx buffer");
		return -ENOBUFS;
	}

	err = tg_parse_params(sh, argc, argv, &rsp_code, &enable_srm, &enable_srmp);
	if (err != 0) {
		net_buf_unref(buf);
		shell_help(sh);
		return SHELL_CMD_HELP_PRINTED;
	}

	if (rsp_code != 0U) {
		goto error_rsp;
	}

	err = tg_add_header_srm(buf, &ca_tg_srm, enable_srm);
	if (err != 0) {
		shell_error(sh, "Fail to add SRM header %d", err);
		net_buf_unref(buf);
		return err;
	}

	err = tg_add_header_srm_param(buf, &ca_tg_srm, enable_srmp);
	if (err != 0) {
		shell_error(sh, "Fail to add SRMP header %d", err);
		net_buf_unref(buf);
		return err;
	}

	if (ca_tg_chunked_req) {
		if (is_srm_no_wait_enabled(ca_tg_srm)) {
			net_buf_unref(buf);
			return 0;
		}
		rsp_code = BT_OBEX_RSP_CODE_CONTINUE;
		goto error_rsp;
	}

	err = bt_obex_add_header_body_or_end_body(buf, ca_tg_peer_mopl,
						  sizeof(jpeg_1000001) - ca_tg_offest,
						  jpeg_1000001 + ca_tg_offest, &len);
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
			set_srm_no_wait(&ca_tg_srm);
			ca_tg_offest += len;
		} else {
			ca_tg_srm = 0;
			ca_tg_chunked_req = false;
			ca_tg_offest = 0;
		}
	}
	return err;
}

#endif /* CONFIG_BT_AVRCP_TG_COVER_ART */

#if defined(CONFIG_BT_AVRCP_CT_COVER_ART)
#define CA_CT_APP_PARAM_1                                                                          \
	"Application parameter 1 for AVRCP CT Cover Art Test. This parameter will be included"     \
	"in the GET request to force multiple packets during transmission. The length of this"     \
	"must fit within COVER_ART_MOPL to ensure proper fragmentation handling."
static struct bt_avrcp_cover_art_ct *ca_ct;
static uint32_t ca_ct_conn_id;
static uint8_t ca_ct_srm;
static bool ca_ct_chunked_req;
static uint8_t ca_ct_rsp_code;

static void ct_l2cap_connected(struct bt_avrcp_ct *ct, struct bt_avrcp_cover_art_ct *cover_art_ct)
{
	bt_shell_print("Cover Art CT L2CAP Connected: %p", cover_art_ct);
}

static void ct_l2cap_disconnected(struct bt_avrcp_cover_art_ct *ct)
{
	ca_ct = NULL;
	ca_ct_srm = 0;
	ca_ct_chunked_req = false;
	ca_ct_rsp_code = BT_OBEX_RSP_CODE_SUCCESS;
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

static int ct_parse_header_srm(struct net_buf *buf, uint8_t *srm_state)
{
	struct bt_obex_hdr hdr;
	uint8_t srm = 0xFF;
	int err;

	/* Skip if local SRM is not enabled or remote SRM is already enabled */
	if (!is_local_srm_enabled(*srm_state) || is_remote_srm_enabled(*srm_state)) {
		return 0;
	}

	hdr.id = BT_OBEX_HEADER_ID_SRM;
	hdr.len = 0;
	hdr.data = NULL;
	err = bt_obex_header_parse(buf, ca_find_header_cb, &hdr);
	if (err != 0) {
		return err;
	}

	if ((hdr.data != NULL) && (hdr.len == 1)) {
		srm = hdr.data[0];
	}

	if (srm == BT_OBEX_SRM_ENABLE) {
		*srm_state |= REMOTE_SRM_ENABLED;
	}

	return 0;
}

static int ct_parse_header_srm_param(struct net_buf *buf, uint8_t *srm_state)
{
	struct bt_obex_hdr hdr;
	uint8_t srmp = 0xFF;
	int err;

	/* Skip if SRM is not enabled or already in SRM no-wait mode */
	if (!is_srm_enabled(*srm_state) || is_srm_no_wait_enabled(*srm_state)) {
		return 0;
	}

	hdr.id = BT_OBEX_HEADER_ID_SRM_PARAM;
	hdr.len = 0;
	hdr.data = NULL;
	err = bt_obex_header_parse(buf, ca_find_header_cb, &hdr);
	if (err != 0) {
		return err;
	}

	if ((hdr.data != NULL) && (hdr.len == 1)) {
		srmp = hdr.data[0];
	}

	if (srmp == BT_OBEX_SRMP_WAIT) {
		*srm_state |= REMOTE_SRMP_ENABLED;
	} else {
		*srm_state &= ~REMOTE_SRMP_ENABLED;
	}

	set_srm_no_wait(srm_state);

	return 0;
}

static void ct_get_image_properties(struct bt_avrcp_cover_art_ct *ct, uint8_t rsp_code,
				    struct net_buf *buf)
{
	ca_ct_rsp_code = rsp_code;
	bt_shell_print("Cover Art CT %p get_image_properties rsp, rsp_code %s, data len %u", ct,
		       bt_obex_rsp_code_to_str(rsp_code), buf->len);
	cover_art_parse_headers(buf);

	if (rsp_code != BT_OBEX_RSP_CODE_CONTINUE) {
		ca_ct_srm = 0;
		ca_ct_chunked_req = false;
		return;
	}

	ct_parse_header_srm(buf, &ca_ct_srm);
	ct_parse_header_srm_param(buf, &ca_ct_srm);
}

static void ct_get_image(struct bt_avrcp_cover_art_ct *ct, uint8_t rsp_code, struct net_buf *buf)
{
	ca_ct_rsp_code = rsp_code;
	bt_shell_print("Cover Art CT %p get_image rsp, rsp_code %s, data len %u", ct,
		       bt_obex_rsp_code_to_str(rsp_code), buf->len);
	cover_art_parse_headers(buf);

	if (rsp_code != BT_OBEX_RSP_CODE_CONTINUE) {
		ca_ct_srm = 0;
		ca_ct_chunked_req = false;
		return;
	}

	ct_parse_header_srm(buf, &ca_ct_srm);
	ct_parse_header_srm_param(buf, &ca_ct_srm);
}

static void ct_get_linked_thumbnail(struct bt_avrcp_cover_art_ct *ct, uint8_t rsp_code,
				    struct net_buf *buf)
{
	ca_ct_rsp_code = rsp_code;
	bt_shell_print("Cover Art CT %p get_linked_thumbnail rsp, rsp_code %s, data len %u", ct,
		       bt_obex_rsp_code_to_str(rsp_code), buf->len);
	cover_art_parse_headers(buf);

	if (rsp_code != BT_OBEX_RSP_CODE_CONTINUE) {
		ca_ct_srm = 0;
		ca_ct_chunked_req = false;
		return;
	}

	ct_parse_header_srm(buf, &ca_ct_srm);
	ct_parse_header_srm_param(buf, &ca_ct_srm);
}

static void ct_abort(struct bt_avrcp_cover_art_ct *ct, uint8_t rsp_code, struct net_buf *buf)
{
	ca_ct_srm = 0;
	ca_ct_chunked_req = false;
	ca_ct_rsp_code = BT_OBEX_RSP_CODE_SUCCESS;
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
	struct bt_avrcp_cover_art_ct_cb *cb;
	int err;

	if ((argc > 1) && (!strcmp(argv[1], "NULL"))) {
		cb = NULL;
	} else {
		cb = &cover_art_ct_cb;
	}

	err = bt_avrcp_cover_art_ct_cb_register(cb);
	if (err != 0) {
		shell_error(sh, "Failed to reg cover art CT cb (err %d)", err);
	}

	return err;
}

static int cmd_ct_alloc_buf(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_buf_pool *pool;
	struct net_buf *buf;

	if ((argc > 1) && (!strcmp(argv[1], "NULL"))) {
		pool = NULL;
	} else {
		pool = &tx_pool;
	}

	buf = bt_avrcp_cover_art_ct_create_pdu(ca_ct, pool);
	if (buf == NULL) {
		shell_error(sh, "Fail to allocate tx buffer");
		return -ENOBUFS;
	} else {
		shell_print(sh, "Allocate tx buffer successfully: %p", buf);
		net_buf_unref(buf);
		return 0;
	}
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
	bool add_header = false;
	struct net_buf *buf = NULL;
	int err;

	if ((argc > 1) && (!strcmp(argv[1], "true"))) {
		add_header = true;
	}

	if (add_header) {
		struct bt_obex_tlv tlv[1];

		buf = bt_avrcp_cover_art_ct_create_pdu(ca_ct, &tx_pool);
		if (buf == NULL) {
			shell_error(sh, "Fail to allocate tx buffer");
			return -ENOBUFS;
		}

		tlv[0].type = USER_DEFINED_APP_PARAM_HDR;
		tlv[0].data_len = strlen(APP_PARAM_USED_IN_ABORT);
		tlv[0].data = (const uint8_t *)APP_PARAM_USED_IN_ABORT;
		err = bt_obex_add_header_app_param(buf, ARRAY_SIZE(tlv), tlv);
		if (err != 0) {
			net_buf_unref(buf);
			bt_shell_error("Failed to add application parameter header");
			return err;
		}
	}

	err = bt_avrcp_cover_art_ct_abort(ca_ct, buf);
	if (err != 0) {
		shell_error(sh, "OBEX abort failed (err %d)", err);
		if (buf != NULL) {
			net_buf_unref(buf);
		}
	}

	return err;
}

static int ct_parse_params(const struct shell *sh, size_t argc, char *argv[], bool *enable_chunk,
			   bool *enable_srm, bool *enable_srmp)
{
	int arg_idx = 1;

	/* Initialize output parameters */
	if (enable_chunk) {
		*enable_chunk = false;
	}
	if (enable_srm) {
		*enable_srm = false;
	}
	if (enable_srmp) {
		*enable_srmp = false;
	}

	/* Parse optional parameters */
	while (arg_idx < argc) {
		if (!strcmp(argv[arg_idx], "chunked_req")) {
			if (arg_idx + 1 >= argc) {
				shell_error(sh, "[true or false] is needed after 'chunked_req'");
				return -EINVAL;
			}
			arg_idx++;
			if (!strcmp(argv[arg_idx], "true")) {
				*enable_chunk = true;
			} else if (!strcmp(argv[arg_idx], "false")) {
				*enable_chunk = false;
			} else {
				shell_error(sh,
					    "Invalid chunked_req value: %s (expected true/false)",
					    argv[arg_idx]);
				return -EINVAL;
			}

			bt_shell_print("Chunk mode: %s", enable_chunk ? "enabled" : "disabled");
		} else if (!strcmp(argv[arg_idx], "srm")) {
			if (arg_idx + 1 >= argc) {
				shell_error(sh, "[true or false] is needed after 'srm'");
				return -EINVAL;
			}
			arg_idx++;
			if (!strcmp(argv[arg_idx], "true")) {
				*enable_srm = true;
			} else if (!strcmp(argv[arg_idx], "false")) {
				*enable_srm = false;
			} else {
				shell_error(sh, "Invalid srm value: %s (expected true/false)",
					    argv[arg_idx]);
				return -EINVAL;
			}
			bt_shell_print("SRM mode: %s", enable_srm ? "enabled" : "disabled");
		} else if (!strcmp(argv[arg_idx], "srmp")) {
			if (arg_idx + 1 >= argc) {
				shell_error(sh, "[true or false] is needed after 'srmp'");
				return -EINVAL;
			}
			arg_idx++;
			if (!strcmp(argv[arg_idx], "true")) {
				*enable_srmp = true;
			} else if (!strcmp(argv[arg_idx], "false")) {
				*enable_srmp = false;
			} else {
				shell_error(sh, "Invalid srmp value: %s (expected true/false)",
					    argv[arg_idx]);
				return -EINVAL;
			}
			bt_shell_print("SRMP wait: %s", enable_srmp ? "enabled" : "disabled");
		} else {
			shell_error(sh, "Unknown parameter: %s", argv[arg_idx]);
			return -EINVAL;
		}
		arg_idx++;
	}

	if (*enable_srmp && !*enable_srm) {
		shell_error(sh, "SRMP requires SRM to be enabled");
		return -EINVAL;
	}

	return 0;
}

static int ct_add_header_srm(struct net_buf *buf, uint8_t *srm_state, bool enable_srm)
{
	int err;

	/* Add SRM header if enabled */
	if (enable_srm) {
		err = bt_obex_add_header_srm(buf, BT_OBEX_SRM_ENABLE);
		if (err != 0) {
			return err;
		}
		*srm_state |= LOCAL_SRM_ENABLED;
	}

	return 0;
}

static int ct_add_header_srm_param(struct net_buf *buf, uint8_t *srm_state, uint8_t rsp_code,
				   bool enable_srmp)
{
	int err;

	/* Handle first packet with local SRM enabled */
	if ((rsp_code != BT_OBEX_RSP_CODE_CONTINUE) && (is_local_srm_enabled(*srm_state))) {
		if (enable_srmp) {
			err = bt_obex_add_header_srm_param(buf, BT_OBEX_SRMP_WAIT);
			if (err != 0) {
				return err;
			}
			*srm_state |= LOCAL_SRMP_ENABLED;
		}

		return 0;
	}

	/* Skip if SRM is not fully enabled or already in SRM no-wait mode */
	if (!is_srm_enabled(*srm_state) || is_srm_no_wait_enabled(*srm_state)) {
		return 0;
	}

	/* Handle subsequent packets - If SRM Enabled but waiting, add SRMP header if enabled */
	if (enable_srmp) {
		err = bt_obex_add_header_srm_param(buf, BT_OBEX_SRMP_WAIT);
		if (err != 0) {
			return err;
		}
		*srm_state |= LOCAL_SRMP_ENABLED;
	} else {
		*srm_state &= ~LOCAL_SRMP_ENABLED;
	}

	return 0;
}

#define IMAGE_HANDLE "\x00\x31\x00\x30\x00\x30\x00\x30\x00\x30\x00\x30\x00\x31\x00\x00"
#if defined(CONFIG_BT_AVRCP_CT_COVER_ART_GET_IMAGE_PROPERTIES)
static int cmd_ct_get_image_properties(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_buf *buf;
	bool enable_chunk = false;
	bool enable_srm = false;
	bool enable_srmp = false;
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

	err = ct_parse_params(sh, argc, argv, &enable_chunk, &enable_srm, &enable_srmp);
	if (err != 0) {
		net_buf_unref(buf);
		shell_help(sh);
		return SHELL_CMD_HELP_PRINTED;
	}

	err = ct_add_header_srm_param(buf, &ca_ct_srm, ca_ct_rsp_code, enable_srmp);
	if (err != 0) {
		shell_error(sh, "Fail to add SRMP header %d", err);
		goto failed;
	}

	/* Send next chunked request */
	if (ca_ct_chunked_req) {
		struct bt_obex_tlv tlv[1];

		tlv[0].type = USER_DEFINED_APP_PARAM_HDR;
		tlv[0].data_len = strlen(CA_CT_APP_PARAM_1);
		tlv[0].data = (const uint8_t *)CA_CT_APP_PARAM_1;
		err = bt_obex_add_header_app_param(buf, ARRAY_SIZE(tlv), tlv);
		if (err != 0) {
			bt_shell_error("Failed to add application parameter header");
			goto failed;
		}

		ca_ct_chunked_req = false;
		goto continue_req;
	}

	if (ca_ct_rsp_code == BT_OBEX_RSP_CODE_CONTINUE) {
		if (is_srm_no_wait_enabled(ca_ct_srm)) {
			/* SRM Enabled, don't send next request */
			return 0;
		} else {
			/* SRM Disabled or SRM Enabled but waiting, send next request with final bit
			 */
			goto continue_req;
		}
	}

	ca_ct_chunked_req = enable_chunk;

	err = bt_obex_add_header_conn_id(buf, ca_ct->client._conn_id);
	if (err != 0) {
		shell_error(sh, "Fail to add conn id header %d", err);
		goto failed;
	}

	err = ct_add_header_srm(buf, &ca_ct_srm, enable_srm);
	if (err != 0) {
		shell_error(sh, "Fail to add SRM header %d", err);
		goto failed;
	}

	err = ct_add_header_srm_param(buf, &ca_ct_srm, ca_ct_rsp_code, enable_srmp);
	if (err != 0) {
		shell_error(sh, "Fail to add SRMP header %d", err);
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
	err = bt_avrcp_cover_art_ct_get_image_properties(ca_ct, !ca_ct_chunked_req, buf);
	if (err == 0) {
		set_srm_no_wait(&ca_ct_srm);
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
	bool enable_chunk = false;
	bool enable_srm = false;
	bool enable_srmp = false;
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

	err = ct_parse_params(sh, argc, argv, &enable_chunk, &enable_srm, &enable_srmp);
	if (err != 0) {
		net_buf_unref(buf);
		shell_help(sh);
		return SHELL_CMD_HELP_PRINTED;
	}

	err = ct_add_header_srm_param(buf, &ca_ct_srm, ca_ct_rsp_code, enable_srmp);
	if (err != 0) {
		shell_error(sh, "Fail to add SRMP header %d", err);
		goto failed;
	}

	/* Send next chunked request */
	if (ca_ct_chunked_req) {
		struct bt_obex_tlv tlv[1];

		tlv[0].type = USER_DEFINED_APP_PARAM_HDR;
		tlv[0].data_len = strlen(CA_CT_APP_PARAM_1);
		tlv[0].data = (const uint8_t *)CA_CT_APP_PARAM_1;
		err = bt_obex_add_header_app_param(buf, ARRAY_SIZE(tlv), tlv);
		if (err != 0) {
			bt_shell_error("Failed to add application parameter header");
			goto failed;
		}

		ca_ct_chunked_req = false;
		goto continue_req;
	}

	if (ca_ct_rsp_code == BT_OBEX_RSP_CODE_CONTINUE) {
		if (is_srm_no_wait_enabled(ca_ct_srm)) {
			/* SRM Enabled, don't send next request */
			return 0;
		} else {
			/* SRM Disabled or SRM Enabled but waiting, send next request with final bit
			 */
			goto continue_req;
		}
	}

	ca_ct_chunked_req = enable_chunk;

	err = bt_obex_add_header_conn_id(buf, ca_ct->client._conn_id);
	if (err != 0) {
		shell_error(sh, "Fail to add conn id header %d", err);
		goto failed;
	}

	err = ct_add_header_srm(buf, &ca_ct_srm, enable_srm);
	if (err != 0) {
		shell_error(sh, "Fail to add SRM header %d", err);
		goto failed;
	}

	err = ct_add_header_srm_param(buf, &ca_ct_srm, ca_ct_rsp_code, enable_srmp);
	if (err != 0) {
		shell_error(sh, "Fail to add SRMP header %d", err);
		goto failed;
	}

	err = bt_obex_add_header_type(buf, sizeof(BT_BIP_HDR_TYPE_GET_IMAGE),
				      (uint8_t *)BT_BIP_HDR_TYPE_GET_IMAGE);
	if (err != 0) {
		shell_error(sh, "Fail to add type header %d", err);
		goto failed;
	}

	err = bt_bip_add_header_image_handle(buf, sizeof(IMAGE_HANDLE) - 1, (uint8_t *)IMAGE_HANDLE);
	if (err != 0) {
		shell_error(sh, "Fail to add image handle header %d", err);
		goto failed;
	}

	err = bt_bip_add_header_image_desc(buf, sizeof(IMAGE_DESC), (uint8_t *)IMAGE_DESC);
	if (err != 0) {
		shell_error(sh, "Fail to add image descriptor header %d", err);
		goto failed;
	}

continue_req:
	err = bt_avrcp_cover_art_ct_get_image(ca_ct, !ca_ct_chunked_req, buf);
	if (err == 0) {
		set_srm_no_wait(&ca_ct_srm);
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
	bool enable_chunk = false;
	bool enable_srm = false;
	bool enable_srmp = false;
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

	err = ct_parse_params(sh, argc, argv, &enable_chunk, &enable_srm, &enable_srmp);
	if (err != 0) {
		net_buf_unref(buf);
		shell_help(sh);
		return SHELL_CMD_HELP_PRINTED;
	}

	err = ct_add_header_srm_param(buf, &ca_ct_srm, ca_ct_rsp_code, enable_srmp);
	if (err != 0) {
		shell_error(sh, "Fail to add SRMP header %d", err);
		goto failed;
	}

	/* Send next chunked request */
	if (ca_ct_chunked_req) {
		struct bt_obex_tlv tlv[1];

		tlv[0].type = USER_DEFINED_APP_PARAM_HDR;
		tlv[0].data_len = strlen(CA_CT_APP_PARAM_1);
		tlv[0].data = (const uint8_t *)CA_CT_APP_PARAM_1;
		err = bt_obex_add_header_app_param(buf, ARRAY_SIZE(tlv), tlv);
		if (err != 0) {
			bt_shell_error("Failed to add application parameter header");
			goto failed;
		}

		ca_ct_chunked_req = false;
		goto continue_req;
	}

	if (ca_ct_rsp_code == BT_OBEX_RSP_CODE_CONTINUE) {
		if (is_srm_no_wait_enabled(ca_ct_srm)) {
			/* SRM Enabled, don't send next request */
			return 0;
		} else {
			/* SRM Disabled or SRM Enabled but waiting, send next request with final bit
			 */
			goto continue_req;
		}
	}

	ca_ct_chunked_req = enable_chunk;

	err = bt_obex_add_header_conn_id(buf, ca_ct->client._conn_id);
	if (err != 0) {
		shell_error(sh, "Fail to add conn id header %d", err);
		goto failed;
	}

	err = ct_add_header_srm(buf, &ca_ct_srm, enable_srm);
	if (err != 0) {
		shell_error(sh, "Fail to add SRM header %d", err);
		goto failed;
	}

	err = ct_add_header_srm_param(buf, &ca_ct_srm, ca_ct_rsp_code, enable_srmp);
	if (err != 0) {
		shell_error(sh, "Fail to add SRMP header %d", err);
		goto failed;
	}

	err = bt_obex_add_header_type(buf, sizeof(BT_BIP_HDR_TYPE_GET_LINKED_THUMBNAIL),
				      (uint8_t *)BT_BIP_HDR_TYPE_GET_LINKED_THUMBNAIL);
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
	err = bt_avrcp_cover_art_ct_get_linked_thumbnail(ca_ct, !ca_ct_chunked_req, buf);
	if (err == 0) {
		set_srm_no_wait(&ca_ct_srm);
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
#define RSP_HELP_2 "<rsp: noerror, error> [rsp_code] [srm <true|false>] [srmp <true|false>]"
#define RSP_HELP_3 "<rsp: continue, success, error> [rsp_code] [add_header: true, false]"

SHELL_STATIC_SUBCMD_SET_CREATE(
	tg_cmds, SHELL_CMD_ARG(reg, NULL, "[NULL]", cmd_tg_reg, 1, 1),
	SHELL_CMD_ARG(alloc_buf, NULL, "[NULL]", cmd_tg_alloc_buf, 1, 1),
	SHELL_CMD_ARG(l2cap_disconnect, NULL, HELP_NONE, cmd_tg_l2cap_disconnect, 1, 0),
	SHELL_CMD_ARG(connect, NULL, RSP_HELP_1, cmd_tg_connect, 2, 1),
	SHELL_CMD_ARG(disconnect, NULL, RSP_HELP_1, cmd_tg_disconnect, 2, 1),
	SHELL_CMD_ARG(abort, NULL, RSP_HELP_3, cmd_tg_abort, 2, 2),
	SHELL_CMD_ARG(get_image_properties, NULL, RSP_HELP_2, cmd_tg_get_image_properties, 2, 5),
	SHELL_CMD_ARG(get_image, NULL, RSP_HELP_2, cmd_tg_get_image, 2, 5),
	SHELL_CMD_ARG(get_linked_thumbnail, NULL, RSP_HELP_2, cmd_tg_get_linked_thumbnail, 2, 5),
	SHELL_SUBCMD_SET_END);
#endif /* CONFIG_BT_AVRCP_TG_COVER_ART */

#if defined(CONFIG_BT_AVRCP_CT_COVER_ART)
#define REQ_HELP_1 "[chunked_req <true|false>] [srm <true|false>] [srmp <true|false>]"
#define REQ_HELP_2 "[add_header: true, false]"

SHELL_STATIC_SUBCMD_SET_CREATE(
	ct_cmds, SHELL_CMD_ARG(reg, NULL, "[NULL]", cmd_ct_reg, 1, 1),
	SHELL_CMD_ARG(alloc_buf, NULL, "[NULL]", cmd_ct_alloc_buf, 1, 1),
	SHELL_CMD_ARG(l2cap_connect, NULL, "<psm>", cmd_ct_l2cap_connect, 2, 0),
	SHELL_CMD_ARG(l2cap_disconnect, NULL, HELP_NONE, cmd_ct_l2cap_disconnect, 1, 0),
	SHELL_CMD_ARG(connect, NULL, HELP_NONE, cmd_ct_connect, 1, 0),
	SHELL_CMD_ARG(disconnect, NULL, HELP_NONE, cmd_ct_disconnect, 1, 0),
	SHELL_CMD_ARG(abort, NULL, REQ_HELP_2, cmd_ct_abort, 1, 1),
#if defined(CONFIG_BT_AVRCP_CT_COVER_ART_GET_IMAGE_PROPERTIES)
	SHELL_CMD_ARG(get_image_properties, NULL, REQ_HELP_1, cmd_ct_get_image_properties, 1, 6),
#endif /* CONFIG_BT_AVRCP_CT_COVER_ART_GET_IMAGE_PROPERTIES */
#if defined(CONFIG_BT_AVRCP_CT_COVER_ART_GET_IMAGE)
	SHELL_CMD_ARG(get_image, NULL, REQ_HELP_1, cmd_ct_get_image, 1, 6),
#endif /* CONFIG_BT_AVRCP_CT_COVER_ART_GET_IMAGE */
#if defined(CONFIG_BT_AVRCP_CT_COVER_ART_GET_LINKED_THUMBNAIL)
	SHELL_CMD_ARG(get_linked_thumbnail, NULL, REQ_HELP_1, cmd_ct_get_linked_thumbnail, 1, 6),
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
