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

NET_BUF_POOL_FIXED_DEFINE(tx_pool, CONFIG_BT_MAX_CONN, BT_L2CAP_BUF_SIZE(CONFIG_BT_L2CAP_TX_MTU),
			  16, NULL);

static struct bt_spp *default_spp;
static struct net_buf *tx_buf;
static uint8_t spp_initied;
static const struct bt_uuid *uuid = BT_UUID_DECLARE_16(BT_SDP_SERIAL_PORT_SVCLASS);

void spp_connected_cb(struct bt_spp *spp_ins, uint8_t channel)
{
	default_spp = spp_ins;
	tx_buf = bt_spp_create_pdu(spp_ins, &tx_pool);
	bt_shell_print("SPP:%p, channel:%d connected ", spp_ins, channel);
}

void spp_disconnected_cb(struct bt_spp *spp_ins)
{
	default_spp = NULL;
	net_buf_unref(tx_buf);
	tx_buf = NULL;

	bt_shell_print("SPP:%p disconnected", spp_ins);
}

void spp_recv_cb(struct bt_spp *spp_ins, struct net_buf *buf)
{
	bt_shell_print("SPP:%p, data len:%d", spp_ins, buf->len);

	bt_shell_hexdump(buf->data, buf->len);
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
		shell_print(sh, "fail to register SPP callback, err:%d", err);
	} else {
		shell_print(sh, "register SPP callback success");
	}

	return 0;
}

static int cmd_register_srv(const struct shell *sh, int32_t argc, char *argv[])
{
	int err;
	uint8_t channel;

	if (spp_initied == 0) {
		shell_print(sh, "need to register SPP callback first");
		return -ENOEXEC;
	}

	if (argc < 2) {
		shell_print(sh, "please input channel");
		return -ENOEXEC;
	}

	channel = strtoul(argv[1], NULL, 16);

	err = bt_spp_register_srv(channel);
	if (err < 0) {
		shell_print(sh, "fail to register SPP service, err:%d", err);
	} else {
		shell_print(sh, "register spp srv channel:%d success", channel);
	}

	return err;
}

static int cmd_spp_connect(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	if (spp_initied == 0) {
		shell_print(sh, "need to register SPP callback first");
		return -ENOEXEC;
	}

	if (default_conn == NULL) {
		shell_error(sh, "please connect bt first");
		return -ENOEXEC;
	}

	if (default_spp != NULL) {
		shell_error(sh, "SPP already connected");
		return -EALREADY;
	}

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

	if (tx_buf == NULL) {
		shell_error(sh, "SPP tx_buf is NULL");
		return -ENOEXEC;
	}

	net_buf_add_mem(tx_buf, send_buf, ARRAY_SIZE(send_buf));
	bt_spp_send(default_spp, tx_buf);

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
