/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "dfu.h"

#include <stdlib.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/shell/shell.h>
#include <zephyr/bluetooth/mesh.h>
#include <zephyr/bluetooth/mesh/shell.h>
#include <zephyr/dfu/mcuboot.h>
#include <zephyr/storage/flash_map.h>

#include "utils.h"
#include "blob.h"
#include "../dfu_slot.h"

/***************************************************************************************************
 * Implementation of models' instances
 **************************************************************************************************/

extern const struct shell *bt_mesh_shell_ctx_shell;

#if defined(CONFIG_BT_MESH_SHELL_DFU_CLI)

static void dfu_cli_ended(struct bt_mesh_dfu_cli *cli,
			  enum bt_mesh_dfu_status reason)
{
	shell_print(bt_mesh_shell_ctx_shell, "DFU ended: %u", reason);
}

static void dfu_cli_applied(struct bt_mesh_dfu_cli *cli)
{
	shell_print(bt_mesh_shell_ctx_shell, "DFU applied.");
}

static void dfu_cli_lost_target(struct bt_mesh_dfu_cli *cli,
				struct bt_mesh_dfu_target *target)
{
	shell_print(bt_mesh_shell_ctx_shell, "DFU target lost: 0x%04x", target->blob.addr);
}

static void dfu_cli_confirmed(struct bt_mesh_dfu_cli *cli)
{
	shell_print(bt_mesh_shell_ctx_shell, "DFU confirmed");
}

const struct bt_mesh_dfu_cli_cb dfu_cli_cb = {
	.ended = dfu_cli_ended,
	.applied = dfu_cli_applied,
	.lost_target = dfu_cli_lost_target,
	.confirmed = dfu_cli_confirmed,
};

struct bt_mesh_dfu_cli bt_mesh_shell_dfu_cli = BT_MESH_DFU_CLI_INIT(&dfu_cli_cb);

#endif /* CONFIG_BT_MESH_SHELL_DFU_CLI */

#if defined(CONFIG_BT_MESH_SHELL_DFU_SRV)

struct shell_dfu_fwid {
	uint8_t type;
	struct mcuboot_img_sem_ver ver;
};

static struct bt_mesh_dfu_img dfu_imgs[] = { {
	.fwid = &((struct shell_dfu_fwid){ 0x01, { 1, 0, 0, 0 } }),
	.fwid_len = sizeof(struct shell_dfu_fwid),
} };

static int dfu_meta_check(struct bt_mesh_dfu_srv *srv,
			  const struct bt_mesh_dfu_img *img,
			  struct net_buf_simple *metadata,
			  enum bt_mesh_dfu_effect *effect)
{
	return 0;
}

static int dfu_start(struct bt_mesh_dfu_srv *srv,
		     const struct bt_mesh_dfu_img *img,
		     struct net_buf_simple *metadata,
		     const struct bt_mesh_blob_io **io)
{
	shell_print(bt_mesh_shell_ctx_shell, "DFU setup");

	*io = bt_mesh_shell_blob_io;

	return 0;
}

static void dfu_end(struct bt_mesh_dfu_srv *srv, const struct bt_mesh_dfu_img *img, bool success)
{
	if (!success) {
		shell_print(bt_mesh_shell_ctx_shell, "DFU failed");
		return;
	}

	if (!bt_mesh_shell_blob_valid) {
		bt_mesh_dfu_srv_rejected(srv);
		return;
	}

	bt_mesh_dfu_srv_verified(srv);
}

static int dfu_apply(struct bt_mesh_dfu_srv *srv,
		     const struct bt_mesh_dfu_img *img)
{
	if (!bt_mesh_shell_blob_valid) {
		return -EINVAL;
	}

	shell_print(bt_mesh_shell_ctx_shell, "Applying DFU transfer...");

	return 0;
}

static const struct bt_mesh_dfu_srv_cb dfu_handlers = {
	.check = dfu_meta_check,
	.start = dfu_start,
	.end = dfu_end,
	.apply = dfu_apply,
};

struct bt_mesh_dfu_srv bt_mesh_shell_dfu_srv =
	BT_MESH_DFU_SRV_INIT(&dfu_handlers, dfu_imgs, ARRAY_SIZE(dfu_imgs));

#endif /* CONFIG_BT_MESH_SHELL_DFU_SRV */

void bt_mesh_shell_dfu_cmds_init(void)
{
#if defined(CONFIG_BT_MESH_SHELL_DFU_SRV) && defined(CONFIG_BOOTLOADER_MCUBOOT)
	struct mcuboot_img_header img_header;

	int err = boot_read_bank_header(FIXED_PARTITION_ID(slot0_partition),
					&img_header, sizeof(img_header));
	if (!err) {
		struct shell_dfu_fwid *fwid =
			(struct shell_dfu_fwid *)dfu_imgs[0].fwid;

		fwid->ver = img_header.h.v1.sem_ver;

		boot_write_img_confirmed();
	}
#endif
}

/***************************************************************************************************
 * Shell Commands
 **************************************************************************************************/

