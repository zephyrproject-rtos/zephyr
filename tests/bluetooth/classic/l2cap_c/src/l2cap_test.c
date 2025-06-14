/* l2cap_test.c - Bluetooth classic l2cap smoke test */

/*
 * Copyright 2025 NXP
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

static uint16_t data_bredr_mtu = 48;
#define DATA_POOL_SIZE 200

NET_BUF_POOL_FIXED_DEFINE(data_tx_pool, 1, BT_L2CAP_SDU_BUF_SIZE(DATA_POOL_SIZE),
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);
NET_BUF_POOL_FIXED_DEFINE(data_rx_pool, 1, DATA_POOL_SIZE, 8, NULL);

struct l2cap_br_chan {
	bool active;
	struct bt_l2cap_br_chan chan;
#if defined(CONFIG_BT_L2CAP_RET_FC)
	struct k_fifo l2cap_recv_fifo;
	bool hold_credit;
#endif /* CONFIG_BT_L2CAP_RET_FC */
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

#define APPL_L2CAP_CONNECTION_MAX_COUNT 2
static struct l2cap_br_chan br_l2cap[APPL_L2CAP_CONNECTION_MAX_COUNT];
static struct bt_l2cap_br_server br_l2cap_server[APPL_L2CAP_CONNECTION_MAX_COUNT];

