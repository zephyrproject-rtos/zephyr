/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <zephyr/types.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/mesh.h>

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_MESH_DEBUG_MODEL)
#define LOG_MODULE_NAME bt_mesh_health_cli
#include "common/log.h"

#include "net.h"
#include "foundation.h"

static int32_t msg_timeout;

static struct bt_mesh_health_cli *health_cli;

struct health_fault_param {
	uint16_t   cid;
	uint8_t   *expect_test_id;
	uint8_t   *test_id;
	uint8_t   *faults;
	size_t *fault_count;
};

static int health_fault_status(struct bt_mesh_model *model,
			       struct bt_mesh_msg_ctx *ctx,
			       struct net_buf_simple *buf)
{
	struct health_fault_param *param;
	uint8_t test_id;
	uint16_t cid;

	BT_DBG("net_idx 0x%04x app_idx 0x%04x src 0x%04x len %u: %s",
	       ctx->net_idx, ctx->app_idx, ctx->addr, buf->len,
	       bt_hex(buf->data, buf->len));

	test_id = net_buf_simple_pull_u8(buf);
	cid = net_buf_simple_pull_le16(buf);

	if (bt_mesh_msg_ack_ctx_match(&health_cli->ack_ctx,
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
				BT_WARN("Got more faults than there's space for");
			} else {
				*param->fault_count = buf->len;
			}

			memcpy(param->faults, buf->data, *param->fault_count);
		}

		bt_mesh_msg_ack_ctx_rx(&health_cli->ack_ctx);
	}

done:
	if (health_cli->fault_status) {
		health_cli->fault_status(health_cli, ctx->addr, test_id, cid,
					 buf->data, buf->len);
	}

	return 0;
}

static int health_current_status(struct bt_mesh_model *model,
				 struct bt_mesh_msg_ctx *ctx,
				 struct net_buf_simple *buf)
{
	uint8_t test_id;
	uint16_t cid;

	BT_DBG("net_idx 0x%04x app_idx 0x%04x src 0x%04x len %u: %s",
	       ctx->net_idx, ctx->app_idx, ctx->addr, buf->len,
	       bt_hex(buf->data, buf->len));

	test_id = net_buf_simple_pull_u8(buf);
	cid = net_buf_simple_pull_le16(buf);

	BT_DBG("Test ID 0x%02x Company ID 0x%04x Fault Count %u", test_id, cid,
	       buf->len);

	if (health_cli->current_status) {
		health_cli->current_status(health_cli, ctx->addr, test_id, cid,
					   buf->data, buf->len);
	}

	return 0;
}

struct health_period_param {
	uint8_t *divisor;
};

static int health_period_status(struct bt_mesh_model *model,
				struct bt_mesh_msg_ctx *ctx,
				struct net_buf_simple *buf)
{
	struct health_period_param *param;
	uint8_t divisor;

	BT_DBG("net_idx 0x%04x app_idx 0x%04x src 0x%04x len %u: %s",
	       ctx->net_idx, ctx->app_idx, ctx->addr, buf->len,
	       bt_hex(buf->data, buf->len));

	divisor = net_buf_simple_pull_u8(buf);

	if (bt_mesh_msg_ack_ctx_match(&health_cli->ack_ctx,
				      OP_HEALTH_PERIOD_STATUS, ctx->addr,
				      (void **)&param)) {
		if (param->divisor) {
			*param->divisor = divisor;
		}

		bt_mesh_msg_ack_ctx_rx(&health_cli->ack_ctx);
	}

	if (health_cli->period_status) {
		health_cli->period_status(health_cli, ctx->addr, divisor);
	}

	return 0;
}

struct health_attention_param {
	uint8_t *attention;
};

static int health_attention_status(struct bt_mesh_model *model,
				   struct bt_mesh_msg_ctx *ctx,
				   struct net_buf_simple *buf)
{
	struct health_attention_param *param;
	uint8_t attention;

	BT_DBG("net_idx 0x%04x app_idx 0x%04x src 0x%04x len %u: %s",
	       ctx->net_idx, ctx->app_idx, ctx->addr, buf->len,
	       bt_hex(buf->data, buf->len));

	attention = net_buf_simple_pull_u8(buf);

	if (bt_mesh_msg_ack_ctx_match(&health_cli->ack_ctx, OP_ATTENTION_STATUS,
				      ctx->addr, (void **)&param)) {
		if (param->attention) {
			*param->attention = attention;
		}

		bt_mesh_msg_ack_ctx_rx(&health_cli->ack_ctx);
	}

