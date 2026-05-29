/* smp_br_sc_only.c - Bluetooth classic SMP Secure Connections Only Mode smoke test */

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

#include <zephyr/shell/shell.h>

#include "host/shell/bt.h"
#include "common/bt_shell_private.h"

#define DATA_BREDR_MTU 48

NET_BUF_POOL_FIXED_DEFINE(data_rx_pool, 1, DATA_BREDR_MTU, 8, NULL);

struct l2cap_br_chan {
	bool active;
	struct bt_l2cap_br_chan chan;
};

#define APPL_L2CAP_CONNECTION_MAX_COUNT 1
static struct l2cap_br_chan l2cap_chans[APPL_L2CAP_CONNECTION_MAX_COUNT];
static struct bt_l2cap_server l2cap_servers[APPL_L2CAP_CONNECTION_MAX_COUNT];

static int l2cap_recv(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	struct l2cap_br_chan *br_chan = CONTAINER_OF(
		CONTAINER_OF(chan, struct bt_l2cap_br_chan, chan), struct l2cap_br_chan, chan);

	bt_shell_print("Incoming data channel %d len %u", ARRAY_INDEX(l2cap_chans, br_chan),
		       buf->len);

	if (buf->len) {
		bt_shell_hexdump(buf->data, buf->len);
	}

	return 0;
}

static void l2cap_connected(struct bt_l2cap_chan *chan)
{
	struct l2cap_br_chan *br_chan = CONTAINER_OF(
		CONTAINER_OF(chan, struct bt_l2cap_br_chan, chan), struct l2cap_br_chan, chan);

	bt_shell_print("Channel %d connected", ARRAY_INDEX(l2cap_chans, br_chan));
}

static void l2cap_disconnected(struct bt_l2cap_chan *chan)
{
	struct l2cap_br_chan *br_chan = CONTAINER_OF(
		CONTAINER_OF(chan, struct bt_l2cap_br_chan, chan), struct l2cap_br_chan, chan);

	br_chan->active = false;
	bt_shell_print("Channel %d disconnected", ARRAY_INDEX(l2cap_chans, br_chan));
}

static struct net_buf *l2cap_alloc_buf(struct bt_l2cap_chan *chan)
{
	bt_shell_print("Channel %p requires buffer", chan);

	return net_buf_alloc(&data_rx_pool, K_NO_WAIT);
}

static const struct bt_l2cap_chan_ops l2cap_ops = {
	.alloc_buf = l2cap_alloc_buf,
	.recv = l2cap_recv,
	.connected = l2cap_connected,
	.disconnected = l2cap_disconnected,
};

static struct l2cap_br_chan *l2cap_alloc_chan(void)
{
	ARRAY_FOR_EACH(l2cap_chans, index) {
		if (l2cap_chans[index].active == false) {
			l2cap_chans[index].active = true;
			l2cap_chans[index].chan.chan.ops = &l2cap_ops;
			l2cap_chans[index].chan.rx.mtu = DATA_BREDR_MTU;
			return &l2cap_chans[index];
		}
	}
	return NULL;
}

static int l2cap_accept(struct bt_conn *conn, struct bt_l2cap_server *server,
			struct bt_l2cap_chan **chan)
{
	struct l2cap_br_chan *br_chan;

	bt_shell_print("Incoming BR/EDR conn %p", conn);

	br_chan = l2cap_alloc_chan();
	if (br_chan == NULL) {
		bt_shell_error("No channels available");
		return -ENOMEM;
	}

	*chan = &br_chan->chan.chan;

	return 0;
}

static struct bt_l2cap_server *l2cap_alloc_server(uint16_t psm)
{
	ARRAY_FOR_EACH(l2cap_servers, index) {
		if (l2cap_servers[index].psm == 0) {
			l2cap_servers[index].psm = psm;
			l2cap_servers[index].accept = l2cap_accept;
			return &l2cap_servers[index];
		}
	}
	return NULL;
}

