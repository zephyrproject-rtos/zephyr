/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/bluetooth/mesh.h>
#include <common/bt_str.h>
#include "dfu_slot.h"
#include "dfd.h"
#include "dfu.h"
#include "dfd_srv_internal.h"
#include "net.h"
#include "transport.h"

#define LOG_LEVEL CONFIG_BT_MESH_DFU_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_mesh_dfd_srv);

#define ERASE_BLOCK_SIZE DT_PROP(DT_CHOSEN(zephyr_flash), erase_block_size)

#define DFD_UPLOAD_STATUS_MSG_MAXLEN (5 + CONFIG_BT_MESH_DFU_FWID_MAXLEN)

BUILD_ASSERT((DFD_UPLOAD_STATUS_MSG_MAXLEN + BT_MESH_MODEL_OP_LEN(BT_MESH_DFD_OP_UPLOAD_STATUS) +
	      BT_MESH_MIC_SHORT) <= BT_MESH_TX_SDU_MAX,
	     "The Firmware Distribution Upload Status message does not fit into the maximum "
	     "outgoing SDU size.");

#define DFD_UPLOAD_START_MSG_MAXLEN (16 + CONFIG_BT_MESH_DFU_FWID_MAXLEN + \
				     CONFIG_BT_MESH_DFU_METADATA_MAXLEN)

BUILD_ASSERT((DFD_UPLOAD_START_MSG_MAXLEN + BT_MESH_MODEL_OP_LEN(BT_MESH_DFD_OP_UPLOAD_START) +
	      BT_MESH_MIC_SHORT) <= BT_MESH_RX_SDU_MAX,
	     "The Firmware Distribution Upload Start message does not fit into the maximum "
	     "incoming SDU size.");

#define DFD_RECEIVERS_LIST_MSG_MAXLEN (4 + CONFIG_BT_MESH_DFD_SRV_TARGETS_MAX * 5)

BUILD_ASSERT((DFD_RECEIVERS_LIST_MSG_MAXLEN + BT_MESH_MODEL_OP_LEN(BT_MESH_DFD_OP_RECEIVERS_LIST) +
	      BT_MESH_MIC_SHORT) <= BT_MESH_TX_SDU_MAX,
	     "The Firmware Distribution Receivers List message does not fit into the maximum "
	     "outgoing SDU size.");

#define DFD_RECEIVERS_ADD_MSG_MAXLEN (CONFIG_BT_MESH_DFD_SRV_TARGETS_MAX * 3)

BUILD_ASSERT((DFD_RECEIVERS_ADD_MSG_MAXLEN + BT_MESH_MODEL_OP_LEN(BT_MESH_DFD_OP_RECEIVERS_ADD) +
	      BT_MESH_MIC_SHORT) <= BT_MESH_RX_SDU_MAX,
	     "The Firmware Distribution Receivers Add message does not fit into the maximum "
	     "incoming SDU size.");

struct slot_search_ctx {
	off_t offset;
	size_t size;
	bool failed;
};

static void dfd_phase_set(struct bt_mesh_dfd_srv *srv,
			  enum bt_mesh_dfd_phase new_phase)
{
	srv->phase = new_phase;

	if (srv->cb && srv->cb->phase) {
		srv->cb->phase(srv, srv->phase);
	}
}

static struct bt_mesh_dfu_target *target_get(struct bt_mesh_dfd_srv *srv,
					     uint16_t addr)
{
	for (int i = 0; i < srv->target_cnt; ++i) {
		if (addr == srv->targets[i].blob.addr) {
			return &srv->targets[i];
		}
	}

	return NULL;
}

static bool is_busy(const struct bt_mesh_dfd_srv *srv)
{
	return srv->phase == BT_MESH_DFD_PHASE_TRANSFER_ACTIVE ||
	       srv->phase == BT_MESH_DFD_PHASE_TRANSFER_SUCCESS ||
	       srv->phase == BT_MESH_DFD_PHASE_APPLYING_UPDATE;
}

static bool upload_is_busy(const struct bt_mesh_dfd_srv *srv)
{
	return bt_mesh_blob_srv_is_busy(&srv->upload.blob) ||
	       srv->upload.phase == BT_MESH_DFD_UPLOAD_PHASE_TRANSFER_ACTIVE;
}

static int slot_del(struct bt_mesh_dfd_srv *srv, const struct bt_mesh_dfu_slot *slot)
{
	if (srv->cb && srv->cb->del) {
		srv->cb->del(srv, slot);
	}

	return bt_mesh_dfu_slot_del(slot);
}

static void receivers_status_rsp(struct bt_mesh_dfd_srv *srv,
				 struct bt_mesh_msg_ctx *ctx,
				 enum bt_mesh_dfd_status status)
{
	BT_MESH_MODEL_BUF_DEFINE(buf, BT_MESH_DFD_OP_RECEIVERS_STATUS, 3);
	bt_mesh_model_msg_init(&buf, BT_MESH_DFD_OP_RECEIVERS_STATUS);

	net_buf_simple_add_u8(&buf, status);
	net_buf_simple_add_le16(&buf, srv->target_cnt);

	bt_mesh_model_send(srv->mod, ctx, &buf, NULL, NULL);
}

static int handle_receivers_add(struct bt_mesh_model *mod, struct bt_mesh_msg_ctx *ctx,
				struct net_buf_simple *buf)
{
	enum bt_mesh_dfd_status status = BT_MESH_DFD_SUCCESS;
	struct bt_mesh_dfd_srv *srv = mod->user_data;

	if (buf->len % 3) {
		return -EINVAL;
	}