#if defined(CONFIG_BT_MESH_SHELL_DFU_METADATA)

NET_BUF_SIMPLE_DEFINE_STATIC(dfu_comp_data, BT_MESH_TX_SDU_MAX);

static int cmd_dfu_comp_clear(const struct shell *sh, size_t argc, char *argv[])
{
	net_buf_simple_reset(&dfu_comp_data);
	return 0;
}

static int cmd_dfu_comp_add(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_buf_simple_state state;
	int err = 0;

	if (argc < 6) {
		return -EINVAL;
	}

	if (net_buf_simple_tailroom(&dfu_comp_data) < 10) {
		shell_print(sh, "Buffer is too small: %u",
			    net_buf_simple_tailroom(&dfu_comp_data));
		return -EMSGSIZE;
	}

	net_buf_simple_save(&dfu_comp_data, &state);

	for (size_t i = 1; i <= 5; i++) {
		net_buf_simple_add_le16(&dfu_comp_data, shell_strtoul(argv[i], 0, &err));
	}

	if (err) {
		net_buf_simple_restore(&dfu_comp_data, &state);
		shell_warn(sh, "Unable to parse input string argument");
		return err;
	}

	return 0;
}

static int cmd_dfu_comp_elem_add(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t sig_model_count;
	uint8_t vnd_model_count;
	struct net_buf_simple_state state;
	int err = 0;

	if (argc < 5) {
		return -EINVAL;
	}

	net_buf_simple_save(&dfu_comp_data, &state);

	sig_model_count = shell_strtoul(argv[2], 0, &err);
	vnd_model_count = shell_strtoul(argv[3], 0, &err);

	if (argc < 4 + sig_model_count + vnd_model_count * 2) {
		return -EINVAL;
	}

	if (net_buf_simple_tailroom(&dfu_comp_data) < 4 + sig_model_count * 2 +
	    vnd_model_count * 4) {
		shell_print(sh, "Buffer is too small: %u",
			    net_buf_simple_tailroom(&dfu_comp_data));
		return -EMSGSIZE;
	}

	net_buf_simple_add_le16(&dfu_comp_data, shell_strtoul(argv[1], 0, &err));
	net_buf_simple_add_u8(&dfu_comp_data, sig_model_count);
	net_buf_simple_add_u8(&dfu_comp_data, vnd_model_count);

	for (size_t i = 0; i < sig_model_count; i++) {
		net_buf_simple_add_le16(&dfu_comp_data, shell_strtoul(argv[4 + i], 0, &err));
	}

	for (size_t i = 0; i < vnd_model_count; i++) {
		size_t arg_i = 4 + sig_model_count + i * 2;

		net_buf_simple_add_le16(&dfu_comp_data, shell_strtoul(argv[arg_i], 0, &err));
		net_buf_simple_add_le16(&dfu_comp_data, shell_strtoul(argv[arg_i + 1], 0, &err));
	}

	if (err) {
		net_buf_simple_restore(&dfu_comp_data, &state);
		shell_warn(sh, "Unable to parse input string argument");
		return err;
	}

	return 0;
}

static int cmd_dfu_comp_hash_get(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t key[16] = {};
	uint32_t hash;
	int err;

	if (dfu_comp_data.len < 14) {
		shell_print(sh, "Composition data is not set");
		return -EINVAL;
	}

	if (argc > 1) {
		hex2bin(argv[1], strlen(argv[1]), key, sizeof(key));
	}

	shell_print(sh, "Composition data to be hashed:");
	shell_print(sh, "\tCID: 0x%04x", sys_get_le16(&dfu_comp_data.data[0]));
	shell_print(sh, "\tPID: 0x%04x", sys_get_le16(&dfu_comp_data.data[2]));
	shell_print(sh, "\tVID: 0x%04x", sys_get_le16(&dfu_comp_data.data[4]));
	shell_print(sh, "\tCPRL: %u", sys_get_le16(&dfu_comp_data.data[6]));
	shell_print(sh, "\tFeatures: 0x%x", sys_get_le16(&dfu_comp_data.data[8]));

	for (size_t i = 10; i < dfu_comp_data.len - 4;) {
		uint8_t sig_model_count = dfu_comp_data.data[i + 2];
		uint8_t vnd_model_count = dfu_comp_data.data[i + 3];

		shell_print(sh, "\tElem: %u", sys_get_le16(&dfu_comp_data.data[i]));
		shell_print(sh, "\t\tNumS: %u", sig_model_count);
		shell_print(sh, "\t\tNumV: %u", vnd_model_count);

		for (size_t j = 0; j < sig_model_count; j++) {
			shell_print(sh, "\t\tSIG Model ID: 0x%04x",
				    sys_get_le16(&dfu_comp_data.data[i + 4 + j * 2]));
		}

		for (size_t j = 0; j < vnd_model_count; j++) {
			size_t arg_i = i + 4 + sig_model_count * 2 + j * 4;

			shell_print(sh, "\t\tVnd Company ID: 0x%04x, Model ID: 0x%04x",
				    sys_get_le16(&dfu_comp_data.data[arg_i]),
				    sys_get_le16(&dfu_comp_data.data[arg_i + 2]));
		}

		i += 4 + sig_model_count * 2 + vnd_model_count * 4;
	}

	err = bt_mesh_dfu_metadata_comp_hash_get(&dfu_comp_data, key, &hash);
	if (err) {
		shell_print(sh, "Failed to compute composition data hash: %d\n", err);
		return err;
	}

	shell_print(sh, "Composition data hash: 0x%04x", hash);

	return 0;
}

