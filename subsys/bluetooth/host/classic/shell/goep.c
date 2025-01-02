/** @file
 * @brief Bluetooth GOEP shell module
 *
 * Provide some Bluetooth shell commands that can be useful to applications.
 */
/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/types.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/bluetooth/classic/rfcomm.h>
#include <zephyr/bluetooth/classic/sdp.h>
#include <zephyr/bluetooth/classic/goep.h>

#include <zephyr/shell/shell.h>

#include "host/shell/bt.h"
#include "common/bt_shell_private.h"

struct bt_goep_app {
	struct bt_goep goep;
	struct bt_conn *conn;
};

static struct bt_goep_app goep_app;

static struct bt_goep_transport_rfcomm_server rfcomm_server;
static struct bt_goep_transport_l2cap_server l2cap_server;

static struct bt_goep_app *goep_alloc(struct bt_conn *conn)
{
	if (goep_app.conn) {
		return NULL;
	}

	goep_app.conn = conn;
	return &goep_app;
}

static void goep_free(struct bt_goep_app *goep)
{
	goep->conn = NULL;
}

void goep_transport_connected(struct bt_conn *conn, struct bt_goep *goep)
{
	bt_shell_print("GOEP %p transport connected on %p", goep, conn);
}

void goep_transport_disconnected(struct bt_goep *goep)
{
	struct bt_goep_app *g_app = CONTAINER_OF(goep, struct bt_goep_app, goep);

	goep_free(g_app);
	bt_shell_print("GOEP %p transport disconnected", goep);
}

struct bt_goep_transport_ops goep_transport_ops = {
	.connected = goep_transport_connected,
	.disconnected = goep_transport_disconnected,
};

static void goep_server_connect(struct bt_obex *obex, uint8_t version, uint16_t mopl,
				struct net_buf *buf)
{
}

static void goep_server_disconnect(struct bt_obex *obex, struct net_buf *buf)
{
}

static void goep_server_put(struct bt_obex *obex, bool final, struct net_buf *buf)
{
}

static void goep_server_get(struct bt_obex *obex, bool final, struct net_buf *buf)
{
}

static void goep_server_abort(struct bt_obex *obex, struct net_buf *buf)
{
}

static void goep_server_setpath(struct bt_obex *obex, uint8_t flags, struct net_buf *buf)
{
}

static void goep_server_action(struct bt_obex *obex, bool final, struct net_buf *buf)
{
}

struct bt_obex_server_ops goep_server_ops = {
	.connect = goep_server_connect,
	.disconnect = goep_server_disconnect,
	.put = goep_server_put,
	.get = goep_server_get,
	.abort = goep_server_abort,
	.setpath = goep_server_setpath,
	.action = goep_server_action,
};

static void goep_client_connect(struct bt_obex *obex, uint8_t rsp_code, uint8_t version,
				uint16_t mopl, struct net_buf *buf)
{
}

static void goep_client_disconnect(struct bt_obex *obex, uint8_t rsp_code, struct net_buf *buf)
{
}

static void goep_client_put(struct bt_obex *obex, uint8_t rsp_code, struct net_buf *buf)
{
}

static void goep_client_get(struct bt_obex *obex, uint8_t rsp_code, struct net_buf *buf)
{
}

static void goep_client_abort(struct bt_obex *obex, uint8_t rsp_code, struct net_buf *buf)
{
}

static void goep_client_setpath(struct bt_obex *obex, uint8_t rsp_code, struct net_buf *buf)
{
}

static void goep_client_action(struct bt_obex *obex, uint8_t rsp_code, struct net_buf *buf)
{
}

struct bt_obex_client_ops goep_client_ops = {
	.connect = goep_client_connect,
	.disconnect = goep_client_disconnect,
	.put = goep_client_put,
	.get = goep_client_get,
	.abort = goep_client_abort,
	.setpath = goep_client_setpath,
	.action = goep_client_action,
};

static int rfcomm_accept(struct bt_conn *conn, struct bt_goep_transport_rfcomm_server *server,
			 struct bt_goep **goep)
{
	struct bt_goep_app *g_app;

	g_app = goep_alloc(conn);
	if (!g_app) {
		bt_shell_print("Cannot allocate goep instance");
		return -ENOMEM;
	}

	g_app->goep.transport_ops = &goep_transport_ops;
	g_app->goep.obex.server_ops = &goep_server_ops;
	*goep = &g_app->goep;
	return 0;
}

static int cmd_register_rfcomm(const struct shell *sh, size_t argc, char *argv[])
{
	int err;
	uint8_t channel;

	if (rfcomm_server.rfcomm.channel) {
		shell_error(sh, "RFCOMM has been registered");
		return -EBUSY;
	}

	channel = (uint8_t)strtoul(argv[1], NULL, 16);

	rfcomm_server.rfcomm.channel = channel;
	rfcomm_server.accept = rfcomm_accept;
	err = bt_goep_transport_rfcomm_server_register(&rfcomm_server);
	if (err) {
		shell_error(sh, "Fail to register RFCOMM server (error %d)", err);
		rfcomm_server.rfcomm.channel = 0;
		return -ENOEXEC;
	}
	shell_print(sh, "RFCOMM server (channel %02x) is registered", rfcomm_server.rfcomm.channel);
	return 0;
}

