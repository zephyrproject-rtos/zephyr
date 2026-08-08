/** @file
 * @brief Bluetooth FTP shell module
 *
 * Provide some Bluetooth shell commands that can be useful to applications.
 */
/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/types.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/bluetooth/classic/rfcomm.h>
#include <zephyr/bluetooth/classic/sdp.h>
#include <zephyr/bluetooth/classic/ftp.h>

#include <zephyr/shell/shell.h>

#include "host/shell/bt.h"
#include "common/bt_shell_private.h"

#define FTP_MOPL         CONFIG_BT_GOEP_RFCOMM_MTU
#define FTP_NAME_MAX_LEN 64

#define LOCAL_AUTH_ENABLED BIT(0)
#define PEER_AUTH_ENABLED  BIT(1)

/* Sample file body used for push_file/pull_file testing. */
#define FTP_FILE_BODY                                                                              \
	"This is a sample FTP file for Bluetooth FTP shell test.\r\n"                              \
	"Line 2: testing multi-packet OBEX fragmentation.\r\n"                                     \
	"Line 3: special chars @#$%&*()_+-=[]{}|;:',.<>?/\r\n"                                     \
	"Line 4: 0123456789\r\n"                                                                   \
	"Line 5: ABCDEFGHIJKLMNOPQRSTUVWXYZ abcdefghijklmnopqrstuvwxyz\r\n"                        \
	"Line 6: This module provides interactive shell commands for testing Bluetooth FTP.\r\n"   \
	"Line 7: Profile (FTP) functionality over OBEX/GOEP protocol, supports FTP client.\r\n"    \
	"Line 8: and server operations, including file transfer, folder navigation.\r\n"           \
	"Line 9: and advanced features like Single Response Mode (SRM) for throughput.\r\n"        \
	"Line 10: Key features:\r\n"                                                               \
	"Line 11: - Client/Server role support via RFCOMM and L2CAP\r\n"                           \
	"Line 12: - File operations: push, pull, delete, rename, move, copy\r\n"                   \
	"Line 13: - Folder operations: navigate, create, list\r\n"                                 \
	"Line 14: - OBEX authentication with password protection\r\n"                              \
	"Line 15: - Multi-packet fragmentation testing\r\n"                                        \
	"Line 16: - SRM and SRMP support for GOEP v2\r\n"                                          \
	"End of file.\r\n"

#if defined(CONFIG_BT_FTP_CLIENT)
NET_BUF_POOL_FIXED_DEFINE(ftp_client_tx_pool, CONFIG_BT_MAX_CONN, BT_RFCOMM_BUF_SIZE(FTP_MOPL),
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);
#endif /* CONFIG_BT_FTP_CLIENT */

#if defined(CONFIG_BT_FTP_SERVER)
NET_BUF_POOL_FIXED_DEFINE(ftp_server_tx_pool, CONFIG_BT_MAX_CONN, BT_RFCOMM_BUF_SIZE(FTP_MOPL),
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);
#endif /* CONFIG_BT_FTP_SERVER */

#define BT_OBEX_SRM_ENABLE 0x01
#define BT_OBEX_SRMP_WAIT  0x01

static int add_header_srm(struct net_buf *buf, bool enable_srm)
{
	int err;

	if (enable_srm) {
		err = bt_obex_add_header_srm(buf, BT_OBEX_SRM_ENABLE);
		if (err != 0) {
			return err;
		}
	}

	return 0;
}

static int add_header_srm_param(struct net_buf *buf, bool enable_srmp)
{
	int err;

	if (enable_srmp) {
		err = bt_obex_add_header_srm_param(buf, BT_OBEX_SRMP_WAIT);
		if (err != 0) {
			return err;
		}
	}

	return 0;
}

static bool ftp_parse_headers_cb(struct bt_obex_hdr *hdr, void *user_data)
{
	ARG_UNUSED(user_data);

	bt_shell_print("HI 0x%02x Len %d", hdr->id, hdr->len);

	switch (hdr->id) {
	case BT_OBEX_HEADER_ID_CONN_ID:
		if (hdr->len == 4) {
			bt_shell_print("Conn ID: 0x%08x", sys_get_be32(hdr->data));
		} else {
			bt_shell_hexdump(hdr->data, hdr->len);
		}
		break;
	case BT_OBEX_HEADER_ID_SRM:
		if (hdr->len == 1) {
			bt_shell_print("OBEX SRM: 0x%02x", hdr->data[0]);
		} else {
			bt_shell_hexdump(hdr->data, hdr->len);
		}
		break;
	case BT_OBEX_HEADER_ID_SRM_PARAM:
		if (hdr->len == 1) {
			bt_shell_print("OBEX SRMP: 0x%02x", hdr->data[0]);
		} else {
			bt_shell_hexdump(hdr->data, hdr->len);
		}
		break;
	default:
		bt_shell_hexdump(hdr->data, hdr->len);
		break;
	}

	return true;
}

static int ftp_parse_headers(struct net_buf *buf)
{
	int err;

	if (buf == NULL) {
		return 0;
	}

	err = bt_obex_header_parse(buf, ftp_parse_headers_cb, NULL);
	if (err != 0) {
		bt_shell_warn("Fail to parse FTP headers (err %d)", err);
	}

	return err;
}

static int parse_srm(const struct shell *sh, size_t argc, char *argv[], bool *enable_srm)
{
	ARG_UNUSED(sh);

	for (size_t i = 1; i < argc; i++) {
		if (strcmp(argv[i], "srm") == 0) {
			*enable_srm = true;
			break;
		}
	}

	return 0;
}

static int parse_srmp(const struct shell *sh, size_t argc, char *argv[], bool *enable_srmp)
{
	ARG_UNUSED(sh);

	for (size_t i = 1; i < argc; i++) {
		if (strcmp(argv[i], "srmp") == 0) {
			*enable_srmp = true;
			break;
		}
	}

	return 0;
}

static bool ftp_find_tlv_param_cb(struct bt_obex_tlv *hdr, void *user_data)
{
	struct bt_obex_tlv *tlv = (struct bt_obex_tlv *)user_data;

	if (hdr->type == tlv->type) {
		tlv->data = hdr->data;
		tlv->data_len = hdr->data_len;
		return false;
	}

	return true;
}

static int ftp_extract_auth_challenge(struct net_buf *buf, struct bt_obex_tlv *auth_tlv)
{
	int err;
	uint16_t length;
	const uint8_t *auth;

	if (auth_tlv == NULL) {
		return -EINVAL;
	}

	err = bt_obex_get_header_auth_challenge(buf, &length, &auth);
	if (err != 0) {
		return err;
	}

	auth_tlv->type = BT_OBEX_CHALLENGE_TAG_NONCE;
	auth_tlv->data = NULL;
	auth_tlv->data_len = 0;

	bt_obex_tlv_parse(length, auth, ftp_find_tlv_param_cb, auth_tlv);

	if (auth_tlv->data == NULL || auth_tlv->data_len != BT_OBEX_CHALLENGE_TAG_NONCE_LEN) {
		return -EINVAL;
	}

	return 0;
}

static int ftp_extract_auth_response(struct net_buf *buf, struct bt_obex_tlv *auth_tlv)
{
	int err;
	uint16_t length;
	const uint8_t *auth;

	if (auth_tlv == NULL) {
		return -EINVAL;
	}

	err = bt_obex_get_header_auth_rsp(buf, &length, &auth);
	if (err != 0) {
		return err;
	}

	auth_tlv->type = BT_OBEX_RESPONSE_TAG_REQ_DIGEST;
	auth_tlv->data = NULL;
	auth_tlv->data_len = 0;

	bt_obex_tlv_parse(length, auth, ftp_find_tlv_param_cb, auth_tlv);

	if (auth_tlv->data == NULL || auth_tlv->data_len != BT_OBEX_RESPONSE_TAG_REQ_DIGEST_LEN) {
		return -EINVAL;
	}

	return 0;
}

#if defined(CONFIG_BT_FTP_CLIENT)
struct ftp_client_instance {
	struct bt_ftp_client client;
	struct bt_conn *conn;
	uint32_t conn_id;
	uint16_t mopl;
	uint16_t tx_cnt;
	uint8_t local_nonce[BT_OBEX_CHALLENGE_TAG_NONCE_LEN];
	uint8_t peer_nonce[BT_OBEX_CHALLENGE_TAG_NONCE_LEN];
	uint8_t pwd[CONFIG_BT_FTP_PWD_MAX_LEN + 1];
	uint8_t auth_state;
	uint8_t rsp_code;
	uint8_t flags;
	bool goep_v2;
};

static struct ftp_client_instance ftp_client_instances[CONFIG_BT_MAX_CONN];

static struct ftp_client_instance *ftp_client_alloc(void)
{
	uint8_t index;

	if (default_conn == NULL) {
		bt_shell_warn("Not connected");
		return NULL;
	}

	index = bt_conn_index(default_conn);
	if (index >= CONFIG_BT_MAX_CONN) {
		bt_shell_warn("conn index %u out of range (max %u)", index, CONFIG_BT_MAX_CONN);
		return NULL;
	}

	if (ftp_client_instances[index].conn != NULL) {
		bt_shell_warn("FTP client already connected");
		return NULL;
	}

	ftp_client_instances[index].conn = bt_conn_ref(default_conn);

	return &ftp_client_instances[index];
}

static void ftp_client_free(struct bt_ftp_client *client)
{
	struct ftp_client_instance *inst = CONTAINER_OF(client, struct ftp_client_instance, client);

	if (inst->conn != NULL) {
		bt_conn_unref(inst->conn);
		memset(&inst->conn, 0, sizeof(*inst) - offsetof(struct ftp_client_instance, conn));
	}
}

static struct ftp_client_instance *ftp_client_find(void)
{
	uint8_t index;

	if (default_conn == NULL) {
		bt_shell_warn("Not connected");
		return NULL;
	}

	index = bt_conn_index(default_conn);
	if (index >= CONFIG_BT_MAX_CONN) {
		bt_shell_warn("conn index %u out of range (max %u)", index, CONFIG_BT_MAX_CONN);
		return NULL;
	}

