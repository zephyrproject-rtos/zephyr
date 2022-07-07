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
#include <zephyr/sys/byteorder.h>
#include <zephyr/zephyr.h>

#include <zephyr/settings/settings.h>

#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/bluetooth/rfcomm.h>
#include <zephyr/bluetooth/sdp.h>

#include <zephyr/shell/shell.h>

#include "bt.h"

#define DATA_MTU 48

NET_BUF_POOL_FIXED_DEFINE(pool, 1, DATA_MTU, 8, NULL);

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

static void rfcomm_recv(struct bt_rfcomm_dlc *dlci, struct net_buf *buf)
{
	shell_print(ctx_shell, "Incoming data dlc %p len %u", dlci, buf->len);
}

static void rfcomm_connected(struct bt_rfcomm_dlc *dlci)
{
	shell_print(ctx_shell, "Dlc %p connected", dlci);
}

static void rfcomm_disconnected(struct bt_rfcomm_dlc *dlci)
{
	shell_print(ctx_shell, "Dlc %p disconnected", dlci);
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
	shell_print(ctx_shell, "Incoming RFCOMM conn %p", conn);

	if (rfcomm_dlc.session) {
		shell_error(ctx_shell, "No channels available");
		return -ENOMEM;
	}

	*dlc = &rfcomm_dlc;

	return 0;
}

struct bt_rfcomm_server rfcomm_server = {
	.accept = &rfcomm_accept,
};

static int cmd_register(const struct shell *sh, size_t argc, char *argv[])
{
	int ret;

	if (rfcomm_server.channel) {
		shell_error(sh, "Already registered");
		return -ENOEXEC;
	}

	rfcomm_server.channel = BT_RFCOMM_CHAN_SPP;

	ret = bt_rfcomm_server_register(&rfcomm_server);
	if (ret < 0) {
		shell_error(sh, "Unable to register channel %x", ret);
		rfcomm_server.channel = 0U;
		return -ENOEXEC;
	} else {
		shell_print(sh, "RFCOMM channel %u registered",
			    rfcomm_server.channel);
		bt_sdp_register_service(&spp_rec);
	}

	return 0;
}

static int cmd_connect(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t channel;
	int err;

	if (!default_conn) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	channel = strtoul(argv[1], NULL, 16);

	err = bt_rfcomm_dlc_connect(default_conn, &rfcomm_dlc, channel);
	if (err < 0) {
		shell_error(sh, "Unable to connect to channel %d (err %u)",
			    channel, err);
	} else {
		shell_print(sh, "RFCOMM connection pending");
	}

	return err;
}

static int cmd_send(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t buf_data[DATA_MTU] = { [0 ... (DATA_MTU - 1)] = 0xff };
	int ret, len, count = 1;
	struct net_buf *buf;

	if (argc > 1) {
		count = strtoul(argv[1], NULL, 10);
	}

	while (count--) {
		buf = bt_rfcomm_create_pdu(&pool);
		/* Should reserve one byte in tail for FCS */
		len = MIN(rfcomm_dlc.mtu, net_buf_tailroom(buf) - 1);

		net_buf_add_mem(buf, buf_data, len);
		ret = bt_rfcomm_dlc_send(&rfcomm_dlc, buf);
		if (ret < 0) {
			shell_error(sh, "Unable to send: %d", -ret);
			net_buf_unref(buf);
			return -ENOEXEC;
		}
	}

	return 0;
}

static int cmd_disconnect(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	err = bt_rfcomm_dlc_disconnect(&rfcomm_dlc);
	if (err) {
		shell_error(sh, "Unable to disconnect: %u", -err);
	}

	return err;
}

#define HELP_NONE "[none]"
#define HELP_ADDR_LE "<address: XX:XX:XX:XX:XX:XX> <type: (public|random)>"

SHELL_STATIC_SUBCMD_SET_CREATE(rfcomm_cmds,
	SHELL_CMD_ARG(register, NULL, "<channel>", cmd_register, 2, 0),
	SHELL_CMD_ARG(connect, NULL, "<channel>", cmd_connect, 2, 0),
	SHELL_CMD_ARG(disconnect, NULL, HELP_NONE, cmd_disconnect, 1, 0),
	SHELL_CMD_ARG(send, NULL, "<number of packets>", cmd_send, 2, 0),
	SHELL_SUBCMD_SET_END
);

static int cmd_rfcomm(const struct shell *sh, size_t argc, char **argv)
{
	if (argc == 1) {
		shell_help(sh);
		/* shell returns 1 when help is printed */
		return 1;
	}

	shell_error(sh, "%s unknown parameter: %s", argv[0], argv[1]);

	return -ENOEXEC;
}

SHELL_CMD_ARG_REGISTER(rfcomm, &rfcomm_cmds, "Bluetooth RFCOMM shell commands",
		       cmd_rfcomm, 1, 1);
