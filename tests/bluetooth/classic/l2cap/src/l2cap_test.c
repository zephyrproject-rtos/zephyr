/* sdp_client.c - Bluetooth classic SDP client smoke test */

/*
 * Copyright 2024 NXP
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
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/bluetooth/classic/rfcomm.h>
#include <zephyr/bluetooth/classic/sdp.h>

#include <zephyr/shell/shell.h>

#include "host/shell/bt.h"
#include "common/bt_shell_private.h"

#define DATA_BREDR_MTU 48

NET_BUF_POOL_FIXED_DEFINE(data_tx_pool, 1, BT_L2CAP_SDU_BUF_SIZE(DATA_BREDR_MTU),
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);
NET_BUF_POOL_FIXED_DEFINE(data_rx_pool, 1, DATA_BREDR_MTU, 8, NULL);

struct l2cap_br_chan {
	struct bt_l2cap_br_chan chan;
#if defined(CONFIG_BT_L2CAP_RET_FC)
	struct k_fifo l2cap_recv_fifo;
	bool hold_credit;
#endif /* CONFIG_BT_L2CAP_RET_FC */
};

struct app_l2cap_br_chan {
	bool active;
	uint8_t id;
	struct bt_conn *conn;
	struct l2cap_br_chan l2cap_chan;
};

struct bt_l2cap_br_server {
	struct bt_l2cap_server server;
#if defined(CONFIG_BT_L2CAP_RET_FC)
	uint8_t options;
#endif /* CONFIG_BT_L2CAP_RET_FC */
};

#define BT_L2CAP_BR_SERVER_OPT_RET           BIT(0)
#define BT_L2CAP_BR_SERVER_OPT_FC            BIT(1)
#define BT_L2CAP_BR_SERVER_OPT_ERET          BIT(2)
#define BT_L2CAP_BR_SERVER_OPT_STREAM        BIT(3)
#define BT_L2CAP_BR_SERVER_OPT_MODE_OPTIONAL BIT(4)
#define BT_L2CAP_BR_SERVER_OPT_EXT_WIN_SIZE  BIT(5)
#define BT_L2CAP_BR_SERVER_OPT_HOLD_CREDIT   BIT(6)

struct app_l2cap_br_server {
	bool active;
	uint8_t id;
	struct bt_conn *conn;
	struct bt_l2cap_br_server l2cap_server;
};

#define APPL_L2CAP_CONNECTION_MAX_COUNT 2
struct app_l2cap_br_chan br_l2cap[APPL_L2CAP_CONNECTION_MAX_COUNT] = {0};
struct app_l2cap_br_server br_l2cap_server[APPL_L2CAP_CONNECTION_MAX_COUNT] = {0};

static int l2cap_recv(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	struct l2cap_br_chan *br_chan = CONTAINER_OF(chan, struct l2cap_br_chan, chan.chan);
	struct app_l2cap_br_chan *appl_br_chan =
		CONTAINER_OF(br_chan, struct app_l2cap_br_chan, l2cap_chan);

	bt_shell_print("Incoming data channel %d len %u", appl_br_chan->id, buf->len);

	if (buf->len) {
		printk("Incoming data :%.*s\r\n", buf->len, buf->data);
	}

#if defined(CONFIG_BT_L2CAP_RET_FC)
	if (br_chan->hold_credit) {
		k_fifo_put(&br_chan->l2cap_recv_fifo, buf);
		return -EINPROGRESS;
	}
#endif /* CONFIG_BT_L2CAP_RET_FC */
	(void)br_chan;

	return 0;
}

static struct net_buf *l2cap_alloc_buf(struct bt_l2cap_chan *chan)
{
	bt_shell_print("Channel %p requires buffer", chan);

	return net_buf_alloc(&data_rx_pool, K_NO_WAIT);
}

static void l2cap_connected(struct bt_l2cap_chan *chan)
{
	struct l2cap_br_chan *br_chan = CONTAINER_OF(chan, struct l2cap_br_chan, chan.chan);
	struct app_l2cap_br_chan *appl_br_chan =
		CONTAINER_OF(br_chan, struct app_l2cap_br_chan, l2cap_chan);
	bt_shell_print("Channel %d connected", appl_br_chan->id);

#if defined(CONFIG_BT_L2CAP_RET_FC)
	switch (br_chan->chan.rx.mode) {
	case BT_L2CAP_BR_LINK_MODE_BASIC:
		bt_shell_print("It is basic mode");
		if (br_chan->hold_credit) {
			br_chan->hold_credit = false;
			bt_shell_warn("hold_credit is unsupported in basic mode");
		}
		break;
	case BT_L2CAP_BR_LINK_MODE_RET:
		bt_shell_print("It is retransmission mode");
		break;
	case BT_L2CAP_BR_LINK_MODE_FC:
		bt_shell_print("It is flow control mode");
		break;
	case BT_L2CAP_BR_LINK_MODE_ERET:
		bt_shell_print("It is enhance retransmission mode");
		break;
	case BT_L2CAP_BR_LINK_MODE_STREAM:
		bt_shell_print("It is streaming mode");
		break;
	default:
		bt_shell_error("It is unknown mode");
		break;
	}
#endif /* CONFIG_BT_L2CAP_RET_FC */
	(void)br_chan;
}

