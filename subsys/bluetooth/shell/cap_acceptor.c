/**
 * @file
 * @brief Shell APIs for Bluetooth CAP acceptor
 *
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdlib.h>

#include <zephyr/types.h>
#include <zephyr/shell/shell.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/audio/cap.h>
#include "bt.h"

extern const struct shell *ctx_shell;
static struct bt_csip_set_member_svc_inst *cap_csip_svc_inst;
static uint8_t sirk_read_rsp = BT_CSIP_READ_SIRK_REQ_RSP_ACCEPT;

static void locked_cb(struct bt_conn *conn,
		      struct bt_csip_set_member_svc_inst *svc_inst,
		      bool locked)
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

static uint8_t sirk_read_req_cb(struct bt_conn *conn,
				struct bt_csip_set_member_svc_inst *svc_inst)
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

static struct bt_csip_set_member_cb csip_set_member_cbs = {
	.lock_changed = locked_cb,
	.sirk_read_req = sirk_read_req_cb,
};

static int cmd_cap_acceptor_init(const struct shell *sh, size_t argc,
				 char **argv)
{
	int err;
	struct bt_csip_set_member_register_param param = {
		.set_size = 2,
		.rank = 1,
		.lockable = true,
		/* Using the CSIS test sample SIRK */
		.set_sirk = { 0xcd, 0xcc, 0x72, 0xdd, 0x86, 0x8c, 0xcd, 0xce,
			      0x22, 0xfd, 0xa1, 0x21, 0x09, 0x7d, 0x7d, 0x45 },
		.cb = &csip_set_member_cbs
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

	err = bt_cap_acceptor_register(&param, &cap_csip_svc_inst);
	if (err != 0) {
		shell_error(sh, "Could not register CAS: %d", err);

		return err;
	}

	return 0;
}

static int cmd_cap_acceptor_print_sirk(const struct shell *sh, size_t argc,
				       char *argv[])
{
	bt_csip_set_member_print_sirk(cap_csip_svc_inst);

	return 0;
}

static int cmd_cap_acceptor_lock(const struct shell *sh, size_t argc,
				 char *argv[])
{
	int err;

	err = bt_csip_set_member_lock(cap_csip_svc_inst, true, false);
	if (err != 0) {
		shell_error(sh, "Failed to set lock: %d", err);

		return -ENOEXEC;
	}

	shell_print(sh, "Set locked");

	return 0;
}

static int cmd_cap_acceptor_release(const struct shell *sh, size_t argc,
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

	err = bt_csip_set_member_lock(cap_csip_svc_inst, false, force);

	if (err != 0) {
		shell_error(sh, "Failed to release lock: %d", err);

		return -ENOEXEC;
	}

	shell_print(sh, "Set released");

	return 0;
}

static int cmd_cap_acceptor_set_sirk_rsp(const struct shell *sh, size_t argc,
					 char *argv[])
{
	if (strcmp(argv[1], "accept") == 0) {
		sirk_read_rsp = BT_CSIP_READ_SIRK_REQ_RSP_ACCEPT;
	} else if (strcmp(argv[1], "accept_enc") == 0) {
		sirk_read_rsp = BT_CSIP_READ_SIRK_REQ_RSP_ACCEPT_ENC;
	} else if (strcmp(argv[1], "reject") == 0) {
		sirk_read_rsp = BT_CSIP_READ_SIRK_REQ_RSP_REJECT;
	} else if (strcmp(argv[1], "oob") == 0) {
		sirk_read_rsp = BT_CSIP_READ_SIRK_REQ_RSP_OOB_ONLY;
	} else {
		shell_error(sh, "Unknown parameter: %s", argv[1]);
		return -ENOEXEC;
	}

	return 0;
}

static int cmd_cap_acceptor(const struct shell *sh, size_t argc, char **argv)
{
	shell_error(sh, "%s unknown parameter: %s", argv[0], argv[1]);

	return -ENOEXEC;
}

SHELL_STATIC_SUBCMD_SET_CREATE(cap_acceptor_cmds,
	SHELL_CMD_ARG(init, NULL,
		      "Initialize the service and register callbacks "
		      "[size <int>] [rank <int>] [not-lockable] [sirk <data>]",
		      cmd_cap_acceptor_init, 1, 4),
	SHELL_CMD_ARG(lock, NULL,
		      "Lock the set",
		      cmd_cap_acceptor_lock, 1, 0),
	SHELL_CMD_ARG(release, NULL,
		      "Release the set [force]",
		      cmd_cap_acceptor_release, 1, 1),
	SHELL_CMD_ARG(print_sirk, NULL,
		      "Print the currently used SIRK",
		      cmd_cap_acceptor_print_sirk, 1, 0),
	SHELL_CMD_ARG(set_sirk_rsp, NULL,
		      "Set the response used in SIRK requests "
		      "<accept, accept_enc, reject, oob>",
		      cmd_cap_acceptor_set_sirk_rsp, 2, 0),
		      SHELL_SUBCMD_SET_END
);

SHELL_CMD_ARG_REGISTER(cap_acceptor, &cap_acceptor_cmds, "Bluetooth CAP acceptor shell commands",
		       cmd_cap_acceptor, 1, 1);