	if (bt_mesh_dfu_cli_is_busy(&srv->dfu)) {
		receivers_status_rsp(srv, ctx,
				     BT_MESH_DFD_ERR_BUSY_WITH_DISTRIBUTION);
		return 0;
	}

	while (buf->len >= 3 && status == BT_MESH_DFD_SUCCESS) {
		uint8_t img_idx;
		uint16_t addr;

		addr = net_buf_simple_pull_le16(buf);
		img_idx = net_buf_simple_pull_u8(buf);

		status = bt_mesh_dfd_srv_receiver_add(srv, addr, img_idx);
	}

	receivers_status_rsp(srv, ctx, status);

	return 0;
}

static int handle_receivers_delete_all(struct bt_mesh_model *mod, struct bt_mesh_msg_ctx *ctx,
				       struct net_buf_simple *buf)
{
	struct bt_mesh_dfd_srv *srv = mod->user_data;

	receivers_status_rsp(srv, ctx, bt_mesh_dfd_srv_receivers_delete_all(srv));

	return 0;
}

static int handle_receivers_get(struct bt_mesh_model *mod, struct bt_mesh_msg_ctx *ctx,
				struct net_buf_simple *buf)
{
	struct bt_mesh_dfd_srv *srv = mod->user_data;
	uint16_t first, cnt;
	uint8_t progress;
	int i;

	first = net_buf_simple_pull_le16(buf);
	cnt = net_buf_simple_pull_le16(buf);
	if (cnt == 0) {
		return -EINVAL;
	}

	/* Create a buffer that can fit the full target list, maxing out at TX_SDU_MAX: */
	NET_BUF_SIMPLE_DEFINE(
		rsp, BT_MESH_MODEL_BUF_LEN(BT_MESH_DFD_OP_RECEIVERS_LIST,
					   DFD_RECEIVERS_LIST_MSG_MAXLEN));
	bt_mesh_model_msg_init(&rsp, BT_MESH_DFD_OP_RECEIVERS_LIST);

	net_buf_simple_add_le16(&rsp, srv->target_cnt);
	net_buf_simple_add_le16(&rsp, first);

	cnt = MIN(cnt, srv->target_cnt - first);
	progress = bt_mesh_dfu_cli_progress(&srv->dfu) / 2;

	for (i = 0; i < cnt && net_buf_simple_tailroom(&rsp) >= 5 + BT_MESH_MIC_SHORT; i++) {
		const struct bt_mesh_dfu_target *t = &srv->targets[i + first];

		net_buf_simple_add_le32(
			&rsp, ((t->blob.addr & BIT_MASK(15)) |
			       ((t->phase & BIT_MASK(4)) << 15U) |
			       ((t->status & BIT_MASK(3)) << 19U) |
			       ((t->blob.status & BIT_MASK(4)) << 22U) |
			       ((progress & BIT_MASK(6)) << 26U)));
		net_buf_simple_add_u8(&rsp, t->img_idx);
	}

	bt_mesh_model_send(srv->mod, ctx, &rsp, NULL, NULL);

	return 0;
}

static enum bt_mesh_dfu_iter slot_space_cb(const struct bt_mesh_dfu_slot *slot,
					   void *user_data)
{
	size_t *total = user_data;

	*total += slot->size;

	return BT_MESH_DFU_ITER_CONTINUE;
}

static int handle_capabilities_get(struct bt_mesh_model *mod, struct bt_mesh_msg_ctx *ctx,
				   struct net_buf_simple *buf)
{
	size_t size = 0;

	BT_MESH_MODEL_BUF_DEFINE(rsp, BT_MESH_DFD_OP_CAPABILITIES_STATUS, 17);
	bt_mesh_model_msg_init(&rsp, BT_MESH_DFD_OP_CAPABILITIES_STATUS);

	net_buf_simple_add_le16(&rsp, CONFIG_BT_MESH_DFD_SRV_TARGETS_MAX);
	net_buf_simple_add_le16(&rsp, CONFIG_BT_MESH_DFU_SLOT_CNT);
	net_buf_simple_add_le32(&rsp, CONFIG_BT_MESH_DFD_SRV_SLOT_MAX_SIZE);
	net_buf_simple_add_le32(&rsp, CONFIG_BT_MESH_DFD_SRV_SLOT_SPACE);

	/* Remaining size */
	(void)bt_mesh_dfu_slot_foreach(slot_space_cb, &size);
	size = MIN(size, CONFIG_BT_MESH_DFD_SRV_SLOT_SPACE);

	net_buf_simple_add_le32(&rsp, CONFIG_BT_MESH_DFD_SRV_SLOT_SPACE - size);
	net_buf_simple_add_u8(&rsp, 0U); /* OOB retrieval not supported */

	bt_mesh_model_send(mod, ctx, &rsp, NULL, NULL);

	return 0;
}

static void status_rsp(struct bt_mesh_dfd_srv *srv, struct bt_mesh_msg_ctx *ctx,
		       enum bt_mesh_dfd_status status)
{
	BT_MESH_MODEL_BUF_DEFINE(rsp, BT_MESH_DFD_OP_STATUS, 12);
	bt_mesh_model_msg_init(&rsp, BT_MESH_DFD_OP_STATUS);

	net_buf_simple_add_u8(&rsp, status);
	net_buf_simple_add_u8(&rsp, srv->phase);

	if (srv->phase == BT_MESH_DFD_PHASE_IDLE || !srv->dfu.xfer.slot) {
		bt_mesh_model_send(srv->mod, ctx, &rsp, NULL, NULL);
		return;
	}