static void l2cap_disconnected(struct bt_l2cap_chan *chan)
{
	struct l2cap_br_chan *br_chan = CONTAINER_OF(chan, struct l2cap_br_chan, chan.chan);
	struct app_l2cap_br_chan *appl_br_chan =
		CONTAINER_OF(br_chan, struct app_l2cap_br_chan, l2cap_chan);
	appl_br_chan->conn = NULL;
	appl_br_chan->active = false;

	bt_shell_print("Channel %d disconnected", appl_br_chan->id);

#if defined(CONFIG_BT_L2CAP_RET_FC)
	struct net_buf *buf;
	do {
		buf = k_fifo_get(&br_chan->l2cap_recv_fifo, K_NO_WAIT);
		if (buf != NULL) {
			net_buf_unref(buf);
		}
	} while (buf != NULL);
#endif /* CONFIG_BT_L2CAP_RET_FC */
}

static const struct bt_l2cap_chan_ops l2cap_ops = {
	.alloc_buf = l2cap_alloc_buf,
	.recv = l2cap_recv,
	.connected = l2cap_connected,
	.disconnected = l2cap_disconnected,
};

struct app_l2cap_br_chan *appl_br_l2cap(struct bt_conn *conn)
{
	for (uint8_t index = 0; index < APPL_L2CAP_CONNECTION_MAX_COUNT; index++) {
		if (br_l2cap[index].conn == NULL && br_l2cap[index].active == false) {
			br_l2cap[index].conn = conn;
			br_l2cap[index].active = true;
			br_l2cap[index].id = index;
			br_l2cap[index].l2cap_chan.chan.chan.ops = &l2cap_ops;
			br_l2cap[index].l2cap_chan.chan.rx.mtu = DATA_BREDR_MTU;
#if defined(CONFIG_BT_L2CAP_RET_FC)
			k_fifo_init(&br_l2cap[index].l2cap_chan.l2cap_recv_fifo);
#endif
			return &br_l2cap[index];
		}
	}
	return NULL;
}

static int l2cap_accept(struct bt_conn *conn, struct bt_l2cap_server *server,
			struct bt_l2cap_chan **chan)
{
	struct bt_l2cap_br_server *br_server;
	struct app_l2cap_br_server *appl_l2cap_server;
	struct app_l2cap_br_chan *appl_l2cap = NULL;
	struct l2cap_br_chan *l2cap_chan = NULL;

	br_server = CONTAINER_OF(server, struct bt_l2cap_br_server, server);
	appl_l2cap_server = CONTAINER_OF(br_server, struct app_l2cap_br_server, l2cap_server);

	appl_l2cap = appl_br_l2cap(conn);
	if (!appl_l2cap) {
		bt_shell_error("No channels application br chan");
		return -ENOMEM;
	}
	l2cap_chan = &appl_l2cap->l2cap_chan;
	*chan = &l2cap_chan->chan.chan;

	bt_shell_print("Incoming BR/EDR conn %p", conn);

#if defined(CONFIG_BT_L2CAP_RET_FC)
	if (br_server->options & BT_L2CAP_BR_SERVER_OPT_HOLD_CREDIT) {
		l2cap_chan->hold_credit = true;
	} else {
		l2cap_chan->hold_credit = false;
	}

	if (br_server->options & BT_L2CAP_BR_SERVER_OPT_EXT_WIN_SIZE) {
		l2cap_chan->chan.rx.extended_control = true;
	} else {
		l2cap_chan->chan.rx.extended_control = false;
	}

	if (br_server->options & BT_L2CAP_BR_SERVER_OPT_MODE_OPTIONAL) {
		l2cap_chan->chan.rx.optional = true;
	} else {
		l2cap_chan->chan.rx.optional = false;
	}

	l2cap_chan->chan.rx.fcs = BT_L2CAP_BR_FCS_16BIT;

