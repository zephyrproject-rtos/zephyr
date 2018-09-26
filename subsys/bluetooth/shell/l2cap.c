/** @file
 * @brief Bluetooth shell module
 *
 * Provide some Bluetooth shell commands that can be useful to applications.
 */

/*
 * Copyright (c) 2017 Intel Corporation
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

#define CREDITS			10
#define DATA_MTU		(23 * CREDITS)

NET_BUF_POOL_DEFINE(data_tx_pool, 1, DATA_MTU, BT_BUF_USER_DATA_MIN, NULL);
NET_BUF_POOL_DEFINE(data_rx_pool, 1, DATA_MTU, BT_BUF_USER_DATA_MIN, NULL);

static u32_t l2cap_rate;
static u32_t l2cap_recv_delay;
static K_FIFO_DEFINE(l2cap_recv_fifo);
struct l2ch {
	struct k_delayed_work recv_work;
	struct bt_l2cap_le_chan ch;
};
#define L2CH_CHAN(_chan) CONTAINER_OF(_chan, struct l2ch, ch.chan)
#define L2CH_WORK(_work) CONTAINER_OF(_work, struct l2ch, recv_work)
#define L2CAP_CHAN(_chan) _chan->ch.chan

static int l2cap_recv_metrics(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	static u32_t len;
	static u32_t cycle_stamp;
	u32_t delta;

	delta = k_cycle_get_32() - cycle_stamp;
	delta = SYS_CLOCK_HW_CYCLES_TO_NS(delta);

	/* if last data rx-ed was greater than 1 second in the past,
	 * reset the metrics.
	 */
	if (delta > 1000000000) {
		len = 0;
		l2cap_rate = 0;
		cycle_stamp = k_cycle_get_32();
	} else {
		len += buf->len;
		l2cap_rate = ((u64_t)len << 3) * 1000000000 / delta;
	}

	return 0;
}

static void l2cap_recv_cb(struct k_work *work)
{
	struct l2ch *c = L2CH_WORK(work);
	struct net_buf *buf;

	while ((buf = net_buf_get(&l2cap_recv_fifo, K_NO_WAIT))) {
		print(ctx_shell, "Confirming reception");
		bt_l2cap_chan_recv_complete(&c->ch.chan, buf);
	}
}

static int l2cap_recv(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	struct l2ch *l2ch = L2CH_CHAN(chan);

	print(ctx_shell, "Incoming data channel %p len %u", chan, buf->len);

	if (buf->len) {
		hexdump(ctx_shell, buf->data, buf->len);
	}

	if (l2cap_recv_delay) {
		/* Submit work only if queue is empty */
		if (k_fifo_is_empty(&l2cap_recv_fifo)) {
			print(ctx_shell, "Delaying response in %u ms...",
			      l2cap_recv_delay);
			k_delayed_work_submit(&l2ch->recv_work,
					      l2cap_recv_delay);
		}
		net_buf_put(&l2cap_recv_fifo, buf);
		return -EINPROGRESS;
	}

	return 0;
}

static void l2cap_connected(struct bt_l2cap_chan *chan)
{
	struct l2ch *c = L2CH_CHAN(chan);

	k_delayed_work_init(&c->recv_work, l2cap_recv_cb);

	print(ctx_shell, "Channel %p connected", chan);
}

static void l2cap_disconnected(struct bt_l2cap_chan *chan)
{
	print(ctx_shell, "Channel %p disconnected", chan);
}

static struct net_buf *l2cap_alloc_buf(struct bt_l2cap_chan *chan)
{
	/* print if metrics is disabled */
	if (chan->ops->recv != l2cap_recv_metrics) {
		print(ctx_shell, "Channel %p requires buffer", chan);
	}

	return net_buf_alloc(&data_rx_pool, K_FOREVER);
}

static struct bt_l2cap_chan_ops l2cap_ops = {
	.alloc_buf	= l2cap_alloc_buf,
	.recv		= l2cap_recv,
	.connected	= l2cap_connected,
	.disconnected	= l2cap_disconnected,
};


static struct l2ch l2ch_chan = {
	.ch.chan.ops	= &l2cap_ops,
	.ch.rx.mtu	= DATA_MTU,
};

static int l2cap_accept(struct bt_conn *conn, struct bt_l2cap_chan **chan)
{
	print(ctx_shell, "Incoming conn %p", conn);

	if (l2ch_chan.ch.chan.conn) {
		print(ctx_shell, "No channels available");
		return -ENOMEM;
	}

	*chan = &l2ch_chan.ch.chan;

	return 0;
}

static struct bt_l2cap_server server = {
	.accept		= l2cap_accept,
};