static int cmd_dfu_metadata_encode(const struct shell *sh, size_t argc, char *argv[])
{
	char md_str[2 * CONFIG_BT_MESH_DFU_METADATA_MAXLEN + 1];
	uint8_t user_data[CONFIG_BT_MESH_DFU_METADATA_MAXLEN - 18];
	struct bt_mesh_dfu_metadata md;
	size_t len;
	int err = 0;

	NET_BUF_SIMPLE_DEFINE(buf, CONFIG_BT_MESH_DFU_METADATA_MAXLEN);

	if (argc < 9) {
		return -EINVAL;
	}

	md.fw_ver.major = shell_strtoul(argv[1], 0, &err);
	md.fw_ver.minor = shell_strtoul(argv[2], 0, &err);
	md.fw_ver.revision = shell_strtoul(argv[3], 0, &err);
	md.fw_ver.build_num = shell_strtoul(argv[4], 0, &err);
	md.fw_size = shell_strtoul(argv[5], 0, &err);
	md.fw_core_type = shell_strtoul(argv[6], 0, &err);
	md.comp_hash = shell_strtoul(argv[7], 0, &err);
	md.elems = shell_strtoul(argv[8], 0, &err);

	if (err) {
		shell_warn(sh, "Unable to parse input string argument");
		return err;
	}

	if (argc > 9) {
		if (sizeof(user_data) < strlen(argv[9]) / 2) {
			shell_print(sh, "User data is too big.");
			return -EINVAL;
		}

		md.user_data_len = hex2bin(argv[9], strlen(argv[9]), user_data, sizeof(user_data));
		md.user_data = user_data;
	} else {
		md.user_data_len = 0;
	}

	shell_print(sh, "Metadata to be encoded:");
	shell_print(sh, "\tVersion: %u.%u.%u+%u", md.fw_ver.major, md.fw_ver.minor,
		    md.fw_ver.revision, md.fw_ver.build_num);
	shell_print(sh, "\tSize: %u", md.fw_size);
	shell_print(sh, "\tCore Type: 0x%x", md.fw_core_type);
	shell_print(sh, "\tComposition data hash: 0x%x", md.comp_hash);
	shell_print(sh, "\tElements: %u", md.elems);

	if (argc > 9) {
		shell_print(sh, "\tUser data: %s", argv[10]);
	}

	shell_print(sh, "\tUser data length: %u", md.user_data_len);

	err = bt_mesh_dfu_metadata_encode(&md, &buf);
	if (err) {
		shell_print(sh, "Failed to encode metadata: %d", err);
		return err;
	}

	len = bin2hex(buf.data, buf.len, md_str, sizeof(md_str));
	md_str[len] = '\0';
	shell_print(sh, "Encoded metadata: %s", md_str);

	return 0;
}

#endif /* CONFIG_BT_MESH_SHELL_DFU_METADATA */

#if defined(CONFIG_BT_MESH_SHELL_DFU_SLOT)

static int cmd_dfu_slot_add(const struct shell *sh, size_t argc, char *argv[])
{
	const struct bt_mesh_dfu_slot *slot;
	size_t size;
	uint8_t fwid[CONFIG_BT_MESH_DFU_FWID_MAXLEN];
	size_t fwid_len = 0;
	uint8_t metadata[CONFIG_BT_MESH_DFU_METADATA_MAXLEN];
	size_t metadata_len = 0;
	const char *uri = "";
	int err = 0;

	size = shell_strtoul(argv[1], 0, &err);
	if (err) {
		shell_warn(sh, "Unable to parse input string argument");
		return err;
	}

	if (argc > 2) {
		fwid_len = hex2bin(argv[2], strlen(argv[2]), fwid,
				   sizeof(fwid));
	}

	if (argc > 3) {
		metadata_len = hex2bin(argv[3], strlen(argv[3]), metadata,
				       sizeof(metadata));
	}

	if (argc > 4) {
		uri = argv[4];
	}

	shell_print(sh, "Adding slot (size: %u)", size);

	slot = bt_mesh_dfu_slot_add(size, fwid, fwid_len, metadata,
				    metadata_len, uri, strlen(uri));
	if (!slot) {
		shell_print(sh, "Failed.");
		return 0;
	}

	bt_mesh_dfu_slot_valid_set(slot, true);

	shell_print(sh, "Slot added. ID: %u", bt_mesh_dfu_slot_idx_get(slot));

	return 0;
}

