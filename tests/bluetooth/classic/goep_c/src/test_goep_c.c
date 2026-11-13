/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 * pylint: disable=duplicate-code
 */

#include <errno.h>
#include <zephyr/types.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <psa/crypto.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/kernel.h>

#include <zephyr/settings/settings.h>

#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/bluetooth/classic/rfcomm.h>
#include <zephyr/bluetooth/classic/sdp.h>
#include <zephyr/bluetooth/classic/goep.h>

#include <zephyr/shell/shell.h>

#include "host/shell/bt.h"
#include "common/bt_shell_private.h"

#define HELP_NONE "[none]"

struct bt_goep_app {
	struct bt_goep_transport goep_transport;
	struct bt_goep goep;
	struct bt_obex_client client;
	struct bt_obex_server server;
	struct bt_conn *conn;
	struct net_buf *tx_buf;
};

#define STRING_LEN_MAX  20
#define TLV_COUNT       5
#define TLV_BUFFER_SIZE 64
#define PWD_MAX_LENGTH  16
#define GOEP_SERVER_PWD '0000'

extern struct bt_goep_app goep_app;
extern struct bt_obex_tlv tlvs[TLV_COUNT];
extern uint8_t tlv_buffers[TLV_COUNT][TLV_BUFFER_SIZE];
extern uint8_t tlv_count;
extern struct bt_obex_server_ops goep_server_ops;
extern struct bt_obex_client_ops goep_client_ops;

struct bt_goep_app_pwd {
	uint8_t pwd[PWD_MAX_LENGTH];
	uint8_t auth_challenge[16U];
	struct bt_goep_app *goep_app;
};

static struct bt_goep_app_pwd goep_app_pwd;

struct header_user_data {
	uint8_t id;
	uint8_t tlv_tag;
	uint8_t tlv_len;
	uint8_t *tlv_data;
	bool found;
};

static int bt_goep_generate_auth_challenage(const uint8_t *pwd, uint8_t *auth_chal_req)
{
	int64_t nowtime = k_uptime_get();
	uint8_t h[PWD_MAX_LENGTH + 1U + sizeof(int64_t)] = {0};
	size_t len;
	uint16_t pwd_len;

	if (!pwd) {
		return -EINVAL;
	}

	if (!auth_chal_req) {
		return -EINVAL;
	}
	pwd_len = strlen((char *)pwd);
	memcpy(h, &nowtime, 8U);
	h[8U] = ':';
	memcpy(h + sizeof(int64_t) + 1U, pwd, strlen((char *)pwd));
	psa_hash_compute(PSA_ALG_MD5, (const unsigned char *)h, sizeof(int64_t) + 1U + pwd_len,
			 auth_chal_req, 16U, &len);
	return 0;
}

static int bt_goep_generate_auth_response(const uint8_t *pwd, uint8_t *auth_chal_req,
					  uint8_t *auth_chal_rsp)
{
	uint8_t h[PWD_MAX_LENGTH + 16U + 1U] = {0};
	size_t len;
	uint16_t pwd_len;

	if (!pwd) {
		bt_shell_print("no available password");
		return -EINVAL;
	}

	if (!auth_chal_req) {
		bt_shell_print("no available auth_chal_req");
		return -EINVAL;
	}

	if (!auth_chal_rsp) {
		bt_shell_print("no available auth_chal_rsp");
		return -EINVAL;
	}
	pwd_len = strlen((char *)pwd);
	memcpy(h, auth_chal_req, 16U);
	h[16U] = ':';
	memcpy(h + 17U, pwd, pwd_len);
	psa_hash_compute(PSA_ALG_MD5, (const unsigned char *)h, 17U + pwd_len, auth_chal_rsp, 16U,
			 &len);
	return 0;
}

static int bt_goep_verify_auth(uint8_t *chal, uint8_t *rsp, const uint8_t *pwd)
{
	uint8_t result[16U] = {0};

	bt_goep_generate_auth_response(pwd, chal, (uint8_t *)&result);

	return memcmp(result, (const uint8_t *)rsp, 16U);
}

