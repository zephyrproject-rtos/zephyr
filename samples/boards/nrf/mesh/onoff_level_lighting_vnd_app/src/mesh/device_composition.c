/* Bluetooth: Mesh Generic OnOff, Generic Level, Lighting & Vendor Models
 *
 * Copyright (c) 2018 Vikrant More
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/gpio.h>

#include "ble_mesh.h"
#include "common.h"
#include "device_composition.h"
#include "state_binding.h"
#include "transition.h"
#include "storage.h"

static struct bt_mesh_health_srv health_srv = {
};

BT_MESH_HEALTH_PUB_DEFINE(health_pub, 0);

/* Definitions of models publication context (Start) */
BT_MESH_MODEL_PUB_DEFINE(gen_onoff_srv_pub_root, NULL, 2 + 3);
BT_MESH_MODEL_PUB_DEFINE(gen_onoff_cli_pub_root, NULL, 2 + 4);

BT_MESH_MODEL_PUB_DEFINE(gen_level_srv_pub_root, NULL, 2 + 5);
BT_MESH_MODEL_PUB_DEFINE(gen_level_cli_pub_root, NULL, 2 + 7);

BT_MESH_MODEL_PUB_DEFINE(gen_def_trans_time_srv_pub, NULL, 2 + 1);
BT_MESH_MODEL_PUB_DEFINE(gen_def_trans_time_cli_pub, NULL, 2 + 1);

BT_MESH_MODEL_PUB_DEFINE(gen_power_onoff_srv_pub, NULL, 2 + 1);
BT_MESH_MODEL_PUB_DEFINE(gen_power_onoff_cli_pub, NULL, 2 + 1);

BT_MESH_MODEL_PUB_DEFINE(light_lightness_srv_pub, NULL, 2 + 5);
BT_MESH_MODEL_PUB_DEFINE(light_lightness_cli_pub, NULL, 2 + 5);

BT_MESH_MODEL_PUB_DEFINE(light_ctl_srv_pub, NULL, 2 + 9);
BT_MESH_MODEL_PUB_DEFINE(light_ctl_cli_pub, NULL, 2 + 9);

BT_MESH_MODEL_PUB_DEFINE(vnd_pub, NULL, 3 + 6);

BT_MESH_MODEL_PUB_DEFINE(gen_level_srv_pub_s0, NULL, 2 + 5);
BT_MESH_MODEL_PUB_DEFINE(gen_level_cli_pub_s0, NULL, 2 + 7);
/* Definitions of models publication context (End) */

struct lightness light;
struct temperature light_temp;
struct delta_uv duv;

struct light_ctl_state light_state = {
	.light = &light,
	.temp = &light_temp,
	.duv = &duv,
	.transition = &onoff_transition,
};

struct light_ctl_state *const ctl = &light_state;

/* Definitions of models user data (Start) */

struct vendor_state vnd_user_data;

/* Definitions of models user data (End) */

static struct bt_mesh_elem elements[];

/* message handlers (Start) */

/* Generic OnOff Server message handlers */
static int gen_onoff_get(struct bt_mesh_model *model,
			 struct bt_mesh_msg_ctx *ctx,
			 struct net_buf_simple *buf)
{
	struct net_buf_simple *msg = NET_BUF_SIMPLE(2 + 3 + 4);

	bt_mesh_model_msg_init(msg, BT_MESH_MODEL_OP_GEN_ONOFF_STATUS);
	net_buf_simple_add_u8(msg, (uint8_t) get_current(ONOFF));

	if (ctl->light->current == ctl->light->target) {
		goto send;
	}

	if (ctl->transition->counter) {
		calculate_rt(ctl->transition);
		net_buf_simple_add_u8(msg, (uint8_t) get_target(ONOFF));
		net_buf_simple_add_u8(msg, ctl->transition->rt);
	}

send:
	if (bt_mesh_model_send(model, ctx, msg, NULL, NULL)) {
		printk("Unable to send GEN_ONOFF_SRV Status response\n");
	}

	return 0;
}

void gen_onoff_publish(struct bt_mesh_model *model)
{
	int err;
	struct net_buf_simple *msg = model->pub->msg;

	if (model->pub->addr == BT_MESH_ADDR_UNASSIGNED) {
		return;
	}

	bt_mesh_model_msg_init(msg, BT_MESH_MODEL_OP_GEN_ONOFF_STATUS);
	net_buf_simple_add_u8(msg, (uint8_t) get_current(ONOFF));

	if (ctl->transition->counter) {
		calculate_rt(ctl->transition);
		net_buf_simple_add_u8(msg, (uint8_t) get_target(ONOFF));
		net_buf_simple_add_u8(msg, ctl->transition->rt);
	}

	err = bt_mesh_model_publish(model);
	if (err) {
		printk("bt_mesh_model_publish err %d\n", err);
	}
}

static int gen_onoff_set_unack(struct bt_mesh_model *model,
			       struct bt_mesh_msg_ctx *ctx,
			       struct net_buf_simple *buf)
{
	uint8_t tid, onoff, tt, delay;
	int64_t now;

	onoff = net_buf_simple_pull_u8(buf);
	tid = net_buf_simple_pull_u8(buf);

	if (onoff > STATE_ON) {
		return 0;
	}

	now = k_uptime_get();
	if (ctl->last_tid == tid &&
	    ctl->last_src_addr == ctx->addr &&
	    ctl->last_dst_addr == ctx->recv_dst &&
	    (now - ctl->last_msg_timestamp <= (6 * MSEC_PER_SEC))) {
		return 0;
	}

	switch (buf->len) {
	case 0x00:      /* No optional fields are available */
		tt = ctl->tt;
		delay = 0U;
		break;
	case 0x02:      /* Optional fields are available */
		tt = net_buf_simple_pull_u8(buf);
		if ((tt & 0x3F) == 0x3F) {
			return 0;
		}

		delay = net_buf_simple_pull_u8(buf);
		break;
	default:
		return 0;
	}

	ctl->transition->counter = 0U;
	k_timer_stop(&ctl->transition->timer);

	ctl->last_tid = tid;
	ctl->last_src_addr = ctx->addr;
	ctl->last_dst_addr = ctx->recv_dst;
	ctl->last_msg_timestamp = now;
	ctl->transition->tt = tt;
	ctl->transition->delay = delay;
	ctl->transition->type = NON_MOVE;
	set_target(ONOFF, &onoff);

	if (ctl->light->target != ctl->light->current) {
		set_transition_values(ONOFF);
	} else {
		return 0;
	}

	/* For Instantaneous Transition */
	if (ctl->transition->counter == 0U) {
		ctl->light->current = ctl->light->target;
	}

	ctl->transition->just_started = true;
	gen_onoff_publish(model);
	onoff_handler();

	return 0;
}

static int gen_onoff_set(struct bt_mesh_model *model,
			 struct bt_mesh_msg_ctx *ctx,
			 struct net_buf_simple *buf)
{
	uint8_t tid, onoff, tt, delay;
	int64_t now;

	onoff = net_buf_simple_pull_u8(buf);
	tid = net_buf_simple_pull_u8(buf);

	if (onoff > STATE_ON) {
		return 0;
	}

	now = k_uptime_get();
	if (ctl->last_tid == tid &&
	    ctl->last_src_addr == ctx->addr &&
	    ctl->last_dst_addr == ctx->recv_dst &&
	    (now - ctl->last_msg_timestamp <= (6 * MSEC_PER_SEC))) {
		(void)gen_onoff_get(model, ctx, buf);
		return 0;
	}

	switch (buf->len) {
	case 0x00:      /* No optional fields are available */
		tt = ctl->tt;
		delay = 0U;
		break;
	case 0x02:      /* Optional fields are available */
		tt = net_buf_simple_pull_u8(buf);
		if ((tt & 0x3F) == 0x3F) {
			return 0;
		}

		delay = net_buf_simple_pull_u8(buf);
		break;
	default:
		return 0;
	}

	ctl->transition->counter = 0U;
	k_timer_stop(&ctl->transition->timer);

	ctl->last_tid = tid;
	ctl->last_src_addr = ctx->addr;
	ctl->last_dst_addr = ctx->recv_dst;
	ctl->last_msg_timestamp = now;
	ctl->transition->tt = tt;
	ctl->transition->delay = delay;
	ctl->transition->type = NON_MOVE;
	set_target(ONOFF, &onoff);

	if (ctl->light->target != ctl->light->current) {
		set_transition_values(ONOFF);
	} else {
		(void)gen_onoff_get(model, ctx, buf);
		return 0;
	}

	/* For Instantaneous Transition */
	if (ctl->transition->counter == 0U) {
		ctl->light->current = ctl->light->target;
	}

	ctl->transition->just_started = true;
	(void)gen_onoff_get(model, ctx, buf);
	gen_onoff_publish(model);
	onoff_handler();

	return 0;
}

/* Generic OnOff Client message handlers */
static int gen_onoff_status(struct bt_mesh_model *model,
			    struct bt_mesh_msg_ctx *ctx,
			    struct net_buf_simple *buf)
{
	printk("Acknowledgement from GEN_ONOFF_SRV\n");
	printk("Present OnOff = %02x\n", net_buf_simple_pull_u8(buf));

	if (buf->len == 2U) {
		printk("Target OnOff = %02x\n", net_buf_simple_pull_u8(buf));
		printk("Remaining Time = %02x\n", net_buf_simple_pull_u8(buf));
	}

	return 0;
}

/* Generic Level (LIGHTNESS) Server message handlers */
static int gen_level_get(struct bt_mesh_model *model,
			 struct bt_mesh_msg_ctx *ctx,
			 struct net_buf_simple *buf)
{
	struct net_buf_simple *msg = NET_BUF_SIMPLE(2 + 5 + 4);

	bt_mesh_model_msg_init(msg, BT_MESH_MODEL_OP_GEN_LEVEL_STATUS);
	net_buf_simple_add_le16(msg, (int16_t) get_current(LEVEL_LIGHT));

	if (ctl->light->current == ctl->light->target) {
		goto send;
	}

	if (ctl->transition->counter) {
		calculate_rt(ctl->transition);
		net_buf_simple_add_le16(msg, (int16_t) get_target(LEVEL_LIGHT));
		net_buf_simple_add_u8(msg, ctl->transition->rt);
	}

send:
	if (bt_mesh_model_send(model, ctx, msg, NULL, NULL)) {
		printk("Unable to send GEN_LEVEL_SRV Status response\n");
	}

	return 0;
}

void gen_level_publish(struct bt_mesh_model *model)
{
	int err;
	struct net_buf_simple *msg = model->pub->msg;

	if (model->pub->addr == BT_MESH_ADDR_UNASSIGNED) {
		return;
	}

	bt_mesh_model_msg_init(msg, BT_MESH_MODEL_OP_GEN_LEVEL_STATUS);
	net_buf_simple_add_le16(msg, (int16_t) get_current(LEVEL_LIGHT));

	if (ctl->transition->counter) {
		calculate_rt(ctl->transition);
		net_buf_simple_add_le16(msg, (int16_t) get_target(LEVEL_LIGHT));
		net_buf_simple_add_u8(msg, ctl->transition->rt);
	}

	err = bt_mesh_model_publish(model);
	if (err) {
		printk("bt_mesh_model_publish err %d\n", err);
	}
}

static int gen_level_set_unack(struct bt_mesh_model *model,
			       struct bt_mesh_msg_ctx *ctx,
			       struct net_buf_simple *buf)
{
	uint8_t tid, tt, delay;
	int16_t level;
	int64_t now;

	level = (int16_t) net_buf_simple_pull_le16(buf);
	tid = net_buf_simple_pull_u8(buf);

	now = k_uptime_get();
	if (ctl->last_tid == tid &&
	    ctl->last_src_addr == ctx->addr &&
	    ctl->last_dst_addr == ctx->recv_dst &&
	    (now - ctl->last_msg_timestamp <= (6 * MSEC_PER_SEC))) {
		return 0;
	}

	switch (buf->len) {
	case 0x00:      /* No optional fields are available */
		tt = ctl->tt;
		delay = 0U;
		break;
	case 0x02:      /* Optional fields are available */
		tt = net_buf_simple_pull_u8(buf);
		if ((tt & 0x3F) == 0x3F) {
			return 0;
		}

		delay = net_buf_simple_pull_u8(buf);
		break;
	default:
		return 0;
	}

	ctl->transition->counter = 0U;
	k_timer_stop(&ctl->transition->timer);

	ctl->last_tid = tid;
	ctl->last_src_addr = ctx->addr;
	ctl->last_dst_addr = ctx->recv_dst;
	ctl->last_msg_timestamp = now;
	ctl->transition->tt = tt;
	ctl->transition->delay = delay;
	ctl->transition->type = NON_MOVE;
	set_target(LEVEL_LIGHT, &level);

	if (ctl->light->target != ctl->light->current) {
		set_transition_values(LEVEL_LIGHT);
	} else {
		return 0;
	}

	/* For Instantaneous Transition */
	if (ctl->transition->counter == 0U) {
		ctl->light->current = ctl->light->target;
	}

	ctl->transition->just_started = true;
	gen_level_publish(model);
	level_lightness_handler();

	return 0;
}

static int gen_level_set(struct bt_mesh_model *model,
			 struct bt_mesh_msg_ctx *ctx,
			 struct net_buf_simple *buf)
{
	uint8_t tid, tt, delay;
	int16_t level;
	int64_t now;

	level = (int16_t) net_buf_simple_pull_le16(buf);
	tid = net_buf_simple_pull_u8(buf);

	now = k_uptime_get();
	if (ctl->last_tid == tid &&
	    ctl->last_src_addr == ctx->addr &&
	    ctl->last_dst_addr == ctx->recv_dst &&
	    (now - ctl->last_msg_timestamp <= (6 * MSEC_PER_SEC))) {
		(void)gen_level_get(model, ctx, buf);
		return 0;
	}

