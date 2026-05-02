/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "blob.h"

#include <stdlib.h>
#include <zephyr/shell/shell.h>
#include <zephyr/bluetooth/mesh.h>
#include <zephyr/bluetooth/mesh/shell.h>

#include "common/bt_shell_private.h"
#include "utils.h"

/***************************************************************************************************
 * Implementation of models' instances
 **************************************************************************************************/

static uint8_t blob_rx_sum;
bool bt_mesh_shell_blob_valid;
static const char *blob_data = "blob";

static int blob_io_open(const struct bt_mesh_blob_io *io,
		    const struct bt_mesh_blob_xfer *xfer,
		    enum bt_mesh_blob_io_mode mode)
{
	blob_rx_sum = 0;
	bt_mesh_shell_blob_valid = true;
	return 0;
}

static int blob_chunk_wr(const struct bt_mesh_blob_io *io,
			 const struct bt_mesh_blob_xfer *xfer,
			 const struct bt_mesh_blob_block *block,
			 const struct bt_mesh_blob_chunk *chunk)
{
	int i;

	for (i = 0; i < chunk->size; ++i) {
		blob_rx_sum += chunk->data[i];
		if (chunk->data[i] !=
		    blob_data[(i + chunk->offset) % sizeof(blob_data)]) {
			bt_mesh_shell_blob_valid = false;
		}
	}

	return 0;
}

static int blob_chunk_rd(const struct bt_mesh_blob_io *io,
			 const struct bt_mesh_blob_xfer *xfer,
			 const struct bt_mesh_blob_block *block,
			 const struct bt_mesh_blob_chunk *chunk)
{
	for (int i = 0; i < chunk->size; ++i) {
		chunk->data[i] =
			blob_data[(i + chunk->offset) % sizeof(blob_data)];
	}

	return 0;
}

static const struct bt_mesh_blob_io dummy_blob_io = {
	.open = blob_io_open,
	.rd = blob_chunk_rd,
	.wr = blob_chunk_wr,
};

const struct bt_mesh_blob_io *bt_mesh_shell_blob_io;

#if defined(CONFIG_BT_MESH_SHELL_BLOB_CLI)

static struct {
	struct bt_mesh_blob_cli_inputs inputs;
	struct bt_mesh_blob_target targets[32];
	struct bt_mesh_blob_target_pull pull[32];
	uint8_t target_count;
	struct bt_mesh_blob_xfer xfer;
} blob_cli_xfer;

static void blob_cli_lost_target(struct bt_mesh_blob_cli *cli,
				 struct bt_mesh_blob_target *target,
				 enum bt_mesh_blob_status reason)
{
	bt_shell_print("Mesh Blob: Lost target 0x%04x (reason: %u)", target->addr, reason);
}

static void blob_cli_caps(struct bt_mesh_blob_cli *cli,
			  const struct bt_mesh_blob_cli_caps *caps)
{
	static const char * const modes[] = {
		"none",
		"push",
		"pull",
		"all",
	};

	if (!caps) {
		bt_shell_print("None of the targets can be used for BLOB transfer");
		return;
	}

	bt_shell_print("Mesh BLOB: capabilities:");
	bt_shell_print("\tMax BLOB size: %u bytes", caps->max_size);
	bt_shell_print("\tBlock size: %u-%u (%u-%u bytes)",
		       caps->min_block_size_log, caps->max_block_size_log,
		       1 << caps->min_block_size_log,
		       1 << caps->max_block_size_log);
	bt_shell_print("\tMax chunks: %u", caps->max_chunks);
	bt_shell_print("\tChunk size: %u", caps->max_chunk_size);
	bt_shell_print("\tMTU size: %u", caps->mtu_size);
	bt_shell_print("\tModes: %s", modes[caps->modes]);
}

static void blob_cli_end(struct bt_mesh_blob_cli *cli,
			 const struct bt_mesh_blob_xfer *xfer, bool success)
{
	if (success) {
		bt_shell_print("Mesh BLOB transfer complete.");
	} else {
		bt_shell_print("Mesh BLOB transfer failed.");
	}
}

static uint8_t get_progress(const struct bt_mesh_blob_xfer_info *info)
{
	uint8_t total_blocks;
	uint8_t blocks_not_rxed = 0;
	uint8_t blocks_not_rxed_size;
	int i;

	total_blocks = DIV_ROUND_UP(info->size, 1U << info->block_size_log);

	blocks_not_rxed_size = DIV_ROUND_UP(total_blocks, 8);

	for (i = 0; i < blocks_not_rxed_size; i++) {
		blocks_not_rxed += info->missing_blocks[i % 8] & (1 << (i % 8));
	}

	return (total_blocks - blocks_not_rxed) / total_blocks;
}

