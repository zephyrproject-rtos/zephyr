/** @file
 *  @brief Bluetooth Audio shell
 *
 */

/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <ctype.h>
#include <zephyr.h>
#include <shell/shell.h>
#include <sys/printk.h>
#include <sys/byteorder.h>
#include <sys/util.h>

#include <bluetooth/hci.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/iso.h>

#include "bt.h"

static void iso_recv(struct bt_iso_chan *chan, struct net_buf *buf)
{
	printk("Incoming data channel %p len %u\n", chan, buf->len);
}

static void iso_connected(struct bt_iso_chan *chan)
{
	printk("ISO Channel %p connected\n", chan);
}

static void iso_disconnected(struct bt_iso_chan *chan)
{
	printk("ISO Channel %p disconnected\n", chan);
}

static struct bt_iso_chan_ops iso_ops = {
	.recv		= iso_recv,
	.connected	= iso_connected,
	.disconnected	= iso_disconnected,
};

static struct bt_iso_chan_qos iso_qos = {
	.sca = 0x07,
};

static struct bt_iso_chan iso_chan = {
	.ops = &iso_ops,
	.qos = &iso_qos,
};

static int iso_accept(struct bt_conn *conn, struct bt_iso_chan **chan)
{
	printk("Incoming conn %p\n", conn);

	if (iso_chan.conn) {
		printk("No channels available\n");
		return -ENOMEM;
	}

	*chan = &iso_chan;

	return 0;
}

struct bt_iso_server iso_server = {
	.sec_level = BT_SECURITY_L1,
	.accept = iso_accept,
};

static int cmd_listen(const struct shell *shell, size_t argc, char *argv[])
{
	int err;

	if (argc > 1) {
		iso_server.sec_level = *argv[1] - '0';
	}

	err = bt_iso_server_register(&iso_server);
	if (err) {
		shell_error(shell, "Unable to register ISO cap (err %d)",
			    err);
	}

	return err;
}

static int cmd_bind(const struct shell *shell, size_t argc, char *argv[])
{
	struct bt_conn *conns[1];
	struct bt_iso_chan *chans[1];
	int err;

	if (!default_conn) {
		shell_error(shell, "Not connected");
		return 0;
	}

	if (iso_chan.conn) {
		shell_error(shell, "Already bound");
		return 0;
	}

	conns[0] = default_conn;
	chans[0] = &iso_chan;

	if (argc > 1) {
		chans[0]->qos->dir = strtol(argv[1], NULL, 0);
	}

	if (argc > 2) {
		chans[0]->qos->interval = strtol(argv[2], NULL, 0);
	}

	if (argc > 3) {
		chans[0]->qos->packing = strtol(argv[3], NULL, 0);
	}

	if (argc > 4) {
		chans[0]->qos->framing = strtol(argv[4], NULL, 0);
	}

	if (argc > 5) {
		chans[0]->qos->latency = strtol(argv[5], NULL, 0);
	}

	if (argc > 6) {
		chans[0]->qos->sdu = strtol(argv[6], NULL, 0);
	}

	if (argc > 7) {
		chans[0]->qos->phy = strtol(argv[7], NULL, 0);
	}

	if (argc > 8) {
		chans[0]->qos->rtn = strtol(argv[8], NULL, 0);
	}

	err = bt_iso_chan_bind(conns, 1, chans);
	if (err) {
		shell_error(shell, "Unable to bind (err %d)", err);
		return 0;
	}

	shell_print(shell, "ISO Channel bound");

	return 0;
}

static int cmd_connect(const struct shell *shell, size_t argc, char *argv[])
{
	struct bt_iso_chan *chans[1];
	int err;

	if (!iso_chan.conn) {
		shell_error(shell, "Not bound");
		return 0;
	}

	chans[0] = &iso_chan;

	err = bt_iso_chan_connect(chans, 1);
	if (err) {
		shell_error(shell, "Unable to connect (err %d)", err);
		return 0;
	}

	shell_print(shell, "ISO Connect pending...");

	return 0;
}

#define DATA_MTU CONFIG_BT_ISO_TX_MTU
NET_BUF_POOL_FIXED_DEFINE(tx_pool, 1, DATA_MTU, NULL);

static int cmd_send(const struct shell *shell, size_t argc, char *argv[])
{
	static uint8_t buf_data[DATA_MTU] = { [0 ... (DATA_MTU - 1)] = 0xff };
	int ret, len, count = 1;
	struct net_buf *buf;

	if (argc > 1) {
		count = strtoul(argv[1], NULL, 10);
	}

	if (!iso_chan.conn) {
		shell_error(shell, "Not bound");
		return 0;
	}

	len = MIN(iso_chan.qos->sdu, DATA_MTU - BT_ISO_CHAN_SEND_RESERVE);

	while (count--) {
		buf = net_buf_alloc(&tx_pool, K_FOREVER);
		net_buf_reserve(buf, BT_ISO_CHAN_SEND_RESERVE);

		net_buf_add_mem(buf, buf_data, len);
		ret = bt_iso_chan_send(&iso_chan, buf);
		if (ret < 0) {
			shell_print(shell, "Unable to send: %d", -ret);
			net_buf_unref(buf);
			return -ENOEXEC;
		}
	}

	shell_print(shell, "ISO sending...");

	return 0;
}

static int cmd_disconnect(const struct shell *shell, size_t argc,
			      char *argv[])
{
	int err;

	err = bt_iso_chan_disconnect(&iso_chan);
	if (err) {
		shell_error(shell, "Unable to disconnect (err %d)", err);
		return 0;
	}

	shell_print(shell, "ISO Disconnect pending...");

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(iso_cmds,
	SHELL_CMD_ARG(bind, NULL, "[dir] [interval] [packing] [framing] "
		      "[latency] [sdu] [phy] [rtn]", cmd_bind, 1, 8),
	SHELL_CMD_ARG(connect, NULL, "Connect ISO Channel", cmd_connect, 1, 0),
	SHELL_CMD_ARG(listen, NULL, "[security level]", cmd_listen, 1, 1),
	SHELL_CMD_ARG(send, NULL, "Send to ISO Channel [count]",
		      cmd_send, 1, 1),
	SHELL_CMD_ARG(disconnect, NULL, "Disconnect ISO Channel",
		      cmd_disconnect, 1, 0),
	SHELL_SUBCMD_SET_END
);

static int cmd_iso(const struct shell *shell, size_t argc, char **argv)
{
	if (argc > 1) {
		shell_error(shell, "%s unknown parameter: %s",
			    argv[0], argv[1]);
	} else {
		shell_error(shell, "%s Missing subcommand", argv[0]);
	}

	return -ENOEXEC;
}

SHELL_CMD_ARG_REGISTER(iso, &iso_cmds, "Bluetooth ISO shell commands",
		       cmd_iso, 1, 1);
