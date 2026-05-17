/*
 * Copyright 2026 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/classic/hid_device.h>
#include <zephyr/bluetooth/classic/hid_host.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_string_conv.h>

#include "host/shell/bt.h"
#include "common/bt_shell_private.h"

#define HID_HOST_TX_BUF_COUNT 4

NET_BUF_POOL_FIXED_DEFINE(hid_host_pool, HID_HOST_TX_BUF_COUNT,
			  BT_L2CAP_BUF_SIZE(CONFIG_BT_L2CAP_TX_MTU),
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

static struct bt_hid_host *default_hid_host;
static bool host_registered;

static void host_connected_cb(struct bt_hid_host *hid)
{
	bt_shell_print("HID Host: connected (%p)", hid);
	default_hid_host = hid;
}

static void host_disconnected_cb(struct bt_hid_host *hid)
{
	bt_shell_print("HID Host: disconnected");
	if (default_hid_host == hid) {
		default_hid_host = NULL;
	}
}

static void host_input_report_cb(struct bt_hid_host *hid, uint8_t report_id,
				 struct net_buf *buf)
{
	bt_shell_print("HID Host: input_report id %u len %u", report_id, buf->len);
	bt_shell_hexdump(buf->data, buf->len);
}

static void host_get_report_rsp_cb(struct bt_hid_host *hid, uint8_t type,
				   uint8_t report_id, struct net_buf *buf)
{
	bt_shell_print("HID Host: get_report_rsp type %u id %u len %u",
		       type, report_id, buf->len);
	bt_shell_hexdump(buf->data, buf->len);
}

static void host_handshake_cb(struct bt_hid_host *hid, uint8_t result)
{
	bt_shell_print("HID Host: handshake %u", result);
}

static void host_protocol_mode_cb(struct bt_hid_host *hid, uint8_t mode)
{
	bt_shell_print("HID Host: protocol_mode %u (%s)", mode,
		       mode == 0 ? "boot" : "report");
}

static void host_vc_unplug_cb(struct bt_hid_host *hid)
{
	bt_shell_print("HID Host: virtual_cable_unplug");
	default_hid_host = NULL;
}

static const struct bt_hid_host_cb host_cb = {
	.connected = host_connected_cb,
	.disconnected = host_disconnected_cb,
	.input_report = host_input_report_cb,
	.get_report_rsp = host_get_report_rsp_cb,
	.handshake = host_handshake_cb,
	.protocol_mode = host_protocol_mode_cb,
	.vc_unplug = host_vc_unplug_cb,
};

static int cmd_register(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	if (host_registered) {
		shell_error(sh, "already registered");
		return -EALREADY;
	}

	err = bt_hid_host_register(&host_cb);
	if (err) {
		shell_error(sh, "register failed (%d)", err);
		return err;
	}

	host_registered = true;
	shell_print(sh, "registered");
	return 0;
}

static int cmd_unregister(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	if (!host_registered) {
		shell_error(sh, "not registered");
		return -ENOEXEC;
	}

	err = bt_hid_host_unregister();
	if (err) {
		shell_error(sh, "unregister failed (%d)", err);
		return err;
	}

	host_registered = false;
	default_hid_host = NULL;
	shell_print(sh, "unregistered");
	return 0;
}

static int cmd_connect(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	if (!host_registered) {
		shell_error(sh, "not registered");
		return -ENOEXEC;
	}

	if (!default_conn) {
		shell_error(sh, "not connected");
		return -ENOEXEC;
	}

	err = bt_hid_host_connect(default_conn, &default_hid_host);
	if (err) {
		shell_error(sh, "connect failed (%d)", err);
		return err;
	}

	return 0;
}

static int cmd_disconnect(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	if (!default_hid_host) {
		shell_error(sh, "not connected");
		return -ENOEXEC;
	}

	err = bt_hid_host_disconnect(default_hid_host);
	if (err) {
		shell_error(sh, "disconnect failed (%d)", err);
		return err;
	}

	return 0;
}

static int cmd_get_report(const struct shell *sh, size_t argc, char *argv[])
{
	int err = 0;
	long type, report_id, buf_size;

	if (!default_hid_host) {
		shell_error(sh, "not connected");
		return -ENOEXEC;
	}

	type = shell_strtol(argv[1], 0, &err);
	report_id = shell_strtol(argv[2], 0, &err);
	buf_size = shell_strtol(argv[3], 0, &err);
	if (err) {
		shell_error(sh, "invalid parameter");
		return -EINVAL;
	}

	err = bt_hid_host_get_report(default_hid_host, (uint8_t)type,
				     (uint8_t)report_id, (uint16_t)buf_size);
	if (err) {
		shell_error(sh, "get_report failed (%d)", err);
		return err;
	}

	return 0;
}

static int cmd_set_report(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_buf *buf;
	int err = 0;
	long type;

	if (!default_hid_host) {
		shell_error(sh, "not connected");
		return -ENOEXEC;
	}

	type = shell_strtol(argv[1], 0, &err);
	if (err || argc < 3) {
		shell_error(sh, "Usage: set_report <type> <hex bytes...>");
		return -EINVAL;
	}

	buf = bt_hid_host_create_pdu(&hid_host_pool);
	if (!buf) {
		shell_error(sh, "no buffer");
		return -ENOMEM;
	}

	for (int i = 2; i < argc; i++) {
		long val = shell_strtol(argv[i], 16, &err);

		if (err) {
			net_buf_unref(buf);
			shell_error(sh, "invalid hex byte");
			return -EINVAL;
		}
		net_buf_add_u8(buf, (uint8_t)val);
	}

	err = bt_hid_host_set_report(default_hid_host, (uint8_t)type, buf);
	if (err) {
		net_buf_unref(buf);
		shell_error(sh, "set_report failed (%d)", err);
		return err;
	}

	return 0;
}

static int cmd_get_protocol(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	if (!default_hid_host) {
		shell_error(sh, "not connected");
		return -ENOEXEC;
	}

	err = bt_hid_host_get_protocol(default_hid_host);
	if (err) {
		shell_error(sh, "get_protocol failed (%d)", err);
		return err;
	}

	return 0;
}

static int cmd_set_protocol(const struct shell *sh, size_t argc, char *argv[])
{
	int err;
	uint8_t protocol;

	if (!default_hid_host) {
		shell_error(sh, "not connected");
		return -ENOEXEC;
	}

	if (strcmp(argv[1], "boot") == 0) {
		protocol = BT_HID_PROTOCOL_BOOT_MODE;
	} else if (strcmp(argv[1], "report") == 0) {
		protocol = BT_HID_PROTOCOL_REPORT_MODE;
	} else {
		shell_error(sh, "Usage: set_protocol <boot|report>");
		return -EINVAL;
	}

	err = bt_hid_host_set_protocol(default_hid_host, protocol);
	if (err) {
		shell_error(sh, "set_protocol failed (%d)", err);
		return err;
	}

	return 0;
}

static int cmd_send(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_buf *buf;
	int err = 0;

	if (!default_hid_host) {
		shell_error(sh, "not connected");
		return -ENOEXEC;
	}

	if (argc < 2) {
		shell_error(sh, "Usage: send <hex bytes...>");
		return -EINVAL;
	}

	buf = bt_hid_host_create_pdu(&hid_host_pool);
	if (!buf) {
		shell_error(sh, "no buffer");
		return -ENOMEM;
	}

	for (int i = 1; i < argc; i++) {
		long val = shell_strtol(argv[i], 16, &err);

		if (err) {
			net_buf_unref(buf);
			shell_error(sh, "invalid hex byte");
			return -EINVAL;
		}
		net_buf_add_u8(buf, (uint8_t)val);
	}

	err = bt_hid_host_send_output_report(default_hid_host, buf);
	if (err) {
		net_buf_unref(buf);
		shell_error(sh, "send failed (%d)", err);
		return err;
	}

	shell_print(sh, "sent");
	return 0;
}

static int cmd_suspend(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	if (!default_hid_host) {
		shell_error(sh, "not connected");
		return -ENOEXEC;
	}

	err = bt_hid_host_suspend(default_hid_host);
	if (err) {
		shell_error(sh, "suspend failed (%d)", err);
		return err;
	}

	shell_print(sh, "suspended");
	return 0;
}

static int cmd_exit_suspend(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	if (!default_hid_host) {
		shell_error(sh, "not connected");
		return -ENOEXEC;
	}

	err = bt_hid_host_exit_suspend(default_hid_host);
	if (err) {
		shell_error(sh, "exit_suspend failed (%d)", err);
		return err;
	}

	shell_print(sh, "exit_suspend");
	return 0;
}

static int cmd_vcu(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	if (!default_hid_host) {
		shell_error(sh, "not connected");
		return -ENOEXEC;
	}

	err = bt_hid_host_virtual_cable_unplug(default_hid_host);
	if (err) {
		shell_error(sh, "vcu failed (%d)", err);
		return err;
	}

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(hid_host_cmds,
	SHELL_CMD_ARG(register, NULL, "register HID Host", cmd_register, 1, 0),
	SHELL_CMD_ARG(unregister, NULL, "unregister HID Host", cmd_unregister, 1, 0),
	SHELL_CMD_ARG(connect, NULL, "connect to HID Device", cmd_connect, 1, 0),
	SHELL_CMD_ARG(disconnect, NULL, "disconnect", cmd_disconnect, 1, 0),
	SHELL_CMD_ARG(get_report, NULL, "<type> <id> <buf_size>", cmd_get_report, 4, 0),
	SHELL_CMD_ARG(set_report, NULL, "<type> <hex...>", cmd_set_report, 3, 8),
	SHELL_CMD_ARG(get_protocol, NULL, "get protocol mode", cmd_get_protocol, 1, 0),
	SHELL_CMD_ARG(set_protocol, NULL, "<boot|report>", cmd_set_protocol, 2, 0),
	SHELL_CMD_ARG(send, NULL, "<hex bytes...> output report", cmd_send, 2, 8),
	SHELL_CMD_ARG(suspend, NULL, "send SUSPEND", cmd_suspend, 1, 0),
	SHELL_CMD_ARG(exit_suspend, NULL, "send EXIT_SUSPEND", cmd_exit_suspend, 1, 0),
	SHELL_CMD_ARG(vcu, NULL, "virtual cable unplug", cmd_vcu, 1, 0),
	SHELL_SUBCMD_SET_END
);

static int cmd_hid_host(const struct shell *sh, size_t argc, char **argv)
{
	if (argc == 1) {
		shell_help(sh);
		return SHELL_CMD_HELP_PRINTED;
	}

	shell_error(sh, "%s unknown parameter: %s", argv[0], argv[1]);
	return -ENOEXEC;
}

SHELL_CMD_ARG_REGISTER(hid_host, &hid_host_cmds, "Bluetooth HID Host commands",
		       cmd_hid_host, 1, 0);
