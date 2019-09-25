/* Bluetooth: Mesh Generic OnOff, Generic Level, Lighting & Vendor Models
 *
 * Copyright (c) 2018 Vikrant More
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/gpio.h>

#include "app_gpio.h"
#include "storage.h"

#include "ble_mesh.h"
#include "device_composition.h"
#include "state_binding.h"
#include "transition.h"

static struct bt_mesh_cfg_srv cfg_srv = {
	.relay = BT_MESH_RELAY_ENABLED,
	.beacon = BT_MESH_BEACON_ENABLED,

#if defined(CONFIG_BT_MESH_FRIEND)
	.frnd = BT_MESH_FRIEND_ENABLED,
#else
	.frnd = BT_MESH_FRIEND_NOT_SUPPORTED,
#endif

#if defined(CONFIG_BT_MESH_GATT_PROXY)
	.gatt_proxy = BT_MESH_GATT_PROXY_ENABLED,
#else
	.gatt_proxy = BT_MESH_GATT_PROXY_NOT_SUPPORTED,
#endif

	.default_ttl = 7,

	/* 2 transmissions with 20ms interval */
	.net_transmit = BT_MESH_TRANSMIT(2, 20),

	/* 3 transmissions with 20ms interval */
	.relay_retransmit = BT_MESH_TRANSMIT(3, 20),
};

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

/* Definitions of models user data (Start) */
struct generic_onoff_state gen_onoff_srv_root_user_data = {
	.transition = &transition,
};

struct generic_level_state gen_level_srv_root_user_data = {
	.transition = &transition,
};

struct gen_def_trans_time_state gen_def_trans_time_srv_user_data;

struct generic_onpowerup_state gen_power_onoff_srv_user_data;

struct light_lightness_state light_lightness_srv_user_data = {
	.transition = &transition,
};

struct light_ctl_state light_ctl_srv_user_data = {
	.transition = &transition,
};

struct vendor_state vnd_user_data;

struct generic_level_state gen_level_srv_s0_user_data = {
	.transition = &transition,
};
/* Definitions of models user data (End) */

static struct bt_mesh_elem elements[];

/* message handlers (Start) */

/* Generic OnOff Server message handlers */
static void gen_onoff_get(struct bt_mesh_model *model,
			  struct bt_mesh_msg_ctx *ctx,
			  struct net_buf_simple *buf)
{
	struct net_buf_simple *msg = NET_BUF_SIMPLE(2 + 3 + 4);
	struct generic_onoff_state *state = model->user_data;

	bt_mesh_model_msg_init(msg, BT_MESH_MODEL_OP_GEN_ONOFF_STATUS);
	net_buf_simple_add_u8(msg, state->onoff);

	if (lightness == target_lightness) {
		goto send;
	}

	if (state->transition->counter) {
		calculate_rt(state->transition);
		net_buf_simple_add_u8(msg, state->target_onoff);
		net_buf_simple_add_u8(msg, state->transition->rt);
	}

send:
	if (bt_mesh_model_send(model, ctx, msg, NULL, NULL)) {
		printk("Unable to send GEN_ONOFF_SRV Status response\n");
	}
}

void gen_onoff_publish(struct bt_mesh_model *model)
{
	int err;
	struct net_buf_simple *msg = model->pub->msg;
	struct generic_onoff_state *state = model->user_data;

	if (model->pub->addr == BT_MESH_ADDR_UNASSIGNED) {
		return;
	}

	bt_mesh_model_msg_init(msg, BT_MESH_MODEL_OP_GEN_ONOFF_STATUS);
	net_buf_simple_add_u8(msg, state->onoff);

	if (lightness == target_lightness) {
		goto publish;
	}

	if (state->transition->counter) {
		calculate_rt(state->transition);
		net_buf_simple_add_u8(msg, state->target_onoff);
		net_buf_simple_add_u8(msg, state->transition->rt);
	}

publish:
	err = bt_mesh_model_publish(model);
	if (err) {
		printk("bt_mesh_model_publish err %d\n", err);
	}
}

static void gen_onoff_set_unack(struct bt_mesh_model *model,
				struct bt_mesh_msg_ctx *ctx,
				struct net_buf_simple *buf)
{
	u8_t tid, onoff, tt, delay;
	s64_t now;
	struct generic_onoff_state *state = model->user_data;

	onoff = net_buf_simple_pull_u8(buf);
	tid = net_buf_simple_pull_u8(buf);

	if (onoff > STATE_ON) {
		return;
	}

	now = k_uptime_get();
	if (state->last_tid == tid &&
	    state->last_src_addr == ctx->addr &&
	    state->last_dst_addr == ctx->recv_dst &&
	    (now - state->last_msg_timestamp <= K_SECONDS(6))) {
		return;
	}

	switch (buf->len) {
	case 0x00:	/* No optional fields are available */
		tt = default_tt;
		delay = 0U;
		break;
	case 0x02:	/* Optional fields are available */
		tt = net_buf_simple_pull_u8(buf);
		if ((tt & 0x3F) == 0x3F) {
			return;
		}

		delay = net_buf_simple_pull_u8(buf);
		break;
	default:
		return;
	}

	*ptr_counter = 0U;
	k_timer_stop(ptr_timer);

	state->last_tid = tid;
	state->last_src_addr = ctx->addr;
	state->last_dst_addr = ctx->recv_dst;
	state->last_msg_timestamp = now;
	state->target_onoff = onoff;
	state->transition->tt = tt;
	state->transition->delay = delay;
	state->transition->type = ONOFF_TT;

	if (state->target_onoff != state->onoff) {
		onoff_tt_values(state);
	} else {
		if (lightness != light_lightness_srv_user_data.def &&
		    state->onoff == STATE_ON) {
			onoff_tt_values(state);
		} else {
			gen_onoff_publish(model);
			return;
		}
	}

	/* For Instantaneous Transition */
	if (state->transition->counter == 0U) {
		state->onoff = state->target_onoff;
	}

	state->transition->just_started = true;
	gen_onoff_publish(model);
	onoff_handler(state);
}

static void gen_onoff_set(struct bt_mesh_model *model,
			  struct bt_mesh_msg_ctx *ctx,
			  struct net_buf_simple *buf)
{
	u8_t tid, onoff, tt, delay;
	s64_t now;
	struct generic_onoff_state *state = model->user_data;

	onoff = net_buf_simple_pull_u8(buf);
	tid = net_buf_simple_pull_u8(buf);

	if (onoff > STATE_ON) {
		return;
	}

	now = k_uptime_get();
	if (state->last_tid == tid &&
	    state->last_src_addr == ctx->addr &&
	    state->last_dst_addr == ctx->recv_dst &&
	    (now - state->last_msg_timestamp <= K_SECONDS(6))) {
		gen_onoff_get(model, ctx, buf);
		return;
	}

	switch (buf->len) {
	case 0x00:	/* No optional fields are available */
		tt = default_tt;
		delay = 0U;
		break;
	case 0x02:	/* Optional fields are available */
		tt = net_buf_simple_pull_u8(buf);
		if ((tt & 0x3F) == 0x3F) {
			return;
		}

		delay = net_buf_simple_pull_u8(buf);
		break;
	default:
		return;
	}

	*ptr_counter = 0U;
	k_timer_stop(ptr_timer);

	state->last_tid = tid;
	state->last_src_addr = ctx->addr;
	state->last_dst_addr = ctx->recv_dst;
	state->last_msg_timestamp = now;
	state->target_onoff = onoff;
	state->transition->tt = tt;
	state->transition->delay = delay;
	state->transition->type = ONOFF_TT;

	if (state->target_onoff != state->onoff) {
		onoff_tt_values(state);
	} else {
		if (lightness != light_lightness_srv_user_data.def &&
		    state->onoff == STATE_ON) {
			onoff_tt_values(state);
		} else {
			gen_onoff_get(model, ctx, buf);
			gen_onoff_publish(model);
			return;
		}
	}

	/* For Instantaneous Transition */
	if (state->transition->counter == 0U) {
		state->onoff = state->target_onoff;
	}

	state->transition->just_started = true;
	gen_onoff_get(model, ctx, buf);
	gen_onoff_publish(model);
	onoff_handler(state);
}

/* Generic OnOff Client message handlers */
static void gen_onoff_status(struct bt_mesh_model *model,
			     struct bt_mesh_msg_ctx *ctx,
			     struct net_buf_simple *buf)
{
	printk("Acknownledgement from GEN_ONOFF_SRV\n");
	printk("Present OnOff = %02x\n", net_buf_simple_pull_u8(buf));

	if (buf->len == 2U) {
		printk("Target OnOff = %02x\n", net_buf_simple_pull_u8(buf));
		printk("Remaining Time = %02x\n", net_buf_simple_pull_u8(buf));
	}
}

/* Generic Level Server message handlers */
static void gen_level_get(struct bt_mesh_model *model,
			  struct bt_mesh_msg_ctx *ctx,
			  struct net_buf_simple *buf)
{
	struct net_buf_simple *msg = NET_BUF_SIMPLE(2 + 5 + 4);
	struct generic_level_state *state = model->user_data;

	bt_mesh_model_msg_init(msg, BT_MESH_MODEL_OP_GEN_LEVEL_STATUS);
	net_buf_simple_add_le16(msg, state->level);

	if (state->level == state->target_level) {
		goto send;
	}

	if (state->transition->counter) {
		if (state->transition->type == LEVEL_TT_MOVE ||
		    state->transition->type == LEVEL_TEMP_TT_MOVE) {
			net_buf_simple_add_le16(msg, state->target_level);
			net_buf_simple_add_u8(msg, UNKNOWN_VALUE);
		} else {
			calculate_rt(state->transition);
			net_buf_simple_add_le16(msg, state->target_level);
			net_buf_simple_add_u8(msg, state->transition->rt);
		}
	}

send:
	if (bt_mesh_model_send(model, ctx, msg, NULL, NULL)) {
		printk("Unable to send GEN_LEVEL_SRV Status response\n");
	}
}

void gen_level_publish(struct bt_mesh_model *model)
{
	int err;
	struct net_buf_simple *msg = model->pub->msg;
	struct generic_level_state *state = model->user_data;

	if (model->pub->addr == BT_MESH_ADDR_UNASSIGNED) {
		return;
	}

	bt_mesh_model_msg_init(msg, BT_MESH_MODEL_OP_GEN_LEVEL_STATUS);
	net_buf_simple_add_le16(msg, state->level);

	if (state->level == state->target_level) {
		goto publish;
	}

	if (state->transition->counter) {
		if (state->transition->type == LEVEL_TT_MOVE ||
		    state->transition->type == LEVEL_TEMP_TT_MOVE) {
			net_buf_simple_add_le16(msg, state->target_level);
			net_buf_simple_add_u8(msg, UNKNOWN_VALUE);
		} else {
			calculate_rt(state->transition);
			net_buf_simple_add_le16(msg, state->target_level);
			net_buf_simple_add_u8(msg, state->transition->rt);
		}
	}

publish:
	err = bt_mesh_model_publish(model);
	if (err) {
		printk("bt_mesh_model_publish err %d\n", err);
	}
}

static void gen_level_set_unack(struct bt_mesh_model *model,
				struct bt_mesh_msg_ctx *ctx,
				struct net_buf_simple *buf)
{
	u8_t tid, tt, delay;
	s16_t level;
	s64_t now;
	struct generic_level_state *state = model->user_data;

