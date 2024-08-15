/** @file
 *  @brief Bluetooth Call Control Profile Call Control Client shell
 */

/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <errno.h>
#include <stdint.h>
#include <stdio.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/audio/tbs.h>
#include <zephyr/bluetooth/audio/ccp.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/shell/shell.h>

#include "common/bt_shell_private.h"
#include "host/shell/bt.h"

static struct bt_ccp_call_control_client *clients[CONFIG_BT_MAX_CONN];

static void ccp_call_control_client_discover_cb(struct bt_ccp_call_control_client *client, int err,
						struct bt_ccp_call_control_client_bearers *bearers)
{
	struct bt_ccp_call_control_client_bearer *gtbs_bearer = NULL;
	uint8_t tbs_count = 0U;

	if (err != 0) {
		bt_shell_error("Failed to discover TBS: %d", err);
		return;
	}
#if defined(CONFIG_BT_TBS_CLIENT_GTBS)
	gtbs_bearer = bearers->gtbs_bearer;
#endif /* CONFIG_BT_TBS_CLIENT_GTBS */
#if defined(CONFIG_BT_TBS_CLIENT_TBS)
	tbs_count = bearers->tbs_count;
#endif /* CONFIG_BT_TBS_CLIENT_TBS */

	bt_shell_info("Discovery completed with %s%u TBS bearers",
		      gtbs_bearer != NULL ? "GTBS and " : "", tbs_count);
}

static int cmd_ccp_call_control_client_discover(const struct shell *sh, size_t argc, char *argv[])
{
	static bool cb_registered;

	int err;

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	if (!cb_registered) {
		static struct bt_ccp_call_control_client_cb ccp_call_control_client_cbs = {
			.discover = ccp_call_control_client_discover_cb,
		};

		err = bt_ccp_call_control_client_register_cb(&ccp_call_control_client_cbs);
		if (err != 0) {
			shell_error(sh, "Failed to register CCP Call Control Client cbs (err %d)",
				    err);
			return -ENOEXEC;
		}
	}

	err = bt_ccp_call_control_client_discover(default_conn,
						  &clients[bt_conn_index(default_conn)]);
	if (err != 0) {
		shell_error(sh, "Failed to discover GTBS: %d", err);

		return -ENOEXEC;
	}

	return 0;
}

static int cmd_ccp_call_control_client(const struct shell *sh, size_t argc, char **argv)
{
	if (argc > 1) {
		shell_error(sh, "%s unknown parameter: %s", argv[0], argv[1]);
	} else {
		shell_error(sh, "%s Missing subcommand", argv[0]);
	}

	return -ENOEXEC;
}

SHELL_STATIC_SUBCMD_SET_CREATE(ccp_call_control_client_cmds,
			       SHELL_CMD_ARG(discover, NULL,
					     "Discover GTBS and TBS on remote device",
					     cmd_ccp_call_control_client_discover, 1, 0),
			       SHELL_SUBCMD_SET_END);

SHELL_CMD_ARG_REGISTER(ccp_call_control_client, &ccp_call_control_client_cmds,
		       "Bluetooth CCP Call Control Client shell commands",
		       cmd_ccp_call_control_client, 1, 1);