static void xfer_progress(struct bt_mesh_blob_cli *cli,
			  struct bt_mesh_blob_target *target,
			  const struct bt_mesh_blob_xfer_info *info)
{
	uint8_t progress = get_progress(info);

	bt_shell_print("BLOB transfer progress received from target 0x%04x:\n"
		       "\tphase: %d\n"
		       "\tprogress: %u%%",
		       target->addr, info->phase, progress);
}

static void xfer_progress_complete(struct bt_mesh_blob_cli *cli)
{
	bt_shell_print("Determine BLOB transfer progress procedure complete");
}

static const struct bt_mesh_blob_cli_cb blob_cli_handlers = {
	.lost_target = blob_cli_lost_target,
	.caps = blob_cli_caps,
	.end = blob_cli_end,
	.xfer_progress = xfer_progress,
	.xfer_progress_complete = xfer_progress_complete,
};

struct bt_mesh_blob_cli bt_mesh_shell_blob_cli = {
	.cb = &blob_cli_handlers
};

#endif /* CONFIG_BT_MESH_SHELL_BLOB_CLI */

#if defined(CONFIG_BT_MESH_SHELL_BLOB_SRV)

static int64_t blob_time;

static int blob_srv_start(struct bt_mesh_blob_srv *srv,
			  struct bt_mesh_msg_ctx *ctx,
			  struct bt_mesh_blob_xfer *xfer)
{
	bt_shell_print("BLOB start");
	blob_time = k_uptime_get();
	return 0;
}

static void blob_srv_end(struct bt_mesh_blob_srv *srv, uint64_t id,
			 bool success)
{
	if (success) {
		int64_t duration = k_uptime_delta(&blob_time);

		bt_shell_print("BLOB completed in %u.%03u s",
			       (uint32_t)(duration / MSEC_PER_SEC),
			       (uint32_t)(duration % MSEC_PER_SEC));
	} else {
		bt_shell_print("BLOB cancelled");
	}
}

static const struct bt_mesh_blob_srv_cb blob_srv_cb = {
	.start = blob_srv_start,
	.end = blob_srv_end,
};

struct bt_mesh_blob_srv bt_mesh_shell_blob_srv = {
	.cb = &blob_srv_cb
};

#endif /* CONFIG_BT_MESH_SHELL_BLOB_SRV */

void bt_mesh_shell_blob_cmds_init(void)
{
	bt_mesh_shell_blob_io = &dummy_blob_io;
}

/***************************************************************************************************
 * Shell Commands
 **************************************************************************************************/

#if defined(CONFIG_BT_MESH_SHELL_BLOB_IO_FLASH)

static struct bt_mesh_blob_io_flash blob_flash_stream;

static int cmd_flash_stream_set(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t area_id;
	uint32_t offset = 0;
	int err = 0;

	if (argc < 2) {
		return -EINVAL;
	}

	area_id = shell_strtoul(argv[1], 0, &err);

	if (argc >= 3) {
		offset = shell_strtoul(argv[2], 0, &err);
	}

	if (err) {
		shell_warn(sh, "Unable to parse input string argument");
		return err;
	}

	err = bt_mesh_blob_io_flash_init(&blob_flash_stream, area_id, offset);
	if (err) {
		shell_error(sh, "Failed to init BLOB IO Flash module: %d\n", err);
	}

	bt_mesh_shell_blob_io = &blob_flash_stream.io;

	shell_print(sh, "Flash stream is initialized with area %u, offset: %u", area_id, offset);

	return 0;
}

static int cmd_flash_stream_unset(const struct shell *sh, size_t argc, char *argv[])
{
	bt_mesh_shell_blob_io = &dummy_blob_io;
	return 0;
}

#endif /* CONFIG_BT_MESH_SHELL_BLOB_IO_FLASH */

#if defined(CONFIG_BT_MESH_SHELL_BLOB_CLI)

static const struct bt_mesh_model *mod_cli;

