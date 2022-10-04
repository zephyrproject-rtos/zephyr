/**
 * @file
 * @brief Shell APIs for Bluetooth CSIS
 *
 * Copyright (c) 2020 Bose Corporation
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>

#include <zephyr/zephyr.h>
#include <zephyr/types.h>
#include <zephyr/shell/shell.h>
#include <stdlib.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/audio/csis.h>
#include "bt.h"

extern const struct shell *ctx_shell;
static struct bt_csis *csis;
static uint8_t sirk_read_rsp = BT_CSIS_READ_SIRK_REQ_RSP_ACCEPT;

static void locked_cb(struct bt_conn *conn, struct bt_csis *csis, bool locked)
{
	if (conn == NULL) {
		shell_error(ctx_shell, "Server %s the device",
			    locked ? "locked" : "released");
	} else {
		char addr[BT_ADDR_LE_STR_LEN];

		conn_addr_str(conn, addr, sizeof(addr));

		shell_print(ctx_shell, "Client %s %s the device",
			    addr, locked ? "locked" : "released");
	}
}

static uint8_t sirk_read_req_cb(struct bt_conn *conn, struct bt_csis *csis)
{
	char addr[BT_ADDR_LE_STR_LEN];
	static const char *const rsp_strings[] = {
		"Accept", "Accept Enc", "Reject", "OOB only"
	};

	conn_addr_str(conn, addr, sizeof(addr));

	shell_print(ctx_shell, "Client %s requested to read the sirk. "
		    "Responding with %s", addr, rsp_strings[sirk_read_rsp]);

	return sirk_read_rsp;
}

struct bt_csis_cb csis_cbs = {
	.lock_changed = locked_cb,
	.sirk_read_req = sirk_read_req_cb,
};

static int cmd_csis_register(const struct shell *sh, size_t argc, char **argv)
{
	int err;
	struct bt_csis_register_param param = {
		.set_size = 2,
		.rank = 1,
		.lockable = true,
		/* Using the CSIS test sample SIRK */
		.set_sirk = { 0xcd, 0xcc, 0x72, 0xdd, 0x86, 0x8c, 0xcd, 0xce,
			      0x22, 0xfd, 0xa1, 0x21, 0x09, 0x7d, 0x7d, 0x45 },
		.cb = &csis_cbs
	};

	for (size_t argn = 1; argn < argc; argn++) {
		const char *arg = argv[argn];

		if (strcmp(arg, "size") == 0) {
			param.set_size = strtol(argv[++argn], NULL, 10);
		} else if (strcmp(arg, "rank") == 0) {
			param.rank = strtol(argv[++argn], NULL, 10);
		} else if (strcmp(arg, "not-lockable") == 0) {
			param.lockable = false;
		} else if (strcmp(arg, "sirk") == 0) {
			size_t len;

			argn++;
			len = hex2bin(argv[argn], strlen(argv[argn]),
				      param.set_sirk, sizeof(param.set_sirk));
			if (len == 0) {
				shell_error(sh, "Could not parse SIRK");
				return -ENOEXEC;
			}
		} else {
			shell_help(sh);
			return SHELL_CMD_HELP_PRINTED;
		}
	}

	err = bt_csis_register(&param, &csis);
	if (err != 0) {
		shell_error(sh, "Could not register CSIS: %d", err);
		return err;
	}

	return 0;
}

static int cmd_csis_advertise(const struct shell *sh, size_t argc,
			      char *argv[])
{
	int err;

	if (strcmp(argv[1], "off") == 0) {
		err = bt_csis_advertise(csis, false);
		if (err != 0) {
			shell_error(sh, "Failed to stop advertising %d", err);
			return -ENOEXEC;
		}
		shell_print(sh, "Advertising stopped");
	} else if (strcmp(argv[1], "on") == 0) {
		err = bt_csis_advertise(csis, true);
		if (err != 0) {
			shell_error(sh, "Failed to start advertising %d", err);
			return -ENOEXEC;
		}
		shell_print(sh, "Advertising started");
	} else {
		shell_error(sh, "Invalid argument: %s", argv[1]);
		return -ENOEXEC;
	}

	return 0;
}

