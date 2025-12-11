/** @file
 *  @brief Bluetooth Call Control Profile Call Control Client shell
 */

/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/audio/tbs.h>
#include <zephyr/bluetooth/audio/ccp.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_string_conv.h>
#include <zephyr/sys/__assert.h>

#include "common/bt_shell_private.h"
#include "host/shell/bt.h"

static struct bt_ccp_call_control_client *clients[CONFIG_BT_MAX_CONN];

static struct bt_ccp_call_control_client *get_client_by_conn(const struct bt_conn *conn)
{
	return clients[bt_conn_index(conn)];
}

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

	bt_shell_info("Discovery completed for client %p with %s%u TBS bearers", client,
		      gtbs_bearer != NULL ? "GTBS and " : "", tbs_count);
}

#if defined(CONFIG_BT_TBS_CLIENT_BEARER_PROVIDER_NAME)
static void
ccp_call_control_client_bearer_provider_name_cb(struct bt_ccp_call_control_client_bearer *bearer,
						int err, const char *name)
{
	if (err != 0) {
		bt_shell_error("Failed to read bearer %p name: %d", (void *)bearer, err);
		return;
	}

	bt_shell_info("Bearer %p name: %s", (void *)bearer, name);
}
#endif /* CONFIG_BT_TBS_CLIENT_BEARER_PROVIDER_NAME */

static struct bt_ccp_call_control_client_cb ccp_call_control_client_cbs = {
	.discover = ccp_call_control_client_discover_cb,
#if defined(CONFIG_BT_TBS_CLIENT_BEARER_PROVIDER_NAME)
	.bearer_provider_name = ccp_call_control_client_bearer_provider_name_cb,
#endif /* CONFIG_BT_TBS_CLIENT_BEARER_PROVIDER_NAME */
};

static int cmd_ccp_call_control_client_discover(const struct shell *sh, size_t argc, char *argv[])
{
	static bool cb_registered;
	int err;

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	if (!cb_registered) {
		err = bt_ccp_call_control_client_register_cb(&ccp_call_control_client_cbs);
		if (err != 0) {
			shell_error(sh, "Failed to register CCP Call Control Client cbs (err %d)",
				    err);
			return -ENOEXEC;
		}

		cb_registered = true;
	}

	err = bt_ccp_call_control_client_discover(default_conn,
						  &clients[bt_conn_index(default_conn)]);
	if (err != 0) {
		shell_error(sh, "Failed to discover GTBS: %d", err);

		return -ENOEXEC;
	}

	return 0;
}

static int validate_and_get_index(const struct shell *sh, const char *index_arg)
{
	unsigned long index;
	int err = 0;

	index = shell_strtoul(index_arg, 0, &err);
	if (err != 0) {
		shell_error(sh, "Could not parse index: %d", err);

		return -ENOEXEC;
	}

	if (index >= CONFIG_BT_CCP_CALL_CONTROL_CLIENT_BEARER_COUNT) {
		shell_error(sh, "Invalid index: %lu", index);

		return -ENOEXEC;
	}

	return (int)index;
}

static struct bt_ccp_call_control_client_bearer *get_bearer_by_index(uint8_t index)
{
	struct bt_ccp_call_control_client_bearers bearers;
	struct bt_ccp_call_control_client *client;
	int err;

	client = get_client_by_conn(default_conn);
	if (client == NULL) {
		return NULL;
	}

	err = bt_ccp_call_control_client_get_bearers(client, &bearers);
	__ASSERT_NO_MSG(err == 0);

#if defined(CONFIG_BT_TBS_CLIENT_GTBS) && defined(CONFIG_BT_TBS_CLIENT_TBS)
	/* If GTBS is enabled then it is at index 0. If the index is not 0, then we decrement it so
	 * that it can be used as a direct index to the TBS bearers (where index 0 is a TBS inst)
	 */
	if (index == 0) {
		return bearers.gtbs_bearer;
	}
	index--;
#elif defined(CONFIG_BT_TBS_CLIENT_GTBS)
	return bearers.gtbs_bearer;
#endif

#if defined(CONFIG_BT_TBS_CLIENT_TBS)
	return bearers.tbs_bearers[index];
#endif /* CONFIG_BT_TBS_CLIENT_GTBS */
}

static int cmd_ccp_call_control_client_read_bearer_name(const struct shell *sh, size_t argc,
							char *argv[])
{
	struct bt_ccp_call_control_client_bearer *bearer;
	int index = 0;
	int err;

	if (argc > 1) {
		index = validate_and_get_index(sh, argv[1]);
		if (index < 0) {
			return index;
		}
	}

	bearer = get_bearer_by_index(index);
	if (bearer == NULL) {
		shell_error(sh, "Failed to get bearer for index %d", index);

		return -ENOEXEC;
	}

	err = bt_ccp_call_control_client_read_bearer_provider_name(bearer);
	if (err != 0) {
		shell_error(sh, "Failed to read bearer[%d] provider name: %d", index, err);

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
			       SHELL_CMD_ARG(read_bearer_name, NULL, "Get bearer name [index]",
					     cmd_ccp_call_control_client_read_bearer_name, 1, 1),
			       SHELL_SUBCMD_SET_END);

SHELL_CMD_ARG_REGISTER(ccp_call_control_client, &ccp_call_control_client_cmds,
		       "Bluetooth CCP Call Control Client shell commands",
		       cmd_ccp_call_control_client, 1, 1);