	level = (s16_t) net_buf_simple_pull_le16(buf);
	tid = net_buf_simple_pull_u8(buf);

	now = k_uptime_get();
	if (state->last_tid == tid &&
	    state->last_src_addr == ctx->addr &&
	    state->last_dst_addr == ctx->recv_dst &&
	    (now - state->last_msg_timestamp <= K_SECONDS(6))) {
		return;
	}

	switch (buf->len) {
	case 0x00:	/* No optional fields are available */
		tt = default_tt;
		delay = 0U;
		break;
	case 0x02:	/* Optional fields are available */
		tt = net_buf_simple_pull_u8(buf);
		if ((tt & 0x3F) == 0x3F) {
			return;
		}

		delay = net_buf_simple_pull_u8(buf);
		break;
	default:
		return;
	}

	*ptr_counter = 0U;
	k_timer_stop(ptr_timer);

	state->last_tid = tid;
	state->last_src_addr = ctx->addr;
	state->last_dst_addr = ctx->recv_dst;
	state->last_msg_timestamp = now;
	state->target_level = level;
	state->transition->tt = tt;
	state->transition->delay = delay;
	state->transition->type = UNKNOWN_TT;

	if (state->target_level != state->level) {
		level_tt_values(state);
	} else {
		gen_level_publish(model);
		return;
	}

	/* For Instantaneous Transition */
	if (state->transition->counter == 0U) {
		state->level = state->target_level;
	}

	state->transition->just_started = true;
	gen_level_publish(model);

	if (bt_mesh_model_elem(model)->addr == elements[0].addr) {
		/* Root element */
		state->transition->type = LEVEL_TT;
		level_lightness_handler(state);
	} else if (bt_mesh_model_elem(model)->addr == elements[1].addr) {
		/* Secondary element */
		state->transition->type = LEVEL_TEMP_TT;
		level_temp_handler(state);
	}
}

static void gen_level_set(struct bt_mesh_model *model,
			  struct bt_mesh_msg_ctx *ctx,
			  struct net_buf_simple *buf)
{
	u8_t tid, tt, delay;
	s16_t level;
	s64_t now;
	struct generic_level_state *state = model->user_data;

	level = (s16_t) net_buf_simple_pull_le16(buf);
	tid = net_buf_simple_pull_u8(buf);

	now = k_uptime_get();
	if (state->last_tid == tid &&
	    state->last_src_addr == ctx->addr &&
	    state->last_dst_addr == ctx->recv_dst &&
	    (now - state->last_msg_timestamp <= K_SECONDS(6))) {
		gen_level_get(model, ctx, buf);
		return;
	}

	switch (buf->len) {
	case 0x00:	/* No optional fields are available */
		tt = default_tt;
		delay = 0U;
		break;
	case 0x02:	/* Optional fields are available */
		tt = net_buf_simple_pull_u8(buf);
		if ((tt & 0x3F) == 0x3F) {
			return;
		}

		delay = net_buf_simple_pull_u8(buf);
		break;
	default:
		return;
	}

	*ptr_counter = 0U;
	k_timer_stop(ptr_timer);

	state->last_tid = tid;
	state->last_src_addr = ctx->addr;
	state->last_dst_addr = ctx->recv_dst;
	state->last_msg_timestamp = now;
	state->target_level = level;
	state->transition->tt = tt;
	state->transition->delay = delay;
	state->transition->type = UNKNOWN_TT;

	if (state->target_level != state->level) {
		level_tt_values(state);
	} else {
		gen_level_get(model, ctx, buf);
		gen_level_publish(model);
		return;
	}

	/* For Instantaneous Transition */
	if (state->transition->counter == 0U) {
		state->level = state->target_level;
	}

	state->transition->just_started = true;
	gen_level_get(model, ctx, buf);
	gen_level_publish(model);

	if (bt_mesh_model_elem(model)->addr == elements[0].addr) {
		/* Root element */
		state->transition->type = LEVEL_TT;
		level_lightness_handler(state);
	} else if (bt_mesh_model_elem(model)->addr == elements[1].addr) {
		/* Secondary element */
		state->transition->type = LEVEL_TEMP_TT;
		level_temp_handler(state);
	}
}

static void gen_delta_set_unack(struct bt_mesh_model *model,
				struct bt_mesh_msg_ctx *ctx,
				struct net_buf_simple *buf)
{
	u8_t tid, tt, delay;
	s32_t tmp32, delta;
	s64_t now;
	struct generic_level_state *state = model->user_data;

	delta = (s32_t) net_buf_simple_pull_le32(buf);
	tid = net_buf_simple_pull_u8(buf);

	now = k_uptime_get();
	if (state->last_tid == tid &&
	    state->last_src_addr == ctx->addr &&
	    state->last_dst_addr == ctx->recv_dst &&
	    (now - state->last_msg_timestamp <= K_SECONDS(6))) {

		if (state->last_delta == delta) {
			return;
		}
		tmp32 = state->last_level + delta;

	} else {
		state->last_level = state->level;
		tmp32 = state->level + delta;
	}

	switch (buf->len) {
	case 0x00:	/* No optional fields are available */
		tt = default_tt;
		delay = 0U;
		break;
	case 0x02:	/* Optional fields are available */
		tt = net_buf_simple_pull_u8(buf);
		if ((tt & 0x3F) == 0x3F) {
			return;
		}

		delay = net_buf_simple_pull_u8(buf);
		break;
	default:
		return;
	}

	*ptr_counter = 0U;
	k_timer_stop(ptr_timer);

	state->last_tid = tid;
	state->last_src_addr = ctx->addr;
	state->last_dst_addr = ctx->recv_dst;
	state->last_msg_timestamp = now;
	state->last_delta = delta;

	if (tmp32 < INT16_MIN) {
		tmp32 = INT16_MIN;
	} else if (tmp32 > INT16_MAX) {
		tmp32 = INT16_MAX;
	}

	state->target_level = tmp32;
	state->transition->tt = tt;
	state->transition->delay = delay;
	state->transition->type = UNKNOWN_TT;

	if (state->target_level != state->level) {
		level_tt_values(state);
	} else {
		gen_level_publish(model);
		return;
	}

	/* For Instantaneous Transition */
	if (state->transition->counter == 0U) {
		state->level = state->target_level;
	}

	state->transition->just_started = true;
	gen_level_publish(model);

	if (bt_mesh_model_elem(model)->addr == elements[0].addr) {
		/* Root element */
		state->transition->type = LEVEL_TT_DELTA;
		level_lightness_handler(state);
	} else if (bt_mesh_model_elem(model)->addr == elements[1].addr) {
		/* Secondary element */
		state->transition->type = LEVEL_TEMP_TT_DELTA;
		level_temp_handler(state);
	}
}

static void gen_delta_set(struct bt_mesh_model *model,
			  struct bt_mesh_msg_ctx *ctx,
			  struct net_buf_simple *buf)
{
	u8_t tid, tt, delay;
	s32_t tmp32, delta;
	s64_t now;
	struct generic_level_state *state = model->user_data;

	delta = (s32_t) net_buf_simple_pull_le32(buf);
	tid = net_buf_simple_pull_u8(buf);

	now = k_uptime_get();
	if (state->last_tid == tid &&
	    state->last_src_addr == ctx->addr &&
	    state->last_dst_addr == ctx->recv_dst &&
	    (now - state->last_msg_timestamp <= K_SECONDS(6))) {

		if (state->last_delta == delta) {
			gen_level_get(model, ctx, buf);
			return;
		}
		tmp32 = state->last_level + delta;

	} else {
		state->last_level = state->level;
		tmp32 = state->level + delta;
	}

	switch (buf->len) {
	case 0x00:	/* No optional fields are available */
		tt = default_tt;
		delay = 0U;
		break;
	case 0x02:	/* Optional fields are available */
		tt = net_buf_simple_pull_u8(buf);
		if ((tt & 0x3F) == 0x3F) {
			return;
		}

		delay = net_buf_simple_pull_u8(buf);
		break;
	default:
		return;
	}

	*ptr_counter = 0U;
	k_timer_stop(ptr_timer);

	state->last_tid = tid;
	state->last_src_addr = ctx->addr;
	state->last_dst_addr = ctx->recv_dst;
	state->last_msg_timestamp = now;
	state->last_delta = delta;

	if (tmp32 < INT16_MIN) {
		tmp32 = INT16_MIN;
	} else if (tmp32 > INT16_MAX) {
		tmp32 = INT16_MAX;
	}

	state->target_level = tmp32;
	state->transition->tt = tt;
	state->transition->delay = delay;
	state->transition->type = UNKNOWN_TT;

	if (state->target_level != state->level) {
		level_tt_values(state);
	} else {
		gen_level_get(model, ctx, buf);
		gen_level_publish(model);
		return;
	}

	/* For Instantaneous Transition */
	if (state->transition->counter == 0U) {
		state->level = state->target_level;
	}

	state->transition->just_started = true;
	gen_level_get(model, ctx, buf);
	gen_level_publish(model);

	if (bt_mesh_model_elem(model)->addr == elements[0].addr) {
		/* Root element */
		state->transition->type = LEVEL_TT_DELTA;
		level_lightness_handler(state);
	} else if (bt_mesh_model_elem(model)->addr == elements[1].addr) {
		/* Secondary element */
		state->transition->type = LEVEL_TEMP_TT_DELTA;
		level_temp_handler(state);
	}
}

static void gen_move_set_unack(struct bt_mesh_model *model,
			       struct bt_mesh_msg_ctx *ctx,
			       struct net_buf_simple *buf)
{
	u8_t tid, tt, delay;
	s16_t delta;
	s64_t now;
	struct generic_level_state *state = model->user_data;

	delta = (s16_t) net_buf_simple_pull_le16(buf);
	tid = net_buf_simple_pull_u8(buf);

	now = k_uptime_get();
	if (state->last_tid == tid &&
	    state->last_src_addr == ctx->addr &&
	    state->last_dst_addr == ctx->recv_dst &&
	    (now - state->last_msg_timestamp <= K_SECONDS(6))) {
		return;
	}

	switch (buf->len) {
	case 0x00:	/* No optional fields are available */
		tt = default_tt;
		delay = 0U;
		break;
	case 0x02:	/* Optional fields are available */
		tt = net_buf_simple_pull_u8(buf);
		if ((tt & 0x3F) == 0x3F) {
			return;
		}

		delay = net_buf_simple_pull_u8(buf);
		break;
	default:
		return;
	}

	*ptr_counter = 0U;
	k_timer_stop(ptr_timer);

	state->last_tid = tid;
	state->last_src_addr = ctx->addr;
	state->last_dst_addr = ctx->recv_dst;
	state->last_msg_timestamp = now;
	state->last_delta = delta;

	if (delta < 0) {
		state->target_level = INT16_MIN;
	} else if (delta > 0) {
		state->target_level = INT16_MAX;
	} else if (delta == 0) {
		state->target_level = state->level;
	}

	state->transition->tt = tt;
	state->transition->delay = delay;
	state->transition->type = UNKNOWN_TT;

	if (state->target_level != state->level) {
		level_move_tt_values(state);
	} else {
		gen_level_publish(model);
		return;
	}

	if (state->transition->counter == 0U) {
		return;
	}

	state->transition->just_started = true;

