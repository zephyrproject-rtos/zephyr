/*  Bluetooth Mesh */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <zephyr/types.h>
#include <misc/byteorder.h>
#include <misc/util.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/mesh.h>

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BLUETOOTH_MESH_DEBUG_MODEL)
#include "common/log.h"

#include "mesh.h"
#include "adv.h"
#include "net.h"
#include "transport.h"
#include "access.h"
#include "foundation.h"

#define HEALTH_TEST_STANDARD 0x00

/* Increasing this requires also increasing the system workqueue */
#define MAX_FAULTS 32
#define HEALTH_STATUS_SIZE_MAX (1 + 3 + MAX_FAULTS + 4)

#if BT_MESH_TX_SDU_MAX < HEALTH_STATUS_SIZE_MAX
#define HEALTH_STATUS_SIZE BT_MESH_TX_SDU_MAX
#else
#define HEALTH_STATUS_SIZE HEALTH_STATUS_SIZE_MAX
#endif

/* Health Server context of the primary element */
struct bt_mesh_health *health_srv;

static void health_get_registered(struct bt_mesh_model *mod,
				  u16_t company_id,
				  struct net_buf_simple *msg)
{
	struct bt_mesh_health *srv = mod->user_data;
	u8_t fault_count;
	u8_t *test_id;
	int err;

	BT_DBG("Company ID 0x%04x", company_id);

	bt_mesh_model_msg_init(msg, OP_HEALTH_FAULT_STATUS);

	test_id = net_buf_simple_add(msg, 1);
	net_buf_simple_add_le16(msg, company_id);
	fault_count = net_buf_simple_tailroom(msg) - 4;

	if (srv->fault_get_reg) {
		err = srv->fault_get_reg(mod, company_id, test_id,
					 net_buf_simple_tail(msg),
					 &fault_count);
		if (err) {
			BT_ERR("Failed to get faults (err %d)", err);
			*test_id = HEALTH_TEST_STANDARD;
		} else {
			net_buf_simple_add(msg, fault_count);
		}
	} else {
		BT_WARN("No callback for getting faults");
		*test_id = HEALTH_TEST_STANDARD;
	}
}

static size_t health_get_current(struct bt_mesh_model *mod,
				 struct net_buf_simple *msg)
{
	struct bt_mesh_health *srv = mod->user_data;
	u8_t *test_id, *company_ptr;
	u16_t company_id;
	u8_t fault_count;
	int err;

	bt_mesh_model_msg_init(msg, OP_HEALTH_CURRENT_STATUS);

	test_id = net_buf_simple_add(msg, 1);
	company_ptr = net_buf_simple_add(msg, sizeof(company_id));

	fault_count = net_buf_simple_tailroom(msg) - 4;

	if (srv->fault_get_cur) {
		err = srv->fault_get_cur(mod, test_id, &company_id,
					 net_buf_simple_tail(msg),
					 &fault_count);
		if (err) {
			BT_ERR("Failed to get faults (err %d)", err);
			sys_put_le16(0, company_ptr);
			*test_id = HEALTH_TEST_STANDARD;
			fault_count = 0;
		} else {
			sys_put_le16(company_id, company_ptr);
		}
	} else {
		BT_WARN("No callback for getting faults");
		net_buf_simple_push_u8(msg, HEALTH_TEST_STANDARD);
		net_buf_simple_push_le16(msg, 0);
		fault_count = 0;
	}

	return fault_count;
}

static void health_fault_get(struct bt_mesh_model *model,
			     struct bt_mesh_msg_ctx *ctx,
			     struct net_buf_simple *buf)
{
	struct net_buf_simple *msg = NET_BUF_SIMPLE(HEALTH_STATUS_SIZE);
	u16_t company_id;

	company_id = net_buf_simple_pull_le16(buf);

	BT_DBG("company_id 0x%04x", company_id);

	health_get_registered(model, company_id, msg);

	if (bt_mesh_model_send(model, ctx, msg, NULL, NULL)) {
		BT_ERR("Unable to send Health Current Status response");
	}
}

static void health_fault_clear_unrel(struct bt_mesh_model *model,
				     struct bt_mesh_msg_ctx *ctx,
				     struct net_buf_simple *buf)
{
	struct bt_mesh_health *srv = model->user_data;
	u16_t company_id;

	company_id = net_buf_simple_pull_le16(buf);

	BT_DBG("company_id 0x%04x", company_id);

	if (srv->fault_clear) {
		srv->fault_clear(model, company_id);
	}
}

