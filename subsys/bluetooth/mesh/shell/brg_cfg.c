/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <zephyr/shell/shell.h>
#include <zephyr/bluetooth/mesh.h>
#include <zephyr/bluetooth/mesh/shell.h>

#include "mesh/foundation.h"
#include "utils.h"

static int cmd_subnet_bridge_get(const struct shell *sh, size_t argc, char *argv[])
{
	enum bt_mesh_brg_cfg_state rsp;
	int err;

	err = bt_mesh_brg_cfg_cli_get(bt_mesh_shell_target_ctx.net_idx,
				      bt_mesh_shell_target_ctx.dst, &rsp);
	if (err) {
		shell_error(sh, "Failed to send Subnet Bridge Get (err %d)", err);
		return -ENOEXEC;
	}

	shell_print(sh, "Subnet Bridge State: %s",
		    (rsp == BT_MESH_BRG_CFG_ENABLED) ? "Enabled" : "Disabled");
	return 0;
}

static int cmd_subnet_bridge_set(const struct shell *sh, size_t argc, char *argv[])
{
	enum bt_mesh_brg_cfg_state set, rsp;
	int err = 0;

	set = shell_strtobool(argv[1], 0, &err) ? BT_MESH_BRG_CFG_ENABLED
						: BT_MESH_BRG_CFG_DISABLED;

	if (err) {
		shell_warn(sh, "Unable to parse input string argument");
		return err;
	}

	err = bt_mesh_brg_cfg_cli_set(bt_mesh_shell_target_ctx.net_idx,
				      bt_mesh_shell_target_ctx.dst, set, &rsp);
	if (err) {
		shell_error(sh, "Failed to send Subnet Bridge Set (err %d)", err);
		return -ENOEXEC;
	}

	shell_print(sh, "Subnet Bridge State: %s",
		    (rsp == BT_MESH_BRG_CFG_ENABLED) ? "Enabled" : "Disabled");
	return 0;
}

static int cmd_bridging_table_size_get(const struct shell *sh, size_t argc, char *argv[])
{
	uint16_t rsp;
	int err;

	err = bt_mesh_brg_cfg_cli_table_size_get(bt_mesh_shell_target_ctx.net_idx,
						 bt_mesh_shell_target_ctx.dst, &rsp);
	if (err) {
		shell_error(sh, "Failed to send Bridging Table Size Get (err %d)", err);
		return -ENOEXEC;
	}

	shell_print(sh, "Bridging Table Size: %u", rsp);
	return 0;
}

static int cmd_bridging_table_add(const struct shell *sh, size_t argc, char *argv[])
{
	struct bt_mesh_brg_cfg_table_entry entry;
	struct bt_mesh_brg_cfg_table_status rsp;
	int err = 0;

	entry.directions = shell_strtoul(argv[1], 0, &err);
	entry.net_idx1 = shell_strtoul(argv[2], 0, &err);
	entry.net_idx2 = shell_strtoul(argv[3], 0, &err);
	entry.addr1 = shell_strtoul(argv[4], 0, &err);
	entry.addr2 = shell_strtoul(argv[5], 0, &err);
	if (err) {
		shell_warn(sh, "Unable to parse input string argument");
		return err;
	}

	err = bt_mesh_brg_cfg_cli_table_add(bt_mesh_shell_target_ctx.net_idx,
					    bt_mesh_shell_target_ctx.dst, &entry, &rsp);
	if (err) {
		shell_error(sh, "Failed to send Bridging Table Add (err %d)", err);
		return -ENOEXEC;
	}

	if (rsp.status) {
		shell_print(sh, "Bridging Table Add failed with status 0x%02x", rsp.status);
	} else {
		shell_print(sh, "Bridging Table Add was successful.");
	}
	return 0;
}

static int cmd_bridging_table_remove(const struct shell *sh, size_t argc, char *argv[])
{
	uint16_t net_idx1, net_idx2, addr1, addr2;
	struct bt_mesh_brg_cfg_table_status rsp;
	int err = 0;

	net_idx1 = shell_strtoul(argv[1], 0, &err);
	net_idx2 = shell_strtoul(argv[2], 0, &err);
	addr1 = shell_strtoul(argv[3], 0, &err);
	addr2 = shell_strtoul(argv[4], 0, &err);
	if (err) {
		shell_warn(sh, "Unable to parse input string argument");
		return err;
	}

	err = bt_mesh_brg_cfg_cli_table_remove(bt_mesh_shell_target_ctx.net_idx,
					       bt_mesh_shell_target_ctx.dst, net_idx1, net_idx2,
					       addr1, addr2, &rsp);
	if (err) {
		shell_error(sh, "Failed to send Bridging Table Remove (err %d)", err);
		return -ENOEXEC;
	}

	if (rsp.status) {
		shell_print(sh, "Bridging Table Remove failed with status 0x%02x", rsp.status);
	} else {
		shell_print(sh, "Bridging Table Remove was successful.");
	}
	return 0;
}

