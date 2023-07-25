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
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/mesh.h>

#include "mesh.h"
#include "adv.h"
#include "net.h"
#include "transport.h"
#include "access.h"
#include "foundation.h"

#define LOG_LEVEL CONFIG_BT_MESH_MODEL_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_mesh_health_srv);

#define HEALTH_TEST_STANDARD 0x00

/* Health Server context of the primary element */
struct bt_mesh_health_srv *health_srv;

static void health_get_registered(struct bt_mesh_model *mod,
				  uint16_t company_id,
				  struct net_buf_simple *msg)
{
	struct bt_mesh_health_srv *srv = mod->user_data;
	uint8_t *test_id;

	LOG_DBG("Company ID 0x%04x", company_id);

	bt_mesh_model_msg_init(msg, OP_HEALTH_FAULT_STATUS);

	test_id = net_buf_simple_add(msg, 1);
	net_buf_simple_add_le16(msg, company_id);

	if (srv->cb && srv->cb->fault_get_reg) {
		uint8_t fault_count = net_buf_simple_tailroom(msg) - 4;
		int err;

		err = srv->cb->fault_get_reg(mod, company_id, test_id,
					     net_buf_simple_tail(msg),
					     &fault_count);
		if (err) {
			LOG_ERR("Failed to get faults (err %d)", err);
			*test_id = HEALTH_TEST_STANDARD;
		} else {
			net_buf_simple_add(msg, fault_count);
		}
	} else {
		LOG_WRN("No callback for getting faults");
		*test_id = HEALTH_TEST_STANDARD;
	}
}

static size_t health_get_current(struct bt_mesh_model *mod,
				 struct net_buf_simple *msg)
{
	struct bt_mesh_health_srv *srv = mod->user_data;
	const struct bt_mesh_comp *comp;
	uint8_t *test_id, *company_ptr;
	uint16_t company_id;
	uint8_t fault_count;

	bt_mesh_model_msg_init(msg, OP_HEALTH_CURRENT_STATUS);

	test_id = net_buf_simple_add(msg, 1);
	company_ptr = net_buf_simple_add(msg, sizeof(company_id));
	comp = bt_mesh_comp_get();

	if (srv->cb && srv->cb->fault_get_cur) {
		fault_count = net_buf_simple_tailroom(msg);
		int err;

		err = srv->cb->fault_get_cur(mod, test_id, &company_id,
					     net_buf_simple_tail(msg),
					     &fault_count);
		if (err) {
			LOG_ERR("Failed to get faults (err %d)", err);
			sys_put_le16(comp->cid, company_ptr);
			*test_id = HEALTH_TEST_STANDARD;
			fault_count = 0U;
		} else {
			sys_put_le16(company_id, company_ptr);
			net_buf_simple_add(msg, fault_count);
		}
	} else {
		LOG_WRN("No callback for getting faults");
		sys_put_le16(comp->cid, company_ptr);
		*test_id = HEALTH_TEST_STANDARD;
		fault_count = 0U;
	}

	return fault_count;
}

static int health_fault_get(struct bt_mesh_model *model,
			    struct bt_mesh_msg_ctx *ctx,
			    struct net_buf_simple *buf)
{
	NET_BUF_SIMPLE_DEFINE(sdu, BT_MESH_TX_SDU_MAX);
	uint16_t company_id;

	company_id = net_buf_simple_pull_le16(buf);

	LOG_DBG("company_id 0x%04x", company_id);

	health_get_registered(model, company_id, &sdu);

	if (bt_mesh_model_send(model, ctx, &sdu, NULL, NULL)) {
		LOG_ERR("Unable to send Health Current Status response");
	}

	return 0;
}

static int health_fault_clear_unrel(struct bt_mesh_model *model,
				    struct bt_mesh_msg_ctx *ctx,
				    struct net_buf_simple *buf)
{
	struct bt_mesh_health_srv *srv = model->user_data;
	uint16_t company_id;

	company_id = net_buf_simple_pull_le16(buf);

	LOG_DBG("company_id 0x%04x", company_id);

	if (srv->cb && srv->cb->fault_clear) {
		return srv->cb->fault_clear(model, company_id);
	}

	return 0;
}

static int health_fault_clear(struct bt_mesh_model *model,
			      struct bt_mesh_msg_ctx *ctx,
			      struct net_buf_simple *buf)
{
	NET_BUF_SIMPLE_DEFINE(sdu, BT_MESH_TX_SDU_MAX);
	struct bt_mesh_health_srv *srv = model->user_data;
	uint16_t company_id;

	company_id = net_buf_simple_pull_le16(buf);

	LOG_DBG("company_id 0x%04x", company_id);

	if (srv->cb && srv->cb->fault_clear) {
		int err;

		err = srv->cb->fault_clear(model, company_id);
		if (err) {
			return err;
		}
	}