	if (ftp_client_instances[index].conn == NULL) {
		bt_shell_warn("No connected FTP client for conn %s", bt_conn_dst_str(default_conn));
		return NULL;
	}

	return &ftp_client_instances[index];
}

/* Client callbacks */
static void ftp_client_rfcomm_connected(struct bt_conn *conn, struct bt_ftp_client *client)
{
	struct ftp_client_instance *inst = CONTAINER_OF(client, struct ftp_client_instance, client);

	inst->goep_v2 = false;
	bt_shell_print("FTP client RFCOMM connected: %p, addr: %s", client, bt_conn_dst_str(conn));
}

static void ftp_client_rfcomm_disconnected(struct bt_ftp_client *client)
{
	bt_shell_print("FTP client RFCOMM disconnected: %p", client);
	ftp_client_free(client);
}

static void ftp_client_l2cap_connected(struct bt_conn *conn, struct bt_ftp_client *client)
{
	struct ftp_client_instance *inst = CONTAINER_OF(client, struct ftp_client_instance, client);

	inst->goep_v2 = true;
	bt_shell_print("FTP client L2CAP connected: %p, addr: %s", client, bt_conn_dst_str(conn));
}

static void ftp_client_l2cap_disconnected(struct bt_ftp_client *client)
{
	bt_shell_print("FTP client L2CAP disconnected: %p", client);
	ftp_client_free(client);
}

void ftp_client_clear_auth_state(struct ftp_client_instance *inst)
{
	inst->auth_state = 0;
	memset(inst->pwd, 0, sizeof(inst->pwd));
	memset(inst->local_nonce, 0, sizeof(inst->local_nonce));
	memset(inst->peer_nonce, 0, sizeof(inst->peer_nonce));
}

static void ftp_client_connect(struct bt_ftp_client *client, uint8_t rsp_code, uint8_t version,
			       uint16_t mopl, struct net_buf *buf)
{
	struct ftp_client_instance *inst = CONTAINER_OF(client, struct ftp_client_instance, client);
	int err;
	struct bt_obex_tlv auth;

	bt_shell_print("FTP client %p OBEX connect rsp, rsp_code %s, version %02x, mopl %04x",
		       client, bt_obex_rsp_code_to_str(rsp_code), version, mopl);
	ftp_parse_headers(buf);

	inst->mopl = mopl;

	if (rsp_code == BT_FTP_RSP_CODE_SUCCESS) {
		err = bt_obex_get_header_conn_id(buf, &inst->conn_id);
		if (err != 0) {
			bt_shell_warn("No connection ID in connect response");
		}

		if ((inst->auth_state & LOCAL_AUTH_ENABLED) != 0) {
			/* We sent a challenge; verify the server's auth response. */
			err = ftp_extract_auth_response(buf, &auth);
			if (err != 0) {
				bt_shell_error(
					"Authentication failed: no auth response from server");
				goto disconnect;
			}

			err = bt_ftp_verify_authentication(inst->local_nonce, (uint8_t *)auth.data,
							   inst->pwd);
			if (err != 0) {
				bt_shell_error(
					"Authentication failed: auth response verification failed");
				goto disconnect;
			}

			bt_shell_print("Connection established (authentication succeeded)");
		} else {
			bt_shell_print("Connection established (no auth required)");
		}

		goto clear;

	} else if (rsp_code == BT_FTP_RSP_CODE_UNAUTH) {
		/* Server requires authentication; extract its challenge nonce. */
		err = ftp_extract_auth_challenge(buf, &auth);
		if (err != 0) {
			bt_shell_warn("Failed to extract auth challenge (err %d)", err);
			goto clear;
		}

		inst->auth_state |= PEER_AUTH_ENABLED;
		memcpy(inst->peer_nonce, auth.data, auth.data_len);
		bt_shell_print("Server requires authentication");
		bt_shell_print("Re-send connect with password: ftp client connect <password>");
		return;
	}
	goto clear;

disconnect:
	bt_shell_warn("Disconnecting due to authentication failure");
	err = bt_ftp_client_disconnect(client, NULL);
	if (err != 0) {
		bt_shell_error("Disconnect failed (err %d)", err);
	}

clear:
	ftp_client_clear_auth_state(inst);
}

static void ftp_client_disconnect(struct bt_ftp_client *client, uint8_t rsp_code,
				  struct net_buf *buf)
{
	bt_shell_print("FTP client %p OBEX disconnect rsp, rsp_code %s", client,
		       bt_obex_rsp_code_to_str(rsp_code));
	ftp_parse_headers(buf);
}

static void ftp_client_abort(struct bt_ftp_client *client, uint8_t rsp_code, struct net_buf *buf)
{
	struct ftp_client_instance *inst = CONTAINER_OF(client, struct ftp_client_instance, client);

	bt_shell_print("FTP client %p abort rsp, rsp_code %s", client,
		       bt_obex_rsp_code_to_str(rsp_code));
	ftp_parse_headers(buf);

	inst->rsp_code = BT_OBEX_RSP_CODE_SUCCESS;
	inst->tx_cnt = 0;
}

static void ftp_client_set_folder(struct bt_ftp_client *client, uint8_t rsp_code,
				  struct net_buf *buf)
{
	struct ftp_client_instance *inst = CONTAINER_OF(client, struct ftp_client_instance, client);
	const char *s = inst->flags == BT_FTP_SET_FOLDER_FLAGS_NEW ? "create_folder" : "set_folder";

	bt_shell_print("FTP client %p %s rsp, rsp_code %s", client, s,
		       bt_obex_rsp_code_to_str(rsp_code));
	ftp_parse_headers(buf);
}

static void ftp_client_pull_folder_listing(struct bt_ftp_client *client, uint8_t rsp_code,
					   struct net_buf *buf)
{
	struct ftp_client_instance *inst = CONTAINER_OF(client, struct ftp_client_instance, client);

	bt_shell_print("FTP client %p pull_folder_listing rsp, rsp_code %s, data len %u", client,
		       bt_obex_rsp_code_to_str(rsp_code), buf->len);
	ftp_parse_headers(buf);

	inst->rsp_code = rsp_code;
}

static void ftp_client_push_file(struct bt_ftp_client *client, uint8_t rsp_code,
				 struct net_buf *buf)
{
	struct ftp_client_instance *inst = CONTAINER_OF(client, struct ftp_client_instance, client);

	bt_shell_print("FTP client %p push_file rsp, rsp_code %s", client,
		       bt_obex_rsp_code_to_str(rsp_code));
	ftp_parse_headers(buf);

	inst->rsp_code = rsp_code;
}

static void ftp_client_pull_file(struct bt_ftp_client *client, uint8_t rsp_code,
				 struct net_buf *buf)
{
	struct ftp_client_instance *inst = CONTAINER_OF(client, struct ftp_client_instance, client);

	bt_shell_print("FTP client %p pull_file rsp, rsp_code %s, data len %u", client,
		       bt_obex_rsp_code_to_str(rsp_code), buf->len);
	ftp_parse_headers(buf);

	inst->rsp_code = rsp_code;
}

static void ftp_client_delete(struct bt_ftp_client *client, uint8_t rsp_code, struct net_buf *buf)
{
	bt_shell_print("FTP client %p delete rsp, rsp_code %s", client,
		       bt_obex_rsp_code_to_str(rsp_code));
	ftp_parse_headers(buf);
}

static void ftp_client_rename(struct bt_ftp_client *client, uint8_t rsp_code, struct net_buf *buf)
{
	bt_shell_print("FTP client %p rename rsp, rsp_code %s", client,
		       bt_obex_rsp_code_to_str(rsp_code));
	ftp_parse_headers(buf);
}

static void ftp_client_copy(struct bt_ftp_client *client, uint8_t rsp_code, struct net_buf *buf)
{
	bt_shell_print("FTP client %p copy rsp, rsp_code %s", client,
		       bt_obex_rsp_code_to_str(rsp_code));
	ftp_parse_headers(buf);
}

static void ftp_client_set_permission(struct bt_ftp_client *client, uint8_t rsp_code,
				      struct net_buf *buf)
{
	bt_shell_print("FTP client %p set_permission rsp, rsp_code %s", client,
		       bt_obex_rsp_code_to_str(rsp_code));
	ftp_parse_headers(buf);
}

static struct bt_ftp_client_cb ftp_client_cb = {
	.rfcomm_connected = ftp_client_rfcomm_connected,
	.rfcomm_disconnected = ftp_client_rfcomm_disconnected,
	.l2cap_connected = ftp_client_l2cap_connected,
	.l2cap_disconnected = ftp_client_l2cap_disconnected,
	.connect = ftp_client_connect,
	.disconnect = ftp_client_disconnect,
	.abort = ftp_client_abort,
	.set_folder = ftp_client_set_folder,
	.pull_folder_listing = ftp_client_pull_folder_listing,
	.push_file = ftp_client_push_file,
	.pull_file = ftp_client_pull_file,
	.delete = ftp_client_delete,
	.rename = ftp_client_rename,
	.copy = ftp_client_copy,
	.set_permission = ftp_client_set_permission,
};

/* Client shell commands */
static int cmd_client_rfcomm_connect(const struct shell *sh, size_t argc, char *argv[])
{
	int err = 0;
	uint8_t channel;
	struct ftp_client_instance *inst;

	channel = (uint8_t)shell_strtoul(argv[1], 16, &err);
	if (err != 0) {
		shell_error(sh, "Invalid channel %s", argv[1]);
		return -ENOEXEC;
	}

	inst = ftp_client_alloc();
	if (inst == NULL) {
		return -EAGAIN;
	}

	err = bt_ftp_client_rfcomm_connect(default_conn, &inst->client, &ftp_client_cb, channel);
	if (err != 0) {
		ftp_client_free(&inst->client);
		shell_error(sh, "RFCOMM connect failed (err %d)", err);
	}

	return err;
}

