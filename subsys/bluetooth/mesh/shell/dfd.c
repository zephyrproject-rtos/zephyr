/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <zephyr/bluetooth/mesh.h>
#include <zephyr/shell/shell.h>

#include "utils.h"

#include "../dfu_slot.h"
#include "../dfd_srv_internal.h"
#include "../access.h"

static struct bt_mesh_model *mod;

static void print_receivers_status(const struct shell *sh, struct bt_mesh_dfd_srv *srv,
				   enum bt_mesh_dfd_status status)
{
	shell_print(sh, "{\"status\": %d, \"target_cnt\": %d}", status, srv->target_cnt);
}

static void print_dfd_status(const struct shell *sh, struct bt_mesh_dfd_srv *srv,
			     enum bt_mesh_dfd_status status)
{
	shell_fprintf(sh, SHELL_NORMAL, "{ \"status\": %d, \"phase\": %d", status,
		      srv->phase);

	if (srv->phase != BT_MESH_DFD_PHASE_IDLE && srv->dfu.xfer.slot) {
		shell_fprintf(sh, SHELL_NORMAL, ", \"group\": 0x%04x, \"app_idx\": %d, "
			      "\"ttl\": %d, \"timeout_base\": %d, \"xfer_mode\": %d, "
			      "\"apply\": %d, \"slot_idx\": %d", srv->inputs.group,
			      srv->inputs.app_idx, srv->inputs.ttl, srv->inputs.timeout_base,
			      srv->dfu.xfer.blob.mode, srv->apply, srv->slot_idx);
	}

	shell_print(sh, " }");
}

static void print_fw_status(const struct shell *sh, enum bt_mesh_dfd_status status,
			    uint16_t idx, const uint8_t *fwid, size_t fwid_len)
{
	shell_fprintf(sh, SHELL_NORMAL, "{ \"status\": %d, \"slot_cnt\": %d, \"idx\": %d",
		      status, bt_mesh_dfu_slot_foreach(NULL, NULL), idx);
	if (fwid) {
		shell_fprintf(sh, SHELL_NORMAL, ", \"fwid\": \"");
		for (size_t i = 0; i < fwid_len; i++) {
			shell_fprintf(sh, SHELL_NORMAL, "%02x", fwid[i]);
		}
		shell_fprintf(sh, SHELL_NORMAL, "\"");
	}
	shell_print(sh, " }");
}

static enum bt_mesh_dfu_iter slot_space_cb(const struct bt_mesh_dfu_slot *slot,
					   void *user_data)
{
	size_t *total = user_data;

	*total += slot->size;

	return BT_MESH_DFU_ITER_CONTINUE;
}

static int cmd_dfd_receivers_add(const struct shell *sh, size_t argc, char *argv[])
{
	if (!mod && !bt_mesh_shell_mdl_first_get(BT_MESH_MODEL_ID_DFD_SRV, &mod)) {
		return -ENODEV;
	}

	struct bt_mesh_dfd_srv *dfd_srv = mod->user_data;

	if (bt_mesh_dfu_cli_is_busy(&dfd_srv->dfu)) {
		print_receivers_status(sh, dfd_srv,
				       BT_MESH_DFD_ERR_BUSY_WITH_DISTRIBUTION);
		return -EBUSY;
	}

	char *outer_state, *inner_state;

	char *token = strtok_r(argv[1], ";", &outer_state);

	while (token) {
		char *addr_str = strtok_r(token, ",", &inner_state);
		char *img_idx_str = strtok_r(NULL, ",", &inner_state);
		int err = 0;

		if (addr_str == NULL || img_idx_str == NULL) {
			return -EINVAL;
		}

		uint16_t addr = shell_strtoul(addr_str, 0, &err);
		uint8_t img_idx = shell_strtoul(img_idx_str, 0, &err);

		if (err) {
			shell_warn(sh, "Unable to parse input string argument");
			return err;
		}

		enum bt_mesh_dfd_status status = bt_mesh_dfd_srv_receiver_add(
			dfd_srv, addr, img_idx);

		if (status != BT_MESH_DFD_SUCCESS) {
			print_receivers_status(sh, dfd_srv, status);
			return status == BT_MESH_DFD_ERR_INSUFFICIENT_RESOURCES ?
				-ENOSPC : -EINVAL;
		}

		token = strtok_r(NULL, ";", &outer_state);
	}

	print_receivers_status(sh, dfd_srv, BT_MESH_DFD_SUCCESS);

	return 0;
}

