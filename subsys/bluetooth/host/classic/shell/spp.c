/*
 * Copyright 2025 Xiaomi Corporation
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

#include "host/shell/bt.h"
#include "common/bt_shell_private.h"

#define HELP_NONE "[none]"

static struct bt_spp *default_spp;
static uint8_t spp_initied;

void spp_connected_cb(struct bt_spp *spp_ins, uint8_t channel)
{
	default_spp = spp_ins;

	bt_shell_print("SPP:%p, channel:%d connected ", spp_ins, channel);
}

void spp_disconnected_cb(struct bt_spp *spp_ins, uint8_t channel)
{
	default_spp = NULL;

	bt_shell_print("SPP:%p disconnected", spp_ins);
}

void spp_recv_cb(struct bt_spp *spp_ins, uint8_t channel, const uint8_t *data, uint16_t len)
{
	bt_shell_print("SPP:%p, channel:%d, data len:%d", spp_ins, channel, len);
	bt_shell_hexdump(data, len);
}

static struct bt_spp_cb spp_cb = {
	.connected = spp_connected_cb,
	.disconnected = spp_disconnected_cb,
	.recv = spp_recv_cb,
};

static int cmd_register_cb(const struct shell *sh, int32_t argc, char *argv[])
{
	int err;

	if (spp_initied) {
		shell_print(sh, "already registered SPP callbacks");
		return 0;
	}

	spp_initied = 1;
	err = bt_spp_register_cb(&spp_cb);
	if (err < 0) {
		shell_print(sh, "fail to register SPP callback");
	} else {
		shell_print(sh, "register SPP callback success");
	}

	return 0;
}

static int cmd_register_srv(const struct shell *sh, int32_t argc, char *argv[])
{
	int ret;
	uint8_t channel;
	static const struct bt_uuid *uuid;

	if (spp_initied == 0) {
		shell_print(sh, "need to register SPP callback first");
		return -ENOEXEC;
	}

	if (argc < 2) {
		shell_print(sh, "please input channel");
		return -ENOEXEC;
	}

	channel = strtoul(argv[1], NULL, 16);
	uuid = BT_UUID_DECLARE_16(BT_SDP_SERIAL_PORT_SVCLASS);

	ret = bt_spp_register_srv(channel, uuid);
	if (ret != 0) {
		shell_print(sh, "fail");
		return ret;
	}

	shell_print(sh, "register spp srv channel:%d success", channel);
	return ret;
}

static int cmd_spp_connect(const struct shell *sh, size_t argc, char *argv[])
{
	static const struct bt_uuid *uuid;
	int err;

	if (spp_initied == 0) {
		shell_print(sh, "need to register SPP callback first");
		return -ENOEXEC;
	}

	if (default_conn == NULL) {
		shell_error(sh, "please connect bt first");
		return -ENOEXEC;
	}

	uuid = BT_UUID_DECLARE_16(BT_SDP_SERIAL_PORT_SVCLASS);
	err = bt_spp_connect(default_conn, &default_spp, uuid);
	if (err < 0) {
		shell_error(sh, "fail to connect spp device");
	}

	return 0;
}

static int cmd_spp_send(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t send_buf[] = {0, 1, 2, 3, 4, 5};

	if (spp_initied == 0) {
		shell_print(sh, "need to register SPP callback first");
		return -ENOEXEC;
	}

	if (default_spp == NULL) {
		shell_error(sh, "SPP is not connected");
		return -ENOEXEC;
	}

	bt_spp_send(default_spp, send_buf, ARRAY_SIZE(send_buf));

	return 0;
}

static int cmd_spp_disconnect(const struct shell *sh, size_t argc, char *argv[])
{
	if (spp_initied == 0) {
		shell_print(sh, "need to register SPP callback first");
		return -ENOEXEC;
	}

	if (default_spp == NULL) {
		shell_error(sh, "SPP is not connected");
		return -ENOEXEC;
	}

	bt_spp_disconnect(default_spp);
	default_spp = NULL;

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	spp_cmds, SHELL_CMD_ARG(register_cb, NULL, "register SPP callbacks", cmd_register_cb, 1, 0),
	SHELL_CMD_ARG(register_srv, NULL, "register SPP service : <channel>", cmd_register_srv, 2,
		      0),
	SHELL_CMD_ARG(connect, NULL, HELP_NONE, cmd_spp_connect, 1, 0),
	SHELL_CMD_ARG(send, NULL, HELP_NONE, cmd_spp_send, 1, 0),
	SHELL_CMD_ARG(disconnect, NULL, HELP_NONE, cmd_spp_disconnect, 1, 0), SHELL_SUBCMD_SET_END);

static int cmd_spp(const struct shell *sh, size_t argc, char **argv)
{
	if (argc == 1) {
		shell_help(sh);
		/* sh returns 1 when help is printed */
		return SHELL_CMD_HELP_PRINTED;
	}

	shell_error(sh, "%s unknown parameter: %s", argv[0], argv[1]);

	return -ENOEXEC;
}

SHELL_CMD_ARG_REGISTER(spp, &spp_cmds, "Bluetooth SPP sh commands", cmd_spp, 1, 1);