static int cmd_dfu_slot_del(const struct shell *sh, size_t argc, char *argv[])
{
	const struct bt_mesh_dfu_slot *slot;
	uint8_t idx;
	int err = 0;

	idx = shell_strtoul(argv[1], 0, &err);
	if (err) {
		shell_warn(sh, "Unable to parse input string argument");
		return err;
	}

	slot = bt_mesh_dfu_slot_at(idx);
	if (!slot) {
		shell_print(sh, "No slot at %u", idx);
		return 0;
	}

	err = bt_mesh_dfu_slot_del(slot);
	if (err) {
		shell_print(sh, "Failed deleting slot %u (err: %d)", idx,
			    err);
		return 0;
	}

	shell_print(sh, "Slot %u deleted.", idx);
	return 0;
}

static int cmd_dfu_slot_del_all(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	err = bt_mesh_dfu_slot_del_all();
	if (err) {
		shell_print(sh, "Failed deleting all slots (err: %d)", err);
		return 0;
	}

	shell_print(sh, "All slots deleted.");
	return 0;
}

static void slot_info_print(const struct shell *sh, const struct bt_mesh_dfu_slot *slot,
			    const uint8_t *idx)
{
	char fwid[2 * CONFIG_BT_MESH_DFU_FWID_MAXLEN + 1];
	char metadata[2 * CONFIG_BT_MESH_DFU_METADATA_MAXLEN + 1];
	char uri[CONFIG_BT_MESH_DFU_URI_MAXLEN + 1];
	size_t len;

	len = bin2hex(slot->fwid, slot->fwid_len, fwid, sizeof(fwid));
	fwid[len] = '\0';
	len = bin2hex(slot->metadata, slot->metadata_len, metadata,
		      sizeof(metadata));
	metadata[len] = '\0';
	memcpy(uri, slot->uri, slot->uri_len);
	uri[slot->uri_len] = '\0';

	if (idx != NULL) {
		shell_print(sh, "Slot %u:", *idx);
	} else {
		shell_print(sh, "Slot:");
	}
	shell_print(sh, "\tSize:     %u bytes", slot->size);
	shell_print(sh, "\tFWID:     %s", fwid);
	shell_print(sh, "\tMetadata: %s", metadata);
	shell_print(sh, "\tURI:      %s", uri);
}

static int cmd_dfu_slot_get(const struct shell *sh, size_t argc, char *argv[])
{
	const struct bt_mesh_dfu_slot *slot;
	uint8_t idx;
	int err = 0;

	idx = shell_strtoul(argv[1], 0, &err);
	if (err) {
		shell_warn(sh, "Unable to parse input string argument");
		return err;
	}

	slot = bt_mesh_dfu_slot_at(idx);
	if (!slot) {
		shell_print(sh, "No slot at %u", idx);
		return 0;
	}

	slot_info_print(sh, slot, &idx);
	return 0;
}

#endif /* defined(CONFIG_BT_MESH_SHELL_DFU_SLOT) */

#if defined(CONFIG_BT_MESH_SHELL_DFU_CLI)

static const struct bt_mesh_model *mod_cli;

static struct {
	struct bt_mesh_dfu_target targets[32];
	struct bt_mesh_blob_target_pull pull[32];
	size_t target_cnt;
	struct bt_mesh_blob_cli_inputs inputs;
} dfu_tx;

static void dfu_tx_prepare(void)
{
	sys_slist_init(&dfu_tx.inputs.targets);

	for (size_t i = 0; i < dfu_tx.target_cnt; i++) {
		/* Reset target context. */
		uint16_t addr = dfu_tx.targets[i].blob.addr;

		memset(&dfu_tx.targets[i].blob, 0, sizeof(struct bt_mesh_blob_target));
		memset(&dfu_tx.pull[i], 0, sizeof(struct bt_mesh_blob_target_pull));
		dfu_tx.targets[i].blob.addr = addr;
		dfu_tx.targets[i].blob.pull = &dfu_tx.pull[i];

		sys_slist_append(&dfu_tx.inputs.targets, &dfu_tx.targets[i].blob.n);
	}
}

static int cmd_dfu_target(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t img_idx;
	uint16_t addr;
	int err = 0;

	addr = shell_strtoul(argv[1], 0, &err);
	img_idx = shell_strtoul(argv[2], 0, &err);

	if (err) {
		shell_warn(sh, "Unable to parse input string argument");
		return err;
	}

	if (dfu_tx.target_cnt == ARRAY_SIZE(dfu_tx.targets)) {
		shell_print(sh, "No room.");
		return 0;
	}

	for (size_t i = 0; i < dfu_tx.target_cnt; i++) {
		if (dfu_tx.targets[i].blob.addr == addr) {
			shell_print(sh, "Target 0x%04x already exists", addr);
			return 0;
		}
	}

	dfu_tx.targets[dfu_tx.target_cnt].blob.addr = addr;
	dfu_tx.targets[dfu_tx.target_cnt].img_idx = img_idx;
	sys_slist_append(&dfu_tx.inputs.targets, &dfu_tx.targets[dfu_tx.target_cnt].blob.n);
	dfu_tx.target_cnt++;

	shell_print(sh, "Added target 0x%04x", addr);
	return 0;
}

