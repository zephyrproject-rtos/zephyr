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

static int cmd_large_comp_data_get(const struct shell *sh, size_t argc, char *argv[])
{
	NET_BUF_SIMPLE_DEFINE(comp, 64);
	uint8_t page;
	uint16_t offset;
	int err = 0;

	net_buf_simple_init(&comp, 0);

	page = shell_strtoul(argv[1], 0, &err);
	offset = shell_strtoul(argv[2], 0, &err);

	if (err) {
		shell_warn(sh, "Unable to parse input string argument");
		return err;
	}

	err = bt_mesh_large_comp_data_get(bt_mesh_shell_target_ctx.net_idx,
					  bt_mesh_shell_target_ctx.dst, page, offset,
					  &comp);
	if (err) {
		shell_print(sh, "Failed to send Large Composition Data Get (err=%d)", err);
		return err;
	}

	shell_print(sh, "Large Composition Data Get len=%d", comp.len);

	return 0;
}

static int cmd_models_metadata_get(const struct shell *sh, size_t argc, char *argv[])
{
	NET_BUF_SIMPLE_DEFINE(metadata, 64);
	uint8_t page;
	uint16_t offset;
	int err = 0;

	net_buf_simple_init(&metadata, 0);

	page = shell_strtoul(argv[1], 0, &err);
	offset = shell_strtoul(argv[2], 0, &err);

	if (err) {
		shell_warn(sh, "Unable to parse input string argument");
		return err;
	}

	err = bt_mesh_models_metadata_get(bt_mesh_shell_target_ctx.net_idx,
					  bt_mesh_shell_target_ctx.dst, page, offset,
					  &metadata);
	if (err) {
		shell_print(sh, "Failed to send Models Metadata Get (err=%d)", err);
		return err;
	}

	shell_print(sh, "Models Metadata Get len=%d", metadata.len);

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	large_comp_data_cmds,
	SHELL_CMD_ARG(large-comp-data-get, NULL, "<page> <offset>", cmd_large_comp_data_get, 3, 0),
	SHELL_CMD_ARG(models-metadata-get, NULL, "<page> <offset>", cmd_models_metadata_get, 3, 0),
	SHELL_SUBCMD_SET_END);

SHELL_SUBCMD_ADD((mesh, models), lcd, &large_comp_data_cmds,
		 "Large Comp Data Cli commands",
		 bt_mesh_shell_mdl_cmds_help, 1, 1);
