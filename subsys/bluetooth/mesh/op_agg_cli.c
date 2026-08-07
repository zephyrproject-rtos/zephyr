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

NET_BUF_SIMPLE_DEFINE_STATIC(srcs, BT_MESH_TX_SDU_MAX);
NET_BUF_SIMPLE_DEFINE_STATIC(sdu, BT_MESH_TX_SDU_MAX);

/** Mesh Opcodes Aggregator Client Model Context */
static struct bt_mesh_op_agg_cli {
	/** Composition data model entry pointer. */
	const struct bt_mesh_model *model;
	/** Acknowledge context. */
	struct bt_mesh_msg_ack_ctx ack_ctx;
	/** List of source element addresses.
	 * Used by Client to match aggregated responses
	 * with local source client models.
	 */
	struct net_buf_simple *srcs;
	/** Aggregator context. */
	struct op_agg_ctx ctx;

} cli = {.srcs = &srcs, .ctx.sdu = &sdu};

static int32_t msg_timeout;

static int handle_status(const struct bt_mesh_model *model,
			 struct bt_mesh_msg_ctx *ctx,
			 struct net_buf_simple *buf)
{
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
			cli.ctx.initialized = true;
			return -EINVAL;
		}

		if (cli.srcs->len < 2) {
			LOG_ERR("Mismatch in sources address buffer");
			cli.ctx.initialized = true;
			return -ENOENT;
		}

		addr = net_buf_simple_pull_le16(cli.srcs);

		/* Empty item means unacked msg. */
		if (!msg.len) {
			continue;
		}

		ctx->recv_dst = addr;
		err = bt_mesh_model_recv(ctx, &msg);
		if (err) {
			LOG_ERR("Opcodes Aggregator receive error %d", err);
			cli.ctx.initialized = true;
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
	if (!BT_MESH_ADDR_IS_UNICAST(elem_addr)) {
		LOG_ERR("Element address shall be a unicast address");
		return -EINVAL;
	}

	if (cli.ctx.initialized) {
		LOG_ERR("Opcodes Aggregator is already configured");
		return -EALREADY;
	}

	cli.ctx.net_idx = net_idx;
	cli.ctx.app_idx = app_idx;
	cli.ctx.addr = dst;
	cli.ctx.initialized = true;

	net_buf_simple_init(cli.srcs, 0);
	bt_mesh_model_msg_init(cli.ctx.sdu, OP_OPCODES_AGGREGATOR_SEQUENCE);
	net_buf_simple_add_le16(cli.ctx.sdu, elem_addr);

	return 0;
}

int bt_mesh_op_agg_cli_seq_send(void)
{
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = cli.ctx.net_idx,
		.app_idx = cli.ctx.app_idx,
		.addr = cli.ctx.addr,
	};
	int err;

	if (!cli.ctx.initialized) {
		LOG_ERR("Opcodes Aggregator not initialized");
		return -EINVAL;
	}

	err = bt_mesh_msg_ack_ctx_prepare(&cli.ack_ctx, OP_OPCODES_AGGREGATOR_STATUS,
					  cli.ctx.addr, NULL);
	if (err) {
		return err;
	}

	cli.ctx.initialized = false;

	err = bt_mesh_model_send(cli.model, &ctx, cli.ctx.sdu, NULL, NULL);
	if (err) {
		LOG_ERR("model_send() failed (err %d)", err);
		bt_mesh_msg_ack_ctx_clear(&cli.ack_ctx);
		return err;
	}

	return bt_mesh_msg_ack_ctx_wait(&cli.ack_ctx, K_MSEC(msg_timeout));
}

void bt_mesh_op_agg_cli_seq_abort(void)
{
	cli.ctx.initialized = false;
}

bool bt_mesh_op_agg_cli_seq_is_started(void)
{
	return cli.ctx.initialized;
}

size_t bt_mesh_op_agg_cli_seq_tailroom(void)
{
	return net_buf_simple_tailroom(cli.ctx.sdu);
}

int32_t bt_mesh_op_agg_cli_timeout_get(void)
{
	return msg_timeout;
}

void bt_mesh_op_agg_cli_timeout_set(int32_t timeout)
{
	msg_timeout = timeout;
}

int bt_mesh_op_agg_cli_send(const struct bt_mesh_model *model, struct net_buf_simple *msg)
{
	uint16_t src = bt_mesh_model_elem(model)->rt->addr;

	if (net_buf_simple_tailroom(&srcs) < 2) {
		return -ENOMEM;
	}

	net_buf_simple_add_le16(&srcs, src);
	return bt_mesh_op_agg_encode_msg(msg, cli.ctx.sdu);
}

int bt_mesh_op_agg_cli_accept(struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf)
{

	return cli.ctx.initialized && (ctx->net_idx == cli.ctx.net_idx) &&
	       (ctx->addr == cli.ctx.addr) && (ctx->app_idx == cli.ctx.app_idx) &&
	       !bt_mesh_op_agg_is_op_agg_msg(buf);
}

const struct bt_mesh_model_cb _bt_mesh_op_agg_cli_cb = {
	.init = op_agg_cli_init,
};