static int cmd_client_rfcomm_disconnect(const struct shell *sh, size_t argc, char *argv[])
{
	int err;
	struct ftp_client_instance *inst;

	inst = ftp_client_find();
	if (inst == NULL) {
		return -ENODEV;
	}

	err = bt_ftp_client_rfcomm_disconnect(&inst->client);
	if (err != 0) {
		shell_error(sh, "RFCOMM disconnect failed (err %d)", err);
	}

	return err;
}

static int cmd_client_l2cap_connect(const struct shell *sh, size_t argc, char *argv[])
{
	int err = 0;
	uint16_t psm;
	struct ftp_client_instance *inst;

	psm = (uint16_t)shell_strtoul(argv[1], 16, &err);
	if (err != 0) {
		shell_error(sh, "Invalid PSM %s", argv[1]);
		return -ENOEXEC;
	}

	inst = ftp_client_alloc();
	if (inst == NULL) {
		return -EAGAIN;
	}

	err = bt_ftp_client_l2cap_connect(default_conn, &inst->client, &ftp_client_cb, psm);
	if (err != 0) {
		ftp_client_free(&inst->client);
		shell_error(sh, "L2CAP connect failed (err %d)", err);
	}

	return err;
}

static int cmd_client_l2cap_disconnect(const struct shell *sh, size_t argc, char *argv[])
{
	int err;
	struct ftp_client_instance *inst;

	inst = ftp_client_find();
	if (inst == NULL) {
		return -ENODEV;
	}

	err = bt_ftp_client_l2cap_disconnect(&inst->client);
	if (err != 0) {
		shell_error(sh, "L2CAP disconnect failed (err %d)", err);
	}

	return err;
}

static int cmd_client_connect(const struct shell *sh, size_t argc, char *argv[])
{
	int err;
	struct net_buf *buf;
	const struct bt_uuid_128 *uuid = BT_FTP_UUID;
	uint8_t val[BT_UUID_SIZE_128];
	struct ftp_client_instance *inst;
	const char *pwd = NULL;

	inst = ftp_client_find();
	if (inst == NULL) {
		return -ENODEV;
	}

	if (argc > 1) {
		pwd = argv[1];
		if (strlen(pwd) > CONFIG_BT_FTP_PWD_MAX_LEN) {
			shell_error(sh, "[password] too long (max %u)", CONFIG_BT_FTP_PWD_MAX_LEN);
			return -EINVAL;
		}
	}

	/* If the server previously challenged us (PEER_AUTH_ENABLED) and no
	 * password is given now, we cannot proceed with re-authentication.
	 */
	if (pwd == NULL && (inst->auth_state & PEER_AUTH_ENABLED) != 0) {
		shell_error(sh, "[password] required: server requested authentication");
		shell_error(sh, "usage: ftp client connect <password>");
		return -EINVAL;
	}

	/* If a password was given outside of the authentication flow, warn the
	 * user but proceed (the password will be used for the local challenge).
	 */
	if (pwd != NULL && (inst->auth_state & PEER_AUTH_ENABLED) == 0) {
		bt_shell_warn("[password] is ignored: authentication is not in progress");
		pwd = NULL;
	}

	buf = bt_ftp_client_create_pdu(&inst->client, &ftp_client_tx_pool);
	if (buf == NULL) {
		shell_error(sh, "Fail to allocate tx buffer");
		return -ENOBUFS;
	}

	/* Add target header - FTP service UUID */
	sys_memcpy_swap(val, uuid->val, BT_UUID_SIZE_128);
	err = bt_obex_add_header_target(buf, BT_UUID_SIZE_128, val);
	if (err != 0) {
		shell_error(sh, "Fail to add target header (err %d)", err);
		goto failed;
	}

	if (pwd != NULL) {
		struct bt_obex_tlv challenge_data;
		uint8_t digest[BT_OBEX_RESPONSE_TAG_REQ_DIGEST_LEN];

		/* Store password for later verification of server auth response. */
		strncpy((char *)inst->pwd, pwd, sizeof(inst->pwd) - 1);
		inst->pwd[sizeof(inst->pwd) - 1] = '\0';

		/* If the server sent us a challenge (re-connect after UNAUTH),
		 * add a local auth challenge so the server can authenticate us.
		 */
		err = bt_ftp_calculate_nonce(inst->pwd, inst->local_nonce);
		if (err != 0) {
			shell_error(sh, "Fail to calculate nonce (err %d)", err);
			goto failed;
		}

		challenge_data.type = BT_OBEX_CHALLENGE_TAG_NONCE;
		challenge_data.data = inst->local_nonce;
		challenge_data.data_len = BT_OBEX_CHALLENGE_TAG_NONCE_LEN;

		err = bt_obex_add_header_auth_challenge(buf, 1U, &challenge_data);
		if (err != 0) {
			shell_error(sh, "Fail to add auth challenge header (err %d)", err);
			goto failed;
		}

		/* Then add the auth response to respond the challenge from the Server. */
		err = bt_ftp_calculate_rsp_digest(inst->pwd, inst->peer_nonce, digest);
		if (err != 0) {
			shell_error(sh, "Fail to calculate auth response (err %d)", err);
			goto failed;
		}

		challenge_data.type = BT_OBEX_RESPONSE_TAG_REQ_DIGEST;
		challenge_data.data = digest;
		challenge_data.data_len = BT_OBEX_RESPONSE_TAG_REQ_DIGEST_LEN;

		err = bt_obex_add_header_auth_rsp(buf, 1U, &challenge_data);
		if (err != 0) {
			shell_error(sh, "Fail to add auth response header (err %d)", err);
			goto failed;
		}

		inst->auth_state |= LOCAL_AUTH_ENABLED;
	}

	err = bt_ftp_client_connect(&inst->client, buf);
	if (err == 0) {
		return 0;
	}

	shell_error(sh, "Connect failed (err %d)", err);
failed:
	net_buf_unref(buf);
	return err;
}

static int cmd_client_disconnect(const struct shell *sh, size_t argc, char *argv[])
{
	int err;
	struct ftp_client_instance *inst;

	inst = ftp_client_find();
	if (inst == NULL) {
		return -ENODEV;
	}

	err = bt_ftp_client_disconnect(&inst->client, NULL);
	if (err != 0) {
		shell_error(sh, "Disconnect failed (err %d)", err);
	}

	return err;
}

static int cmd_client_abort(const struct shell *sh, size_t argc, char *argv[])
{
	int err;
	struct ftp_client_instance *inst;

	inst = ftp_client_find();
	if (inst == NULL) {
		return -ENODEV;
	}

	err = bt_ftp_client_abort(&inst->client, NULL);
	if (err != 0) {
		shell_error(sh, "Abort failed (err %d)", err);
	}

	return err;
}

/* Convert an ASCII string to a big-endian UTF-16 encoding in buf_out.
 * buf_out must have at least strlen(name)*2+2 bytes.
 * Returns the number of bytes written (including the null terminator pair),
 * or a negative error code if the string is too long.
 */
static int ascii_to_unicode(const char *name, char *buf_out, uint16_t buf_size, uint16_t *out_len)
{
	uint16_t name_len = (uint16_t)strlen(name);
	uint16_t unicode_len = name_len * 2U + 2U;

	if (unicode_len > buf_size) {
		bt_shell_error("Name shall be less than %d chars", FTP_NAME_MAX_LEN);
		return -ENOMEM;
	}

	memset(buf_out, '\x00', buf_size);
	for (uint16_t i = 0U; i < name_len; i++) {
		buf_out[i * 2U + 1U] = name[i];
	}

	*out_len = unicode_len;
	return 0;
}

static int add_header_name(struct net_buf *buf, const char *name)
{
	char unicode_name[FTP_NAME_MAX_LEN * 2];
	uint16_t unicode_name_len;
	int err;

	if ((name == NULL) || (strlen(name) == 0)) {
		return bt_obex_add_header_name(buf, 0, NULL);
	}

	err = ascii_to_unicode(name, unicode_name, sizeof(unicode_name), &unicode_name_len);
	if (err != 0) {
		return err;
	}

	return bt_obex_add_header_name(buf, unicode_name_len, unicode_name);
}

static int add_header_dest_name(struct net_buf *buf, const char *name)
{
	char unicode_name[FTP_NAME_MAX_LEN * 2];
	uint16_t unicode_name_len;
	int err;

	if ((name == NULL) || (strlen(name) == 0)) {
		return -EINVAL;
	}

	err = ascii_to_unicode(name, unicode_name, sizeof(unicode_name), &unicode_name_len);
	if (err != 0) {
		return err;
	}

	return bt_obex_add_header_dest_name(buf, unicode_name_len, unicode_name);
}

static int cmd_client_set_folder(const struct shell *sh, size_t argc, char *argv[])
{
	int err;
	struct net_buf *buf;
	struct ftp_client_instance *inst;
	uint8_t flags;
	const char *path = argv[1];
	const char *name = NULL;

	inst = ftp_client_find();
	if (inst == NULL) {
		return -ENODEV;
	}

	if (strcmp(path, "/") == 0) {
		flags = BT_FTP_SET_FOLDER_FLAGS_ROOT;
	} else if (strncmp(path, "..", 2) == 0) {
		flags = BT_FTP_SET_FOLDER_FLAGS_UP;
	} else {
		flags = BT_FTP_SET_FOLDER_FLAGS_DOWN;
		if (strncmp(path, "./", 2) == 0) {
			if (path[2] != '\0') {
				name = &path[2];
			} else {
				shell_warn(sh, "Don't need to set current folder");
				shell_help(sh);
				return SHELL_CMD_HELP_PRINTED;
			}
		} else {
			name = path;
		}
	}

	buf = bt_ftp_client_create_pdu(&inst->client, &ftp_client_tx_pool);
	if (buf == NULL) {
		shell_error(sh, "Fail to allocate tx buffer");
		return -ENOBUFS;
	}

	err = bt_obex_add_header_conn_id(buf, inst->conn_id);
	if (err != 0) {
		shell_error(sh, "Fail to add conn id header (err %d)", err);
		goto failed;
	}

	err = add_header_name(buf, name);
	if (err != 0) {
		shell_error(sh, "Fail to add name header (err %d)", err);
		goto failed;
	}

	err = bt_ftp_client_set_folder(&inst->client, flags, buf);
	if (err == 0) {
		inst->flags = flags;
		return 0;
	}

	shell_error(sh, "Set folder failed (err %d)", err);
failed:
	net_buf_unref(buf);
	return err;
}

