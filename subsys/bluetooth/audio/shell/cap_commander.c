/**
 * @file
 * @brief Shell APIs for Bluetooth CAP commander
 *
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdlib.h>

#include <zephyr/types.h>
#include <zephyr/shell/shell.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/audio/cap.h>

#include "shell/bt.h"
#include "audio.h"

static void cap_discover_cb(struct bt_conn *conn, int err,
			    const struct bt_csip_set_coordinator_csis_inst *csis_inst)
{
	if (err != 0) {
		shell_error(ctx_shell, "discover failed (%d)", err);
		return;
	}

	shell_print(ctx_shell, "discovery completed%s", csis_inst == NULL ? "" : " with CSIS");
}

#if defined(CONFIG_BT_VCP_VOL_CTLR)
static void cap_volume_changed_cb(struct bt_conn *conn, int err)
{
	if (err != 0) {
		shell_error(ctx_shell, "Volume change failed (%d)", err);
		return;
	}

	shell_print(ctx_shell, "Volume change completed");
}
#endif /* CONFIG_BT_VCP_VOL_CTLR */

static struct bt_cap_commander_cb cbs = {
	.discovery_complete = cap_discover_cb,
#if defined(CONFIG_BT_VCP_VOL_CTLR)
	.volume_changed = cap_volume_changed_cb,
#endif /* CONFIG_BT_VCP_VOL_CTLR */
};

static int cmd_cap_commander_discover(const struct shell *sh, size_t argc, char *argv[])
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
		bt_cap_commander_register_cb(&cbs);
		cbs_registered = true;
	}

	err = bt_cap_commander_discover(default_conn);
	if (err != 0) {
		shell_error(sh, "Fail: %d", err);
	}

	return err;
}

#if defined(CONFIG_BT_VCP_VOL_CTLR)
static void populate_connected_conns(struct bt_conn *conn, void *data)
{
	struct bt_conn **connected_conns = (struct bt_conn **)data;

	for (int i = 0; i < CONFIG_BT_MAX_CONN; i++) {
		if (connected_conns[i] == NULL) {
			connected_conns[i] = conn;
			return;
		}
	}
}

static int cmd_cap_commander_change_volume(const struct shell *sh, size_t argc, char *argv[])
{
	struct bt_conn *connected_conns[CONFIG_BT_MAX_CONN] = {0};
	union bt_cap_set_member members[CONFIG_BT_MAX_CONN] = {0};
	struct bt_cap_commander_change_volume_param param = {
		.members = members,
	};
	unsigned long volume;
	int err = 0;

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	volume = shell_strtoul(argv[1], 10, &err);
	if (err != 0) {
		shell_error(sh, "Failed to parse volume from %s", argv[1]);

		return -ENOEXEC;
	}

	if (volume > UINT8_MAX) {
		shell_error(sh, "Invalid volume %lu", volume);

		return -ENOEXEC;
	}
	param.volume = (uint8_t)volume;

	param.type = BT_CAP_SET_TYPE_AD_HOC;
	/* TODO: Add support for coordinated sets */

	/* Populate the array of connected connections */
	bt_conn_foreach(BT_CONN_TYPE_LE, populate_connected_conns, (void *)connected_conns);

	param.count = 0U;
	param.members = members;
	for (size_t i = 0; i < ARRAY_SIZE(connected_conns); i++) {
		struct bt_conn *conn = connected_conns[i];

		if (conn == NULL) {
			break;
		}

		param.members[i].member = conn;
		param.count++;
	}

	shell_print(sh, "Setting volume to %u on %zu connections", param.volume, param.count);

	err = bt_cap_commander_change_volume(&param);
	if (err != 0) {
		shell_print(sh, "Failed to change volume: %d", err);

		return -ENOEXEC;
	}

	return 0;
}
#endif /* CONFIG_BT_VCP_VOL_CTLR */

static int cmd_cap_commander(const struct shell *sh, size_t argc, char **argv)
{
	if (argc > 1) {
		shell_error(sh, "%s unknown parameter: %s", argv[0], argv[1]);
	} else {
		shell_error(sh, "%s Missing subcommand", argv[0]);
	}

	return -ENOEXEC;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	cap_commander_cmds,
	SHELL_CMD_ARG(discover, NULL, "Discover CAS", cmd_cap_commander_discover, 1, 0),
#if defined(CONFIG_BT_VCP_VOL_CTLR)
	SHELL_CMD_ARG(change_volume, NULL, "Change volume on all connections <volume>",
		      cmd_cap_commander_change_volume, 2, 0),
#endif /* CONFIG_BT_VCP_VOL_CTLR */
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_ARG_REGISTER(cap_commander, &cap_commander_cmds, "Bluetooth CAP commander shell commands",
		       cmd_cap_commander, 1, 1);