	net_buf_simple_add_le16(&rsp, srv->inputs.group);
	net_buf_simple_add_le16(&rsp, srv->inputs.app_idx);
	net_buf_simple_add_u8(&rsp, srv->inputs.ttl);
	net_buf_simple_add_le16(&rsp, srv->inputs.timeout_base);
	net_buf_simple_add_u8(&rsp, ((srv->dfu.xfer.blob.mode & BIT_MASK(2)) |
				     ((srv->apply & BIT_MASK(1)) << 2)));
	net_buf_simple_add_le16(&rsp, srv->slot_idx);

	bt_mesh_model_send(srv->mod, ctx, &rsp, NULL, NULL);
}

static int handle_get(struct bt_mesh_model *mod, struct bt_mesh_msg_ctx *ctx,
		      struct net_buf_simple *buf)
{
	struct bt_mesh_dfd_srv *srv = mod->user_data;

	status_rsp(srv, ctx, BT_MESH_DFD_SUCCESS);

	return 0;
}

static int handle_start(struct bt_mesh_model *mod, struct bt_mesh_msg_ctx *ctx,
			struct net_buf_simple *buf)
{
	struct bt_mesh_dfd_srv *srv = mod->user_data;
	struct bt_mesh_dfd_start_params params;
	uint8_t byte;

	params.app_idx = net_buf_simple_pull_le16(buf);
	params.ttl = net_buf_simple_pull_u8(buf);
	params.timeout_base = net_buf_simple_pull_le16(buf);
	byte = net_buf_simple_pull_u8(buf);
	params.xfer_mode = byte & BIT_MASK(2);
	params.apply = (byte >> 2U) & BIT_MASK(1);
	params.slot_idx = net_buf_simple_pull_le16(buf);

	if (buf->len == 16) {
		/* TODO: Virtual addresses not supported. */
		status_rsp(srv, ctx, BT_MESH_DFD_ERR_INTERNAL);
		return 0;
	}

	if (buf->len != 2) {
		return -EINVAL;
	}

	params.group = net_buf_simple_pull_le16(buf);

	status_rsp(srv, ctx, bt_mesh_dfd_srv_start(srv, &params));

	return 0;
}

static int handle_suspend(struct bt_mesh_model *mod,
			   struct bt_mesh_msg_ctx *ctx,
			   struct net_buf_simple *buf)
{
	struct bt_mesh_dfd_srv *srv = mod->user_data;

	status_rsp(srv, ctx, bt_mesh_dfd_srv_suspend(srv));

	return 0;
}

static int handle_cancel(struct bt_mesh_model *mod, struct bt_mesh_msg_ctx *ctx,
			 struct net_buf_simple *buf)
{
	struct bt_mesh_dfd_srv *srv = mod->user_data;

	bt_mesh_dfd_srv_cancel(srv, ctx);

	return 0;
}

static int handle_apply(struct bt_mesh_model *mod, struct bt_mesh_msg_ctx *ctx,
			struct net_buf_simple *buf)
{
	struct bt_mesh_dfd_srv *srv = mod->user_data;

	status_rsp(srv, ctx, bt_mesh_dfd_srv_apply(srv));

	return 0;
}

static void upload_status_rsp(struct bt_mesh_dfd_srv *srv,
			      struct bt_mesh_msg_ctx *ctx,
			      enum bt_mesh_dfd_status status)
{
	BT_MESH_MODEL_BUF_DEFINE(rsp, BT_MESH_DFD_OP_UPLOAD_STATUS,
				 DFD_UPLOAD_STATUS_MSG_MAXLEN);
	bt_mesh_model_msg_init(&rsp, BT_MESH_DFD_OP_UPLOAD_STATUS);

	net_buf_simple_add_u8(&rsp, status);
	net_buf_simple_add_u8(&rsp, srv->upload.phase);

	if (srv->upload.phase == BT_MESH_DFD_UPLOAD_PHASE_IDLE ||
	    !srv->upload.slot) {
		bt_mesh_model_send(srv->mod, ctx, &rsp, NULL, NULL);
		return;
	}

	net_buf_simple_add_u8(&rsp,
			      bt_mesh_blob_srv_progress(&srv->upload.blob));
	net_buf_simple_add_mem(&rsp, srv->upload.slot->fwid,
			       srv->upload.slot->fwid_len);

	bt_mesh_model_send(srv->mod, ctx, &rsp, NULL, NULL);
}

static int handle_upload_get(struct bt_mesh_model *mod, struct bt_mesh_msg_ctx *ctx,
			     struct net_buf_simple *buf)
{
	struct bt_mesh_dfd_srv *srv = mod->user_data;

	upload_status_rsp(srv, ctx, BT_MESH_DFD_SUCCESS);

	return 0;
}

static int handle_upload_start(struct bt_mesh_model *mod, struct bt_mesh_msg_ctx *ctx,
			       struct net_buf_simple *buf)
{
	struct bt_mesh_dfd_srv *srv = mod->user_data;
	const struct bt_mesh_dfu_slot *old_slot = srv->upload.slot;
	size_t meta_len, fwid_len, size;
	const uint8_t *meta, *fwid;
	uint16_t timeout_base;
	uint64_t blob_id;
	int err, idx;
	uint8_t ttl;

	ttl = net_buf_simple_pull_u8(buf);
	timeout_base = net_buf_simple_pull_le16(buf);
	blob_id = net_buf_simple_pull_le64(buf);
	size = net_buf_simple_pull_le32(buf);
	meta_len = net_buf_simple_pull_u8(buf);
	if (buf->len < meta_len) {
		return -EINVAL;
	}

	meta = net_buf_simple_pull_mem(buf, meta_len);
	fwid_len = buf->len;
	fwid = net_buf_simple_pull_mem(buf, fwid_len);

	LOG_DBG("Upload Start: size: %d, fwid: %s, metadata: %s", size, bt_hex(fwid, fwid_len),
		bt_hex(meta, meta_len));