static int cmd_client_create_folder(const struct shell *sh, size_t argc, char *argv[])
{
	int err;
	struct net_buf *buf;
	struct ftp_client_instance *inst;
	const char *name = argv[1];

	inst = ftp_client_find();
	if (inst == NULL) {
		return -ENODEV;
	}

	buf = bt_ftp_client_create_pdu(&inst->client, &ftp_client_tx_pool);
	if (buf == NULL) {
		shell_error(sh, "Fail to allocate tx buffer");
		return -ENOBUFS;
	}

	err = bt_obex_add_header_conn_id(buf, inst->conn_id);
	if (err != 0) {
		shell_error(sh, "Fail to add conn id header (err %d)", err);
		goto failed;
	}

	err = add_header_name(buf, name);
	if (err != 0) {
		shell_error(sh, "Fail to add name header (err %d)", err);
		goto failed;
	}

	err = bt_ftp_client_set_folder(&inst->client, BT_FTP_SET_FOLDER_FLAGS_NEW, buf);
	if (err == 0) {
		inst->flags = BT_FTP_SET_FOLDER_FLAGS_NEW;
		return 0;
	}

	shell_error(sh, "Create folder failed (err %d)", err);
failed:
	net_buf_unref(buf);
	return err;
}

static int cmd_client_pull_folder_listing(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_buf *buf;
	int err;
	struct ftp_client_instance *inst;
	bool enable_srmp = false;
	bool enable_srm = false;
	const char *name = NULL;

	inst = ftp_client_find();
	if (inst == NULL) {
		return -ENODEV;
	}

	if (argc > 1 && strcmp(argv[1], "name") == 0) {
		if (argc < 3) {
			shell_error(sh, "[name_string] is needed if the name is present");
			return -EINVAL;
		}
		name = argv[2];
		argc -= 2;
		argv = &argv[2];
	}

	(void)parse_srm(sh, argc, argv, &enable_srm);
	(void)parse_srmp(sh, argc, argv, &enable_srmp);

	buf = bt_ftp_client_create_pdu(&inst->client, &ftp_client_tx_pool);
	if (buf == NULL) {
		shell_error(sh, "Fail to allocate tx buffer");
		return -ENOBUFS;
	}

	/* Continuation packet - only add SRMP if needed */
	if (inst->rsp_code == BT_OBEX_RSP_CODE_CONTINUE) {
		err = add_header_srm_param(buf, enable_srmp);
		if (err != 0) {
			shell_error(sh, "Fail to add SRMP header (err %d)", err);
			goto failed;
		}
		goto continue_req;
	}

	/* First request */
	err = bt_obex_add_header_conn_id(buf, inst->conn_id);
	if (err != 0) {
		shell_error(sh, "Fail to add conn id header (err %d)", err);
		goto failed;
	}

	err = add_header_srm(buf, inst->goep_v2 && enable_srm);
	if (err != 0) {
		shell_error(sh, "Fail to add SRM header (err %d)", err);
		goto failed;
	}

	err = add_header_srm_param(buf, enable_srmp);
	if (err != 0) {
		shell_error(sh, "Fail to add SRMP header (err %d)", err);
		goto failed;
	}

	err = bt_obex_add_header_type(buf, sizeof(BT_FTP_FOLDER_LISTING_TYPE),
				      BT_FTP_FOLDER_LISTING_TYPE);
	if (err != 0) {
		shell_error(sh, "Fail to add type header (err %d)", err);
		goto failed;
	}

	err = add_header_name(buf, name);
	if (err != 0) {
		shell_error(sh, "Fail to add name header (err %d)", err);
		goto failed;
	}

continue_req:
	err = bt_ftp_client_pull_folder_listing(&inst->client, true, buf);
	if (err == 0) {
		return 0;
	}

	shell_error(sh, "Pull folder listing failed (err %d)", err);
failed:
	net_buf_unref(buf);
	return err;
}

static int cmd_client_push_file(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_buf *buf;
	int err;
	struct ftp_client_instance *inst;
	bool enable_srm = false;
	const char *name = argv[1];
	const char *body = FTP_FILE_BODY;
	uint16_t len = 0;
	bool final;

	inst = ftp_client_find();
	if (inst == NULL) {
		return -ENODEV;
	}

	(void)parse_srm(sh, argc - 1, &argv[1], &enable_srm);

	buf = bt_ftp_client_create_pdu(&inst->client, &ftp_client_tx_pool);
	if (buf == NULL) {
		shell_error(sh, "Fail to allocate tx buffer");
		return -ENOBUFS;
	}

	/* Continuation packet - body was partially sent already */
	if (inst->tx_cnt > 0U && inst->tx_cnt < sizeof(FTP_FILE_BODY)) {
		goto continue_req;
	}

	/* First packet */
	err = bt_obex_add_header_conn_id(buf, inst->conn_id);
	if (err != 0) {
		shell_error(sh, "Fail to add conn id header (err %d)", err);
		goto failed;
	}

	err = add_header_srm(buf, inst->goep_v2 && enable_srm);
	if (err != 0) {
		shell_error(sh, "Fail to add SRM header (err %d)", err);
		goto failed;
	}

	err = add_header_name(buf, name);
	if (err != 0) {
		shell_error(sh, "Fail to add name header (err %d)", err);
		goto failed;
	}

continue_req:
	err = bt_obex_add_header_body_or_end_body(buf, inst->mopl,
						  sizeof(FTP_FILE_BODY) - inst->tx_cnt,
						  (const uint8_t *)(body + inst->tx_cnt), &len);
	if (err != 0) {
		shell_error(sh, "Fail to add body (err %d)", err);
		goto failed;
	}

	final = bt_obex_has_header(buf, BT_OBEX_HEADER_ID_END_BODY);

	err = bt_ftp_client_push_file(&inst->client, final, buf);
	if (err == 0) {
		if (!final) {
			inst->tx_cnt += len;
		} else {
			inst->tx_cnt = 0;
		}
		return 0;
	}

	shell_error(sh, "Push file failed (err %d)", err);
failed:
	net_buf_unref(buf);
	return err;
}

static int cmd_client_pull_file(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_buf *buf;
	int err;
	struct ftp_client_instance *inst;
	bool enable_srm = false;
	bool enable_srmp = false;
	const char *name = argv[1];

	inst = ftp_client_find();
	if (inst == NULL) {
		return -ENODEV;
	}

	(void)parse_srm(sh, argc - 1, &argv[1], &enable_srm);
	(void)parse_srmp(sh, argc - 1, &argv[1], &enable_srmp);

	buf = bt_ftp_client_create_pdu(&inst->client, &ftp_client_tx_pool);
	if (buf == NULL) {
		shell_error(sh, "Fail to allocate tx buffer");
		return -ENOBUFS;
	}

	/* Continuation packet - only add SRMP if needed */
	if (inst->rsp_code == BT_OBEX_RSP_CODE_CONTINUE) {
		err = add_header_srm_param(buf, enable_srmp);
		if (err != 0) {
			shell_error(sh, "Fail to add SRMP header (err %d)", err);
			goto failed;
		}
		goto continue_req;
	}

	err = bt_obex_add_header_conn_id(buf, inst->conn_id);
	if (err != 0) {
		shell_error(sh, "Fail to add conn id header (err %d)", err);
		goto failed;
	}

	err = add_header_srm(buf, inst->goep_v2 && enable_srm);
	if (err != 0) {
		shell_error(sh, "Fail to add SRM header (err %d)", err);
		goto failed;
	}

	err = add_header_srm_param(buf, enable_srmp);
	if (err != 0) {
		shell_error(sh, "Fail to add SRMP header (err %d)", err);
		goto failed;
	}

	err = add_header_name(buf, name);
	if (err != 0) {
		shell_error(sh, "Fail to add name header (err %d)", err);
		goto failed;
	}

continue_req:
	err = bt_ftp_client_pull_file(&inst->client, true, buf);
	if (err == 0) {
		return 0;
	}

	shell_error(sh, "Pull file failed (err %d)", err);
failed:
	net_buf_unref(buf);
	return err;
}

static int cmd_client_delete(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_buf *buf;
	int err;
	struct ftp_client_instance *inst;
	const char *name = argv[1];

	inst = ftp_client_find();
	if (inst == NULL) {
		return -ENODEV;
	}

	buf = bt_ftp_client_create_pdu(&inst->client, &ftp_client_tx_pool);
	if (buf == NULL) {
		shell_error(sh, "Fail to allocate tx buffer");
		return -ENOBUFS;
	}

	err = bt_obex_add_header_conn_id(buf, inst->conn_id);
	if (err != 0) {
		shell_error(sh, "Fail to add conn id header (err %d)", err);
		goto failed;
	}

	err = add_header_name(buf, name);
	if (err != 0) {
		shell_error(sh, "Fail to add name header (err %d)", err);
		goto failed;
	}

	err = bt_ftp_client_delete(&inst->client, true, buf);
	if (err == 0) {
		return 0;
	}

	shell_error(sh, "Delete failed (err %d)", err);
failed:
	net_buf_unref(buf);
	return err;
}