static int cmd_dfu_targets_reset(const struct shell *sh, size_t argc, char *argv[])
{
	dfu_tx_prepare();
	return 0;
}

static int cmd_dfu_target_state(const struct shell *sh, size_t argc, char *argv[])
{
	struct bt_mesh_dfu_target_status rsp;
	struct bt_mesh_msg_ctx ctx = {
		.send_ttl = BT_MESH_TTL_DEFAULT,
		.net_idx = bt_mesh_shell_target_ctx.net_idx,
		.addr = bt_mesh_shell_target_ctx.dst,
		.app_idx = bt_mesh_shell_target_ctx.app_idx,
	};
	int err;

	if (!mod_cli && !bt_mesh_shell_mdl_first_get(BT_MESH_MODEL_ID_DFU_CLI, &mod_cli)) {
		return -ENODEV;
	}

	err = bt_mesh_dfu_cli_status_get((struct bt_mesh_dfu_cli *)mod_cli->user_data,
					 &ctx, &rsp);
	if (err) {
		shell_print(sh, "Failed getting target status (err: %d)",
			    err);
		return 0;
	}

	shell_print(sh, "Target 0x%04x:", bt_mesh_shell_target_ctx.dst);
	shell_print(sh, "\tStatus:     %u", rsp.status);
	shell_print(sh, "\tPhase:      %u", rsp.phase);
	if (rsp.phase != BT_MESH_DFU_PHASE_IDLE) {
		shell_print(sh, "\tEffect:       %u", rsp.effect);
		shell_print(sh, "\tImg Idx:      %u", rsp.img_idx);
		shell_print(sh, "\tTTL:          %u", rsp.ttl);
		shell_print(sh, "\tTimeout base: %u", rsp.timeout_base);
	}

	return 0;
}

static enum bt_mesh_dfu_iter dfu_img_cb(struct bt_mesh_dfu_cli *cli,
					struct bt_mesh_msg_ctx *ctx,
					uint8_t idx, uint8_t total,
					const struct bt_mesh_dfu_img *img,
					void *cb_data)
{
	char fwid[2 * CONFIG_BT_MESH_DFU_FWID_MAXLEN + 1];
	size_t len;

	len = bin2hex(img->fwid, img->fwid_len, fwid, sizeof(fwid));
	fwid[len] = '\0';

	shell_print(bt_mesh_shell_ctx_shell, "Image %u:", idx);
	shell_print(bt_mesh_shell_ctx_shell, "\tFWID: %s", fwid);
	if (img->uri) {
		shell_print(bt_mesh_shell_ctx_shell, "\tURI:  %s", img->uri);
	}

	return BT_MESH_DFU_ITER_CONTINUE;
}

static int cmd_dfu_target_imgs(const struct shell *sh, size_t argc, char *argv[])
{
	struct bt_mesh_msg_ctx ctx = {
		.send_ttl = BT_MESH_TTL_DEFAULT,
		.net_idx = bt_mesh_shell_target_ctx.net_idx,
		.addr = bt_mesh_shell_target_ctx.dst,
		.app_idx = bt_mesh_shell_target_ctx.app_idx,
	};
	uint8_t img_cnt = 0xff;
	int err = 0;

	if (!mod_cli && !bt_mesh_shell_mdl_first_get(BT_MESH_MODEL_ID_DFU_CLI, &mod_cli)) {
		return -ENODEV;
	}

	if (argc == 2) {
		img_cnt = shell_strtoul(argv[1], 0, &err);
		if (err) {
			shell_warn(sh, "Unable to parse input string argument");
			return err;
		}
	}

	shell_print(sh, "Requesting DFU images in 0x%04x", bt_mesh_shell_target_ctx.dst);

	err = bt_mesh_dfu_cli_imgs_get((struct bt_mesh_dfu_cli *)mod_cli->user_data,
				       &ctx, dfu_img_cb, NULL, img_cnt);
	if (err) {
		shell_print(sh, "Request failed (err: %d)", err);
	}

	return 0;
}

