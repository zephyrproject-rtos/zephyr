/** @file
 *  @brief Audio Video Remote Control Profile shell functions.
 */

/*
 * Copyright (c) 2024 Xiaomi InC.
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

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/classic/avrcp.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/l2cap.h>

#include <zephyr/shell/shell.h>

#include "host/shell/bt.h"
#include "common/bt_shell_private.h"

struct bt_avrcp *default_avrcp;
static bool avrcp_registered;

static void avrcp_connected(struct bt_avrcp *avrcp)
{
	default_avrcp = avrcp;
	bt_shell_print("AVRCP connected");
}

static void avrcp_disconnected(struct bt_avrcp *avrcp)
{
	bt_shell_print("AVRCP disconnected");
}

static void avrcp_unit_info_rsp(struct bt_avrcp *avrcp, struct bt_avrcp_unit_info_rsp *rsp)
{
	bt_shell_print("AVRCP unit info received, unit type = 0x%02x, company_id = 0x%06x",
		       rsp->unit_type, rsp->company_id);
}

static void avrcp_subunit_info_rsp(struct bt_avrcp *avrcp, struct bt_avrcp_subunit_info_rsp *rsp)
{
	int i;

	bt_shell_print("AVRCP subunit info received, subunit type = 0x%02x, extended subunit = %d",
		       rsp->subunit_type, rsp->max_subunit_id);
	for (i = 0; i < rsp->max_subunit_id; i++) {
		bt_shell_print("extended subunit id = %d, subunit type = 0x%02x",
			       rsp->extended_subunit_id[i], rsp->extended_subunit_type[i]);
	}
}

static void avrcp_passthrough_rsp(struct bt_avrcp *avrcp, struct bt_avrcp_passthrough_rsp *rsp)
{
	if (rsp->response == BT_AVRCP_RSP_ACCEPTED) {
		bt_shell_print(
			"AVRCP passthough command accepted, operation id = 0x%02x, state = %d",
			rsp->operation_id, rsp->state);
	} else {
		bt_shell_print("AVRCP passthough command rejected, operation id = 0x%02x, state = "
			       "%d, response = %d",
			       rsp->operation_id, rsp->state, rsp->response);
	}
}

static struct bt_avrcp_cb avrcp_cb = {
	.connected = avrcp_connected,
	.disconnected = avrcp_disconnected,
	.unit_info_rsp = avrcp_unit_info_rsp,
	.subunit_info_rsp = avrcp_subunit_info_rsp,
	.passthrough_rsp = avrcp_passthrough_rsp,
};

static int register_cb(const struct shell *sh)
{
	int err;

	if (avrcp_registered) {
		return 0;
	}

	err = bt_avrcp_register_cb(&avrcp_cb);
	if (!err) {
		avrcp_registered = true;
		shell_print(sh, "AVRCP callbacks registered");
	} else {
		shell_print(sh, "failed to register callbacks");
	}

	return err;
}

static int cmd_register_cb(const struct shell *sh, int32_t argc, char *argv[])
{
	if (avrcp_registered) {
		shell_print(sh, "already registered");
		return 0;
	}

	register_cb(sh);

	return 0;
}

static int cmd_connect(const struct shell *sh, int32_t argc, char *argv[])
{
	if (!avrcp_registered) {
		if (register_cb(sh) != 0) {
			return -ENOEXEC;
		}
	}

	if (!default_conn) {
		shell_error(sh, "BR/EDR not connected");
		return -ENOEXEC;
	}

	default_avrcp = bt_avrcp_connect(default_conn);
	if (NULL == default_avrcp) {
		shell_error(sh, "fail to connect AVRCP");
	}

	return 0;
}

static int cmd_disconnect(const struct shell *sh, int32_t argc, char *argv[])
{
	if (!avrcp_registered) {
		if (register_cb(sh) != 0) {
			return -ENOEXEC;
		}
	}

	if (default_avrcp != NULL) {
		bt_avrcp_disconnect(default_avrcp);
		default_avrcp = NULL;
	} else {
		shell_error(sh, "AVRCP is not connected");
	}

	return 0;
}

static int cmd_get_unit_info(const struct shell *sh, int32_t argc, char *argv[])
{
	if (!avrcp_registered) {
		if (register_cb(sh) != 0) {
			return -ENOEXEC;
		}
	}

	if (default_avrcp != NULL) {
		bt_avrcp_get_unit_info(default_avrcp);
	} else {
		shell_error(sh, "AVRCP is not connected");
	}

	return 0;
}

static int cmd_get_subunit_info(const struct shell *sh, int32_t argc, char *argv[])
{
	if (!avrcp_registered) {
		if (register_cb(sh) != 0) {
			return -ENOEXEC;
		}
	}

	if (default_avrcp != NULL) {
		bt_avrcp_get_subunit_info(default_avrcp);
	} else {
		shell_error(sh, "AVRCP is not connected");
	}

	return 0;
}

static int cmd_passthrough(const struct shell *sh, bt_avrcp_opid_t operation_id,
			   const uint8_t *payload, uint8_t len)
{
	if (!avrcp_registered) {
		if (register_cb(sh) != 0) {
			return -ENOEXEC;
		}
	}

	if (default_avrcp != NULL) {
		bt_avrcp_passthrough(default_avrcp, operation_id, BT_AVRCP_BUTTON_PRESSED, payload,
				     len);
		bt_avrcp_passthrough(default_avrcp, operation_id, BT_AVRCP_BUTTON_RELEASED, payload,
				     len);
	} else {
		shell_error(sh, "AVRCP is not connected");
	}

	return 0;
}

static int cmd_play(const struct shell *sh, int32_t argc, char *argv[])
{
	return cmd_passthrough(sh, BT_AVRCP_OPID_PLAY, NULL, 0);
}

static int cmd_pause(const struct shell *sh, int32_t argc, char *argv[])
{
	return cmd_passthrough(sh, BT_AVRCP_OPID_PAUSE, NULL, 0);
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	avrcp_cmds,
	SHELL_CMD_ARG(register_cb, NULL, "register avrcp callbacks", cmd_register_cb, 1, 0),
	SHELL_CMD_ARG(connect, NULL, "connect AVRCP", cmd_connect, 1, 0),
	SHELL_CMD_ARG(disconnect, NULL, "disconnect AVRCP", cmd_disconnect, 1, 0),
	SHELL_CMD_ARG(get_unit, NULL, "get unit info", cmd_get_unit_info, 1, 0),
	SHELL_CMD_ARG(get_subunit, NULL, "get subunit info", cmd_get_subunit_info, 1, 0),
	SHELL_CMD_ARG(play, NULL, "request a play at the remote player", cmd_play, 1, 0),
	SHELL_CMD_ARG(pause, NULL, "request a pause at the remote player", cmd_pause, 1, 0),
	SHELL_SUBCMD_SET_END);

static int cmd_avrcp(const struct shell *sh, size_t argc, char **argv)
{
	if (argc == 1) {
		shell_help(sh);
		/* sh returns 1 when help is printed */
		return 1;
	}

	shell_error(sh, "%s unknown parameter: %s", argv[0], argv[1]);

	return -ENOEXEC;
}

SHELL_CMD_ARG_REGISTER(avrcp, &avrcp_cmds, "Bluetooth AVRCP sh commands", cmd_avrcp, 1, 1);