static int cmd_client_rename(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_buf *buf;
	int err;
	struct ftp_client_instance *inst;
	const char *src_name = argv[1];
	const char *dst_name = argv[2];

	inst = ftp_client_find();
	if (inst == NULL) {
		return -ENODEV;
	}

	buf = bt_ftp_client_create_pdu(&inst->client, &ftp_client_tx_pool);
	if (buf == NULL) {
		shell_error(sh, "Fail to allocate tx buffer");
		return -ENOBUFS;
	}

	err = bt_obex_add_header_conn_id(buf, inst->conn_id);
	if (err != 0) {
		shell_error(sh, "Fail to add conn id header (err %d)", err);
		goto failed;
	}

	err = bt_obex_add_header_action_id(buf, BT_OBEX_ACTION_MOVE_RENAME);
	if (err != 0) {
		shell_error(sh, "Fail to add action id header (err %d)", err);
		goto failed;
	}

	err = add_header_name(buf, src_name);
	if (err != 0) {
		shell_error(sh, "Fail to add name header (err %d)", err);
		goto failed;
	}

	err = add_header_dest_name(buf, dst_name);
	if (err != 0) {
		shell_error(sh, "Fail to add dest name header (err %d)", err);
		goto failed;
	}

	err = bt_ftp_client_rename(&inst->client, true, buf);
	if (err == 0) {
		return 0;
	}

	shell_error(sh, "Rename failed (err %d)", err);
failed:
	net_buf_unref(buf);
	return err;
}

static int cmd_client_copy(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_buf *buf;
	int err;
	struct ftp_client_instance *inst;
	const char *src_name = argv[1];
	const char *dst_name = argv[2];

	inst = ftp_client_find();
	if (inst == NULL) {
		return -ENODEV;
	}

	buf = bt_ftp_client_create_pdu(&inst->client, &ftp_client_tx_pool);
	if (buf == NULL) {
		shell_error(sh, "Fail to allocate tx buffer");
		return -ENOBUFS;
	}

	err = bt_obex_add_header_conn_id(buf, inst->conn_id);
	if (err != 0) {
		shell_error(sh, "Fail to add conn id header (err %d)", err);
		goto failed;
	}

	err = bt_obex_add_header_action_id(buf, BT_OBEX_ACTION_COPY);
	if (err != 0) {
		shell_error(sh, "Fail to add action id header (err %d)", err);
		goto failed;
	}

	err = add_header_name(buf, src_name);
	if (err != 0) {
		shell_error(sh, "Fail to add name header (err %d)", err);
		goto failed;
	}

	err = add_header_dest_name(buf, dst_name);
	if (err != 0) {
		shell_error(sh, "Fail to add dest name header (err %d)", err);
		goto failed;
	}

	err = bt_ftp_client_copy(&inst->client, true, buf);
	if (err == 0) {
		return 0;
	}

	shell_error(sh, "Copy failed (err %d)", err);
failed:
	net_buf_unref(buf);
	return err;
}

static int cmd_client_set_permission(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_buf *buf;
	int err = 0;
	struct ftp_client_instance *inst;
	const char *name = argv[1];
	uint32_t permission;

	inst = ftp_client_find();
	if (inst == NULL) {
		return -ENODEV;
	}

	permission = (uint32_t)shell_strtoul(argv[2], 16, &err);
	if (err != 0) {
		shell_error(sh, "Invalid permission %s", argv[2]);
		return -ENOEXEC;
	}

	buf = bt_ftp_client_create_pdu(&inst->client, &ftp_client_tx_pool);
	if (buf == NULL) {
		shell_error(sh, "Fail to allocate tx buffer");
		return -ENOBUFS;
	}

	err = bt_obex_add_header_conn_id(buf, inst->conn_id);
	if (err != 0) {
		shell_error(sh, "Fail to add conn id header (err %d)", err);
		goto failed;
	}

	err = bt_obex_add_header_action_id(buf, BT_OBEX_ACTION_SET_PERM);
	if (err != 0) {
		shell_error(sh, "Fail to add action id header (err %d)", err);
		goto failed;
	}

	err = add_header_name(buf, name);
	if (err != 0) {
		shell_error(sh, "Fail to add name header (err %d)", err);
		goto failed;
	}

	err = bt_obex_add_header_perm(buf, permission);
	if (err != 0) {
		shell_error(sh, "Fail to add permission header (err %d)", err);
		goto failed;
	}

	err = bt_ftp_client_set_permission(&inst->client, true, buf);
	if (err == 0) {
		return 0;
	}

	shell_error(sh, "Set permission failed (err %d)", err);
failed:
	net_buf_unref(buf);
	return err;
}
#endif /* CONFIG_BT_FTP_CLIENT */

#if defined(CONFIG_BT_FTP_SERVER)
struct ftp_server_instance {
	struct bt_ftp_server server;
	struct bt_conn *conn;
	uint16_t mopl;
	uint16_t tx_cnt;
	uint8_t local_nonce[BT_OBEX_CHALLENGE_TAG_NONCE_LEN];
	uint8_t peer_nonce[BT_OBEX_CHALLENGE_TAG_NONCE_LEN];
	uint8_t pwd[CONFIG_BT_FTP_PWD_MAX_LEN + 1];
	uint8_t auth_state;
	uint8_t rsp_code;
	bool final;
	bool goep_v2;
};

static struct ftp_server_instance ftp_server_instances[CONFIG_BT_MAX_CONN];

static struct ftp_server_instance *ftp_server_alloc(struct bt_conn *conn)
{
	uint8_t index;

	if (conn == NULL) {
		bt_shell_warn("conn is NULL");
		return NULL;
	}

	index = bt_conn_index(conn);
	if (index >= CONFIG_BT_MAX_CONN) {
		bt_shell_warn("conn index %u out of range (max %u)", index, CONFIG_BT_MAX_CONN);
		return NULL;
	}

	if (ftp_server_instances[index].conn != NULL) {
		bt_shell_warn("FTP server instance [%u] already in use", index);
		return NULL;
	}

	ftp_server_instances[index].conn = bt_conn_ref(conn);

	return &ftp_server_instances[index];
}

static void ftp_server_free(struct bt_ftp_server *server)
{
	struct ftp_server_instance *inst = CONTAINER_OF(server, struct ftp_server_instance, server);

	if (inst->conn != NULL) {
		bt_conn_unref(inst->conn);
		memset(&inst->conn, 0, sizeof(*inst) - offsetof(struct ftp_server_instance, conn));
	}
}

static struct ftp_server_instance *ftp_server_find(void)
{
	uint8_t index;

	if (default_conn == NULL) {
		bt_shell_warn("Not connected");
		return NULL;
	}

	index = bt_conn_index(default_conn);
	if (index >= CONFIG_BT_MAX_CONN) {
		bt_shell_warn("conn index %u out of range (max %u)", index, CONFIG_BT_MAX_CONN);
		return NULL;
	}

	if (ftp_server_instances[index].conn == NULL) {
		bt_shell_warn("No connected FTP server for conn %s", bt_conn_dst_str(default_conn));
		return NULL;
	}

	return &ftp_server_instances[index];
}

/* Server callbacks */
static void ftp_server_rfcomm_connected(struct bt_conn *conn, struct bt_ftp_server *server)
{
	struct ftp_server_instance *inst = CONTAINER_OF(server, struct ftp_server_instance, server);

	inst->goep_v2 = false;
	bt_shell_print("FTP server RFCOMM connected: %p, addr: %s", server, bt_conn_dst_str(conn));
}

static void ftp_server_rfcomm_disconnected(struct bt_ftp_server *server)
{
	bt_shell_print("FTP server RFCOMM disconnected: %p", server);
	ftp_server_free(server);
}

static void ftp_server_l2cap_connected(struct bt_conn *conn, struct bt_ftp_server *server)
{
	struct ftp_server_instance *inst = CONTAINER_OF(server, struct ftp_server_instance, server);

	inst->goep_v2 = true;
	bt_shell_print("FTP server L2CAP connected: %p, addr: %s", server, bt_conn_dst_str(conn));
}

static void ftp_server_l2cap_disconnected(struct bt_ftp_server *server)
{
	bt_shell_print("FTP server L2CAP disconnected: %p", server);
	ftp_server_free(server);
}

static void ftp_server_connect(struct bt_ftp_server *server, uint8_t version, uint16_t mopl,
			       struct net_buf *buf)
{
	struct ftp_server_instance *inst = CONTAINER_OF(server, struct ftp_server_instance, server);
	int err;
	struct bt_obex_tlv auth;

	bt_shell_print("FTP server %p OBEX connect req, version %02x, mopl %04x", server, version,
		       mopl);
	ftp_parse_headers(buf);

	inst->mopl = mopl;

	/* Verify client's auth response if we previously sent a challenge. */
	if ((inst->auth_state & LOCAL_AUTH_ENABLED) != 0) {
		err = ftp_extract_auth_response(buf, &auth);
		if (err != 0) {
			bt_shell_warn("Authentication failed: no auth response from client");
			return;
		}

		err = bt_ftp_verify_authentication(inst->local_nonce, (uint8_t *)auth.data,
						   inst->pwd);
		if (err != 0) {
			bt_shell_error("Authentication failed: auth response verification failed");
			return;
		}
		bt_shell_print("Authentication succeeded");

		err = ftp_extract_auth_challenge(buf, &auth);
		if (err != 0) {
			bt_shell_warn("Failed to extract client auth challenge (err %d)", err);
		} else {
			inst->auth_state |= PEER_AUTH_ENABLED;
			memcpy(inst->peer_nonce, auth.data, auth.data_len);
			bt_shell_print("Client requires authentication");
		}
	}
}

static void ftp_server_disconnect(struct bt_ftp_server *server, struct net_buf *buf)
{
	bt_shell_print("FTP server %p OBEX disconnect req", server);
	ftp_parse_headers(buf);
}

static void ftp_server_abort(struct bt_ftp_server *server, struct net_buf *buf)
{
	bt_shell_print("FTP server %p abort req", server);
	ftp_parse_headers(buf);
}

static void ftp_server_set_folder(struct bt_ftp_server *server, uint8_t flags, struct net_buf *buf)
{
	if (flags == BT_FTP_SET_FOLDER_FLAGS_NEW) {
		bt_shell_print("FTP server %p create_folder req", server);
	} else {
		bt_shell_print("FTP server %p set_folder req, flags %02x", server, flags);
	}

	ftp_parse_headers(buf);
}