	switch (buf->len) {
	case 0x00:      /* No optional fields are available */
		tt = ctl->tt;
		delay = 0U;
		break;
	case 0x02:      /* Optional fields are available */
		tt = net_buf_simple_pull_u8(buf);
		if ((tt & 0x3F) == 0x3F) {
			return 0;
		}

		delay = net_buf_simple_pull_u8(buf);
		break;
	default:
		return 0;
	}

	ctl->transition->counter = 0U;
	k_timer_stop(&ctl->transition->timer);

	ctl->last_tid = tid;
	ctl->last_src_addr = ctx->addr;
	ctl->last_dst_addr = ctx->recv_dst;
	ctl->last_msg_timestamp = now;
	ctl->transition->tt = tt;
	ctl->transition->delay = delay;
	ctl->transition->type = NON_MOVE;
	set_target(LEVEL_LIGHT, &level);

	if (ctl->light->target != ctl->light->current) {
		set_transition_values(LEVEL_LIGHT);
	} else {
		(void)gen_level_get(model, ctx, buf);
		return 0;
	}

	/* For Instantaneous Transition */
	if (ctl->transition->counter == 0U) {
		ctl->light->current = ctl->light->target;
	}

	ctl->transition->just_started = true;
	(void)gen_level_get(model, ctx, buf);
	gen_level_publish(model);
	level_lightness_handler();

	return 0;
}

static int gen_delta_set_unack(struct bt_mesh_model *model,
			       struct bt_mesh_msg_ctx *ctx,
			       struct net_buf_simple *buf)
{
	uint8_t tid, tt, delay;
	static int16_t last_level;
	int32_t target, delta;
	int64_t now;

	delta = (int32_t) net_buf_simple_pull_le32(buf);
	tid = net_buf_simple_pull_u8(buf);

	now = k_uptime_get();
	if (ctl->last_tid == tid &&
	    ctl->last_src_addr == ctx->addr &&
	    ctl->last_dst_addr == ctx->recv_dst &&
	    (now - ctl->last_msg_timestamp <= (6 * MSEC_PER_SEC))) {

		if (ctl->light->delta == delta) {
			return 0;
		}
		target = last_level + delta;

	} else {
		last_level = (int16_t) get_current(LEVEL_LIGHT);
		target = last_level + delta;
	}

	switch (buf->len) {
	case 0x00:      /* No optional fields are available */
		tt = ctl->tt;
		delay = 0U;
		break;
	case 0x02:      /* Optional fields are available */
		tt = net_buf_simple_pull_u8(buf);
		if ((tt & 0x3F) == 0x3F) {
			return 0;
		}

		delay = net_buf_simple_pull_u8(buf);
		break;
	default:
		return 0;
	}

	ctl->transition->counter = 0U;
	k_timer_stop(&ctl->transition->timer);

	ctl->last_tid = tid;
	ctl->last_src_addr = ctx->addr;
	ctl->last_dst_addr = ctx->recv_dst;
	ctl->last_msg_timestamp = now;
	ctl->transition->tt = tt;
	ctl->transition->delay = delay;
	ctl->transition->type = NON_MOVE;

	if (target < INT16_MIN) {
		target = INT16_MIN;
	} else if (target > INT16_MAX) {
		target = INT16_MAX;
	}

	set_target(DELTA_LEVEL_LIGHT, &target);

	if (ctl->light->target != ctl->light->current) {
		set_transition_values(LEVEL_LIGHT);
	} else {
		return 0;
	}

	/* For Instantaneous Transition */
	if (ctl->transition->counter == 0U) {
		ctl->light->current = ctl->light->target;
	}

	ctl->transition->just_started = true;
	gen_level_publish(model);
	level_lightness_handler();

	return 0;
}

static int gen_delta_set(struct bt_mesh_model *model,
			 struct bt_mesh_msg_ctx *ctx,
			 struct net_buf_simple *buf)
{
	uint8_t tid, tt, delay;
	static int16_t last_level;
	int32_t target, delta;
	int64_t now;

	delta = (int32_t) net_buf_simple_pull_le32(buf);
	tid = net_buf_simple_pull_u8(buf);

	now = k_uptime_get();
	if (ctl->last_tid == tid &&
	    ctl->last_src_addr == ctx->addr &&
	    ctl->last_dst_addr == ctx->recv_dst &&
	    (now - ctl->last_msg_timestamp <= (6 * MSEC_PER_SEC))) {

		if (ctl->light->delta == delta) {
			(void)gen_level_get(model, ctx, buf);
			return 0;
		}
		target = last_level + delta;

	} else {
		last_level = (int16_t) get_current(LEVEL_LIGHT);
		target = last_level + delta;
	}

	switch (buf->len) {
	case 0x00:      /* No optional fields are available */
		tt = ctl->tt;
		delay = 0U;
		break;
	case 0x02:      /* Optional fields are available */
		tt = net_buf_simple_pull_u8(buf);
		if ((tt & 0x3F) == 0x3F) {
			return 0;
		}

		delay = net_buf_simple_pull_u8(buf);
		break;
	default:
		return 0;
	}

	ctl->transition->counter = 0U;
	k_timer_stop(&ctl->transition->timer);

	ctl->last_tid = tid;
	ctl->last_src_addr = ctx->addr;
	ctl->last_dst_addr = ctx->recv_dst;
	ctl->last_msg_timestamp = now;
	ctl->transition->tt = tt;
	ctl->transition->delay = delay;
	ctl->transition->type = NON_MOVE;

	if (target < INT16_MIN) {
		target = INT16_MIN;
	} else if (target > INT16_MAX) {
		target = INT16_MAX;
	}

	set_target(DELTA_LEVEL_LIGHT, &target);

	if (ctl->light->target != ctl->light->current) {
		set_transition_values(LEVEL_LIGHT);
	} else {
		(void)gen_level_get(model, ctx, buf);
		return 0;
	}

	/* For Instantaneous Transition */
	if (ctl->transition->counter == 0U) {
		ctl->light->current = ctl->light->target;
	}

	ctl->transition->just_started = true;
	(void)gen_level_get(model, ctx, buf);
	gen_level_publish(model);
	level_lightness_handler();

	return 0;
}

static int gen_move_set_unack(struct bt_mesh_model *model,
			      struct bt_mesh_msg_ctx *ctx,
			      struct net_buf_simple *buf)
{
	uint8_t tid, tt, delay;
	int16_t delta;
	uint16_t target;
	int64_t now;

	delta = (int16_t) net_buf_simple_pull_le16(buf);
	tid = net_buf_simple_pull_u8(buf);

	now = k_uptime_get();
	if (ctl->last_tid == tid &&
	    ctl->last_src_addr == ctx->addr &&
	    ctl->last_dst_addr == ctx->recv_dst &&
	    (now - ctl->last_msg_timestamp <= (6 * MSEC_PER_SEC))) {
		return 0;
	}

	switch (buf->len) {
	case 0x00:      /* No optional fields are available */
		tt = ctl->tt;
		delay = 0U;
		break;
	case 0x02:      /* Optional fields are available */
		tt = net_buf_simple_pull_u8(buf);
		if ((tt & 0x3F) == 0x3F) {
			return 0;
		}

		delay = net_buf_simple_pull_u8(buf);
		break;
	default:
		return 0;
	}

	ctl->transition->counter = 0U;
	k_timer_stop(&ctl->transition->timer);

	ctl->last_tid = tid;
	ctl->last_src_addr = ctx->addr;
	ctl->last_dst_addr = ctx->recv_dst;
	ctl->last_msg_timestamp = now;
	ctl->transition->tt = tt;
	ctl->transition->delay = delay;
	ctl->transition->type = MOVE;
	ctl->light->delta = delta;

	if (delta < 0) {
		target = ctl->light->range_min;
	} else if (delta > 0) {
		target = ctl->light->range_max;
	} else if (delta == 0) {
		target = ctl->light->current;
	}
	set_target(MOVE_LIGHT, &target);

	if (ctl->light->target != ctl->light->current) {
		set_transition_values(MOVE_LIGHT);
	} else {
		return 0;
	}

	if (ctl->transition->counter == 0U) {
		return 0;
	}

	ctl->transition->just_started = true;
	gen_level_publish(model);
	level_lightness_handler();

	return 0;
}

static int gen_move_set(struct bt_mesh_model *model,
			struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf)
{
	uint8_t tid, tt, delay;
	int16_t delta;
	uint16_t target;
	int64_t now;

	delta = (int16_t) net_buf_simple_pull_le16(buf);
	tid = net_buf_simple_pull_u8(buf);

	now = k_uptime_get();
	if (ctl->last_tid == tid &&
	    ctl->last_src_addr == ctx->addr &&
	    ctl->last_dst_addr == ctx->recv_dst &&
	    (now - ctl->last_msg_timestamp <= (6 * MSEC_PER_SEC))) {
		(void)gen_level_get(model, ctx, buf);
		return 0;
	}

	switch (buf->len) {
	case 0x00:      /* No optional fields are available */
		tt = ctl->tt;
		delay = 0U;
		break;
	case 0x02:      /* Optional fields are available */
		tt = net_buf_simple_pull_u8(buf);
		if ((tt & 0x3F) == 0x3F) {
			return 0;
		}

		delay = net_buf_simple_pull_u8(buf);
		break;
	default:
		return 0;
	}

	ctl->transition->counter = 0U;
	k_timer_stop(&ctl->transition->timer);

	ctl->last_tid = tid;
	ctl->last_src_addr = ctx->addr;
	ctl->last_dst_addr = ctx->recv_dst;
	ctl->last_msg_timestamp = now;
	ctl->transition->tt = tt;
	ctl->transition->delay = delay;
	ctl->transition->type = MOVE;
	ctl->light->delta = delta;

	if (delta < 0) {
		target = ctl->light->range_min;
	} else if (delta > 0) {
		target = ctl->light->range_max;
	} else if (delta == 0) {
		target = ctl->light->current;
	}
	set_target(MOVE_LIGHT, &target);

	if (ctl->light->target != ctl->light->current) {
		set_transition_values(MOVE_LIGHT);
	} else {
		(void)gen_level_get(model, ctx, buf);
		return 0;
	}

	if (ctl->transition->counter == 0U) {
		return 0;
	}

	ctl->transition->just_started = true;
	(void)gen_level_get(model, ctx, buf);
	gen_level_publish(model);
	level_lightness_handler();

	return 0;
}

/* Generic Level Client message handlers */
static int gen_level_status(struct bt_mesh_model *model,
			    struct bt_mesh_msg_ctx *ctx,
			    struct net_buf_simple *buf)
{
	printk("Acknowledgement from GEN_LEVEL_SRV\n");
	printk("Present Level = %04x\n", net_buf_simple_pull_le16(buf));

	if (buf->len == 3U) {
		printk("Target Level = %04x\n", net_buf_simple_pull_le16(buf));
		printk("Remaining Time = %02x\n", net_buf_simple_pull_u8(buf));
	}

	return 0;
}

/* Generic Default Transition Time Server message handlers */
static int gen_def_trans_time_get(struct bt_mesh_model *model,
				  struct bt_mesh_msg_ctx *ctx,
				  struct net_buf_simple *buf)
{
	struct net_buf_simple *msg = NET_BUF_SIMPLE(2 + 1 + 4);

	bt_mesh_model_msg_init(msg, BT_MESH_MODEL_GEN_DEF_TRANS_TIME_STATUS);
	net_buf_simple_add_u8(msg, ctl->tt);

	if (bt_mesh_model_send(model, ctx, msg, NULL, NULL)) {
		printk("Unable to send GEN_DEF_TT_SRV Status response\n");
	}

	return 0;
}

static void gen_def_trans_time_publish(struct bt_mesh_model *model)
{
	int err;
	struct net_buf_simple *msg = model->pub->msg;

	if (model->pub->addr == BT_MESH_ADDR_UNASSIGNED) {
		return;
	}

	bt_mesh_model_msg_init(msg, BT_MESH_MODEL_GEN_DEF_TRANS_TIME_STATUS);
	net_buf_simple_add_u8(msg, ctl->tt);

	err = bt_mesh_model_publish(model);
	if (err) {
		printk("bt_mesh_model_publish err %d\n", err);
	}
}

static int gen_def_trans_time_set_unack(struct bt_mesh_model *model,
					struct bt_mesh_msg_ctx *ctx,
					struct net_buf_simple *buf)
{
	uint8_t tt;

	tt = net_buf_simple_pull_u8(buf);

	if ((tt & 0x3F) == 0x3F) {
		return 0;
	}

	if (ctl->tt != tt) {
		ctl->tt = tt;

		gen_def_trans_time_publish(model);
		save_on_flash(GEN_DEF_TRANS_TIME_STATE);
	}

	return 0;
}

static int gen_def_trans_time_set(struct bt_mesh_model *model,
				  struct bt_mesh_msg_ctx *ctx,
				  struct net_buf_simple *buf)
{
	uint8_t tt;

	tt = net_buf_simple_pull_u8(buf);

	if ((tt & 0x3F) == 0x3F) {
		return 0;
	}

	if (ctl->tt != tt) {
		ctl->tt = tt;

		(void)gen_def_trans_time_get(model, ctx, buf);
		gen_def_trans_time_publish(model);
		save_on_flash(GEN_DEF_TRANS_TIME_STATE);
	} else {
		(void)gen_def_trans_time_get(model, ctx, buf);
	}

	return 0;
}

/* Generic Default Transition Time Client message handlers */
static int gen_def_trans_time_status(struct bt_mesh_model *model,
				     struct bt_mesh_msg_ctx *ctx,
				     struct net_buf_simple *buf)
{
	printk("Acknowledgement from GEN_DEF_TT_SRV\n");
	printk("Transition Time = %02x\n", net_buf_simple_pull_u8(buf));

	return 0;
}

/* Generic Power OnOff Server message handlers */
static int gen_onpowerup_get(struct bt_mesh_model *model,
			     struct bt_mesh_msg_ctx *ctx,
			     struct net_buf_simple *buf)
{
	struct net_buf_simple *msg = NET_BUF_SIMPLE(2 + 1 + 4);