	if (health_cli->attention_status) {
		health_cli->attention_status(health_cli, ctx->addr, attention);
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

static int cli_prepare(void *param, uint32_t op, uint16_t addr)
{
	return bt_mesh_msg_ack_ctx_prepare(&health_cli->ack_ctx, op, addr, param);
}

int bt_mesh_health_attention_get(uint16_t addr, uint16_t app_idx, uint8_t *attention)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, OP_ATTENTION_GET, 0);
	struct bt_mesh_msg_ctx ctx = {
		.app_idx = app_idx,
		.addr = addr,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	struct health_attention_param param = {
		.attention = attention,
	};
	int err;

	err = cli_prepare(&param, OP_ATTENTION_STATUS, addr);
	if (err) {
		return err;
	}

	bt_mesh_model_msg_init(&msg, OP_ATTENTION_GET);

	err = bt_mesh_model_send(health_cli->model, &ctx, &msg, NULL, NULL);
	if (err) {
		BT_ERR("model_send() failed (err %d)", err);
		bt_mesh_msg_ack_ctx_clear(&health_cli->ack_ctx);
		return err;
	}

	if (!attention) {
		bt_mesh_msg_ack_ctx_clear(&health_cli->ack_ctx);
		return 0;
	}

	return bt_mesh_msg_ack_ctx_wait(&health_cli->ack_ctx, K_MSEC(msg_timeout));
}

int bt_mesh_health_attention_set(uint16_t addr, uint16_t app_idx,
				 uint8_t attention, uint8_t *updated_attention)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, OP_ATTENTION_SET, 1);
	struct bt_mesh_msg_ctx ctx = {
		.app_idx = app_idx,
		.addr = addr,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	struct health_attention_param param = {
		.attention = updated_attention,
	};
	int err;

	err = cli_prepare(&param, OP_ATTENTION_STATUS, addr);
	if (err) {
		return err;
	}

	bt_mesh_model_msg_init(&msg, OP_ATTENTION_SET);
	net_buf_simple_add_u8(&msg, attention);

	err = bt_mesh_model_send(health_cli->model, &ctx, &msg, NULL, NULL);
	if (err) {
		BT_ERR("model_send() failed (err %d)", err);
		bt_mesh_msg_ack_ctx_clear(&health_cli->ack_ctx);
		return err;
	}

	if (!updated_attention) {
		bt_mesh_msg_ack_ctx_clear(&health_cli->ack_ctx);
		return 0;
	}

	return bt_mesh_msg_ack_ctx_wait(&health_cli->ack_ctx, K_MSEC(msg_timeout));
}

int bt_mesh_health_attention_set_unack(uint16_t addr, uint16_t app_idx,
				       uint8_t attention)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, OP_ATTENTION_SET_UNREL, 1);
	struct bt_mesh_msg_ctx ctx = {
		.app_idx = app_idx,
		.addr = addr,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	err = cli_prepare(NULL, OP_ATTENTION_STATUS, addr);
	if (err) {
		return err;
	}

	bt_mesh_model_msg_init(&msg, OP_ATTENTION_SET_UNREL);
	net_buf_simple_add_u8(&msg, attention);

	err = bt_mesh_model_send(health_cli->model, &ctx, &msg, NULL, NULL);
	if (err) {
		BT_ERR("model_send() failed (err %d)", err);
		bt_mesh_msg_ack_ctx_clear(&health_cli->ack_ctx);
		return err;
	}

	bt_mesh_msg_ack_ctx_clear(&health_cli->ack_ctx);
	return 0;
}

int bt_mesh_health_period_get(uint16_t addr, uint16_t app_idx, uint8_t *divisor)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, OP_HEALTH_PERIOD_GET, 0);
	struct bt_mesh_msg_ctx ctx = {
		.app_idx = app_idx,
		.addr = addr,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	struct health_period_param param = {
		.divisor = divisor,
	};
	int err;

	err = cli_prepare(&param, OP_HEALTH_PERIOD_STATUS, addr);
	if (err) {
		return err;
	}

	bt_mesh_model_msg_init(&msg, OP_HEALTH_PERIOD_GET);

	err = bt_mesh_model_send(health_cli->model, &ctx, &msg, NULL, NULL);
	if (err) {
		BT_ERR("model_send() failed (err %d)", err);
		bt_mesh_msg_ack_ctx_clear(&health_cli->ack_ctx);
		return err;
	}

	if (!divisor) {
		bt_mesh_msg_ack_ctx_clear(&health_cli->ack_ctx);
		return 0;
	}

	return bt_mesh_msg_ack_ctx_wait(&health_cli->ack_ctx,
					K_MSEC(msg_timeout));
}