static void ftp_server_pull_folder_listing(struct bt_ftp_server *server, bool final,
					   struct net_buf *buf)
{
	bt_shell_print("FTP server %p pull_folder_listing req, final %s", server,
		       final ? "true" : "false");
	ftp_parse_headers(buf);
}

static void ftp_server_push_file(struct bt_ftp_server *server, bool final, struct net_buf *buf)
{
	struct ftp_server_instance *inst = CONTAINER_OF(server, struct ftp_server_instance, server);

	bt_shell_print("FTP server %p push_file req, final %s, data len %u", server,
		       final ? "true" : "false", buf->len);
	ftp_parse_headers(buf);

	inst->final = final;
}

static void ftp_server_pull_file(struct bt_ftp_server *server, bool final, struct net_buf *buf)
{
	bt_shell_print("FTP server %p pull_file req, final %s", server, final ? "true" : "false");
	ftp_parse_headers(buf);
}

static void ftp_server_delete(struct bt_ftp_server *server, bool final, struct net_buf *buf)
{
	bt_shell_print("FTP server %p delete req, final %s", server, final ? "true" : "false");
	ftp_parse_headers(buf);
}

static void ftp_server_rename(struct bt_ftp_server *server, bool final, struct net_buf *buf)
{
	bt_shell_print("FTP server %p rename req, final %s", server, final ? "true" : "false");
	ftp_parse_headers(buf);
}

static void ftp_server_copy(struct bt_ftp_server *server, bool final, struct net_buf *buf)
{
	bt_shell_print("FTP server %p copy req, final %s", server, final ? "true" : "false");
	ftp_parse_headers(buf);
}

static void ftp_server_set_permission(struct bt_ftp_server *server, bool final, struct net_buf *buf)
{
	bt_shell_print("FTP server %p set_permission req, final %s", server,
		       final ? "true" : "false");
	ftp_parse_headers(buf);
}

static struct bt_ftp_server_cb ftp_server_cb = {
	.rfcomm_connected = ftp_server_rfcomm_connected,
	.rfcomm_disconnected = ftp_server_rfcomm_disconnected,
	.l2cap_connected = ftp_server_l2cap_connected,
	.l2cap_disconnected = ftp_server_l2cap_disconnected,
	.connect = ftp_server_connect,
	.disconnect = ftp_server_disconnect,
	.abort = ftp_server_abort,
	.set_folder = ftp_server_set_folder,
	.pull_folder_listing = ftp_server_pull_folder_listing,
	.push_file = ftp_server_push_file,
	.pull_file = ftp_server_pull_file,
	.delete = ftp_server_delete,
	.rename = ftp_server_rename,
	.copy = ftp_server_copy,
	.set_permission = ftp_server_set_permission,
};

/* RFCOMM server registration */
static int ftp_server_rfcomm_accept(struct bt_conn *conn,
				    struct bt_ftp_server_rfcomm *ftp_server_rfcomm,
				    struct bt_ftp_server **ftp_server)
{
	struct ftp_server_instance *inst;

	inst = ftp_server_alloc(conn);
	if (inst == NULL) {
		bt_shell_warn("Cannot allocate FTP server instance");
		return -ENOMEM;
	}

	(void)bt_ftp_server_register(&inst->server, &ftp_server_cb);

	*ftp_server = &inst->server;
	return 0;
}

static struct bt_ftp_server_rfcomm ftp_server_rfcomm = {
	.accept = ftp_server_rfcomm_accept,
};

/* L2CAP server registration */
static int ftp_server_l2cap_accept(struct bt_conn *conn,
				   struct bt_ftp_server_l2cap *ftp_server_l2cap,
				   struct bt_ftp_server **ftp_server)
{
	struct ftp_server_instance *inst;

	inst = ftp_server_alloc(conn);
	if (inst == NULL) {
		bt_shell_warn("Cannot allocate FTP server instance");
		return -ENOMEM;
	}

	(void)bt_ftp_server_register(&inst->server, &ftp_server_cb);

	*ftp_server = &inst->server;
	return 0;
}

static struct bt_ftp_server_l2cap ftp_server_l2cap = {
	.accept = ftp_server_l2cap_accept,
};

/* Server shell commands */

static int cmd_server_rfcomm_register(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	err = bt_ftp_server_rfcomm_register(&ftp_server_rfcomm);
	if (err != 0) {
		shell_error(sh, "RFCOMM server register failed (err %d)", err);
		return err;
	}

	shell_print(sh, "FTP RFCOMM server registered, channel %u",
		    ftp_server_rfcomm.server.rfcomm.channel);
	return 0;
}

static int cmd_server_rfcomm_disconnect(const struct shell *sh, size_t argc, char *argv[])
{
	int err;
	struct ftp_server_instance *inst;

	inst = ftp_server_find();
	if (inst == NULL) {
		return -ENODEV;
	}

	err = bt_ftp_server_rfcomm_disconnect(&inst->server);
	if (err != 0) {
		shell_error(sh, "RFCOMM disconnect failed (err %d)", err);
	}

	return err;
}

static int cmd_server_l2cap_register(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	err = bt_ftp_server_l2cap_register(&ftp_server_l2cap);
	if (err != 0) {
		shell_error(sh, "L2CAP server register failed (err %d)", err);
		return err;
	}

	shell_print(sh, "FTP L2CAP server registered, psm 0x%04x",
		    ftp_server_l2cap.server.l2cap.psm);
	return 0;
}

static int cmd_server_l2cap_disconnect(const struct shell *sh, size_t argc, char *argv[])
{
	int err;
	struct ftp_server_instance *inst;

	inst = ftp_server_find();
	if (inst == NULL) {
		return -ENODEV;
	}

	err = bt_ftp_server_l2cap_disconnect(&inst->server);
	if (err != 0) {
		shell_error(sh, "L2CAP disconnect failed (err %d)", err);
	}

	return err;
}

void ftp_server_clear_auth_state(struct ftp_server_instance *inst)
{
	inst->auth_state = 0;
	memset(inst->pwd, 0, sizeof(inst->pwd));
	memset(inst->local_nonce, 0, sizeof(inst->local_nonce));
	memset(inst->peer_nonce, 0, sizeof(inst->peer_nonce));
}

static int cmd_server_connect(const struct shell *sh, size_t argc, char *argv[])
{
	int err;
	struct net_buf *buf;
	struct ftp_server_instance *inst;
	uint8_t rsp_code;
	const char *rsp;
	const char *pwd = NULL;
	struct bt_obex_tlv challenge_data;

	inst = ftp_server_find();
	if (inst == NULL) {
		return -ENODEV;
	}

	rsp = argv[1];
	if (strcmp(rsp, "unauth") == 0) {
		rsp_code = BT_OBEX_RSP_CODE_UNAUTH;
		if (argc < 3) {
			shell_error(sh, "[password] is needed if the rsp is %s", rsp);
			shell_help(sh);
			return SHELL_CMD_HELP_PRINTED;
		}
		pwd = argv[2];
	} else if (strcmp(rsp, "success") == 0) {
		rsp_code = BT_OBEX_RSP_CODE_SUCCESS;
		if (argc > 2) {
			shell_warn(sh, "[password] arguments ignored");
		}
	} else if (strcmp(rsp, "error") == 0) {
		if (argc < 3) {
			shell_error(sh, "[rsp_code] is needed if the rsp is %s", rsp);
			shell_help(sh);
			return SHELL_CMD_HELP_PRINTED;
		}
		rsp_code = (uint8_t)strtoul(argv[2], NULL, 16);

		if (rsp_code == BT_OBEX_RSP_CODE_UNAUTH) {
			if (argc < 4) {
				shell_error(sh, "[password] is needed if the rsp_code is unauth");
				shell_help(sh);
				return SHELL_CMD_HELP_PRINTED;
			}
			pwd = argv[3];
		}
	} else {
		shell_help(sh);
		return SHELL_CMD_HELP_PRINTED;
	}

	if (pwd != NULL && strlen(pwd) > CONFIG_BT_FTP_PWD_MAX_LEN) {
		shell_error(sh, "[password] too long (max %u)", CONFIG_BT_FTP_PWD_MAX_LEN);
		return -EINVAL;
	}

	buf = bt_ftp_server_create_pdu(&inst->server, &ftp_server_tx_pool);
	if (buf == NULL) {
		shell_error(sh, "Fail to allocate tx buffer");
		return -ENOBUFS;
	}

	if (rsp_code == BT_OBEX_RSP_CODE_SUCCESS) {
		if ((inst->auth_state & PEER_AUTH_ENABLED) != 0U) {
			uint8_t digest[BT_OBEX_RESPONSE_TAG_REQ_DIGEST_LEN];

			/* Client challenged us; respond with auth response. */
			err = bt_ftp_calculate_rsp_digest(inst->pwd, inst->peer_nonce, digest);
			if (err != 0) {
				shell_error(sh, "Fail to calculate auth response digest (err %d)",
					    err);
				goto failed;
			}

			challenge_data.type = BT_OBEX_RESPONSE_TAG_REQ_DIGEST;
			challenge_data.data = digest;
			challenge_data.data_len = BT_OBEX_RESPONSE_TAG_REQ_DIGEST_LEN;

			err = bt_obex_add_header_auth_rsp(buf, 1U, &challenge_data);
			if (err != 0) {
				shell_error(sh, "Fail to add auth response header (err %d)", err);
				goto failed;
			}
		}
	} else if (rsp_code == BT_OBEX_RSP_CODE_UNAUTH) {
		/* Send an auth challenge so the client must authenticate. */
		strncpy((char *)inst->pwd, pwd, sizeof(inst->pwd) - 1);
		inst->pwd[sizeof(inst->pwd) - 1] = '\0';

		err = bt_ftp_calculate_nonce(inst->pwd, inst->local_nonce);
		if (err != 0) {
			shell_error(sh, "Fail to calculate nonce (err %d)", err);
			goto failed;
		}

		challenge_data.type = BT_OBEX_CHALLENGE_TAG_NONCE;
		challenge_data.data = inst->local_nonce;
		challenge_data.data_len = BT_OBEX_CHALLENGE_TAG_NONCE_LEN;

		err = bt_obex_add_header_auth_challenge(buf, 1U, &challenge_data);
		if (err != 0) {
			shell_error(sh, "Fail to add auth challenge header (err %d)", err);
			goto failed;
		}

		inst->auth_state |= LOCAL_AUTH_ENABLED;
	} else {
		/* no action */
	}

	err = bt_ftp_server_connect(&inst->server, rsp_code, buf);
	if (err == 0) {
		if (rsp_code != BT_OBEX_RSP_CODE_UNAUTH) {
			ftp_server_clear_auth_state(inst);
		}
		return 0;
	}

	shell_error(sh, "Connect response failed (err %d)", err);
failed:
	net_buf_unref(buf);
	return err;
}

