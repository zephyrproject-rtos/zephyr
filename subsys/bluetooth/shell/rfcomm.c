/** @file
 * @brief Bluetooth RFCOMM shell module
 *
 * Provide some Bluetooth shell commands that can be useful to applications.
 */

/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/types.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <misc/byteorder.h>
#include <zephyr.h>

#include <settings/settings.h>

#include <bluetooth/hci.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/l2cap.h>
#include <bluetooth/rfcomm.h>
#include <bluetooth/sdp.h>

#include <shell/shell.h>

#include "bt.h"

#define DATA_MTU 48

NET_BUF_POOL_DEFINE(pool, 1, DATA_MTU, BT_BUF_USER_DATA_MIN, NULL);

static struct bt_sdp_attribute spp_attrs[] = {
	BT_SDP_NEW_SERVICE,
	BT_SDP_LIST(
		BT_SDP_ATTR_SVCLASS_ID_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
			BT_SDP_ARRAY_16(BT_SDP_SERIAL_PORT_SVCLASS)
		},
		)
	),
	BT_SDP_LIST(
		BT_SDP_ATTR_PROTO_DESC_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 12),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3),
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
				BT_SDP_ARRAY_16(BT_SDP_PROTO_L2CAP)
			},
			)
		},
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 5),
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
				BT_SDP_ARRAY_16(BT_SDP_PROTO_RFCOMM)
			},
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UINT8),
				BT_SDP_ARRAY_8(BT_RFCOMM_CHAN_SPP)
			},
			)
		},
		)
	),
	BT_SDP_LIST(
		BT_SDP_ATTR_PROFILE_DESC_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 8),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
				BT_SDP_ARRAY_16(BT_SDP_SERIAL_PORT_SVCLASS)
			},
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
				BT_SDP_ARRAY_16(0x0102)
			},
			)
		},
		)
	),
	BT_SDP_SERVICE_NAME("Serial Port"),
};

static struct bt_sdp_record spp_rec = BT_SDP_RECORD(spp_attrs);
static const struct shell *rf_shell;

static void rfcomm_recv(struct bt_rfcomm_dlc *dlci, struct net_buf *buf)
{
	print(rf_shell, "Incoming data dlc %p len %u\n", dlci,
		     buf->len);
}

static void rfcomm_connected(struct bt_rfcomm_dlc *dlci)
{
	print(rf_shell, "Dlc %p connected\n", dlci);
}

static void rfcomm_disconnected(struct bt_rfcomm_dlc *dlci)
{
	print(rf_shell, "Dlc %p disconnected\n", dlci);
}

static struct bt_rfcomm_dlc_ops rfcomm_ops = {
	.recv		= rfcomm_recv,
	.connected	= rfcomm_connected,
	.disconnected	= rfcomm_disconnected,
};

static struct bt_rfcomm_dlc rfcomm_dlc = {
	.ops = &rfcomm_ops,
	.mtu = 30,
};

static int rfcomm_accept(struct bt_conn *conn, struct bt_rfcomm_dlc **dlc)
{
	print(rf_shell, "Incoming RFCOMM conn %p\n", conn);

	if (rfcomm_dlc.session) {
		error(rf_shell, "No channels available\n");
		return -ENOMEM;
	}

	*dlc = &rfcomm_dlc;

	return 0;
}

struct bt_rfcomm_server rfcomm_server = {
	.accept = &rfcomm_accept,
};

static void cmd_register(const struct shell *shell, size_t argc, char *argv[])
{
	int ret;

	if (rfcomm_server.channel) {
		error(shell, "Already registered\n");
		return;
	}

	rf_shell = shell;
	rfcomm_server.channel = BT_RFCOMM_CHAN_SPP;

	ret = bt_rfcomm_server_register(&rfcomm_server);
	if (ret < 0) {
		error(shell, "Unable to register channel %x\n", ret);
		rfcomm_server.channel = 0;
	} else {
		print(shell, "RFCOMM channel %u registered\n",
			     rfcomm_server.channel);
		bt_sdp_register_service(&spp_rec);
	}
}

static void cmd_connect(const struct shell *shell, size_t argc, char *argv[])
{
	u8_t channel;
	int err;

	if (!default_conn) {
		error(shell, "Not connected\n");
		return;
	}

	if (!shell_cmd_precheck(shell, argc == 2, NULL, 0)) {
		return;
	}

	channel = strtoul(argv[1], NULL, 16);

	err = bt_rfcomm_dlc_connect(default_conn, &rfcomm_dlc, channel);
	if (err < 0) {
		error(shell, "Unable to connect to channel %d (err %u)\n",
			    channel, err);
	} else {
		print(shell, "RFCOMM connection pending\n");
	}
}

static void cmd_send(const struct shell *shell, size_t argc, char *argv[])
{
	u8_t buf_data[DATA_MTU] = { [0 ... (DATA_MTU - 1)] = 0xff };
	int ret, len, count = 1;
	struct net_buf *buf;

	if (argc > 1) {
		count = strtoul(argv[1], NULL, 10);
	}

	while (count--) {
		buf = bt_rfcomm_create_pdu(&pool);
		/* Should reserve one byte in tail for FCS */
		len = min(rfcomm_dlc.mtu, net_buf_tailroom(buf) - 1);

		net_buf_add_mem(buf, buf_data, len);
		ret = bt_rfcomm_dlc_send(&rfcomm_dlc, buf);
		if (ret < 0) {
			error(shell, "Unable to send: %d\n", -ret);
			net_buf_unref(buf);
			break;
		}
	}
}

static void cmd_disconnect(const struct shell *shell, size_t argc, char *argv[])
{
	int err;

	err = bt_rfcomm_dlc_disconnect(&rfcomm_dlc);
	if (err) {
		error(shell, "Unable to disconnect: %u\n", -err);
	}
}

#define HELP_NONE "[none]"
#define HELP_ADDR_LE "<address: XX:XX:XX:XX:XX:XX> <type: (public|random)>"

SHELL_CREATE_STATIC_SUBCMD_SET(rfcomm_cmds) {
	SHELL_CMD(register, NULL, "<channel>", cmd_register),
	SHELL_CMD(connect, NULL, "<channel>", cmd_connect),
	SHELL_CMD(disconnect, NULL, HELP_NONE, cmd_disconnect),
	SHELL_CMD(send, NULL, "<number of packets>", cmd_send),
	SHELL_SUBCMD_SET_END
};

static void cmd_rfcomm(const struct shell *shell, size_t argc, char **argv)
{
	if (argc == 1) {
		shell_help_print(shell, NULL, 0);
		return;
	}

	if (!shell_cmd_precheck(shell, (argc == 2), NULL, 0)) {
		return;
	}

	error(shell, "%s:%s%s\r\n", argv[0], "unknown parameter: ", argv[1]);
}

SHELL_CMD_REGISTER(rfcomm, &rfcomm_cmds, "Bluetooth RFCOMM shell commands",
		   cmd_rfcomm);