	if (size > CONFIG_BT_MESH_DFD_SRV_SLOT_MAX_SIZE ||
	    fwid_len > CONFIG_BT_MESH_DFU_FWID_MAXLEN ||
	    meta_len > CONFIG_BT_MESH_DFU_METADATA_MAXLEN) {
		upload_status_rsp(srv, ctx,
				  BT_MESH_DFD_ERR_INSUFFICIENT_RESOURCES);
		return 0;
	}

	if (upload_is_busy(srv)) {
		if (!srv->upload.slot) {
			LOG_WRN("Busy with no upload slot");
			upload_status_rsp(srv, ctx, BT_MESH_DFD_ERR_INTERNAL);
			return 0;
		}

		if (srv->upload.slot->fwid_len == fwid_len &&
		    !memcmp(srv->upload.slot->fwid, fwid, fwid_len) &&
		    srv->upload.slot->metadata_len == meta_len &&
		    !memcmp(srv->upload.slot->metadata, meta, meta_len) &&
		    srv->upload.blob.state.xfer.id == blob_id &&
		    srv->upload.blob.state.ttl == ttl &&
		    srv->upload.blob.state.timeout_base == timeout_base) {
			LOG_DBG("Duplicate upload start");
			upload_status_rsp(srv, ctx, BT_MESH_DFD_SUCCESS);
			return 0;
		}

		LOG_WRN("Upload already in progress");
		upload_status_rsp(srv, ctx, BT_MESH_DFD_ERR_BUSY_WITH_UPLOAD);
		return 0;
	}

	idx = bt_mesh_dfu_slot_get(fwid, fwid_len, &srv->upload.slot);
	if (idx >= 0 && bt_mesh_dfu_slot_is_valid(srv->upload.slot)) {
		LOG_DBG("Already received image");
		srv->upload.phase = BT_MESH_DFD_UPLOAD_PHASE_TRANSFER_SUCCESS;
		upload_status_rsp(srv, ctx, BT_MESH_DFD_SUCCESS);
		return 0;
	}

	if (old_slot && !bt_mesh_dfu_slot_is_valid(old_slot)) {
		LOG_DBG("Deleting old invalid slot");
		slot_del(srv, old_slot);
	}

	/* TODO Store transfer state before slot is added. */

	srv->upload.slot = bt_mesh_dfu_slot_add(size, fwid, fwid_len, meta,
						meta_len, NULL, 0);
	if (!srv->upload.slot) {
		LOG_WRN("No space for slot");
		upload_status_rsp(srv, ctx,
				  BT_MESH_DFD_ERR_INSUFFICIENT_RESOURCES);
		return 0;
	}

	srv->io = NULL;
	err = srv->cb->recv(srv, srv->upload.slot, &srv->io);
	if (err || !srv->io) {
		LOG_ERR("App rejected upload. err: %d io: %p", err, srv->io);
		slot_del(srv, srv->upload.slot);
		upload_status_rsp(srv, ctx, BT_MESH_DFD_ERR_INTERNAL);
		return 0;
	}

	err = bt_mesh_blob_srv_recv(&srv->upload.blob, blob_id, srv->io, ttl,
				    timeout_base);
	if (err) {
		LOG_ERR("BLOB Server rejected upload (err: %d)", err);
		slot_del(srv, srv->upload.slot);
		upload_status_rsp(srv, ctx, BT_MESH_DFD_ERR_INTERNAL);
		return 0;
	}

	LOG_DBG("%s", bt_hex(fwid, fwid_len));

	srv->upload.phase = BT_MESH_DFD_UPLOAD_PHASE_TRANSFER_ACTIVE;
	upload_status_rsp(srv, ctx, BT_MESH_DFD_SUCCESS);

	return 0;
}

static int handle_upload_start_oob(struct bt_mesh_model *mod, struct bt_mesh_msg_ctx *ctx,
				   struct net_buf_simple *buf)
{
	struct bt_mesh_dfd_srv *srv = mod->user_data;

	LOG_DBG("");

	upload_status_rsp(srv, ctx, BT_MESH_DFD_ERR_URI_NOT_SUPPORTED);

	return 0;
}

static int handle_upload_cancel(struct bt_mesh_model *mod, struct bt_mesh_msg_ctx *ctx,
				struct net_buf_simple *buf)
{
	struct bt_mesh_dfd_srv *srv = mod->user_data;

	srv->upload.phase = BT_MESH_DFD_UPLOAD_PHASE_IDLE;
	(void)bt_mesh_blob_srv_cancel(&srv->upload.blob);
	upload_status_rsp(srv, ctx, BT_MESH_DFD_SUCCESS);

	return 0;
}

static void fw_status_rsp(struct bt_mesh_dfd_srv *srv,
			  struct bt_mesh_msg_ctx *ctx,
			  enum bt_mesh_dfd_status status, uint16_t idx,
			  const uint8_t *fwid, size_t fwid_len)
{
	BT_MESH_MODEL_BUF_DEFINE(rsp, BT_MESH_DFD_OP_FW_STATUS,
				 7 + CONFIG_BT_MESH_DFU_FWID_MAXLEN);
	bt_mesh_model_msg_init(&rsp, BT_MESH_DFD_OP_FW_STATUS);

	net_buf_simple_add_u8(&rsp, status);
	net_buf_simple_add_le16(&rsp, bt_mesh_dfu_slot_foreach(NULL, NULL));

	net_buf_simple_add_le16(&rsp, idx);
	if (fwid) {
		net_buf_simple_add_mem(&rsp, fwid, fwid_len);
	}

	bt_mesh_model_send(srv->mod, ctx, &rsp, NULL, NULL);
}