static int cmd_add_header_auth_challenge(const struct shell *sh, size_t argc, char *argv[])
{
	int err;
	uint8_t auth_chal_req[16U] = {0};

	if (strlen(argv[1]) > PWD_MAX_LENGTH) {
		bt_shell_print("the length of password is too long");
		return -EINVAL;
	}

	if (tlv_count >= (uint8_t)ARRAY_SIZE(tlvs)) {
		bt_shell_print("No space of TLV array, add auth_challenge and clear tlvs");
		goto add_header;
	}

	bt_goep_generate_auth_challenage((uint8_t *)argv[1], auth_chal_req);
	memcpy(&tlv_buffers[tlv_count][0], auth_chal_req, 16);

	tlvs[tlv_count].type = 0x00;
	tlvs[tlv_count].data_len = 16U;
	tlvs[tlv_count].data = &tlv_buffers[tlv_count][0];

	tlv_count++;

add_header:
	err = bt_obex_add_header_auth_challenge(goep_app.tx_buf, (size_t)tlv_count, tlvs);
	if (err) {
		shell_error(sh, "Fail to add header auth_challenge");
	}
	goep_app_pwd.goep_app = &goep_app;
	strncpy((char *)goep_app_pwd.pwd, (char *)argv[1], strlen((char *)argv[1]));
	memcpy(goep_app_pwd.auth_challenge, auth_chal_req, 16U);
	tlv_count = 0;
	return err;
}

static int cmd_add_header_auth_rsp(const struct shell *sh, size_t argc, char *argv[])
{
	int err;
	uint8_t auth_challenage_rsp[16U] = {0};

	if (tlv_count >= (uint8_t)ARRAY_SIZE(tlvs)) {
		shell_warn(sh, "No space of TLV array, add auth_rsp and clear tlvs");
		goto add_header;
	}

	bt_goep_generate_auth_response((uint8_t *)argv[1], goep_app_pwd.auth_challenge,
				       auth_challenage_rsp);
	memcpy(&tlv_buffers[tlv_count][0], auth_challenage_rsp, 16);

	tlvs[tlv_count].type = 0x00;
	tlvs[tlv_count].data_len = 16U;
	tlvs[tlv_count].data = &tlv_buffers[tlv_count][0];

	tlv_count++;

add_header:
	err = bt_obex_add_header_auth_rsp(goep_app.tx_buf, (size_t)tlv_count, tlvs);
	if (err) {
		shell_error(sh, "Fail to add header auth_rsp");
	}
	tlv_count = 0;
	return err;
}

static bool goep_parse_tlvs_cb(struct bt_obex_tlv *tlv, void *user_data)
{
	struct header_user_data *parse_data = (struct header_user_data *)user_data;

	if (tlv->type == parse_data->tlv_tag) {
		memcpy((void *)parse_data->tlv_data, (void *)tlv->data, tlv->data_len);
		parse_data->found = true;
	}

	return true;
}

static bool goep_parse_headers_cb(struct bt_obex_hdr *hdr, void *user_data)
{
	struct header_user_data *parse_data = (struct header_user_data *)user_data;

	switch (hdr->id) {
	case BT_OBEX_HEADER_ID_APP_PARAM:
	case BT_OBEX_HEADER_ID_AUTH_CHALLENGE:
	case BT_OBEX_HEADER_ID_AUTH_RSP: {
		int err;

		if (hdr->id == parse_data->id) {
			err = bt_obex_tlv_parse(hdr->len, hdr->data, goep_parse_tlvs_cb, user_data);
			if (err) {
				bt_shell_error("Fail to parse OBEX TLV triplet");
			}
		}
	} break;
	case BT_OBEX_HEADER_ID_BODY:
	case BT_OBEX_HEADER_ID_END_BODY:
	case BT_OBEX_HEADER_ID_DEST_NAME:
	case BT_OBEX_HEADER_ID_NAME: {
		if (hdr->id == parse_data->id) {
			memcpy((void *)parse_data->tlv_data, (void *)hdr->data, hdr->len);
			parse_data->tlv_len = hdr->len;
			parse_data->found = true;
		}
	} break;
	case BT_OBEX_HEADER_ID_SRM:
	case BT_OBEX_HEADER_ID_SRM_PARAM: {
		if (hdr->id == parse_data->id) {
			memcpy((void *)parse_data->tlv_data, (void *)hdr->data, hdr->len);
			parse_data->found = true;
		}
	} break;
	case BT_OBEX_HEADER_ID_ACTION_ID: {
		if (hdr->id == parse_data->id) {
			memcpy((void *)parse_data->tlv_data, (void *)hdr->data, 1U);
			parse_data->found = true;
		}
	} break;
	case BT_OBEX_HEADER_ID_CONN_ID: {
		if (hdr->id == parse_data->id) {
			memcpy((void *)parse_data->tlv_data, (void *)hdr->data, hdr->len);
			parse_data->found = true;
		}
	} break;
	default:
		bt_shell_hexdump(hdr->data, hdr->len);
		parse_data->found = false;
		break;
	}

	return true;
}

