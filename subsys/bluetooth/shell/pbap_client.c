/*
 * Copyright 2024 Xiaomi Corporation
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

#include <zephyr/settings/settings.h>

#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/classic/pbap_client.h>
#include <zephyr/shell/shell.h>

#include "bt.h"

#define PBAP_PB_NAME  "telecom/pb.vcf"
#define PBAP_PB_PATH  "telecom/pb"
#define PBAP_PB_VCARD "0.vcf"

#define HELP_NONE "[none]"

struct bt_pbap_client *default_pbap;

void pbap_client_connected(struct bt_pbap_client *client)
{
	shell_print(ctx_shell, "pbap connected:%p", client);
}

void pbap_client_disconnected(struct bt_pbap_client *client)
{
	shell_print(ctx_shell, "pbap disconnected:%p", client);
}

void pbap_client_recv(struct bt_pbap_client *client, struct bt_pbap_result *result)
{
	shell_print(ctx_shell, "pbap client:%p, event:%d", client, result->event);
}

struct bt_pbap_client_cb pbap_client_cb = {
	.connected = pbap_client_connected,
	.disconnected = pbap_client_disconnected,
	.recv = pbap_client_recv,
};

static int cmd_connect(const struct shell *sh, int32_t argc, char *argv[])
{
	if (!default_conn) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	default_pbap = bt_pbap_client_connect(default_conn, &pbap_client_cb);
	if (!default_pbap) {
		shell_error(sh, "pbap client fail to connect");
	}

	return 0;
}

static int cmd_disconnect(const struct shell *sh, int32_t argc, char *argv[])
{
	int ret;

	if (!default_pbap) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	ret = bt_pbap_client_disconnect(default_pbap);
	if (ret < 0) {
		shell_error(sh, "pbap client fail to disconnect pbap client, err:%d", ret);
	}

	return 0;
}

static int cmd_get_size(const struct shell *sh, int32_t argc, char *argv[])
{
	int ret;
	struct pbap_client_param param = {0};

	if (!default_pbap) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	param.op_cmd = PBAP_CLIENT_OP_CMD_GET_SIZE;
	param.path = PBAP_PB_NAME;

	ret = bt_pbap_client_request(default_pbap, &param);
	if (ret < 0) {
		shell_error(sh, "pbap client fail to get size, err:%d", ret);
	}

	return 0;
}

static int cmd_get_pb(const struct shell *sh, int32_t argc, char *argv[])
{
	int ret;
	struct pbap_client_param param = {0};

	if (!default_pbap) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	param.op_cmd = PBAP_CLIENT_OP_CMD_GET_PB;
	param.path = PBAP_PB_NAME;

	ret = bt_pbap_client_request(default_pbap, &param);
	if (ret) {
		shell_error(sh, "pbap client fail to get pb, err:%d", ret);
	}

	return 0;
}

static int cmd_set_path(const struct shell *sh, int32_t argc, char *argv[])
{
	int ret;
	struct pbap_client_param param = {0};

	if (!default_pbap) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	param.op_cmd = PBAP_CLIENT_OP_CMD_SET_PATH;
	param.path = PBAP_PB_PATH;

	ret = bt_pbap_client_request(default_pbap, &param);
	if (ret) {
		shell_error(sh, "pbap client fail to set patch, err:%d", ret);
	}

	return 0;
}

static int cmd_get_vcard(const struct shell *sh, int32_t argc, char *argv[])
{
	int ret;
	struct pbap_client_param param = {0};

	if (!default_pbap) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	param.op_cmd = PBAP_CLIENT_OP_CMD_GET_VCARD;
	param.vcard_name = PBAP_PB_VCARD;
	param.vcard_name_len = strlen(PBAP_PB_VCARD) + 1;

	ret = bt_pbap_client_request(default_pbap, &param);
	if (ret) {
		shell_error(sh, "pbap client fail to get vcard, err:%d", ret);
	}

	return 0;
}

static int cmd_get_continue(const struct shell *sh, int32_t argc, char *argv[])
{
	int ret;
	struct pbap_client_param param = {0};

	if (!default_pbap) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	param.op_cmd = PBAP_CLIENT_OP_CMD_GET_CONTINUE;

	ret = bt_pbap_client_request(default_pbap, &param);
	if (ret) {
		shell_error(sh, "pbap client fail to get continue, err:%d", ret);
	}

	return 0;
}

static int cmd_listing(const struct shell *sh, int32_t argc, char *argv[])
{
	int ret;
	struct pbap_client_param param = {0};

	if (!default_pbap) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	param.op_cmd = PBAP_CLIENT_OP_CMD_LISTING;
	param.search_attr = 1;

	ret = bt_pbap_client_request(default_pbap, &param);
	if (ret) {
		shell_error(sh, "pbap client fail to listing, err:%d", ret);
	}

	return 0;
}

static int cmd_abort(const struct shell *sh, int32_t argc, char *argv[])
{
	int ret;
	struct pbap_client_param param = {0};

	if (!default_pbap) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	param.op_cmd = PBAP_CLIENT_OP_CMD_ABORT;

	ret = bt_pbap_client_request(default_pbap, &param);
	if (ret) {
		shell_error(sh, "pbap client fail to abort, err:%d", ret);
	}

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(pbap_client_cmds,
			       SHELL_CMD_ARG(connect, NULL, HELP_NONE, cmd_connect, 1, 0),
			       SHELL_CMD_ARG(disconnect, NULL, HELP_NONE, cmd_disconnect, 1, 0),
			       SHELL_CMD_ARG(get_size, NULL, HELP_NONE, cmd_get_size, 1, 0),
			       SHELL_CMD_ARG(get_pb, NULL, HELP_NONE, cmd_get_pb, 1, 0),
			       SHELL_CMD_ARG(set_path, NULL, HELP_NONE, cmd_set_path, 1, 0),
			       SHELL_CMD_ARG(get_vcard, NULL, HELP_NONE, cmd_get_vcard, 1, 0),
			       SHELL_CMD_ARG(get_continue, NULL, HELP_NONE, cmd_get_continue, 1, 0),
			       SHELL_CMD_ARG(listing, NULL, HELP_NONE, cmd_listing, 1, 0),
			       SHELL_CMD_ARG(abort, NULL, HELP_NONE, cmd_abort, 1, 0),
			       SHELL_SUBCMD_SET_END);

static int cmd_pbap(const struct shell *sh, size_t argc, char **argv)
{
	if (argc == 1) {
		shell_help(sh);
		/* sh returns 1 when help is printed */
		return 1;
	}

	shell_error(sh, "%s unknown parameter: %s", argv[0], argv[1]);

	return -ENOEXEC;
}

SHELL_CMD_ARG_REGISTER(pbapc, &pbap_client_cmds, "Bluetooth PBAP Client sh commands", cmd_pbap, 1,
		       1);