static int cmd_dfu_target_check(const struct shell *sh, size_t argc, char *argv[])
{
	struct bt_mesh_dfu_metadata_status rsp;
	const struct bt_mesh_dfu_slot *slot;
	struct bt_mesh_msg_ctx ctx = {
		.send_ttl = BT_MESH_TTL_DEFAULT,
		.net_idx = bt_mesh_shell_target_ctx.net_idx,
		.addr = bt_mesh_shell_target_ctx.dst,
		.app_idx = bt_mesh_shell_target_ctx.app_idx,
	};
	uint8_t slot_idx, img_idx;
	int err = 0;

	if (!mod_cli && !bt_mesh_shell_mdl_first_get(BT_MESH_MODEL_ID_DFU_CLI, &mod_cli)) {
		return -ENODEV;
	}

	slot_idx = shell_strtoul(argv[1], 0, &err);
	img_idx = shell_strtoul(argv[2], 0, &err);

	if (err) {
		shell_warn(sh, "Unable to parse input string argument");
		return err;
	}

	slot = bt_mesh_dfu_slot_at(slot_idx);
	if (!slot) {
		shell_print(sh, "No image in slot %u", slot_idx);
		return 0;
	}

	err = bt_mesh_dfu_cli_metadata_check((struct bt_mesh_dfu_cli *)mod_cli->user_data,
					     &ctx, img_idx, slot, &rsp);
	if (err) {
		shell_print(sh, "Metadata check failed. err: %d", err);
		return 0;
	}

	shell_print(sh, "Slot %u check for 0x%04x image %u:", slot_idx,
		    bt_mesh_shell_target_ctx.dst, img_idx);
	shell_print(sh, "\tStatus: %u", rsp.status);
	shell_print(sh, "\tEffect: 0x%x", rsp.effect);

	return 0;
}

static int cmd_dfu_send(const struct shell *sh, size_t argc, char *argv[])
{
	struct bt_mesh_dfu_cli_xfer_blob_params blob_params;
	struct bt_mesh_dfu_cli_xfer xfer = { 0 };
	uint8_t slot_idx;
	uint16_t group;
	int err = 0;

	if (!mod_cli && !bt_mesh_shell_mdl_first_get(BT_MESH_MODEL_ID_DFU_CLI, &mod_cli)) {
		return -ENODEV;
	}

	slot_idx = shell_strtoul(argv[1], 0, &err);
	if (argc > 2) {
		group = shell_strtoul(argv[2], 0, &err);
	} else {
		group = BT_MESH_ADDR_UNASSIGNED;
	}

	if (argc > 3) {
		xfer.mode = shell_strtoul(argv[3], 0, &err);
	} else {
		xfer.mode = BT_MESH_BLOB_XFER_MODE_PUSH;
	}

	if (argc > 5) {
		blob_params.block_size_log = shell_strtoul(argv[4], 0, &err);
		blob_params.chunk_size = shell_strtoul(argv[5], 0, &err);
		xfer.blob_params = &blob_params;
	} else {
		xfer.blob_params = NULL;
	}

	if (err) {
		shell_warn(sh, "Unable to parse input string argument");
		return err;
	}

	if (!dfu_tx.target_cnt) {
		shell_print(sh, "No targets.");
		return 0;
	}

	xfer.slot = bt_mesh_dfu_slot_at(slot_idx);
	if (!xfer.slot) {
		shell_print(sh, "No image in slot %u", slot_idx);
		return 0;
	}

	shell_print(sh, "Starting DFU from slot %u (%u targets)", slot_idx,
		    dfu_tx.target_cnt);

	dfu_tx.inputs.group = group;
	dfu_tx.inputs.app_idx = bt_mesh_shell_target_ctx.app_idx;
	dfu_tx.inputs.ttl = BT_MESH_TTL_DEFAULT;

	err = bt_mesh_dfu_cli_send((struct bt_mesh_dfu_cli *)mod_cli->user_data,
				   &dfu_tx.inputs, bt_mesh_shell_blob_io, &xfer);
	if (err) {
		shell_print(sh, "Failed (err: %d)", err);
		return 0;
	}
	return 0;
}

static int cmd_dfu_tx_cancel(const struct shell *sh, size_t argc, char *argv[])
{
	struct bt_mesh_msg_ctx ctx = {
		.send_ttl = BT_MESH_TTL_DEFAULT,
		.net_idx = bt_mesh_shell_target_ctx.net_idx,
		.addr = bt_mesh_shell_target_ctx.dst,
		.app_idx = bt_mesh_shell_target_ctx.app_idx,
	};
	int err = 0;

	if (!mod_cli && !bt_mesh_shell_mdl_first_get(BT_MESH_MODEL_ID_DFU_CLI, &mod_cli)) {
		return -ENODEV;
	}

	if (argc == 2) {
		ctx.addr = shell_strtoul(argv[1], 0, &err);
		if (err) {
			shell_warn(sh, "Unable to parse input string argument");
			return err;
		}

		shell_print(sh, "Cancelling DFU for 0x%04x", ctx.addr);
	} else {
		shell_print(sh, "Cancelling DFU");
	}

	err = bt_mesh_dfu_cli_cancel((struct bt_mesh_dfu_cli *)mod_cli->user_data,
				     (argc == 2) ? &ctx : NULL);
	if (err) {
		shell_print(sh, "Failed (err: %d)", err);
	}

	return 0;
}

static int cmd_dfu_apply(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	if (!mod_cli && !bt_mesh_shell_mdl_first_get(BT_MESH_MODEL_ID_DFU_CLI, &mod_cli)) {
		return -ENODEV;
	}

	shell_print(sh, "Applying DFU");

	err = bt_mesh_dfu_cli_apply((struct bt_mesh_dfu_cli *)mod_cli->user_data);
	if (err) {
		shell_print(sh, "Failed (err: %d)", err);
	}

	return 0;
}