static int goep_parse_headers(struct net_buf *buf, void *user_data)
{
	int err;

	if (!buf) {
		return 0;
	}

	err = bt_obex_header_parse(buf, goep_parse_headers_cb, user_data);
	if (err) {
		bt_shell_print("Fail to parse OBEX Headers");
	}

	return err;
}

static void goep_server_connect(struct bt_obex_server *server, uint8_t version, uint16_t mopl,
				struct net_buf *buf)
{
	bt_shell_print("OBEX server %p conn req, version %02x, mopl %04x", server, version, mopl);
	uint8_t auth_challenage_rsp[16] = {0};
	struct header_user_data user_data;

	user_data.tlv_data = goep_app_pwd.auth_challenge;
	user_data.id = BT_OBEX_HEADER_ID_AUTH_CHALLENGE;
	user_data.tlv_tag = 0x00;
	user_data.found = false;

	goep_parse_headers(buf, &user_data);
	if (user_data.found == true) {
		bt_shell_print("Get auth challenge header, should add auth rsp");
	}

	user_data.tlv_data = auth_challenage_rsp;
	user_data.id = BT_OBEX_HEADER_ID_AUTH_RSP;
	user_data.tlv_tag = 0x00;
	user_data.found = false;
	goep_parse_headers(buf, &user_data);
	if (user_data.found == true) {
		bt_shell_print("Get auth challenge response");
		if (bt_goep_verify_auth(goep_app_pwd.auth_challenge, auth_challenage_rsp,
					goep_app_pwd.pwd) == 0) {
			bt_shell_print("Authentication successful");
		} else {
			bt_shell_print("Authentication failed");
			bt_obex_disconnect(&goep_app.client, goep_app.tx_buf);
		}
	}
}

static void goep_server_disconnect(struct bt_obex_server *server, struct net_buf *buf)
{
	bt_shell_print("OBEX server %p disconn req", server);
	uint8_t rsp[16] = {0};
	struct header_user_data user_data;

	user_data.tlv_data = rsp;
	user_data.id = BT_OBEX_HEADER_ID_CONN_ID;
	user_data.found = false;
	goep_parse_headers(buf, &user_data);
	if (user_data.found == true) {
		bt_shell_print("connection id : %d", sys_get_be32(rsp));
	}
}

static void goep_unicode_to_ascii(uint8_t *des, uint8_t *src, int len)
{
	int i = 0;

	if ((src == NULL) || (des == NULL)) {
		return;
	}

	while (i < len / 2) {
		des[i] = src[i * 2] != 0 ? src[i * 2] : src[i * 2 + 1];
		i += 1U;
	}

	des[i] = 0x00U; /* terminate with  0x00 */
}

static void goep_server_put(struct bt_obex_server *server, bool final, struct net_buf *buf)
{
	bt_shell_print("OBEX server %p put req, final %s, data len %d", server,
		       final ? "true" : "false", buf->len);
	uint8_t data[STRING_LEN_MAX] = {0};
	struct header_user_data user_data;

	user_data.tlv_data = data;
	user_data.id = BT_OBEX_HEADER_ID_BODY;
	user_data.found = false;
	goep_parse_headers(buf, &user_data);
	if (user_data.found == true) {
		bt_shell_print("body : %s", user_data.tlv_data);
	}

	user_data.id = BT_OBEX_HEADER_ID_END_BODY;
	user_data.found = false;
	memset(data, 0, sizeof(data));
	goep_parse_headers(buf, &user_data);
	if (user_data.found == true) {
		bt_shell_print("end of body : %s", user_data.tlv_data);
	}

	user_data.id = BT_OBEX_HEADER_ID_SRM;
	user_data.found = false;
	memset(data, 0, sizeof(data));
	goep_parse_headers(buf, &user_data);
	if (user_data.found == true) {
		bt_shell_print("srm : %d", user_data.tlv_data[0]);
	}

	user_data.id = BT_OBEX_HEADER_ID_SRM_PARAM;
	user_data.found = false;
	memset(data, 0, sizeof(data));
	goep_parse_headers(buf, &user_data);
	if (user_data.found == true) {
		bt_shell_print("srmp : %d", user_data.tlv_data[0]);
	}
}

