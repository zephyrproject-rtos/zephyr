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

static const struct bt_mesh_model *mod;

/***************************************************************************************************
 * Implementation of the model's instance
 **************************************************************************************************/

static void rpr_scan_report(struct bt_mesh_rpr_cli *cli,
			    const struct bt_mesh_rpr_node *srv,
			    struct bt_mesh_rpr_unprov *unprov,
			    struct net_buf_simple *adv_data)
{
	char uuid_hex_str[32 + 1];

	bin2hex(unprov->uuid, 16, uuid_hex_str, sizeof(uuid_hex_str));

	bt_shell_print("Server 0x%04x:\n"
		       "\tuuid:   %s\n"
		       "\tOOB:    0x%04x",
		       srv->addr, uuid_hex_str, unprov->oob);

	while (adv_data && adv_data->len > 2) {
		uint8_t len, type;
		uint8_t data[31];

		len = net_buf_simple_pull_u8(adv_data);
		if (len == 0) {
			/* No data in this AD Structure. */
			continue;
		}

		if (len > adv_data->len) {
			/* Malformed AD Structure. */
			break;
		}

		type = net_buf_simple_pull_u8(adv_data);
		if ((--len) > 0) {
			uint8_t dlen;

			/* Pull all length, but print only what fits into `data` array. */
			dlen = MIN(len, sizeof(data) - 1);
			memcpy(data, net_buf_simple_pull_mem(adv_data, len), dlen);
			len = dlen;
		}
		data[len] = '\0';

		if (type == BT_DATA_URI) {
			bt_shell_print("\tURI:    \"\\x%02x%s\"", data[0], &data[1]);
		} else if (type == BT_DATA_NAME_COMPLETE) {
			bt_shell_print("\tName:   \"%s\"", data);
		} else {
			char string[64 + 1];

			bin2hex(data, len, string, sizeof(string));
			bt_shell_print("\t0x%02x:  %s", type, string);
		}
	}
}

struct bt_mesh_rpr_cli bt_mesh_shell_rpr_cli = {
	.scan_report = rpr_scan_report,
};

/***************************************************************************************************
 * Shell Commands
 **************************************************************************************************/

static int cmd_scan(const struct shell *sh, size_t argc, char *argv[])
{
	struct bt_mesh_rpr_scan_status rsp;
	const struct bt_mesh_rpr_node srv = {
		.addr = bt_mesh_shell_target_ctx.dst,
		.net_idx = bt_mesh_shell_target_ctx.net_idx,
		.ttl = BT_MESH_TTL_DEFAULT,
	};
	uint8_t uuid[16] = {0};
	uint8_t timeout;
	int err = 0;

	if (!mod && !bt_mesh_shell_mdl_first_get(BT_MESH_MODEL_ID_REMOTE_PROV_CLI, &mod)) {
		return -ENODEV;
	}

	timeout = shell_strtoul(argv[1], 0, &err);
	if (err) {
		shell_warn(sh, "Unable to parse input string argument");
		return err;
	}

	if (argc > 2) {
		hex2bin(argv[2], strlen(argv[2]), uuid, 16);
	}

	err = bt_mesh_rpr_scan_start((struct bt_mesh_rpr_cli *)mod->rt->user_data,
				     &srv, argc > 2 ? uuid : NULL, timeout,
				     BT_MESH_RPR_SCAN_MAX_DEVS_ANY, &rsp);
	if (err) {
		shell_print(sh, "Scan start failed: %d", err);
		return err;
	}

	if (rsp.status == BT_MESH_RPR_SUCCESS) {
		shell_print(sh, "Scan started.");
	} else {
		shell_print(sh, "Scan start response: %d", rsp.status);
	}

	return 0;
}