static int cmd_l2cap_register(const struct shell *sh, size_t argc, char *argv[])
{
	uint16_t psm = strtoul(argv[1], NULL, 16);
	struct bt_l2cap_server *br_server;

	ARRAY_FOR_EACH(l2cap_servers, index) {
		if (l2cap_servers[index].psm == psm) {
			shell_print(sh, "Already registered");
			return -ENOEXEC;
		}
	}

	br_server = l2cap_alloc_server(psm);
	if (br_server == NULL) {
		shell_error(sh, "No servers available");
		return -ENOMEM;
	}

	if ((argc > 3) && (strcmp(argv[2], "sec") == 0)) {
		br_server->sec_level = strtoul(argv[3], NULL, 16);
	} else {
		br_server->sec_level = BT_SECURITY_L1;
	}

	if (bt_l2cap_br_server_register(br_server) < 0) {
		br_server->psm = 0U;
		shell_error(sh, "Unable to register psm");
		return -ENOEXEC;
	}

	shell_print(sh, "L2CAP psm %u registered", br_server->psm);

	return 0;
}

static int cmd_l2cap_connect(const struct shell *sh, size_t argc, char *argv[])
{
	int err;
	struct bt_conn_info info;
	struct l2cap_br_chan *br_chan;
	uint16_t psm;

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	br_chan = l2cap_alloc_chan();
	if (br_chan == NULL) {
		shell_error(sh, "No channels available");
		br_chan->active = false;
		return -ENOMEM;
	}

	err = bt_conn_get_info(default_conn, &info);
	if ((err < 0) || (info.type != BT_CONN_TYPE_BR)) {
		shell_error(sh, "Invalid conn type");
		br_chan->active = false;
		return -ENOEXEC;
	}

	psm = strtoul(argv[1], NULL, 16);

	if ((argc > 3) && (strcmp(argv[2], "sec") == 0)) {
		br_chan->chan.required_sec_level = strtoul(argv[3], NULL, 16);
	} else {
		br_chan->chan.required_sec_level = BT_SECURITY_L1;
	}

	err = bt_l2cap_chan_connect(default_conn, &br_chan->chan.chan, psm);
	if (err < 0) {
		br_chan->active = false;
		shell_error(sh, "Unable to connect to psm %u (err %d)", psm, err);
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
	if ((id >= ARRAY_SIZE(l2cap_chans)) || (!l2cap_chans[id].active)) {
		shell_print(sh, "channel %d not connected", id);
		return -ENOEXEC;
	}

	err = bt_l2cap_chan_disconnect(&l2cap_chans[id].chan.chan);
	if (err) {
		shell_error(sh, "Unable to disconnect: %u", -err);
		return err;
	}

	return 0;
}

static int cmd_set_security(const struct shell *sh, size_t argc, char *argv[])
{
	uint16_t psm = strtoul(argv[1], NULL, 16);
	uint8_t sec = strtoul(argv[2], NULL, 16);

	if (sec > BT_SECURITY_L4) {
		shell_error(sh, "Invalid security level: %d", sec);
		return -ENOEXEC;
	}

	ARRAY_FOR_EACH(l2cap_servers, index) {
		if (l2cap_servers[index].psm == psm) {
			l2cap_servers[index].sec_level = sec;
			shell_print(sh, "L2CAP psm %u security level %u", psm, sec);
			return 0;
		}
	}

	shell_error(sh, "L2CAP psm %u not registered", psm);
	return -ENOEXEC;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	l2cap_br_cmds,
	SHELL_CMD_ARG(register, NULL, "<psm> [sec] [sec: 0 - 4]", cmd_l2cap_register, 2, 2),
	SHELL_CMD_ARG(connect, NULL, "<psm> [sec] [sec: 0 - 4]", cmd_l2cap_connect, 2, 2),
	SHELL_CMD_ARG(disconnect, NULL, "<id>", cmd_l2cap_disconnect, 2, 0),
	SHELL_CMD_ARG(security, NULL, "<psm> <security level: 0 - 4>", cmd_set_security, 3, 0),
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