	bt_mesh_model_msg_init(msg, BT_MESH_MODEL_GEN_ONPOWERUP_STATUS);
	net_buf_simple_add_u8(msg, ctl->onpowerup);

	if (bt_mesh_model_send(model, ctx, msg, NULL, NULL)) {
		printk("Unable to send GEN_POWER_ONOFF_SRV Status response\n");
	}

	return 0;
}

/* Generic Power OnOff Client message handlers */
static int gen_onpowerup_status(struct bt_mesh_model *model,
				struct bt_mesh_msg_ctx *ctx,
				struct net_buf_simple *buf)
{
	printk("Acknowledgement from GEN_POWER_ONOFF_SRV\n");
	printk("OnPowerUp = %02x\n", net_buf_simple_pull_u8(buf));

	return 0;
}

/* Generic Power OnOff Setup Server message handlers */

static void gen_onpowerup_publish(struct bt_mesh_model *model)
{
	int err;
	struct net_buf_simple *msg = model->pub->msg;

	if (model->pub->addr == BT_MESH_ADDR_UNASSIGNED) {
		return;
	}

	bt_mesh_model_msg_init(msg, BT_MESH_MODEL_GEN_ONPOWERUP_STATUS);
	net_buf_simple_add_u8(msg, ctl->onpowerup);

	err = bt_mesh_model_publish(model);
	if (err) {
		printk("bt_mesh_model_publish err %d\n", err);
	}
}

static int gen_onpowerup_set_unack(struct bt_mesh_model *model,
				   struct bt_mesh_msg_ctx *ctx,
				   struct net_buf_simple *buf)
{
	uint8_t onpowerup;

	onpowerup = net_buf_simple_pull_u8(buf);

	if (onpowerup > STATE_RESTORE) {
		return 0;
	}

	if (ctl->onpowerup != onpowerup) {
		ctl->onpowerup = onpowerup;

		gen_onpowerup_publish(model);
		save_on_flash(GEN_ONPOWERUP_STATE);
	}

	return 0;
}

static int gen_onpowerup_set(struct bt_mesh_model *model,
			     struct bt_mesh_msg_ctx *ctx,
			     struct net_buf_simple *buf)
{
	uint8_t onpowerup;

	onpowerup = net_buf_simple_pull_u8(buf);

	if (onpowerup > STATE_RESTORE) {
		return 0;
	}

	if (ctl->onpowerup != onpowerup) {
		ctl->onpowerup = onpowerup;

		(void)gen_onpowerup_get(model, ctx, buf);
		gen_onpowerup_publish(model);
		save_on_flash(GEN_ONPOWERUP_STATE);
	} else {
		(void)gen_onpowerup_get(model, ctx, buf);
	}

	return 0;
}

/* Vendor Model message handlers*/
static int vnd_get(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
		   struct net_buf_simple *buf)
{
	struct net_buf_simple *msg = NET_BUF_SIMPLE(3 + 6 + 4);
	struct vendor_state *state = model->user_data;

	/* This is dummy response for demo purpose */
	state->response = 0xA578FEB3;

	bt_mesh_model_msg_init(msg, BT_MESH_MODEL_OP_3(0x04, CID_ZEPHYR));
	net_buf_simple_add_le16(msg, state->current);
	net_buf_simple_add_le32(msg, state->response);

	if (bt_mesh_model_send(model, ctx, msg, NULL, NULL)) {
		printk("Unable to send VENDOR Status response\n");
	}

	return 0;
}

static int vnd_set_unack(struct bt_mesh_model *model,
			 struct bt_mesh_msg_ctx *ctx,
			 struct net_buf_simple *buf)
{
	uint8_t tid;
	int current;
	int64_t now;
	struct vendor_state *state = model->user_data;

	current = net_buf_simple_pull_le16(buf);
	tid = net_buf_simple_pull_u8(buf);

	now = k_uptime_get();
	if (state->last_tid == tid &&
	    state->last_src_addr == ctx->addr &&
	    state->last_dst_addr == ctx->recv_dst &&
	    (now - state->last_msg_timestamp <= (6 * MSEC_PER_SEC))) {
		return 0;
	}

	state->last_tid = tid;
	state->last_src_addr = ctx->addr;
	state->last_dst_addr = ctx->recv_dst;
	state->last_msg_timestamp = now;
	state->current = current;

	printk("Vendor model message = %04x\n", state->current);

	update_vnd_led_gpio();

	return 0;
}

static int vnd_set(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
		   struct net_buf_simple *buf)
{
	(void)vnd_set_unack(model, ctx, buf);
	(void)vnd_get(model, ctx, buf);

	return 0;
}

static int vnd_status(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
		      struct net_buf_simple *buf)
{
	printk("Acknowledgement from Vendor\n");
	printk("cmd = %04x\n", net_buf_simple_pull_le16(buf));
	printk("response = %08x\n", net_buf_simple_pull_le32(buf));

	return 0;
}

/* Light Lightness Server message handlers */
static int light_lightness_get(struct bt_mesh_model *model,
			       struct bt_mesh_msg_ctx *ctx,
			       struct net_buf_simple *buf)
{
	struct net_buf_simple *msg = NET_BUF_SIMPLE(2 + 5 + 4);

	bt_mesh_model_msg_init(msg, BT_MESH_MODEL_LIGHT_LIGHTNESS_STATUS);
	net_buf_simple_add_le16(msg, (uint16_t) get_current(ACTUAL));

	if (ctl->light->current == ctl->light->target) {
		goto send;
	}

	if (ctl->transition->counter) {
		calculate_rt(ctl->transition);
		net_buf_simple_add_le16(msg, (uint16_t) get_target(ACTUAL));
		net_buf_simple_add_u8(msg, ctl->transition->rt);
	}

send:
	if (bt_mesh_model_send(model, ctx, msg, NULL, NULL)) {
		printk("Unable to send LightLightnessAct Status response\n");
	}

	return 0;
}

void light_lightness_publish(struct bt_mesh_model *model)
{
	int err;
	struct net_buf_simple *msg = model->pub->msg;

	if (model->pub->addr == BT_MESH_ADDR_UNASSIGNED) {
		return;
	}

	bt_mesh_model_msg_init(msg, BT_MESH_MODEL_LIGHT_LIGHTNESS_STATUS);
	net_buf_simple_add_le16(msg, (uint16_t) get_current(ACTUAL));

	if (ctl->transition->counter) {
		calculate_rt(ctl->transition);
		net_buf_simple_add_le16(msg, (uint16_t) get_target(ACTUAL));
		net_buf_simple_add_u8(msg, ctl->transition->rt);
	}

	err = bt_mesh_model_publish(model);
	if (err) {
		printk("bt_mesh_model_publish err %d\n", err);
	}
}

static int light_lightness_set_unack(struct bt_mesh_model *model,
				     struct bt_mesh_msg_ctx *ctx,
				     struct net_buf_simple *buf)
{
	uint8_t tid, tt, delay;
	uint16_t actual;
	int64_t now;

	actual = net_buf_simple_pull_le16(buf);
	tid = net_buf_simple_pull_u8(buf);

	now = k_uptime_get();
	if (ctl->last_tid == tid &&
	    ctl->last_src_addr == ctx->addr &&
	    ctl->last_dst_addr == ctx->recv_dst &&
	    (now - ctl->last_msg_timestamp <= (6 * MSEC_PER_SEC))) {
		return 0;
	}

	switch (buf->len) {
	case 0x00:      /* No optional fields are available */
		tt = ctl->tt;
		delay = 0U;
		break;
	case 0x02:      /* Optional fields are available */
		tt = net_buf_simple_pull_u8(buf);
		if ((tt & 0x3F) == 0x3F) {
			return 0;
		}

		delay = net_buf_simple_pull_u8(buf);
		break;
	default:
		return 0;
	}

	ctl->transition->counter = 0U;
	k_timer_stop(&ctl->transition->timer);

	ctl->last_tid = tid;
	ctl->last_src_addr = ctx->addr;
	ctl->last_dst_addr = ctx->recv_dst;
	ctl->last_msg_timestamp = now;
	ctl->transition->tt = tt;
	ctl->transition->delay = delay;
	ctl->transition->type = NON_MOVE;
	set_target(ACTUAL, &actual);

	if (ctl->light->target != ctl->light->current) {
		set_transition_values(ACTUAL);
	} else {
		return 0;
	}

	/* For Instantaneous Transition */
	if (ctl->transition->counter == 0U) {
		ctl->light->current = ctl->light->target;
	}

	ctl->transition->just_started = true;
	light_lightness_publish(model);
	light_lightness_actual_handler();

	return 0;
}

static int light_lightness_set(struct bt_mesh_model *model,
				struct bt_mesh_msg_ctx *ctx,
				struct net_buf_simple *buf)
{
	uint8_t tid, tt, delay;
	uint16_t actual;
	int64_t now;

	actual = net_buf_simple_pull_le16(buf);
	tid = net_buf_simple_pull_u8(buf);

	now = k_uptime_get();
	if (ctl->last_tid == tid &&
	    ctl->last_src_addr == ctx->addr &&
	    ctl->last_dst_addr == ctx->recv_dst &&
	    (now - ctl->last_msg_timestamp <= (6 * MSEC_PER_SEC))) {
		(void)light_lightness_get(model, ctx, buf);
		return 0;
	}

	switch (buf->len) {
	case 0x00:      /* No optional fields are available */
		tt = ctl->tt;
		delay = 0U;
		break;
	case 0x02:      /* Optional fields are available */
		tt = net_buf_simple_pull_u8(buf);
		if ((tt & 0x3F) == 0x3F) {
			return 0;
		}

		delay = net_buf_simple_pull_u8(buf);
		break;
	default:
		return 0;
	}

	ctl->transition->counter = 0U;
	k_timer_stop(&ctl->transition->timer);

	ctl->last_tid = tid;
	ctl->last_src_addr = ctx->addr;
	ctl->last_dst_addr = ctx->recv_dst;
	ctl->last_msg_timestamp = now;
	ctl->transition->tt = tt;
	ctl->transition->delay = delay;
	ctl->transition->type = NON_MOVE;
	set_target(ACTUAL, &actual);

	if (ctl->light->target != ctl->light->current) {
		set_transition_values(ACTUAL);
	} else {
		(void)light_lightness_get(model, ctx, buf);
		return 0;
	}

	/* For Instantaneous Transition */
	if (ctl->transition->counter == 0U) {
		ctl->light->current = ctl->light->target;
	}

	ctl->transition->just_started = true;
	(void)light_lightness_get(model, ctx, buf);
	light_lightness_publish(model);
	light_lightness_actual_handler();

	return 0;
}

static int light_lightness_linear_get(struct bt_mesh_model *model,
				      struct bt_mesh_msg_ctx *ctx,
				      struct net_buf_simple *buf)
{
	struct net_buf_simple *msg = NET_BUF_SIMPLE(2 + 5 + 4);

	bt_mesh_model_msg_init(msg,
			       BT_MESH_MODEL_LIGHT_LIGHTNESS_LINEAR_STATUS);
	net_buf_simple_add_le16(msg, (uint16_t) get_current(LINEAR));

	if (ctl->light->current == ctl->light->target) {
		goto send;
	}

	if (ctl->transition->counter) {
		calculate_rt(ctl->transition);
		net_buf_simple_add_le16(msg, (uint16_t) get_target(LINEAR));
		net_buf_simple_add_u8(msg, ctl->transition->rt);
	}

send:
	if (bt_mesh_model_send(model, ctx, msg, NULL, NULL)) {
		printk("Unable to send LightLightnessLin Status response\n");
	}

	return 0;
}

void light_lightness_linear_publish(struct bt_mesh_model *model)
{
	int err;
	struct net_buf_simple *msg = model->pub->msg;

	if (model->pub->addr == BT_MESH_ADDR_UNASSIGNED) {
		return;
	}

	bt_mesh_model_msg_init(msg,
			       BT_MESH_MODEL_LIGHT_LIGHTNESS_LINEAR_STATUS);
	net_buf_simple_add_le16(msg, (uint16_t) get_current(LINEAR));

	if (ctl->transition->counter) {
		calculate_rt(ctl->transition);
		net_buf_simple_add_le16(msg, (uint16_t) get_target(LINEAR));
		net_buf_simple_add_u8(msg, ctl->transition->rt);
	}

	err = bt_mesh_model_publish(model);
	if (err) {
		printk("bt_mesh_model_publish err %d\n", err);
	}
}

static int light_lightness_linear_set_unack(struct bt_mesh_model *model,
					    struct bt_mesh_msg_ctx *ctx,
					    struct net_buf_simple *buf)
{
	uint8_t tid, tt, delay;
	uint16_t linear;
	int64_t now;

	linear = net_buf_simple_pull_le16(buf);
	tid = net_buf_simple_pull_u8(buf);

	now = k_uptime_get();
	if (ctl->last_tid == tid &&
	    ctl->last_src_addr == ctx->addr &&
	    ctl->last_dst_addr == ctx->recv_dst &&
	    (now - ctl->last_msg_timestamp <= (6 * MSEC_PER_SEC))) {
		return 0;
	}

	switch (buf->len) {
	case 0x00:      /* No optional fields are available */
		tt = ctl->tt;
		delay = 0U;
		break;
	case 0x02:      /* Optional fields are available */
		tt = net_buf_simple_pull_u8(buf);
		if ((tt & 0x3F) == 0x3F) {
			return 0;
		}

		delay = net_buf_simple_pull_u8(buf);
		break;
	default:
		return 0;
	}

	ctl->transition->counter = 0U;
	k_timer_stop(&ctl->transition->timer);

	ctl->last_tid = tid;
	ctl->last_src_addr = ctx->addr;
	ctl->last_dst_addr = ctx->recv_dst;
	ctl->last_msg_timestamp = now;
	ctl->transition->tt = tt;
	ctl->transition->delay = delay;
	ctl->transition->type = NON_MOVE;
	set_target(LINEAR, &linear);