static int cmd_dfd_receivers_delete_all(const struct shell *sh, size_t argc, char *argv[])
{
	if (!mod && !bt_mesh_shell_mdl_first_get(BT_MESH_MODEL_ID_DFD_SRV, &mod)) {
		return -ENODEV;
	}

	struct bt_mesh_dfd_srv *dfd_srv = mod->user_data;

	enum bt_mesh_dfd_status status = bt_mesh_dfd_srv_receivers_delete_all(
		dfd_srv);

	print_receivers_status(sh, dfd_srv, status);

	if (status != BT_MESH_DFD_SUCCESS) {
		return status == BT_MESH_DFD_ERR_BUSY_WITH_DISTRIBUTION ? -EBUSY : -EINVAL;
	}

	return 0;
}

static int cmd_dfd_receivers_get(const struct shell *sh, size_t argc, char *argv[])
{
	if (!mod && !bt_mesh_shell_mdl_first_get(BT_MESH_MODEL_ID_DFD_SRV, &mod)) {
		return -ENODEV;
	}

	struct bt_mesh_dfd_srv *dfd_srv = mod->user_data;
	int err = 0;

	uint16_t first = shell_strtoul(argv[1], 0, &err);
	uint16_t cnt = shell_strtoul(argv[2], 0, &err);

	if (err) {
		shell_warn(sh, "Unable to parse input string argument");
		return err;
	}

	if (cnt == 0 || dfd_srv->target_cnt <= first) {
		return -EINVAL;
	}

	cnt = MIN(cnt, dfd_srv->target_cnt - first);
	uint8_t progress = bt_mesh_dfu_cli_progress(&dfd_srv->dfu) / 2;

	shell_print(sh, "{\n\t\"target_cnt\": %d,\n\t\"targets\": {",
		    dfd_srv->target_cnt);
	for (int i = 0; i < cnt; i++) {
		const struct bt_mesh_dfu_target *t = &dfd_srv->targets[i + first];

		shell_print(sh, "\t\t\"%d\": { \"blob_addr\": 0x%04x, \"phase\": %d, "
			    "\"status\": %d, \"blob_status\": %d, \"progress\": %d, "
			    "\"img_idx\": %d }%s", i + first, t->blob.addr, t->phase, t->status,
			    t->blob.status, progress, t->img_idx, (i == cnt - 1) ? "" : ",");
	}
	shell_print(sh, "\t}\n}");

	return 0;
}

static int cmd_dfd_capabilities_get(const struct shell *sh, size_t argc, char *argv[])
{
	size_t size = 0;
	/* Remaining size */
	(void)bt_mesh_dfu_slot_foreach(slot_space_cb, &size);
	size = MIN(size, CONFIG_BT_MESH_DFD_SRV_SLOT_SPACE);

	shell_print(sh, "{ \"targets_max\": %d, \"slot_cnt\": %d, \"slot_max_size\": %d, "
		    "\"slot_space\": %d, \"remaining_space\": %d, \"oob_supported\": false }",
		    CONFIG_BT_MESH_DFD_SRV_TARGETS_MAX, CONFIG_BT_MESH_DFU_SLOT_CNT,
		    CONFIG_BT_MESH_DFD_SRV_SLOT_MAX_SIZE, CONFIG_BT_MESH_DFD_SRV_SLOT_SPACE,
		    CONFIG_BT_MESH_DFD_SRV_SLOT_SPACE - size);

	return 0;
}

static int cmd_dfd_get(const struct shell *sh, size_t argc, char *argv[])
{
	if (!mod && !bt_mesh_shell_mdl_first_get(BT_MESH_MODEL_ID_DFD_SRV, &mod)) {
		return -ENODEV;
	}

	struct bt_mesh_dfd_srv *dfd_srv = mod->user_data;

	print_dfd_status(sh, dfd_srv, BT_MESH_DFD_SUCCESS);

	return 0;
}