static int cmd_csis_update_rsi(const struct shell *sh, size_t argc,
				char *argv[])
{
	int err;

	if (bt_csis_advertise(csis, false) != 0) {
		shell_error(sh,
			    "Failed to stop advertising - rsi not updated");
		return -ENOEXEC;
	}
	err = bt_csis_advertise(csis, true);
	if (err != 0) {
		shell_error(sh,
			    "Failed to start advertising  - rsi not updated");
		return -ENOEXEC;
	}

	shell_print(sh, "RSI and optionally RPA updated");

	return 0;
}

static int cmd_csis_print_sirk(const struct shell *sh, size_t argc,
			       char *argv[])
{
	bt_csis_print_sirk(csis);
	return 0;
}

static int cmd_csis_lock(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	err = bt_csis_lock(csis, true, false);
	if (err != 0) {
		shell_error(sh, "Failed to set lock: %d", err);
		return -ENOEXEC;
	}

	shell_print(sh, "Set locked");

	return 0;
}

static int cmd_csis_release(const struct shell *sh, size_t argc,
			    char *argv[])
{
	bool force = false;
	int err;

	if (argc > 1) {
		if (strcmp(argv[1], "force") == 0) {
			force = true;
		} else {
			shell_error(sh, "Unknown parameter: %s", argv[1]);
			return -ENOEXEC;
		}
	}

	err = bt_csis_lock(csis, false, force);

	if (err != 0) {
		shell_error(sh, "Failed to release lock: %d", err);
		return -ENOEXEC;
	}

	shell_print(sh, "Set released");

	return 0;
}

static int cmd_csis_set_sirk_rsp(const struct shell *sh, size_t argc,
				 char *argv[])
{
	if (strcmp(argv[1], "accept") == 0) {
		sirk_read_rsp = BT_CSIS_READ_SIRK_REQ_RSP_ACCEPT;
	} else if (strcmp(argv[1], "accept_enc") == 0) {
		sirk_read_rsp = BT_CSIS_READ_SIRK_REQ_RSP_ACCEPT_ENC;
	} else if (strcmp(argv[1], "reject") == 0) {
		sirk_read_rsp = BT_CSIS_READ_SIRK_REQ_RSP_REJECT;
	} else if (strcmp(argv[1], "oob") == 0) {
		sirk_read_rsp = BT_CSIS_READ_SIRK_REQ_RSP_OOB_ONLY;
	} else {
		shell_error(sh, "Unknown parameter: %s", argv[1]);
		return -ENOEXEC;
	}

	return 0;
}

static int cmd_csis(const struct shell *sh, size_t argc, char **argv)
{
	shell_error(sh, "%s unknown parameter: %s", argv[0], argv[1]);

	return -ENOEXEC;
}

SHELL_STATIC_SUBCMD_SET_CREATE(csis_cmds,
	SHELL_CMD_ARG(register, NULL,
		      "Initialize the service and register callbacks "
		      "[size <int>] [rank <int>] [not-lockable] [sirk <data>]",
		      cmd_csis_register, 1, 4),
	SHELL_CMD_ARG(advertise, NULL,
		      "Start/stop advertising CSIS RSIs <on/off>",
		      cmd_csis_advertise, 2, 0),
	SHELL_CMD_ARG(update_rsi, NULL,
		      "Update the advertised RSI",
		      cmd_csis_update_rsi, 1, 0),
	SHELL_CMD_ARG(lock, NULL,
		      "Lock the set",
		      cmd_csis_lock, 1, 0),
	SHELL_CMD_ARG(release, NULL,
		      "Release the set [force]",
		      cmd_csis_release, 1, 1),
	SHELL_CMD_ARG(print_sirk, NULL,
		      "Print the currently used SIRK",
		      cmd_csis_print_sirk, 1, 0),
	SHELL_CMD_ARG(set_sirk_rsp, NULL,
		      "Set the response used in SIRK requests "
		      "<accept, accept_enc, reject, oob>",
		      cmd_csis_set_sirk_rsp, 2, 0),
		      SHELL_SUBCMD_SET_END
);

SHELL_CMD_ARG_REGISTER(csis, &csis_cmds, "Bluetooth CSIS shell commands",
		       cmd_csis, 1, 1);
