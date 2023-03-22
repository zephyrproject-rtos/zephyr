/*  Bluetooth Mesh */

/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <zephyr/types.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/mesh.h>

#include "msg.h"

#include <common/bt_str.h>

#define LOG_LEVEL CONFIG_BT_MESH_MODEL_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_mesh_large_comp_data_cli);

#include "net.h"
#include "access.h"
#include "foundation.h"

/** Mesh Large Composition Data Client Model Context */
static struct bt_mesh_large_comp_data_cli {
	/** Composition data model entry pointer. */
	struct bt_mesh_model *model;

	/* Internal parameters for tracking message responses. */
	struct bt_mesh_msg_ack_ctx ack_ctx;
} cli;

static int32_t msg_timeout;

static int large_comp_data_status(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
				  struct net_buf_simple *buf)
{
	struct net_buf_simple *comp;
	size_t to_copy;

	LOG_DBG("net_idx 0x%04x app_idx 0x%04x src 0x%04x len %u: %s",
		ctx->net_idx, ctx->app_idx, ctx->addr, buf->len,
		bt_hex(buf->data, buf->len));

	if (!bt_mesh_msg_ack_ctx_match(&cli.ack_ctx, OP_LARGE_COMP_DATA_STATUS,
				       ctx->addr, (void **)&comp)) {
		return 0;
	}

	to_copy = MIN(net_buf_simple_tailroom(comp), buf->len);
	net_buf_simple_add_mem(comp, buf->data, to_copy);

	bt_mesh_msg_ack_ctx_rx(&cli.ack_ctx);

	return 0;
}

static int models_metadata_status(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
				  struct net_buf_simple *buf)
{
	struct net_buf_simple *metadata;
	size_t to_copy;

	LOG_DBG("net_idx 0x%04x app_idx 0x%04x src 0x%04x len %u: %s",
		ctx->net_idx, ctx->app_idx, ctx->addr, buf->len,
		bt_hex(buf->data, buf->len));

	if (!bt_mesh_msg_ack_ctx_match(&cli.ack_ctx, OP_MODELS_METADATA_STATUS,
				       ctx->addr, (void **)&metadata)) {
		return 0;
	}

	metadata = (struct net_buf_simple *)cli.ack_ctx.user_data;

	to_copy = MIN(net_buf_simple_tailroom(metadata), buf->len);
	net_buf_simple_add_mem(metadata, buf->data, to_copy);

	bt_mesh_msg_ack_ctx_rx(&cli.ack_ctx);

	return 0;
}

const struct bt_mesh_model_op _bt_mesh_large_comp_data_cli_op[] = {
	{ OP_LARGE_COMP_DATA_STATUS,   BT_MESH_LEN_MIN(5),  large_comp_data_status },
	{ OP_MODELS_METADATA_STATUS,   BT_MESH_LEN_MIN(5),  models_metadata_status },
	BT_MESH_MODEL_OP_END,
};

static int large_comp_data_cli_init(struct bt_mesh_model *model)
{
	if (!bt_mesh_model_in_primary(model)) {
		LOG_ERR("Configuration Client only allowed in primary element");
		return -EINVAL;
	}

	model->keys[0] = BT_MESH_KEY_DEV_ANY;
	model->flags |= BT_MESH_MOD_DEVKEY_ONLY;

	cli.model = model;

	msg_timeout = 5000;
	bt_mesh_msg_ack_ctx_init(&cli.ack_ctx);

	return 0;
}

const struct bt_mesh_model_cb _bt_mesh_large_comp_data_cli_cb = {
	.init = large_comp_data_cli_init,
};

int bt_mesh_large_comp_data_get(uint16_t net_idx, uint16_t addr, uint8_t page,
				size_t offset, struct net_buf_simple *comp)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, OP_LARGE_COMP_DATA_GET, 3);
	struct bt_mesh_msg_ctx ctx = BT_MESH_MSG_CTX_INIT_DEV(net_idx, addr);
	const struct bt_mesh_msg_rsp_ctx rsp = {
		.ack = &cli.ack_ctx,
		.op = OP_LARGE_COMP_DATA_STATUS,
		.user_data = comp,
		.timeout = msg_timeout,
	};

	bt_mesh_model_msg_init(&msg, OP_LARGE_COMP_DATA_GET);
	net_buf_simple_add_u8(&msg, page);
	net_buf_simple_add_le16(&msg, offset);

	return bt_mesh_msg_ackd_send(cli.model, &ctx, &msg, comp ? &rsp : NULL);
}

int bt_mesh_models_metadata_get(uint16_t net_idx, uint16_t addr, uint8_t page,
				size_t offset, struct net_buf_simple *metadata)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, OP_MODELS_METADATA_STATUS, 3);
	struct bt_mesh_msg_ctx ctx = BT_MESH_MSG_CTX_INIT_DEV(net_idx, addr);
	const struct bt_mesh_msg_rsp_ctx rsp = {
		.ack = &cli.ack_ctx,
		.op = OP_MODELS_METADATA_STATUS,
		.user_data = metadata,
		.timeout = msg_timeout,
	};

	bt_mesh_model_msg_init(&msg, OP_MODELS_METADATA_GET);
	net_buf_simple_add_u8(&msg, page);
	net_buf_simple_add_le16(&msg, offset);

	return bt_mesh_msg_ackd_send(cli.model, &ctx, &msg, metadata ? &rsp : NULL);
}