static int cmd_bridged_subnets_get(const struct shell *sh, size_t argc, char *argv[])
{
	struct bt_mesh_brg_cfg_filter_netkey filter_net_idx;
	uint8_t start_idx;
	struct bt_mesh_brg_cfg_subnets_list rsp = {
		.list = NET_BUF_SIMPLE(CONFIG_BT_MESH_BRG_TABLE_ITEMS_MAX * 3),
	};
	int err = 0;

	net_buf_simple_init(rsp.list, 0);

	filter_net_idx.filter = shell_strtoul(argv[1], 0, &err);
	filter_net_idx.net_idx = shell_strtoul(argv[2], 0, &err);
	start_idx = shell_strtoul(argv[3], 0, &err);
	if (err) {
		shell_warn(sh, "Unable to parse input string argument");
		return err;
	}

	err = bt_mesh_brg_cfg_cli_subnets_get(bt_mesh_shell_target_ctx.net_idx,
					      bt_mesh_shell_target_ctx.dst, filter_net_idx,
					      start_idx, &rsp);
	if (err) {
		shell_error(sh, "Failed to send Bridged Subnets Get (err %d)", err);
		return -ENOEXEC;
	}

	shell_print(sh, "Bridged Subnets List:");
	shell_print(sh, "\tfilter: %02x", rsp.net_idx_filter.filter);
	shell_print(sh, "\tnet_idx: %04x", rsp.net_idx_filter.net_idx);
	shell_print(sh, "\tstart_idx: %u", rsp.start_idx);
	if (rsp.list) {
		uint16_t net_idx1, net_idx2;
		int i = 0;

		while (rsp.list->len) {
			key_idx_unpack_pair(rsp.list, &net_idx1, &net_idx2);
			shell_print(sh, "\tEntry %d:", i++);
			shell_print(sh, "\t\tnet_idx1: 0x%04x, net_idx2: 0x%04x", net_idx1,
				    net_idx2);
		}
	}
	return 0;
}

static int cmd_bridging_table_get(const struct shell *sh, size_t argc, char *argv[])
{
	uint16_t net_idx1, net_idx2, start_idx;
	struct bt_mesh_brg_cfg_table_list rsp = {
		.list = NET_BUF_SIMPLE(CONFIG_BT_MESH_BRG_TABLE_ITEMS_MAX * 5),
	};
	int err = 0;

	net_buf_simple_init(rsp.list, 0);

	net_idx1 = shell_strtoul(argv[1], 0, &err);
	net_idx2 = shell_strtoul(argv[2], 0, &err);
	start_idx = shell_strtoul(argv[3], 0, &err);
	if (err) {
		shell_warn(sh, "Unable to parse input string argument");
		return err;
	}

	err = bt_mesh_brg_cfg_cli_table_get(bt_mesh_shell_target_ctx.net_idx,
					    bt_mesh_shell_target_ctx.dst, net_idx1, net_idx2,
					    start_idx, &rsp);
	if (err) {
		shell_error(sh, "Failed to send Bridging Table Get (err %d)", err);
		return -ENOEXEC;
	}

	if (rsp.status) {
		shell_print(sh, "Bridging Table Get failed with status 0x%02x", rsp.status);
	} else {
		shell_print(sh, "Bridging Table List:");
		shell_print(sh, "\tstatus: %02x", rsp.status);
		shell_print(sh, "\tnet_idx1: %04x", rsp.net_idx1);
		shell_print(sh, "\tnet_idx2: %04x", rsp.net_idx2);
		shell_print(sh, "\tstart_idx: %u", rsp.start_idx);
		if (rsp.list) {
			uint16_t addr1, addr2;
			uint8_t directions;
			int i = 0;

			while (rsp.list->len) {
				addr1 = net_buf_simple_pull_le16(rsp.list);
				addr2 = net_buf_simple_pull_le16(rsp.list);
				directions = net_buf_simple_pull_u8(rsp.list);
				shell_print(sh, "\tEntry %d:", i++);
				shell_print(sh,
					    "\t\taddr1: 0x%04x, addr2: 0x%04x, directions: 0x%02x",
					    addr1, addr2, directions);
			}
		}
	}
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	brg_cfg_cmds, SHELL_CMD_ARG(bridge - get, NULL, NULL, cmd_subnet_bridge_get, 1, 0),
	SHELL_CMD_ARG(bridge - set, NULL, "<State(disable, enable)>", cmd_subnet_bridge_set, 2, 0),
	SHELL_CMD_ARG(table - size - get, NULL, NULL, cmd_bridging_table_size_get, 1, 0),
	SHELL_CMD_ARG(table - add, NULL, "<Directions> <NetIdx1> <NetIdx2> <Addr1> <Addr2>",
		      cmd_bridging_table_add, 6, 0),
	SHELL_CMD_ARG(table - remove, NULL, "<NetIdx1> <NetIdx2> <Addr1> <Addr2>",
		      cmd_bridging_table_remove, 5, 0),
	SHELL_CMD_ARG(subnets - get, NULL, "<Filter> <NetIdx> <StartIdx>", cmd_bridged_subnets_get,
		      4, 0),
	SHELL_CMD_ARG(table - get, NULL, "<NetIdx1> <NetIdx2> <StartIdx>", cmd_bridging_table_get,
		      4, 0),
	SHELL_SUBCMD_SET_END);

SHELL_SUBCMD_ADD((mesh, models), brg, &brg_cfg_cmds, "Bridge Configuration Cli commands",
		 bt_mesh_shell_mdl_cmds_help, 1, 1);