	if (bt_mesh_model_elem(model)->addr == elements[0].addr) {
		/* Root element */
		state->transition->type = LEVEL_TT_MOVE;
		gen_level_publish(model);
		level_lightness_handler(state);
	} else if (bt_mesh_model_elem(model)->addr == elements[1].addr) {
		/* Secondary element */
		state->transition->type = LEVEL_TEMP_TT_MOVE;
		gen_level_publish(model);
		level_temp_handler(state);
	}
}

static void gen_move_set(struct bt_mesh_model *model,
			 struct bt_mesh_msg_ctx *ctx,
			 struct net_buf_simple *buf)
{
	u8_t tid, tt, delay;
	s16_t delta;
	s64_t now;
	struct generic_level_state *state = model->user_data;

	delta = (s16_t) net_buf_simple_pull_le16(buf);
	tid = net_buf_simple_pull_u8(buf);

	now = k_uptime_get();
	if (state->last_tid == tid &&
	    state->last_src_addr == ctx->addr &&
	    state->last_dst_addr == ctx->recv_dst &&
	    (now - state->last_msg_timestamp <= K_SECONDS(6))) {
		gen_level_get(model, ctx, buf);
		return;
	}

	switch (buf->len) {
	case 0x00:	/* No optional fields are available */
		tt = default_tt;
		delay = 0U;
		break;
	case 0x02:	/* Optional fields are available */
		tt = net_buf_simple_pull_u8(buf);
		if ((tt & 0x3F) == 0x3F) {
			return;
		}

		delay = net_buf_simple_pull_u8(buf);
		break;
	default:
		return;
	}

	*ptr_counter = 0U;
	k_timer_stop(ptr_timer);

	state->last_tid = tid;
	state->last_src_addr = ctx->addr;
	state->last_dst_addr = ctx->recv_dst;
	state->last_msg_timestamp = now;
	state->last_delta = delta;

	if (delta < 0) {
		state->target_level = INT16_MIN;
	} else if (delta > 0) {
		state->target_level = INT16_MAX;
	} else if (delta == 0) {
		state->target_level = state->level;
	}

	state->transition->tt = tt;
	state->transition->delay = delay;
	state->transition->type = UNKNOWN_TT;

	if (state->target_level != state->level) {
		level_move_tt_values(state);
	} else {
		gen_level_get(model, ctx, buf);
		gen_level_publish(model);
		return;
	}

	if (state->transition->counter == 0U) {
		return;
	}

	state->transition->just_started = true;

	if (bt_mesh_model_elem(model)->addr == elements[0].addr) {
		/* Root element */
		state->transition->type = LEVEL_TT_MOVE;
		gen_level_get(model, ctx, buf);
		gen_level_publish(model);
		level_lightness_handler(state);
	} else if (bt_mesh_model_elem(model)->addr == elements[1].addr) {
		/* Secondary element */
		state->transition->type = LEVEL_TEMP_TT_MOVE;
		gen_level_get(model, ctx, buf);
		gen_level_publish(model);
		level_temp_handler(state);
	}
}

/* Generic Level Client message handlers */
static void gen_level_status(struct bt_mesh_model *model,
			     struct bt_mesh_msg_ctx *ctx,
			     struct net_buf_simple *buf)
{
	printk("Acknownledgement from GEN_LEVEL_SRV\n");
	printk("Present Level = %04x\n", net_buf_simple_pull_le16(buf));

	if (buf->len == 3U) {
		printk("Target Level = %04x\n", net_buf_simple_pull_le16(buf));
		printk("Remaining Time = %02x\n", net_buf_simple_pull_u8(buf));
	}
}

/* Generic Default Transition Time Server message handlers */
static void gen_def_trans_time_get(struct bt_mesh_model *model,
				   struct bt_mesh_msg_ctx *ctx,
				   struct net_buf_simple *buf)
{
	struct net_buf_simple *msg = NET_BUF_SIMPLE(2 + 1 + 4);
	struct gen_def_trans_time_state *state = model->user_data;

	bt_mesh_model_msg_init(msg, BT_MESH_MODEL_GEN_DEF_TRANS_TIME_STATUS);
	net_buf_simple_add_u8(msg, state->tt);

	if (bt_mesh_model_send(model, ctx, msg, NULL, NULL)) {
		printk("Unable to send GEN_DEF_TT_SRV Status response\n");
	}
}

static void gen_def_trans_time_publish(struct bt_mesh_model *model)
{
	int err;
	struct net_buf_simple *msg = model->pub->msg;
	struct gen_def_trans_time_state *state = model->user_data;

	if (model->pub->addr == BT_MESH_ADDR_UNASSIGNED) {
		return;
	}

	bt_mesh_model_msg_init(msg, BT_MESH_MODEL_GEN_DEF_TRANS_TIME_STATUS);
	net_buf_simple_add_u8(msg, state->tt);

	err = bt_mesh_model_publish(model);
	if (err) {
		printk("bt_mesh_model_publish err %d\n", err);
	}
}

static bool gen_def_trans_time_setunack(struct bt_mesh_model *model,
					struct bt_mesh_msg_ctx *ctx,
					struct net_buf_simple *buf)
{
	u8_t tt;
	struct gen_def_trans_time_state *state = model->user_data;

	tt = net_buf_simple_pull_u8(buf);

	/* Here, Model specification is silent about tid implementation */

	if ((tt & 0x3F) == 0x3F) {
		return false;
	}

	if (state->tt != tt) {
		state->tt = tt;
		default_tt = tt;

		save_on_flash(GEN_DEF_TRANS_TIME_STATE);
	}

	return true;
}

static void gen_def_trans_time_set_unack(struct bt_mesh_model *model,
					 struct bt_mesh_msg_ctx *ctx,
					 struct net_buf_simple *buf)
{
	if (gen_def_trans_time_setunack(model, ctx, buf) == true) {
		gen_def_trans_time_publish(model);
	}
}

static void gen_def_trans_time_set(struct bt_mesh_model *model,
				   struct bt_mesh_msg_ctx *ctx,
				   struct net_buf_simple *buf)
{
	if (gen_def_trans_time_setunack(model, ctx, buf) == true) {
		gen_def_trans_time_get(model, ctx, buf);
		gen_def_trans_time_publish(model);
	}
}

/* Generic Default Transition Time Client message handlers */
static void gen_def_trans_time_status(struct bt_mesh_model *model,
				      struct bt_mesh_msg_ctx *ctx,
				      struct net_buf_simple *buf)
{
	printk("Acknownledgement from GEN_DEF_TT_SRV\n");
	printk("Transition Time = %02x\n", net_buf_simple_pull_u8(buf));
}

/* Generic Power OnOff Server message handlers */
static void gen_onpowerup_get(struct bt_mesh_model *model,
			      struct bt_mesh_msg_ctx *ctx,
			      struct net_buf_simple *buf)
{
	struct net_buf_simple *msg = NET_BUF_SIMPLE(2 + 1 + 4);
	struct generic_onpowerup_state *state = model->user_data;

	bt_mesh_model_msg_init(msg, BT_MESH_MODEL_GEN_ONPOWERUP_STATUS);
	net_buf_simple_add_u8(msg, state->onpowerup);

	if (bt_mesh_model_send(model, ctx, msg, NULL, NULL)) {
		printk("Unable to send GEN_POWER_ONOFF_SRV Status response\n");
	}
}

/* Generic Power OnOff Client message handlers */
static void gen_onpowerup_status(struct bt_mesh_model *model,
				 struct bt_mesh_msg_ctx *ctx,
				 struct net_buf_simple *buf)
{
	printk("Acknownledgement from GEN_POWER_ONOFF_SRV\n");
	printk("OnPowerUp = %02x\n", net_buf_simple_pull_u8(buf));
}

/* Generic Power OnOff Setup Server message handlers */

static void gen_onpowerup_publish(struct bt_mesh_model *model)
{
	int err;
	struct net_buf_simple *msg = model->pub->msg;
	struct generic_onpowerup_state *state = model->user_data;

	if (model->pub->addr == BT_MESH_ADDR_UNASSIGNED) {
		return;
	}

	bt_mesh_model_msg_init(msg, BT_MESH_MODEL_GEN_ONPOWERUP_STATUS);
	net_buf_simple_add_u8(msg, state->onpowerup);

	err = bt_mesh_model_publish(model);
	if (err) {
		printk("bt_mesh_model_publish err %d\n", err);
	}
}

static bool gen_onpowerup_setunack(struct bt_mesh_model *model,
				   struct bt_mesh_msg_ctx *ctx,
				   struct net_buf_simple *buf)
{
	u8_t onpowerup;
	struct generic_onpowerup_state *state = model->user_data;

	onpowerup = net_buf_simple_pull_u8(buf);

	/* Here, Model specification is silent about tid implementation */

	if (onpowerup > STATE_RESTORE) {
		return false;
	}

	if (state->onpowerup != onpowerup) {
		state->onpowerup = onpowerup;

		save_on_flash(GEN_ONPOWERUP_STATE);
	}

	return true;
}

static void gen_onpowerup_set_unack(struct bt_mesh_model *model,
				    struct bt_mesh_msg_ctx *ctx,
				    struct net_buf_simple *buf)
{
	if (gen_onpowerup_setunack(model, ctx, buf) == true) {
		gen_onpowerup_publish(model);
	}
}

static void gen_onpowerup_set(struct bt_mesh_model *model,
			      struct bt_mesh_msg_ctx *ctx,
			      struct net_buf_simple *buf)
{
	if (gen_onpowerup_setunack(model, ctx, buf) == true) {
		gen_onpowerup_get(model, ctx, buf);
		gen_onpowerup_publish(model);
	}
}

/* Vendor Model message handlers*/
static void vnd_get(struct bt_mesh_model *model,
		    struct bt_mesh_msg_ctx *ctx,
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
}

static void vnd_set_unack(struct bt_mesh_model *model,
			  struct bt_mesh_msg_ctx *ctx,
			  struct net_buf_simple *buf)
{
	u8_t tid;
	int current;
	s64_t now;
	struct vendor_state *state = model->user_data;

	current = net_buf_simple_pull_le16(buf);
	tid = net_buf_simple_pull_u8(buf);

	now = k_uptime_get();
	if (state->last_tid == tid &&
	    state->last_src_addr == ctx->addr &&
	    state->last_dst_addr == ctx->recv_dst &&
	    (now - state->last_msg_timestamp <= K_SECONDS(6))) {
		return;
	}

	state->last_tid = tid;
	state->last_src_addr = ctx->addr;
	state->last_dst_addr = ctx->recv_dst;
	state->last_msg_timestamp = now;
	state->current = current;

	printk("Vendor model message = %04x\n", state->current);

	if (state->current == STATE_ON) {
		/* LED2 On */
		gpio_pin_write(led_device[1], DT_ALIAS_LED1_GPIOS_PIN, 0);
	} else {
		/* LED2 Off */
		gpio_pin_write(led_device[1], DT_ALIAS_LED1_GPIOS_PIN, 1);
	}
}

static void vnd_set(struct bt_mesh_model *model,
		    struct bt_mesh_msg_ctx *ctx,
		    struct net_buf_simple *buf)
{
	vnd_set_unack(model, ctx, buf);
	vnd_get(model, ctx, buf);
}

static void vnd_status(struct bt_mesh_model *model,
		       struct bt_mesh_msg_ctx *ctx,
		       struct net_buf_simple *buf)
{
	printk("Acknownledgement from Vendor\n");
	printk("cmd = %04x\n", net_buf_simple_pull_le16(buf));
	printk("response = %08x\n", net_buf_simple_pull_le32(buf));
}

