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

enum {
	SPP_IDLE,
	SPP_REGISTERED,
	SPP_CONNECTING,
	SPP_CONNECTED,
	SPP_DISCONNECTED,
};

#define DATA_MTU CONFIG_BT_SPP_L2CAP_MTU

NET_BUF_POOL_FIXED_DEFINE(tx_pool, CONFIG_BT_MAX_CONN, BT_L2CAP_BUF_SIZE(CONFIG_BT_SPP_L2CAP_MTU),
			  16, NULL);

static struct bt_spp default_spp;
static struct net_buf *tx_buf;
static uint8_t spp_state;
static struct bt_spp_server spp_server;
static union {
	struct bt_uuid_16 u16;
	struct bt_uuid_32 u32;
	struct bt_uuid_128 u128;
} spp_uuid;

static void spp_connected(struct bt_conn *conn, struct bt_spp *spp)
{
	spp_state = SPP_CONNECTED;

	bt_shell_print("SPP:%p, conn:%p connected ", spp, conn);
}

static void spp_disconnected(struct bt_spp *spp)
{
	spp_state = SPP_DISCONNECTED;

	bt_shell_print("SPP:%p disconnected", spp);
}

static void spp_recv(struct bt_spp *spp, struct net_buf *buf)
{
	bt_shell_print("SPP:%p, data len:%d", spp, buf->len);

	bt_shell_hexdump(buf->data, buf->len);
}

static struct bt_spp_ops spp_ops = {
	.connected = spp_connected,
	.disconnected = spp_disconnected,
	.recv = spp_recv,
};

static int spp_accept(struct bt_conn *conn, struct bt_spp_server *server, struct bt_spp **spp)
{
	if (spp_state == SPP_CONNECTED) {
		bt_shell_print("SPP already connected");
		return -EALREADY;
	}

	default_spp.ops = &spp_ops;
	spp_state = SPP_CONNECTED;

	*spp = &default_spp;

	return 0;
}

static int cmd_register_server(const struct shell *sh, int32_t argc, char *argv[])
{
	int err;
	uint8_t channel;
	struct bt_sdp_record *record;

	if (argc < 2) {
		shell_print(sh, "please input channel");
		return -ENOEXEC;
	}

	channel = strtoul(argv[1], NULL, 16);

	record = bt_spp_alloc_record(channel);
	if (record == NULL) {
		shell_print(sh, "fail to alloc sdp record");
		return -ENOEXEC;
	}

	spp_server.sdp_record = record;
	spp_server.accept = spp_accept;
	spp_server.rfcomm_server.channel = channel;

	err = bt_spp_server_register(&spp_server);
	if (err < 0) {
		shell_print(sh, "fail to register SPP service, err:%d", err);
		spp_server.sdp_record = NULL;
		return -ENOEXEC;
	}

	shell_print(sh, "register spp srv channel:%d success", channel);

	return err;
}

static int cmd_spp_connect(const struct shell *sh, size_t argc, char *argv[])
{
	int err;
	size_t len;

	if (default_conn == NULL) {
		shell_error(sh, "please connect bt first");
		return -ENOEXEC;
	}

	len = strlen(argv[1]);

	if (len == (BT_UUID_SIZE_16 * 2)) {
		uint16_t val;

		spp_uuid.u16.uuid.type = BT_UUID_TYPE_16;
		hex2bin(argv[1], len, (uint8_t *)&val, sizeof(val));
		spp_uuid.u16.val = sys_be16_to_cpu(val);
	} else if (len == (BT_UUID_SIZE_32 * 2)) {
		uint32_t val;

		spp_uuid.u32.uuid.type = BT_UUID_TYPE_32;
		hex2bin(argv[1], len, (uint8_t *)&val, sizeof(val));
		spp_uuid.u32.val = sys_be32_to_cpu(val);
	} else if (len == (BT_UUID_SIZE_128 * 2)) {
		uint8_t uuid128[BT_UUID_SIZE_128];

		spp_uuid.u128.uuid.type = BT_UUID_TYPE_128;
		hex2bin(argv[1], len, &uuid128[0], sizeof(uuid128));
		sys_memcpy_swap(spp_uuid.u128.val, uuid128, sizeof(uuid128));
	} else {
		shell_error(sh, "Invalid UUID");
		return -ENOEXEC;
	}

	default_spp.ops = &spp_ops;
	spp_state = SPP_CONNECTING;

	err = bt_spp_connect(default_conn, &default_spp, (struct bt_uuid *)&spp_uuid);
	if (err < 0) {
		shell_error(sh, "fail to connect spp device, err:%d", err);
		default_spp.ops = NULL;
		spp_state = SPP_DISCONNECTED;
		return -ENOEXEC;
	}

	return 0;
}

static int cmd_spp_send(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t data[DATA_MTU];
	int len = DATA_MTU;
	int err;

	memset(data, 0xff, sizeof(data));

	if (spp_state != SPP_CONNECTED) {
		shell_error(sh, "SPP is not connected");
		return -ENOEXEC;
	}

	tx_buf = bt_rfcomm_create_pdu(&tx_pool);
	if (tx_buf == NULL) {
		shell_error(sh, "SPP tx_buf is NULL");
		return -ENOEXEC;
	}

	if (argc > 1) {
		len = strtoul(argv[1], NULL, 10);
	}

	len = MIN(DATA_MTU, len);
	shell_print(sh, "Send data len:%d", len);

	net_buf_add_mem(tx_buf, data, len);

	err = bt_spp_send(&default_spp, tx_buf);
	if (err < 0) {
		shell_error(sh, "fail to send data, err:%d", err);
		net_buf_unref(tx_buf);
		return -ENOEXEC;
	}

	return 0;
}

static int cmd_spp_disconnect(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	if (spp_state != SPP_CONNECTED) {
		shell_error(sh, "SPP is not connected");
		return -ENOEXEC;
	}

	err = bt_spp_disconnect(&default_spp);
	if (err < 0) {
		shell_error(sh, "fail to disconnect spp, err:%d", err);
		return -ENOEXEC;
	}

	shell_print(sh, "spp disconnecting");

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	spp_cmds,
	SHELL_CMD_ARG(register_server, NULL, "register SPP server : <channel>", cmd_register_server,
		      2, 0),
	SHELL_CMD_ARG(connect, NULL, HELP_NONE, cmd_spp_connect, 2, 0),
	SHELL_CMD_ARG(send, NULL, "send [length of packet(s)]", cmd_spp_send, 1, 2),
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
