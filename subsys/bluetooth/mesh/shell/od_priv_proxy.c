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

static int cmd_od_priv_gatt_proxy_set(const struct shell *sh, size_t argc,
				      char *argv[])
{
	uint8_t val, val_rsp;
	int err = 0;

	struct bt_mesh_msg_ctx ctx = BT_MESH_MSG_CTX_INIT_DEV(bt_mesh_shell_target_ctx.net_idx,
							      bt_mesh_shell_target_ctx.dst);

	if (argc < 2) {
		err = bt_mesh_od_priv_proxy_cli_get(&ctx, &val_rsp);
	} else {
		val = shell_strtoul(argv[1], 0, &err);

		if (err) {
			shell_warn(sh, "Unable to parse input string argument");
			return err;
		}

		err = bt_mesh_od_priv_proxy_cli_set(&ctx, val, &val_rsp);
	}

	if (err) {
		shell_print(sh, "Unable to send On-Demand Private GATT Proxy Get/Set (err %d)",
			    err);
		return 0;
	}

	shell_print(sh, "On-Demand Private GATT Proxy is set to 0x%02x", val_rsp);

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	od_priv_proxy_cmds,
	SHELL_CMD_ARG(gatt-proxy, NULL, "[Dur(s)]", cmd_od_priv_gatt_proxy_set, 1, 1),
	SHELL_SUBCMD_SET_END);

SHELL_SUBCMD_ADD((mesh, models), od_priv_proxy, &od_priv_proxy_cmds,
		 "On-Demand Private Proxy Cli commands",
		 bt_mesh_shell_mdl_cmds_help, 1, 1);