/* Light Lightness Server message handlers */
static void light_lightness_get(struct bt_mesh_model *model,
				struct bt_mesh_msg_ctx *ctx,
				struct net_buf_simple *buf)
{
	struct net_buf_simple *msg = NET_BUF_SIMPLE(2 + 5 + 4);
	struct light_lightness_state *state = model->user_data;

	bt_mesh_model_msg_init(msg, BT_MESH_MODEL_LIGHT_LIGHTNESS_STATUS);
	net_buf_simple_add_le16(msg, state->actual);

	if (state->actual == state->target_actual) {
		goto send;
	}

	if (state->transition->counter) {
		calculate_rt(state->transition);
		net_buf_simple_add_le16(msg, state->target_actual);
		net_buf_simple_add_u8(msg, state->transition->rt);
	}

send:
	if (bt_mesh_model_send(model, ctx, msg, NULL, NULL)) {
		printk("Unable to send LightLightnessAct Status response\n");
	}
}

void light_lightness_publish(struct bt_mesh_model *model)
{
	int err;
	struct net_buf_simple *msg = model->pub->msg;
	struct light_lightness_state *state = model->user_data;

	if (model->pub->addr == BT_MESH_ADDR_UNASSIGNED) {
		return;
	}

	bt_mesh_model_msg_init(msg, BT_MESH_MODEL_LIGHT_LIGHTNESS_STATUS);
	net_buf_simple_add_le16(msg, state->actual);

	if (state->actual == state->target_actual) {
		goto publish;
	}

	if (state->transition->counter) {
		calculate_rt(state->transition);
		net_buf_simple_add_le16(msg, state->target_actual);
		net_buf_simple_add_u8(msg, state->transition->rt);
	}

publish:
	err = bt_mesh_model_publish(model);
	if (err) {
		printk("bt_mesh_model_publish err %d\n", err);
	}
}

static void light_lightness_set_unack(struct bt_mesh_model *model,
				      struct bt_mesh_msg_ctx *ctx,
				      struct net_buf_simple *buf)
{
	u8_t tid, tt, delay;
	u16_t actual;
	s64_t now;
	struct light_lightness_state *state = model->user_data;

	actual = net_buf_simple_pull_le16(buf);
	tid = net_buf_simple_pull_u8(buf);

	now = k_uptime_get();
	if (state->last_tid == tid &&
	    state->last_src_addr == ctx->addr &&
	    state->last_dst_addr == ctx->recv_dst &&
	    (now - state->last_msg_timestamp <= K_SECONDS(6))) {
		return;
	}

	switch (buf->len) {
	case 0x00:	/* No optional fields are available */
		tt = default_tt;
		delay = 0U;
		break;
	case 0x02:	/* Optional fields are available */
		tt = net_buf_simple_pull_u8(buf);
		if ((tt & 0x3F) == 0x3F) {
			return;
		}

		delay = net_buf_simple_pull_u8(buf);
		break;
	default:
		return;
	}

	*ptr_counter = 0U;
	k_timer_stop(ptr_timer);

	state->last_tid = tid;
	state->last_src_addr = ctx->addr;
	state->last_dst_addr = ctx->recv_dst;
	state->last_msg_timestamp = now;

	if (actual > 0 && actual < state->light_range_min) {
		actual = state->light_range_min;
	} else if (actual > state->light_range_max) {
		actual = state->light_range_max;
	}

	state->target_actual = actual;
	state->transition->tt = tt;
	state->transition->delay = delay;
	state->transition->type = LIGHT_ACTUAL_TT;

	if (state->target_actual != state->actual) {
		light_lightness_actual_tt_values(state);
	} else {
		light_lightness_publish(model);
		return;
	}

	/* For Instantaneous Transition */
	if (state->transition->counter == 0U) {
		state->actual = state->target_actual;
	}

	state->transition->just_started = true;
	light_lightness_publish(model);
	light_lightness_actual_handler(state);
}

static void light_lightness_set(struct bt_mesh_model *model,
				struct bt_mesh_msg_ctx *ctx,
				struct net_buf_simple *buf)
{
	u8_t tid, tt, delay;
	u16_t actual;
	s64_t now;
	struct light_lightness_state *state = model->user_data;

	actual = net_buf_simple_pull_le16(buf);
	tid = net_buf_simple_pull_u8(buf);

	now = k_uptime_get();
	if (state->last_tid == tid &&
	    state->last_src_addr == ctx->addr &&
	    state->last_dst_addr == ctx->recv_dst &&
	    (now - state->last_msg_timestamp <= K_SECONDS(6))) {
		light_lightness_get(model, ctx, buf);
		return;
	}

	switch (buf->len) {
	case 0x00:	/* No optional fields are available */
		tt = default_tt;
		delay = 0U;
		break;
	case 0x02:	/* Optional fields are available */
		tt = net_buf_simple_pull_u8(buf);
		if ((tt & 0x3F) == 0x3F) {
			return;
		}

		delay = net_buf_simple_pull_u8(buf);
		break;
	default:
		return;
	}

	*ptr_counter = 0U;
	k_timer_stop(ptr_timer);

	state->last_tid = tid;
	state->last_src_addr = ctx->addr;
	state->last_dst_addr = ctx->recv_dst;
	state->last_msg_timestamp = now;

	if (actual > 0 && actual < state->light_range_min) {
		actual = state->light_range_min;
	} else if (actual > state->light_range_max) {
		actual = state->light_range_max;
	}

	state->target_actual = actual;
	state->transition->tt = tt;
	state->transition->delay = delay;
	state->transition->type = LIGHT_ACTUAL_TT;

	if (state->target_actual != state->actual) {
		light_lightness_actual_tt_values(state);
	} else {
		light_lightness_get(model, ctx, buf);
		light_lightness_publish(model);
		return;
	}

	/* For Instantaneous Transition */
	if (state->transition->counter == 0U) {
		state->actual = state->target_actual;
	}

	state->transition->just_started = true;
	light_lightness_get(model, ctx, buf);
	light_lightness_publish(model);
	light_lightness_actual_handler(state);
}

static void light_lightness_linear_get(struct bt_mesh_model *model,
				       struct bt_mesh_msg_ctx *ctx,
				       struct net_buf_simple *buf)
{
	struct net_buf_simple *msg = NET_BUF_SIMPLE(2 + 5 + 4);
	struct light_lightness_state *state = model->user_data;

	bt_mesh_model_msg_init(msg,
			       BT_MESH_MODEL_LIGHT_LIGHTNESS_LINEAR_STATUS);
	net_buf_simple_add_le16(msg, state->linear);

	if (state->linear == state->target_linear) {
		goto send;
	}

	if (state->transition->counter) {
		calculate_rt(state->transition);
		net_buf_simple_add_le16(msg, state->target_linear);
		net_buf_simple_add_u8(msg, state->transition->rt);
	}

send:
	if (bt_mesh_model_send(model, ctx, msg, NULL, NULL)) {
		printk("Unable to send LightLightnessLin Status response\n");
	}
}

void light_lightness_linear_publish(struct bt_mesh_model *model)
{
	int err;
	struct net_buf_simple *msg = model->pub->msg;
	struct light_lightness_state *state = model->user_data;

	if (model->pub->addr == BT_MESH_ADDR_UNASSIGNED) {
		return;
	}

	bt_mesh_model_msg_init(msg,
			       BT_MESH_MODEL_LIGHT_LIGHTNESS_LINEAR_STATUS);
	net_buf_simple_add_le16(msg, state->linear);

	if (state->linear == state->target_linear) {
		goto publish;
	}

	if (state->transition->counter) {
		calculate_rt(state->transition);
		net_buf_simple_add_le16(msg, state->target_linear);
		net_buf_simple_add_u8(msg, state->transition->rt);
	}

publish:
	err = bt_mesh_model_publish(model);
	if (err) {
		printk("bt_mesh_model_publish err %d\n", err);
	}
}

static void light_lightness_linear_set_unack(struct bt_mesh_model *model,
					     struct bt_mesh_msg_ctx *ctx,
					     struct net_buf_simple *buf)
{
	u8_t tid, tt, delay;
	u16_t linear;
	s64_t now;
	struct light_lightness_state *state = model->user_data;

	linear = net_buf_simple_pull_le16(buf);
	tid = net_buf_simple_pull_u8(buf);

	now = k_uptime_get();
	if (state->last_tid == tid &&
	    state->last_src_addr == ctx->addr &&
	    state->last_dst_addr == ctx->recv_dst &&
	    (now - state->last_msg_timestamp <= K_SECONDS(6))) {
		return;
	}

	switch (buf->len) {
	case 0x00:	/* No optional fields are available */
		tt = default_tt;
		delay = 0U;
		break;
	case 0x02:	/* Optional fields are available */
		tt = net_buf_simple_pull_u8(buf);
		if ((tt & 0x3F) == 0x3F) {
			return;
		}

		delay = net_buf_simple_pull_u8(buf);
		break;
	default:
		return;
	}

	*ptr_counter = 0U;
	k_timer_stop(ptr_timer);

	state->last_tid = tid;
	state->last_src_addr = ctx->addr;
	state->last_dst_addr = ctx->recv_dst;
	state->last_msg_timestamp = now;
	state->target_linear = linear;
	state->transition->tt = tt;
	state->transition->delay = delay;
	state->transition->type = LIGHT_LINEAR_TT;

	if (state->target_linear != state->linear) {
		light_lightness_linear_tt_values(state);
	} else {
		light_lightness_linear_publish(model);
		return;
	}

	/* For Instantaneous Transition */
	if (state->transition->counter == 0U) {
		state->linear = state->target_linear;
	}

	state->transition->just_started = true;
	light_lightness_linear_publish(model);
	light_lightness_linear_handler(state);
}

static void light_lightness_linear_set(struct bt_mesh_model *model,
				       struct bt_mesh_msg_ctx *ctx,
				       struct net_buf_simple *buf)
{
	u8_t tid, tt, delay;
	u16_t linear;
	s64_t now;
	struct light_lightness_state *state = model->user_data;

	linear = net_buf_simple_pull_le16(buf);
	tid = net_buf_simple_pull_u8(buf);

	now = k_uptime_get();
	if (state->last_tid == tid &&
	    state->last_src_addr == ctx->addr &&
	    state->last_dst_addr == ctx->recv_dst &&
	    (now - state->last_msg_timestamp <= K_SECONDS(6))) {
		light_lightness_linear_get(model, ctx, buf);
		return;
	}

	switch (buf->len) {
	case 0x00:	/* No optional fields are available */
		tt = default_tt;
		delay = 0U;
		break;
	case 0x02:	/* Optional fields are available */
		tt = net_buf_simple_pull_u8(buf);
		if ((tt & 0x3F) == 0x3F) {
			return;
		}

		delay = net_buf_simple_pull_u8(buf);
		break;
	default:
		return;
	}

	*ptr_counter = 0U;
	k_timer_stop(ptr_timer);

	state->last_tid = tid;
	state->last_src_addr = ctx->addr;
	state->last_dst_addr = ctx->recv_dst;
	state->last_msg_timestamp = now;
	state->target_linear = linear;
	state->transition->tt = tt;
	state->transition->delay = delay;
	state->transition->type = LIGHT_LINEAR_TT;