static void health_fault_clear(struct bt_mesh_model *model,
			       struct bt_mesh_msg_ctx *ctx,
			       struct net_buf_simple *buf)
{
	struct net_buf_simple *msg = NET_BUF_SIMPLE(HEALTH_STATUS_SIZE);
	struct bt_mesh_health *srv = model->user_data;
	u16_t company_id;

	company_id = net_buf_simple_pull_le16(buf);

	BT_DBG("company_id 0x%04x", company_id);

	if (srv->fault_clear) {
		srv->fault_clear(model, company_id);
	}

	health_get_registered(model, company_id, msg);

	if (bt_mesh_model_send(model, ctx, msg, NULL, NULL)) {
		BT_ERR("Unable to send Health Current Status response");
	}
}

static void health_fault_test_unrel(struct bt_mesh_model *model,
				    struct bt_mesh_msg_ctx *ctx,
				    struct net_buf_simple *buf)
{
	struct bt_mesh_health *srv = model->user_data;
	const struct bt_mesh_comp *comp;
	u16_t company_id;
	u8_t test_id;

	test_id = net_buf_simple_pull_u8(buf);
	company_id = net_buf_simple_pull_le16(buf);

	BT_DBG("test 0x%02x company 0x%04x", test_id, company_id);

	comp = bt_mesh_comp_get();
	if (comp->cid != company_id) {
		BT_WARN("CID 0x%04x doesn't match composition CID 0x%04x",
			company_id, comp->cid);
		return;
	}

	if (srv->fault_test) {
		srv->fault_test(model, test_id, company_id);
	}
}

static void health_fault_test(struct bt_mesh_model *model,
			      struct bt_mesh_msg_ctx *ctx,
			      struct net_buf_simple *buf)
{
	struct net_buf_simple *msg = NET_BUF_SIMPLE(HEALTH_STATUS_SIZE);
	struct bt_mesh_health *srv = model->user_data;
	u16_t company_id;
	u8_t test_id;

	BT_DBG("");

	test_id = net_buf_simple_pull_u8(buf);
	company_id = net_buf_simple_pull_le16(buf);

	BT_DBG("test 0x%02x company 0x%04x", test_id, company_id);

	if (srv->fault_test) {
		srv->fault_test(model, test_id, company_id);
	}

	health_get_registered(model, company_id, msg);

	if (bt_mesh_model_send(model, ctx, msg, NULL, NULL)) {
		BT_ERR("Unable to send Health Current Status response");
	}
}

static void send_attention_status(struct bt_mesh_model *model,
				  struct bt_mesh_msg_ctx *ctx)
{
	/* Needed size: opcode (2 bytes) + msg + MIC */
	struct net_buf_simple *msg = NET_BUF_SIMPLE(2 + 1 + 4);
	struct bt_mesh_health *srv = model->user_data;
	u8_t time;

	time = k_delayed_work_remaining_get(&srv->attention.timer) / 1000;
	BT_DBG("%u second%s", time, (time == 1) ? "" : "s");

	bt_mesh_model_msg_init(msg, OP_ATTENTION_STATUS);

	net_buf_simple_add_u8(msg, time);

	bt_mesh_model_send(model, ctx, msg, NULL, NULL);
}

static void attention_get(struct bt_mesh_model *model,
			  struct bt_mesh_msg_ctx *ctx,
			  struct net_buf_simple *buf)
{
	BT_DBG("");

	send_attention_status(model, ctx);
}

static void attention_set_unrel(struct bt_mesh_model *model,
				struct bt_mesh_msg_ctx *ctx,
				struct net_buf_simple *buf)
{
	u8_t time;

	time = net_buf_simple_pull_u8(buf);

	BT_DBG("%u second%s", time, (time == 1) ? "" : "s");

	bt_mesh_attention(model, time);
}

static void attention_set(struct bt_mesh_model *model,
			  struct bt_mesh_msg_ctx *ctx,
			  struct net_buf_simple *buf)
{
	BT_DBG("");

	attention_set_unrel(model, ctx, buf);

	send_attention_status(model, ctx);
}

static void send_health_period_status(struct bt_mesh_model *model,
				      struct bt_mesh_msg_ctx *ctx)
{
	/* Needed size: opcode (2 bytes) + msg + MIC */
	struct net_buf_simple *msg = NET_BUF_SIMPLE(2 + 1 + 4);
	struct bt_mesh_health *srv = model->user_data;

	bt_mesh_model_msg_init(msg, OP_HEALTH_PERIOD_STATUS);

	net_buf_simple_add_u8(msg, srv->period);

