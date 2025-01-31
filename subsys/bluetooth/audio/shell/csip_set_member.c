/**
 * @file
 * @brief Shell APIs for Bluetooth CSIP set member
 *
 * Copyright (c) 2020 Bose Corporation
 * Copyright (c) 2021-2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/audio/csip.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/kernel.h>
#include <zephyr/shell/shell_string_conv.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/types.h>
#include <zephyr/shell/shell.h>

#include "host/shell/bt.h"
#include "common/bt_shell_private.h"

struct bt_csip_set_member_svc_inst *svc_inst;
static uint8_t sirk_read_rsp = BT_CSIP_READ_SIRK_REQ_RSP_ACCEPT;

static void locked_cb(struct bt_conn *conn,
		      struct bt_csip_set_member_svc_inst *inst,
		      bool locked)
{
	if (conn == NULL) {
		bt_shell_error("Server %s the device",
			       locked ? "locked" : "released");
	} else {
		char addr[BT_ADDR_LE_STR_LEN];

		conn_addr_str(conn, addr, sizeof(addr));

		bt_shell_print("Client %s %s the device",
			       addr, locked ? "locked" : "released");
	}
}

static uint8_t sirk_read_req_cb(struct bt_conn *conn,
				struct bt_csip_set_member_svc_inst *inst)
{
	char addr[BT_ADDR_LE_STR_LEN];
	static const char *const rsp_strings[] = {
		"Accept", "Accept Enc", "Reject", "OOB only"
	};

	conn_addr_str(conn, addr, sizeof(addr));

	bt_shell_print("Client %s requested to read the sirk. Responding with %s",
		       addr, rsp_strings[sirk_read_rsp]);

	return sirk_read_rsp;
}

static struct bt_csip_set_member_cb csip_set_member_cbs = {
	.lock_changed = locked_cb,
	.sirk_read_req = sirk_read_req_cb,
};

static int cmd_csip_set_member_register(const struct shell *sh, size_t argc, char **argv)
{
	int err;
	struct bt_csip_set_member_register_param param = {
		.set_size = 2,
		.rank = 1,
		.lockable = true,
		/* Using the CSIS test sample SIRK */
		.sirk = { 0xcd, 0xcc, 0x72, 0xdd, 0x86, 0x8c, 0xcd, 0xce,
			  0x22, 0xfd, 0xa1, 0x21, 0x09, 0x7d, 0x7d, 0x45 },
		.cb = &csip_set_member_cbs,
	};

	for (size_t argn = 1; argn < argc; argn++) {
		const char *arg = argv[argn];
		if (strcmp(arg, "size") == 0) {
			unsigned long set_size;

			argn++;
			if (argn == argc) {
				shell_help(sh);
				return SHELL_CMD_HELP_PRINTED;
			}

			set_size = shell_strtoul(argv[argn], 0, &err);
			if (err != 0) {
				shell_error(sh, "Could not parse set_size: %d",
					    err);

				return -ENOEXEC;
			}

			if (set_size > UINT8_MAX) {
				shell_error(sh, "Invalid set_size: %lu",
					    set_size);

				return -ENOEXEC;
			}

			param.set_size = set_size;
		} else if (strcmp(arg, "rank") == 0) {
			unsigned long rank;

			argn++;
			if (argn == argc) {
				shell_help(sh);
				return SHELL_CMD_HELP_PRINTED;
			}

			rank = shell_strtoul(argv[argn], 0, &err);
			if (err != 0) {
				shell_error(sh, "Could not parse rank: %d",
					    err);

				return -ENOEXEC;
			}

			if (rank > UINT8_MAX) {
				shell_error(sh, "Invalid rank: %lu", rank);

				return -ENOEXEC;
			}

			param.rank = rank;
		} else if (strcmp(arg, "not-lockable") == 0) {
			param.lockable = false;
		} else if (strcmp(arg, "sirk") == 0) {
			size_t len;

			argn++;
			if (argn == argc) {
				shell_help(sh);
				return SHELL_CMD_HELP_PRINTED;
			}

			len = hex2bin(argv[argn], strlen(argv[argn]), param.sirk,
				      sizeof(param.sirk));
			if (len == 0) {
				shell_error(sh, "Could not parse SIRK");
				return -ENOEXEC;
			}
		} else {
			shell_help(sh);
			return SHELL_CMD_HELP_PRINTED;
		}
	}

	err = bt_csip_set_member_register(&param, &svc_inst);
	if (err != 0) {
		shell_error(sh, "Could not register CSIP: %d", err);
		return err;
	}

	return 0;
}

