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
};

#define APPL_L2CAP_CONNECTION_MAX_COUNT 2
static struct l2cap_br_chan br_l2cap[APPL_L2CAP_CONNECTION_MAX_COUNT];
static struct bt_l2cap_server br_l2cap_server[APPL_L2CAP_CONNECTION_MAX_COUNT];

static int l2cap_recv(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	struct l2cap_br_chan *br_chan = CONTAINER_OF(
		CONTAINER_OF(chan, struct bt_l2cap_br_chan, chan), struct l2cap_br_chan, chan);

	bt_shell_print("Incoming data channel %d len %u", ARRAY_INDEX(br_l2cap, br_chan), buf->len);

	if (buf->len) {
		bt_shell_print("Incoming data :%.*s\r\n", buf->len, buf->data);
	}

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
}

static void l2cap_disconnected(struct bt_l2cap_chan *chan)
{
	struct l2cap_br_chan *br_chan = CONTAINER_OF(
		CONTAINER_OF(chan, struct bt_l2cap_br_chan, chan), struct l2cap_br_chan, chan);

	br_chan->active = false;
	bt_shell_print("Channel %d disconnected", ARRAY_INDEX(br_l2cap, br_chan));
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
			return &br_l2cap[index];
		}
	}
	return NULL;
}

static int l2cap_accept(struct bt_conn *conn, struct bt_l2cap_server *server,
			struct bt_l2cap_chan **chan)
{
	struct l2cap_br_chan *l2cap_chan;

	l2cap_chan = appl_br_l2cap();
	if (l2cap_chan == NULL) {
		bt_shell_error("No channels application br chan");
		return -ENOMEM;
	}

	*chan = &l2cap_chan->chan.chan;

	bt_shell_print("Incoming BR/EDR conn %p", conn);

	return 0;
}

static struct bt_l2cap_server *appl_br_l2cap_server_alloc(uint16_t psm)
{
	ARRAY_FOR_EACH(br_l2cap_server, index) {
		if (br_l2cap_server[index].psm == 0) {
			br_l2cap_server[index].psm = psm;
			br_l2cap_server[index].accept = l2cap_accept;
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

	index = 2;
	while (index < argc) {
		if (!strcmp(argv[index], "sec")) {
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

	if (id >= ARRAY_SIZE(br_l2cap)) {
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

	if (id >= ARRAY_SIZE(br_l2cap)) {
		shell_error(sh, "id is invalid");
		return -EINVAL;
	}

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
		shell_print(sh, "channel %d is invalid", id);
		return -EINVAL;
	}
	return 0;
}

static bool l2cap_psm_registered(uint16_t psm)
{
	ARRAY_FOR_EACH(br_l2cap_server, index) {
		if (br_l2cap_server[index].psm == psm) {
			return true;
		}
	}
	return false;
}

static int cmd_l2cap_register(const struct shell *sh, size_t argc, char *argv[])
{
	uint16_t psm = strtoul(argv[1], NULL, 16);
	struct bt_l2cap_server *l2cap_server;

	if (l2cap_psm_registered(psm)) {
		shell_print(sh, "Already registered");
		return -ENOEXEC;
	}

	l2cap_server = appl_br_l2cap_server_alloc(psm);
	if (l2cap_server == NULL) {
		bt_shell_error("No server available");
		return -ENOMEM;
	}

	if (argc == 4) {
		if (!strcmp(argv[argc - 2], "sec")) {
			l2cap_server->sec_level = strtoul(argv[argc - 1], NULL, 16);
		} else {
			l2cap_server->psm = 0;
			shell_help(sh);
			return SHELL_CMD_HELP_PRINTED;
		}
	}

	if (bt_l2cap_br_server_register(l2cap_server) < 0) {
		shell_error(sh, "Unable to register psm");
		l2cap_server->psm = 0U;
		return -ENOEXEC;
	}

	shell_print(sh, "L2CAP psm %u registered", l2cap_server->psm);

	return 0;
}

static int cmd_change_mtu(const struct shell *sh, size_t argc, char *argv[])
{
	data_bredr_mtu = strtoul(argv[1], NULL, 16);
	return 0;
}

static int cmd_read_mtu(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t id = strtoul(argv[1], NULL, 16);
	char *role = argv[2];

	if (id >= ARRAY_SIZE(br_l2cap)) {
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

#define HELP_REGISTER "<psm> [sec : value]"
#define HELP_CONNECT  "<psm> [sec : value] [mtu : value]"

SHELL_STATIC_SUBCMD_SET_CREATE(l2cap_br_cmds,
	SHELL_CMD_ARG(register, NULL, HELP_REGISTER, cmd_l2cap_register, 1, 2),
	SHELL_CMD_ARG(connect, NULL, HELP_CONNECT, cmd_connect, 1, 4),
	SHELL_CMD_ARG(disconnect, NULL, "<id>", cmd_l2cap_disconnect, 2, 0),
	SHELL_CMD_ARG(send, NULL, "<id> <length of data> <data> ", cmd_l2cap_send, 4, 0),
	SHELL_CMD_ARG(change_mtu, NULL, "<mtu>", cmd_change_mtu, 2, 0),
	SHELL_CMD_ARG(read_mtu, NULL, "<id> <local/peer>", cmd_read_mtu, 3, 0),
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

SHELL_CMD_REGISTER(l2cap_br, &l2cap_br_cmds, "Bluetooth classic l2cap shell commands",
		   cmd_default_handler);