	if (state->target_linear != state->linear) {
		light_lightness_linear_tt_values(state);
	} else {
		light_lightness_linear_get(model, ctx, buf);
		light_lightness_linear_publish(model);
		return;
	}

	/* For Instantaneous Transition */
	if (state->transition->counter == 0U) {
		state->linear = state->target_linear;
	}

	state->transition->just_started = true;
	light_lightness_linear_get(model, ctx, buf);
	light_lightness_linear_publish(model);
	light_lightness_linear_handler(state);
}

static void light_lightness_last_get(struct bt_mesh_model *model,
				     struct bt_mesh_msg_ctx *ctx,
				     struct net_buf_simple *buf)
{
	struct net_buf_simple *msg = NET_BUF_SIMPLE(2 + 2 + 4);
	struct light_lightness_state *state = model->user_data;

	bt_mesh_model_msg_init(msg, BT_MESH_MODEL_LIGHT_LIGHTNESS_LAST_STATUS);
	net_buf_simple_add_le16(msg, state->last);

	if (bt_mesh_model_send(model, ctx, msg, NULL, NULL)) {
		printk("Unable to send LightLightnessLast Status response\n");
	}
}

static void light_lightness_default_get(struct bt_mesh_model *model,
					struct bt_mesh_msg_ctx *ctx,
					struct net_buf_simple *buf)
{
	struct net_buf_simple *msg = NET_BUF_SIMPLE(2 + 2 + 4);
	struct light_lightness_state *state = model->user_data;

	bt_mesh_model_msg_init(msg,
			       BT_MESH_MODEL_LIGHT_LIGHTNESS_DEFAULT_STATUS);
	net_buf_simple_add_le16(msg, state->def);

	if (bt_mesh_model_send(model, ctx, msg, NULL, NULL)) {
		printk("Unable to send LightLightnessDef Status response\n");
	}
}

static void light_lightness_range_get(struct bt_mesh_model *model,
				      struct bt_mesh_msg_ctx *ctx,
				      struct net_buf_simple *buf)
{
	struct net_buf_simple *msg = NET_BUF_SIMPLE(2 + 5 + 4);
	struct light_lightness_state *state = model->user_data;

	state->status_code = RANGE_SUCCESSFULLY_UPDATED;

	bt_mesh_model_msg_init(msg, BT_MESH_MODEL_LIGHT_LIGHTNESS_RANGE_STATUS);
	net_buf_simple_add_u8(msg, state->status_code);
	net_buf_simple_add_le16(msg, state->light_range_min);
	net_buf_simple_add_le16(msg, state->light_range_max);

	if (bt_mesh_model_send(model, ctx, msg, NULL, NULL)) {
		printk("Unable to send LightLightnessRange Status response\n");
	}
}

/* Light Lightness Setup Server message handlers */

static void light_lightness_default_publish(struct bt_mesh_model *model)
{
	int err;
	struct net_buf_simple *msg = model->pub->msg;
	struct light_lightness_state *state = model->user_data;

	if (model->pub->addr == BT_MESH_ADDR_UNASSIGNED) {
		return;
	}

	bt_mesh_model_msg_init(msg,
			       BT_MESH_MODEL_LIGHT_LIGHTNESS_DEFAULT_STATUS);
	net_buf_simple_add_le16(msg, state->def);

	err = bt_mesh_model_publish(model);
	if (err) {
		printk("bt_mesh_model_publish err %d\n", err);
	}
}

static void light_lightness_default_set_unack(struct bt_mesh_model *model,
					      struct bt_mesh_msg_ctx *ctx,
					      struct net_buf_simple *buf)
{
	u16_t lightness;
	struct light_lightness_state *state = model->user_data;

	lightness = net_buf_simple_pull_le16(buf);

	/* Here, Model specification is silent about tid implementation */

	if (state->def != lightness) {
		state->def = lightness;
		light_ctl_srv_user_data.lightness_def = state->def;

		save_on_flash(LIGHTNESS_TEMP_DEF_STATE);
	}

	light_lightness_default_publish(model);
}

static void light_lightness_default_set(struct bt_mesh_model *model,
					struct bt_mesh_msg_ctx *ctx,
					struct net_buf_simple *buf)
{
	light_lightness_default_set_unack(model, ctx, buf);
	light_lightness_default_get(model, ctx, buf);
	light_lightness_default_publish(model);
}

static void light_lightness_range_publish(struct bt_mesh_model *model)
{
	int err;
	struct net_buf_simple *msg = model->pub->msg;
	struct light_lightness_state *state = model->user_data;

	if (model->pub->addr == BT_MESH_ADDR_UNASSIGNED) {
		return;
	}

	bt_mesh_model_msg_init(msg, BT_MESH_MODEL_LIGHT_LIGHTNESS_RANGE_STATUS);
	net_buf_simple_add_u8(msg, state->status_code);
	net_buf_simple_add_le16(msg, state->light_range_min);
	net_buf_simple_add_le16(msg, state->light_range_max);

	err = bt_mesh_model_publish(model);
	if (err) {
		printk("bt_mesh_model_publish err %d\n", err);
	}
}

static bool light_lightness_range_setunack(struct bt_mesh_model *model,
					   struct bt_mesh_msg_ctx *ctx,
					   struct net_buf_simple *buf)
{
	u16_t min, max;
	struct light_lightness_state *state = model->user_data;

	min = net_buf_simple_pull_le16(buf);
	max = net_buf_simple_pull_le16(buf);

	/* Here, Model specification is silent about tid implementation */

	if (min == 0U || max == 0U) {
		return false;
	} else {
		if (min <= max) {
			state->status_code = RANGE_SUCCESSFULLY_UPDATED;

			if (state->light_range_min != min ||
			    state->light_range_max != max) {

				state->light_range_min = min;
				state->light_range_max = max;

				save_on_flash(LIGHTNESS_RANGE);
			}
		} else {
			/* The provided value for Range Max cannot be set */
			state->status_code = CANNOT_SET_RANGE_MAX;
			return false;
		}
	}

	return true;
}

static void light_lightness_range_set_unack(struct bt_mesh_model *model,
					    struct bt_mesh_msg_ctx *ctx,
					    struct net_buf_simple *buf)
{
	if (light_lightness_range_setunack(model, ctx, buf) == true) {
		light_lightness_range_publish(model);
	}
}

static void light_lightness_range_set(struct bt_mesh_model *model,
				      struct bt_mesh_msg_ctx *ctx,
				      struct net_buf_simple *buf)
{
	if (light_lightness_range_setunack(model, ctx, buf) == true) {
		light_lightness_range_get(model, ctx, buf);
		light_lightness_range_publish(model);
	}
}

/* Light Lightness Client message handlers */
static void light_lightness_status(struct bt_mesh_model *model,
				   struct bt_mesh_msg_ctx *ctx,
				   struct net_buf_simple *buf)
{
	printk("Acknownledgement from LIGHT_LIGHTNESS_SRV (Actual)\n");
	printk("Present Lightness = %04x\n", net_buf_simple_pull_le16(buf));

	if (buf->len == 3U) {
		printk("Target Lightness = %04x\n",
		       net_buf_simple_pull_le16(buf));
		printk("Remaining Time = %02x\n", net_buf_simple_pull_u8(buf));
	}
}

static void light_lightness_linear_status(struct bt_mesh_model *model,
					  struct bt_mesh_msg_ctx *ctx,
					  struct net_buf_simple *buf)
{
	printk("Acknownledgement from LIGHT_LIGHTNESS_SRV (Linear)\n");
	printk("Present Lightness = %04x\n", net_buf_simple_pull_le16(buf));

	if (buf->len == 3U) {
		printk("Target Lightness = %04x\n",
		       net_buf_simple_pull_le16(buf));
		printk("Remaining Time = %02x\n", net_buf_simple_pull_u8(buf));
	}
}

static void light_lightness_last_status(struct bt_mesh_model *model,
					struct bt_mesh_msg_ctx *ctx,
					struct net_buf_simple *buf)
{
	printk("Acknownledgement from LIGHT_LIGHTNESS_SRV (Last)\n");
	printk("Lightness = %04x\n", net_buf_simple_pull_le16(buf));
}

static void light_lightness_default_status(struct bt_mesh_model *model,
					   struct bt_mesh_msg_ctx *ctx,
					   struct net_buf_simple *buf)
{
	printk("Acknownledgement from LIGHT_LIGHTNESS_SRV (Default)\n");
	printk("Lightness = %04x\n", net_buf_simple_pull_le16(buf));
}

static void light_lightness_range_status(struct bt_mesh_model *model,
					 struct bt_mesh_msg_ctx *ctx,
					 struct net_buf_simple *buf)
{
	printk("Acknownledgement from LIGHT_LIGHTNESS_SRV (Lightness Range)\n");
	printk("Status Code = %02x\n", net_buf_simple_pull_u8(buf));
	printk("Range Min = %04x\n", net_buf_simple_pull_le16(buf));
	printk("Range Max = %04x\n", net_buf_simple_pull_le16(buf));
}

/* Light CTL Server message handlers */
static void light_ctl_get(struct bt_mesh_model *model,
			  struct bt_mesh_msg_ctx *ctx,
			  struct net_buf_simple *buf)
{
	struct net_buf_simple *msg = NET_BUF_SIMPLE(2 + 9 + 4);
	struct light_ctl_state *state = model->user_data;

	bt_mesh_model_msg_init(msg, BT_MESH_MODEL_LIGHT_CTL_STATUS);
	net_buf_simple_add_le16(msg, state->lightness);
	net_buf_simple_add_le16(msg, state->temp);

	if (state->lightness == state->target_lightness &&
	    state->temp == state->target_temp) {
		goto send;
	}

	if (state->transition->counter) {
		calculate_rt(state->transition);
		net_buf_simple_add_le16(msg, state->target_lightness);
		net_buf_simple_add_le16(msg, state->target_temp);
		net_buf_simple_add_u8(msg, state->transition->rt);
	}

send:
	if (bt_mesh_model_send(model, ctx, msg, NULL, NULL)) {
		printk("Unable to send LightCTL Status response\n");
	}
}

void light_ctl_publish(struct bt_mesh_model *model)
{
	int err;
	struct net_buf_simple *msg = model->pub->msg;
	struct light_ctl_state *state = model->user_data;

	if (model->pub->addr == BT_MESH_ADDR_UNASSIGNED) {
		return;
	}

	bt_mesh_model_msg_init(msg, BT_MESH_MODEL_LIGHT_CTL_STATUS);

	/* Here, as per Model specification, status should be
	 * made up of lightness & temperature values only
	 */
	net_buf_simple_add_le16(msg, state->lightness);
	net_buf_simple_add_le16(msg, state->temp);

	if (state->lightness == state->target_lightness &&
	    state->temp == state->target_temp) {
		goto publish;
	}

	if (state->transition->counter) {
		calculate_rt(state->transition);
		net_buf_simple_add_le16(msg, state->target_lightness);
		net_buf_simple_add_le16(msg, state->target_temp);
		net_buf_simple_add_u8(msg, state->transition->rt);
	}

publish:
	err = bt_mesh_model_publish(model);
	if (err) {
		printk("bt_mesh_model_publish err %d\n", err);
	}
}

static void light_ctl_set_unack(struct bt_mesh_model *model,
				struct bt_mesh_msg_ctx *ctx,
				struct net_buf_simple *buf)
{
	u8_t tid, tt, delay;
	s16_t delta_uv;
	u16_t lightness, temp;
	s64_t now;
	struct light_ctl_state *state = model->user_data;