	if (ctl->light->target != ctl->light->current) {
		set_transition_values(LINEAR);
	} else {
		return 0;
	}

	/* For Instantaneous Transition */
	if (ctl->transition->counter == 0U) {
		ctl->light->current = ctl->light->target;
	}

	ctl->transition->just_started = true;
	light_lightness_linear_publish(model);
	light_lightness_linear_handler();

	return 0;
}

static int light_lightness_linear_set(struct bt_mesh_model *model,
				      struct bt_mesh_msg_ctx *ctx,
				      struct net_buf_simple *buf)
{
	uint8_t tid, tt, delay;
	uint16_t linear;
	int64_t now;

	linear = net_buf_simple_pull_le16(buf);
	tid = net_buf_simple_pull_u8(buf);

	now = k_uptime_get();
	if (ctl->last_tid == tid &&
	    ctl->last_src_addr == ctx->addr &&
	    ctl->last_dst_addr == ctx->recv_dst &&
	    (now - ctl->last_msg_timestamp <= (6 * MSEC_PER_SEC))) {
		(void)light_lightness_linear_get(model, ctx, buf);
		return 0;
	}

	switch (buf->len) {
	case 0x00:      /* No optional fields are available */
		tt = ctl->tt;
		delay = 0U;
		break;
	case 0x02:      /* Optional fields are available */
		tt = net_buf_simple_pull_u8(buf);
		if ((tt & 0x3F) == 0x3F) {
			return 0;
		}

		delay = net_buf_simple_pull_u8(buf);
		break;
	default:
		return 0;
	}

	ctl->transition->counter = 0U;
	k_timer_stop(&ctl->transition->timer);

	ctl->last_tid = tid;
	ctl->last_src_addr = ctx->addr;
	ctl->last_dst_addr = ctx->recv_dst;
	ctl->last_msg_timestamp = now;
	ctl->transition->tt = tt;
	ctl->transition->delay = delay;
	ctl->transition->type = NON_MOVE;
	set_target(LINEAR, &linear);

	if (ctl->light->target != ctl->light->current) {
		set_transition_values(LINEAR);
	} else {
		(void)light_lightness_linear_get(model, ctx, buf);
		return 0;
	}

	/* For Instantaneous Transition */
	if (ctl->transition->counter == 0U) {
		ctl->light->current = ctl->light->target;
	}

	ctl->transition->just_started = true;
	(void)light_lightness_linear_get(model, ctx, buf);
	light_lightness_linear_publish(model);
	light_lightness_linear_handler();

	return 0;
}

static int light_lightness_last_get(struct bt_mesh_model *model,
				    struct bt_mesh_msg_ctx *ctx,
				    struct net_buf_simple *buf)
{
	struct net_buf_simple *msg = NET_BUF_SIMPLE(2 + 2 + 4);

	bt_mesh_model_msg_init(msg, BT_MESH_MODEL_LIGHT_LIGHTNESS_LAST_STATUS);
	net_buf_simple_add_le16(msg, ctl->light->last);

	if (bt_mesh_model_send(model, ctx, msg, NULL, NULL)) {
		printk("Unable to send LightLightnessLast Status response\n");
	}

	return 0;
}

static int light_lightness_default_get(struct bt_mesh_model *model,
				       struct bt_mesh_msg_ctx *ctx,
				       struct net_buf_simple *buf)
{
	struct net_buf_simple *msg = NET_BUF_SIMPLE(2 + 2 + 4);

	bt_mesh_model_msg_init(msg,
			       BT_MESH_MODEL_LIGHT_LIGHTNESS_DEFAULT_STATUS);
	net_buf_simple_add_le16(msg, ctl->light->def);

	if (bt_mesh_model_send(model, ctx, msg, NULL, NULL)) {
		printk("Unable to send LightLightnessDef Status response\n");
	}

	return 0;
}

static int light_lightness_range_get(struct bt_mesh_model *model,
				     struct bt_mesh_msg_ctx *ctx,
				     struct net_buf_simple *buf)
{
	struct net_buf_simple *msg = NET_BUF_SIMPLE(2 + 5 + 4);

	ctl->light->status_code = RANGE_SUCCESSFULLY_UPDATED;

	bt_mesh_model_msg_init(msg, BT_MESH_MODEL_LIGHT_LIGHTNESS_RANGE_STATUS);
	net_buf_simple_add_u8(msg, ctl->light->status_code);
	net_buf_simple_add_le16(msg, ctl->light->range_min);
	net_buf_simple_add_le16(msg, ctl->light->range_max);

	if (bt_mesh_model_send(model, ctx, msg, NULL, NULL)) {
		printk("Unable to send LightLightnessRange Status response\n");
	}

	return 0;
}

/* Light Lightness Setup Server message handlers */

static void light_lightness_default_publish(struct bt_mesh_model *model)
{
	int err;
	struct net_buf_simple *msg = model->pub->msg;

	if (model->pub->addr == BT_MESH_ADDR_UNASSIGNED) {
		return;
	}

	bt_mesh_model_msg_init(msg,
			       BT_MESH_MODEL_LIGHT_LIGHTNESS_DEFAULT_STATUS);
	net_buf_simple_add_le16(msg, ctl->light->def);

	err = bt_mesh_model_publish(model);
	if (err) {
		printk("bt_mesh_model_publish err %d\n", err);
	}
}

static int light_lightness_default_set_unack(struct bt_mesh_model *model,
					     struct bt_mesh_msg_ctx *ctx,
					     struct net_buf_simple *buf)
{
	uint16_t lightness;

	lightness = net_buf_simple_pull_le16(buf);
	lightness = constrain_lightness(lightness);

	if (ctl->light->def != lightness) {
		ctl->light->def = lightness;

		light_lightness_default_publish(model);
		save_on_flash(DEF_STATES);
	}

	return 0;
}

static int light_lightness_default_set(struct bt_mesh_model *model,
				       struct bt_mesh_msg_ctx *ctx,
				       struct net_buf_simple *buf)
{
	uint16_t lightness;

	lightness = net_buf_simple_pull_le16(buf);
	lightness = constrain_lightness(lightness);

	if (ctl->light->def != lightness) {
		ctl->light->def = lightness;

		(void)light_lightness_default_get(model, ctx, buf);
		light_lightness_default_publish(model);
		save_on_flash(DEF_STATES);
	} else {
		(void)light_lightness_default_get(model, ctx, buf);
	}

	return 0;
}

static void light_lightness_range_publish(struct bt_mesh_model *model)
{
	int err;
	struct net_buf_simple *msg = model->pub->msg;

	if (model->pub->addr == BT_MESH_ADDR_UNASSIGNED) {
		return;
	}

	bt_mesh_model_msg_init(msg, BT_MESH_MODEL_LIGHT_LIGHTNESS_RANGE_STATUS);
	net_buf_simple_add_u8(msg, ctl->light->status_code);
	net_buf_simple_add_le16(msg, ctl->light->range_min);
	net_buf_simple_add_le16(msg, ctl->light->range_max);

	err = bt_mesh_model_publish(model);
	if (err) {
		printk("bt_mesh_model_publish err %d\n", err);
	}
}

static int light_lightness_range_set_unack(struct bt_mesh_model *model,
					   struct bt_mesh_msg_ctx *ctx,
					   struct net_buf_simple *buf)
{
	uint16_t min, max;

	min = net_buf_simple_pull_le16(buf);
	max = net_buf_simple_pull_le16(buf);

	if (min == 0U || max == 0U) {
		return 0;
	}

	if (min <= max) {
		ctl->light->status_code = RANGE_SUCCESSFULLY_UPDATED;

		if (ctl->light->range_min != min ||
		    ctl->light->range_max != max) {

			ctl->light->range_min = min;
			ctl->light->range_max = max;

			light_lightness_range_publish(model);
			save_on_flash(LIGHTNESS_RANGE);
		}
	} else {
		/* The provided value for Range Max cannot be set */
		ctl->light->status_code = CANNOT_SET_RANGE_MAX;
		return 0;
	}

	return 0;
}

static int light_lightness_range_set(struct bt_mesh_model *model,
				     struct bt_mesh_msg_ctx *ctx,
				     struct net_buf_simple *buf)
{
	uint16_t min, max;

	min = net_buf_simple_pull_le16(buf);
	max = net_buf_simple_pull_le16(buf);

	if (min == 0U || max == 0U) {
		return 0;
	}

	if (min <= max) {
		ctl->light->status_code = RANGE_SUCCESSFULLY_UPDATED;

		if (ctl->light->range_min != min ||
		    ctl->light->range_max != max) {

			ctl->light->range_min = min;
			ctl->light->range_max = max;

			(void)light_lightness_range_get(model, ctx, buf);
			light_lightness_range_publish(model);
			save_on_flash(LIGHTNESS_RANGE);
		} else {
			(void)light_lightness_range_get(model, ctx, buf);
		}
	} else {
		/* The provided value for Range Max cannot be set */
		ctl->light->status_code = CANNOT_SET_RANGE_MAX;
		return 0;
	}

	return 0;
}

/* Light Lightness Client message handlers */
static int light_lightness_status(struct bt_mesh_model *model,
				  struct bt_mesh_msg_ctx *ctx,
				  struct net_buf_simple *buf)
{
	printk("Acknowledgement from LIGHT_LIGHTNESS_SRV (Actual)\n");
	printk("Present Lightness = %04x\n", net_buf_simple_pull_le16(buf));

	if (buf->len == 3U) {
		printk("Target Lightness = %04x\n",
		       net_buf_simple_pull_le16(buf));
		printk("Remaining Time = %02x\n", net_buf_simple_pull_u8(buf));
	}

	return 0;
}

static int light_lightness_linear_status(struct bt_mesh_model *model,
					 struct bt_mesh_msg_ctx *ctx,
					 struct net_buf_simple *buf)
{
	printk("Acknowledgement from LIGHT_LIGHTNESS_SRV (Linear)\n");
	printk("Present Lightness = %04x\n", net_buf_simple_pull_le16(buf));

	if (buf->len == 3U) {
		printk("Target Lightness = %04x\n",
		       net_buf_simple_pull_le16(buf));
		printk("Remaining Time = %02x\n", net_buf_simple_pull_u8(buf));
	}

	return 0;
}

static int light_lightness_last_status(struct bt_mesh_model *model,
				       struct bt_mesh_msg_ctx *ctx,
				       struct net_buf_simple *buf)
{
	printk("Acknowledgement from LIGHT_LIGHTNESS_SRV (Last)\n");
	printk("Lightness = %04x\n", net_buf_simple_pull_le16(buf));

	return 0;
}

static int light_lightness_default_status(struct bt_mesh_model *model,
					  struct bt_mesh_msg_ctx *ctx,
					  struct net_buf_simple *buf)
{
	printk("Acknowledgement from LIGHT_LIGHTNESS_SRV (Default)\n");
	printk("Lightness = %04x\n", net_buf_simple_pull_le16(buf));

	return 0;
}

static int light_lightness_range_status(struct bt_mesh_model *model,
					struct bt_mesh_msg_ctx *ctx,
					struct net_buf_simple *buf)
{
	printk("Acknowledgement from LIGHT_LIGHTNESS_SRV (Lightness Range)\n");
	printk("Status Code = %02x\n", net_buf_simple_pull_u8(buf));
	printk("Range Min = %04x\n", net_buf_simple_pull_le16(buf));
	printk("Range Max = %04x\n", net_buf_simple_pull_le16(buf));

	return 0;
}

/* Light CTL Server message handlers */
static int light_ctl_get(struct bt_mesh_model *model,
			 struct bt_mesh_msg_ctx *ctx,
			 struct net_buf_simple *buf)
{
	struct net_buf_simple *msg = NET_BUF_SIMPLE(2 + 9 + 4);

	bt_mesh_model_msg_init(msg, BT_MESH_MODEL_LIGHT_CTL_STATUS);
	net_buf_simple_add_le16(msg, (uint16_t) get_current(CTL_LIGHT));
	net_buf_simple_add_le16(msg, (uint16_t) get_current(CTL_TEMP));

	if (ctl->light->current == ctl->light->target &&
	    ctl->temp->current == ctl->temp->target) {
		goto send;
	}

	if (ctl->transition->counter) {
		calculate_rt(ctl->transition);
		net_buf_simple_add_le16(msg, (uint16_t) get_target(CTL_LIGHT));
		net_buf_simple_add_le16(msg, (uint16_t) get_target(CTL_TEMP));
		net_buf_simple_add_u8(msg, ctl->transition->rt);
	}

send:
	if (bt_mesh_model_send(model, ctx, msg, NULL, NULL)) {
		printk("Unable to send LightCTL Status response\n");
	}

	return 0;
}

void light_ctl_publish(struct bt_mesh_model *model)
{
	int err;
	struct net_buf_simple *msg = model->pub->msg;

	if (model->pub->addr == BT_MESH_ADDR_UNASSIGNED) {
		return;
	}

	bt_mesh_model_msg_init(msg, BT_MESH_MODEL_LIGHT_CTL_STATUS);

	/* Here, as per Model specification, status should be
	 * made up of lightness & temperature values only
	 */
	net_buf_simple_add_le16(msg, (uint16_t) get_current(CTL_LIGHT));
	net_buf_simple_add_le16(msg, (uint16_t) get_current(CTL_TEMP));

	if (ctl->transition->counter) {
		calculate_rt(ctl->transition);
		net_buf_simple_add_le16(msg, (uint16_t) get_target(CTL_LIGHT));
		net_buf_simple_add_le16(msg, (uint16_t) get_target(CTL_TEMP));
		net_buf_simple_add_u8(msg, ctl->transition->rt);
	}

	err = bt_mesh_model_publish(model);
	if (err) {
		printk("bt_mesh_model_publish err %d\n", err);
	}
}

static int light_ctl_set_unack(struct bt_mesh_model *model,
			       struct bt_mesh_msg_ctx *ctx,
			       struct net_buf_simple *buf)
{
	uint8_t tid, tt, delay;
	int16_t delta_uv;
	uint16_t lightness, temp;
	int64_t now;