static int l2cap_recv(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	struct l2cap_br_chan *br_chan = CONTAINER_OF(
		CONTAINER_OF(chan, struct bt_l2cap_br_chan, chan), struct l2cap_br_chan, chan);

	bt_shell_print("Incoming data channel %d len %u", ARRAY_INDEX(br_l2cap, br_chan), buf->len);

	if (buf->len) {
		bt_shell_print("Incoming data :%.*s\r\n", buf->len, buf->data);
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
	struct l2cap_br_chan *br_chan = CONTAINER_OF(
		CONTAINER_OF(chan, struct bt_l2cap_br_chan, chan), struct l2cap_br_chan, chan);

	bt_shell_print("Channel %d connected", ARRAY_INDEX(br_l2cap, br_chan));

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
	struct l2cap_br_chan *br_chan = CONTAINER_OF(
		CONTAINER_OF(chan, struct bt_l2cap_br_chan, chan), struct l2cap_br_chan, chan);

	br_chan->active = false;
	bt_shell_print("Channel %d disconnected", ARRAY_INDEX(br_l2cap, br_chan));

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

static struct l2cap_br_chan *appl_br_l2cap(void)
{
	ARRAY_FOR_EACH(br_l2cap, index) {
		if (br_l2cap[index].active == false) {
			br_l2cap[index].active = true;
			br_l2cap[index].chan.chan.ops = &l2cap_ops;
			br_l2cap[index].chan.rx.mtu = data_bredr_mtu;
#if defined(CONFIG_BT_L2CAP_RET_FC)
			k_fifo_init(&br_l2cap[index].l2cap_recv_fifo);
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
	struct l2cap_br_chan *l2cap_chan;

	br_server = CONTAINER_OF(server, struct bt_l2cap_br_server, server);

	l2cap_chan = appl_br_l2cap();
	if (l2cap_chan == NULL) {
		bt_shell_error("No channels application br chan");
		return -ENOMEM;
	}

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
	} else {
		l2cap_chan->chan.rx.mode = BT_L2CAP_BR_LINK_MODE_BASIC;
		l2cap_chan->chan.rx.max_transmit = 0;
	}
#endif /* CONFIG_BT_L2CAP_RET_FC */
	(void)br_server;
	return 0;
}

static struct bt_l2cap_br_server *appl_br_l2cap_server_alloc(uint16_t psm)
{
	ARRAY_FOR_EACH(br_l2cap_server, index) {
		if (br_l2cap_server[index].server.psm == 0) {
			br_l2cap_server[index].server.psm = psm;
			br_l2cap_server[index].server.accept = l2cap_accept;
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
	uint8_t index;
	struct l2cap_br_chan *l2cap_chan;

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	l2cap_chan = appl_br_l2cap();
	if (l2cap_chan == NULL) {
		bt_shell_error("No channels application br chan");
		l2cap_chan->active = false;
		return -ENOMEM;
	}

	err = bt_conn_get_info(default_conn, &info);
	if ((err < 0) || (info.type != BT_CONN_TYPE_BR)) {
		shell_error(sh, "Invalid conn type");
		l2cap_chan->active = false;
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
		l2cap_chan->active = false;
		shell_help(sh);
		return SHELL_CMD_HELP_PRINTED;
	}

	l2cap_chan->hold_credit = false;
	l2cap_chan->chan.rx.optional = false;
	l2cap_chan->chan.rx.extended_control = false;

	index = 3;
	while (index < argc) {
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
			l2cap_chan->active = false;
			shell_help(sh);
			return SHELL_CMD_HELP_PRINTED;
		}
		index++;
	}

	if ((l2cap_chan->chan.rx.extended_control) &&
	    ((l2cap_chan->chan.rx.mode != BT_L2CAP_BR_LINK_MODE_ERET) &&
	     (l2cap_chan->chan.rx.mode != BT_L2CAP_BR_LINK_MODE_STREAM))) {
		l2cap_chan->active = false;
		shell_error(sh, "[extended_control] only supports mode eret and stream");
		return -ENOEXEC;
	}

	if (l2cap_chan->hold_credit && (l2cap_chan->chan.rx.mode == BT_L2CAP_BR_LINK_MODE_BASIC)) {
		l2cap_chan->active = false;
		shell_error(sh, "[hold_credit] cannot support basic mode");
		return -ENOEXEC;
	}

	l2cap_chan->chan.rx.max_window = CONFIG_BT_L2CAP_MAX_WINDOW_SIZE;
#endif /* CONFIG_BT_L2CAP_RET_FC */

	err = bt_l2cap_chan_connect(default_conn, &l2cap_chan->chan.chan, psm);
	if (err < 0) {
		shell_error(sh, "Unable to connect to psm %u (err %d)", psm, err);
		l2cap_chan->active = false;
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

	if (id >= APPL_L2CAP_CONNECTION_MAX_COUNT) {
		shell_error(sh, "id is invalid");
		return -EINVAL;
	}

	if (br_l2cap[id].active == true) {
		err = bt_l2cap_chan_disconnect(&br_l2cap[id].chan.chan);
		if (err) {
			shell_error(sh, "Unable to disconnect: %u", -err);
			return err;
		}
	}

	return 0;
}

static int cmd_l2cap_send(const struct shell *sh, size_t argc, char *argv[])
{
	int err, data_len = 0;
	uint8_t id;
	struct net_buf *buf;
	uint16_t send_len;

	id = strtoul(argv[1], NULL, 16);
	data_len = strtoul(argv[3], NULL, 16);
	send_len = MIN(data_len, data_bredr_mtu);

	if (id >= APPL_L2CAP_CONNECTION_MAX_COUNT) {
		shell_error(sh, "id is invalid");
		return -EINVAL;
	}

	shell_print(sh, "send data len = %d", data_len);

	if (br_l2cap[id].active == true) {
		buf = net_buf_alloc(&data_tx_pool, K_SECONDS(2));
		if (buf == NULL) {
			if (br_l2cap[id].chan.state != BT_L2CAP_CONNECTED) {
				shell_error(sh, "Channel disconnected, stopping TX");
				return -EAGAIN;
			}
			shell_error(sh, "Allocation timeout, stopping TX");
			return -EAGAIN;
		}
		net_buf_reserve(buf, BT_L2CAP_CHAN_SEND_RESERVE);
		net_buf_add_mem(buf, argv[2], send_len);
		err = bt_l2cap_chan_send(&br_l2cap[id].chan.chan, buf);
		if (err < 0) {
			shell_error(sh, "Unable to send: %d", -err);
			net_buf_unref(buf);
			return -ENOEXEC;
		}
	} else {
		shell_print(sh, "channel %d not support", id);
		return -EINVAL;
	}
	return 0;
}

static bool l2cap_psm_registered(uint16_t psm)
{
	for (uint8_t index = 0; index < APPL_L2CAP_CONNECTION_MAX_COUNT; index++) {
		if (br_l2cap_server[index].server.psm == psm) {
			return true;
		}
	}
	return false;
}

static int cmd_l2cap_register(const struct shell *sh, size_t argc, char *argv[])
{
	uint16_t psm = strtoul(argv[1], NULL, 16);
	uint8_t index = 0;
	struct bt_l2cap_br_server *l2cap_server;

	if (l2cap_psm_registered(psm)) {
		shell_print(sh, "Already registered");
		return -ENOEXEC;
	}

	l2cap_server = appl_br_l2cap_server_alloc(psm);
	if (l2cap_server == NULL) {
		bt_shell_error("No channels application br chan");
		return -ENOMEM;
	}
	l2cap_server->server.psm = psm;

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

	index = 3;
	while (index < argc) {
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
		index++;
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

static int cmd_change_mtu(const struct shell *sh, size_t argc, char *argv[])
{
	uint16_t mtu = strtoul(argv[1], NULL, 16);

	if (mtu < 48 || mtu > 65535) {
		shell_error(sh, "mtu must be in range 48-65535");
		return -EINVAL;
	}
	data_bredr_mtu = mtu;
	return 0;
}

static int cmd_read_mtu(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t id = strtoul(argv[1], NULL, 16);
	char *role = argv[2];

	if (id >= APPL_L2CAP_CONNECTION_MAX_COUNT) {
		shell_error(sh, "id is invalid");
		return -EINVAL;
	}

	if (br_l2cap[id].active == true) {
		if (!strcmp(role, "local")) {
			shell_print(sh, "local mtu = %d", br_l2cap[id].chan.rx.mtu);
		} else if (!strcmp(role, "peer")) {
			shell_print(sh, "peer mtu = %d", br_l2cap[id].chan.tx.mtu);
		} else {
			shell_error(sh, "role must be local or peer");
			return -EINVAL;
		}
	}
	return 0;
}

#define HELP_REGISTER                                                                              \
	"<psm> <mode : basic, ret, fc, eret, stream> [hold_credit] "                               \
	"[mode_optional] [extended_control] [sec : value]"
#define HELP_CONNECT                                                                               \
	"<psm> <mode : basic, ret, fc, eret, stream> [hold_credit] "                               \
	"[mode_optional] [extended_control] [sec : value] [mtu : value]"

SHELL_STATIC_SUBCMD_SET_CREATE(
	l2cap_br_cmds, SHELL_CMD_ARG(register, NULL, HELP_REGISTER, cmd_l2cap_register, 2, 5),
	SHELL_CMD_ARG(connect, NULL, HELP_CONNECT, cmd_connect, 2, 7),
	SHELL_CMD_ARG(disconnect, NULL, "<id>", cmd_l2cap_disconnect, 2, 0),
	SHELL_CMD_ARG(send, NULL, "<id> <length of data> <data> ", cmd_l2cap_send, 4, 0),
	SHELL_CMD_ARG(change_mtu, NULL, "<mtu>", cmd_change_mtu, 2, 0),
	SHELL_CMD_ARG(read_mtu, NULL, "<id> <local/peer>", cmd_read_mtu, 3, 0),
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