	lightness = net_buf_simple_pull_le16(buf);
	temp = net_buf_simple_pull_le16(buf);
	delta_uv = (s16_t) net_buf_simple_pull_le16(buf);
	tid = net_buf_simple_pull_u8(buf);

	if (temp < TEMP_MIN || temp > TEMP_MAX) {
		return;
	}

	now = k_uptime_get();
	if (state->last_tid == tid &&
	    state->last_src_addr == ctx->addr &&
	    state->last_dst_addr == ctx->recv_dst &&
	    (now - state->last_msg_timestamp <= K_SECONDS(6))) {
		return;
	}

	switch (buf->len) {
	case 0x00:	/* No optional fields are available */
		tt = default_tt;
		delay = 0U;
		break;
	case 0x02:	/* Optional fields are available */
		tt = net_buf_simple_pull_u8(buf);
		if ((tt & 0x3F) == 0x3F) {
			return;
		}

		delay = net_buf_simple_pull_u8(buf);
		break;
	default:
		return;
	}

	*ptr_counter = 0U;
	k_timer_stop(ptr_timer);

	state->last_tid = tid;
	state->last_src_addr = ctx->addr;
	state->last_dst_addr = ctx->recv_dst;
	state->last_msg_timestamp = now;
	state->target_lightness = lightness;

	if (temp < state->temp_range_min) {
		temp = state->temp_range_min;
	} else if (temp > state->temp_range_max) {
		temp = state->temp_range_max;
	}

	state->target_temp = temp;
	state->target_delta_uv = delta_uv;
	state->transition->tt = tt;
	state->transition->delay = delay;
	state->transition->type = LIGHT_CTL_TT;

	if (state->target_lightness != state->lightness ||
	    state->target_temp != state->temp ||
	    state->target_delta_uv != state->delta_uv) {
		light_ctl_tt_values(state);
	} else {
		light_ctl_publish(model);
		return;
	}

	/* For Instantaneous Transition */
	if (state->transition->counter == 0U) {
		state->lightness = state->target_lightness;
		state->temp = state->target_temp;
		state->delta_uv = state->target_delta_uv;
	}

	state->transition->just_started = true;
	light_ctl_publish(model);
	light_ctl_handler(state);
}

static void light_ctl_set(struct bt_mesh_model *model,
			  struct bt_mesh_msg_ctx *ctx,
			  struct net_buf_simple *buf)
{
	u8_t tid, tt, delay;
	s16_t delta_uv;
	u16_t lightness, temp;
	s64_t now;
	struct light_ctl_state *state = model->user_data;

	lightness = net_buf_simple_pull_le16(buf);
	temp = net_buf_simple_pull_le16(buf);
	delta_uv = (s16_t) net_buf_simple_pull_le16(buf);
	tid = net_buf_simple_pull_u8(buf);

	if (temp < TEMP_MIN || temp > TEMP_MAX) {
		return;
	}

	now = k_uptime_get();
	if (state->last_tid == tid &&
	    state->last_src_addr == ctx->addr &&
	    state->last_dst_addr == ctx->recv_dst &&
	    (now - state->last_msg_timestamp <= K_SECONDS(6))) {
		light_ctl_get(model, ctx, buf);
		return;
	}

	switch (buf->len) {
	case 0x00:	/* No optional fields are available */
		tt = default_tt;
		delay = 0U;
		break;
	case 0x02:	/* Optional fields are available */
		tt = net_buf_simple_pull_u8(buf);
		if ((tt & 0x3F) == 0x3F) {
			return;
		}

		delay = net_buf_simple_pull_u8(buf);
		break;
	default:
		return;
	}

	*ptr_counter = 0U;
	k_timer_stop(ptr_timer);

	state->last_tid = tid;
	state->last_src_addr = ctx->addr;
	state->last_dst_addr = ctx->recv_dst;
	state->last_msg_timestamp = now;
	state->target_lightness = lightness;

	if (temp < state->temp_range_min) {
		temp = state->temp_range_min;
	} else if (temp > state->temp_range_max) {
		temp = state->temp_range_max;
	}

	state->target_temp = temp;
	state->target_delta_uv = delta_uv;
	state->transition->tt = tt;
	state->transition->delay = delay;
	state->transition->type = LIGHT_CTL_TT;

	if (state->target_lightness != state->lightness ||
	    state->target_temp != state->temp ||
	    state->target_delta_uv != state->delta_uv) {
		light_ctl_tt_values(state);
	} else {
		light_ctl_get(model, ctx, buf);
		light_ctl_publish(model);
		return;
	}

	/* For Instantaneous Transition */
	if (state->transition->counter == 0U) {
		state->lightness = state->target_lightness;
		state->temp = state->target_temp;
		state->delta_uv = state->target_delta_uv;
	}

	state->transition->just_started = true;
	light_ctl_get(model, ctx, buf);
	light_ctl_publish(model);
	light_ctl_handler(state);
}

static void light_ctl_temp_range_get(struct bt_mesh_model *model,
				     struct bt_mesh_msg_ctx *ctx,
				     struct net_buf_simple *buf)
{
	struct net_buf_simple *msg = NET_BUF_SIMPLE(2 + 5 + 4);
	struct light_ctl_state *state = model->user_data;

	state->status_code = RANGE_SUCCESSFULLY_UPDATED;

	bt_mesh_model_msg_init(msg, BT_MESH_MODEL_LIGHT_CTL_TEMP_RANGE_STATUS);
	net_buf_simple_add_u8(msg, state->status_code);
	net_buf_simple_add_le16(msg, state->temp_range_min);
	net_buf_simple_add_le16(msg, state->temp_range_max);

	if (bt_mesh_model_send(model, ctx, msg, NULL, NULL)) {
		printk("Unable to send LightCTL Temp Range Status response\n");
	}
}

static void light_ctl_default_get(struct bt_mesh_model *model,
				  struct bt_mesh_msg_ctx *ctx,
				  struct net_buf_simple *buf)
{
	struct net_buf_simple *msg = NET_BUF_SIMPLE(2 + 6 + 4);
	struct light_ctl_state *state = model->user_data;

	bt_mesh_model_msg_init(msg, BT_MESH_MODEL_LIGHT_CTL_DEFAULT_STATUS);
	net_buf_simple_add_le16(msg, state->lightness_def);
	net_buf_simple_add_le16(msg, state->temp_def);
	net_buf_simple_add_le16(msg, state->delta_uv_def);

	if (bt_mesh_model_send(model, ctx, msg, NULL, NULL)) {
		printk("Unable to send LightCTL Default Status response\n");
	}
}

/* Light CTL Setup Server message handlers */

static void light_ctl_default_publish(struct bt_mesh_model *model)
{
	int err;
	struct net_buf_simple *msg = model->pub->msg;
	struct light_ctl_state *state = model->user_data;

	if (model->pub->addr == BT_MESH_ADDR_UNASSIGNED) {
		return;
	}

	bt_mesh_model_msg_init(msg, BT_MESH_MODEL_LIGHT_CTL_DEFAULT_STATUS);
	net_buf_simple_add_le16(msg, state->lightness_def);
	net_buf_simple_add_le16(msg, state->temp_def);
	net_buf_simple_add_le16(msg, state->delta_uv_def);

	err = bt_mesh_model_publish(model);
	if (err) {
		printk("bt_mesh_model_publish err %d\n", err);
	}
}

static bool light_ctl_default_setunack(struct bt_mesh_model *model,
				       struct bt_mesh_msg_ctx *ctx,
				       struct net_buf_simple *buf)
{
	u16_t lightness, temp;
	s16_t delta_uv;
	struct light_ctl_state *state = model->user_data;

	lightness = net_buf_simple_pull_le16(buf);
	temp = net_buf_simple_pull_le16(buf);
	delta_uv = (s16_t) net_buf_simple_pull_le16(buf);

	/* Here, Model specification is silent about tid implementation */

	if (temp < TEMP_MIN || temp > TEMP_MAX) {
		return false;
	}

	if (temp < state->temp_range_min) {
		temp = state->temp_range_min;
	} else if (temp > state->temp_range_max) {
		temp = state->temp_range_max;
	}

	if (state->lightness_def != lightness || state->temp_def != temp ||
	    state->delta_uv_def != delta_uv) {
		state->lightness_def = lightness;
		state->temp_def = temp;
		state->delta_uv_def = delta_uv;

		light_lightness_srv_user_data.def = lightness;

		save_on_flash(LIGHTNESS_TEMP_DEF_STATE);
	}

	return true;
}

static void light_ctl_default_set_unack(struct bt_mesh_model *model,
					struct bt_mesh_msg_ctx *ctx,
					struct net_buf_simple *buf)
{
	if (light_ctl_default_setunack(model, ctx, buf) == true) {
		light_ctl_default_publish(model);
	}
}

static void light_ctl_default_set(struct bt_mesh_model *model,
				  struct bt_mesh_msg_ctx *ctx,
				  struct net_buf_simple *buf)
{
	if (light_ctl_default_setunack(model, ctx, buf) == true) {
		light_ctl_default_get(model, ctx, buf);
		light_ctl_default_publish(model);
	}
}

static void light_ctl_temp_range_publish(struct bt_mesh_model *model)
{
	int err;
	struct net_buf_simple *msg = model->pub->msg;
	struct light_ctl_state *state = model->user_data;

	if (model->pub->addr == BT_MESH_ADDR_UNASSIGNED) {
		return;
	}

	bt_mesh_model_msg_init(msg, BT_MESH_MODEL_LIGHT_CTL_TEMP_RANGE_STATUS);
	net_buf_simple_add_u8(msg, state->status_code);
	net_buf_simple_add_le16(msg, state->temp_range_min);
	net_buf_simple_add_le16(msg, state->temp_range_max);

	err = bt_mesh_model_publish(model);
	if (err) {
		printk("bt_mesh_model_publish err %d\n", err);
	}
}

static bool light_ctl_temp_range_setunack(struct bt_mesh_model *model,
					  struct bt_mesh_msg_ctx *ctx,
					  struct net_buf_simple *buf)
{
	u16_t min, max;
	struct light_ctl_state *state = model->user_data;

	min = net_buf_simple_pull_le16(buf);
	max = net_buf_simple_pull_le16(buf);

	/* Here, Model specification is silent about tid implementation */

	/* This is as per 6.1.3.1 in Mesh Model Specification */
	if (min < TEMP_MIN || min > TEMP_MAX ||
	    max < TEMP_MIN || max > TEMP_MAX) {
		return false;
	}

	if (min <= max) {
		state->status_code = RANGE_SUCCESSFULLY_UPDATED;

			if (state->temp_range_min != min ||
			    state->temp_range_max != max) {

				state->temp_range_min = min;
				state->temp_range_max = max;

				save_on_flash(TEMPERATURE_RANGE);
			}
	} else {
		/* The provided value for Range Max cannot be set */
		state->status_code = CANNOT_SET_RANGE_MAX;
		return false;
	}

	return true;
}

static void light_ctl_temp_range_set_unack(struct bt_mesh_model *model,
					   struct bt_mesh_msg_ctx *ctx,
					   struct net_buf_simple *buf)
{
	if (light_ctl_temp_range_setunack(model, ctx, buf) == true) {
		light_ctl_temp_range_publish(model);
	}
}