	if (br_server->options & BT_L2CAP_BR_SERVER_OPT_STREAM) {
		l2cap_chan->chan.rx.mode = BT_L2CAP_BR_LINK_MODE_STREAM;
		l2cap_chan->chan.rx.max_window = CONFIG_BT_L2CAP_MAX_WINDOW_SIZE;
		l2cap_chan->chan.rx.max_transmit = 0;
	} else if (br_server->options & BT_L2CAP_BR_SERVER_OPT_ERET) {
		l2cap_chan->chan.rx.mode = BT_L2CAP_BR_LINK_MODE_ERET;
		l2cap_chan->chan.rx.max_window = CONFIG_BT_L2CAP_MAX_WINDOW_SIZE;
		l2cap_chan->chan.rx.max_transmit = 3;
	} else if (br_server->options & BT_L2CAP_BR_SERVER_OPT_FC) {
		l2cap_chan->chan.rx.mode = BT_L2CAP_BR_LINK_MODE_FC;
		l2cap_chan->chan.rx.max_window = CONFIG_BT_L2CAP_MAX_WINDOW_SIZE;
		l2cap_chan->chan.rx.max_transmit = 3;
	} else if (br_server->options & BT_L2CAP_BR_SERVER_OPT_RET) {
		l2cap_chan->chan.rx.mode = BT_L2CAP_BR_LINK_MODE_RET;
		l2cap_chan->chan.rx.max_window = CONFIG_BT_L2CAP_MAX_WINDOW_SIZE;
		l2cap_chan->chan.rx.max_transmit = 3;
	}
#endif /* CONFIG_BT_L2CAP_RET_FC */
	(void)br_server;
	return 0;
}

struct app_l2cap_br_server *appl_br_l2cap_server_alloc(uint16_t psm)
{
	for (uint8_t index = 0; index < APPL_L2CAP_CONNECTION_MAX_COUNT; index++) {
		if (br_l2cap_server[index].conn == NULL && br_l2cap_server[index].active == false) {
			br_l2cap_server[index].active = true;
			br_l2cap_server[index].id = index;
			br_l2cap_server[index].l2cap_server.server.psm = psm;
			br_l2cap_server[index].l2cap_server.server.accept = l2cap_accept;
			return &br_l2cap_server[index];
		}
	}
	return NULL;
}

static int cmd_connect(const struct shell *sh, size_t argc, char *argv[])
{
	uint16_t psm;
	struct bt_conn_info info;
	int err;
	struct app_l2cap_br_chan *appl_l2cap = NULL;
	struct l2cap_br_chan *l2cap_chan = NULL;

	if (!default_conn) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	appl_l2cap = appl_br_l2cap(default_conn);
	if (!appl_l2cap) {
		bt_shell_error("No channels application br chan");
		return -ENOMEM;
	}
	l2cap_chan = &appl_l2cap->l2cap_chan;
	if (l2cap_chan->chan.chan.conn) {
		bt_shell_error("No channels available");
		return -ENOMEM;
	}

	err = bt_conn_get_info(default_conn, &info);
	if ((err < 0) || (info.type != BT_CONN_TYPE_BR)) {
		shell_error(sh, "Invalid conn type");
		return -ENOEXEC;
	}

	psm = strtoul(argv[1], NULL, 16);

#if defined(CONFIG_BT_L2CAP_RET_FC)
	if (!strcmp(argv[2], "basic")) {
		l2cap_chan->chan.rx.mode = BT_L2CAP_BR_LINK_MODE_BASIC;
	} else if (!strcmp(argv[2], "ret")) {
		l2cap_chan->chan.rx.mode = BT_L2CAP_BR_LINK_MODE_RET;
		l2cap_chan->chan.rx.max_transmit = 3;
	} else if (!strcmp(argv[2], "fc")) {
		l2cap_chan->chan.rx.mode = BT_L2CAP_BR_LINK_MODE_FC;
		l2cap_chan->chan.rx.max_transmit = 3;
	} else if (!strcmp(argv[2], "eret")) {
		l2cap_chan->chan.rx.mode = BT_L2CAP_BR_LINK_MODE_ERET;
		l2cap_chan->chan.rx.max_transmit = 3;
	} else if (!strcmp(argv[2], "stream")) {
		l2cap_chan->chan.rx.mode = BT_L2CAP_BR_LINK_MODE_STREAM;
		l2cap_chan->chan.rx.max_transmit = 0;
	} else {
		shell_help(sh);
		return SHELL_CMD_HELP_PRINTED;
	}

	l2cap_chan->hold_credit = false;
	l2cap_chan->chan.rx.optional = false;
	l2cap_chan->chan.rx.extended_control = false;

	for (size_t index = 3; index < argc; index++) {
		if (!strcmp(argv[index], "hold_credit")) {
			l2cap_chan->hold_credit = true;
		} else if (!strcmp(argv[index], "mode_optional")) {
			l2cap_chan->chan.rx.optional = true;
		} else if (!strcmp(argv[index], "extended_control")) {
			l2cap_chan->chan.rx.extended_control = true;
		} else if (!strcmp(argv[index], "sec")) {
			l2cap_chan->chan.required_sec_level = strtoul(argv[++index], NULL, 16);
		} else if (!strcmp(argv[index], "mtu")) {
			l2cap_chan->chan.rx.mtu = strtoul(argv[++index], NULL, 16);
		} else {
			shell_help(sh);
			return SHELL_CMD_HELP_PRINTED;
		}
	}

	if ((l2cap_chan->chan.rx.extended_control) &&
	    ((l2cap_chan->chan.rx.mode != BT_L2CAP_BR_LINK_MODE_ERET) &&
	     (l2cap_chan->chan.rx.mode != BT_L2CAP_BR_LINK_MODE_STREAM))) {
		shell_error(sh, "[extended_control] only supports mode eret and stream");
		return -ENOEXEC;
	}

	if (l2cap_chan->hold_credit && (l2cap_chan->chan.rx.mode == BT_L2CAP_BR_LINK_MODE_BASIC)) {
		shell_error(sh, "[hold_credit] cannot support basic mode");
		return -ENOEXEC;
	}

	l2cap_chan->chan.rx.max_window = CONFIG_BT_L2CAP_MAX_WINDOW_SIZE;
#endif /* CONFIG_BT_L2CAP_RET_FC */

	err = bt_l2cap_chan_connect(default_conn, &l2cap_chan->chan.chan, psm);
	if (err < 0) {
		shell_error(sh, "Unable to connect to psm %u (err %d)", psm, err);
		appl_l2cap->conn = NULL;
		appl_l2cap->active = false;
	} else {
		shell_print(sh, "L2CAP connection pending");
	}

	return err;
}