static int handle_fw_get(struct bt_mesh_model *mod, struct bt_mesh_msg_ctx *ctx,
			 struct net_buf_simple *buf)
{
	struct bt_mesh_dfd_srv *srv = mod->user_data;
	const struct bt_mesh_dfu_slot *slot;
	const uint8_t *fwid;
	size_t fwid_len;
	int idx;

	fwid_len = buf->len;
	fwid = net_buf_simple_pull_mem(buf, fwid_len);

	idx = bt_mesh_dfu_slot_get(fwid, fwid_len, &slot);
	if (idx >= 0 && bt_mesh_dfu_slot_is_valid(slot)) {
		fw_status_rsp(srv, ctx, BT_MESH_DFD_SUCCESS, idx, fwid,
			      fwid_len);
	} else {
		fw_status_rsp(srv, ctx, BT_MESH_DFD_ERR_FW_NOT_FOUND, 0xffff,
			      fwid, fwid_len);
	}

	return 0;
}

static int handle_fw_get_by_index(struct bt_mesh_model *mod, struct bt_mesh_msg_ctx *ctx,
				  struct net_buf_simple *buf)
{
	struct bt_mesh_dfd_srv *srv = mod->user_data;
	const struct bt_mesh_dfu_slot *slot;
	uint16_t idx;

	idx = net_buf_simple_pull_le16(buf);

	slot = bt_mesh_dfu_slot_at(idx);
	if (slot && bt_mesh_dfu_slot_is_valid(slot)) {
		fw_status_rsp(srv, ctx, BT_MESH_DFD_SUCCESS, idx, slot->fwid,
			      slot->fwid_len);
	} else {
		fw_status_rsp(srv, ctx, BT_MESH_DFD_ERR_FW_NOT_FOUND, idx,
			      NULL, 0);
	}

	return 0;
}

static int handle_fw_delete(struct bt_mesh_model *mod, struct bt_mesh_msg_ctx *ctx,
			    struct net_buf_simple *buf)
{
	struct bt_mesh_dfd_srv *srv = mod->user_data;
	const uint8_t *fwid;
	size_t fwid_len;

	fwid_len = buf->len;
	fwid = net_buf_simple_pull_mem(buf, fwid_len);

	enum bt_mesh_dfd_status status = bt_mesh_dfd_srv_fw_delete(srv, &fwid_len, &fwid);

	fw_status_rsp(srv, ctx, status, 0xffff, fwid, fwid_len);

	return 0;
}

static enum bt_mesh_dfu_iter slot_del_cb(const struct bt_mesh_dfu_slot *slot,
					 void *user_data)
{
	struct bt_mesh_dfd_srv *srv = user_data;

	if (srv->cb && srv->cb->del) {
		srv->cb->del(srv, slot);
	}

	return BT_MESH_DFU_ITER_CONTINUE;
}

static int handle_fw_delete_all(struct bt_mesh_model *mod, struct bt_mesh_msg_ctx *ctx,
				struct net_buf_simple *buf)
{
	struct bt_mesh_dfd_srv *srv = mod->user_data;

	fw_status_rsp(srv, ctx, bt_mesh_dfd_srv_fw_delete_all(srv), 0xffff, NULL, 0);

	return 0;
}

const struct bt_mesh_model_op _bt_mesh_dfd_srv_op[] = {
	{ BT_MESH_DFD_OP_RECEIVERS_ADD, BT_MESH_LEN_MIN(3), handle_receivers_add },
	{ BT_MESH_DFD_OP_RECEIVERS_DELETE_ALL, BT_MESH_LEN_EXACT(0), handle_receivers_delete_all },
	{ BT_MESH_DFD_OP_RECEIVERS_GET, BT_MESH_LEN_EXACT(4), handle_receivers_get },
	{ BT_MESH_DFD_OP_CAPABILITIES_GET, BT_MESH_LEN_EXACT(0), handle_capabilities_get },
	{ BT_MESH_DFD_OP_GET, BT_MESH_LEN_EXACT(0), handle_get },
	{ BT_MESH_DFD_OP_START, BT_MESH_LEN_MIN(10), handle_start },
	{ BT_MESH_DFD_OP_SUSPEND, BT_MESH_LEN_EXACT(0), handle_suspend },
	{ BT_MESH_DFD_OP_CANCEL, BT_MESH_LEN_EXACT(0), handle_cancel },
	{ BT_MESH_DFD_OP_APPLY, BT_MESH_LEN_EXACT(0), handle_apply },
	{ BT_MESH_DFD_OP_UPLOAD_GET, BT_MESH_LEN_EXACT(0), handle_upload_get },
	{ BT_MESH_DFD_OP_UPLOAD_START, BT_MESH_LEN_MIN(16), handle_upload_start },
	{ BT_MESH_DFD_OP_UPLOAD_START_OOB, BT_MESH_LEN_EXACT(2), handle_upload_start_oob },
	{ BT_MESH_DFD_OP_UPLOAD_CANCEL, BT_MESH_LEN_EXACT(0), handle_upload_cancel },
	{ BT_MESH_DFD_OP_FW_GET, BT_MESH_LEN_MIN(0), handle_fw_get },
	{ BT_MESH_DFD_OP_FW_GET_BY_INDEX, BT_MESH_LEN_EXACT(2), handle_fw_get_by_index },
	{ BT_MESH_DFD_OP_FW_DELETE, BT_MESH_LEN_MIN(0), handle_fw_delete },
	{ BT_MESH_DFD_OP_FW_DELETE_ALL, BT_MESH_LEN_EXACT(0), handle_fw_delete_all },

	BT_MESH_MODEL_OP_END
};