static void light_ctl_temp_range_set(struct bt_mesh_model *model,
				     struct bt_mesh_msg_ctx *ctx,
				     struct net_buf_simple *buf)
{
	if (light_ctl_temp_range_setunack(model, ctx, buf) == true) {
		light_ctl_temp_range_get(model, ctx, buf);
		light_ctl_temp_range_publish(model);
	}
}

/* Light CTL Client message handlers */
static void light_ctl_status(struct bt_mesh_model *model,
			     struct bt_mesh_msg_ctx *ctx,
			     struct net_buf_simple *buf)
{
	printk("Acknownledgement from LIGHT_CTL_SRV\n");
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
}

static void light_ctl_temp_range_status(struct bt_mesh_model *model,
					struct bt_mesh_msg_ctx *ctx,
					struct net_buf_simple *buf)
{
	printk("Acknownledgement from LIGHT_CTL_SRV (Temperature Range)\n");
	printk("Status Code = %02x\n", net_buf_simple_pull_u8(buf));
	printk("Range Min = %04x\n", net_buf_simple_pull_le16(buf));
	printk("Range Max = %04x\n", net_buf_simple_pull_le16(buf));
}

static void light_ctl_temp_status(struct bt_mesh_model *model,
				  struct bt_mesh_msg_ctx *ctx,
				  struct net_buf_simple *buf)
{
	printk("Acknownledgement from LIGHT_CTL_TEMP_SRV\n");
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
}

static void light_ctl_default_status(struct bt_mesh_model *model,
				     struct bt_mesh_msg_ctx *ctx,
				     struct net_buf_simple *buf)
{
	printk("Acknownledgement from LIGHT_CTL_SRV (Default)\n");
	printk("Lightness = %04x\n", net_buf_simple_pull_le16(buf));
	printk("Temperature = %04x\n", net_buf_simple_pull_le16(buf));
	printk("Delta UV = %04x\n", net_buf_simple_pull_le16(buf));
}

/* Light CTL Temp. Server message handlers */
static void light_ctl_temp_get(struct bt_mesh_model *model,
			       struct bt_mesh_msg_ctx *ctx,
			       struct net_buf_simple *buf)
{
	struct net_buf_simple *msg = NET_BUF_SIMPLE(2 + 9 + 4);
	struct light_ctl_state *state = model->user_data;

	bt_mesh_model_msg_init(msg, BT_MESH_MODEL_LIGHT_CTL_TEMP_STATUS);
	net_buf_simple_add_le16(msg, state->temp);
	net_buf_simple_add_le16(msg, state->delta_uv);

	if (state->temp == state->target_temp &&
	    state->delta_uv == state->target_delta_uv) {
		goto send;
	}

	if (state->transition->counter) {
		calculate_rt(state->transition);
		net_buf_simple_add_le16(msg, state->target_temp);
		net_buf_simple_add_le16(msg, state->target_delta_uv);
		net_buf_simple_add_u8(msg, state->transition->rt);
	}

send:
	if (bt_mesh_model_send(model, ctx, msg, NULL, NULL)) {
		printk("Unable to send LightCTL Temp. Status response\n");
	}
}

void light_ctl_temp_publish(struct bt_mesh_model *model)
{
	int err;
	struct net_buf_simple *msg = model->pub->msg;
	struct light_ctl_state *state = model->user_data;

	if (model->pub->addr == BT_MESH_ADDR_UNASSIGNED) {
		return;
	}

	bt_mesh_model_msg_init(msg, BT_MESH_MODEL_LIGHT_CTL_TEMP_STATUS);
	net_buf_simple_add_le16(msg, state->temp);
	net_buf_simple_add_le16(msg, state->delta_uv);

	if (state->temp == state->target_temp &&
	    state->delta_uv == state->target_delta_uv) {
		goto publish;
	}

	if (state->transition->counter) {
		calculate_rt(state->transition);
		net_buf_simple_add_le16(msg, state->target_temp);
		net_buf_simple_add_le16(msg, state->target_delta_uv);
		net_buf_simple_add_u8(msg, state->transition->rt);
	}

publish:
	err = bt_mesh_model_publish(model);
	if (err) {
		printk("bt_mesh_model_publish err %d\n", err);
	}
}

static void light_ctl_temp_set_unack(struct bt_mesh_model *model,
				     struct bt_mesh_msg_ctx *ctx,
				     struct net_buf_simple *buf)
{
	u8_t tid, tt, delay;
	s16_t delta_uv;
	u16_t temp;
	s64_t now;
	struct light_ctl_state *state = model->user_data;

	temp = net_buf_simple_pull_le16(buf);
	delta_uv = (s16_t) net_buf_simple_pull_le16(buf);
	tid = net_buf_simple_pull_u8(buf);

	if (temp < TEMP_MIN || temp > TEMP_MAX) {
		return;
	}

	now = k_uptime_get();
	if (state->last_tid == tid &&
	    state->last_src_addr == ctx->addr &&
	    state->last_dst_addr == ctx->recv_dst &&
	    (now - state->last_msg_timestamp <= K_SECONDS(6))) {
		return;
	}

	switch (buf->len) {
	case 0x00:	/* No optional fields are available */
		tt = default_tt;
		delay = 0U;
		break;
	case 0x02:	/* Optional fields are available */
		tt = net_buf_simple_pull_u8(buf);
		if ((tt & 0x3F) == 0x3F) {
			return;
		}

		delay = net_buf_simple_pull_u8(buf);
		break;
	default:
		return;
	}

	*ptr_counter = 0U;
	k_timer_stop(ptr_timer);

	state->last_tid = tid;
	state->last_src_addr = ctx->addr;
	state->last_dst_addr = ctx->recv_dst;
	state->last_msg_timestamp = now;

	if (temp < state->temp_range_min) {
		temp = state->temp_range_min;
	} else if (temp > state->temp_range_max) {
		temp = state->temp_range_max;
	}

	state->target_temp = temp;
	state->target_delta_uv = delta_uv;
	state->transition->tt = tt;
	state->transition->delay = delay;
	state->transition->type = LIGHT_CTL_TEMP_TT;

	if (state->target_temp != state->temp ||
	    state->target_delta_uv != state->delta_uv) {
		light_ctl_temp_tt_values(state);
	} else {
		light_ctl_temp_publish(model);
		return;
	}

	/* For Instantaneous Transition */
	if (state->transition->counter == 0U) {
		state->temp = state->target_temp;
		state->delta_uv = state->target_delta_uv;
	}

	state->transition->just_started = true;
	light_ctl_temp_publish(model);
	light_ctl_temp_handler(state);
}

static void light_ctl_temp_set(struct bt_mesh_model *model,
			       struct bt_mesh_msg_ctx *ctx,
			       struct net_buf_simple *buf)
{
	u8_t tid, tt, delay;
	s16_t delta_uv;
	u16_t temp;
	s64_t now;
	struct light_ctl_state *state = model->user_data;

	temp = net_buf_simple_pull_le16(buf);
	delta_uv = (s16_t) net_buf_simple_pull_le16(buf);
	tid = net_buf_simple_pull_u8(buf);

	if (temp < TEMP_MIN || temp > TEMP_MAX) {
		return;
	}

	now = k_uptime_get();
	if (state->last_tid == tid &&
	    state->last_src_addr == ctx->addr &&
	    state->last_dst_addr == ctx->recv_dst &&
	    (now - state->last_msg_timestamp <= K_SECONDS(6))) {
		light_ctl_temp_get(model, ctx, buf);
		return;
	}

	switch (buf->len) {
	case 0x00:	/* No optional fields are available */
		tt = default_tt;
		delay = 0U;
		break;
	case 0x02:	/* Optional fields are available */
		tt = net_buf_simple_pull_u8(buf);
		if ((tt & 0x3F) == 0x3F) {
			return;
		}

		delay = net_buf_simple_pull_u8(buf);
		break;
	default:
		return;
	}

	*ptr_counter = 0U;
	k_timer_stop(ptr_timer);

	state->last_tid = tid;
	state->last_src_addr = ctx->addr;
	state->last_dst_addr = ctx->recv_dst;
	state->last_msg_timestamp = now;

	if (temp < state->temp_range_min) {
		temp = state->temp_range_min;
	} else if (temp > state->temp_range_max) {
		temp = state->temp_range_max;
	}

	state->target_temp = temp;
	state->target_delta_uv = delta_uv;
	state->transition->tt = tt;
	state->transition->delay = delay;
	state->transition->type = LIGHT_CTL_TEMP_TT;

	if (state->target_temp != state->temp ||
	    state->target_delta_uv != state->delta_uv) {
		light_ctl_temp_tt_values(state);
	} else {
		light_ctl_temp_get(model, ctx, buf);
		light_ctl_temp_publish(model);
		return;
	}

	/* For Instantaneous Transition */
	if (state->transition->counter == 0U) {
		state->temp = state->target_temp;
		state->delta_uv = state->target_delta_uv;
	}

	state->transition->just_started = true;
	light_ctl_temp_get(model, ctx, buf);
	light_ctl_temp_publish(model);
	light_ctl_temp_handler(state);
}

/* message handlers (End) */

/* Mapping of message handlers for Generic OnOff Server (0x1000) */
static const struct bt_mesh_model_op gen_onoff_srv_op[] = {
	{ BT_MESH_MODEL_OP_2(0x82, 0x01), 0, gen_onoff_get },
	{ BT_MESH_MODEL_OP_2(0x82, 0x02), 2, gen_onoff_set },
	{ BT_MESH_MODEL_OP_2(0x82, 0x03), 2, gen_onoff_set_unack },
	BT_MESH_MODEL_OP_END,
};

/* Mapping of message handlers for Generic OnOff Client (0x1001) */
static const struct bt_mesh_model_op gen_onoff_cli_op[] = {
	{ BT_MESH_MODEL_OP_2(0x82, 0x04), 1, gen_onoff_status },
	BT_MESH_MODEL_OP_END,
};

/* Mapping of message handlers for Generic Levl Server (0x1002) */
static const struct bt_mesh_model_op gen_level_srv_op[] = {
	{ BT_MESH_MODEL_OP_2(0x82, 0x05), 0, gen_level_get },
	{ BT_MESH_MODEL_OP_2(0x82, 0x06), 3, gen_level_set },
	{ BT_MESH_MODEL_OP_2(0x82, 0x07), 3, gen_level_set_unack },
	{ BT_MESH_MODEL_OP_2(0x82, 0x09), 5, gen_delta_set },
	{ BT_MESH_MODEL_OP_2(0x82, 0x0A), 5, gen_delta_set_unack },
	{ BT_MESH_MODEL_OP_2(0x82, 0x0B), 3, gen_move_set },
	{ BT_MESH_MODEL_OP_2(0x82, 0x0C), 3, gen_move_set_unack },
	BT_MESH_MODEL_OP_END,
};

/* Mapping of message handlers for Generic Level Client (0x1003) */
static const struct bt_mesh_model_op gen_level_cli_op[] = {
	{ BT_MESH_MODEL_OP_2(0x82, 0x08), 2, gen_level_status },
	BT_MESH_MODEL_OP_END,
};

