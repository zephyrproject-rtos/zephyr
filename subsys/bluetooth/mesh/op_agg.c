/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/mesh.h>

#include "foundation.h"
#include "op_agg.h"

#define LOG_LEVEL CONFIG_BT_MESH_MODEL_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_mesh_op_agg);

#define IS_LENGTH_LONG(buf) ((buf)->data[0] & 1)
#define LENGTH_SHORT_MAX BIT_MASK(7)

NET_BUF_SIMPLE_DEFINE_STATIC(sdu, BT_MESH_TX_SDU_MAX);

#ifdef CONFIG_BT_MESH_OP_AGG_CLI
NET_BUF_SIMPLE_DEFINE_STATIC(srcs, BT_MESH_TX_SDU_MAX);
#endif

static struct op_agg_ctx agg_ctx = {
	.sdu = &sdu,
#ifdef CONFIG_BT_MESH_OP_AGG_CLI
	.srcs = &srcs,
#endif
};

struct op_agg_ctx *bt_mesh_op_agg_ctx_get(void)
{
	return &agg_ctx;
}

static bool ctx_match(struct bt_mesh_msg_ctx *ctx)
{
	return (ctx->net_idx == agg_ctx.net_idx) && (ctx->addr == agg_ctx.addr) &&
	       (ctx->app_idx == agg_ctx.app_idx);
}

int bt_mesh_op_agg_accept(struct bt_mesh_msg_ctx *msg_ctx)
{
#ifdef CONFIG_BT_MESH_OP_AGG_CLI
	return agg_ctx.initialized && ctx_match(msg_ctx);
#else
	return 0;
#endif
}

void bt_mesh_op_agg_ctx_reinit(void)
{
	agg_ctx.initialized = true;
}

int bt_mesh_op_agg_send(struct bt_mesh_model *model,
			struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *msg,
			const struct bt_mesh_send_cb *cb)
{
	uint16_t src;
	int err;

	/* Model responded so mark this message as acknowledged */
	agg_ctx.ack = true;

	/* Store source address so that Opcodes Aggregator Client can match
	 * response with source model
	 */
	src = bt_mesh_model_elem(model)->addr;

	if (net_buf_simple_tailroom(&srcs) < 2) {
		return -ENOMEM;
	}

	net_buf_simple_add_le16(&srcs, src);

	err = bt_mesh_op_agg_encode_msg(msg);
	if (err) {
		agg_ctx.rsp_err = ACCESS_STATUS_RESPONSE_OVERFLOW;
	}

	return err;
}

int bt_mesh_op_agg_encode_msg(struct net_buf_simple *msg)
{
	if (msg->len > LENGTH_SHORT_MAX) {
		if (net_buf_simple_tailroom(agg_ctx.sdu) < (msg->len + 2)) {
			return -ENOMEM;
		}

		net_buf_simple_add_le16(agg_ctx.sdu, (msg->len << 1) | 1);
	} else {
		if (net_buf_simple_tailroom(agg_ctx.sdu) < (msg->len + 1)) {
			return -ENOMEM;
		}

		net_buf_simple_add_u8(agg_ctx.sdu, msg->len << 1);
	}
	net_buf_simple_add_mem(agg_ctx.sdu, msg->data, msg->len);

	return 0;
}

int bt_mesh_op_agg_decode_msg(struct net_buf_simple *msg,
			      struct net_buf_simple *buf)
{
	uint16_t len;

	if (IS_LENGTH_LONG(buf)) {
		if (buf->len < 2) {
			return -EINVAL;
		}

		len = net_buf_simple_pull_le16(buf) >> 1;
	} else {
		if (buf->len < 1) {
			return -EINVAL;
		}

		len = net_buf_simple_pull_u8(buf) >> 1;
	}

	if (buf->len < len) {
		return -EINVAL;
	}

	net_buf_simple_init_with_data(msg, net_buf_simple_pull_mem(buf, len), len);

	return 0;
}