static void blob_cli_inputs_prepare(uint16_t group)
{
	int i;

	blob_cli_xfer.inputs.ttl = BT_MESH_TTL_DEFAULT;
	blob_cli_xfer.inputs.group = group;
	blob_cli_xfer.inputs.app_idx = bt_mesh_shell_target_ctx.app_idx;
	sys_slist_init(&blob_cli_xfer.inputs.targets);

	for (i = 0; i < blob_cli_xfer.target_count; ++i) {
		/* Reset target context. */
		uint16_t addr = blob_cli_xfer.targets[i].addr;

		memset(&blob_cli_xfer.targets[i], 0, sizeof(struct bt_mesh_blob_target));
		memset(&blob_cli_xfer.pull[i], 0, sizeof(struct bt_mesh_blob_target_pull));
		blob_cli_xfer.targets[i].addr = addr;
		blob_cli_xfer.targets[i].pull = &blob_cli_xfer.pull[i];

		sys_slist_append(&blob_cli_xfer.inputs.targets,
				 &blob_cli_xfer.targets[i].n);
	}
}

static int cmd_tx(const struct shell *sh, size_t argc, char *argv[])
{
	uint16_t group;
	int err = 0;

	if (!mod_cli && !bt_mesh_shell_mdl_first_get(BT_MESH_MODEL_ID_BLOB_CLI, &mod_cli)) {
		return -ENODEV;
	}

	blob_cli_xfer.xfer.id = shell_strtoul(argv[1], 0, &err);
	blob_cli_xfer.xfer.size = shell_strtoul(argv[2], 0, &err);
	blob_cli_xfer.xfer.block_size_log = shell_strtoul(argv[3], 0, &err);
	blob_cli_xfer.xfer.chunk_size = shell_strtoul(argv[4], 0, &err);

	if (argc >= 6) {
		group = shell_strtoul(argv[5], 0, &err);
	} else {
		group = BT_MESH_ADDR_UNASSIGNED;
	}

	if (argc < 7 || !strcmp(argv[6], "push")) {
		blob_cli_xfer.xfer.mode = BT_MESH_BLOB_XFER_MODE_PUSH;
	} else if (!strcmp(argv[6], "pull")) {
		blob_cli_xfer.xfer.mode = BT_MESH_BLOB_XFER_MODE_PULL;
	} else {
		shell_print(sh, "Mode must be either push or pull");
		return -EINVAL;
	}

	if (argc >= 8) {
		blob_cli_xfer.inputs.timeout_base = shell_strtoul(argv[7], 0, &err);
	} else {
		blob_cli_xfer.inputs.timeout_base = 0;
	}

	if (err) {
		shell_warn(sh, "Unable to parse input string argument");
		return err;
	}

	if (!blob_cli_xfer.target_count) {
		shell_print(sh, "Failed: No targets");
		return 0;
	}

	blob_cli_inputs_prepare(group);

	shell_print(sh,
		    "Sending transfer 0x%x (mode: %s, %u bytes) to 0x%04x",
		    (uint32_t)blob_cli_xfer.xfer.id,
		    blob_cli_xfer.xfer.mode == BT_MESH_BLOB_XFER_MODE_PUSH ?
			    "push" :
			    "pull",
		    blob_cli_xfer.xfer.size, group);

	err = bt_mesh_blob_cli_send((struct bt_mesh_blob_cli *)mod_cli->rt->user_data,
				    &blob_cli_xfer.inputs,
				    &blob_cli_xfer.xfer, bt_mesh_shell_blob_io);
	if (err) {
		shell_print(sh, "BLOB transfer TX failed (err: %d)", err);
	}

	return 0;
}

static int cmd_target(const struct shell *sh, size_t argc, char *argv[])
{
	struct bt_mesh_blob_target *t;
	int err = 0;

	if (blob_cli_xfer.target_count ==
	    ARRAY_SIZE(blob_cli_xfer.targets)) {
		shell_print(sh, "No more room");
		return 0;
	}

	t = &blob_cli_xfer.targets[blob_cli_xfer.target_count];
	t->addr = shell_strtoul(argv[1], 0, &err);

	if (err) {
		shell_warn(sh, "Unable to parse input string argument");
		return err;
	}

	shell_print(sh, "Added target 0x%04x", t->addr);

	blob_cli_xfer.target_count++;
	return 0;
}