/* Mapping of message handlers for Generic Default TT Server (0x1004) */
static const struct bt_mesh_model_op gen_def_trans_time_srv_op[] = {
	{ BT_MESH_MODEL_OP_2(0x82, 0x0D), 0, gen_def_trans_time_get },
	{ BT_MESH_MODEL_OP_2(0x82, 0x0E), 1, gen_def_trans_time_set },
	{ BT_MESH_MODEL_OP_2(0x82, 0x0F), 1, gen_def_trans_time_set_unack },
	BT_MESH_MODEL_OP_END,
};

/* Mapping of message handlers for Generic Default TT Client (0x1005) */
static const struct bt_mesh_model_op gen_def_trans_time_cli_op[] = {
	{ BT_MESH_MODEL_OP_2(0x82, 0x10), 1, gen_def_trans_time_status },
	BT_MESH_MODEL_OP_END,
};

/* Mapping of message handlers for Generic Power OnOff Server (0x1006) */
static const struct bt_mesh_model_op gen_power_onoff_srv_op[] = {
	{ BT_MESH_MODEL_OP_2(0x82, 0x11), 0, gen_onpowerup_get },
	BT_MESH_MODEL_OP_END,
};

/* Mapping of message handlers for Generic Power OnOff Setup Server (0x1007) */
static const struct bt_mesh_model_op gen_power_onoff_setup_srv_op[] = {
	{ BT_MESH_MODEL_OP_2(0x82, 0x13), 1, gen_onpowerup_set },
	{ BT_MESH_MODEL_OP_2(0x82, 0x14), 1, gen_onpowerup_set_unack },
	BT_MESH_MODEL_OP_END,
};

/* Mapping of message handlers for Generic Power OnOff Client (0x1008) */
static const struct bt_mesh_model_op gen_power_onoff_cli_op[] = {
	{ BT_MESH_MODEL_OP_2(0x82, 0x12), 1, gen_onpowerup_status },
	BT_MESH_MODEL_OP_END,
};

/* Mapping of message handlers for Light Lightness Server (0x1300) */
static const struct bt_mesh_model_op light_lightness_srv_op[] = {
	{ BT_MESH_MODEL_OP_2(0x82, 0x4B), 0, light_lightness_get },
	{ BT_MESH_MODEL_OP_2(0x82, 0x4C), 3, light_lightness_set },
	{ BT_MESH_MODEL_OP_2(0x82, 0x4D), 3, light_lightness_set_unack },
	{ BT_MESH_MODEL_OP_2(0x82, 0x4F), 0, light_lightness_linear_get },
	{ BT_MESH_MODEL_OP_2(0x82, 0x50), 3, light_lightness_linear_set },
	{ BT_MESH_MODEL_OP_2(0x82, 0x51), 3,
	  light_lightness_linear_set_unack },
	{ BT_MESH_MODEL_OP_2(0x82, 0x53), 0, light_lightness_last_get },
	{ BT_MESH_MODEL_OP_2(0x82, 0x55), 0, light_lightness_default_get },
	{ BT_MESH_MODEL_OP_2(0x82, 0x57), 0, light_lightness_range_get },
	BT_MESH_MODEL_OP_END,
};

/* Mapping of message handlers for Light Lightness Setup Server (0x1301) */
static const struct bt_mesh_model_op light_lightness_setup_srv_op[] = {
	{ BT_MESH_MODEL_OP_2(0x82, 0x59), 2, light_lightness_default_set },
	{ BT_MESH_MODEL_OP_2(0x82, 0x5A), 2,
	  light_lightness_default_set_unack },
	{ BT_MESH_MODEL_OP_2(0x82, 0x5B), 4, light_lightness_range_set },
	{ BT_MESH_MODEL_OP_2(0x82, 0x5C), 4, light_lightness_range_set_unack },
	BT_MESH_MODEL_OP_END,
};

/* Mapping of message handlers for Light Lightness Client (0x1302) */
static const struct bt_mesh_model_op light_lightness_cli_op[] = {
	{ BT_MESH_MODEL_OP_2(0x82, 0x4E), 2, light_lightness_status },
	{ BT_MESH_MODEL_OP_2(0x82, 0x52), 2, light_lightness_linear_status },
	{ BT_MESH_MODEL_OP_2(0x82, 0x54), 2, light_lightness_last_status },
	{ BT_MESH_MODEL_OP_2(0x82, 0x56), 2, light_lightness_default_status },
	{ BT_MESH_MODEL_OP_2(0x82, 0x58), 5, light_lightness_range_status },
	BT_MESH_MODEL_OP_END,
};

/* Mapping of message handlers for Light CTL Server (0x1303) */
static const struct bt_mesh_model_op light_ctl_srv_op[] = {
	{ BT_MESH_MODEL_OP_2(0x82, 0x5D), 0, light_ctl_get },
	{ BT_MESH_MODEL_OP_2(0x82, 0x5E), 7, light_ctl_set },
	{ BT_MESH_MODEL_OP_2(0x82, 0x5F), 7, light_ctl_set_unack },
	{ BT_MESH_MODEL_OP_2(0x82, 0x62), 0, light_ctl_temp_range_get },
	{ BT_MESH_MODEL_OP_2(0x82, 0x67), 0, light_ctl_default_get },
	BT_MESH_MODEL_OP_END,
};

/* Mapping of message handlers for Light CTL Setup Server (0x1304) */
static const struct bt_mesh_model_op light_ctl_setup_srv_op[] = {
	{ BT_MESH_MODEL_OP_2(0x82, 0x69), 6, light_ctl_default_set },
	{ BT_MESH_MODEL_OP_2(0x82, 0x6A), 6, light_ctl_default_set_unack },
	{ BT_MESH_MODEL_OP_2(0x82, 0x6B), 4, light_ctl_temp_range_set },
	{ BT_MESH_MODEL_OP_2(0x82, 0x6C), 4, light_ctl_temp_range_set_unack },
	BT_MESH_MODEL_OP_END,
};

/* Mapping of message handlers for Light CTL Client (0x1305) */
static const struct bt_mesh_model_op light_ctl_cli_op[] = {
	{ BT_MESH_MODEL_OP_2(0x82, 0x60), 4, light_ctl_status },
	{ BT_MESH_MODEL_OP_2(0x82, 0x63), 5, light_ctl_temp_range_status },
	{ BT_MESH_MODEL_OP_2(0x82, 0x66), 4, light_ctl_temp_status },
	{ BT_MESH_MODEL_OP_2(0x82, 0x68), 6, light_ctl_default_status },
	BT_MESH_MODEL_OP_END,
};

/* Mapping of message handlers for Light CTL Temp. Server (0x1306) */
static const struct bt_mesh_model_op light_ctl_temp_srv_op[] = {
	{ BT_MESH_MODEL_OP_2(0x82, 0x61), 0, light_ctl_temp_get },
	{ BT_MESH_MODEL_OP_2(0x82, 0x64), 5, light_ctl_temp_set },
	{ BT_MESH_MODEL_OP_2(0x82, 0x65), 5, light_ctl_temp_set_unack },
	BT_MESH_MODEL_OP_END,
};

/* Mapping of message handlers for Vendor (0x4321) */
static const struct bt_mesh_model_op vnd_ops[] = {
	{ BT_MESH_MODEL_OP_3(0x01, CID_ZEPHYR), 0, vnd_get },
	{ BT_MESH_MODEL_OP_3(0x02, CID_ZEPHYR), 3, vnd_set },
	{ BT_MESH_MODEL_OP_3(0x03, CID_ZEPHYR), 3, vnd_set_unack },
	{ BT_MESH_MODEL_OP_3(0x04, CID_ZEPHYR), 6, vnd_status },
	BT_MESH_MODEL_OP_END,
};

struct bt_mesh_model root_models[] = {
	BT_MESH_MODEL_CFG_SRV(&cfg_srv),
	BT_MESH_MODEL_HEALTH_SRV(&health_srv, &health_pub),

	BT_MESH_MODEL(BT_MESH_MODEL_ID_GEN_ONOFF_SRV,
		      gen_onoff_srv_op, &gen_onoff_srv_pub_root,
		      &gen_onoff_srv_root_user_data),
	BT_MESH_MODEL(BT_MESH_MODEL_ID_GEN_ONOFF_CLI,
		      gen_onoff_cli_op, &gen_onoff_cli_pub_root,
		      NULL),

	BT_MESH_MODEL(BT_MESH_MODEL_ID_GEN_LEVEL_SRV,
		      gen_level_srv_op, &gen_level_srv_pub_root,
		      &gen_level_srv_root_user_data),
	BT_MESH_MODEL(BT_MESH_MODEL_ID_GEN_LEVEL_CLI,
		      gen_level_cli_op, &gen_level_cli_pub_root,
		      NULL),

	BT_MESH_MODEL(BT_MESH_MODEL_ID_GEN_DEF_TRANS_TIME_SRV,
		      gen_def_trans_time_srv_op,
		      &gen_def_trans_time_srv_pub,
		      &gen_def_trans_time_srv_user_data),
	BT_MESH_MODEL(BT_MESH_MODEL_ID_GEN_DEF_TRANS_TIME_CLI,
		      gen_def_trans_time_cli_op,
		      &gen_def_trans_time_cli_pub,
		      NULL),

	BT_MESH_MODEL(BT_MESH_MODEL_ID_GEN_POWER_ONOFF_SRV,
		      gen_power_onoff_srv_op, &gen_power_onoff_srv_pub,
		      &gen_power_onoff_srv_user_data),
	BT_MESH_MODEL(BT_MESH_MODEL_ID_GEN_POWER_ONOFF_SETUP_SRV,
		      gen_power_onoff_setup_srv_op,
		      &gen_power_onoff_srv_pub,
		      &gen_power_onoff_srv_user_data),
	BT_MESH_MODEL(BT_MESH_MODEL_ID_GEN_POWER_ONOFF_CLI,
		      gen_power_onoff_cli_op, &gen_power_onoff_cli_pub,
		      NULL),

	BT_MESH_MODEL(BT_MESH_MODEL_ID_LIGHT_LIGHTNESS_SRV,
		      light_lightness_srv_op, &light_lightness_srv_pub,
		      &light_lightness_srv_user_data),
	BT_MESH_MODEL(BT_MESH_MODEL_ID_LIGHT_LIGHTNESS_SETUP_SRV,
		      light_lightness_setup_srv_op,
		      &light_lightness_srv_pub,
		      &light_lightness_srv_user_data),
	BT_MESH_MODEL(BT_MESH_MODEL_ID_LIGHT_LIGHTNESS_CLI,
		      light_lightness_cli_op, &light_lightness_cli_pub,
		      NULL),

	BT_MESH_MODEL(BT_MESH_MODEL_ID_LIGHT_CTL_SRV,
		      light_ctl_srv_op, &light_ctl_srv_pub,
		      &light_ctl_srv_user_data),
	BT_MESH_MODEL(BT_MESH_MODEL_ID_LIGHT_CTL_SETUP_SRV,
		      light_ctl_setup_srv_op, &light_ctl_srv_pub,
		      &light_ctl_srv_user_data),
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
		      gen_level_srv_op, &gen_level_srv_pub_s0,
		      &gen_level_srv_s0_user_data),
	BT_MESH_MODEL(BT_MESH_MODEL_ID_GEN_LEVEL_CLI,
		      gen_level_cli_op, &gen_level_cli_pub_s0,
		      NULL),

	BT_MESH_MODEL(BT_MESH_MODEL_ID_LIGHT_CTL_TEMP_SRV,
		      light_ctl_temp_srv_op, &light_ctl_srv_pub,
		      &light_ctl_srv_user_data),
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