static int cmd_dfd_start(const struct shell *sh, size_t argc, char *argv[])
{
	if (!mod && !bt_mesh_shell_mdl_first_get(BT_MESH_MODEL_ID_DFD_SRV, &mod)) {
		return -ENODEV;
	}

	struct bt_mesh_dfd_srv *dfd_srv = mod->user_data;
	struct bt_mesh_dfd_start_params params;
	int err = 0;

	params.app_idx = shell_strtoul(argv[1], 0, &err);
	params.slot_idx = shell_strtoul(argv[2], 0, &err);
	if (argc > 3) {
		params.group = shell_strtoul(argv[3], 0, &err);
	} else {
		params.group = BT_MESH_ADDR_UNASSIGNED;
	}

	if (argc > 4) {
		params.apply = strcmp(argv[4], "true") ? false : true;
	} else {
		params.apply = true;
	}

	if (argc > 5) {
		params.ttl = shell_strtoul(argv[5], 0, &err);
	} else {
		params.ttl = BT_MESH_TTL_DEFAULT;
	}

	if (argc > 6) {
		params.timeout_base = shell_strtoul(argv[6], 0, &err);
	} else {
		params.timeout_base = 0U;
	}

	if (argc > 7) {
		params.xfer_mode = (enum bt_mesh_blob_xfer_mode)shell_strtoul(argv[7], 0, &err);
	} else {
		params.xfer_mode = BT_MESH_BLOB_XFER_MODE_PUSH;
	}

	if (err) {
		shell_warn(sh, "Unable to parse input string argument");
		return err;
	}

	enum bt_mesh_dfd_status status = bt_mesh_dfd_srv_start(dfd_srv, &params);

	print_dfd_status(sh, dfd_srv, status);
	if (status != BT_MESH_DFD_SUCCESS) {
		return -EINVAL;
	}

	return 0;
}

static int cmd_dfd_suspend(const struct shell *sh, size_t argc, char *argv[])
{
	if (!mod && !bt_mesh_shell_mdl_first_get(BT_MESH_MODEL_ID_DFD_SRV, &mod)) {
		return -ENODEV;
	}

	struct bt_mesh_dfd_srv *dfd_srv = mod->user_data;

	enum bt_mesh_dfd_status status = bt_mesh_dfd_srv_suspend(dfd_srv);

	print_dfd_status(sh, dfd_srv, status);
	if (status != BT_MESH_DFD_SUCCESS) {
		return -EINVAL;
	}

	return 0;
}

static int cmd_dfd_cancel(const struct shell *sh, size_t argc, char *argv[])
{
	if (!mod && !bt_mesh_shell_mdl_first_get(BT_MESH_MODEL_ID_DFD_SRV, &mod)) {
		return -ENODEV;
	}

	struct bt_mesh_dfd_srv *dfd_srv = mod->user_data;

	enum bt_mesh_dfd_status status = bt_mesh_dfd_srv_cancel(dfd_srv, NULL);

	print_dfd_status(sh, dfd_srv, status);
	if (status != BT_MESH_DFD_SUCCESS) {
		return -EINVAL;
	}

	return 0;
}

static int cmd_dfd_apply(const struct shell *sh, size_t argc, char *argv[])
{
	if (!mod && !bt_mesh_shell_mdl_first_get(BT_MESH_MODEL_ID_DFD_SRV, &mod)) {
		return -ENODEV;
	}

	struct bt_mesh_dfd_srv *dfd_srv = mod->user_data;

	enum bt_mesh_dfd_status status = bt_mesh_dfd_srv_apply(dfd_srv);

	print_dfd_status(sh, dfd_srv, status);
	if (status != BT_MESH_DFD_SUCCESS) {
		return -EINVAL;
	}

	return 0;
}

static int cmd_dfd_fw_get(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t fwid[CONFIG_BT_MESH_DFU_FWID_MAXLEN];
	size_t hexlen = strlen(argv[1]);
	size_t fwid_len = hex2bin(argv[1], hexlen, fwid, CONFIG_BT_MESH_DFU_FWID_MAXLEN);

	if (fwid_len != ((hexlen + 1) / 2)) {
		return -EINVAL;
	}

	const struct bt_mesh_dfu_slot *slot;
	int idx = bt_mesh_dfu_slot_get(fwid, fwid_len, &slot);

	if (idx >= 0 && bt_mesh_dfu_slot_is_valid(slot)) {
		print_fw_status(sh, BT_MESH_DFD_SUCCESS, idx, fwid, fwid_len);
	} else {
		print_fw_status(sh, BT_MESH_DFD_ERR_FW_NOT_FOUND, 0xffff, fwid, fwid_len);
		return -ENOENT;
	}

	return 0;
}

static int cmd_dfd_fw_get_by_idx(const struct shell *sh, size_t argc, char *argv[])
{
	int err = 0;
	uint16_t idx = shell_strtoul(argv[1], 0, &err);
	const struct bt_mesh_dfu_slot *slot = bt_mesh_dfu_slot_at(idx);

	if (err) {
		shell_warn(sh, "Unable to parse input string argument");
		return err;
	}

	if (slot && bt_mesh_dfu_slot_is_valid(slot)) {
		print_fw_status(sh, BT_MESH_DFD_SUCCESS, idx, slot->fwid, slot->fwid_len);
	} else {
		print_fw_status(sh, BT_MESH_DFD_ERR_FW_NOT_FOUND, idx, NULL, 0);
		return -ENOENT;
	}

	return 0;
}