static int cmd_caps(const struct shell *sh, size_t argc, char *argv[])
{
	uint16_t group;
	int err = 0;

	if (!mod_cli && !bt_mesh_shell_mdl_first_get(BT_MESH_MODEL_ID_BLOB_CLI, &mod_cli)) {
		return -ENODEV;
	}

	shell_print(sh, "Retrieving transfer capabilities...");

	if (argc > 1) {
		group = shell_strtoul(argv[1], 0, &err);
	} else {
		group = BT_MESH_ADDR_UNASSIGNED;
	}

	if (argc > 2) {
		blob_cli_xfer.inputs.timeout_base = shell_strtoul(argv[2], 0, &err);
	} else {
		blob_cli_xfer.inputs.timeout_base = 0;
	}

	if (err) {
		shell_warn(sh, "Unable to parse input string argument");
		return err;
	}

	if (!blob_cli_xfer.target_count) {
		shell_print(sh, "Failed: No targets");
		return 0;
	}

	blob_cli_inputs_prepare(group);

	err = bt_mesh_blob_cli_caps_get((struct bt_mesh_blob_cli *)mod_cli->rt->user_data,
					&blob_cli_xfer.inputs);
	if (err) {
		shell_print(sh, "Boundary check start failed (err: %d)", err);
	}

	return 0;
}

static int cmd_tx_cancel(const struct shell *sh, size_t argc,
			      char *argv[])
{
	if (!mod_cli && !bt_mesh_shell_mdl_first_get(BT_MESH_MODEL_ID_BLOB_CLI, &mod_cli)) {
		return -ENODEV;
	}

	shell_print(sh, "Cancelling transfer");
	bt_mesh_blob_cli_cancel((struct bt_mesh_blob_cli *)mod_cli->rt->user_data);

	return 0;
}

static int cmd_tx_get(const struct shell *sh, size_t argc, char *argv[])
{
	uint16_t group;
	int err;

	if (!mod_cli && !bt_mesh_shell_mdl_first_get(BT_MESH_MODEL_ID_BLOB_CLI, &mod_cli)) {
		return -ENODEV;
	}

	if (argc > 1) {
		group = shell_strtoul(argv[1], 0, &err);
	} else {
		group = BT_MESH_ADDR_UNASSIGNED;
	}

	if (!blob_cli_xfer.target_count) {
		shell_print(sh, "Failed: No targets");
		return -EINVAL;
	}

	blob_cli_inputs_prepare(group);

	err = bt_mesh_blob_cli_xfer_progress_get((struct bt_mesh_blob_cli *)mod_cli->rt->user_data,
						 &blob_cli_xfer.inputs);
	if (err) {
		shell_print(sh, "ERR %d", err);
		return err;
	}
	return 0;
}

static int cmd_tx_suspend(const struct shell *sh, size_t argc,
			       char *argv[])
{
	if (!mod_cli && !bt_mesh_shell_mdl_first_get(BT_MESH_MODEL_ID_BLOB_CLI, &mod_cli)) {
		return -ENODEV;
	}

	shell_print(sh, "Suspending transfer");
	bt_mesh_blob_cli_suspend((struct bt_mesh_blob_cli *)mod_cli->rt->user_data);

	return 0;
}

static int cmd_tx_resume(const struct shell *sh, size_t argc, char *argv[])
{
	if (!mod_cli && !bt_mesh_shell_mdl_first_get(BT_MESH_MODEL_ID_BLOB_CLI, &mod_cli)) {
		return -ENODEV;
	}

	shell_print(sh, "Resuming transfer");
	bt_mesh_blob_cli_resume((struct bt_mesh_blob_cli *)mod_cli->rt->user_data);

	return 0;
}

#endif /* CONFIG_BT_MESH_SHELL_BLOB_CLI */

#if defined(CONFIG_BT_MESH_SHELL_BLOB_SRV)

static const struct bt_mesh_model *mod_srv;

static int cmd_rx(const struct shell *sh, size_t argc, char *argv[])
{
	uint16_t timeout_base;
	uint32_t id;
	int err = 0;

	if (!mod_srv && !bt_mesh_shell_mdl_first_get(BT_MESH_MODEL_ID_BLOB_SRV, &mod_srv)) {
		return -ENODEV;
	}

	id = shell_strtoul(argv[1], 0, &err);
	blob_rx_sum = 0;

	if (argc > 2) {
		timeout_base = shell_strtoul(argv[2], 0, &err);
	} else {
		timeout_base = 0U;
	}

	if (err) {
		shell_warn(sh, "Unable to parse input string argument");
		return err;
	}

	shell_print(sh, "Receive BLOB 0x%x", id);
	err = bt_mesh_blob_srv_recv((struct bt_mesh_blob_srv *)mod_srv->rt->user_data,
				    id, bt_mesh_shell_blob_io, BT_MESH_TTL_MAX, timeout_base);
	if (err) {
		shell_print(sh, "BLOB RX setup failed (%d)", err);
	}

	return 0;
}