static int cmd_dfu_confirm(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	if (!mod_cli && !bt_mesh_shell_mdl_first_get(BT_MESH_MODEL_ID_DFU_CLI, &mod_cli)) {
		return -ENODEV;
	}

	shell_print(sh, "Confirming DFU");

	err = bt_mesh_dfu_cli_confirm((struct bt_mesh_dfu_cli *)mod_cli->user_data);
	if (err) {
		shell_print(sh, "Failed (err: %d)", err);
	}

	return 0;
}

static int cmd_dfu_suspend(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	if (!mod_cli && !bt_mesh_shell_mdl_first_get(BT_MESH_MODEL_ID_DFU_CLI, &mod_cli)) {
		return -ENODEV;
	}

	shell_print(sh, "Suspending DFU");

	err = bt_mesh_dfu_cli_suspend((struct bt_mesh_dfu_cli *)mod_cli->user_data);
	if (err) {
		shell_print(sh, "Failed (err: %d)", err);
	}

	return 0;
}

static int cmd_dfu_resume(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	if (!mod_cli && !bt_mesh_shell_mdl_first_get(BT_MESH_MODEL_ID_DFU_CLI, &mod_cli)) {
		return -ENODEV;
	}

	shell_print(sh, "Resuming DFU");

	err = bt_mesh_dfu_cli_resume((struct bt_mesh_dfu_cli *)mod_cli->user_data);
	if (err) {
		shell_print(sh, "Failed (err: %d)", err);
	}

	return 0;
}

static int cmd_dfu_tx_progress(const struct shell *sh, size_t argc, char *argv[])
{
	if (!mod_cli && !bt_mesh_shell_mdl_first_get(BT_MESH_MODEL_ID_DFU_CLI, &mod_cli)) {
		return -ENODEV;
	}

	shell_print(sh, "DFU progress: %u %%",
		    bt_mesh_dfu_cli_progress((struct bt_mesh_dfu_cli *)mod_cli->user_data));
	return 0;
}

#endif /* CONFIG_BT_MESH_SHELL_DFU_CLI */

#if defined(CONFIG_BT_MESH_SHELL_DFU_SRV)

static const struct bt_mesh_model *mod_srv;

static int cmd_dfu_applied(const struct shell *sh, size_t argc, char *argv[])
{
	if (!mod_srv && !bt_mesh_shell_mdl_first_get(BT_MESH_MODEL_ID_DFU_SRV, &mod_srv)) {
		return -ENODEV;
	}

	bt_mesh_dfu_srv_applied((struct bt_mesh_dfu_srv *)mod_srv->user_data);
	return 0;
}

static int cmd_dfu_rx_cancel(const struct shell *sh, size_t argc, char *argv[])
{
	if (!mod_srv && !bt_mesh_shell_mdl_first_get(BT_MESH_MODEL_ID_DFU_SRV, &mod_srv)) {
		return -ENODEV;
	}

	bt_mesh_dfu_srv_cancel((struct bt_mesh_dfu_srv *)mod_srv->user_data);
	return 0;
}

static int cmd_dfu_rx_progress(const struct shell *sh, size_t argc, char *argv[])
{
	if (!mod_srv && !bt_mesh_shell_mdl_first_get(BT_MESH_MODEL_ID_DFU_SRV, &mod_srv)) {
		return -ENODEV;
	}

	shell_print(sh, "DFU progress: %u %%",
		    bt_mesh_dfu_srv_progress((struct bt_mesh_dfu_srv *)mod_srv->user_data));
	return 0;
}

#endif /* CONFIG_BT_MESH_SHELL_DFU_SRV */

#if defined(CONFIG_BT_MESH_SHELL_DFU_CLI)
BT_MESH_SHELL_MDL_INSTANCE_CMDS(cli_instance_cmds, BT_MESH_MODEL_ID_DFU_CLI, mod_cli);
#endif

#if defined(CONFIG_BT_MESH_SHELL_DFU_SRV)
BT_MESH_SHELL_MDL_INSTANCE_CMDS(srv_instance_cmds, BT_MESH_MODEL_ID_DFU_SRV, mod_srv);
#endif

#if defined(CONFIG_BT_MESH_SHELL_DFU_METADATA)
SHELL_STATIC_SUBCMD_SET_CREATE(
	dfu_metadata_cmds,
	SHELL_CMD_ARG(comp-clear, NULL, NULL, cmd_dfu_comp_clear, 1, 0),
	SHELL_CMD_ARG(comp-add, NULL, "<CID> <ProductID> <VendorID> <Crpl> <Features>",
		      cmd_dfu_comp_add, 6, 0),
	SHELL_CMD_ARG(comp-elem-add, NULL, "<Loc> <NumS> <NumV> "
		      "{<SigMID>|<VndCID> <VndMID>}...",
		      cmd_dfu_comp_elem_add, 5, 10),
	SHELL_CMD_ARG(comp-hash-get, NULL, "[<Key>]", cmd_dfu_comp_hash_get, 1, 1),
	SHELL_CMD_ARG(metadata-encode, NULL, "<Major> <Minor> <Rev> <BuildNum> <Size> "
		      "<CoreType> <Hash> <Elems> [<UserData>]",
		      cmd_dfu_metadata_encode, 9, 1),
	SHELL_SUBCMD_SET_END);
