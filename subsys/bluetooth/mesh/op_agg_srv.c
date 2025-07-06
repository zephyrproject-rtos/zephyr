/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/mesh.h>

#include "access.h"
#include "foundation.h"
#include "net.h"
#include "mesh.h"
#include "transport.h"
#include "op_agg.h"

#define LOG_LEVEL CONFIG_BT_MESH_MODEL_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_mesh_op_agg_srv);

NET_BUF_SIMPLE_DEFINE_STATIC(sdu, BT_MESH_TX_SDU_MAX);

/** Mesh Opcodes Aggregator Server Model Context */
static struct bt_mesh_op_agg_srv {
	/** Composition data model entry pointer. */
	const struct bt_mesh_model *model;
	/** Response error code. */
	int rsp_err;
	/** Indicates that the received aggregated message
	 * was acknowledged by local server model.
	 */
	bool ack;
	/** Aggregator context. */
	struct op_agg_ctx ctx;
} srv = {.ctx.sdu = &sdu};

static int handle_sequence(const struct bt_mesh_model *model,
			   struct bt_mesh_msg_ctx *ctx,
			   struct net_buf_simple *buf)
{
	struct net_buf_simple_state state;
	struct net_buf_simple msg;
	uint16_t elem;
	uint8_t *status;
	int err;

	elem = net_buf_simple_pull_le16(buf);
	ctx->recv_dst = elem;

	bt_mesh_model_msg_init(srv.ctx.sdu, OP_OPCODES_AGGREGATOR_STATUS);
	status = net_buf_simple_add_u8(srv.ctx.sdu, 0);
	net_buf_simple_add_le16(srv.ctx.sdu, elem);

	srv.ctx.net_idx = ctx->net_idx;
	srv.ctx.app_idx = ctx->app_idx;
	srv.ctx.addr = ctx->addr;
	srv.ctx.initialized = true;

	if (!BT_MESH_ADDR_IS_UNICAST(elem)) {
		LOG_WRN("Address is not unicast, ignoring.");
		return -EINVAL;
	}

	net_buf_simple_save(buf, &state);
	while (buf->len > 0) {
		err = bt_mesh_op_agg_decode_msg(&msg, buf);
		if (err) {
			LOG_ERR("Unable to parse Opcodes Aggregator Sequence message (err %d)",
				err);
			return err;
		}
	}
	net_buf_simple_restore(buf, &state);

	if (!bt_mesh_elem_find(elem)) {
		*status = ACCESS_STATUS_INVALID_ADDRESS;
		goto send;
	}

	while (buf->len > 0) {
		(void) bt_mesh_op_agg_decode_msg(&msg, buf);

		srv.ack = false;
		srv.rsp_err = 0;
		err = bt_mesh_model_recv(ctx, &msg);

		if (srv.rsp_err) {
			*status = srv.rsp_err;
			break;
		}

		if (err) {
			*status = err;
			break;
		}

		if (!srv.ack) {
			net_buf_simple_add_u8(srv.ctx.sdu, 0);
		}
	}

send:
	srv.ctx.initialized = false;
	err = bt_mesh_model_send(model, ctx, srv.ctx.sdu, NULL, NULL);
	if (err) {
		LOG_ERR("Unable to send Opcodes Aggregator Status");
		return err;
	}

	return 0;
}

const struct bt_mesh_model_op _bt_mesh_op_agg_srv_op[] = {
	{ OP_OPCODES_AGGREGATOR_SEQUENCE, BT_MESH_LEN_MIN(2), handle_sequence },
	BT_MESH_MODEL_OP_END,
};

static int op_agg_srv_init(const struct bt_mesh_model *model)
{
	if (!bt_mesh_model_in_primary(model)) {
		LOG_ERR("Opcodes Aggregator Server only allowed in primary element");
		return -EINVAL;
	}

	/* Opcodes Aggregator Server model shall use the device key and
	 * application keys.
	 */
	model->keys[0] = BT_MESH_KEY_DEV_ANY;

	srv.model = model;

	return 0;
}

int bt_mesh_op_agg_srv_send(const struct bt_mesh_model *model, struct net_buf_simple *msg)
{
	int err;

	/* Model responded so mark this message as acknowledged */
	srv.ack = true;

	err = bt_mesh_op_agg_encode_msg(msg, srv.ctx.sdu);
	if (err) {
		srv.rsp_err = ACCESS_STATUS_RESPONSE_OVERFLOW;
	}

	return err;
}

int bt_mesh_op_agg_srv_accept(struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf)
{

	return srv.ctx.initialized && (ctx->net_idx == srv.ctx.net_idx) &&
	       (ctx->addr == srv.ctx.addr) && (ctx->app_idx == srv.ctx.app_idx) &&
	       !bt_mesh_op_agg_is_op_agg_msg(buf);
}

const struct bt_mesh_model_cb _bt_mesh_op_agg_srv_cb = {
	.init = op_agg_srv_init,
};