static void dfu_suspended(struct bt_mesh_dfu_cli *cli)
{
	struct bt_mesh_dfd_srv *srv =
		CONTAINER_OF(cli, struct bt_mesh_dfd_srv, dfu);

	srv->phase = BT_MESH_DFD_PHASE_TRANSFER_SUSPENDED;
}

static void dfu_ended(struct bt_mesh_dfu_cli *cli,
		      enum bt_mesh_dfu_status reason)
{
	struct bt_mesh_dfd_srv *srv =
		CONTAINER_OF(cli, struct bt_mesh_dfd_srv, dfu);
	int err;

	LOG_DBG("reason: %u, phase: %u, apply: %u", reason, srv->phase, srv->apply);

	if (srv->phase == BT_MESH_DFD_PHASE_IDLE) {
		return;
	}

	if (srv->phase == BT_MESH_DFD_PHASE_CANCELING_UPDATE) {
		dfd_phase_set(srv, BT_MESH_DFD_PHASE_IDLE);
		return;
	}

	if (reason != BT_MESH_DFU_SUCCESS) {
		dfd_phase_set(srv, BT_MESH_DFD_PHASE_FAILED);
		return;
	}

	if (!srv->apply) {
		dfd_phase_set(srv, BT_MESH_DFD_PHASE_TRANSFER_SUCCESS);
		return;
	}

	dfd_phase_set(srv, BT_MESH_DFD_PHASE_APPLYING_UPDATE);

	err = bt_mesh_dfu_cli_apply(cli);
	if (err) {
		LOG_ERR("Apply failed: %d", err);
		dfd_phase_set(srv, BT_MESH_DFD_PHASE_FAILED);
	}
}

static void dfu_applied(struct bt_mesh_dfu_cli *cli)
{
	struct bt_mesh_dfd_srv *srv =
		CONTAINER_OF(cli, struct bt_mesh_dfd_srv, dfu);
	int err;

	if (srv->phase == BT_MESH_DFD_PHASE_CANCELING_UPDATE) {
		dfd_phase_set(srv, BT_MESH_DFD_PHASE_FAILED);
		return;
	}

	if (srv->phase != BT_MESH_DFD_PHASE_APPLYING_UPDATE) {
		return;
	}

	err = bt_mesh_dfu_cli_confirm(cli);
	if (err) {
		LOG_ERR("Confirm failed: %d", err);
		dfd_phase_set(srv, BT_MESH_DFD_PHASE_FAILED);
	}
}

static void dfu_confirmed(struct bt_mesh_dfu_cli *cli)
{
	struct bt_mesh_dfd_srv *srv =
		CONTAINER_OF(cli, struct bt_mesh_dfd_srv, dfu);

	if (srv->phase != BT_MESH_DFD_PHASE_APPLYING_UPDATE &&
	    srv->phase != BT_MESH_DFD_PHASE_CANCELING_UPDATE) {
		return;
	}

	dfd_phase_set(srv, BT_MESH_DFD_PHASE_COMPLETED);
}

const struct bt_mesh_dfu_cli_cb _bt_mesh_dfd_srv_dfu_cb = {
	.suspended = dfu_suspended,
	.ended = dfu_ended,
	.applied = dfu_applied,
	.confirmed = dfu_confirmed,
};

static int upload_start(struct bt_mesh_blob_srv *b, struct bt_mesh_msg_ctx *ctx,
			struct bt_mesh_blob_xfer *xfer)
{
	LOG_DBG("");
	return 0;
}

static void upload_end(struct bt_mesh_blob_srv *b, uint64_t id, bool success)
{
	struct bt_mesh_dfd_srv *srv =
		CONTAINER_OF(b, struct bt_mesh_dfd_srv, upload.blob);

	if (srv->upload.phase != BT_MESH_DFD_UPLOAD_PHASE_TRANSFER_ACTIVE) {
		return;
	}

	LOG_DBG("%u", success);

	if (success) {
		bt_mesh_dfu_slot_valid_set(srv->upload.slot, true);
		srv->upload.phase = BT_MESH_DFD_UPLOAD_PHASE_TRANSFER_SUCCESS;
		return;
	}

	/* Will delete slot when we start a new upload */
	srv->upload.phase = BT_MESH_DFD_UPLOAD_PHASE_TRANSFER_ERROR;
}

static void upload_timeout(struct bt_mesh_blob_srv *b)
{
	LOG_DBG("");

	upload_end(b, b->state.xfer.id, false);
}

const struct bt_mesh_blob_srv_cb _bt_mesh_dfd_srv_blob_cb = {
	.start = upload_start,
	.end = upload_end,
	.suspended = upload_timeout,
};

static int dfd_srv_init(struct bt_mesh_model *mod)
{
	struct bt_mesh_dfd_srv *srv = mod->user_data;

	srv->mod = mod;

	if (IS_ENABLED(CONFIG_BT_MESH_MODEL_EXTENSIONS)) {
		bt_mesh_model_extend(mod, srv->upload.blob.mod);
	}

	return 0;
}

static void dfd_srv_reset(struct bt_mesh_model *mod)
{
	struct bt_mesh_dfd_srv *srv = mod->user_data;

	dfd_phase_set(srv, BT_MESH_DFD_PHASE_IDLE);
	srv->upload.phase = BT_MESH_DFD_UPLOAD_PHASE_IDLE;

	sys_slist_init(&srv->inputs.targets);
	srv->target_cnt = 0;

	bt_mesh_dfu_slot_foreach(slot_del_cb, srv);
	bt_mesh_dfu_slot_del_all();
}

const struct bt_mesh_model_cb _bt_mesh_dfd_srv_cb = {
	.init = dfd_srv_init,
	.reset = dfd_srv_reset,
};

