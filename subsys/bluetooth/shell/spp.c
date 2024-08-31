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
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/classic/spp.h>
#include <zephyr/bluetooth/classic/sdp.h>

#include <zephyr/shell/shell.h>

#include "bt.h"

#define HELP_NONE "[none]"

static struct bt_spp *spp;

void spp_connected_cb(struct bt_spp *spp_ins, uint8_t port)
{
	spp = spp_ins;

	shell_print(ctx_shell, "spp:%p, port:%d connected ", spp_ins, port);
}

void spp_disconnected_cb(struct bt_spp *spp_ins, uint8_t port)
{
	spp = NULL;

	shell_print(ctx_shell, "spp:%p disconnected", spp_ins);
}

void spp_recv_cb(struct bt_spp *spp_ins, uint8_t port, uint8_t *data, uint16_t len)
{
	shell_print(ctx_shell, "spp:%p, port:%d, data len:%d", spp_ins, port, len);
}

static struct bt_spp_cb spp_cb = {
	.connected = spp_connected_cb,
	.disconnected = spp_disconnected_cb,
	.recv = spp_recv_cb,
};

static int cmd_register_cb(const struct shell *sh, int32_t argc, char *argv[])
{
	bt_spp_register_cb(&spp_cb);

	shell_print(sh, "register spp cb success");
	return 0;
}

static int cmd_register_srv(const struct shell *sh, int32_t argc, char *argv[])
{
	int ret;
	uint8_t port;
	const struct bt_uuid *uuid = BT_UUID_DECLARE_16(BT_SDP_SERIAL_PORT_SVCLASS);

	if (argc < 2) {
		shell_print(sh, "please input port");
		return -ENOEXEC;
	}

	port = strtoul(argv[1], NULL, 16);

	ret = bt_spp_register_srv(port, uuid);
	if (ret != 0) {
		shell_print(sh, "fail");
		return ret;
	}

	shell_print(sh, "register spp srv port:%d success", port);
	return ret;
}

static int cmd_spp_connect(const struct shell *sh, size_t argc, char *argv[])
{
	const struct bt_uuid *uuid = BT_UUID_DECLARE_16(BT_SDP_SERIAL_PORT_SVCLASS);

	if (!default_conn) {
		shell_error(sh, "please connect bt first");
		return -ENOEXEC;
	}

	spp = bt_spp_connect(default_conn, uuid);
	if (!spp) {
		shell_error(sh, "fail to connect spp device");
	}

	return 0;
}

static int cmd_spp_send(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t send_buf[] = {0, 1, 2, 3, 4, 5};

	if (!spp) {
		shell_print(sh, "spp not connected");
		return -ENOEXEC;
	}

	bt_spp_send(spp, send_buf, ARRAY_SIZE(send_buf));

	return 0;
}

static int cmd_spp_disconnect(const struct shell *sh, size_t argc, char *argv[])
{
	if (!spp) {
		shell_print(sh, "spp not connected");
		return -ENOEXEC;
	}

	bt_spp_disconnect(spp);

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	spp_cmds, SHELL_CMD_ARG(register_cb, NULL, "register SPP callbacks", cmd_register_cb, 1, 0),
	SHELL_CMD_ARG(register_srv, NULL, "register SPP service : <port>", cmd_register_srv, 2, 0),
	SHELL_CMD_ARG(connect, NULL, HELP_NONE, cmd_spp_connect, 1, 0),
	SHELL_CMD_ARG(send, NULL, HELP_NONE, cmd_spp_send, 1, 0),
	SHELL_CMD_ARG(disconnect, NULL, HELP_NONE, cmd_spp_disconnect, 1, 0), SHELL_SUBCMD_SET_END);

static int cmd_spp(const struct shell *sh, size_t argc, char **argv)
{
	if (argc == 1) {
		shell_help(sh);
		return SHELL_CMD_HELP_PRINTED;
	}

	shell_error(sh, "%s unknown parameter: %s", argv[0], argv[1]);

	return -ENOEXEC;
}

SHELL_CMD_ARG_REGISTER(spp, &spp_cmds, "Bluetooth SPP sh commands", cmd_spp, 1, 1);
