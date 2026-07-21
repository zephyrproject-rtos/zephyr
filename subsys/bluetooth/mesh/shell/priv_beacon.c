/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <zephyr/shell/shell.h>
#include <zephyr/bluetooth/mesh.h>
#include <zephyr/bluetooth/mesh/shell.h>

#include "utils.h"

static int cmd_priv_beacon_get(const struct shell *sh, size_t argc, char *argv[])
{
	struct bt_mesh_priv_beacon val;
	int err;

	err = bt_mesh_priv_beacon_cli_get(bt_mesh_shell_target_ctx.net_idx,
					  bt_mesh_shell_target_ctx.dst,
					  &val);
	if (err) {
		shell_error(sh, "Failed to send Private Beacon Get (err %d)", err);
		return 0;
	}

	shell_print(sh, "Private Beacon state: %u, %u", val.enabled, val.rand_interval);

	return 0;
}

static int cmd_priv_beacon_set(const struct shell *sh, size_t argc, char *argv[])
{
	struct bt_mesh_priv_beacon val;
	int err = 0;

	val.enabled = shell_strtobool(argv[1], 0, &err);
	if (err) {
		shell_warn(sh, "Unable to parse input string argument");
		return err;
	}

	val.rand_interval = shell_strtoul(argv[2], 0, &err);
	if (err) {
		shell_warn(sh, "Unable to parse input string argument");
		return err;
	}

	err = bt_mesh_priv_beacon_cli_set(bt_mesh_shell_target_ctx.net_idx,
					  bt_mesh_shell_target_ctx.dst,
					  &val, &val);
	if (err) {
		shell_error(sh, "Failed to send Private Beacon Set (err %d)", err);
		return 0;
	}

	return 0;
}

static int cmd_priv_gatt_proxy_get(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t state;
	int err;

	err = bt_mesh_priv_beacon_cli_gatt_proxy_get(bt_mesh_shell_target_ctx.net_idx,
						     bt_mesh_shell_target_ctx.dst, &state);
	if (err) {
		shell_error(sh, "Failed to send Private GATT Proxy Get (err %d)", err);
		return 0;
	}

	shell_print(sh, "Private GATT Proxy state: %u", state);

	return 0;
}

static int cmd_priv_gatt_proxy_set(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t state;
	int err = 0;

	state = shell_strtobool(argv[1], 0, &err);
	if (err) {
		shell_warn(sh, "Unable to parse input string argument");
		return err;
	}

	err = bt_mesh_priv_beacon_cli_gatt_proxy_set(bt_mesh_shell_target_ctx.net_idx,
						     bt_mesh_shell_target_ctx.dst, state, &state);
	if (err) {
		shell_error(sh, "Failed to send Private GATT Proxy Set (err %d)", err);
		return 0;
	}

	return 0;
}

static int cmd_priv_node_id_get(const struct shell *sh, size_t argc, char *argv[])
{
	struct bt_mesh_priv_node_id val;
	uint16_t key_net_idx;
	int err = 0;

	key_net_idx = shell_strtoul(argv[1], 0, &err);

	err = bt_mesh_priv_beacon_cli_node_id_get(bt_mesh_shell_target_ctx.net_idx,
						  bt_mesh_shell_target_ctx.dst, key_net_idx, &val);
	if (err) {
		shell_error(sh, "Failed to send Private Node Identity Get (err %d)", err);
		return 0;
	}

	shell_print(sh, "Private Node Identity state: (net_idx: %u, state: %u, status: %u)",
		    val.net_idx, val.state, val.status);

	return 0;
}

static int cmd_priv_node_id_set(const struct shell *sh, size_t argc, char *argv[])
{
	struct bt_mesh_priv_node_id val;
	int err = 0;

	val.net_idx = shell_strtoul(argv[1], 0, &err);
	val.state = shell_strtoul(argv[2], 0, &err);

	if (err) {
		shell_warn(sh, "Unable to parse input string argument");
		return err;
	}

	err = bt_mesh_priv_beacon_cli_node_id_set(bt_mesh_shell_target_ctx.net_idx,
						  bt_mesh_shell_target_ctx.dst, &val, &val);
	if (err) {
		shell_error(sh, "Failed to send Private Node Identity Set (err %d)", err);
		return 0;
	}

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	priv_beacons_cmds,
	SHELL_CMD_ARG(priv-beacon-get, NULL, NULL, cmd_priv_beacon_get, 1, 0),
	SHELL_CMD_ARG(priv-beacon-set, NULL, "<Val(off, on)> <RandInt(10s steps)>",
		      cmd_priv_beacon_set, 3, 0),
	SHELL_CMD_ARG(priv-gatt-proxy-get, NULL, NULL, cmd_priv_gatt_proxy_get, 1, 0),
	SHELL_CMD_ARG(priv-gatt-proxy-set, NULL, "Val(off, on)> ", cmd_priv_gatt_proxy_set,
		      2, 0),
	SHELL_CMD_ARG(priv-node-id-get, NULL, "<NetKeyIdx>", cmd_priv_node_id_get, 2, 0),
	SHELL_CMD_ARG(priv-node-id-set, NULL, "<NetKeyIdx> <State>", cmd_priv_node_id_set, 3,
		      0),
	SHELL_SUBCMD_SET_END);

SHELL_SUBCMD_ADD((mesh, models), prb, &priv_beacons_cmds, "Private Beacon Cli commands",
		 bt_mesh_shell_mdl_cmds_help, 1, 1);