	lightness = net_buf_simple_pull_le16(buf);
	temp = net_buf_simple_pull_le16(buf);
	delta_uv = (int16_t) net_buf_simple_pull_le16(buf);
	tid = net_buf_simple_pull_u8(buf);

	if (temp < TEMP_MIN || temp > TEMP_MAX) {
		return 0;
	}

	now = k_uptime_get();
	if (ctl->last_tid == tid &&
	    ctl->last_src_addr == ctx->addr &&
	    ctl->last_dst_addr == ctx->recv_dst &&
	    (now - ctl->last_msg_timestamp <= (6 * MSEC_PER_SEC))) {
		return 0;
	}

	switch (buf->len) {
	case 0x00:      /* No optional fields are available */
		tt = ctl->tt;
		delay = 0U;
		break;
	case 0x02:      /* Optional fields are available */
		tt = net_buf_simple_pull_u8(buf);
		if ((tt & 0x3F) == 0x3F) {
			return 0;
		}

		delay = net_buf_simple_pull_u8(buf);
		break;
	default:
		return 0;
	}

	ctl->transition->counter = 0U;
	k_timer_stop(&ctl->transition->timer);

	ctl->last_tid = tid;
	ctl->last_src_addr = ctx->addr;
	ctl->last_dst_addr = ctx->recv_dst;
	ctl->last_msg_timestamp = now;
	ctl->transition->tt = tt;
	ctl->transition->delay = delay;
	ctl->transition->type = NON_MOVE;
	set_target(CTL_LIGHT, &lightness);
	set_target(CTL_TEMP, &temp);
	set_target(CTL_DELTA_UV, &delta_uv);

	if (ctl->light->target != ctl->light->current ||
	    ctl->temp->target != ctl->temp->current ||
	    ctl->duv->target != ctl->duv->current) {
		set_transition_values(CTL_LIGHT);
	} else {
		return 0;
	}

	/* For Instantaneous Transition */
	if (ctl->transition->counter == 0U) {
		ctl->light->current = ctl->light->target;
		ctl->temp->current = ctl->temp->target;
		ctl->duv->current = ctl->duv->target;
	}

	ctl->transition->just_started = true;
	light_ctl_publish(model);
	light_ctl_handler();

	return 0;
}

static int light_ctl_set(struct bt_mesh_model *model,
			 struct bt_mesh_msg_ctx *ctx,
			 struct net_buf_simple *buf)
{
	uint8_t tid, tt, delay;
	int16_t delta_uv;
	uint16_t lightness, temp;
	int64_t now;

	lightness = net_buf_simple_pull_le16(buf);
	temp = net_buf_simple_pull_le16(buf);
	delta_uv = (int16_t) net_buf_simple_pull_le16(buf);
	tid = net_buf_simple_pull_u8(buf);

	if (temp < TEMP_MIN || temp > TEMP_MAX) {
		return 0;
	}

	now = k_uptime_get();
	if (ctl->last_tid == tid &&
	    ctl->last_src_addr == ctx->addr &&
	    ctl->last_dst_addr == ctx->recv_dst &&
	    (now - ctl->last_msg_timestamp <= (6 * MSEC_PER_SEC))) {
		(void)light_ctl_get(model, ctx, buf);
		return 0;
	}

	switch (buf->len) {
	case 0x00:      /* No optional fields are available */
		tt = ctl->tt;
		delay = 0U;
		break;
	case 0x02:      /* Optional fields are available */
		tt = net_buf_simple_pull_u8(buf);
		if ((tt & 0x3F) == 0x3F) {
			return 0;
		}

		delay = net_buf_simple_pull_u8(buf);
		break;
	default:
		return 0;
	}

	ctl->transition->counter = 0U;
	k_timer_stop(&ctl->transition->timer);

	ctl->last_tid = tid;
	ctl->last_src_addr = ctx->addr;
	ctl->last_dst_addr = ctx->recv_dst;
	ctl->last_msg_timestamp = now;
	ctl->transition->tt = tt;
	ctl->transition->delay = delay;
	ctl->transition->type = NON_MOVE;
	set_target(CTL_LIGHT, &lightness);
	set_target(CTL_TEMP, &temp);
	set_target(CTL_DELTA_UV, &delta_uv);

	if (ctl->light->target != ctl->light->current ||
	    ctl->temp->target != ctl->temp->current ||
	    ctl->duv->target != ctl->duv->current) {
		set_transition_values(CTL_LIGHT);
	} else {
		(void)light_ctl_get(model, ctx, buf);
		return 0;
	}

	/* For Instantaneous Transition */
	if (ctl->transition->counter == 0U) {
		ctl->light->current = ctl->light->target;
		ctl->temp->current = ctl->temp->target;
		ctl->duv->current = ctl->duv->target;
	}

	ctl->transition->just_started = true;
	(void)light_ctl_get(model, ctx, buf);
	light_ctl_publish(model);
	light_ctl_handler();

	return 0;
}

static int light_ctl_temp_range_get(struct bt_mesh_model *model,
				    struct bt_mesh_msg_ctx *ctx,
				    struct net_buf_simple *buf)
{
	struct net_buf_simple *msg = NET_BUF_SIMPLE(2 + 5 + 4);

	ctl->temp->status_code = RANGE_SUCCESSFULLY_UPDATED;

	bt_mesh_model_msg_init(msg, BT_MESH_MODEL_LIGHT_CTL_TEMP_RANGE_STATUS);
	net_buf_simple_add_u8(msg, ctl->temp->status_code);
	net_buf_simple_add_le16(msg, ctl->temp->range_min);
	net_buf_simple_add_le16(msg, ctl->temp->range_max);

	if (bt_mesh_model_send(model, ctx, msg, NULL, NULL)) {
		printk("Unable to send LightCTL Temp Range Status response\n");
	}

	return 0;
}

static int light_ctl_default_get(struct bt_mesh_model *model,
				 struct bt_mesh_msg_ctx *ctx,
				 struct net_buf_simple *buf)
{
	struct net_buf_simple *msg = NET_BUF_SIMPLE(2 + 6 + 4);

	bt_mesh_model_msg_init(msg, BT_MESH_MODEL_LIGHT_CTL_DEFAULT_STATUS);
	net_buf_simple_add_le16(msg, ctl->light->def);
	net_buf_simple_add_le16(msg, ctl->temp->def);
	net_buf_simple_add_le16(msg, ctl->duv->def);

	if (bt_mesh_model_send(model, ctx, msg, NULL, NULL)) {
		printk("Unable to send LightCTL Default Status response\n");
	}

	return 0;
}

/* Light CTL Setup Server message handlers */

static void light_ctl_default_publish(struct bt_mesh_model *model)
{
	int err;
	struct net_buf_simple *msg = model->pub->msg;

	if (model->pub->addr == BT_MESH_ADDR_UNASSIGNED) {
		return;
	}

	bt_mesh_model_msg_init(msg, BT_MESH_MODEL_LIGHT_CTL_DEFAULT_STATUS);
	net_buf_simple_add_le16(msg, ctl->light->def);
	net_buf_simple_add_le16(msg, ctl->temp->def);
	net_buf_simple_add_le16(msg, ctl->duv->def);

	err = bt_mesh_model_publish(model);
	if (err) {
		printk("bt_mesh_model_publish err %d\n", err);
	}
}

static int light_ctl_default_set_unack(struct bt_mesh_model *model,
					struct bt_mesh_msg_ctx *ctx,
					struct net_buf_simple *buf)
{
	uint16_t lightness, temp;
	int16_t delta_uv;

	lightness = net_buf_simple_pull_le16(buf);
	temp = net_buf_simple_pull_le16(buf);
	delta_uv = (int16_t) net_buf_simple_pull_le16(buf);

	if (temp < TEMP_MIN || temp > TEMP_MAX) {
		return 0;
	}

	lightness = constrain_lightness(lightness);
	temp = constrain_temperature(temp);

	if (ctl->light->def != lightness || ctl->temp->def != temp ||
	    ctl->duv->def != delta_uv) {
		ctl->light->def = lightness;
		ctl->temp->def = temp;
		ctl->duv->def = delta_uv;

		light_ctl_default_publish(model);
		save_on_flash(DEF_STATES);
	}

	return 0;
}

static int light_ctl_default_set(struct bt_mesh_model *model,
				 struct bt_mesh_msg_ctx *ctx,
				 struct net_buf_simple *buf)
{
	uint16_t lightness, temp;
	int16_t delta_uv;

	lightness = net_buf_simple_pull_le16(buf);
	temp = net_buf_simple_pull_le16(buf);
	delta_uv = (int16_t) net_buf_simple_pull_le16(buf);

	if (temp < TEMP_MIN || temp > TEMP_MAX) {
		return 0;
	}

	lightness = constrain_lightness(lightness);
	temp = constrain_temperature(temp);

	if (ctl->light->def != lightness || ctl->temp->def != temp ||
	    ctl->duv->def != delta_uv) {
		ctl->light->def = lightness;
		ctl->temp->def = temp;
		ctl->duv->def = delta_uv;

		(void)light_ctl_default_get(model, ctx, buf);
		light_ctl_default_publish(model);
		save_on_flash(DEF_STATES);
	} else {
		(void)light_ctl_default_get(model, ctx, buf);
	}

	return 0;
}

static void light_ctl_temp_range_publish(struct bt_mesh_model *model)
{
	int err;
	struct net_buf_simple *msg = model->pub->msg;

	if (model->pub->addr == BT_MESH_ADDR_UNASSIGNED) {
		return;
	}

	bt_mesh_model_msg_init(msg, BT_MESH_MODEL_LIGHT_CTL_TEMP_RANGE_STATUS);
	net_buf_simple_add_u8(msg, ctl->temp->status_code);
	net_buf_simple_add_le16(msg, ctl->temp->range_min);
	net_buf_simple_add_le16(msg, ctl->temp->range_max);

	err = bt_mesh_model_publish(model);
	if (err) {
		printk("bt_mesh_model_publish err %d\n", err);
	}
}

static int light_ctl_temp_range_set_unack(struct bt_mesh_model *model,
					  struct bt_mesh_msg_ctx *ctx,
					  struct net_buf_simple *buf)
{
	uint16_t min, max;

	min = net_buf_simple_pull_le16(buf);
	max = net_buf_simple_pull_le16(buf);

	/* This is as per 6.1.3.1 in Mesh Model Specification */
	if (min < TEMP_MIN || min > TEMP_MAX ||
	    max < TEMP_MIN || max > TEMP_MAX) {
		return 0;
	}

	if (min <= max) {
		ctl->temp->status_code = RANGE_SUCCESSFULLY_UPDATED;

		if (ctl->temp->range_min != min ||
		    ctl->temp->range_max != max) {

			ctl->temp->range_min = min;
			ctl->temp->range_max = max;

			light_ctl_temp_range_publish(model);
			save_on_flash(TEMPERATURE_RANGE);
		}
	} else {
		/* The provided value for Range Max cannot be set */
		ctl->temp->status_code = CANNOT_SET_RANGE_MAX;
		return 0;
	}

	return 0;
}

static int light_ctl_temp_range_set(struct bt_mesh_model *model,
				    struct bt_mesh_msg_ctx *ctx,
				    struct net_buf_simple *buf)
{
	uint16_t min, max;

	min = net_buf_simple_pull_le16(buf);
	max = net_buf_simple_pull_le16(buf);

	/* This is as per 6.1.3.1 in Mesh Model Specification */
	if (min < TEMP_MIN || min > TEMP_MAX ||
	    max < TEMP_MIN || max > TEMP_MAX) {
		return 0;
	}

	if (min <= max) {
		ctl->temp->status_code = RANGE_SUCCESSFULLY_UPDATED;

		if (ctl->temp->range_min != min ||
		    ctl->temp->range_max != max) {

			ctl->temp->range_min = min;
			ctl->temp->range_max = max;

			(void)light_ctl_temp_range_get(model, ctx, buf);
			light_ctl_temp_range_publish(model);
			save_on_flash(TEMPERATURE_RANGE);
		} else {
			(void)light_ctl_temp_range_get(model, ctx, buf);
		}
	} else {
		/* The provided value for Range Max cannot be set */
		ctl->temp->status_code = CANNOT_SET_RANGE_MAX;
		return 0;
	}

	return 0;
}

/* Light CTL Client message handlers */
static int light_ctl_status(struct bt_mesh_model *model,
			    struct bt_mesh_msg_ctx *ctx,
			    struct net_buf_simple *buf)
{
	printk("Acknowledgement from LIGHT_CTL_SRV\n");
	printk("Present CTL Lightness = %04x\n", net_buf_simple_pull_le16(buf));
	printk("Present CTL Temperature = %04x\n",
	       net_buf_simple_pull_le16(buf));

	if (buf->len == 5U) {
		printk("Target CTL Lightness = %04x\n",
		       net_buf_simple_pull_le16(buf));
		printk("Target CTL Temperature = %04x\n",
		       net_buf_simple_pull_le16(buf));
		printk("Remaining Time = %02x\n", net_buf_simple_pull_u8(buf));
	}

	return 0;
}

static int light_ctl_temp_range_status(struct bt_mesh_model *model,
				       struct bt_mesh_msg_ctx *ctx,
				       struct net_buf_simple *buf)
{
	printk("Acknowledgement from LIGHT_CTL_SRV (Temperature Range)\n");
	printk("Status Code = %02x\n", net_buf_simple_pull_u8(buf));
	printk("Range Min = %04x\n", net_buf_simple_pull_le16(buf));
	printk("Range Max = %04x\n", net_buf_simple_pull_le16(buf));

	return 0;
}

static int light_ctl_temp_status(struct bt_mesh_model *model,
				 struct bt_mesh_msg_ctx *ctx,
				 struct net_buf_simple *buf)
{
	printk("Acknowledgement from LIGHT_CTL_TEMP_SRV\n");
	printk("Present CTL Temperature = %04x\n",
	       net_buf_simple_pull_le16(buf));
	printk("Present CTL Delta UV = %04x\n",
	       net_buf_simple_pull_le16(buf));