	bt_mesh_model_send(model, ctx, msg, NULL, NULL);
}

static void health_period_get(struct bt_mesh_model *model,
			      struct bt_mesh_msg_ctx *ctx,
			      struct net_buf_simple *buf)
{
	BT_DBG("");

	send_health_period_status(model, ctx);
}

static void health_period_set_unrel(struct bt_mesh_model *model,
				    struct bt_mesh_msg_ctx *ctx,
				    struct net_buf_simple *buf)
{
	struct bt_mesh_health *srv = model->user_data;
	u8_t period;

	period = net_buf_simple_pull_u8(buf);
	if (period > 15) {
		BT_WARN("Prohibited period value %u", period);
		return;
	}

	BT_DBG("period %u", period);

	srv->period = period;
}

static void health_period_set(struct bt_mesh_model *model,
			      struct bt_mesh_msg_ctx *ctx,
			      struct net_buf_simple *buf)
{
	BT_DBG("");

	health_period_set_unrel(model, ctx, buf);

	send_health_period_status(model, ctx);
}

const struct bt_mesh_model_op bt_mesh_health_op[] = {
	{ OP_HEALTH_FAULT_GET,         2,   health_fault_get },
	{ OP_HEALTH_FAULT_CLEAR,       2,   health_fault_clear },
	{ OP_HEALTH_FAULT_CLEAR_UNREL, 2,   health_fault_clear_unrel },
	{ OP_HEALTH_FAULT_TEST,        3,   health_fault_test },
	{ OP_HEALTH_FAULT_TEST_UNREL,  3,   health_fault_test_unrel },
	{ OP_HEALTH_PERIOD_GET,        0,   health_period_get },
	{ OP_HEALTH_PERIOD_SET,        1,   health_period_set },
	{ OP_HEALTH_PERIOD_SET_UNREL,  1,   health_period_set_unrel },
	{ OP_ATTENTION_GET,            0,   attention_get },
	{ OP_ATTENTION_SET,            1,   attention_set },
	{ OP_ATTENTION_SET_UNREL,      1,   attention_set_unrel },
	BT_MESH_MODEL_OP_END,
};

static void health_pub(struct bt_mesh_model *mod)
{
	struct net_buf_simple *msg = NET_BUF_SIMPLE(HEALTH_STATUS_SIZE);
	struct bt_mesh_health *srv = mod->user_data;
	size_t count;
	int err;

	BT_DBG("");

	count = health_get_current(mod, msg);
	if (count) {
		mod->pub->period_div = srv->period;
	} else {
		mod->pub->period_div = 0;
	}

	err = bt_mesh_model_publish(mod, msg);
	if (err) {
		BT_ERR("Publishing failed (err %d)", err);
	}
}

struct bt_mesh_model_pub bt_mesh_health_pub = {
	.func = health_pub,
};

int bt_mesh_fault_update(struct bt_mesh_elem *elem)
{
	struct bt_mesh_model *mod;

	mod = bt_mesh_model_find(elem, BT_MESH_MODEL_ID_HEALTH_SRV);
	if (!mod) {
		return -EINVAL;
	}

	k_delayed_work_submit(&mod->pub->timer, K_NO_WAIT);
	return 0;
}

static void attention_off(struct k_work *work)
{
	struct bt_mesh_health *srv = CONTAINER_OF(work, struct bt_mesh_health,
						  attention.timer.work);
	BT_DBG("");

	if (srv->attention.off) {
		srv->attention.off(srv->model);
	}
}

int bt_mesh_health_init(struct bt_mesh_model *model, bool primary)
{
	struct bt_mesh_health *srv = model->user_data;

	if (!srv) {
		if (!primary) {
			return 0;
		}

		BT_ERR("No Health Server context provided");
		return -EINVAL;
	}

	k_delayed_work_init(&srv->attention.timer, attention_off);

	srv->model = model;

	if (primary) {
		health_srv = srv;
	}

	return 0;
}

void bt_mesh_attention(struct bt_mesh_model *model, u8_t time)
{
	struct bt_mesh_health *srv;

	if (!model) {
		srv = health_srv;
		if (!srv) {
			BT_WARN("No Health Server available");
			return;
		}

		model = srv->model;
	} else {
		srv = model->user_data;
	}

	if (time) {
		if (srv->attention.on) {
			srv->attention.on(model);
		}

		k_delayed_work_submit(&srv->attention.timer, time * 1000);
	} else {
		k_delayed_work_cancel(&srv->attention.timer);

		if (srv->attention.off) {
			srv->attention.off(model);
		}
	}
}