enum bt_mesh_dfd_status bt_mesh_dfd_srv_receiver_add(struct bt_mesh_dfd_srv *srv, uint16_t addr,
						     uint8_t img_idx)
{
	struct bt_mesh_dfu_target *t;
	struct bt_mesh_blob_target_pull *p;

	if (!BT_MESH_ADDR_IS_UNICAST(addr)) {
		return BT_MESH_DFD_SUCCESS;
	}

	t = target_get(srv, addr);
	if (t) {
		t->img_idx = img_idx;
		return BT_MESH_DFD_SUCCESS;
	}

	/* New target node, add it to the list */

	if (srv->target_cnt == ARRAY_SIZE(srv->targets)) {
		return BT_MESH_DFD_ERR_INSUFFICIENT_RESOURCES;
	}

	t = &srv->targets[srv->target_cnt];
	p = &srv->pull_ctxs[srv->target_cnt];
	srv->target_cnt++;

	memset(t, 0, sizeof(*t));
	memset(p, 0, sizeof(*p));
	t->blob.addr = addr;
	t->blob.pull = p;
	t->img_idx = img_idx;

	LOG_DBG("Added receiver 0x%04x img: %u", t->blob.addr,
		t->img_idx);

	return BT_MESH_DFD_SUCCESS;
}

enum bt_mesh_dfd_status bt_mesh_dfd_srv_receivers_delete_all(struct bt_mesh_dfd_srv *srv)
{
	if (bt_mesh_dfu_cli_is_busy(&srv->dfu)) {
		return BT_MESH_DFD_ERR_BUSY_WITH_DISTRIBUTION;
	}

	sys_slist_init(&srv->inputs.targets);
	srv->target_cnt = 0;

	return BT_MESH_DFD_SUCCESS;
}

enum bt_mesh_dfd_status bt_mesh_dfd_srv_start(struct bt_mesh_dfd_srv *srv,
					      struct bt_mesh_dfd_start_params *params)
{
	int err, i;
	struct bt_mesh_dfu_cli_xfer xfer = { 0 };

	if (!srv->target_cnt) {
		return BT_MESH_DFD_ERR_RECEIVERS_LIST_EMPTY;
	}

	if (!bt_mesh_app_key_exists(params->app_idx)) {
		return BT_MESH_DFD_ERR_INVALID_APPKEY_INDEX;
	}

	xfer.mode = params->xfer_mode;
	xfer.slot = bt_mesh_dfu_slot_at(params->slot_idx);
	if (!xfer.slot || !bt_mesh_dfu_slot_is_valid(xfer.slot)) {
		return BT_MESH_DFD_ERR_FW_NOT_FOUND;
	}

	if (srv->inputs.app_idx == params->app_idx &&
	    srv->inputs.timeout_base == params->timeout_base &&
	    srv->inputs.group == params->group && srv->inputs.ttl == params->ttl &&
	    srv->dfu.xfer.blob.mode == xfer.mode && srv->apply == params->apply &&
	    srv->slot_idx == params->slot_idx) {
		if (is_busy(srv) ||
		    srv->phase == BT_MESH_DFD_PHASE_COMPLETED) {
			LOG_WRN("Already completed or in progress");
			return BT_MESH_DFD_SUCCESS;
		} else if (srv->phase == BT_MESH_DFD_PHASE_TRANSFER_SUSPENDED) {
			bt_mesh_dfu_cli_resume(&srv->dfu);
			dfd_phase_set(srv, BT_MESH_DFD_PHASE_TRANSFER_ACTIVE);
			return BT_MESH_DFD_SUCCESS;
		}
	} else if (is_busy(srv) ||
		   srv->phase == BT_MESH_DFD_PHASE_TRANSFER_SUSPENDED) {
		LOG_WRN("Busy with distribution");
		return BT_MESH_DFD_ERR_BUSY_WITH_DISTRIBUTION;
	}

	if (srv->phase == BT_MESH_DFD_PHASE_CANCELING_UPDATE) {
		LOG_WRN("Canceling distribution");
		return BT_MESH_DFD_ERR_BUSY_WITH_DISTRIBUTION;
	}

	srv->io = NULL;
	err = srv->cb->send(srv, xfer.slot, &srv->io);
	if (err || !srv->io) {
		return BT_MESH_DFD_ERR_INTERNAL;
	}

	sys_slist_init(&srv->inputs.targets);
	for (i = 0; i < srv->target_cnt; i++) {
		sys_slist_append(&srv->inputs.targets, &srv->targets[i].blob.n);
	}

	srv->slot_idx = params->slot_idx;
	srv->inputs.app_idx = params->app_idx;
	srv->inputs.timeout_base = params->timeout_base;
	srv->inputs.group = params->group;
	srv->inputs.ttl = params->ttl;
	srv->apply = params->apply;

	LOG_DBG("Distribution Start: slot: %d, appidx: %d, tb: %d, addr: %04X, ttl: %d, apply: %d",
		params->slot_idx, params->app_idx, params->timeout_base, params->group, params->ttl,
		params->apply);

	/* DFD Server will always retrieve targets' capabilities before distributing a firmware.*/
	xfer.blob_params = NULL;

	dfd_phase_set(srv, BT_MESH_DFD_PHASE_TRANSFER_ACTIVE);
	err = bt_mesh_dfu_cli_send(&srv->dfu, &srv->inputs, srv->io, &xfer);
	if (err) {
		dfd_phase_set(srv, BT_MESH_DFD_PHASE_IDLE);
		return BT_MESH_DFD_ERR_INTERNAL;
	}

	return BT_MESH_DFD_SUCCESS;
}