	if (buf->len == 5U) {
		printk("Target CTL Temperature = %04x\n",
		       net_buf_simple_pull_le16(buf));
		printk("Target CTL Delta UV = %04x\n",
		       net_buf_simple_pull_le16(buf));
		printk("Remaining Time = %02x\n", net_buf_simple_pull_u8(buf));
	}

	return 0;
}

static int light_ctl_default_status(struct bt_mesh_model *model,
				    struct bt_mesh_msg_ctx *ctx,
				    struct net_buf_simple *buf)
{
	printk("Acknowledgement from LIGHT_CTL_SRV (Default)\n");
	printk("Lightness = %04x\n", net_buf_simple_pull_le16(buf));
	printk("Temperature = %04x\n", net_buf_simple_pull_le16(buf));
	printk("Delta UV = %04x\n", net_buf_simple_pull_le16(buf));

	return 0;
}

/* Light CTL Temp. Server message handlers */
static int light_ctl_temp_get(struct bt_mesh_model *model,
			      struct bt_mesh_msg_ctx *ctx,
			      struct net_buf_simple *buf)
{
	struct net_buf_simple *msg = NET_BUF_SIMPLE(2 + 9 + 4);

	bt_mesh_model_msg_init(msg, BT_MESH_MODEL_LIGHT_CTL_TEMP_STATUS);
	net_buf_simple_add_le16(msg, (uint16_t) get_current(CTL_TEMP));
	net_buf_simple_add_le16(msg, (int16_t) get_current(CTL_DELTA_UV));

	if (ctl->temp->current == ctl->temp->target &&
	    ctl->duv->current == ctl->duv->target) {
		goto send;
	}

	if (ctl->transition->counter) {
		calculate_rt(ctl->transition);
		net_buf_simple_add_le16(msg, (uint16_t) get_target(CTL_TEMP));
		net_buf_simple_add_le16(msg, (int16_t) get_target(CTL_DELTA_UV));
		net_buf_simple_add_u8(msg, ctl->transition->rt);
	}

send:
	if (bt_mesh_model_send(model, ctx, msg, NULL, NULL)) {
		printk("Unable to send LightCTL Temp. Status response\n");
	}

	return 0;
}

void light_ctl_temp_publish(struct bt_mesh_model *model)
{
	int err;
	struct net_buf_simple *msg = model->pub->msg;

	if (model->pub->addr == BT_MESH_ADDR_UNASSIGNED) {
		return;
	}

	bt_mesh_model_msg_init(msg, BT_MESH_MODEL_LIGHT_CTL_TEMP_STATUS);
	net_buf_simple_add_le16(msg, (uint16_t) get_current(CTL_TEMP));
	net_buf_simple_add_le16(msg, (int16_t) get_current(CTL_DELTA_UV));

	if (ctl->transition->counter) {
		calculate_rt(ctl->transition);
		net_buf_simple_add_le16(msg, (uint16_t) get_target(CTL_TEMP));
		net_buf_simple_add_le16(msg, (int16_t) get_target(CTL_DELTA_UV));
		net_buf_simple_add_u8(msg, ctl->transition->rt);
	}

	err = bt_mesh_model_publish(model);
	if (err) {
		printk("bt_mesh_model_publish err %d\n", err);
	}
}

static int light_ctl_temp_set_unack(struct bt_mesh_model *model,
				    struct bt_mesh_msg_ctx *ctx,
				    struct net_buf_simple *buf)
{
	uint8_t tid, tt, delay;
	int16_t delta_uv;
	uint16_t temp;
	int64_t now;

	temp = net_buf_simple_pull_le16(buf);
	delta_uv = (int16_t) net_buf_simple_pull_le16(buf);
	tid = net_buf_simple_pull_u8(buf);

	if (temp < TEMP_MIN || temp > TEMP_MAX) {
		return 0;
	}

	now = k_uptime_get();
	if (ctl->last_tid == tid &&
	    ctl->last_src_addr == ctx->addr &&
	    ctl->last_dst_addr == ctx->recv_dst &&
	    (now - ctl->last_msg_timestamp <= (6 * MSEC_PER_SEC))) {
		return 0;
	}

	switch (buf->len) {
	case 0x00:      /* No optional fields are available */
		tt = ctl->tt;
		delay = 0U;
		break;
	case 0x02:      /* Optional fields are available */
		tt = net_buf_simple_pull_u8(buf);
		if ((tt & 0x3F) == 0x3F) {
			return 0;
		}

		delay = net_buf_simple_pull_u8(buf);
		break;
	default:
		return 0;
	}

	ctl->transition->counter = 0U;
	k_timer_stop(&ctl->transition->timer);

	ctl->last_tid = tid;
	ctl->last_src_addr = ctx->addr;
	ctl->last_dst_addr = ctx->recv_dst;
	ctl->last_msg_timestamp = now;
	ctl->transition->tt = tt;
	ctl->transition->delay = delay;
	ctl->transition->type = NON_MOVE;
	set_target(CTL_TEMP, &temp);
	set_target(CTL_DELTA_UV, &delta_uv);

	if (ctl->temp->target != ctl->temp->current ||
	    ctl->duv->target != ctl->duv->current) {
		set_transition_values(CTL_TEMP);
	} else {
		return 0;
	}

	/* For Instantaneous Transition */
	if (ctl->transition->counter == 0U) {
		ctl->temp->current = ctl->temp->target;
		ctl->duv->current = ctl->duv->target;
	}

	ctl->transition->just_started = true;
	light_ctl_temp_publish(model);
	light_ctl_temp_handler();

	return 0;
}

static int light_ctl_temp_set(struct bt_mesh_model *model,
			      struct bt_mesh_msg_ctx *ctx,
			      struct net_buf_simple *buf)
{
	uint8_t tid, tt, delay;
	int16_t delta_uv;
	uint16_t temp;
	int64_t now;

	temp = net_buf_simple_pull_le16(buf);
	delta_uv = (int16_t) net_buf_simple_pull_le16(buf);
	tid = net_buf_simple_pull_u8(buf);

	if (temp < TEMP_MIN || temp > TEMP_MAX) {
		return 0;
	}

	now = k_uptime_get();
	if (ctl->last_tid == tid &&
	    ctl->last_src_addr == ctx->addr &&
	    ctl->last_dst_addr == ctx->recv_dst &&
	    (now - ctl->last_msg_timestamp <= (6 * MSEC_PER_SEC))) {
		(void)light_ctl_temp_get(model, ctx, buf);
		return 0;
	}

	switch (buf->len) {
	case 0x00:      /* No optional fields are available */
		tt = ctl->tt;
		delay = 0U;
		break;
	case 0x02:      /* Optional fields are available */
		tt = net_buf_simple_pull_u8(buf);
		if ((tt & 0x3F) == 0x3F) {
			return 0;
		}

		delay = net_buf_simple_pull_u8(buf);
		break;
	default:
		return 0;
	}

	ctl->transition->counter = 0U;
	k_timer_stop(&ctl->transition->timer);

	ctl->last_tid = tid;
	ctl->last_src_addr = ctx->addr;
	ctl->last_dst_addr = ctx->recv_dst;
	ctl->last_msg_timestamp = now;
	ctl->transition->tt = tt;
	ctl->transition->delay = delay;
	ctl->transition->type = NON_MOVE;
	set_target(CTL_TEMP, &temp);
	set_target(CTL_DELTA_UV, &delta_uv);

	if (ctl->temp->target != ctl->temp->current ||
	    ctl->duv->target != ctl->duv->current) {
		set_transition_values(CTL_TEMP);
	} else {
		(void)light_ctl_temp_get(model, ctx, buf);
		return 0;
	}

	/* For Instantaneous Transition */
	if (ctl->transition->counter == 0U) {
		ctl->temp->current = ctl->temp->target;
		ctl->duv->current = ctl->duv->target;
	}

	ctl->transition->just_started = true;
	(void)light_ctl_temp_get(model, ctx, buf);
	light_ctl_temp_publish(model);
	light_ctl_temp_handler();

	return 0;
}

/* Generic Level (TEMPERATURE) Server message handlers */
static int gen_level_get_temp(struct bt_mesh_model *model,
			      struct bt_mesh_msg_ctx *ctx,
			      struct net_buf_simple *buf)
{
	struct net_buf_simple *msg = NET_BUF_SIMPLE(2 + 5 + 4);

	bt_mesh_model_msg_init(msg, BT_MESH_MODEL_OP_GEN_LEVEL_STATUS);
	net_buf_simple_add_le16(msg, (int16_t) get_current(LEVEL_TEMP));

	if (ctl->temp->current == ctl->temp->target) {
		goto send;
	}

	if (ctl->transition->counter) {
		calculate_rt(ctl->transition);
		net_buf_simple_add_le16(msg, (int16_t) get_target(LEVEL_TEMP));
		net_buf_simple_add_u8(msg, ctl->transition->rt);
	}

send:
	if (bt_mesh_model_send(model, ctx, msg, NULL, NULL)) {
		printk("Unable to send GEN_LEVEL_SRV Status response\n");
	}

	return 0;
}

void gen_level_publish_temp(struct bt_mesh_model *model)
{
	int err;
	struct net_buf_simple *msg = model->pub->msg;

	if (model->pub->addr == BT_MESH_ADDR_UNASSIGNED) {
		return;
	}

	bt_mesh_model_msg_init(msg, BT_MESH_MODEL_OP_GEN_LEVEL_STATUS);
	net_buf_simple_add_le16(msg, (int16_t) get_current(LEVEL_TEMP));

	if (ctl->transition->counter) {
		calculate_rt(ctl->transition);
		net_buf_simple_add_le16(msg, (int16_t) get_target(LEVEL_TEMP));
		net_buf_simple_add_u8(msg, ctl->transition->rt);
	}

	err = bt_mesh_model_publish(model);
	if (err) {
		printk("bt_mesh_model_publish err %d\n", err);
	}
}

static int gen_level_set_unack_temp(struct bt_mesh_model *model,
				    struct bt_mesh_msg_ctx *ctx,
				    struct net_buf_simple *buf)
{
	uint8_t tid, tt, delay;
	int16_t level;
	int64_t now;

	level = (int16_t) net_buf_simple_pull_le16(buf);
	tid = net_buf_simple_pull_u8(buf);

	now = k_uptime_get();
	if (ctl->last_tid == tid &&
	    ctl->last_src_addr == ctx->addr &&
	    ctl->last_dst_addr == ctx->recv_dst &&
	    (now - ctl->last_msg_timestamp <= (6 * MSEC_PER_SEC))) {
		return 0;
	}

	switch (buf->len) {
	case 0x00:      /* No optional fields are available */
		tt = ctl->tt;
		delay = 0U;
		break;
	case 0x02:      /* Optional fields are available */
		tt = net_buf_simple_pull_u8(buf);
		if ((tt & 0x3F) == 0x3F) {
			return 0;
		}

		delay = net_buf_simple_pull_u8(buf);
		break;
	default:
		return 0;
	}

	ctl->transition->counter = 0U;
	k_timer_stop(&ctl->transition->timer);

	ctl->last_tid = tid;
	ctl->last_src_addr = ctx->addr;
	ctl->last_dst_addr = ctx->recv_dst;
	ctl->last_msg_timestamp = now;
	ctl->transition->tt = tt;
	ctl->transition->delay = delay;
	ctl->transition->type = NON_MOVE;
	set_target(LEVEL_TEMP, &level);

	if (ctl->temp->target != ctl->temp->current) {
		set_transition_values(LEVEL_TEMP);
	} else {
		return 0;
	}

	/* For Instantaneous Transition */
	if (ctl->transition->counter == 0U) {
		ctl->temp->current = ctl->temp->target;
	}

	ctl->transition->just_started = true;
	gen_level_publish_temp(model);
	level_temp_handler();

	return 0;
}

static int gen_level_set_temp(struct bt_mesh_model *model,
			      struct bt_mesh_msg_ctx *ctx,
			      struct net_buf_simple *buf)
{
	uint8_t tid, tt, delay;
	int16_t level;
	int64_t now;

	level = (int16_t) net_buf_simple_pull_le16(buf);
	tid = net_buf_simple_pull_u8(buf);

	now = k_uptime_get();
	if (ctl->last_tid == tid &&
	    ctl->last_src_addr == ctx->addr &&
	    ctl->last_dst_addr == ctx->recv_dst &&
	    (now - ctl->last_msg_timestamp <= (6 * MSEC_PER_SEC))) {
		(void)gen_level_get_temp(model, ctx, buf);
		return 0;
	}

	switch (buf->len) {
	case 0x00:      /* No optional fields are available */
		tt = ctl->tt;
		delay = 0U;
		break;
	case 0x02:      /* Optional fields are available */
		tt = net_buf_simple_pull_u8(buf);
		if ((tt & 0x3F) == 0x3F) {
			return 0;
		}

		delay = net_buf_simple_pull_u8(buf);
		break;
	default:
		return 0;
	}

	ctl->transition->counter = 0U;
	k_timer_stop(&ctl->transition->timer);

	ctl->last_tid = tid;
	ctl->last_src_addr = ctx->addr;
	ctl->last_dst_addr = ctx->recv_dst;
	ctl->last_msg_timestamp = now;
	ctl->transition->tt = tt;
	ctl->transition->delay = delay;
	ctl->transition->type = NON_MOVE;
	set_target(LEVEL_TEMP, &level);

	if (ctl->temp->target != ctl->temp->current) {
		set_transition_values(LEVEL_TEMP);
	} else {
		(void)gen_level_get_temp(model, ctx, buf);
		return 0;
	}

	/* For Instantaneous Transition */
	if (ctl->transition->counter == 0U) {
		ctl->temp->current = ctl->temp->target;
	}

