/**
 * @file
 * @brief Shell APIs for Bluetooth CAP initiator
 *
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdlib.h>

#include <zephyr/types.h>
#include <zephyr/shell/shell.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/audio/cap.h>

#include "bt.h"

#if defined(CONFIG_BT_AUDIO_UNICAST_CLIENT)

static void cap_discover_cb(struct bt_conn *conn, int err,
			    const struct bt_csip_set_coordinator_csis_inst *csis_inst)
{
	if (err != 0) {
		shell_error(ctx_shell, "discover failed (%d)", err);
		return;
	}

	shell_print(ctx_shell, "discovery completed%s",
		    csis_inst == NULL ? "" : " with CSIS");
}

static struct bt_cap_initiator_cb cbs = {
	.unicast_discovery_complete = cap_discover_cb,
};

static int cmd_cap_initiator_discover(const struct shell *sh, size_t argc,
				    char *argv[])
{
	static bool cbs_registered;
	int err;

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	if (ctx_shell == NULL) {
		ctx_shell = sh;
	}

	if (!cbs_registered) {
		bt_cap_initiator_register_cb(&cbs);
		cbs_registered = true;
	}

	err = bt_cap_initiator_unicast_discover(default_conn);
	if (err != 0) {
		shell_error(sh, "Fail: %d", err);
	}

	return err;
}

#endif /* CONFIG_BT_AUDIO_UNICAST_CLIENT */

static int cmd_cap_initiator(const struct shell *sh, size_t argc, char **argv)
{
	if (argc > 1) {
		shell_error(sh, "%s unknown parameter: %s",
			    argv[0], argv[1]);
	} else {
		shell_error(sh, "%s Missing subcommand", argv[0]);
	}

	return -ENOEXEC;
}

SHELL_STATIC_SUBCMD_SET_CREATE(cap_initiator_cmds,
#if defined(CONFIG_BT_AUDIO_UNICAST_CLIENT)
	SHELL_CMD_ARG(discover, NULL,
		      "Discover CAS", cmd_cap_initiator_discover, 1, 0),
#endif /* CONFIG_BT_AUDIO_UNICAST_CLIENT */
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_ARG_REGISTER(cap_initiator, &cap_initiator_cmds,
		       "Bluetooth CAP initiator shell commands",
		       cmd_cap_initiator, 1, 1);