static int cmd_l2cap_disconnect(const struct shell *sh, size_t argc, char *argv[])
{
	int err;
	uint8_t id;

	id = strtoul(argv[1], NULL, 16);

	if (br_l2cap[id].active == true) {
		err = bt_l2cap_chan_disconnect(&br_l2cap[id].l2cap_chan.chan.chan);
		if (err) {
			shell_error(sh, "Unable to disconnect: %u", -err);
			return err;
		}
	}

	return 0;
}

static int cmd_l2cap_send(const struct shell *sh, size_t argc, char *argv[])
{
	int err, mtu_len = DATA_BREDR_MTU, data_len = 0;
	uint8_t id;
	struct net_buf *buf;
	uint16_t len;
	uint16_t send_length = 0, remaining_data = 0;
	struct l2cap_br_chan *l2cap_chan = NULL;

	id = strtoul(argv[1], NULL, 16);
	data_len = strtoul(argv[3], NULL, 16);
	mtu_len = MIN(l2cap_chan->chan.tx.mtu, DATA_BREDR_MTU);

	shell_print(sh, "data_len = %d", data_len);

	if (br_l2cap[id].active == true) {
		l2cap_chan = &br_l2cap[id].l2cap_chan;
		remaining_data = data_len;
		while (remaining_data > 0) {
			buf = net_buf_alloc(&data_tx_pool, K_SECONDS(2));
			if (!buf) {
				if (l2cap_chan->chan.state != BT_L2CAP_CONNECTED) {
					shell_error(sh, "Channel disconnected, stopping TX");
					return -EAGAIN;
				}
				shell_error(sh, "Allocation timeout, stopping TX");
				return -EAGAIN;
			}
			net_buf_reserve(buf, BT_L2CAP_CHAN_SEND_RESERVE);
			if (remaining_data > mtu_len) {
				net_buf_add_mem(buf, argv[2] + send_length, mtu_len);
				len = mtu_len;
			} else {
				net_buf_add_mem(buf, argv[2] + send_length, remaining_data);
				len = remaining_data;
			}
			err = bt_l2cap_chan_send(&l2cap_chan->chan.chan, buf);
			if (err < 0) {
				shell_error(sh, "Unable to send: %d", -err);
				net_buf_unref(buf);
				return -ENOEXEC;
			}
			send_length += len;
			remaining_data -= len;
		}
	} else {
		shell_print(sh, "channel %d not support", id);
		return -EINVAL;
	}
	return 0;
}