	ctl->transition->just_started = true;
	(void)gen_level_get_temp(model, ctx, buf);
	gen_level_publish_temp(model);
	level_temp_handler();

	return 0;
}

static int gen_delta_set_unack_temp(struct bt_mesh_model *model,
				    struct bt_mesh_msg_ctx *ctx,
				    struct net_buf_simple *buf)
{
	uint8_t tid, tt, delay;
	static int16_t last_level;
	int32_t target, delta;
	int64_t now;

	delta = (int32_t) net_buf_simple_pull_le32(buf);
	tid = net_buf_simple_pull_u8(buf);

	now = k_uptime_get();
	if (ctl->last_tid == tid &&
	    ctl->last_src_addr == ctx->addr &&
	    ctl->last_dst_addr == ctx->recv_dst &&
	    (now - ctl->last_msg_timestamp <= (6 * MSEC_PER_SEC))) {

		if (ctl->temp->delta == delta) {
			return 0;
		}
		target = last_level + delta;

	} else {
		last_level = (int16_t) get_current(LEVEL_TEMP);
		target = last_level + delta;
	}

	switch (buf->len) {
	case 0x00:      /* No optional fields are available */
		tt = ctl->tt;
		delay = 0U;
		break;
	case 0x02:      /* Optional fields are available */
		tt = net_buf_simple_pull_u8(buf);
		if ((tt & 0x3F) == 0x3F) {
			return 0;
		}

		delay = net_buf_simple_pull_u8(buf);
		break;
	default:
		return 0;
	}

	ctl->transition->counter = 0U;
	k_timer_stop(&ctl->transition->timer);

	ctl->last_tid = tid;
	ctl->last_src_addr = ctx->addr;
	ctl->last_dst_addr = ctx->recv_dst;
	ctl->last_msg_timestamp = now;
	ctl->transition->tt = tt;
	ctl->transition->delay = delay;
	ctl->transition->type = NON_MOVE;

	if (target < INT16_MIN) {
		target = INT16_MIN;
	} else if (target > INT16_MAX) {
		target = INT16_MAX;
	}

	set_target(LEVEL_TEMP, &target);

	if (ctl->temp->target != ctl->temp->current) {
		set_transition_values(LEVEL_TEMP);
	} else {
		return 0;
	}

	/* For Instantaneous Transition */
	if (ctl->transition->counter == 0U) {
		ctl->temp->current = ctl->temp->target;
	}

	ctl->transition->just_started = true;
	gen_level_publish_temp(model);
	level_temp_handler();

	return 0;
}

static int gen_delta_set_temp(struct bt_mesh_model *model,
			      struct bt_mesh_msg_ctx *ctx,
			      struct net_buf_simple *buf)
{
	uint8_t tid, tt, delay;
	static int16_t last_level;
	int32_t target, delta;
	int64_t now;

	delta = (int32_t) net_buf_simple_pull_le32(buf);
	tid = net_buf_simple_pull_u8(buf);

	now = k_uptime_get();
	if (ctl->last_tid == tid &&
	    ctl->last_src_addr == ctx->addr &&
	    ctl->last_dst_addr == ctx->recv_dst &&
	    (now - ctl->last_msg_timestamp <= (6 * MSEC_PER_SEC))) {

		if (ctl->temp->delta == delta) {
			(void)gen_level_get_temp(model, ctx, buf);
			return 0;
		}
		target = last_level + delta;

	} else {
		last_level = (int16_t) get_current(LEVEL_TEMP);
		target = last_level + delta;
	}

	switch (buf->len) {
	case 0x00:      /* No optional fields are available */
		tt = ctl->tt;
		delay = 0U;
		break;
	case 0x02:      /* Optional fields are available */
		tt = net_buf_simple_pull_u8(buf);
		if ((tt & 0x3F) == 0x3F) {
			return 0;
		}

		delay = net_buf_simple_pull_u8(buf);
		break;
	default:
		return 0;
	}

	ctl->transition->counter = 0U;
	k_timer_stop(&ctl->transition->timer);

	ctl->last_tid = tid;
	ctl->last_src_addr = ctx->addr;
	ctl->last_dst_addr = ctx->recv_dst;
	ctl->last_msg_timestamp = now;
	ctl->transition->tt = tt;
	ctl->transition->delay = delay;
	ctl->transition->type = NON_MOVE;

	if (target < INT16_MIN) {
		target = INT16_MIN;
	} else if (target > INT16_MAX) {
		target = INT16_MAX;
	}

	set_target(LEVEL_TEMP, &target);

	if (ctl->temp->target != ctl->temp->current) {
		set_transition_values(LEVEL_TEMP);
	} else {
		(void)gen_level_get_temp(model, ctx, buf);
		return 0;
	}

	/* For Instantaneous Transition */
	if (ctl->transition->counter == 0U) {
		ctl->temp->current = ctl->temp->target;
	}

	ctl->transition->just_started = true;
	(void)gen_level_get_temp(model, ctx, buf);
	gen_level_publish_temp(model);
	level_temp_handler();

	return 0;
}

static int gen_move_set_unack_temp(struct bt_mesh_model *model,
				   struct bt_mesh_msg_ctx *ctx,
				   struct net_buf_simple *buf)
{
	uint8_t tid, tt, delay;
	int16_t delta;
	uint16_t target;
	int64_t now;

	delta = (int16_t) net_buf_simple_pull_le16(buf);
	tid = net_buf_simple_pull_u8(buf);

	now = k_uptime_get();
	if (ctl->last_tid == tid &&
	    ctl->last_src_addr == ctx->addr &&
	    ctl->last_dst_addr == ctx->recv_dst &&
	    (now - ctl->last_msg_timestamp <= (6 * MSEC_PER_SEC))) {
		return 0;
	}

	switch (buf->len) {
	case 0x00:      /* No optional fields are available */
		tt = ctl->tt;
		delay = 0U;
		break;
	case 0x02:      /* Optional fields are available */
		tt = net_buf_simple_pull_u8(buf);
		if ((tt & 0x3F) == 0x3F) {
			return 0;
		}

		delay = net_buf_simple_pull_u8(buf);
		break;
	default:
		return 0;
	}

	ctl->transition->counter = 0U;
	k_timer_stop(&ctl->transition->timer);

	ctl->last_tid = tid;
	ctl->last_src_addr = ctx->addr;
	ctl->last_dst_addr = ctx->recv_dst;
	ctl->last_msg_timestamp = now;
	ctl->transition->tt = tt;
	ctl->transition->delay = delay;
	ctl->transition->type = MOVE;
	ctl->temp->delta = delta;

	if (delta < 0) {
		target = ctl->temp->range_min;
	} else if (delta > 0) {
		target = ctl->temp->range_max;
	} else if (delta == 0) {
		target = ctl->temp->current;
	}
	set_target(MOVE_TEMP, &target);

	if (ctl->temp->target != ctl->temp->current) {
		set_transition_values(MOVE_TEMP);
	} else {
		return 0;
	}

	if (ctl->transition->counter == 0U) {
		return 0;
	}

	ctl->transition->just_started = true;
	gen_level_publish_temp(model);
	level_temp_handler();

	return 0;
}

static int gen_move_set_temp(struct bt_mesh_model *model,
			     struct bt_mesh_msg_ctx *ctx,
			     struct net_buf_simple *buf)
{
	uint8_t tid, tt, delay;
	int16_t delta;
	uint16_t target;
	int64_t now;

	delta = (int16_t) net_buf_simple_pull_le16(buf);
	tid = net_buf_simple_pull_u8(buf);

	now = k_uptime_get();
	if (ctl->last_tid == tid &&
	    ctl->last_src_addr == ctx->addr &&
	    ctl->last_dst_addr == ctx->recv_dst &&
	    (now - ctl->last_msg_timestamp <= (6 * MSEC_PER_SEC))) {
		(void)gen_level_get_temp(model, ctx, buf);
		return 0;
	}

	switch (buf->len) {
	case 0x00:      /* No optional fields are available */
		tt = ctl->tt;
		delay = 0U;
		break;
	case 0x02:      /* Optional fields are available */
		tt = net_buf_simple_pull_u8(buf);
		if ((tt & 0x3F) == 0x3F) {
			return 0;
		}

		delay = net_buf_simple_pull_u8(buf);
		break;
	default:
		return 0;
	}

	ctl->transition->counter = 0U;
	k_timer_stop(&ctl->transition->timer);

	ctl->last_tid = tid;
	ctl->last_src_addr = ctx->addr;
	ctl->last_dst_addr = ctx->recv_dst;
	ctl->last_msg_timestamp = now;
	ctl->transition->tt = tt;
	ctl->transition->delay = delay;
	ctl->transition->type = MOVE;
	ctl->temp->delta = delta;

	if (delta < 0) {
		target = ctl->temp->range_min;
	} else if (delta > 0) {
		target = ctl->temp->range_max;
	} else if (delta == 0) {
		target = ctl->temp->current;
	}
	set_target(MOVE_TEMP, &target);

	if (ctl->temp->target != ctl->temp->current) {
		set_transition_values(MOVE_TEMP);
	} else {
		(void)gen_level_get_temp(model, ctx, buf);
		return 0;
	}

	if (ctl->transition->counter == 0U) {
		return 0;
	}

	ctl->transition->just_started = true;
	(void)gen_level_get_temp(model, ctx, buf);
	gen_level_publish_temp(model);
	level_temp_handler();

	return 0;
}

/* Generic Level (TEMPERATURE) Client message handlers */
static int gen_level_status_temp(struct bt_mesh_model *model,
				 struct bt_mesh_msg_ctx *ctx,
				 struct net_buf_simple *buf)
{
	printk("Acknowledgement from GEN_LEVEL_SRV\n");
	printk("Present Level = %04x\n", net_buf_simple_pull_le16(buf));

	if (buf->len == 3U) {
		printk("Target Level = %04x\n", net_buf_simple_pull_le16(buf));
		printk("Remaining Time = %02x\n", net_buf_simple_pull_u8(buf));
	}

	return 0;
}


/* message handlers (End) */

/* Mapping of message handlers for Generic OnOff Server (0x1000) */
static const struct bt_mesh_model_op gen_onoff_srv_op[] = {
	{ BT_MESH_MODEL_OP_2(0x82, 0x01), BT_MESH_LEN_EXACT(0), gen_onoff_get },
	{ BT_MESH_MODEL_OP_2(0x82, 0x02), BT_MESH_LEN_MIN(2),   gen_onoff_set },
	{ BT_MESH_MODEL_OP_2(0x82, 0x03), BT_MESH_LEN_MIN(2),   gen_onoff_set_unack },
	BT_MESH_MODEL_OP_END,
};

/* Mapping of message handlers for Generic OnOff Client (0x1001) */
static const struct bt_mesh_model_op gen_onoff_cli_op[] = {
	{ BT_MESH_MODEL_OP_2(0x82, 0x04), BT_MESH_LEN_MIN(1),   gen_onoff_status },
	BT_MESH_MODEL_OP_END,
};

/* Mapping of message handlers for Generic Level (Light) Server (0x1002) */
static const struct bt_mesh_model_op gen_level_srv_op[] = {
	{ BT_MESH_MODEL_OP_2(0x82, 0x05), BT_MESH_LEN_EXACT(0), gen_level_get },
	{ BT_MESH_MODEL_OP_2(0x82, 0x06), BT_MESH_LEN_MIN(3),   gen_level_set },
	{ BT_MESH_MODEL_OP_2(0x82, 0x07), BT_MESH_LEN_MIN(3),   gen_level_set_unack },
	{ BT_MESH_MODEL_OP_2(0x82, 0x09), BT_MESH_LEN_MIN(5),   gen_delta_set },
	{ BT_MESH_MODEL_OP_2(0x82, 0x0A), BT_MESH_LEN_MIN(5),   gen_delta_set_unack },
	{ BT_MESH_MODEL_OP_2(0x82, 0x0B), BT_MESH_LEN_MIN(3),   gen_move_set },
	{ BT_MESH_MODEL_OP_2(0x82, 0x0C), BT_MESH_LEN_MIN(3),   gen_move_set_unack },
	BT_MESH_MODEL_OP_END,
};

/* Mapping of message handlers for Generic Level (Light) Client (0x1003) */
static const struct bt_mesh_model_op gen_level_cli_op[] = {
	{ BT_MESH_MODEL_OP_2(0x82, 0x08), BT_MESH_LEN_MIN(2),   gen_level_status },
	BT_MESH_MODEL_OP_END,
};

/* Mapping of message handlers for Generic Default TT Server (0x1004) */
static const struct bt_mesh_model_op gen_def_trans_time_srv_op[] = {
	{ BT_MESH_MODEL_OP_2(0x82, 0x0D), BT_MESH_LEN_EXACT(0), gen_def_trans_time_get },
	{ BT_MESH_MODEL_OP_2(0x82, 0x0E), BT_MESH_LEN_EXACT(1), gen_def_trans_time_set },
	{ BT_MESH_MODEL_OP_2(0x82, 0x0F), BT_MESH_LEN_EXACT(1), gen_def_trans_time_set_unack },
	BT_MESH_MODEL_OP_END,
};

/* Mapping of message handlers for Generic Default TT Client (0x1005) */
static const struct bt_mesh_model_op gen_def_trans_time_cli_op[] = {
	{ BT_MESH_MODEL_OP_2(0x82, 0x10), BT_MESH_LEN_EXACT(1), gen_def_trans_time_status },
	BT_MESH_MODEL_OP_END,
};

/* Mapping of message handlers for Generic Power OnOff Server (0x1006) */
static const struct bt_mesh_model_op gen_power_onoff_srv_op[] = {
	{ BT_MESH_MODEL_OP_2(0x82, 0x11), BT_MESH_LEN_MIN(0),   gen_onpowerup_get },
	BT_MESH_MODEL_OP_END,
};

