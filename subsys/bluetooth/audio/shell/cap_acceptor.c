/**
 * @file
 * @brief Shell APIs for Bluetooth CAP acceptor
 *
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/csip.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/shell/shell_string_conv.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/types.h>
#include <zephyr/shell/shell.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/audio/cap.h>
#include "shell/bt.h"

static size_t ad_cap_announcement_data_add(struct bt_data data[], size_t data_size)
{
	static const uint8_t ad_cap_announcement[3] = {
		BT_UUID_16_ENCODE(BT_UUID_CAS_VAL),
		BT_AUDIO_UNICAST_ANNOUNCEMENT_TARGETED,
	};

	__ASSERT(data_size > 0, "No space for AD_CAP_ANNOUNCEMENT");
	data[0].type = BT_DATA_SVC_DATA16;
	data[0].data_len = ARRAY_SIZE(ad_cap_announcement);
	data[0].data = &ad_cap_announcement[0];

	return 1U;
}

#if defined(CONFIG_BT_CAP_ACCEPTOR_SET_MEMBER)
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
	struct bt_csip_set_member_register_param param = {
		.set_size = 2,
		.rank = 1,
		.lockable = true,
		/* Using the CSIS test sample SIRK */
		.sirk = { 0xcd, 0xcc, 0x72, 0xdd, 0x86, 0x8c, 0xcd, 0xce,
			  0x22, 0xfd, 0xa1, 0x21, 0x09, 0x7d, 0x7d, 0x45 },
		.cb = &csip_set_member_cbs,
	};
	int err = 0;

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

	err = bt_cap_acceptor_register(&param, &cap_csip_svc_inst);
	if (err != 0) {
		shell_error(sh, "Could not register CAS: %d", err);

		return err;
	}

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

static int cmd_cap_acceptor_sirk(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t sirk[BT_CSIP_SIRK_SIZE];
	size_t len;
	int err;

	if (cap_csip_svc_inst == NULL) {
		shell_error(sh, "CSIS not registered");

		return -ENOEXEC;
	}

	len = hex2bin(argv[1], strlen(argv[1]), sirk, sizeof(sirk));
	if (len != sizeof(sirk)) {
		shell_error(sh, "Invalid SIRK Length: %zu", len);

		return -ENOEXEC;
	}

	err = bt_csip_set_member_sirk(cap_csip_svc_inst, sirk);
	if (err != 0) {
		shell_error(sh, "Failed to set SIRK: %d", err);
		return -ENOEXEC;
	}

	shell_print(sh, "SIRK updated");

	return 0;
}

static int cmd_cap_acceptor_get_sirk(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t sirk[BT_CSIP_SIRK_SIZE];
	int err;

	if (cap_csip_svc_inst == NULL) {
		shell_error(sh, "CSIS not registered");

		return -ENOEXEC;
	}

	err = bt_csip_set_member_get_sirk(cap_csip_svc_inst, sirk);
	if (err != 0) {
		shell_error(sh, "Failed to get SIRK: %d", err);
		return -ENOEXEC;
	}

	shell_print(sh, "SIRK");
	shell_hexdump(sh, sirk, sizeof(sirk));

	return 0;
}

static int cmd_cap_acceptor_sirk_rsp(const struct shell *sh, size_t argc, char *argv[])
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

SHELL_STATIC_SUBCMD_SET_CREATE(
	cap_acceptor_cmds,
	SHELL_CMD_ARG(init, NULL,
		      "Initialize the service and register callbacks "
		      "[size <int>] [rank <int>] [not-lockable] [sirk <data>]",
		      cmd_cap_acceptor_init, 1, 4),
	SHELL_CMD_ARG(lock, NULL, "Lock the set", cmd_cap_acceptor_lock, 1, 0),
	SHELL_CMD_ARG(release, NULL, "Release the set [force]", cmd_cap_acceptor_release, 1, 1),
	SHELL_CMD_ARG(sirk, NULL, "Set the currently used SIRK <sirk>", cmd_cap_acceptor_sirk, 2,
		      0),
	SHELL_CMD_ARG(get_sirk, NULL, "Get the currently used SIRK", cmd_cap_acceptor_get_sirk, 1,
		      0),
	SHELL_CMD_ARG(sirk_rsp, NULL,
		      "Set the response used in SIRK requests "
		      "<accept, accept_enc, reject, oob>",
		      cmd_cap_acceptor_sirk_rsp, 2, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_ARG_REGISTER(cap_acceptor, &cap_acceptor_cmds, "Bluetooth CAP acceptor shell commands",
		       cmd_cap_acceptor, 1, 1);

size_t cap_acceptor_ad_data_add(struct bt_data data[], size_t data_size, bool discoverable)
{
	size_t ad_len = 0;

	if (!discoverable) {
		return ad_len;
	}

	if (IS_ENABLED(CONFIG_BT_CAP_ACCEPTOR_SET_MEMBER) && cap_csip_svc_inst != NULL) {
		static uint8_t ad_rsi[BT_CSIP_RSI_SIZE];
		int err;

		ad_len += ad_cap_announcement_data_add(data, data_size);

		/* A privacy-enabled Set Member should only advertise RSI values derived
		 * from a SIRK that is exposed in encrypted form.
		 */
		if (IS_ENABLED(CONFIG_BT_PRIVACY) &&
		    !IS_ENABLED(CONFIG_BT_CSIP_SET_MEMBER_ENC_SIRK_SUPPORT)) {
			shell_warn(ctx_shell, "RSI derived from unencrypted SIRK");
		}

		err = bt_csip_set_member_generate_rsi(cap_csip_svc_inst, ad_rsi);
		if (err != 0) {
			shell_error(ctx_shell, "Failed to generate RSI (err %d)", err);

			return err;
		}

		__ASSERT(data_size > ad_len, "No space for AD_RSI");
		data[ad_len].type = BT_DATA_CSIS_RSI;
		data[ad_len].data_len = ARRAY_SIZE(ad_rsi);
		data[ad_len].data = &ad_rsi[0];
		ad_len++;
	}

	return ad_len;
}

#else /* !CONFIG_BT_CAP_ACCEPTOR_SET_MEMBER */

size_t cap_acceptor_ad_data_add(struct bt_data data[], size_t data_size, bool discoverable)
{
	if (!discoverable) {
		return 0U;
	}

	return ad_cap_announcement_data_add(data, data_size);
}

#endif /* CONFIG_BT_CAP_ACCEPTOR_SET_MEMBER */