static int cmd_scan_ext(const struct shell *sh, size_t argc, char *argv[])
{
	const struct bt_mesh_rpr_node srv = {
		.addr = bt_mesh_shell_target_ctx.dst,
		.net_idx = bt_mesh_shell_target_ctx.net_idx,
		.ttl = BT_MESH_TTL_DEFAULT,
	};
	uint8_t ad_types[CONFIG_BT_MESH_RPR_AD_TYPES_MAX];
	uint8_t uuid[16] = {0};
	uint8_t timeout;
	int i, err = 0;

	if (!mod && !bt_mesh_shell_mdl_first_get(BT_MESH_MODEL_ID_REMOTE_PROV_CLI, &mod)) {
		return -ENODEV;
	}

	hex2bin(argv[2], strlen(argv[2]), uuid, 16);

	for (i = 0; i < argc - 3; i++) {
		ad_types[i] = shell_strtoul(argv[3 + i], 0, &err);
	}

	timeout = shell_strtoul(argv[1], 0, &err);
	if (err) {
		shell_warn(sh, "Unable to parse input string argument");
		return err;
	}

	err = bt_mesh_rpr_scan_start_ext((struct bt_mesh_rpr_cli *)mod->rt->user_data,
					 &srv, uuid, timeout, ad_types,
					 (argc - 3));
	if (err) {
		shell_print(sh, "Scan start failed: %d", err);
		return err;
	}

	shell_print(sh, "Extended scan started.");

	return 0;
}

static int cmd_scan_srv(const struct shell *sh, size_t argc, char *argv[])
{
	const struct bt_mesh_rpr_node srv = {
		.addr = bt_mesh_shell_target_ctx.dst,
		.net_idx = bt_mesh_shell_target_ctx.net_idx,
		.ttl = BT_MESH_TTL_DEFAULT,
	};
	uint8_t ad_types[CONFIG_BT_MESH_RPR_AD_TYPES_MAX];
	int i, err = 0;

	if (!mod && !bt_mesh_shell_mdl_first_get(BT_MESH_MODEL_ID_REMOTE_PROV_CLI, &mod)) {
		return -ENODEV;
	}

	for (i = 0; i < argc - 1; i++) {
		ad_types[i] = shell_strtoul(argv[1 + i], 0, &err);
	}

	if (err) {
		shell_warn(sh, "Unable to parse input string argument");
		return err;
	}

	err = bt_mesh_rpr_scan_start_ext((struct bt_mesh_rpr_cli *)mod->rt->user_data,
					 &srv, NULL, 0, ad_types, (argc - 1));
	if (err) {
		shell_print(sh, "Scan start failed: %d", err);
		return err;
	}

	return 0;
}