static int cmd_csip_set_member_sirk(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t sirk[BT_CSIP_SIRK_SIZE];
	size_t len;
	int err;

	if (svc_inst == NULL) {
		shell_error(sh, "CSIS not registered yet");

		return -ENOEXEC;
	}

	len = hex2bin(argv[1], strlen(argv[1]), sirk, sizeof(sirk));
	if (len != sizeof(sirk)) {
		shell_error(sh, "Invalid SIRK Length: %zu", len);

		return -ENOEXEC;
	}

	err = bt_csip_set_member_sirk(svc_inst, sirk);
	if (err != 0) {
		shell_error(sh, "Failed to set SIRK: %d", err);
		return -ENOEXEC;
	}

	shell_print(sh, "SIRK updated");

	return 0;
}

static int cmd_csip_set_member_get_sirk(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t sirk[BT_CSIP_SIRK_SIZE];
	int err;

	if (svc_inst == NULL) {
		shell_error(sh, "CSIS not registered yet");

		return -ENOEXEC;
	}

	err = bt_csip_set_member_get_sirk(svc_inst, sirk);
	if (err != 0) {
		shell_error(sh, "Failed to get SIRK: %d", err);
		return -ENOEXEC;
	}

	shell_print(sh, "SIRK");
	shell_hexdump(sh, sirk, sizeof(sirk));

	return 0;
}

static int cmd_csip_set_member_lock(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	err = bt_csip_set_member_lock(svc_inst, true, false);
	if (err != 0) {
		shell_error(sh, "Failed to set lock: %d", err);
		return -ENOEXEC;
	}

	shell_print(sh, "Set locked");

	return 0;
}

static int cmd_csip_set_member_release(const struct shell *sh, size_t argc,
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

	err = bt_csip_set_member_lock(svc_inst, false, force);

	if (err != 0) {
		shell_error(sh, "Failed to release lock: %d", err);
		return -ENOEXEC;
	}

	shell_print(sh, "Set released");

	return 0;
}

static int cmd_csip_set_member_sirk_rsp(const struct shell *sh, size_t argc, char *argv[])
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

static int cmd_csip_set_member(const struct shell *sh, size_t argc, char **argv)
{
	shell_error(sh, "%s unknown parameter: %s", argv[0], argv[1]);

	return -ENOEXEC;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	csip_set_member_cmds,
	SHELL_CMD_ARG(register, NULL,
		      "Initialize the service and register callbacks "
		      "[size <int>] [rank <int>] [not-lockable] [sirk <data>]",
		      cmd_csip_set_member_register, 1, 4),
	SHELL_CMD_ARG(lock, NULL, "Lock the set", cmd_csip_set_member_lock, 1, 0),
	SHELL_CMD_ARG(release, NULL, "Release the set [force]", cmd_csip_set_member_release, 1, 1),
	SHELL_CMD_ARG(sirk, NULL, "Set the currently used SIRK <sirk>", cmd_csip_set_member_sirk, 2,
		      0),
	SHELL_CMD_ARG(get_sirk, NULL, "Get the currently used SIRK", cmd_csip_set_member_get_sirk,
		      1, 0),
	SHELL_CMD_ARG(sirk_rsp, NULL,
		      "Set the response used in SIRK requests "
		      "<accept, accept_enc, reject, oob>",
		      cmd_csip_set_member_sirk_rsp, 2, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_ARG_REGISTER(csip_set_member, &csip_set_member_cmds,
		       "Bluetooth CSIP set member shell commands",
		       cmd_csip_set_member, 1, 1);

ssize_t csis_ad_data_add(struct bt_data *data_array, const size_t data_array_size,
			 const bool discoverable)
{
	size_t ad_len = 0;

	/* Advertise RSI in discoverable mode only */
	if (svc_inst != NULL && discoverable) {
		static uint8_t ad_rsi[BT_CSIP_RSI_SIZE];
		int err;

		/* A privacy-enabled Set Member should only advertise RSI values derived
		 * from a SIRK that is exposed in encrypted form.
		 */
		if (IS_ENABLED(CONFIG_BT_PRIVACY) &&
		    !IS_ENABLED(CONFIG_BT_CSIP_SET_MEMBER_ENC_SIRK_SUPPORT)) {
			bt_shell_warn("RSI derived from unencrypted SIRK");
		}

		err = bt_csip_set_member_generate_rsi(svc_inst, ad_rsi);
		if (err != 0) {
			bt_shell_error("Failed to generate RSI (err %d)", err);
			return err;
		}

		__ASSERT(data_array_size > ad_len, "No space for AD_RSI");
		data_array[ad_len].type = BT_DATA_CSIS_RSI;
		data_array[ad_len].data_len = ARRAY_SIZE(ad_rsi);
		data_array[ad_len].data = &ad_rsi[0];
		ad_len++;
	}

	return ad_len;
}