	health_get_registered(model, company_id, &sdu);

	if (bt_mesh_model_send(model, ctx, &sdu, NULL, NULL)) {
		LOG_ERR("Unable to send Health Current Status response");
	}

	return 0;
}

static int health_fault_test_unrel(struct bt_mesh_model *model,
				    struct bt_mesh_msg_ctx *ctx,
				    struct net_buf_simple *buf)
{
	struct bt_mesh_health_srv *srv = model->user_data;
	uint16_t company_id;
	uint8_t test_id;

	test_id = net_buf_simple_pull_u8(buf);
	company_id = net_buf_simple_pull_le16(buf);

	LOG_DBG("test 0x%02x company 0x%04x", test_id, company_id);

	if (srv->cb && srv->cb->fault_test) {
		return srv->cb->fault_test(model, test_id, company_id);
	}

	return 0;
}

static int health_fault_test(struct bt_mesh_model *model,
			     struct bt_mesh_msg_ctx *ctx,
			     struct net_buf_simple *buf)
{
	NET_BUF_SIMPLE_DEFINE(sdu, BT_MESH_TX_SDU_MAX);
	struct bt_mesh_health_srv *srv = model->user_data;
	uint16_t company_id;
	uint8_t test_id;

	LOG_DBG("");

	test_id = net_buf_simple_pull_u8(buf);
	company_id = net_buf_simple_pull_le16(buf);

	LOG_DBG("test 0x%02x company 0x%04x", test_id, company_id);

	if (srv->cb && srv->cb->fault_test) {
		int err;

		err = srv->cb->fault_test(model, test_id, company_id);
		if (err) {
			LOG_WRN("Running fault test failed with err %d", err);
			return err;
		}
	}

	health_get_registered(model, company_id, &sdu);

	if (bt_mesh_model_send(model, ctx, &sdu, NULL, NULL)) {
		LOG_ERR("Unable to send Health Current Status response");
	}

	return 0;
}

static int send_attention_status(struct bt_mesh_model *model,
				 struct bt_mesh_msg_ctx *ctx)
{
	/* Needed size: opcode (2 bytes) + msg + MIC */
	BT_MESH_MODEL_BUF_DEFINE(msg, OP_ATTENTION_STATUS, 1);
	struct bt_mesh_health_srv *srv = model->user_data;
	uint8_t time;

	time = k_ticks_to_ms_floor32(
		k_work_delayable_remaining_get(&srv->attn_timer)) / 1000U;
	LOG_DBG("%u second%s", time, (time == 1U) ? "" : "s");

	bt_mesh_model_msg_init(&msg, OP_ATTENTION_STATUS);

	net_buf_simple_add_u8(&msg, time);

	if (bt_mesh_model_send(model, ctx, &msg, NULL, NULL)) {
		LOG_ERR("Unable to send Attention Status");
	}

	return 0;
}

static int attention_get(struct bt_mesh_model *model,
			 struct bt_mesh_msg_ctx *ctx,
			 struct net_buf_simple *buf)
{
	LOG_DBG("");

	return send_attention_status(model, ctx);
}

static int attention_set_unrel(struct bt_mesh_model *model,
			       struct bt_mesh_msg_ctx *ctx,
			       struct net_buf_simple *buf)
{
	uint8_t time;

	time = net_buf_simple_pull_u8(buf);

	LOG_DBG("%u second%s", time, (time == 1U) ? "" : "s");

	bt_mesh_attention(model, time);

	return 0;
}

static int attention_set(struct bt_mesh_model *model,
			 struct bt_mesh_msg_ctx *ctx,
			 struct net_buf_simple *buf)
{
	int err;

	LOG_DBG("");

	err = attention_set_unrel(model, ctx, buf);
	if (err) {
		return err;
	}

	return send_attention_status(model, ctx);
}

static int send_health_period_status(struct bt_mesh_model *model,
				     struct bt_mesh_msg_ctx *ctx)
{
	/* Needed size: opcode (2 bytes) + msg + MIC */
	BT_MESH_MODEL_BUF_DEFINE(msg, OP_HEALTH_PERIOD_STATUS, 1);

	bt_mesh_model_msg_init(&msg, OP_HEALTH_PERIOD_STATUS);

	net_buf_simple_add_u8(&msg, model->pub->period_div);

	if (bt_mesh_model_send(model, ctx, &msg, NULL, NULL)) {
		LOG_ERR("Unable to send Health Period Status");
	}

	return 0;
}

static int health_period_get(struct bt_mesh_model *model,
			      struct bt_mesh_msg_ctx *ctx,
			      struct net_buf_simple *buf)
{
	LOG_DBG("");

	return send_health_period_status(model, ctx);
}