static int parse_rsp_code(const struct shell *sh, size_t argc, char *argv[], uint8_t *rsp_code)
{
	const char *rsp;

	rsp = argv[1];
	if (strcmp(rsp, "success") == 0) {
		*rsp_code = BT_OBEX_RSP_CODE_SUCCESS;
	} else if (strcmp(rsp, "error") == 0) {
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

static int parse_error_code(const struct shell *sh, size_t argc, char *argv[], uint8_t *rsp_code)
{
	const char *rsp;

	rsp = argv[1];
	if (strcmp(rsp, "noerror") == 0) {
		*rsp_code = BT_OBEX_RSP_CODE_SUCCESS;
	} else if (strcmp(rsp, "error") == 0) {
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

static int cmd_server_disconnect(const struct shell *sh, size_t argc, char *argv[])
{
	int err;
	struct ftp_server_instance *inst;
	uint8_t rsp_code = BT_FTP_RSP_CODE_SUCCESS;

	inst = ftp_server_find();
	if (inst == NULL) {
		return -ENODEV;
	}

	err = parse_rsp_code(sh, argc, argv, &rsp_code);
	if (err != 0) {
		return err;
	}

	err = bt_ftp_server_disconnect(&inst->server, rsp_code, NULL);
	if (err != 0) {
		shell_error(sh, "Disconnect response failed (err %d)", err);
	}

	return err;
}

static int cmd_server_abort(const struct shell *sh, size_t argc, char *argv[])
{
	int err;
	struct ftp_server_instance *inst;
	uint8_t rsp_code = BT_FTP_RSP_CODE_SUCCESS;

	inst = ftp_server_find();
	if (inst == NULL) {
		return -ENODEV;
	}

	err = parse_rsp_code(sh, argc, argv, &rsp_code);
	if (err != 0) {
		return err;
	}

	err = bt_ftp_server_abort(&inst->server, rsp_code, NULL);
	if (err == 0) {
		inst->tx_cnt = 0;
	} else {
		shell_error(sh, "Abort response failed (err %d)", err);
	}

	return err;
}

static int cmd_server_set_folder(const struct shell *sh, size_t argc, char *argv[])
{
	int err;
	struct ftp_server_instance *inst;
	uint8_t rsp_code = BT_FTP_RSP_CODE_SUCCESS;

	inst = ftp_server_find();
	if (inst == NULL) {
		return -ENODEV;
	}

	err = parse_rsp_code(sh, argc, argv, &rsp_code);
	if (err != 0) {
		return err;
	}

	err = bt_ftp_server_set_folder(&inst->server, rsp_code, NULL);
	if (err != 0) {
		shell_error(sh, "Set folder response failed (err %d)", err);
	}

	return err;
}

#define FTP_FOLDER_LISTING                                                                         \
	"<?xml version=\"1.0\"?>\r\n"                                                              \
	"<!DOCTYPE folder-listing SYSTEM \"obex-folder-listing.dtd\">\r\n"                         \
	"<folder-listing version=\"1.0\">\r\n"                                                     \
	"<folder name=\"docs\" created=\"20260101T000000Z\"/>\r\n"                                 \
	"<folder name=\"images\" created=\"20260102T000000Z\"/>\r\n"                               \
	"<folder name=\"music\" created=\"20260103T000000Z\"/>\r\n"                                \
	"<folder name=\"videos\" created=\"20260104T000000Z\"/>\r\n"                               \
	"<file name=\"readme.txt\" size=\"128\" created=\"20260101T010000Z\"/>\r\n"                \
	"<file name=\"config.ini\" size=\"256\" created=\"20260101T020000Z\"/>\r\n"                \
	"<file name=\"photo.jpg\" size=\"204800\" created=\"20260102T010000Z\"/>\r\n"              \
	"<file name=\"song.mp3\" size=\"3145728\" created=\"20260103T010000Z\"/>\r\n"              \
	"<file name=\"notes.txt\" size=\"512\" created=\"20260104T010000Z\"/>\r\n"                 \
	"</folder-listing>"

static int cmd_server_pull_folder_listing(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_buf *buf;
	int err;
	struct ftp_server_instance *inst;
	bool enable_srm = false;
	bool enable_srmp = false;
	const char *body = FTP_FOLDER_LISTING;
	uint8_t rsp_code = BT_FTP_RSP_CODE_SUCCESS;
	uint16_t len = 0;

	inst = ftp_server_find();
	if (inst == NULL) {
		return -ENODEV;
	}

	err = parse_error_code(sh, argc, argv, &rsp_code);
	if (err != 0) {
		return err;
	}

	(void)parse_srm(sh, argc, argv, &enable_srm);
	(void)parse_srmp(sh, argc, argv, &enable_srmp);

	buf = bt_ftp_server_create_pdu(&inst->server, &ftp_server_tx_pool);
	if (buf == NULL) {
		shell_error(sh, "Fail to allocate tx buffer");
		return -ENOBUFS;
	}

	if (rsp_code != BT_FTP_RSP_CODE_SUCCESS) {
		goto error_rsp;
	}

	/* Add SRM response header on first response packet */
	err = add_header_srm(buf, inst->goep_v2 && enable_srm);
	if (err != 0) {
		shell_error(sh, "Fail to add SRM header (err %d)", err);
		goto failed;
	}

	err = add_header_srm_param(buf, enable_srmp);
	if (err != 0) {
		shell_error(sh, "Fail to add SRMP header (err %d)", err);
		goto failed;
	}

	err = bt_obex_add_header_body_or_end_body(buf, inst->mopl,
						  sizeof(FTP_FOLDER_LISTING) - inst->tx_cnt,
						  body + inst->tx_cnt, &len);
	if (err != 0) {
		shell_error(sh, "Fail to add body (err %d)", err);
		goto failed;
	}

	if (!bt_obex_has_header(buf, BT_OBEX_HEADER_ID_END_BODY)) {
		rsp_code = BT_FTP_RSP_CODE_CONTINUE;
	}

error_rsp:
	err = bt_ftp_server_pull_folder_listing(&inst->server, rsp_code, buf);
	if (err == 0) {
		if (rsp_code == BT_FTP_RSP_CODE_CONTINUE) {
			inst->tx_cnt += len;
		} else {
			inst->tx_cnt = 0;
		}
		return 0;
	}

	shell_error(sh, "Pull folder listing response failed (err %d)", err);
failed:
	net_buf_unref(buf);
	return err;
}

static int cmd_server_push_file(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_buf *buf;
	int err;
	struct ftp_server_instance *inst;
	bool enable_srm = false;
	bool enable_srmp = false;
	uint8_t rsp_code = BT_FTP_RSP_CODE_SUCCESS;

	inst = ftp_server_find();
	if (inst == NULL) {
		return -ENODEV;
	}

	err = parse_error_code(sh, argc, argv, &rsp_code);
	if (err != 0) {
		return err;
	}

	(void)parse_srm(sh, argc, argv, &enable_srm);
	(void)parse_srmp(sh, argc, argv, &enable_srmp);

	buf = bt_ftp_server_create_pdu(&inst->server, &ftp_server_tx_pool);
	if (buf == NULL) {
		shell_error(sh, "Fail to allocate tx buffer");
		return -ENOBUFS;
	}

	if (rsp_code != BT_FTP_RSP_CODE_SUCCESS) {
		goto error_rsp;
	}

	err = add_header_srm(buf, inst->goep_v2 && enable_srm);
	if (err != 0) {
		shell_error(sh, "Fail to add SRM header (err %d)", err);
		goto failed;
	}

	err = add_header_srm_param(buf, enable_srmp);
	if (err != 0) {
		shell_error(sh, "Fail to add SRMP header (err %d)", err);
		goto failed;
	}

	if (!inst->final) {
		rsp_code = BT_OBEX_RSP_CODE_CONTINUE;
	}

error_rsp:
	err = bt_ftp_server_push_file(&inst->server, rsp_code, buf);
	if (err == 0) {
		return 0;
	}

	shell_error(sh, "Push file failed (err %d)", err);
failed:
	net_buf_unref(buf);
	return err;
}

static int cmd_server_pull_file(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_buf *buf;
	int err;
	struct ftp_server_instance *inst;
	bool enable_srm = false;
	bool enable_srmp = false;
	uint8_t rsp_code = BT_FTP_RSP_CODE_SUCCESS;
	const char *body = FTP_FILE_BODY;
	uint16_t len = 0;

	inst = ftp_server_find();
	if (inst == NULL) {
		return -ENODEV;
	}

	err = parse_error_code(sh, argc, argv, &rsp_code);
	if (err != 0) {
		return err;
	}

	(void)parse_srm(sh, argc, argv, &enable_srm);
	(void)parse_srmp(sh, argc, argv, &enable_srmp);

	buf = bt_ftp_server_create_pdu(&inst->server, &ftp_server_tx_pool);
	if (buf == NULL) {
		shell_error(sh, "Fail to allocate tx buffer");
		return -ENOBUFS;
	}

	if (rsp_code != BT_FTP_RSP_CODE_SUCCESS) {
		goto error_rsp;
	}

	/* Add SRM response headers on first packet */
	err = add_header_srm(buf, inst->goep_v2 && enable_srm);
	if (err != 0) {
		shell_error(sh, "Fail to add SRM header (err %d)", err);
		goto failed;
	}

	err = add_header_srm_param(buf, enable_srmp);
	if (err != 0) {
		shell_error(sh, "Fail to add SRMP header (err %d)", err);
		goto failed;
	}

	err = bt_obex_add_header_body_or_end_body(
		buf, inst->mopl, sizeof(FTP_FILE_BODY) - inst->tx_cnt, body + inst->tx_cnt, &len);
	if (err != 0) {
		shell_error(sh, "Fail to add body (err %d)", err);
		goto failed;
	}

	if (!bt_obex_has_header(buf, BT_OBEX_HEADER_ID_END_BODY)) {
		rsp_code = BT_FTP_RSP_CODE_CONTINUE;
	}

error_rsp:
	err = bt_ftp_server_pull_file(&inst->server, rsp_code, buf);
	if (err == 0) {
		if (rsp_code == BT_FTP_RSP_CODE_CONTINUE) {
			inst->tx_cnt += len;
		} else {
			inst->tx_cnt = 0;
		}
		return 0;
	}

	shell_error(sh, "Pull file response failed (err %d)", err);
failed:
	net_buf_unref(buf);
	return err;
}

static int cmd_server_delete(const struct shell *sh, size_t argc, char *argv[])
{
	int err;
	struct ftp_server_instance *inst;
	uint8_t rsp_code = BT_FTP_RSP_CODE_SUCCESS;

	inst = ftp_server_find();
	if (inst == NULL) {
		return -ENODEV;
	}

	err = parse_rsp_code(sh, argc, argv, &rsp_code);
	if (err != 0) {
		return err;
	}

	err = bt_ftp_server_delete(&inst->server, rsp_code, NULL);
	if (err != 0) {
		shell_error(sh, "Delete response failed (err %d)", err);
	}

	return err;
}

static int cmd_server_rename(const struct shell *sh, size_t argc, char *argv[])
{
	int err;
	struct ftp_server_instance *inst;
	uint8_t rsp_code = BT_FTP_RSP_CODE_SUCCESS;

	inst = ftp_server_find();
	if (inst == NULL) {
		return -ENODEV;
	}

	err = parse_rsp_code(sh, argc, argv, &rsp_code);
	if (err != 0) {
		return err;
	}

	err = bt_ftp_server_rename(&inst->server, rsp_code, NULL);
	if (err != 0) {
		shell_error(sh, "Rename response failed (err %d)", err);
	}

	return err;
}

static int cmd_server_copy(const struct shell *sh, size_t argc, char *argv[])
{
	int err;
	struct ftp_server_instance *inst;
	uint8_t rsp_code = BT_FTP_RSP_CODE_SUCCESS;

	inst = ftp_server_find();
	if (inst == NULL) {
		return -ENODEV;
	}

	err = parse_rsp_code(sh, argc, argv, &rsp_code);
	if (err != 0) {
		return err;
	}

	err = bt_ftp_server_copy(&inst->server, rsp_code, NULL);
	if (err != 0) {
		shell_error(sh, "Copy response failed (err %d)", err);
	}

	return err;
}

static int cmd_server_set_permission(const struct shell *sh, size_t argc, char *argv[])
{
	int err;
	struct ftp_server_instance *inst;
	uint8_t rsp_code = BT_FTP_RSP_CODE_SUCCESS;

	inst = ftp_server_find();
	if (inst == NULL) {
		return -ENODEV;
	}

	err = parse_rsp_code(sh, argc, argv, &rsp_code);
	if (err != 0) {
		return err;
	}

	err = bt_ftp_server_set_permission(&inst->server, rsp_code, NULL);
	if (err != 0) {
		shell_error(sh, "Set permission response failed (err %d)", err);
	}

	return err;
}
#endif /* CONFIG_BT_FTP_SERVER */

static int cmd_common(const struct shell *sh, size_t argc, char **argv)
{
	if (argc == 1) {
		shell_help(sh);
		return SHELL_CMD_HELP_PRINTED;
	}

	shell_error(sh, "%s unknown parameter: %s", argv[0], argv[1]);
	return -ENOEXEC;
}

#define HELP_NONE  "[none]"
#define RSP_HELP_1 "<rsp: success, error> [rsp_code]"
#define RSP_HELP_2 "<rsp: noerror, error> [rsp_code] [srm] [srmp]"

#if defined(CONFIG_BT_FTP_CLIENT)
SHELL_STATIC_SUBCMD_SET_CREATE(
	ftp_client_cmds,
	SHELL_CMD_ARG(rfcomm_connect, NULL, "<channel>", cmd_client_rfcomm_connect, 2, 0),
	SHELL_CMD_ARG(rfcomm_disconnect, NULL, HELP_NONE, cmd_client_rfcomm_disconnect, 1, 0),
	SHELL_CMD_ARG(l2cap_connect, NULL, "<psm>", cmd_client_l2cap_connect, 2, 0),
	SHELL_CMD_ARG(l2cap_disconnect, NULL, HELP_NONE, cmd_client_l2cap_disconnect, 1, 0),
	SHELL_CMD_ARG(connect, NULL, "[password]", cmd_client_connect, 1, 1),
	SHELL_CMD_ARG(disconnect, NULL, HELP_NONE, cmd_client_disconnect, 1, 0),
	SHELL_CMD_ARG(abort, NULL, HELP_NONE, cmd_client_abort, 1, 0),
	SHELL_CMD_ARG(set_folder, NULL, "<path: \"/\" | \"..\" | \"folder\">",
		      cmd_client_set_folder, 2, 0),
	SHELL_CMD_ARG(create_folder, NULL, "<folder_name>", cmd_client_create_folder, 2, 0),
	SHELL_CMD_ARG(pull_folder_listing, NULL, "[name <folder_name>] [srm] [srmp]",
		      cmd_client_pull_folder_listing, 1, 4),
	SHELL_CMD_ARG(push_file, NULL, "<filename> [srm]", cmd_client_push_file, 2, 1),
	SHELL_CMD_ARG(pull_file, NULL, "<filename> [srm] [srmp]", cmd_client_pull_file, 2, 2),
	SHELL_CMD_ARG(delete, NULL, "<filename>", cmd_client_delete, 2, 0),
	SHELL_CMD_ARG(rename, NULL, "<src_name> <dst_name>", cmd_client_rename, 3, 0),
	SHELL_CMD_ARG(copy, NULL, "<src_name> <dst_name>", cmd_client_copy, 3, 0),
	SHELL_CMD_ARG(set_permission, NULL,
		      "<filename> <permission_mask>\n"
		      "Permission mask format (octet): [User][Group][Other]\n"
		      "Each octet: bit0=Read, bit1=Write, bit2=Delete, bit7=Modify\n"
		      "Example: 0x070505 = rwx for user, rx for group/other",
		      cmd_client_set_permission, 3, 0),
	SHELL_SUBCMD_SET_END);
#endif /* CONFIG_BT_FTP_CLIENT */

#if defined(CONFIG_BT_FTP_SERVER)
SHELL_STATIC_SUBCMD_SET_CREATE(
	ftp_server_cmds,
	SHELL_CMD_ARG(rfcomm_register, NULL, HELP_NONE, cmd_server_rfcomm_register, 1, 0),
	SHELL_CMD_ARG(rfcomm_disconnect, NULL, HELP_NONE, cmd_server_rfcomm_disconnect, 1, 0),
	SHELL_CMD_ARG(l2cap_register, NULL, HELP_NONE, cmd_server_l2cap_register, 1, 0),
	SHELL_CMD_ARG(l2cap_disconnect, NULL, HELP_NONE, cmd_server_l2cap_disconnect, 1, 0),
	SHELL_CMD_ARG(connect, NULL, "<rsp: unauth, success, error> [rsp_code] [password]",
		      cmd_server_connect, 2, 2),
	SHELL_CMD_ARG(disconnect, NULL, RSP_HELP_1, cmd_server_disconnect, 2, 1),
	SHELL_CMD_ARG(abort, NULL, RSP_HELP_1, cmd_server_abort, 2, 1),
	SHELL_CMD_ARG(set_folder, NULL, RSP_HELP_1, cmd_server_set_folder, 2, 1),
	SHELL_CMD_ARG(create_folder, NULL, RSP_HELP_1, cmd_server_set_folder, 2, 1),
	SHELL_CMD_ARG(pull_folder_listing, NULL, RSP_HELP_2, cmd_server_pull_folder_listing, 2, 3),
	SHELL_CMD_ARG(push_file, NULL, RSP_HELP_2, cmd_server_push_file, 2, 3),
	SHELL_CMD_ARG(pull_file, NULL, RSP_HELP_2, cmd_server_pull_file, 2, 3),
	SHELL_CMD_ARG(delete, NULL, RSP_HELP_1, cmd_server_delete, 2, 1),
	SHELL_CMD_ARG(rename, NULL, RSP_HELP_1, cmd_server_rename, 2, 1),
	SHELL_CMD_ARG(copy, NULL, RSP_HELP_1, cmd_server_copy, 2, 1),
	SHELL_CMD_ARG(set_permission, NULL, RSP_HELP_1, cmd_server_set_permission, 2, 1),
	SHELL_SUBCMD_SET_END);
#endif /* CONFIG_BT_FTP_SERVER */

SHELL_STATIC_SUBCMD_SET_CREATE(ftp_cmds,
#if defined(CONFIG_BT_FTP_CLIENT)
			       SHELL_CMD_ARG(client, &ftp_client_cmds, "FTP client commands",
					     cmd_common, 1, 0),
#endif /* CONFIG_BT_FTP_CLIENT */
#if defined(CONFIG_BT_FTP_SERVER)
			       SHELL_CMD_ARG(server, &ftp_server_cmds, "FTP server commands",
					     cmd_common, 1, 0),
#endif /* CONFIG_BT_FTP_SERVER */
			       SHELL_SUBCMD_SET_END);

SHELL_CMD_ARG_REGISTER(ftp, &ftp_cmds, "Bluetooth FTP shell commands", cmd_common, 1, 1);