bool l2cap_psm_registed(uint16_t psm)
{
	for (uint8_t index = 0; index < APPL_L2CAP_CONNECTION_MAX_COUNT; index++) {
		if (br_l2cap_server[index].active == true &&
		    br_l2cap_server[index].l2cap_server.server.psm == psm) {
			return true;
		}
	}
	return false;
}
static int cmd_l2cap_register(const struct shell *sh, size_t argc, char *argv[])
{
	uint16_t psm = strtoul(argv[1], NULL, 16);
	struct app_l2cap_br_server *app_l2cap_server;
	struct bt_l2cap_br_server *l2cap_server;
	if (l2cap_psm_registed(psm)) {
		shell_print(sh, "Already registered");
		return -ENOEXEC;
	}

	app_l2cap_server = appl_br_l2cap_server_alloc(psm);
	if (!app_l2cap_server) {
		bt_shell_error("No channels application br chan");
		return -ENOMEM;
	}
	l2cap_server = &app_l2cap_server->l2cap_server;
	l2cap_server->server.psm = strtoul(argv[1], NULL, 16);

#if defined(CONFIG_BT_L2CAP_RET_FC)
	l2cap_server->options = 0;

	if (!strcmp(argv[2], "basic")) {
		/* Support mode: None */
	} else if (!strcmp(argv[2], "ret")) {
		l2cap_server->options |= BT_L2CAP_BR_SERVER_OPT_RET;
	} else if (!strcmp(argv[2], "fc")) {
		l2cap_server->options |= BT_L2CAP_BR_SERVER_OPT_FC;
	} else if (!strcmp(argv[2], "eret")) {
		l2cap_server->options |= BT_L2CAP_BR_SERVER_OPT_ERET;
	} else if (!strcmp(argv[2], "stream")) {
		l2cap_server->options |= BT_L2CAP_BR_SERVER_OPT_STREAM;
	} else {
		l2cap_server->server.psm = 0;
		shell_help(sh);
		return SHELL_CMD_HELP_PRINTED;
	}

	for (size_t index = 3; index < argc; index++) {
		if (!strcmp(argv[index], "hold_credit")) {
			l2cap_server->options |= BT_L2CAP_BR_SERVER_OPT_HOLD_CREDIT;
		} else if (!strcmp(argv[index], "mode_optional")) {
			l2cap_server->options |= BT_L2CAP_BR_SERVER_OPT_MODE_OPTIONAL;
		} else if (!strcmp(argv[index], "extended_control")) {
			l2cap_server->options |= BT_L2CAP_BR_SERVER_OPT_EXT_WIN_SIZE;
		} else if (!strcmp(argv[index], "sec")) {
			l2cap_server->server.sec_level = strtoul(argv[++index], NULL, 16);
		} else {
			l2cap_server->server.psm = 0;
			shell_help(sh);
			return SHELL_CMD_HELP_PRINTED;
		}
	}

	if ((l2cap_server->options & BT_L2CAP_BR_SERVER_OPT_EXT_WIN_SIZE) &&
	    (!(l2cap_server->options &
	       (BT_L2CAP_BR_SERVER_OPT_ERET | BT_L2CAP_BR_SERVER_OPT_STREAM)))) {
		shell_error(sh, "[extended_control] only supports mode eret and stream");
		l2cap_server->server.psm = 0U;
		return -ENOEXEC;
	}
#endif /* CONFIG_BT_L2CAP_RET_FC */

	if (bt_l2cap_br_server_register(&l2cap_server->server) < 0) {
		shell_error(sh, "Unable to register psm");
		l2cap_server->server.psm = 0U;
		return -ENOEXEC;
	}

	shell_print(sh, "L2CAP psm %u registered", l2cap_server->server.psm);

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	l2cap_br_cmds,
	SHELL_CMD_ARG(register, NULL, "<psm> <mode> [option]", cmd_l2cap_register, 2, 5),
	SHELL_CMD_ARG(connect, NULL, "<psm> <mode> [option]", cmd_connect, 2, 3),
	SHELL_CMD_ARG(disconnect, NULL, "[id]", cmd_l2cap_disconnect, 2, 0),
	SHELL_CMD_ARG(send, NULL, "[id] [length of data] [data] ", cmd_l2cap_send, 4, 0),
	SHELL_SUBCMD_SET_END);

static int cmd_default_handler(const struct shell *sh, size_t argc, char **argv)
{
	if (argc == 1) {
		shell_help(sh);
		return SHELL_CMD_HELP_PRINTED;
	}

	shell_error(sh, "%s unknown parameter: %s", argv[0], argv[1]);

	return -EINVAL;
}

SHELL_CMD_REGISTER(l2cap_br, &l2cap_br_cmds, "Bluetooth classic l2cap shell commands",
		   cmd_default_handler);