static int cmd_scan_caps(const struct shell *sh, size_t argc, char *argv[])
{
	struct bt_mesh_rpr_caps caps;
	const struct bt_mesh_rpr_node srv = {
		.addr = bt_mesh_shell_target_ctx.dst,
		.net_idx = bt_mesh_shell_target_ctx.net_idx,
		.ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	if (!mod && !bt_mesh_shell_mdl_first_get(BT_MESH_MODEL_ID_REMOTE_PROV_CLI, &mod)) {
		return -ENODEV;
	}

	err = bt_mesh_rpr_scan_caps_get((struct bt_mesh_rpr_cli *)mod->rt->user_data, &srv, &caps);
	if (err) {
		shell_print(sh, "Scan capabilities get failed: %d", err);
		return err;
	}

	shell_print(sh, "Remote Provisioning scan capabilities of 0x%04x:",
		    bt_mesh_shell_target_ctx.dst);
	shell_print(sh, "\tMax devices:     %u", caps.max_devs);
	shell_print(sh, "\tActive scanning: %s",
		    caps.active_scan ? "true" : "false");
	return 0;
}

static int cmd_scan_get(const struct shell *sh, size_t argc, char *argv[])
{
	struct bt_mesh_rpr_scan_status rsp;
	const struct bt_mesh_rpr_node srv = {
		.addr = bt_mesh_shell_target_ctx.dst,
		.net_idx = bt_mesh_shell_target_ctx.net_idx,
		.ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	if (!mod && !bt_mesh_shell_mdl_first_get(BT_MESH_MODEL_ID_REMOTE_PROV_CLI, &mod)) {
		return -ENODEV;
	}

	err = bt_mesh_rpr_scan_get((struct bt_mesh_rpr_cli *)mod->rt->user_data, &srv, &rsp);
	if (err) {
		shell_print(sh, "Scan get failed: %d", err);
		return err;
	}

	shell_print(sh, "Remote Provisioning scan on 0x%04x:", bt_mesh_shell_target_ctx.dst);
	shell_print(sh, "\tStatus:         %u", rsp.status);
	shell_print(sh, "\tScan type:      %u", rsp.scan);
	shell_print(sh, "\tMax devices:    %u", rsp.max_devs);
	shell_print(sh, "\tRemaining time: %u", rsp.timeout);
	return 0;
}

static int cmd_scan_stop(const struct shell *sh, size_t argc, char *argv[])
{
	struct bt_mesh_rpr_scan_status rsp;
	const struct bt_mesh_rpr_node srv = {
		.addr = bt_mesh_shell_target_ctx.dst,
		.net_idx = bt_mesh_shell_target_ctx.net_idx,
		.ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	if (!mod && !bt_mesh_shell_mdl_first_get(BT_MESH_MODEL_ID_REMOTE_PROV_CLI, &mod)) {
		return -ENODEV;
	}

	err = bt_mesh_rpr_scan_stop((struct bt_mesh_rpr_cli *)mod->rt->user_data, &srv, &rsp);
	if (err || rsp.status) {
		shell_print(sh, "Scan stop failed: %d %u", err, rsp.status);
		return err;
	}

	shell_print(sh, "Remote Provisioning scan on 0x%04x stopped.",
		    bt_mesh_shell_target_ctx.dst);
	return 0;
}

static int cmd_link_get(const struct shell *sh, size_t argc, char *argv[])
{
	struct bt_mesh_rpr_link rsp;
	const struct bt_mesh_rpr_node srv = {
		.addr = bt_mesh_shell_target_ctx.dst,
		.net_idx = bt_mesh_shell_target_ctx.net_idx,
		.ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	if (!mod && !bt_mesh_shell_mdl_first_get(BT_MESH_MODEL_ID_REMOTE_PROV_CLI, &mod)) {
		return -ENODEV;
	}

	err = bt_mesh_rpr_link_get((struct bt_mesh_rpr_cli *)mod->rt->user_data, &srv, &rsp);
	if (err) {
		shell_print(sh, "Link get failed: %d %u", err, rsp.status);
		return err;
	}

	shell_print(sh, "Remote Provisioning Link on 0x%04x:", bt_mesh_shell_target_ctx.dst);
	shell_print(sh, "\tStatus: %u", rsp.status);
	shell_print(sh, "\tState:  %u", rsp.state);
	return 0;
}

static int cmd_link_close(const struct shell *sh, size_t argc, char *argv[])
{
	struct bt_mesh_rpr_link rsp;
	const struct bt_mesh_rpr_node srv = {
		.addr = bt_mesh_shell_target_ctx.dst,
		.net_idx = bt_mesh_shell_target_ctx.net_idx,
		.ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	err = bt_mesh_rpr_link_close((struct bt_mesh_rpr_cli *)mod->rt->user_data, &srv, &rsp);
	if (err) {
		shell_print(sh, "Link close failed: %d %u", err, rsp.status);
		return err;
	}

	shell_print(sh, "Remote Provisioning Link on 0x%04x:", bt_mesh_shell_target_ctx.dst);
	shell_print(sh, "\tStatus: %u", rsp.status);
	shell_print(sh, "\tState:  %u", rsp.state);
	return 0;
}

static int cmd_provision_remote(const struct shell *sh, size_t argc, char *argv[])
{
	struct bt_mesh_rpr_node srv = {
		.addr = bt_mesh_shell_target_ctx.dst,
		.net_idx = bt_mesh_shell_target_ctx.net_idx,
		.ttl = BT_MESH_TTL_DEFAULT,
	};
	uint8_t uuid[16];
	size_t len;
	uint16_t net_idx;
	uint16_t addr;
	int err = 0;

	if (!mod && !bt_mesh_shell_mdl_first_get(BT_MESH_MODEL_ID_REMOTE_PROV_CLI, &mod)) {
		return -ENODEV;
	}

	len = hex2bin(argv[1], strlen(argv[1]), uuid, sizeof(uuid));
	(void)memset(uuid + len, 0, sizeof(uuid) - len);

	net_idx = shell_strtoul(argv[2], 0, &err);
	addr = shell_strtoul(argv[3], 0, &err);
	if (err) {
		shell_warn(sh, "Unable to parse input string argument");
		return err;
	}

	err = bt_mesh_provision_remote((struct bt_mesh_rpr_cli *)mod->rt->user_data,
				       &srv, uuid, net_idx, addr);
	if (err) {
		shell_print(sh, "Prov remote start failed: %d", err);
	}

	return err;
}

static int cmd_reprovision_remote(const struct shell *sh, size_t argc, char *argv[])
{
	struct bt_mesh_rpr_node srv = {
		.addr = bt_mesh_shell_target_ctx.dst,
		.net_idx = bt_mesh_shell_target_ctx.net_idx,
		.ttl = BT_MESH_TTL_DEFAULT,
	};
	bool composition_changed;
	uint16_t addr;
	int err = 0;

	if (!mod && !bt_mesh_shell_mdl_first_get(BT_MESH_MODEL_ID_REMOTE_PROV_CLI, &mod)) {
		return -ENODEV;
	}

	addr = shell_strtoul(argv[1], 0, &err);
	if (err) {
		shell_warn(sh, "Unable to parse input string argument");
		return err;
	}

	if (!BT_MESH_ADDR_IS_UNICAST(addr)) {
		shell_print(sh, "Must be a valid unicast address");
		return -EINVAL;
	}

	composition_changed = (argc > 2 && shell_strtobool(argv[2], 0, &err));
	if (err) {
		shell_warn(sh, "Unable to parse input string argument");
		return err;
	}

	err = bt_mesh_reprovision_remote((struct bt_mesh_rpr_cli *)mod->rt->user_data,
					 &srv, addr, composition_changed);
	if (err) {
		shell_print(sh, "Reprovisioning failed: %d", err);
	}

	return 0;
}

BT_MESH_SHELL_MDL_INSTANCE_CMDS(instance_cmds, BT_MESH_MODEL_ID_REMOTE_PROV_CLI, mod);

SHELL_STATIC_SUBCMD_SET_CREATE(
	rpr_cli_cmds,
	SHELL_CMD_ARG(scan, NULL, "<Timeout(s)> [<UUID(1-16 hex)>]", cmd_scan, 2, 1),
	SHELL_CMD_ARG(scan-ext, NULL, "<Timeout(s)> <UUID(1-16 hex)> [<ADType> ... ]",
		      cmd_scan_ext, 3, CONFIG_BT_MESH_RPR_AD_TYPES_MAX),
	SHELL_CMD_ARG(scan-srv, NULL, "[<ADType> ... ]", cmd_scan_srv, 1,
		      CONFIG_BT_MESH_RPR_AD_TYPES_MAX),
	SHELL_CMD_ARG(scan-caps, NULL, NULL, cmd_scan_caps, 1, 0),
	SHELL_CMD_ARG(scan-get, NULL, NULL, cmd_scan_get, 1, 0),
	SHELL_CMD_ARG(scan-stop, NULL, NULL, cmd_scan_stop, 1, 0),
	SHELL_CMD_ARG(link-get, NULL, NULL, cmd_link_get, 1, 0),
	SHELL_CMD_ARG(link-close, NULL, NULL, cmd_link_close, 1, 0),
	SHELL_CMD_ARG(provision-remote, NULL, "<UUID(1-16 hex)> <NetKeyIdx> <Addr>",
		      cmd_provision_remote, 4, 0),
	SHELL_CMD_ARG(reprovision-remote, NULL, "<Addr> [<CompChanged(false, true)>]",
		      cmd_reprovision_remote, 2, 1),
	SHELL_CMD(instance, &instance_cmds, "Instance commands", bt_mesh_shell_mdl_cmds_help),
	SHELL_SUBCMD_SET_END);

SHELL_SUBCMD_ADD((mesh, models), rpr, &rpr_cli_cmds, "Remote Provisioning Cli commands",
		 bt_mesh_shell_mdl_cmds_help, 1, 1);