static void goep_server_get(struct bt_obex_server *server, bool final, struct net_buf *buf)
{
	bt_shell_print("OBEX server %p get req, final %s, data len %d", server,
		       final ? "true" : "false", buf->len);

	uint8_t data[STRING_LEN_MAX] = {0};
	struct header_user_data user_data;

	user_data.tlv_data = data;
	user_data.id = BT_OBEX_HEADER_ID_SRM;
	user_data.found = false;
	memset(data, 0, sizeof(data));
	goep_parse_headers(buf, &user_data);
	if (user_data.found == true) {
		bt_shell_print("srm : %d", user_data.tlv_data[0]);
	}

	user_data.id = BT_OBEX_HEADER_ID_SRM_PARAM;
	user_data.found = false;
	memset(data, 0, sizeof(data));
	goep_parse_headers(buf, &user_data);
	if (user_data.found == true) {
		bt_shell_print("srmp : %d", user_data.tlv_data[0]);
	}
}

static void goep_server_action(struct bt_obex_server *server, bool final, struct net_buf *buf)
{
	bt_shell_print("OBEX server %p action req, final %s, data len %d", server,
		       final ? "true" : "false", buf->len);

	uint8_t data[STRING_LEN_MAX] = {0};
	uint8_t ascii_data[STRING_LEN_MAX / 2 + 1] = {0};
	struct header_user_data user_data;

	user_data.tlv_data = data;
	user_data.id = BT_OBEX_HEADER_ID_ACTION_ID;
	user_data.found = false;
	memset(data, 0, sizeof(data));
	goep_parse_headers(buf, &user_data);
	if (user_data.found == true) {
		bt_shell_print("action_id : %d", user_data.tlv_data[0]);
	}

	user_data.tlv_data = data;
	user_data.id = BT_OBEX_HEADER_ID_NAME;
	user_data.found = false;
	memset(data, 0, sizeof(data));
	memset(ascii_data, 0, sizeof(ascii_data));
	goep_parse_headers(buf, &user_data);
	if (user_data.found == true) {
		goep_unicode_to_ascii(ascii_data, data, user_data.tlv_len);
		bt_shell_print("name : %s", ascii_data);
	}

	user_data.tlv_data = data;
	user_data.id = BT_OBEX_HEADER_ID_DEST_NAME;
	user_data.found = false;
	memset(data, 0, sizeof(data));
	memset(ascii_data, 0, sizeof(ascii_data));
	goep_parse_headers(buf, &user_data);
	if (user_data.found == true) {
		goep_unicode_to_ascii(ascii_data, data, user_data.tlv_len);
		bt_shell_print("dest name : %s", ascii_data);
	}

	user_data.id = BT_OBEX_HEADER_ID_SRM;
	user_data.found = false;
	memset(data, 0, sizeof(data));
	goep_parse_headers(buf, &user_data);
	if (user_data.found == true) {
		bt_shell_print("srm : %d", user_data.tlv_data[0]);
	}

	user_data.id = BT_OBEX_HEADER_ID_SRM_PARAM;
	user_data.found = false;
	memset(data, 0, sizeof(data));
	goep_parse_headers(buf, &user_data);
	if (user_data.found == true) {
		bt_shell_print("srmp : %d", user_data.tlv_data[0]);
	}
}

