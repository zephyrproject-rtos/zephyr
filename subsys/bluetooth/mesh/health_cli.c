/*
 * Copyright (c) 2017 Intel Corporation
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

#include "common/bt_str.h"

#include "net.h"
#include "foundation.h"
#include "msg.h"

#define LOG_LEVEL CONFIG_BT_MESH_MODEL_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_mesh_health_cli);

static int32_t msg_timeout;

struct health_fault_param {
	uint16_t   cid;
	uint8_t   *expect_test_id;
	uint8_t   *test_id;
	uint8_t   *faults;
	size_t *fault_count;
};

static int health_fault_status(const struct bt_mesh_model *model,
			       struct bt_mesh_msg_ctx *ctx,
			       struct net_buf_simple *buf)
{
	struct bt_mesh_health_cli *cli = model->user_data;
	struct health_fault_param *param;
	uint8_t test_id;
	uint16_t cid;

	LOG_DBG("net_idx 0x%04x app_idx 0x%04x src 0x%04x len %u: %s", ctx->net_idx, ctx->app_idx,
		ctx->addr, buf->len, bt_hex(buf->data, buf->len));

	test_id = net_buf_simple_pull_u8(buf);
	cid = net_buf_simple_pull_le16(buf);

	if (bt_mesh_msg_ack_ctx_match(&cli->ack_ctx,
				      OP_HEALTH_FAULT_STATUS, ctx->addr,
				      (void **)&param)) {
		if (param->expect_test_id &&
		    (test_id != *param->expect_test_id)) {
			goto done;
		}

		if (cid != param->cid) {
			goto done;
		}

		if (param->test_id) {
			*param->test_id = test_id;
		}

		if (param->faults && param->fault_count) {
			if (buf->len > *param->fault_count) {
				LOG_WRN("Got more faults than there's space for");
			} else {
				*param->fault_count = buf->len;
			}

			memcpy(param->faults, buf->data, *param->fault_count);
		}

		bt_mesh_msg_ack_ctx_rx(&cli->ack_ctx);
	}

done:
	if (cli->fault_status) {
		cli->fault_status(cli, ctx->addr, test_id, cid,
					 buf->data, buf->len);
	}

	return 0;
}

static int health_current_status(const struct bt_mesh_model *model,
				 struct bt_mesh_msg_ctx *ctx,
				 struct net_buf_simple *buf)
{
	struct bt_mesh_health_cli *cli = model->user_data;
	uint8_t test_id;
	uint16_t cid;

	LOG_DBG("net_idx 0x%04x app_idx 0x%04x src 0x%04x len %u: %s", ctx->net_idx, ctx->app_idx,
		ctx->addr, buf->len, bt_hex(buf->data, buf->len));

	test_id = net_buf_simple_pull_u8(buf);
	cid = net_buf_simple_pull_le16(buf);

	LOG_DBG("Test ID 0x%02x Company ID 0x%04x Fault Count %u", test_id, cid, buf->len);

	if (cli->current_status) {
		cli->current_status(cli, ctx->addr, test_id, cid,
					   buf->data, buf->len);
	}

	return 0;
}

struct health_period_param {
	uint8_t *divisor;
};

static int health_period_status(const struct bt_mesh_model *model,
				struct bt_mesh_msg_ctx *ctx,
				struct net_buf_simple *buf)
{
	struct bt_mesh_health_cli *cli = model->user_data;
	struct health_period_param *param;
	uint8_t divisor;

	LOG_DBG("net_idx 0x%04x app_idx 0x%04x src 0x%04x len %u: %s", ctx->net_idx, ctx->app_idx,
		ctx->addr, buf->len, bt_hex(buf->data, buf->len));

	divisor = net_buf_simple_pull_u8(buf);

	if (bt_mesh_msg_ack_ctx_match(&cli->ack_ctx,
				      OP_HEALTH_PERIOD_STATUS, ctx->addr,
				      (void **)&param)) {
		if (param->divisor) {
			*param->divisor = divisor;
		}

		bt_mesh_msg_ack_ctx_rx(&cli->ack_ctx);
	}

	if (cli->period_status) {
		cli->period_status(cli, ctx->addr, divisor);
	}

	return 0;
}

struct health_attention_param {
	uint8_t *attention;
};

static int health_attention_status(const struct bt_mesh_model *model,
				   struct bt_mesh_msg_ctx *ctx,
				   struct net_buf_simple *buf)
{
	struct bt_mesh_health_cli *cli = model->user_data;
	struct health_attention_param *param;
	uint8_t attention;

	LOG_DBG("net_idx 0x%04x app_idx 0x%04x src 0x%04x len %u: %s", ctx->net_idx, ctx->app_idx,
		ctx->addr, buf->len, bt_hex(buf->data, buf->len));

	attention = net_buf_simple_pull_u8(buf);

	if (bt_mesh_msg_ack_ctx_match(&cli->ack_ctx, OP_ATTENTION_STATUS,
				      ctx->addr, (void **)&param)) {
		if (param->attention) {
			*param->attention = attention;
		}

		bt_mesh_msg_ack_ctx_rx(&cli->ack_ctx);
	}

	if (cli->attention_status) {
		cli->attention_status(cli, ctx->addr, attention);
	}
	return 0;
}

const struct bt_mesh_model_op bt_mesh_health_cli_op[] = {
	{ OP_HEALTH_FAULT_STATUS,     BT_MESH_LEN_MIN(3),    health_fault_status },
	{ OP_HEALTH_CURRENT_STATUS,   BT_MESH_LEN_MIN(3),    health_current_status },
	{ OP_HEALTH_PERIOD_STATUS,    BT_MESH_LEN_EXACT(1),  health_period_status },
	{ OP_ATTENTION_STATUS,        BT_MESH_LEN_EXACT(1),  health_attention_status },
	BT_MESH_MODEL_OP_END,
};

int bt_mesh_health_cli_attention_get(struct bt_mesh_health_cli *cli, struct bt_mesh_msg_ctx *ctx,
				     uint8_t *attention)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, OP_ATTENTION_GET, 0);
	struct health_attention_param param = {
		.attention = attention,
	};

	bt_mesh_model_msg_init(&msg, OP_ATTENTION_GET);

	const struct bt_mesh_msg_rsp_ctx rsp = {
		.ack = &cli->ack_ctx,
		.op = OP_ATTENTION_STATUS,
		.user_data = &param,
		.timeout = msg_timeout,
	};

	return bt_mesh_msg_ackd_send(cli->model, ctx, &msg, attention ? &rsp : NULL);
}

int bt_mesh_health_cli_attention_set(struct bt_mesh_health_cli *cli, struct bt_mesh_msg_ctx *ctx,
				     uint8_t attention, uint8_t *updated_attention)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, OP_ATTENTION_SET, 1);
	struct health_attention_param param = {
		.attention = updated_attention,
	};

	bt_mesh_model_msg_init(&msg, OP_ATTENTION_SET);
	net_buf_simple_add_u8(&msg, attention);

	const struct bt_mesh_msg_rsp_ctx rsp = {
		.ack = &cli->ack_ctx,
		.op = OP_ATTENTION_STATUS,
		.user_data = &param,
		.timeout = msg_timeout,
	};

	return bt_mesh_msg_ackd_send(cli->model, ctx, &msg, updated_attention ? &rsp : NULL);
}

int bt_mesh_health_cli_attention_set_unack(struct bt_mesh_health_cli *cli,
					   struct bt_mesh_msg_ctx *ctx, uint8_t attention)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, OP_ATTENTION_SET_UNREL, 1);

	bt_mesh_model_msg_init(&msg, OP_ATTENTION_SET_UNREL);
	net_buf_simple_add_u8(&msg, attention);

	return bt_mesh_msg_send(cli->model, ctx, &msg);
}

int bt_mesh_health_cli_period_get(struct bt_mesh_health_cli *cli, struct bt_mesh_msg_ctx *ctx,
				  uint8_t *divisor)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, OP_HEALTH_PERIOD_GET, 0);
	struct health_period_param param = {
		.divisor = divisor,
	};

	bt_mesh_model_msg_init(&msg, OP_HEALTH_PERIOD_GET);

	const struct bt_mesh_msg_rsp_ctx rsp = {
		.ack = &cli->ack_ctx,
		.op = OP_HEALTH_PERIOD_STATUS,
		.user_data = &param,
		.timeout = msg_timeout,
	};

	return bt_mesh_msg_ackd_send(cli->model, ctx, &msg, divisor ? &rsp : NULL);
}

int bt_mesh_health_cli_period_set(struct bt_mesh_health_cli *cli, struct bt_mesh_msg_ctx *ctx,
				  uint8_t divisor, uint8_t *updated_divisor)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, OP_HEALTH_PERIOD_SET, 1);
	struct health_period_param param = {
		.divisor = updated_divisor,
	};

	bt_mesh_model_msg_init(&msg, OP_HEALTH_PERIOD_SET);
	net_buf_simple_add_u8(&msg, divisor);

	const struct bt_mesh_msg_rsp_ctx rsp = {
		.ack = &cli->ack_ctx,
		.op = OP_HEALTH_PERIOD_STATUS,
		.user_data = &param,
		.timeout = msg_timeout,
	};

	return bt_mesh_msg_ackd_send(cli->model, ctx, &msg, updated_divisor ? &rsp : NULL);
}

int bt_mesh_health_cli_period_set_unack(struct bt_mesh_health_cli *cli,
					struct bt_mesh_msg_ctx *ctx, uint8_t divisor)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, OP_HEALTH_PERIOD_SET_UNREL, 1);

	bt_mesh_model_msg_init(&msg, OP_HEALTH_PERIOD_SET_UNREL);
	net_buf_simple_add_u8(&msg, divisor);

	return bt_mesh_msg_send(cli->model, ctx, &msg);
}

int bt_mesh_health_cli_fault_test(struct bt_mesh_health_cli *cli, struct bt_mesh_msg_ctx *ctx,
				  uint16_t cid, uint8_t test_id, uint8_t *faults,
				  size_t *fault_count)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, OP_HEALTH_FAULT_TEST, 3);
	struct health_fault_param param = {
		.cid = cid,
		.expect_test_id = &test_id,
		.faults = faults,
		.fault_count = fault_count,
	};

	bt_mesh_model_msg_init(&msg, OP_HEALTH_FAULT_TEST);
	net_buf_simple_add_u8(&msg, test_id);
	net_buf_simple_add_le16(&msg, cid);

	const struct bt_mesh_msg_rsp_ctx rsp = {
		.ack = &cli->ack_ctx,
		.op = OP_HEALTH_FAULT_STATUS,
		.user_data = &param,
		.timeout = msg_timeout,
	};

	return bt_mesh_msg_ackd_send(cli->model, ctx, &msg, &rsp);
}

int bt_mesh_health_cli_fault_test_unack(struct bt_mesh_health_cli *cli,
					struct bt_mesh_msg_ctx *ctx, uint16_t cid, uint8_t test_id)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, OP_HEALTH_FAULT_TEST_UNREL, 3);

	bt_mesh_model_msg_init(&msg, OP_HEALTH_FAULT_TEST_UNREL);
	net_buf_simple_add_u8(&msg, test_id);
	net_buf_simple_add_le16(&msg, cid);

	return bt_mesh_msg_send(cli->model, ctx, &msg);
}

int bt_mesh_health_cli_fault_clear(struct bt_mesh_health_cli *cli, struct bt_mesh_msg_ctx *ctx,
				   uint16_t cid, uint8_t *test_id, uint8_t *faults,
				   size_t *fault_count)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, OP_HEALTH_FAULT_CLEAR, 2);
	struct health_fault_param param = {
		.cid = cid,
		.test_id = test_id,
		.faults = faults,
		.fault_count = fault_count,
	};

	bt_mesh_model_msg_init(&msg, OP_HEALTH_FAULT_CLEAR);
	net_buf_simple_add_le16(&msg, cid);

	const struct bt_mesh_msg_rsp_ctx rsp = {
		.ack = &cli->ack_ctx,
		.op = OP_HEALTH_FAULT_STATUS,
		.user_data = &param,
		.timeout = msg_timeout,
	};

	return bt_mesh_msg_ackd_send(cli->model, ctx, &msg,
				     (!test_id && (!faults || !fault_count)) ? NULL : &rsp);
}

int bt_mesh_health_cli_fault_clear_unack(struct bt_mesh_health_cli *cli,
					 struct bt_mesh_msg_ctx *ctx, uint16_t cid)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, OP_HEALTH_FAULT_CLEAR_UNREL, 2);

	bt_mesh_model_msg_init(&msg, OP_HEALTH_FAULT_CLEAR_UNREL);
	net_buf_simple_add_le16(&msg, cid);

	return bt_mesh_msg_send(cli->model, ctx, &msg);
}

int bt_mesh_health_cli_fault_get(struct bt_mesh_health_cli *cli, struct bt_mesh_msg_ctx *ctx,
				 uint16_t cid, uint8_t *test_id, uint8_t *faults,
				 size_t *fault_count)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, OP_HEALTH_FAULT_GET, 2);
	struct health_fault_param param = {
		.cid = cid,
		.test_id = test_id,
		.faults = faults,
		.fault_count = fault_count,
	};

	bt_mesh_model_msg_init(&msg, OP_HEALTH_FAULT_GET);
	net_buf_simple_add_le16(&msg, cid);

	const struct bt_mesh_msg_rsp_ctx rsp = {
		.ack = &cli->ack_ctx,
		.op = OP_HEALTH_FAULT_STATUS,
		.user_data = &param,
		.timeout = msg_timeout,
	};

	return bt_mesh_msg_ackd_send(cli->model, ctx, &msg,
				     (!test_id && (!faults || !fault_count)) ? NULL : &rsp);
}

int32_t bt_mesh_health_cli_timeout_get(void)
{
	return msg_timeout;
}

void bt_mesh_health_cli_timeout_set(int32_t timeout)
{
	msg_timeout = timeout;
}

static int health_cli_init(const struct bt_mesh_model *model)
{
	struct bt_mesh_health_cli *cli = model->user_data;

	LOG_DBG("primary %u", bt_mesh_model_in_primary(model));

	if (!cli) {
		LOG_ERR("No Health Client context provided");
		return -EINVAL;
	}

	cli->model = model;
	msg_timeout = CONFIG_BT_MESH_HEALTH_CLI_TIMEOUT;

	cli->pub.msg = &cli->pub_buf;
	net_buf_simple_init_with_data(&cli->pub_buf, cli->pub_data, sizeof(cli->pub_data));

	bt_mesh_msg_ack_ctx_init(&cli->ack_ctx);
	return 0;
}

static void health_cli_reset(const struct bt_mesh_model *model)
{
	struct bt_mesh_health_cli *cli = model->user_data;

	net_buf_simple_reset(cli->pub.msg);
}

const struct bt_mesh_model_cb bt_mesh_health_cli_cb = {
	.init = health_cli_init,
	.reset = health_cli_reset,
};
