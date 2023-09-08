/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/mesh.h>

#include <common/bt_str.h>

#include "access.h"
#include "foundation.h"
#include "net.h"
#include "mesh.h"
#include "op_agg.h"

#define LOG_LEVEL CONFIG_BT_MESH_MODEL_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_mesh_op_agg_cli);

/** Mesh Opcodes Aggregator Client Model Context */
static struct bt_mesh_op_agg_cli {
	/** Composition data model entry pointer. */
	const struct bt_mesh_model *model;

	/* Internal parameters for tracking message responses. */
	struct bt_mesh_msg_ack_ctx ack_ctx;
} cli;

static int32_t msg_timeout;

static int handle_status(const struct bt_mesh_model *model,
			 struct bt_mesh_msg_ctx *ctx,
			 struct net_buf_simple *buf)
{
	struct op_agg_ctx *agg = bt_mesh_op_agg_ctx_get();
	struct net_buf_simple msg;
	uint8_t status;
	uint16_t elem_addr, addr;
	int err;

	LOG_DBG("net_idx 0x%04x app_idx 0x%04x src 0x%04x len %u: %s",
		ctx->net_idx, ctx->app_idx, ctx->addr, buf->len,
		bt_hex(buf->data, buf->len));

	if (!bt_mesh_msg_ack_ctx_match(&cli.ack_ctx,
				       OP_OPCODES_AGGREGATOR_STATUS, ctx->addr,
				       NULL)) {
		LOG_WRN("Unexpected Opcodes Aggregator Status");
		return -ENOENT;
	}

	status = net_buf_simple_pull_u8(buf);
	elem_addr = net_buf_simple_pull_le16(buf);

	while (buf->len > 0) {
		err = bt_mesh_op_agg_decode_msg(&msg, buf);
		if (err) {
			LOG_ERR("Cannot decode aggregated message %d", err);
			bt_mesh_op_agg_ctx_reinit();
			return -EINVAL;
		}

		if (agg->srcs->len < 2) {
			LOG_ERR("Mismatch in sources address buffer");
			bt_mesh_op_agg_ctx_reinit();
			return -ENOENT;
		}

		addr = net_buf_simple_pull_le16(agg->srcs);

		/* Empty item means unacked msg. */
		if (!msg.len) {
			continue;
		}

		ctx->recv_dst = addr;
		err = bt_mesh_model_recv(ctx, &msg);
		if (err) {
			LOG_ERR("Opcodes Aggregator receive error %d", err);
			bt_mesh_op_agg_ctx_reinit();
			return err;
		}
	}

	bt_mesh_msg_ack_ctx_rx(&cli.ack_ctx);

	return 0;
}

const struct bt_mesh_model_op _bt_mesh_op_agg_cli_op[] = {
	{ OP_OPCODES_AGGREGATOR_STATUS, BT_MESH_LEN_MIN(3), handle_status },
	BT_MESH_MODEL_OP_END,
};

static int op_agg_cli_init(const struct bt_mesh_model *model)
{
	if (!bt_mesh_model_in_primary(model)) {
		LOG_ERR("Opcodes Aggregator Client only allowed in primary element");
		return -EINVAL;
	}

	/* Opcodes Aggregator Client model shall use the device key and
	 * application keys.
	 */
	model->keys[0] = BT_MESH_KEY_DEV_ANY;

	msg_timeout = CONFIG_BT_MESH_OP_AGG_CLI_TIMEOUT;
	cli.model = model;
	bt_mesh_msg_ack_ctx_init(&cli.ack_ctx);

	return 0;
}

int bt_mesh_op_agg_cli_seq_start(uint16_t net_idx, uint16_t app_idx, uint16_t dst,
				 uint16_t elem_addr)
{
	struct op_agg_ctx *agg = bt_mesh_op_agg_ctx_get();

	if (!BT_MESH_ADDR_IS_UNICAST(elem_addr)) {
		LOG_ERR("Element address shall be a unicast address");
		return -EINVAL;
	}

	if (agg->initialized) {
		LOG_ERR("Opcodes Aggregator is already configured");
		return -EALREADY;
	}

	agg->net_idx = net_idx;
	agg->app_idx = app_idx;
	agg->addr = dst;
	agg->ack = false;
	agg->rsp_err = 0;
	agg->initialized = true;

	net_buf_simple_init(agg->srcs, 0);
	bt_mesh_model_msg_init(agg->sdu, OP_OPCODES_AGGREGATOR_SEQUENCE);
	net_buf_simple_add_le16(agg->sdu, elem_addr);

	return 0;
}

int bt_mesh_op_agg_cli_seq_send(void)
{
	struct op_agg_ctx *agg = bt_mesh_op_agg_ctx_get();
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = agg->net_idx,
		.app_idx = agg->app_idx,
		.addr = agg->addr,
	};
	int err;

	if (!agg->initialized) {
		LOG_ERR("Opcodes Aggregator not initialized");
		return -EINVAL;
	}

	err = bt_mesh_msg_ack_ctx_prepare(&cli.ack_ctx, OP_OPCODES_AGGREGATOR_STATUS, agg->addr,
					  NULL);
	if (err) {
		return err;
	}

	agg->initialized = false;

	err = bt_mesh_model_send(cli.model, &ctx, agg->sdu, NULL, NULL);
	if (err) {
		LOG_ERR("model_send() failed (err %d)", err);
		bt_mesh_msg_ack_ctx_clear(&cli.ack_ctx);
		return err;
	}

	return bt_mesh_msg_ack_ctx_wait(&cli.ack_ctx, K_MSEC(msg_timeout));
}

void bt_mesh_op_agg_cli_seq_abort(void)
{
	struct op_agg_ctx *agg = bt_mesh_op_agg_ctx_get();

	agg->initialized = false;
}

bool bt_mesh_op_agg_cli_seq_is_started(void)
{
	struct op_agg_ctx *agg = bt_mesh_op_agg_ctx_get();

	return agg->initialized;
}

size_t bt_mesh_op_agg_cli_seq_tailroom(void)
{
	struct op_agg_ctx *agg = bt_mesh_op_agg_ctx_get();

	return net_buf_simple_tailroom(agg->sdu);
}

int32_t bt_mesh_op_agg_cli_timeout_get(void)
{
	return msg_timeout;
}

void bt_mesh_op_agg_cli_timeout_set(int32_t timeout)
{
	msg_timeout = timeout;
}

const struct bt_mesh_model_cb _bt_mesh_op_agg_cli_cb = {
	.init = op_agg_cli_init,
};