static int cmd_rx_cancel(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	if (!mod_srv && !bt_mesh_shell_mdl_first_get(BT_MESH_MODEL_ID_BLOB_SRV, &mod_srv)) {
		return -ENODEV;
	}

	shell_print(sh, "Cancelling BLOB rx");
	err = bt_mesh_blob_srv_cancel((struct bt_mesh_blob_srv *)mod_srv->rt->user_data);
	if (err) {
		shell_print(sh, "BLOB cancel failed (%d)", err);
	}

	return 0;
}

#endif /* CONFIG_BT_MESH_SHELL_BLOB_SRV */

#if defined(CONFIG_BT_MESH_SHELL_BLOB_CLI)
BT_MESH_SHELL_MDL_INSTANCE_CMDS(cli_instance_cmds, BT_MESH_MODEL_ID_BLOB_CLI, mod_cli);
#endif
#if defined(CONFIG_BT_MESH_SHELL_BLOB_SRV)
BT_MESH_SHELL_MDL_INSTANCE_CMDS(srv_instance_cmds, BT_MESH_MODEL_ID_BLOB_SRV, mod_srv);
#endif

#if defined(CONFIG_BT_MESH_SHELL_BLOB_CLI)
SHELL_STATIC_SUBCMD_SET_CREATE(
	blob_cli_cmds,
	/* BLOB Client Model Operations */
	SHELL_CMD_ARG(target, NULL, "<Addr>", cmd_target, 2, 0),
	SHELL_CMD_ARG(caps, NULL, "[<Group> [<TimeoutBase>]]", cmd_caps, 1, 2),
	SHELL_CMD_ARG(tx, NULL, "<ID> <Size> <BlockSizeLog> "
		      "<ChunkSize> [<Group> [<Mode(push, pull)> "
		      "[<TimeoutBase>]]]", cmd_tx, 5, 3),
	SHELL_CMD_ARG(tx-cancel, NULL, NULL, cmd_tx_cancel, 1, 0),
	SHELL_CMD_ARG(tx-get, NULL, "[Group]", cmd_tx_get, 1, 1),
	SHELL_CMD_ARG(tx-suspend, NULL, NULL, cmd_tx_suspend, 1, 0),
	SHELL_CMD_ARG(tx-resume, NULL, NULL, cmd_tx_resume, 1, 0),
	SHELL_CMD(instance, &cli_instance_cmds, "Instance commands", bt_mesh_shell_mdl_cmds_help),
	SHELL_SUBCMD_SET_END);
#endif

#if defined(CONFIG_BT_MESH_SHELL_BLOB_SRV)
SHELL_STATIC_SUBCMD_SET_CREATE(
	blob_srv_cmds,
	/* BLOB Server Model Operations */
	SHELL_CMD_ARG(rx, NULL, "<ID> [<TimeoutBase(10s steps)>]", cmd_rx, 2, 1),
	SHELL_CMD_ARG(rx-cancel, NULL, NULL, cmd_rx_cancel, 1, 0),
	SHELL_CMD(instance, &srv_instance_cmds, "Instance commands", bt_mesh_shell_mdl_cmds_help),
	SHELL_SUBCMD_SET_END);
#endif

SHELL_STATIC_SUBCMD_SET_CREATE(
	blob_cmds,
#if defined(CONFIG_BT_MESH_SHELL_BLOB_IO_FLASH)
	SHELL_CMD_ARG(flash-stream-set, NULL, "<AreaID> [<Offset>]",
		      cmd_flash_stream_set, 2, 1),
	SHELL_CMD_ARG(flash-stream-unset, NULL, NULL, cmd_flash_stream_unset, 1, 0),
#endif
#if defined(CONFIG_BT_MESH_SHELL_BLOB_CLI)
	SHELL_CMD(cli, &blob_cli_cmds, "BLOB Cli commands", bt_mesh_shell_mdl_cmds_help),
#endif
#if defined(CONFIG_BT_MESH_SHELL_BLOB_SRV)
	SHELL_CMD(srv, &blob_srv_cmds, "BLOB Srv commands", bt_mesh_shell_mdl_cmds_help),
#endif
	SHELL_SUBCMD_SET_END);

SHELL_SUBCMD_ADD((mesh, models), blob, &blob_cmds, "BLOB models commands",
		 bt_mesh_shell_mdl_cmds_help, 1, 1);