enum bt_mesh_dfd_status bt_mesh_dfd_srv_suspend(struct bt_mesh_dfd_srv *srv)
{
	int err;

	if (srv->phase == BT_MESH_DFD_PHASE_TRANSFER_SUSPENDED) {
		return BT_MESH_DFD_SUCCESS;
	}

	if (srv->phase != BT_MESH_DFD_PHASE_TRANSFER_ACTIVE) {
		return BT_MESH_DFD_ERR_WRONG_PHASE;
	}

	err = bt_mesh_dfu_cli_suspend(&srv->dfu);
	if (err) {
		return BT_MESH_DFD_ERR_SUSPEND_FAILED;
	}

	srv->phase = BT_MESH_DFD_PHASE_TRANSFER_SUSPENDED;
	return BT_MESH_DFD_SUCCESS;
}

enum bt_mesh_dfd_status bt_mesh_dfd_srv_cancel(struct bt_mesh_dfd_srv *srv,
					       struct bt_mesh_msg_ctx *ctx)
{
	enum bt_mesh_dfd_phase prev_phase;
	int err;

	if (srv->phase == BT_MESH_DFD_PHASE_CANCELING_UPDATE ||
	    srv->phase == BT_MESH_DFD_PHASE_IDLE) {
		if (ctx != NULL) {
			status_rsp(srv, ctx, BT_MESH_DFD_SUCCESS);
		}
		return BT_MESH_DFD_SUCCESS;
	}

	if (srv->phase == BT_MESH_DFD_PHASE_COMPLETED ||
	    srv->phase == BT_MESH_DFD_PHASE_FAILED) {
		dfd_phase_set(srv, BT_MESH_DFD_PHASE_IDLE);
		if (ctx != NULL) {
			status_rsp(srv, ctx, BT_MESH_DFD_SUCCESS);
		}
		return BT_MESH_DFD_SUCCESS;
	}

	/* Phase TRANSFER_ACTIVE, TRANSFER_SUSPENDED, TRANSFER_SUCCESS, APPLYING_UPDATE: */

	prev_phase = srv->phase;
	dfd_phase_set(srv, BT_MESH_DFD_PHASE_CANCELING_UPDATE);
	err = bt_mesh_dfu_cli_cancel(&srv->dfu, NULL);
	if (err) {
		if (ctx != NULL) {
			status_rsp(srv, ctx, BT_MESH_DFD_ERR_INTERNAL);
		}
		return BT_MESH_DFD_ERR_INTERNAL;
	}

	if (ctx != NULL) {
		status_rsp(srv, ctx, BT_MESH_DFD_SUCCESS);
	}

	if (prev_phase == BT_MESH_DFD_PHASE_APPLYING_UPDATE) {
		dfd_phase_set(srv, BT_MESH_DFD_PHASE_IDLE);
		if (ctx != NULL) {
			status_rsp(srv, ctx, BT_MESH_DFD_SUCCESS);
		}
	}

	return BT_MESH_DFD_SUCCESS;
}

enum bt_mesh_dfd_status bt_mesh_dfd_srv_apply(struct bt_mesh_dfd_srv *srv)
{
	int err;

	if (srv->phase == BT_MESH_DFD_PHASE_IDLE ||
	    srv->phase == BT_MESH_DFD_PHASE_CANCELING_UPDATE ||
	    srv->phase == BT_MESH_DFD_PHASE_TRANSFER_ACTIVE ||
	    srv->phase == BT_MESH_DFD_PHASE_TRANSFER_SUSPENDED ||
	    srv->phase == BT_MESH_DFD_PHASE_FAILED) {
		return BT_MESH_DFD_ERR_WRONG_PHASE;
	}

	if (srv->phase == BT_MESH_DFD_PHASE_APPLYING_UPDATE ||
	    srv->phase == BT_MESH_DFD_PHASE_COMPLETED) {
		return BT_MESH_DFD_SUCCESS;
	}

	err = bt_mesh_dfu_cli_apply(&srv->dfu);
	if (err) {
		return BT_MESH_DFD_ERR_INTERNAL;
	}

	dfd_phase_set(srv, BT_MESH_DFD_PHASE_APPLYING_UPDATE);
	return BT_MESH_DFD_SUCCESS;
}

enum bt_mesh_dfd_status bt_mesh_dfd_srv_fw_delete(struct bt_mesh_dfd_srv *srv, size_t *fwid_len,
						  const uint8_t **fwid)
{
	const struct bt_mesh_dfu_slot *slot;
	int idx, err;

	if (srv->phase != BT_MESH_DFD_PHASE_IDLE) {
		*fwid = NULL;
		*fwid_len = 0;
		return BT_MESH_DFD_ERR_BUSY_WITH_DISTRIBUTION;
	}

	idx = bt_mesh_dfu_slot_get(*fwid, *fwid_len, &slot);
	if (idx < 0 || !bt_mesh_dfu_slot_is_valid(slot)) {
		return BT_MESH_DFD_SUCCESS;
	}

	err = slot_del(srv, slot);
	if (err) {
		*fwid = NULL;
		*fwid_len = 0;
		return BT_MESH_DFD_ERR_INTERNAL;
	} else {
		return BT_MESH_DFD_SUCCESS;
	}
}

enum bt_mesh_dfd_status bt_mesh_dfd_srv_fw_delete_all(struct bt_mesh_dfd_srv *srv)
{
	if (srv->phase != BT_MESH_DFD_PHASE_IDLE) {
		return BT_MESH_DFD_ERR_BUSY_WITH_DISTRIBUTION;
	}

	bt_mesh_dfu_slot_foreach(slot_del_cb, srv);

	bt_mesh_dfu_slot_del_all();

	return BT_MESH_DFD_SUCCESS;
}
