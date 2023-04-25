/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/mesh.h>

#include "foundation.h"
#include "msg.h"

#define LOG_LEVEL CONFIG_BT_MESH_MODEL_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_mesh_sol_pdu_rpl_cli);

static struct bt_mesh_sol_pdu_rpl_cli *cli;

static int32_t msg_timeout;

struct sol_rpl_param {
	uint16_t *start;
	uint8_t *len;
};

static int handle_status(const struct bt_mesh_model *mod,
			 struct bt_mesh_msg_ctx *ctx,
			 struct net_buf_simple *buf)
{
	struct sol_rpl_param *param;
	uint16_t primary, range;
	uint8_t len = 0;

	LOG_DBG("");

	if (buf->len > 3) {
		return -EMSGSIZE;
	}

	range = net_buf_simple_pull_le16(buf);
	primary = range >> 1;
	if (primary == 0) {
		return -EINVAL;
	}

	if (range & BIT(0)) {
		if (buf->len == 0) {
			return -EMSGSIZE;
		}

		len = net_buf_simple_pull_u8(buf);

		if (len < 2) {
			return -EINVAL;
		}
	}

	LOG_DBG("SRPL clear status received: range start: %u, range len: %u",
	       primary, len);

	if (bt_mesh_msg_ack_ctx_match(&cli->ack_ctx, OP_SOL_PDU_RPL_ITEM_STATUS,
				       ctx->addr, (void **)&param)) {
		if (param->start) {
			*param->start = primary;
		}

		if (param->len) {
			*param->len = len;
		}

		bt_mesh_msg_ack_ctx_rx(&cli->ack_ctx);
	}

	if (cli->srpl_status) {
		cli->srpl_status(cli, ctx->addr, primary, len);
	}

	return 0;
}

static void sol_pdu_rpl_clear_pdu_create(uint16_t range_start, uint8_t range_len,
					 struct net_buf_simple *msg)
{
	uint16_t range;

	range = range_start << 1 | (range_len >= 2 ? 1U : 0);
	net_buf_simple_add_le16(msg, range);
	if (range_len >= 2) {
		net_buf_simple_add_u8(msg, range_len);
	}
}

int bt_mesh_sol_pdu_rpl_clear(struct bt_mesh_msg_ctx *ctx, uint16_t range_start,
			      uint8_t range_len, uint16_t *start_rsp, uint8_t *len_rsp)
{
	struct sol_rpl_param param = {
		.start = start_rsp,
		.len = len_rsp,
	};

	const struct bt_mesh_msg_rsp_ctx rsp = {
		.ack = &cli->ack_ctx,
		.op = OP_SOL_PDU_RPL_ITEM_STATUS,
		.user_data = &param,
		.timeout = msg_timeout,
	};

	if (range_len == 1) {
		LOG_ERR("Invalid range length");
		return -EINVAL;
	}

	if ((range_start + (range_len > 1 ? range_len : 0)) > 0x8000 || range_start == 0) {
		LOG_ERR("Range outside unicast address range");
		return -EINVAL;
	}

	BT_MESH_MODEL_BUF_DEFINE(msg, OP_SOL_PDU_RPL_ITEM_CLEAR,
				 range_len >= 2 ? 3 : 2);

	bt_mesh_model_msg_init(&msg, OP_SOL_PDU_RPL_ITEM_CLEAR);

	sol_pdu_rpl_clear_pdu_create(range_start, range_len, &msg);

	return bt_mesh_msg_ackd_send(cli->model, ctx, &msg,
				    (start_rsp && len_rsp) ? &rsp : NULL);
}

int bt_mesh_sol_pdu_rpl_clear_unack(struct bt_mesh_msg_ctx *ctx, uint16_t range_start,
				    uint8_t range_len)
{
	if (range_len == 1) {
		LOG_ERR("Invalid range length");
		return -EINVAL;
	}

	if ((range_start + (range_len > 1 ? range_len : 0)) > 0x8000 || range_start == 0) {
		LOG_ERR("Range outside unicast address range");
		return -EINVAL;
	}

	BT_MESH_MODEL_BUF_DEFINE(msg, OP_SOL_PDU_RPL_ITEM_CLEAR,
				 range_len >= 2 ? 3 : 2);

	bt_mesh_model_msg_init(&msg, OP_SOL_PDU_RPL_ITEM_CLEAR_UNACKED);

	sol_pdu_rpl_clear_pdu_create(range_start, range_len, &msg);

	return bt_mesh_msg_send(cli->model, ctx, &msg);

}

const struct bt_mesh_model_op _bt_mesh_sol_pdu_rpl_cli_op[] = {
	{ OP_SOL_PDU_RPL_ITEM_STATUS, BT_MESH_LEN_MIN(2), handle_status },

	BT_MESH_MODEL_OP_END
};

void bt_mesh_sol_pdu_rpl_cli_timeout_set(int32_t timeout)
{
	msg_timeout = timeout;
}

static int sol_pdu_rpl_cli_init(const struct bt_mesh_model *mod)
{
	if (!bt_mesh_model_in_primary(mod)) {
		LOG_ERR("Solicitation PDU RPL Configuration client not in primary element");
		return -EINVAL;
	}

	msg_timeout = CONFIG_BT_MESH_SOL_PDU_RPL_CLI_TIMEOUT;

	cli = mod->user_data;
	cli->model = mod;
	bt_mesh_msg_ack_ctx_init(&cli->ack_ctx);
	return 0;
}

const struct bt_mesh_model_cb _bt_mesh_sol_pdu_rpl_cli_cb = {
	.init = sol_pdu_rpl_cli_init,
};
