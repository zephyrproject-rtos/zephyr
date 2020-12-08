/**
 * @file
 * @brief Shell APIs for Bluetooth CSIP
 *
 * Copyright (c) 2020 Bose Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <bluetooth/conn.h>

#include <zephyr.h>
#include <zephyr/types.h>
#include <shell/shell.h>
#include <stdlib.h>
#include <bluetooth/gatt.h>
#include <bluetooth/bluetooth.h>

#include "bt.h"

#include "../host/audio/csip.h"

static void csis_discover_cb(struct bt_conn *conn, int err, uint8_t set_count)
{
	if (err) {
		shell_error(ctx_shell, "discover failed (%d)", err);
		return;
	}

	shell_print(ctx_shell, "Found %u sets on device", set_count);
}

static void csip_discover_sets_cb(struct bt_conn *conn, int err,
				  uint8_t set_count, struct bt_csip_set_t *sets)
{
	if (err) {
		shell_error(ctx_shell, "Discover sets failed (%d)", err);
		return;
	}

	for (int i = 0; i < set_count; i++) {
		shell_print(ctx_shell, "Set size %d (pointer: %p)\n",
			    sets[i].set_size, &sets[i]);
	}
}

static void bt_csip_discover_members_cb(int err, uint8_t set_size,
					uint8_t members_found)
{
	if (err) {
		shell_error(ctx_shell, "Discover members failed (%d)", err);
		return;
	}

	shell_print(ctx_shell, "Discovered %u/%u set members\n", members_found,
		    set_size);
}

static void csip_lock_set_cb(int err)
{
	if (err) {
		shell_error(ctx_shell, "Lock sets failed (%d)", err);
		return;
	}

	shell_print(ctx_shell, "Set locked\n");
}

static void csip_release_set_cb(int err)
{
	if (err) {
		shell_error(ctx_shell, "Lock sets failed (%d)", err);
		return;
	}

	shell_print(ctx_shell, "Set released\n");
}

static struct bt_csip_cb_t cbs = {
	.lock_set = csip_lock_set_cb,
	.release_set = csip_release_set_cb,
	.members = bt_csip_discover_members_cb,
	.sets = csip_discover_sets_cb,
	.discover = csis_discover_cb,
};

static int cmd_csip_discover(const struct shell *shell, size_t argc,
				    char *argv[])
{
	int result;
	int subscribe = 1;

	if (!ctx_shell) {
		ctx_shell = shell;
	}

	bt_csip_register_cb(&cbs);

	if (argc > 1) {
		subscribe = strtol(argv[1], NULL, 0);

		if (subscribe < 0 || subscribe > 1) {
			shell_error(shell, "Invalid parameter");
			return -ENOEXEC;
		}
	}

	result = bt_csip_discover(default_conn, (bool)subscribe);
	if (result) {
		shell_error(shell, "Fail: %d", result);
	}
	return result;
}

static int cmd_csip_discover_sets(const struct shell *shell, size_t argc,
					 char *argv[])
{
	int result;

	result = bt_csip_discover_sets(default_conn);
	if (result) {
		shell_error(shell, "Fail: %d", result);
	}
	return result;
}

static int cmd_csip_discover_members(const struct shell *shell,
					    size_t argc, char *argv[])
{
	int result;
	struct bt_csip_set_t *set = (struct bt_csip_set_t *)strtol(argv[1],
								   NULL, 0);

	result = bt_csip_discover_members(set);
	if (result) {
		shell_error(shell, "Fail: %d", result);
	}
	return result;
}

static int cmd_csip_lock_set(const struct shell *shell, size_t argc,
				    char *argv[])
{
	int result;

	result = bt_csip_lock_set();
	if (result) {
		shell_error(shell, "Fail: %d", result);
	}
	return result;
}

static int cmd_csip_release_set(const struct shell *shell, size_t argc,
				       char *argv[])
{
	int result;

	result = bt_csip_release_set();
	if (result) {
		shell_error(shell, "Fail: %d", result);
	}
	return result;
}

static int cmd_csip_disconnect(const struct shell *shell, size_t argc,
				      char *argv[])
{
	int result;

	result = bt_csip_disconnect();
	if (result) {
		shell_error(shell, "Fail: %d", result);
	}
	return result;
}

static int cmd_csip(const struct shell *shell, size_t argc, char **argv)
{
	if (argc > 1) {
		shell_error(shell, "%s unknown parameter: %s",
			    argv[0], argv[1]);
	} else {
		shell_error(shell, "%s Missing subcommand", argv[0]);
	}

	return -ENOEXEC;
}


SHELL_STATIC_SUBCMD_SET_CREATE(csip_cmds,
	SHELL_CMD_ARG(init, NULL,
		      "Run discover for CSIS on peer device [subscribe]",
		      cmd_csip_discover, 1, 1),
	SHELL_CMD_ARG(discover_sets, NULL,
		      "Read all set values on connected device",
		      cmd_csip_discover_sets, 1, 0),
	SHELL_CMD_ARG(discover_members, NULL,
		      "Scan for set members <set_pointer>",
		      cmd_csip_discover_members, 2, 0),
	SHELL_CMD_ARG(lock_set, NULL,
		      "Lock set",
		      cmd_csip_lock_set, 1, 0),
	SHELL_CMD_ARG(release_set, NULL,
		      "Release set",
		      cmd_csip_release_set, 1, 0),
	SHELL_CMD_ARG(disconnect, NULL,
		      "Release set",
		      cmd_csip_disconnect, 1, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_ARG_REGISTER(csip, &csip_cmds, "Bluetooth CSIP shell commands",
		       cmd_csip, 1, 1);
