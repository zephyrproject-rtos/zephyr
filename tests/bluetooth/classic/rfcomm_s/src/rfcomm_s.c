/* rfcomm_s.c - Bluetooth classic rfcomm_s smoke test */

/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdlib.h>
#include <string.h>

#include <errno.h>
#include <zephyr/types.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/shell/shell.h>

/* Include stack file */
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/bluetooth/classic/rfcomm.h>
#include <zephyr/bluetooth/classic/sdp.h>
#include "host/shell/bt.h"
#include "common/bt_shell_private.h"

#define DATA_MTU 48

NET_BUF_POOL_FIXED_DEFINE(pool, 1, DATA_MTU, CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

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

/* DLC entity */
static void rfcomm_recv(struct bt_rfcomm_dlc *dlci, struct net_buf *buf)
{
	bt_shell_print("Incoming data dlc %p len %u", dlci, buf->len);
}

static void rfcomm_connected(struct bt_rfcomm_dlc *dlci)
{
	bt_shell_print("Dlc %p connected", dlci);
}

static void rfcomm_disconnected(struct bt_rfcomm_dlc *dlci)
{
	bt_shell_print("Dlc %p disconnected", dlci);
}

static struct bt_rfcomm_dlc_ops rfcomm_ops = {
	.recv		= rfcomm_recv,
	.connected	= rfcomm_connected,
	.disconnected	= rfcomm_disconnected,
};

static struct bt_rfcomm_dlc *rfcomm_dlc;

static struct bt_rfcomm_dlc rfcomm_dlc_9 = {
	.ops = &rfcomm_ops,
	.mtu = 30,
};

static struct bt_rfcomm_dlc rfcomm_dlc_7 = {
	.ops = &rfcomm_ops,
	.mtu = 30,
};

/* RFCOMM server entity */
static int rfcomm_accept(struct bt_conn *conn, struct bt_rfcomm_server *server,
			 struct bt_rfcomm_dlc **dlc)
{
	bt_shell_print("Incoming RFCOMM conn %p", conn);

	if (server->channel == 9U) {
		rfcomm_dlc = &rfcomm_dlc_9;
	} else if (server->channel == 7U) {
		rfcomm_dlc = &rfcomm_dlc_7;
	} else {
		bt_shell_error("No channels available");
		return -ENOMEM;
	}

	if (rfcomm_dlc->session) {
		bt_shell_error("No channels available");
		return -ENOMEM;
	}

	*dlc = rfcomm_dlc;

	return 0;
}

static struct bt_rfcomm_server *rfcomm_server;

static struct bt_rfcomm_server rfcomm_server_9 = {
	.accept = &rfcomm_accept,
};

static struct bt_rfcomm_server rfcomm_server_7 = {
	.accept = &rfcomm_accept,
};

/* RFCOMM shell command */
static int cmd_register(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t channel;
	int err;

	channel = strtoul(argv[1], NULL, 16);
	if (channel == 9U) {
		rfcomm_server = &rfcomm_server_9;
	} else if (channel == 7U) {
		rfcomm_server = &rfcomm_server_7;
	} else {
		shell_print(sh, "Channel %u isn't supported, just support channel 9 and 7",
			    channel);
		return -ENOEXEC;
	}

	if (rfcomm_server->channel) {
		shell_error(sh, "Already registered");
		return -ENOEXEC;
	}
	rfcomm_server->channel = channel;

	err = bt_rfcomm_server_register(rfcomm_server);
	if (err < 0) {
		shell_error(sh, "Unable to register channel %x", err);
		rfcomm_server->channel = 0U;
		return -ENOEXEC;
	}
	shell_print(sh, "RFCOMM channel %u registered", rfcomm_server->channel);
	bt_sdp_register_service(&spp_rec);

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
	if (channel == 9U) {
		rfcomm_dlc = &rfcomm_dlc_9;
	} else if (channel == 7U) {
		rfcomm_dlc = &rfcomm_dlc_7;
	} else {
		shell_print(sh, "Channel %u isn't supported, just support channel 9 and 7",
			    channel);
		return -ENOEXEC;
	}

	if (rfcomm_dlc->session) {
		bt_shell_error("Channel %u is not available", channel);
		return -ENOMEM;
	}

	err = bt_rfcomm_dlc_connect(default_conn, rfcomm_dlc, channel);
	if (err < 0) {
		shell_error(sh, "Unable to connect to channel %d (err %u)", channel, err);
	} else {
		shell_print(sh, "RFCOMM connection pending");
	}

	return err;
}

static int cmd_disconnect(const struct shell *sh, size_t argc, char *argv[])
{
	int err;
	uint8_t channel;

	channel = strtoul(argv[1], NULL, 16);
	if (channel == 9U) {
		rfcomm_dlc = &rfcomm_dlc_9;
	} else if (channel == 7U) {
		rfcomm_dlc = &rfcomm_dlc_7;
	} else {
		shell_print(sh, "Channel %u isn't supported, just support channel 9 and 7",
			    channel);
		return -ENOEXEC;
	}

	err = bt_rfcomm_dlc_disconnect(rfcomm_dlc);
	if (err) {
		shell_error(sh, "Unable to disconnect: %u", -err);
	}

	return err;
}

static int cmd_send(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t buf_data[DATA_MTU];
	uint32_t count;
	int err, len;
	struct net_buf *buf;
	uint8_t channel;

	memset(buf_data, 0xff, sizeof(buf_data));

	channel = strtoul(argv[1], NULL, 16);
	if (channel == 9U) {
		rfcomm_dlc = &rfcomm_dlc_9;
		shell_print(sh, "Send data on channel 9");
	} else if (channel == 7U) {
		rfcomm_dlc = &rfcomm_dlc_7;
		shell_print(sh, "Send data on channel 7");
	} else {
		shell_print(sh, "Channel %u isn't supported, just support channel 9 and 7",
			    channel);
		return -ENOEXEC;
	}

	count = strtoul(argv[2], NULL, 10);

	while (count--) {
		buf = bt_rfcomm_create_pdu(&pool);
		/* Should reserve one byte in tail for FCS */
		len = MIN(rfcomm_dlc->mtu, net_buf_tailroom(buf) - 1);

		net_buf_add_mem(buf, buf_data, len);
		err = bt_rfcomm_dlc_send(rfcomm_dlc, buf);
		if (err < 0) {
			shell_error(sh, "Unable to send: %d", -err);
			net_buf_unref(buf);
			return -ENOEXEC;
		}
	}

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(rfcomm_s_cmds,
	SHELL_CMD_ARG(register, NULL, "<server channel>", cmd_register, 2, 0),
	SHELL_CMD_ARG(connect, NULL, "<server channel>", cmd_connect, 2, 0),
	SHELL_CMD_ARG(disconnect, NULL, "<server channel>", cmd_disconnect, 2, 0),
	SHELL_CMD_ARG(send, NULL, "<server channel> <data>", cmd_send, 3, 0),
	SHELL_SUBCMD_SET_END
);

static int cmd_default_handler(const struct shell *sh, size_t argc, char **argv)
{
	if (argc == 1) {
		shell_help(sh);
		return SHELL_CMD_HELP_PRINTED;
	}

	shell_error(sh, "%s unknown parameter: %s", argv[0], argv[1]);

	return -EINVAL;
}

SHELL_CMD_ARG_REGISTER(rfcomm_s, &rfcomm_s_cmds, "Bluetooth classic rfcomm shell commands",
		       cmd_default_handler, 1, 1);