static int health_period_set_unrel(struct bt_mesh_model *model,
				    struct bt_mesh_msg_ctx *ctx,
				    struct net_buf_simple *buf)
{
	uint8_t period;

	period = net_buf_simple_pull_u8(buf);
	if (period > 15) {
		LOG_WRN("Prohibited period value %u", period);
		return -EINVAL;
	}

	LOG_DBG("period %u", period);

	model->pub->period_div = period;

	return 0;
}

static int health_period_set(struct bt_mesh_model *model,
			     struct bt_mesh_msg_ctx *ctx,
			     struct net_buf_simple *buf)
{
	int err;

	LOG_DBG("");

	err = health_period_set_unrel(model, ctx, buf);
	if (err) {
		return err;
	}

	return send_health_period_status(model, ctx);
}

const struct bt_mesh_model_op bt_mesh_health_srv_op[] = {
	{ OP_HEALTH_FAULT_GET,         BT_MESH_LEN_EXACT(2),   health_fault_get },
	{ OP_HEALTH_FAULT_CLEAR,       BT_MESH_LEN_EXACT(2),   health_fault_clear },
	{ OP_HEALTH_FAULT_CLEAR_UNREL, BT_MESH_LEN_EXACT(2),   health_fault_clear_unrel },
	{ OP_HEALTH_FAULT_TEST,        BT_MESH_LEN_EXACT(3),   health_fault_test },
	{ OP_HEALTH_FAULT_TEST_UNREL,  BT_MESH_LEN_EXACT(3),   health_fault_test_unrel },
	{ OP_HEALTH_PERIOD_GET,        BT_MESH_LEN_EXACT(0),   health_period_get },
	{ OP_HEALTH_PERIOD_SET,        BT_MESH_LEN_EXACT(1),   health_period_set },
	{ OP_HEALTH_PERIOD_SET_UNREL,  BT_MESH_LEN_EXACT(1),   health_period_set_unrel },
	{ OP_ATTENTION_GET,            BT_MESH_LEN_EXACT(0),   attention_get },
	{ OP_ATTENTION_SET,            BT_MESH_LEN_EXACT(1),   attention_set },
	{ OP_ATTENTION_SET_UNREL,      BT_MESH_LEN_EXACT(1),   attention_set_unrel },
	BT_MESH_MODEL_OP_END,
};

static int health_pub_update(struct bt_mesh_model *mod)
{
	struct bt_mesh_model_pub *pub = mod->pub;
	size_t count;

	LOG_DBG("");

	count = health_get_current(mod, pub->msg);
	if (count) {
		pub->fast_period = 1U;
	} else {
		pub->fast_period = 0U;
	}

	return 0;
}

int bt_mesh_health_srv_fault_update(struct bt_mesh_elem *elem)
{
	struct bt_mesh_model *mod;

	mod = bt_mesh_model_find(elem, BT_MESH_MODEL_ID_HEALTH_SRV);
	if (!mod) {
		return -EINVAL;
	}

	/* Let periodic publishing, if enabled, take care of sending the
	 * Health Current Status.
	 */
	if (bt_mesh_model_pub_period_get(mod) > 0) {
		return 0;
	}

	health_pub_update(mod);

	return bt_mesh_model_publish(mod);
}

static void attention_off(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct bt_mesh_health_srv *srv = CONTAINER_OF(dwork,
						      struct bt_mesh_health_srv,
						      attn_timer);
	LOG_DBG("");

	if (srv->cb && srv->cb->attn_off) {
		srv->cb->attn_off(srv->model);
	}
}

static int health_srv_init(struct bt_mesh_model *model)
{
	struct bt_mesh_health_srv *srv = model->user_data;

	if (!srv) {
		LOG_ERR("No Health Server context provided");
		return -EINVAL;
	}

	if (!model->pub) {
		LOG_ERR("Health Server has no publication support");
		return -EINVAL;
	}

	model->pub->update = health_pub_update;

	k_work_init_delayable(&srv->attn_timer, attention_off);

	srv->model = model;

	if (bt_mesh_model_in_primary(model)) {
		health_srv = srv;
	}

	return 0;
}

const struct bt_mesh_model_cb bt_mesh_health_srv_cb = {
	.init = health_srv_init,
};

void bt_mesh_attention(struct bt_mesh_model *model, uint8_t time)
{
	struct bt_mesh_health_srv *srv;

	if (!model) {
		srv = health_srv;
		if (!srv) {
			LOG_WRN("No Health Server available");
			return;
		}

		model = srv->model;
	} else {
		srv = model->user_data;
	}

	if ((time > 0) && srv->cb && srv->cb->attn_on) {
		srv->cb->attn_on(model);
	}

	k_work_reschedule(&srv->attn_timer, K_SECONDS(time));
}