#endif

#if defined(CONFIG_BT_MESH_SHELL_DFU_SLOT)
SHELL_STATIC_SUBCMD_SET_CREATE(
	dfu_slot_cmds,
	SHELL_CMD_ARG(add, NULL,
		      "<Size> [<FwID> [<Metadata> [<URI>]]]",
		      cmd_dfu_slot_add, 2, 3),
	SHELL_CMD_ARG(del, NULL, "<SlotIdx>", cmd_dfu_slot_del, 2, 0),
	SHELL_CMD_ARG(del-all, NULL, NULL, cmd_dfu_slot_del_all, 1, 0),
	SHELL_CMD_ARG(get, NULL, "<SlotIdx>", cmd_dfu_slot_get, 2, 0),
	SHELL_SUBCMD_SET_END);
#endif

#if defined(CONFIG_BT_MESH_SHELL_DFU_CLI)
SHELL_STATIC_SUBCMD_SET_CREATE(
	dfu_cli_cmds,
	/* DFU Client Model Operations */
	SHELL_CMD_ARG(target, NULL, "<Addr> <ImgIdx>", cmd_dfu_target, 3,
		      0),
	SHELL_CMD_ARG(targets-reset, NULL, NULL, cmd_dfu_targets_reset, 1, 0),
	SHELL_CMD_ARG(target-state, NULL, NULL, cmd_dfu_target_state, 1, 0),
	SHELL_CMD_ARG(target-imgs, NULL, "[<MaxCount>]",
		      cmd_dfu_target_imgs, 1, 1),
	SHELL_CMD_ARG(target-check, NULL, "<SlotIdx> <TargetImgIdx>",
		      cmd_dfu_target_check, 3, 0),
	SHELL_CMD_ARG(send, NULL, "<SlotIdx>  [<Group> "
		      "[<Mode(push, pull)> [<BlockSizeLog> <ChunkSize>]]]", cmd_dfu_send, 2, 4),
	SHELL_CMD_ARG(cancel, NULL, "[<Addr>]", cmd_dfu_tx_cancel, 1, 1),
	SHELL_CMD_ARG(apply, NULL, NULL, cmd_dfu_apply, 0, 0),
	SHELL_CMD_ARG(confirm, NULL, NULL, cmd_dfu_confirm, 0, 0),
	SHELL_CMD_ARG(suspend, NULL, NULL, cmd_dfu_suspend, 0, 0),
	SHELL_CMD_ARG(resume, NULL, NULL, cmd_dfu_resume, 0, 0),
	SHELL_CMD_ARG(progress, NULL, NULL, cmd_dfu_tx_progress, 1, 0),
	SHELL_CMD(instance, &cli_instance_cmds, "Instance commands", bt_mesh_shell_mdl_cmds_help),
	SHELL_SUBCMD_SET_END);
#endif

#if defined(CONFIG_BT_MESH_SHELL_DFU_SRV)
SHELL_STATIC_SUBCMD_SET_CREATE(
	dfu_srv_cmds,
	SHELL_CMD_ARG(applied, NULL, NULL, cmd_dfu_applied, 1, 0),
	SHELL_CMD_ARG(rx-cancel, NULL, NULL, cmd_dfu_rx_cancel, 1, 0),
	SHELL_CMD_ARG(progress, NULL, NULL, cmd_dfu_rx_progress, 1, 0),
	SHELL_CMD(instance, &srv_instance_cmds, "Instance commands", bt_mesh_shell_mdl_cmds_help),
	SHELL_SUBCMD_SET_END);
#endif

SHELL_STATIC_SUBCMD_SET_CREATE(
	dfu_cmds,
#if defined(CONFIG_BT_MESH_SHELL_DFU_METADATA)
	SHELL_CMD(metadata, &dfu_metadata_cmds, "Metadata commands", bt_mesh_shell_mdl_cmds_help),
#endif
#if defined(CONFIG_BT_MESH_SHELL_DFU_SLOT)
	SHELL_CMD(slot, &dfu_slot_cmds, "Slot commands", bt_mesh_shell_mdl_cmds_help),
#endif
#if defined(CONFIG_BT_MESH_SHELL_DFU_CLI)
	SHELL_CMD(cli, &dfu_cli_cmds, "DFU Cli commands", bt_mesh_shell_mdl_cmds_help),
#endif
#if defined(CONFIG_BT_MESH_SHELL_DFU_SRV)
	SHELL_CMD(srv, &dfu_srv_cmds, "DFU Srv commands", bt_mesh_shell_mdl_cmds_help),
#endif
	SHELL_SUBCMD_SET_END);

SHELL_SUBCMD_ADD((mesh, models), dfu, &dfu_cmds, "DFU models commands",
		 bt_mesh_shell_mdl_cmds_help, 1, 1);