/* Mapping of message handlers for Generic Power OnOff Setup Server (0x1007) */
static const struct bt_mesh_model_op gen_power_onoff_setup_srv_op[] = {
	{ BT_MESH_MODEL_OP_2(0x82, 0x13), BT_MESH_LEN_EXACT(1), gen_onpowerup_set },
	{ BT_MESH_MODEL_OP_2(0x82, 0x14), BT_MESH_LEN_EXACT(1), gen_onpowerup_set_unack },
	BT_MESH_MODEL_OP_END,
};

/* Mapping of message handlers for Generic Power OnOff Client (0x1008) */
static const struct bt_mesh_model_op gen_power_onoff_cli_op[] = {
	{ BT_MESH_MODEL_OP_2(0x82, 0x12), BT_MESH_LEN_EXACT(1), gen_onpowerup_status },
	BT_MESH_MODEL_OP_END,
};

/* Mapping of message handlers for Light Lightness Server (0x1300) */
static const struct bt_mesh_model_op light_lightness_srv_op[] = {
	{ BT_MESH_MODEL_OP_2(0x82, 0x4B), BT_MESH_LEN_EXACT(0), light_lightness_get },
	{ BT_MESH_MODEL_OP_2(0x82, 0x4C), BT_MESH_LEN_MIN(3),   light_lightness_set },
	{ BT_MESH_MODEL_OP_2(0x82, 0x4D), BT_MESH_LEN_MIN(3),   light_lightness_set_unack },
	{ BT_MESH_MODEL_OP_2(0x82, 0x4F), BT_MESH_LEN_EXACT(0), light_lightness_linear_get },
	{ BT_MESH_MODEL_OP_2(0x82, 0x50), BT_MESH_LEN_MIN(3),   light_lightness_linear_set },
	{ BT_MESH_MODEL_OP_2(0x82, 0x51), BT_MESH_LEN_MIN(3),
	  light_lightness_linear_set_unack },
	{ BT_MESH_MODEL_OP_2(0x82, 0x53), BT_MESH_LEN_EXACT(0), light_lightness_last_get },
	{ BT_MESH_MODEL_OP_2(0x82, 0x55), BT_MESH_LEN_EXACT(0), light_lightness_default_get },
	{ BT_MESH_MODEL_OP_2(0x82, 0x57), BT_MESH_LEN_EXACT(0), light_lightness_range_get },
	BT_MESH_MODEL_OP_END,
};

/* Mapping of message handlers for Light Lightness Setup Server (0x1301) */
static const struct bt_mesh_model_op light_lightness_setup_srv_op[] = {
	{ BT_MESH_MODEL_OP_2(0x82, 0x59), BT_MESH_LEN_EXACT(2), light_lightness_default_set },
	{ BT_MESH_MODEL_OP_2(0x82, 0x5A), BT_MESH_LEN_EXACT(2),
	  light_lightness_default_set_unack },
	{ BT_MESH_MODEL_OP_2(0x82, 0x5B), BT_MESH_LEN_EXACT(4), light_lightness_range_set },
	{ BT_MESH_MODEL_OP_2(0x82, 0x5C), BT_MESH_LEN_EXACT(4), light_lightness_range_set_unack },
	BT_MESH_MODEL_OP_END,
};

/* Mapping of message handlers for Light Lightness Client (0x1302) */
static const struct bt_mesh_model_op light_lightness_cli_op[] = {
	{ BT_MESH_MODEL_OP_2(0x82, 0x4E), BT_MESH_LEN_MIN(2),   light_lightness_status },
	{ BT_MESH_MODEL_OP_2(0x82, 0x52), BT_MESH_LEN_MIN(2),   light_lightness_linear_status },
	{ BT_MESH_MODEL_OP_2(0x82, 0x54), BT_MESH_LEN_EXACT(2), light_lightness_last_status },
	{ BT_MESH_MODEL_OP_2(0x82, 0x56), BT_MESH_LEN_EXACT(2), light_lightness_default_status },
	{ BT_MESH_MODEL_OP_2(0x82, 0x58), BT_MESH_LEN_EXACT(5), light_lightness_range_status },
	BT_MESH_MODEL_OP_END,
};

/* Mapping of message handlers for Light CTL Server (0x1303) */
static const struct bt_mesh_model_op light_ctl_srv_op[] = {
	{ BT_MESH_MODEL_OP_2(0x82, 0x5D), BT_MESH_LEN_EXACT(0), light_ctl_get },
	{ BT_MESH_MODEL_OP_2(0x82, 0x5E), BT_MESH_LEN_MIN(7),   light_ctl_set },
	{ BT_MESH_MODEL_OP_2(0x82, 0x5F), BT_MESH_LEN_MIN(7),   light_ctl_set_unack },
	{ BT_MESH_MODEL_OP_2(0x82, 0x62), BT_MESH_LEN_EXACT(0), light_ctl_temp_range_get },
	{ BT_MESH_MODEL_OP_2(0x82, 0x67), BT_MESH_LEN_EXACT(0), light_ctl_default_get },
	BT_MESH_MODEL_OP_END,
};

/* Mapping of message handlers for Light CTL Setup Server (0x1304) */
static const struct bt_mesh_model_op light_ctl_setup_srv_op[] = {
	{ BT_MESH_MODEL_OP_2(0x82, 0x69), BT_MESH_LEN_EXACT(6), light_ctl_default_set },
	{ BT_MESH_MODEL_OP_2(0x82, 0x6A), BT_MESH_LEN_EXACT(6), light_ctl_default_set_unack },
	{ BT_MESH_MODEL_OP_2(0x82, 0x6B), BT_MESH_LEN_EXACT(4), light_ctl_temp_range_set },
	{ BT_MESH_MODEL_OP_2(0x82, 0x6C), BT_MESH_LEN_EXACT(4), light_ctl_temp_range_set_unack },
	BT_MESH_MODEL_OP_END,
};

/* Mapping of message handlers for Light CTL Client (0x1305) */
static const struct bt_mesh_model_op light_ctl_cli_op[] = {
	{ BT_MESH_MODEL_OP_2(0x82, 0x60), BT_MESH_LEN_MIN(4),   light_ctl_status },
	{ BT_MESH_MODEL_OP_2(0x82, 0x63), BT_MESH_LEN_EXACT(5), light_ctl_temp_range_status },
	{ BT_MESH_MODEL_OP_2(0x82, 0x66), BT_MESH_LEN_MIN(4),   light_ctl_temp_status },
	{ BT_MESH_MODEL_OP_2(0x82, 0x68), BT_MESH_LEN_EXACT(6), light_ctl_default_status },
	BT_MESH_MODEL_OP_END,
};

/* Mapping of message handlers for Light CTL Temp. Server (0x1306) */
static const struct bt_mesh_model_op light_ctl_temp_srv_op[] = {
	{ BT_MESH_MODEL_OP_2(0x82, 0x61), BT_MESH_LEN_EXACT(0), light_ctl_temp_get },
	{ BT_MESH_MODEL_OP_2(0x82, 0x64), BT_MESH_LEN_MIN(5),   light_ctl_temp_set },
	{ BT_MESH_MODEL_OP_2(0x82, 0x65), BT_MESH_LEN_MIN(5),   light_ctl_temp_set_unack },
	BT_MESH_MODEL_OP_END,
};

/* Mapping of message handlers for Vendor (0x4321) */
static const struct bt_mesh_model_op vnd_ops[] = {
	{ BT_MESH_MODEL_OP_3(0x01, CID_ZEPHYR), BT_MESH_LEN_EXACT(0), vnd_get },
	{ BT_MESH_MODEL_OP_3(0x02, CID_ZEPHYR), BT_MESH_LEN_EXACT(3), vnd_set },
	{ BT_MESH_MODEL_OP_3(0x03, CID_ZEPHYR), BT_MESH_LEN_EXACT(3), vnd_set_unack },
	{ BT_MESH_MODEL_OP_3(0x04, CID_ZEPHYR), BT_MESH_LEN_EXACT(6), vnd_status },
	BT_MESH_MODEL_OP_END,
};

/* Mapping of message handlers for Generic Level (Temp.) Server (0x1002) */
static const struct bt_mesh_model_op gen_level_srv_op_temp[] = {
	{ BT_MESH_MODEL_OP_2(0x82, 0x05), BT_MESH_LEN_EXACT(0), gen_level_get_temp },
	{ BT_MESH_MODEL_OP_2(0x82, 0x06), BT_MESH_LEN_MIN(3),   gen_level_set_temp },
	{ BT_MESH_MODEL_OP_2(0x82, 0x07), BT_MESH_LEN_MIN(3),   gen_level_set_unack_temp },
	{ BT_MESH_MODEL_OP_2(0x82, 0x09), BT_MESH_LEN_MIN(5),   gen_delta_set_temp },
	{ BT_MESH_MODEL_OP_2(0x82, 0x0A), BT_MESH_LEN_MIN(5),   gen_delta_set_unack_temp },
	{ BT_MESH_MODEL_OP_2(0x82, 0x0B), BT_MESH_LEN_MIN(3),   gen_move_set_temp },
	{ BT_MESH_MODEL_OP_2(0x82, 0x0C), BT_MESH_LEN_MIN(3),   gen_move_set_unack_temp },
	BT_MESH_MODEL_OP_END,
};

/* Mapping of message handlers for Generic Level (Temp.) Client (0x1003) */
static const struct bt_mesh_model_op gen_level_cli_op_temp[] = {
	{ BT_MESH_MODEL_OP_2(0x82, 0x08), BT_MESH_LEN_MIN(2), gen_level_status_temp },
	BT_MESH_MODEL_OP_END,
};

struct bt_mesh_model root_models[] = {
	BT_MESH_MODEL_CFG_SRV,
	BT_MESH_MODEL_HEALTH_SRV(&health_srv, &health_pub),

	BT_MESH_MODEL(BT_MESH_MODEL_ID_GEN_ONOFF_SRV,
		      gen_onoff_srv_op, &gen_onoff_srv_pub_root,
		      NULL),
	BT_MESH_MODEL(BT_MESH_MODEL_ID_GEN_ONOFF_CLI,
		      gen_onoff_cli_op, &gen_onoff_cli_pub_root,
		      NULL),

	BT_MESH_MODEL(BT_MESH_MODEL_ID_GEN_LEVEL_SRV,
		      gen_level_srv_op, &gen_level_srv_pub_root,
		      NULL),
	BT_MESH_MODEL(BT_MESH_MODEL_ID_GEN_LEVEL_CLI,
		      gen_level_cli_op, &gen_level_cli_pub_root,
		      NULL),

	BT_MESH_MODEL(BT_MESH_MODEL_ID_GEN_DEF_TRANS_TIME_SRV,
		      gen_def_trans_time_srv_op,
		      &gen_def_trans_time_srv_pub,
		      NULL),
	BT_MESH_MODEL(BT_MESH_MODEL_ID_GEN_DEF_TRANS_TIME_CLI,
		      gen_def_trans_time_cli_op,
		      &gen_def_trans_time_cli_pub,
		      NULL),

	BT_MESH_MODEL(BT_MESH_MODEL_ID_GEN_POWER_ONOFF_SRV,
		      gen_power_onoff_srv_op, &gen_power_onoff_srv_pub,
		      NULL),
	BT_MESH_MODEL(BT_MESH_MODEL_ID_GEN_POWER_ONOFF_SETUP_SRV,
		      gen_power_onoff_setup_srv_op,
		      &gen_power_onoff_srv_pub,
		      NULL),
	BT_MESH_MODEL(BT_MESH_MODEL_ID_GEN_POWER_ONOFF_CLI,
		      gen_power_onoff_cli_op, &gen_power_onoff_cli_pub,
		      NULL),

	BT_MESH_MODEL(BT_MESH_MODEL_ID_LIGHT_LIGHTNESS_SRV,
		      light_lightness_srv_op, &light_lightness_srv_pub,
		      NULL),
	BT_MESH_MODEL(BT_MESH_MODEL_ID_LIGHT_LIGHTNESS_SETUP_SRV,
		      light_lightness_setup_srv_op,
		      &light_lightness_srv_pub,
		      NULL),
	BT_MESH_MODEL(BT_MESH_MODEL_ID_LIGHT_LIGHTNESS_CLI,
		      light_lightness_cli_op, &light_lightness_cli_pub,
		      NULL),

	BT_MESH_MODEL(BT_MESH_MODEL_ID_LIGHT_CTL_SRV,
		      light_ctl_srv_op, &light_ctl_srv_pub,
		      NULL),
	BT_MESH_MODEL(BT_MESH_MODEL_ID_LIGHT_CTL_SETUP_SRV,
		      light_ctl_setup_srv_op, &light_ctl_srv_pub,
		      NULL),
	BT_MESH_MODEL(BT_MESH_MODEL_ID_LIGHT_CTL_CLI,
		      light_ctl_cli_op, &light_ctl_cli_pub,
		      NULL),
};

struct bt_mesh_model vnd_models[] = {
	BT_MESH_MODEL_VND(CID_ZEPHYR, 0x4321, vnd_ops,
			  &vnd_pub, &vnd_user_data),
};

struct bt_mesh_model s0_models[] = {
	BT_MESH_MODEL(BT_MESH_MODEL_ID_GEN_LEVEL_SRV,
		      gen_level_srv_op_temp, &gen_level_srv_pub_s0,
		      NULL),
	BT_MESH_MODEL(BT_MESH_MODEL_ID_GEN_LEVEL_CLI,
		      gen_level_cli_op_temp, &gen_level_cli_pub_s0,
		      NULL),

	BT_MESH_MODEL(BT_MESH_MODEL_ID_LIGHT_CTL_TEMP_SRV,
		      light_ctl_temp_srv_op, &light_ctl_srv_pub,
		      NULL),
};

static struct bt_mesh_elem elements[] = {
	BT_MESH_ELEM(0, root_models, vnd_models),
	BT_MESH_ELEM(0, s0_models, BT_MESH_MODEL_NONE),
};

const struct bt_mesh_comp comp = {
	.cid = CID_ZEPHYR,
	.elem = elements,
	.elem_count = ARRAY_SIZE(elements),
};