int bt_mesh_health_period_set(uint16_t addr, uint16_t app_idx, uint8_t divisor,
			      uint8_t *updated_divisor)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, OP_HEALTH_PERIOD_SET, 1);
	struct bt_mesh_msg_ctx ctx = {
		.app_idx = app_idx,
		.addr = addr,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	struct health_period_param param = {
		.divisor = updated_divisor,
	};
	int err;

	err = cli_prepare(&param, OP_HEALTH_PERIOD_STATUS, addr);
	if (err) {
		return err;
	}

	bt_mesh_model_msg_init(&msg, OP_HEALTH_PERIOD_SET);
	net_buf_simple_add_u8(&msg, divisor);

	err = bt_mesh_model_send(health_cli->model, &ctx, &msg, NULL, NULL);
	if (err) {
		BT_ERR("model_send() failed (err %d)", err);
		bt_mesh_msg_ack_ctx_clear(&health_cli->ack_ctx);
		return err;
	}

	if (!updated_divisor) {
		bt_mesh_msg_ack_ctx_clear(&health_cli->ack_ctx);
		return 0;
	}

	return bt_mesh_msg_ack_ctx_wait(&health_cli->ack_ctx,
					K_MSEC(msg_timeout));
}

int bt_mesh_health_period_set_unack(uint16_t addr, uint16_t app_idx,
				    uint8_t divisor)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, OP_HEALTH_PERIOD_SET_UNREL, 1);
	struct bt_mesh_msg_ctx ctx = {
		.app_idx = app_idx,
		.addr = addr,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	err = cli_prepare(NULL, OP_HEALTH_PERIOD_STATUS, addr);
	if (err) {
		return err;
	}

	bt_mesh_model_msg_init(&msg, OP_HEALTH_PERIOD_SET_UNREL);
	net_buf_simple_add_u8(&msg, divisor);

	err = bt_mesh_model_send(health_cli->model, &ctx, &msg, NULL, NULL);
	if (err) {
		BT_ERR("model_send() failed (err %d)", err);
		bt_mesh_msg_ack_ctx_clear(&health_cli->ack_ctx);
		return err;
	}

	bt_mesh_msg_ack_ctx_clear(&health_cli->ack_ctx);
	return 0;
}

int bt_mesh_health_fault_test(uint16_t addr, uint16_t app_idx, uint16_t cid,
				 uint8_t test_id, uint8_t *faults,
				 size_t *fault_count)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, OP_HEALTH_FAULT_TEST, 3);
	struct bt_mesh_msg_ctx ctx = {
		.app_idx = app_idx,
		.addr = addr,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	struct health_fault_param param = {
		.cid = cid,
		.expect_test_id = &test_id,
		.faults = faults,
		.fault_count = fault_count,
	};
	int err;

	err = cli_prepare(&param, OP_HEALTH_FAULT_STATUS, addr);
	if (err) {
		return err;
	}

	bt_mesh_model_msg_init(&msg, OP_HEALTH_FAULT_TEST);
	net_buf_simple_add_u8(&msg, test_id);
	net_buf_simple_add_le16(&msg, cid);

	err = bt_mesh_model_send(health_cli->model, &ctx, &msg, NULL, NULL);
	if (err) {
		BT_ERR("model_send() failed (err %d)", err);
		bt_mesh_msg_ack_ctx_clear(&health_cli->ack_ctx);
		return err;
	}

	if (!faults || !fault_count) {
		bt_mesh_msg_ack_ctx_clear(&health_cli->ack_ctx);
		return 0;
	}

	return bt_mesh_msg_ack_ctx_wait(&health_cli->ack_ctx, K_MSEC(msg_timeout));
}

int bt_mesh_health_fault_test_unack(uint16_t addr, uint16_t app_idx,
				    uint16_t cid, uint8_t test_id)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, OP_HEALTH_FAULT_TEST, 3);
	struct bt_mesh_msg_ctx ctx = {
		.app_idx = app_idx,
		.addr = addr,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	err = cli_prepare(NULL, OP_HEALTH_FAULT_STATUS, addr);
	if (err) {
		return err;
	}

	bt_mesh_model_msg_init(&msg, OP_HEALTH_FAULT_TEST_UNREL);
	net_buf_simple_add_u8(&msg, test_id);
	net_buf_simple_add_le16(&msg, cid);

	err = bt_mesh_model_send(health_cli->model, &ctx, &msg, NULL, NULL);
	if (err) {
		BT_ERR("model_send() failed (err %d)", err);
		bt_mesh_msg_ack_ctx_clear(&health_cli->ack_ctx);
		return err;
	}

	bt_mesh_msg_ack_ctx_clear(&health_cli->ack_ctx);
	return 0;
}