static int cmd_connect_rfcomm(const struct shell *sh, size_t argc, char *argv[])
{
	int err;
	struct bt_goep_app *g_app;
	uint8_t channel;

	if (!default_conn) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	channel = (uint8_t)strtoul(argv[1], NULL, 16);
	if (!channel) {
		shell_error(sh, "Invalid channel");
		return -ENOEXEC;
	}

	g_app = goep_alloc(default_conn);
	if (!g_app) {
		shell_error(sh, "Cannot allocate goep instance");
		return -ENOMEM;
	}

	g_app->goep.transport_ops = &goep_transport_ops;
	g_app->goep.obex.client_ops = &goep_client_ops;

	err = bt_goep_transport_rfcomm_connect(default_conn, &g_app->goep, channel);
	if (err) {
		goep_free(g_app);
		shell_error(sh, "Fail to connect to channel %d (err %d)", channel, err);
	} else {
		shell_print(sh, "GOEP RFCOMM connection pending");
	}

	return err;
}

static int cmd_disconnect_rfcomm(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	if (!default_conn) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	if (!goep_app.conn) {
		shell_error(sh, "No goep transport connection");
		return -ENOEXEC;
	}

	err = bt_goep_transport_rfcomm_disconnect(&goep_app.goep);
	if (err) {
		shell_error(sh, "Fail to disconnect to channel (err %d)", err);
	} else {
		shell_print(sh, "GOEP RFCOMM disconnection pending");
	}
	return err;
}

static int l2cap_accept(struct bt_conn *conn, struct bt_goep_transport_l2cap_server *server,
			struct bt_goep **goep)
{
	struct bt_goep_app *g_app;

	g_app = goep_alloc(conn);
	if (!g_app) {
		bt_shell_print("Cannot allocate goep instance");
		return -ENOMEM;
	}

	g_app->goep.transport_ops = &goep_transport_ops;
	g_app->goep.obex.server_ops = &goep_server_ops;
	*goep = &g_app->goep;
	return 0;
}

static int cmd_register_l2cap(const struct shell *sh, size_t argc, char *argv[])
{
	int err;
	uint16_t psm;

	if (l2cap_server.l2cap.psm) {
		shell_error(sh, "L2CAP server has been registered");
		return -EBUSY;
	}

	psm = (uint16_t)strtoul(argv[1], NULL, 16);

	l2cap_server.l2cap.psm = psm;
	l2cap_server.accept = l2cap_accept;
	err = bt_goep_transport_l2cap_server_register(&l2cap_server);
	if (err) {
		shell_error(sh, "Fail to register L2CAP server (error %d)", err);
		l2cap_server.l2cap.psm = 0;
		return -ENOEXEC;
	}
	shell_print(sh, "L2CAP server (psm %04x) is registered", l2cap_server.l2cap.psm);
	return 0;
}

static int cmd_connect_l2cap(const struct shell *sh, size_t argc, char *argv[])
{
	int err;
	struct bt_goep_app *g_app;
	uint16_t psm;

	if (!default_conn) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	psm = (uint16_t)strtoul(argv[1], NULL, 16);
	if (!psm) {
		shell_error(sh, "Invalid psm");
		return -ENOEXEC;
	}

	g_app = goep_alloc(default_conn);
	if (!g_app) {
		shell_error(sh, "Cannot allocate goep instance");
		return -ENOMEM;
	}

	g_app->goep.transport_ops = &goep_transport_ops;
	g_app->goep.obex.client_ops = &goep_client_ops;

	err = bt_goep_transport_l2cap_connect(default_conn, &g_app->goep, psm);
	if (err) {
		goep_free(g_app);
		shell_error(sh, "Fail to connect to PSM %d (err %d)", psm, err);
	} else {
		shell_print(sh, "GOEP L2CAP connection pending");
	}

	return err;
}

static int cmd_disconnect_l2cap(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	if (!default_conn) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	if (!goep_app.conn) {
		shell_error(sh, "No goep transport connection");
		return -ENOEXEC;
	}

	err = bt_goep_transport_l2cap_disconnect(&goep_app.goep);
	if (err) {
		shell_error(sh, "Fail to disconnect L2CAP conn (err %d)", err);
	} else {
		shell_print(sh, "GOEP L2CAP disconnection pending");
	}
	return err;
}

#define HELP_NONE ""

SHELL_STATIC_SUBCMD_SET_CREATE(goep_cmds,
	SHELL_CMD_ARG(register-rfcomm, NULL, "<channel>", cmd_register_rfcomm, 2, 0),
	SHELL_CMD_ARG(connect-rfcomm, NULL, "<channel>", cmd_connect_rfcomm, 2, 0),
	SHELL_CMD_ARG(disconnect-rfcomm, NULL, HELP_NONE, cmd_disconnect_rfcomm, 1, 0),
	SHELL_CMD_ARG(register-l2cap, NULL, "<psm>", cmd_register_l2cap, 2, 0),
	SHELL_CMD_ARG(connect-l2cap, NULL, "<psm>", cmd_connect_l2cap, 2, 0),
	SHELL_CMD_ARG(disconnect-l2cap, NULL, HELP_NONE, cmd_disconnect_l2cap, 1, 0),
	SHELL_SUBCMD_SET_END
);

static int cmd_goep(const struct shell *sh, size_t argc, char **argv)
{
	if (argc == 1) {
		shell_help(sh);
		return SHELL_CMD_HELP_PRINTED;
	}

	shell_error(sh, "%s unknown parameter: %s", argv[0], argv[1]);

	return -ENOEXEC;
}

SHELL_CMD_ARG_REGISTER(goep, &goep_cmds, "Bluetooth GOEP shell commands", cmd_goep, 1, 1);