static void goep_server_setpath(struct bt_obex_server *server, uint8_t flags, struct net_buf *buf)
{
	bt_shell_print("OBEX server %p setpath req, flags %d, data len %d", server, flags,
		       buf->len);

	uint8_t data[STRING_LEN_MAX] = {0};
	uint8_t ascii_data[STRING_LEN_MAX / 2 + 1] = {0};
	struct header_user_data user_data;

	user_data.tlv_data = data;
	user_data.id = BT_OBEX_HEADER_ID_NAME;
	user_data.found = false;
	memset(data, 0, sizeof(data));
	memset(ascii_data, 0, sizeof(ascii_data));
	goep_parse_headers(buf, &user_data);
	if (flags == BIT(1)) {
		if (user_data.found == true) {
			goep_unicode_to_ascii(ascii_data, data, user_data.tlv_len);
			bt_shell_print("set path to child folder : %s", ascii_data);
		} else {
			bt_shell_print("set path to root");
		}
	} else if (flags & BIT(0)) {
		bt_shell_print("set path to parent folder");
	}
}

static void goep_client_put(struct bt_obex_client *client, uint8_t rsp_code, struct net_buf *buf)
{
	bt_shell_print("OBEX client %p put rsp, rsp_code %s, data len %d", client,
		       bt_obex_rsp_code_to_str(rsp_code), buf->len);

	uint8_t data[STRING_LEN_MAX] = {0};
	struct header_user_data user_data;

	user_data.tlv_data = data;
	user_data.id = BT_OBEX_HEADER_ID_SRM;
	user_data.found = false;
	memset(data, 0, sizeof(data));
	goep_parse_headers(buf, &user_data);
	if (user_data.found == true) {
		bt_shell_print("srm : %d", user_data.tlv_data[0]);
	}

	user_data.id = BT_OBEX_HEADER_ID_SRM_PARAM;
	user_data.found = false;
	memset(data, 0, sizeof(data));
	goep_parse_headers(buf, &user_data);
	if (user_data.found == true) {
		bt_shell_print("srmp : %d", user_data.tlv_data[0]);
	}
}

static void goep_client_get(struct bt_obex_client *client, uint8_t rsp_code, struct net_buf *buf)
{
	bt_shell_print("OBEX client %p get rsp, rsp_code %s, data len %d", client,
		       bt_obex_rsp_code_to_str(rsp_code), buf->len);
	uint8_t data[STRING_LEN_MAX] = {0};
	struct header_user_data user_data;

	user_data.tlv_data = data;
	user_data.id = BT_OBEX_HEADER_ID_BODY;
	user_data.found = false;
	goep_parse_headers(buf, &user_data);
	if (user_data.found == true) {
		bt_shell_print("body : %s", user_data.tlv_data);
	}

	user_data.id = BT_OBEX_HEADER_ID_END_BODY;
	user_data.found = false;
	memset(data, 0, sizeof(data));
	goep_parse_headers(buf, &user_data);
	if (user_data.found == true) {
		bt_shell_print("end of body : %s", user_data.tlv_data);
	}

	user_data.id = BT_OBEX_HEADER_ID_SRM;
	user_data.found = false;
	memset(data, 0, sizeof(data));
	goep_parse_headers(buf, &user_data);
	if (user_data.found == true) {
		bt_shell_print("srm : %d", user_data.tlv_data[0]);
	}

	user_data.id = BT_OBEX_HEADER_ID_SRM_PARAM;
	user_data.found = false;
	memset(data, 0, sizeof(data));
	goep_parse_headers(buf, &user_data);
	if (user_data.found == true) {
		bt_shell_print("srmp : %d", user_data.tlv_data[0]);
	}
}

static void goep_client_action(struct bt_obex_client *client, uint8_t rsp_code, struct net_buf *buf)
{
	bt_shell_print("OBEX client %p action rsp, rsp_code %s, data len %d", client,
		       bt_obex_rsp_code_to_str(rsp_code), buf->len);
	uint8_t data[STRING_LEN_MAX] = {0};
	struct header_user_data user_data;

	user_data.tlv_data = data;
	user_data.id = BT_OBEX_HEADER_ID_SRM;
	user_data.found = false;
	memset(data, 0, sizeof(data));
	goep_parse_headers(buf, &user_data);
	if (user_data.found == true) {
		bt_shell_print("srm : %d", user_data.tlv_data[0]);
	}

	user_data.id = BT_OBEX_HEADER_ID_SRM_PARAM;
	user_data.found = false;
	memset(data, 0, sizeof(data));
	goep_parse_headers(buf, &user_data);
	if (user_data.found == true) {
		bt_shell_print("srmp : %d", user_data.tlv_data[0]);
	}
}