static void cmd_register(const struct shell *shell, size_t argc, char *argv[])
{

	if (!shell_cmd_precheck(shell, argc >= 2, NULL, 0)) {
		return;
	}

	if (server.psm) {
		error(shell, "Already registered");
		return;
	}

	server.psm = strtoul(argv[1], NULL, 16);

	if (argc > 2) {
		server.sec_level = strtoul(argv[2], NULL, 10);
	}

	if (bt_l2cap_server_register(&server) < 0) {
		error(shell, "Unable to register psm");
		server.psm = 0;
	} else {
		print(shell, "L2CAP psm %u sec_level %u registered",
		      server.psm, server.sec_level);
	}
}

static void cmd_connect(const struct shell *shell, size_t argc, char *argv[])
{
	u16_t psm;
	int err;

	if (!default_conn) {
		error(shell, "Not connected");
		return;
	}

	if (!shell_cmd_precheck(shell, argc == 2, NULL, 0)) {
		return;
	}

	if (l2ch_chan.ch.chan.conn) {
		error(shell, "Channel already in use");
		return;
	}

	psm = strtoul(argv[1], NULL, 16);

	err = bt_l2cap_chan_connect(default_conn, &l2ch_chan.ch.chan, psm);
	if (err < 0) {
		error(shell, "Unable to connect to psm %u (err %u)", psm,
		      err);
	} else {
		print(shell, "L2CAP connection pending");
	}
}

static void cmd_disconnect(const struct shell *shell, size_t argc, char *argv[])
{
	int err;

	err = bt_l2cap_chan_disconnect(&l2ch_chan.ch.chan);
	if (err) {
		print(shell, "Unable to disconnect: %u", -err);
	}
}

static void cmd_send(const struct shell *shell, size_t argc, char *argv[])
{
	static u8_t buf_data[DATA_MTU] = { [0 ... (DATA_MTU - 1)] = 0xff };
	int ret, len, count = 1;
	struct net_buf *buf;

	if (argc > 1) {
		count = strtoul(argv[1], NULL, 10);
	}

	len = min(l2ch_chan.ch.tx.mtu, DATA_MTU - BT_L2CAP_CHAN_SEND_RESERVE);

	while (count--) {
		buf = net_buf_alloc(&data_tx_pool, K_FOREVER);
		net_buf_reserve(buf, BT_L2CAP_CHAN_SEND_RESERVE);

		net_buf_add_mem(buf, buf_data, len);
		ret = bt_l2cap_chan_send(&l2ch_chan.ch.chan, buf);
		if (ret < 0) {
			print(shell, "Unable to send: %d", -ret);
			net_buf_unref(buf);
			break;
		}
	}
}

static void cmd_recv(const struct shell *shell, size_t argc, char *argv[])
{
	if (argc > 1) {
		l2cap_recv_delay = strtoul(argv[1], NULL, 10);
	} else {
		print(shell, "l2cap receive delay: %u ms",
			     l2cap_recv_delay);
	}
}

static void cmd_metrics(const struct shell *shell, size_t argc, char *argv[])
{
	const char *action;

	if (argc < 2) {
		print(shell, "l2cap rate: %u bps.", l2cap_rate);

		return;
	}

	action = argv[1];

	if (!strcmp(action, "on")) {
		l2cap_ops.recv = l2cap_recv_metrics;
	} else if (!strcmp(action, "off")) {
		l2cap_ops.recv = l2cap_recv;
	} else {
		shell_help_print(shell, NULL, 0);
		return;
	}

	print(shell, "l2cap metrics %s.", action);
}

#define HELP_NONE "[none]"

SHELL_CREATE_STATIC_SUBCMD_SET(l2cap_cmds) {
	SHELL_CMD(connect, NULL, "<psm>", cmd_connect),
	SHELL_CMD(disconnect, NULL, HELP_NONE, cmd_disconnect),
	SHELL_CMD(metrics, NULL, "<value on, off>", cmd_metrics),
	SHELL_CMD(recv, NULL, "[delay (in miliseconds)", cmd_recv),
	SHELL_CMD(register, NULL, "<psm> [sec_level]", cmd_register),
	SHELL_CMD(send, NULL, "<number of packets>", cmd_send),
	SHELL_SUBCMD_SET_END
};

static void cmd_l2cap(const struct shell *shell, size_t argc, char **argv)
{
	if (argc == 1) {
		shell_help_print(shell, NULL, 0);
		return;
	}

	if (!shell_cmd_precheck(shell, (argc == 2), NULL, 0)) {
		return;
	}

	error(shell, "%s:%s%s", argv[0], "unknown parameter: ", argv[1]);
}

SHELL_CMD_REGISTER(l2cap, &l2cap_cmds, "Bluetooth L2CAP shell commands",
		   cmd_l2cap);