int bt_mesh_health_fault_clear(uint16_t addr, uint16_t app_idx, uint16_t cid,
				 uint8_t *test_id, uint8_t *faults,
				 size_t *fault_count)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, OP_HEALTH_FAULT_CLEAR, 2);
	struct bt_mesh_msg_ctx ctx = {
		.app_idx = app_idx,
		.addr = addr,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	struct health_fault_param param = {
		.cid = cid,
		.test_id = test_id,
		.faults = faults,
		.fault_count = fault_count,
	};
	int err;

	err = cli_prepare(&param, OP_HEALTH_FAULT_STATUS, addr);
	if (err) {
		return err;
	}

	bt_mesh_model_msg_init(&msg, OP_HEALTH_FAULT_CLEAR);
	net_buf_simple_add_le16(&msg, cid);

	err = bt_mesh_model_send(health_cli->model, &ctx, &msg, NULL, NULL);
	if (err) {
		BT_ERR("model_send() failed (err %d)", err);
		bt_mesh_msg_ack_ctx_clear(&health_cli->ack_ctx);
		return err;
	}

	if (!test_id && (!faults || !fault_count)) {
		bt_mesh_msg_ack_ctx_clear(&health_cli->ack_ctx);
		return 0;
	}

	return bt_mesh_msg_ack_ctx_wait(&health_cli->ack_ctx, K_MSEC(msg_timeout));
}

int bt_mesh_health_fault_clear_unack(uint16_t addr, uint16_t app_idx,
				     uint16_t cid)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, OP_HEALTH_FAULT_CLEAR_UNREL, 2);
	struct bt_mesh_msg_ctx ctx = {
		.app_idx = app_idx,
		.addr = addr,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	err = cli_prepare(NULL, OP_HEALTH_FAULT_STATUS, addr);
	if (err) {
		return err;
	}

	bt_mesh_model_msg_init(&msg, OP_HEALTH_FAULT_CLEAR_UNREL);
	net_buf_simple_add_le16(&msg, cid);

	err = bt_mesh_model_send(health_cli->model, &ctx, &msg, NULL, NULL);
	if (err) {
		BT_ERR("model_send() failed (err %d)", err);
		bt_mesh_msg_ack_ctx_clear(&health_cli->ack_ctx);
		return err;
	}

	bt_mesh_msg_ack_ctx_clear(&health_cli->ack_ctx);
	return 0;
}

int bt_mesh_health_fault_get(uint16_t addr, uint16_t app_idx, uint16_t cid,
				 uint8_t *test_id, uint8_t *faults,
				 size_t *fault_count)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, OP_HEALTH_FAULT_GET, 2);
	struct bt_mesh_msg_ctx ctx = {
		.app_idx = app_idx,
		.addr = addr,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	struct health_fault_param param = {
		.cid = cid,
		.test_id = test_id,
		.faults = faults,
		.fault_count = fault_count,
	};
	int err;

	err = cli_prepare(&param, OP_HEALTH_FAULT_STATUS, addr);
	if (err) {
		return err;
	}

	bt_mesh_model_msg_init(&msg, OP_HEALTH_FAULT_GET);
	net_buf_simple_add_le16(&msg, cid);

	err = bt_mesh_model_send(health_cli->model, &ctx, &msg, NULL, NULL);
	if (err) {
		BT_ERR("model_send() failed (err %d)", err);
		bt_mesh_msg_ack_ctx_clear(&health_cli->ack_ctx);
		return err;
	}

	if (!test_id && (!faults || !fault_count)) {
		bt_mesh_msg_ack_ctx_clear(&health_cli->ack_ctx);
		return 0;
	}

	return bt_mesh_msg_ack_ctx_wait(&health_cli->ack_ctx, K_MSEC(msg_timeout));
}

int32_t bt_mesh_health_cli_timeout_get(void)
{
	return msg_timeout;
}

void bt_mesh_health_cli_timeout_set(int32_t timeout)
{
	msg_timeout = timeout;
}

int bt_mesh_health_cli_set(struct bt_mesh_model *model)
{
	if (!model->user_data) {
		BT_ERR("No Health Client context for given model");
		return -EINVAL;
	}

	health_cli = model->user_data;
	msg_timeout = CONFIG_BT_MESH_HEALTH_CLI_TIMEOUT;

	return 0;
}

static int health_cli_init(struct bt_mesh_model *model)
{
	struct bt_mesh_health_cli *cli = model->user_data;

	BT_DBG("primary %u", bt_mesh_model_in_primary(model));

	if (!cli) {
		BT_ERR("No Health Client context provided");
		return -EINVAL;
	}

	cli = model->user_data;
	cli->model = model;
	msg_timeout = 2 * MSEC_PER_SEC;

	/* Set the default health client pointer */
	if (!health_cli) {
		health_cli = cli;
	}

	bt_mesh_msg_ack_ctx_init(&health_cli->ack_ctx);
	return 0;
}

const struct bt_mesh_model_cb bt_mesh_health_cli_cb = {
	.init = health_cli_init,
};