static int cmd_dfd_fw_delete(const struct shell *sh, size_t argc, char *argv[])
{
	if (!mod && !bt_mesh_shell_mdl_first_get(BT_MESH_MODEL_ID_DFD_SRV, &mod)) {
		return -ENODEV;
	}

	struct bt_mesh_dfd_srv *dfd_srv = mod->user_data;

	uint8_t fwid_buf[CONFIG_BT_MESH_DFU_FWID_MAXLEN];
	size_t hexlen = strlen(argv[1]);
	size_t fwid_len = hex2bin(argv[1], hexlen, fwid_buf, CONFIG_BT_MESH_DFU_FWID_MAXLEN);

	if (fwid_len != ((hexlen + 1) / 2)) {
		return -EINVAL;
	}

	const uint8_t *fwid = &fwid_buf[0];

	enum bt_mesh_dfd_status status = bt_mesh_dfd_srv_fw_delete(dfd_srv,
								   &fwid_len, &fwid);

	print_fw_status(sh, status, 0xffff, fwid, fwid_len);

	if (status != BT_MESH_DFD_SUCCESS) {
		return -EINVAL;
	}

	return 0;
}

static int cmd_dfd_fw_delete_all(const struct shell *sh, size_t argc, char *argv[])
{
	if (!mod && !bt_mesh_shell_mdl_first_get(BT_MESH_MODEL_ID_DFD_SRV, &mod)) {
		return -ENODEV;
	}

	struct bt_mesh_dfd_srv *dfd_srv = mod->user_data;

	enum bt_mesh_dfd_status status = bt_mesh_dfd_srv_fw_delete_all(dfd_srv);

	print_fw_status(sh, status, 0xffff, NULL, 0);

	if (status != BT_MESH_DFD_SUCCESS) {
		return -EINVAL;
	}

	return 0;
}

BT_MESH_SHELL_MDL_INSTANCE_CMDS(instance_cmds, BT_MESH_MODEL_ID_DFD_SRV, mod);

SHELL_STATIC_SUBCMD_SET_CREATE(
	dfd_cmds,
	SHELL_CMD_ARG(receivers-add, NULL, "<Addr>,<FwIdx>[;<Addr>,<FwIdx>]...",
		      cmd_dfd_receivers_add, 2, 0),
	SHELL_CMD_ARG(receivers-delete-all, NULL, NULL, cmd_dfd_receivers_delete_all, 1, 0),
	SHELL_CMD_ARG(receivers-get, NULL, "<First> <Count>", cmd_dfd_receivers_get, 3, 0),
	SHELL_CMD_ARG(capabilities-get, NULL, NULL, cmd_dfd_capabilities_get, 1, 0),
	SHELL_CMD_ARG(get, NULL, NULL, cmd_dfd_get, 1, 0),
	SHELL_CMD_ARG(start, NULL,
		      "<AppKeyIdx> <SlotIdx> [<Group> [<PolicyApply> [<TTL> "
		      "[<TimeoutBase> [<XferMode>]]]]]",
		      cmd_dfd_start, 3, 5),
	SHELL_CMD_ARG(suspend, NULL, NULL, cmd_dfd_suspend, 1, 0),
	SHELL_CMD_ARG(cancel, NULL, NULL, cmd_dfd_cancel, 1, 0),
	SHELL_CMD_ARG(apply, NULL, NULL, cmd_dfd_apply, 1, 0),
	SHELL_CMD_ARG(fw-get, NULL, "<FwID>", cmd_dfd_fw_get, 2, 0),
	SHELL_CMD_ARG(fw-get-by-idx, NULL, "<Idx>", cmd_dfd_fw_get_by_idx, 2, 0),
	SHELL_CMD_ARG(fw-delete, NULL, "<FwID>", cmd_dfd_fw_delete, 2, 0),
	SHELL_CMD_ARG(fw-delete-all, NULL, NULL, cmd_dfd_fw_delete_all, 1, 0),
	SHELL_CMD(instance, &instance_cmds, "Instance commands", bt_mesh_shell_mdl_cmds_help),
	SHELL_SUBCMD_SET_END);

SHELL_SUBCMD_ADD((mesh, models), dfd, &dfd_cmds, "Distributor commands",
		 bt_mesh_shell_mdl_cmds_help, 1, 1);