static void goep_client_connect(struct bt_obex_client *client, uint8_t rsp_code, uint8_t version,
				uint16_t mopl, struct net_buf *buf)
{
	bt_shell_print("OBEX client %p conn rsp, rsp_code %s, version %02x, mopl %04x", client,
		       bt_obex_rsp_code_to_str(rsp_code), version, mopl);

	uint8_t rsp[16] = {0};
	struct header_user_data user_data;

	if (rsp_code == BT_OBEX_RSP_CODE_UNAUTH) {
		user_data.tlv_data = goep_app_pwd.auth_challenge;
		user_data.id = BT_OBEX_HEADER_ID_AUTH_CHALLENGE;
		user_data.tlv_tag = 0x00;
		user_data.found = false;

		goep_parse_headers(buf, &user_data);
		if (user_data.found == true) {
			bt_shell_print("Get auth challenge header, should add auth rsp header");
		}
	} else if (rsp_code == BT_OBEX_RSP_CODE_SUCCESS) {
		user_data.tlv_data = rsp;
		user_data.id = BT_OBEX_HEADER_ID_AUTH_RSP;
		user_data.tlv_tag = 0x00;
		user_data.found = false;

		goep_parse_headers(buf, &user_data);
		if (user_data.found == true) {
			bt_shell_print("Get auth challenge response");
			if (bt_goep_verify_auth(goep_app_pwd.auth_challenge, rsp,
						goep_app_pwd.pwd) == 0) {
				bt_shell_print("Authentication successful");
			} else {
				bt_shell_print("Authentication failed");
				bt_obex_disconnect(&goep_app.client, goep_app.tx_buf);
			}
		}

		user_data.tlv_data = rsp;
		user_data.id = BT_OBEX_HEADER_ID_CONN_ID;
		user_data.found = false;
		goep_parse_headers(buf, &user_data);
		if (user_data.found == true) {
			bt_shell_print("connection id : %d", sys_get_be32(rsp));
		}
	}
}

static int cmd_release_goep_app(const struct shell *sh, size_t argc, char *argv[])
{
	goep_app.conn = NULL;
	memset(&goep_app, 0, sizeof(goep_app));
	return 0;
}

static int cmd_register_callback(const struct shell *sh, size_t argc, char *argv[])
{
	goep_server_ops.connect = goep_server_connect;
	goep_server_ops.put = goep_server_put;
	goep_server_ops.get = goep_server_get;
	goep_server_ops.action = goep_server_action;
	goep_server_ops.setpath = goep_server_setpath;
	goep_server_ops.disconnect = goep_server_disconnect;

	goep_client_ops.connect = goep_client_connect;
	goep_client_ops.put = goep_client_put;
	goep_client_ops.get = goep_client_get;
	goep_client_ops.action = goep_client_action;
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	goep_c_cmds, SHELL_CMD_ARG(register_callback, NULL, "", cmd_register_callback, 1, 0),
	SHELL_CMD_ARG(add_auth_req, NULL, "obex add header auth challenge <password>",
		      cmd_add_header_auth_challenge, 2, 0),
	SHELL_CMD_ARG(add_auth_rsp, NULL, "obex add header auth challenge response <password>",
		      cmd_add_header_auth_rsp, 2, 0),
	SHELL_CMD_ARG(release_goep_app, NULL, "", cmd_release_goep_app, 1, 0),
	SHELL_SUBCMD_SET_END);

static int cmd_goep_c(const struct shell *sh, size_t argc, char **argv)
{
	if (argc == 1) {
		shell_help(sh);
		/* sh returns 1 when help is printed */
		return 1;
	}

	shell_error(sh, "%s unknown parameter: %s", argv[0], argv[1]);

	return -ENOEXEC;
}

SHELL_CMD_ARG_REGISTER(goep_c_test, &goep_c_cmds, "Bluetooth test goep client test sh commands",
		       cmd_goep_c, 1, 1);
