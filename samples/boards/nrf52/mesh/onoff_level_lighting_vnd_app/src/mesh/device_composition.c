/* Bluetooth: Mesh Generic OnOff, Generic Level, Lighting & Vendor Models
 *
 * Copyright (c) 2018 Vikrant More
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <board.h>
#include <gpio.h>

#include "common.h"
#include "ble_mesh.h"
#include "device_composition.h"
#include "state_binding.h"

static struct bt_mesh_cfg_srv cfg_srv = {
	.relay = BT_MESH_RELAY_DISABLED,
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

	/* 3 transmissions with 20ms interval */
	.net_transmit = BT_MESH_TRANSMIT(2, 20),
	.relay_retransmit = BT_MESH_TRANSMIT(2, 20),
};

static struct bt_mesh_health_srv health_srv = {
};

BT_MESH_HEALTH_PUB_DEFINE(health_pub, 0);

/* Definitions of models publication context (Start) */
BT_MESH_MODEL_PUB_DEFINE(gen_onoff_srv_pub_root, NULL, 2 + 3);
BT_MESH_MODEL_PUB_DEFINE(gen_onoff_cli_pub_root, NULL, 2 + 4);

BT_MESH_MODEL_PUB_DEFINE(gen_level_srv_pub_root, NULL, 2 + 5);
BT_MESH_MODEL_PUB_DEFINE(gen_level_cli_pub_root, NULL, 2 + 7);

BT_MESH_MODEL_PUB_DEFINE(gen_power_onoff_srv_pub, NULL, 2 + 1);
BT_MESH_MODEL_PUB_DEFINE(gen_power_onoff_cli_pub, NULL, 2 + 1);

BT_MESH_MODEL_PUB_DEFINE(light_lightness_srv_pub, NULL, 2 + 5);
BT_MESH_MODEL_PUB_DEFINE(light_lightness_cli_pub, NULL, 2 + 5);

BT_MESH_MODEL_PUB_DEFINE(light_ctl_srv_pub, NULL, 2 + 9);
BT_MESH_MODEL_PUB_DEFINE(light_ctl_cli_pub, NULL, 2 + 9);

BT_MESH_MODEL_PUB_DEFINE(vnd_pub, NULL, 3 + 6);

BT_MESH_MODEL_PUB_DEFINE(gen_onoff_srv_pub_s0, NULL, 2 + 3);
BT_MESH_MODEL_PUB_DEFINE(gen_onoff_cli_pub_s0, NULL, 2 + 4);

BT_MESH_MODEL_PUB_DEFINE(gen_level_srv_pub_s0, NULL, 2 + 5);
BT_MESH_MODEL_PUB_DEFINE(gen_level_cli_pub_s0, NULL, 2 + 7);
/* Definitions of models publication context (End) */

/* Definitions of models user data (Start) */
struct generic_onoff_state gen_onoff_srv_root_user_data;

struct generic_level_state gen_level_srv_root_user_data;

struct generic_onpowerup_state gen_power_onoff_srv_user_data;

struct light_lightness_state light_lightness_srv_user_data;

struct light_ctl_state light_ctl_srv_user_data;

struct vendor_state vnd_user_data;

struct generic_onoff_state gen_onoff_srv_s0_user_data;

struct generic_level_state gen_level_srv_s0_user_data;
/* Definitions of models user data (End) */

static struct bt_mesh_elem elements[];

/* message handlers (Start) */

/* Generic OnOff Server message handlers */
static void gen_onoff_get(struct bt_mesh_model *model,
			  struct bt_mesh_msg_ctx *ctx,
			  struct net_buf_simple *buf)
{
	struct net_buf_simple *msg = NET_BUF_SIMPLE(2 + 1 + 4);
	struct generic_onoff_state *state = model->user_data;

	bt_mesh_model_msg_init(msg, BT_MESH_MODEL_OP_GEN_ONOFF_STATUS);

	net_buf_simple_add_u8(msg, state->onoff);

	if (bt_mesh_model_send(model, ctx, msg, NULL, NULL)) {
		printk("Unable to send ONOFF_SRV Status response\n");
	}
}

static bool gen_onoff_setunack(struct bt_mesh_model *model,
			    struct bt_mesh_msg_ctx *ctx,
			    struct net_buf_simple *buf)
{
	u8_t tid, onoff;
	s64_t now;
	struct net_buf_simple *msg = model->pub->msg;
	struct generic_onoff_state *state = model->user_data;

	onoff = net_buf_simple_pull_u8(buf);
	tid = net_buf_simple_pull_u8(buf);

	if (onoff > STATE_ON) {
		return false;
	}

	now = k_uptime_get();
	if (state->last_tid == tid && state->last_tx_addr == ctx->addr &&
	    (now - state->last_msg_timestamp <= K_SECONDS(6))) {
		return true;
	}

	state->last_tid = tid;
	state->last_tx_addr = ctx->addr;
	state->last_msg_timestamp = now;
	state->onoff = onoff;

	if (bt_mesh_model_elem(model)->addr == elements[0].addr) {
		/* Root element */
		state_binding(ONOFF, ONOFF_TEMP);
		update_light_state();
	} else if (bt_mesh_model_elem(model)->addr == elements[1].addr) {
		/* Secondary element */
		printk("Hello World\n");
	}

	if (model->pub->addr != BT_MESH_ADDR_UNASSIGNED) {
		int err;

		bt_mesh_model_msg_init(msg,
				       BT_MESH_MODEL_OP_GEN_ONOFF_STATUS);
		net_buf_simple_add_u8(msg, state->onoff);

		err = bt_mesh_model_publish(model);
		if (err) {
			printk("bt_mesh_model_publish err %d\n", err);
		}
	}

	return true;
}

static void gen_onoff_set_unack(struct bt_mesh_model *model,
				struct bt_mesh_msg_ctx *ctx,
				struct net_buf_simple *buf)
{
	gen_onoff_setunack(model, ctx, buf);
}

static void gen_onoff_set(struct bt_mesh_model *model,
			  struct bt_mesh_msg_ctx *ctx,
			  struct net_buf_simple *buf)
{
	if (gen_onoff_setunack(model, ctx, buf) == true) {
		gen_onoff_get(model, ctx, buf);
	}
}

/* Generic OnOff Client message handlers */
static void gen_onoff_status(struct bt_mesh_model *model,
			     struct bt_mesh_msg_ctx *ctx,
			     struct net_buf_simple *buf)
{
	printk("Acknownledgement from GEN_ONOFF_SRV = %02x\n",
	       net_buf_simple_pull_u8(buf));
}

/* Generic Level Server message handlers */
static void gen_level_get(struct bt_mesh_model *model,
			  struct bt_mesh_msg_ctx *ctx,
			  struct net_buf_simple *buf)
{
	struct net_buf_simple *msg = NET_BUF_SIMPLE(2 + 2 + 4);
	struct generic_level_state *state = model->user_data;

	bt_mesh_model_msg_init(msg, BT_MESH_MODEL_OP_GEN_LEVEL_STATUS);

	net_buf_simple_add_le16(msg, state->level);

	if (bt_mesh_model_send(model, ctx, msg, NULL, NULL)) {
		printk("Unable to send LEVEL_SRV Status response\n");
	}
}

static void gen_level_set_unack(struct bt_mesh_model *model,
				struct bt_mesh_msg_ctx *ctx,
				struct net_buf_simple *buf)
{
	u8_t tid;
	s16_t level;
	s64_t now;
	struct net_buf_simple *msg = model->pub->msg;
	struct generic_level_state *state = model->user_data;

	level = (s16_t) net_buf_simple_pull_le16(buf);
	tid = net_buf_simple_pull_u8(buf);

	now = k_uptime_get();
	if (state->last_tid == tid && state->last_tx_addr == ctx->addr &&
	    (now - state->last_msg_timestamp <= K_SECONDS(6))) {
		return;
	}

	state->last_tid = tid;
	state->last_tx_addr = ctx->addr;
	state->last_msg_timestamp = now;
	state->level = level;

	if (bt_mesh_model_elem(model)->addr == elements[0].addr) {
		/* Root element */
		state_binding(LEVEL, IGNORE_TEMP);
		update_light_state();
	} else if (bt_mesh_model_elem(model)->addr == elements[1].addr) {
		/* Secondary element */
		state_binding(IGNORE, LEVEL_TEMP);
		update_light_state();
	}

	if (model->pub->addr != BT_MESH_ADDR_UNASSIGNED) {
		int err;

		bt_mesh_model_msg_init(msg,
				       BT_MESH_MODEL_OP_GEN_LEVEL_STATUS);

		net_buf_simple_add_le16(msg, state->level);

		err = bt_mesh_model_publish(model);
		if (err) {
			printk("bt_mesh_model_publish err %d\n", err);
		}
	}
}

static void gen_level_set(struct bt_mesh_model *model,
			  struct bt_mesh_msg_ctx *ctx,
			  struct net_buf_simple *buf)
{
	gen_level_set_unack(model, ctx, buf);
	gen_level_get(model, ctx, buf);
}

static void gen_delta_set_unack(struct bt_mesh_model *model,
				struct bt_mesh_msg_ctx *ctx,
				struct net_buf_simple *buf)
{
	u8_t tid;
	s32_t tmp32, delta;
	s64_t now;
	struct net_buf_simple *msg = model->pub->msg;
	struct generic_level_state *state = model->user_data;

	delta = (s32_t) net_buf_simple_pull_le32(buf);
	tid = net_buf_simple_pull_u8(buf);

	now = k_uptime_get();
	if (state->last_tid == tid && state->last_tx_addr == ctx->addr) {

		if (now - state->last_msg_timestamp <= K_SECONDS(6)) {
			if (state->last_delta == delta) {
				return;
			}
			tmp32 = state->last_level + delta;
		} else {
			return;
		}

	} else {
		state->last_level = state->level;
		tmp32 = state->level + delta;
	}

	state->last_delta = delta;
	state->last_tid = tid;
	state->last_tx_addr = ctx->addr;
	state->last_msg_timestamp = now;

	if (tmp32 < -32768) {
		tmp32 = -32768;
	} else if (tmp32 > 32767) {
		tmp32 = 32767;
	}

	state->level = tmp32;

	printk("Level -> %d\n", state->level);

	if (bt_mesh_model_elem(model)->addr == elements[0].addr) {
		/* Root element */
		state_binding(DELTA_LEVEL, IGNORE_TEMP);
		update_light_state();
	} else if (bt_mesh_model_elem(model)->addr == elements[1].addr) {
		/* Secondary element */
		state_binding(IGNORE, LEVEL_TEMP);
		update_light_state();
	}

	if (model->pub->addr != BT_MESH_ADDR_UNASSIGNED) {
		int err;

		bt_mesh_model_msg_init(msg,
				       BT_MESH_MODEL_OP_GEN_LEVEL_STATUS);

		net_buf_simple_add_le16(msg, state->level);

		err = bt_mesh_model_publish(model);
		if (err) {
			printk("bt_mesh_model_publish err %d\n", err);
		}
	}
}

static void gen_delta_set(struct bt_mesh_model *model,
			  struct bt_mesh_msg_ctx *ctx,
			  struct net_buf_simple *buf)
{
	gen_delta_set_unack(model, ctx, buf);
	gen_level_get(model, ctx, buf);
}

static void gen_move_set_unack(struct bt_mesh_model *model,
			       struct bt_mesh_msg_ctx *ctx,
			       struct net_buf_simple *buf)
{
}

static void gen_move_set(struct bt_mesh_model *model,
			 struct bt_mesh_msg_ctx *ctx,
			 struct net_buf_simple *buf)
{
	gen_move_set_unack(model, ctx, buf);
	gen_level_get(model, ctx, buf);
}

/* Generic Level Client message handlers */
static void gen_level_status(struct bt_mesh_model *model,
			     struct bt_mesh_msg_ctx *ctx,
			     struct net_buf_simple *buf)
{
	printk("Acknownledgement from GEN_LEVEL_SRV = %04x\n",
	       net_buf_simple_pull_le16(buf));
}

/* Generic Power OnOff Server message handlers */
static void gen_onpowerup_get(struct bt_mesh_model *model,
			      struct bt_mesh_msg_ctx *ctx,
			      struct net_buf_simple *buf)
{
	struct net_buf_simple *msg = NET_BUF_SIMPLE(2 + 1 + 4);
	struct generic_onpowerup_state *state = model->user_data;

	bt_mesh_model_msg_init(msg, BT_MESH_MODEL_OP_2(0x82, 0x12));

	net_buf_simple_add_u8(msg, state->onpowerup);

	if (bt_mesh_model_send(model, ctx, msg, NULL, NULL)) {
		printk("Unable to send POWERONOFF_SRV Status response\n");
	}
}

/* Generic Power OnOff Client message handlers */
static void gen_onpowerup_status(struct bt_mesh_model *model,
				 struct bt_mesh_msg_ctx *ctx,
				 struct net_buf_simple *buf)
{
	printk("Acknownledgement from GEN_POWER_ONOFF_SRV = %02x\n",
	       net_buf_simple_pull_u8(buf));
}

/* Generic Power OnOff Setup Server message handlers */
static bool gen_onpowerup_setunack(struct bt_mesh_model *model,
				struct bt_mesh_msg_ctx *ctx,
				struct net_buf_simple *buf)
{
	u8_t onpowerup;
	struct net_buf_simple *msg = model->pub->msg;
	struct generic_onpowerup_state *state = model->user_data;

	onpowerup = net_buf_simple_pull_u8(buf);

	/* Here, Model specification is silent about tid implementation */

	if (onpowerup > STATE_RESTORE) {
		return false;
	}
	state->onpowerup = onpowerup;

	/* Do some work here to save value of state->onpowerup on SoC flash */

	if (model->pub->addr != BT_MESH_ADDR_UNASSIGNED) {
		int err;

		bt_mesh_model_msg_init(msg,
				       BT_MESH_MODEL_OP_2(0x82, 0x12));
		net_buf_simple_add_u8(msg, state->onpowerup);

		err = bt_mesh_model_publish(model);
		if (err) {
			printk("bt_mesh_model_publish err %d\n", err);
		}
	}

	return true;
}

static void gen_onpowerup_set_unack(struct bt_mesh_model *model,
				    struct bt_mesh_msg_ctx *ctx,
				    struct net_buf_simple *buf)
{
	gen_onpowerup_setunack(model, ctx, buf);
}

static void gen_onpowerup_set(struct bt_mesh_model *model,
			      struct bt_mesh_msg_ctx *ctx,
			      struct net_buf_simple *buf)
{
	if (gen_onpowerup_setunack(model, ctx, buf) == true) {
		gen_onpowerup_get(model, ctx, buf);
	}
}

/* Vendor Model message handlers*/
static void vnd_get(struct bt_mesh_model *model,
		    struct bt_mesh_msg_ctx *ctx,
		    struct net_buf_simple *buf)
{
	struct net_buf_simple *msg = NET_BUF_SIMPLE(3 + 6 + 4);
	struct vendor_state *state = model->user_data;

	bt_mesh_model_msg_init(msg, BT_MESH_MODEL_OP_3(0x03, CID_ZEPHYR));

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
	if (state->last_tid == tid && state->last_tx_addr == ctx->addr &&
	    (now - state->last_msg_timestamp <= 6000)) {
		return;
	}

	state->last_tid = tid;
	state->last_tx_addr = ctx->addr;
	state->last_msg_timestamp = now;
	state->current = current;

	/* This is dummy response for demo purpose */
	state->response = 0xA578FEB3;

	printk("Vendor model message = %04x\n", state->current);

	if (state->current == STATE_ON) {
		/* LED2 On */
		gpio_pin_write(led_device[1], LED1_GPIO_PIN, 0);
	} else {
		/* LED2 Off */
		gpio_pin_write(led_device[1], LED1_GPIO_PIN, 1);
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
	printk("Acknownledgement from Vendor (cmd) = %04x",
	       net_buf_simple_pull_le16(buf));

	printk("  (response) = %08x\n", net_buf_simple_pull_le32(buf));
}

/* Light Lightness Server message handlers */
static void light_lightness_get(struct bt_mesh_model *model,
				struct bt_mesh_msg_ctx *ctx,
				struct net_buf_simple *buf)
{
	struct net_buf_simple *msg = NET_BUF_SIMPLE(2 + 2 + 4);
	struct light_lightness_state *state = model->user_data;

	bt_mesh_model_msg_init(msg, BT_MESH_MODEL_OP_2(0x82, 0x4E));

	net_buf_simple_add_le16(msg, state->actual);

	if (bt_mesh_model_send(model, ctx, msg, NULL, NULL)) {
		printk("Unable to send LightLightnessAct Status response\n");
	}
}

static void light_lightness_set_unack(struct bt_mesh_model *model,
				      struct bt_mesh_msg_ctx *ctx,
				      struct net_buf_simple *buf)
{
	u8_t tid;
	u16_t actual;
	s64_t now;
	struct net_buf_simple *msg = model->pub->msg;
	struct light_lightness_state *state = model->user_data;

	actual = net_buf_simple_pull_le16(buf);
	tid = net_buf_simple_pull_u8(buf);

	now = k_uptime_get();
	if (state->last_tid == tid && state->last_tx_addr == ctx->addr &&
	    (now - state->last_msg_timestamp <= K_SECONDS(6))) {
		return;
	}

	state->last_tid = tid;
	state->last_tx_addr = ctx->addr;
	state->last_msg_timestamp = now;

	if (actual > 0 && actual < state->light_range_min) {
		actual = state->light_range_min;
	} else if (actual > state->light_range_max) {
		actual = state->light_range_max;
	}

	state->actual = actual;

	state_binding(ACTUAL, IGNORE_TEMP);
	update_light_state();

	if (model->pub->addr != BT_MESH_ADDR_UNASSIGNED) {
		int err;

		bt_mesh_model_msg_init(msg,
				       BT_MESH_MODEL_OP_2(0x82, 0x4E));

		net_buf_simple_add_le16(msg, state->actual);

		err = bt_mesh_model_publish(model);
		if (err) {
			printk("bt_mesh_model_publish err %d\n", err);
		}
	}
}

static void light_lightness_set(struct bt_mesh_model *model,
				struct bt_mesh_msg_ctx *ctx,
				struct net_buf_simple *buf)
{
	light_lightness_set_unack(model, ctx, buf);
	light_lightness_get(model, ctx, buf);
}

static void light_lightness_linear_get(struct bt_mesh_model *model,
				       struct bt_mesh_msg_ctx *ctx,
				       struct net_buf_simple *buf)
{
	struct net_buf_simple *msg = NET_BUF_SIMPLE(2 + 2 + 4);
	struct light_lightness_state *state = model->user_data;

	bt_mesh_model_msg_init(msg, BT_MESH_MODEL_OP_2(0x82, 0x52));

	net_buf_simple_add_le16(msg, state->linear);

	if (bt_mesh_model_send(model, ctx, msg, NULL, NULL)) {
		printk("Unable to send LightLightnessLin Status response\n");
	}
}

static void light_lightness_linear_set_unack(struct bt_mesh_model *model,
					     struct bt_mesh_msg_ctx *ctx,
					     struct net_buf_simple *buf)
{
	u8_t tid;
	u16_t linear;
	s64_t now;
	struct net_buf_simple *msg = model->pub->msg;
	struct light_lightness_state *state = model->user_data;

	linear = net_buf_simple_pull_le16(buf);
	tid = net_buf_simple_pull_u8(buf);

	now = k_uptime_get();
	if (state->last_tid == tid && state->last_tx_addr == ctx->addr &&
	    (now - state->last_msg_timestamp <= K_SECONDS(6))) {
		return;
	}

	state->last_tid = tid;
	state->last_tx_addr = ctx->addr;
	state->last_msg_timestamp = now;
	state->linear = linear;

	state_binding(LINEAR, IGNORE_TEMP);
	update_light_state();

	if (model->pub->addr != BT_MESH_ADDR_UNASSIGNED) {
		int err;

		bt_mesh_model_msg_init(msg,
				       BT_MESH_MODEL_OP_2(0x82, 0x52));

		net_buf_simple_add_le16(msg, state->linear);

		err = bt_mesh_model_publish(model);
		if (err) {
			printk("bt_mesh_model_publish err %d\n", err);
		}
	}
}

static void light_lightness_linear_set(struct bt_mesh_model *model,
				       struct bt_mesh_msg_ctx *ctx,
				       struct net_buf_simple *buf)
{
	light_lightness_linear_set_unack(model, ctx, buf);
	light_lightness_linear_get(model, ctx, buf);
}

static void light_lightness_last_get(struct bt_mesh_model *model,
				     struct bt_mesh_msg_ctx *ctx,
				     struct net_buf_simple *buf)
{
	struct net_buf_simple *msg = NET_BUF_SIMPLE(2 + 2 + 4);
	struct light_lightness_state *state = model->user_data;

	bt_mesh_model_msg_init(msg, BT_MESH_MODEL_OP_2(0x82, 0x54));

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

	bt_mesh_model_msg_init(msg, BT_MESH_MODEL_OP_2(0x82, 0x56));

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

	bt_mesh_model_msg_init(msg, BT_MESH_MODEL_OP_2(0x82, 0x58));

	net_buf_simple_add_u8(msg, state->status_code);
	net_buf_simple_add_le16(msg, state->light_range_min);
	net_buf_simple_add_le16(msg, state->light_range_max);

	if (bt_mesh_model_send(model, ctx, msg, NULL, NULL)) {
		printk("Unable to send LightLightnessRange Status response\n");
	}

	state->status_code = RANGE_SUCCESSFULLY_UPDATED;
}

/* Light Lightness Setup Server message handlers */
static void light_lightness_default_set_unack(struct bt_mesh_model *model,
					      struct bt_mesh_msg_ctx *ctx,
					      struct net_buf_simple *buf)
{
	struct net_buf_simple *msg = model->pub->msg;
	struct light_lightness_state *state = model->user_data;

	state->def = net_buf_simple_pull_le16(buf);

	/* Here, Model specification is silent about tid implementation */

	/* Do some work here to save value of state->def on SoC flash */

	if (model->pub->addr != BT_MESH_ADDR_UNASSIGNED) {
		int err;

		bt_mesh_model_msg_init(msg,
				       BT_MESH_MODEL_OP_2(0x82, 0x56));

		net_buf_simple_add_le16(msg, state->def);

		err = bt_mesh_model_publish(model);
		if (err) {
			printk("bt_mesh_model_publish err %d\n", err);
		}
	}
}

static void light_lightness_default_set(struct bt_mesh_model *model,
					struct bt_mesh_msg_ctx *ctx,
					struct net_buf_simple *buf)
{
	light_lightness_default_set_unack(model, ctx, buf);
	light_lightness_default_get(model, ctx, buf);
}

static bool light_lightness_range_setunack(struct bt_mesh_model *model,
					   struct bt_mesh_msg_ctx *ctx,
					   struct net_buf_simple *buf)
{
	u16_t min, max;
	struct net_buf_simple *msg = model->pub->msg;
	struct light_lightness_state *state = model->user_data;

	min = net_buf_simple_pull_le16(buf);
	max = net_buf_simple_pull_le16(buf);

	/* Here, Model specification is silent about tid implementation */

	if (min == 0 || max == 0) {
		return false;
	} else {
		if (min <= max) {
			state->status_code = RANGE_SUCCESSFULLY_UPDATED;

			state->light_range_min = min;
			state->light_range_max = max;

			/* Do some work here to save values of
			 * state->light_range_min &
			 * state->light_range_max
			 * on SoC flash
			 */
		} else {
			/* The provided value for Range Max cannot be set */
			state->status_code = CANNOT_SET_RANGE_MAX;
			return false;
		}
	}

	if (model->pub->addr != BT_MESH_ADDR_UNASSIGNED) {
		int err;

		bt_mesh_model_msg_init(msg, BT_MESH_MODEL_OP_2(0x82, 0x58));
		net_buf_simple_add_u8(msg, state->status_code);
		net_buf_simple_add_le16(msg, state->light_range_min);
		net_buf_simple_add_le16(msg, state->light_range_max);

		err = bt_mesh_model_publish(model);
		if (err) {
			printk("bt_mesh_model_publish err %d\n", err);
		}
	}

	return true;
}

static void light_lightness_range_set_unack(struct bt_mesh_model *model,
					    struct bt_mesh_msg_ctx *ctx,
					    struct net_buf_simple *buf)
{
	light_lightness_range_setunack(model, ctx, buf);
}

static void light_lightness_range_set(struct bt_mesh_model *model,
				      struct bt_mesh_msg_ctx *ctx,
				      struct net_buf_simple *buf)
{
	if (light_lightness_range_setunack(model, ctx, buf) == true) {
		light_lightness_range_get(model, ctx, buf);
	}
}

/* Light Lightness Client message handlers */
static void light_lightness_status(struct bt_mesh_model *model,
				   struct bt_mesh_msg_ctx *ctx,
				   struct net_buf_simple *buf)
{
	printk("Acknownledgement from LIGHT_LIGHTNESS_SRV (actual) = %04x\n",
	       net_buf_simple_pull_le16(buf));
}

static void light_lightness_linear_status(struct bt_mesh_model *model,
					  struct bt_mesh_msg_ctx *ctx,
					  struct net_buf_simple *buf)
{
	printk("Acknownledgement from LIGHT_LIGHTNESS_SRV (linear) = %04x\n",
	       net_buf_simple_pull_le16(buf));
}

static void light_lightness_last_status(struct bt_mesh_model *model,
					struct bt_mesh_msg_ctx *ctx,
					struct net_buf_simple *buf)
{
	printk("Acknownledgement from LIGHT_LIGHTNESS_SRV (last) = %04x\n",
	       net_buf_simple_pull_le16(buf));
}

static void light_lightness_default_status(struct bt_mesh_model *model,
					   struct bt_mesh_msg_ctx *ctx,
					   struct net_buf_simple *buf)
{
	printk("Acknownledgement from LIGHT_LIGHTNESS_SRV (default) = %04x\n",
	       net_buf_simple_pull_le16(buf));
}

static void light_lightness_range_status(struct bt_mesh_model *model,
					 struct bt_mesh_msg_ctx *ctx,
					 struct net_buf_simple *buf)
{
	printk("Acknownledgement from LIGHT_LIGHTNESS_SRV (status code) = %02x",
	       net_buf_simple_pull_u8(buf));

	printk("  (range_min) = %04x",
	       net_buf_simple_pull_le16(buf));

	printk("  (range_max) = %04x\n",
	       net_buf_simple_pull_le16(buf));
}

/* Light CTL Server message handlers */
static void light_ctl_get(struct bt_mesh_model *model,
			  struct bt_mesh_msg_ctx *ctx,
			  struct net_buf_simple *buf)
{
	struct net_buf_simple *msg = NET_BUF_SIMPLE(2 + 4 + 4);
	struct light_ctl_state *state = model->user_data;

	bt_mesh_model_msg_init(msg, BT_MESH_MODEL_OP_2(0x82, 0x60));

	net_buf_simple_add_le16(msg, state->lightness);
	net_buf_simple_add_le16(msg, state->temp);

	if (bt_mesh_model_send(model, ctx, msg, NULL, NULL)) {
		printk("Unable to send LightCTL Status response\n");
	}
}

static bool light_ctl_setunack(struct bt_mesh_model *model,
			       struct bt_mesh_msg_ctx *ctx,
			       struct net_buf_simple *buf)
{
	u8_t tid;
	s16_t delta_uv;
	u16_t lightness, temp;
	s64_t now;
	struct net_buf_simple *msg = model->pub->msg;
	struct light_ctl_state *state = model->user_data;

	lightness = net_buf_simple_pull_le16(buf);
	temp = net_buf_simple_pull_le16(buf);
	delta_uv = (s16_t) net_buf_simple_pull_le16(buf);
	tid = net_buf_simple_pull_u8(buf);

	if (temp < TEMP_MIN || temp > TEMP_MAX) {
		return false;
	}

	now = k_uptime_get();
	if (state->last_tid == tid && state->last_tx_addr == ctx->addr &&
	    (now - state->last_msg_timestamp <= K_SECONDS(6))) {
		return true;
	}

	state->last_tid = tid;
	state->last_tx_addr = ctx->addr;
	state->last_msg_timestamp = now;

	if (temp < state->temp_range_min) {
		temp = state->temp_range_min;
	} else if (temp > state->temp_range_max) {
		temp = state->temp_range_max;
	}

	state->lightness = lightness;
	state->temp = temp;
	state->delta_uv = delta_uv;

	state_binding(CTL, CTL_TEMP);
	update_light_state();

	if (model->pub->addr != BT_MESH_ADDR_UNASSIGNED) {
		int err;

		bt_mesh_model_msg_init(msg,
				       BT_MESH_MODEL_OP_2(0x82, 0x60));

		/* Here, as per Model specification, status should be
		 * made up of lightness & temperature values only
		 */
		net_buf_simple_add_le16(msg, state->lightness);
		net_buf_simple_add_le16(msg, state->temp);

		err = bt_mesh_model_publish(model);
		if (err) {
			printk("bt_mesh_model_publish err %d\n", err);
		}
	}

	return true;
}

static void light_ctl_set_unack(struct bt_mesh_model *model,
				struct bt_mesh_msg_ctx *ctx,
				struct net_buf_simple *buf)
{
	light_ctl_setunack(model, ctx, buf);
}

static void light_ctl_set(struct bt_mesh_model *model,
			  struct bt_mesh_msg_ctx *ctx,
			  struct net_buf_simple *buf)
{
	if (light_ctl_setunack(model, ctx, buf) == true) {
		light_ctl_get(model, ctx, buf);
	}
}

static void light_ctl_temp_range_get(struct bt_mesh_model *model,
				     struct bt_mesh_msg_ctx *ctx,
				     struct net_buf_simple *buf)
{
	struct net_buf_simple *msg = NET_BUF_SIMPLE(2 + 5 + 4);
	struct light_ctl_state *state = model->user_data;

	bt_mesh_model_msg_init(msg, BT_MESH_MODEL_OP_2(0x82, 0x63));

	net_buf_simple_add_u8(msg, state->status_code);
	net_buf_simple_add_le16(msg, state->temp_range_min);
	net_buf_simple_add_le16(msg, state->temp_range_max);

	if (bt_mesh_model_send(model, ctx, msg, NULL, NULL)) {
		printk("Unable to send LightCTL Temp Range Status response\n");
	}

	state->status_code = RANGE_SUCCESSFULLY_UPDATED;
}

static void light_ctl_default_get(struct bt_mesh_model *model,
				  struct bt_mesh_msg_ctx *ctx,
				  struct net_buf_simple *buf)
{
	struct net_buf_simple *msg = NET_BUF_SIMPLE(2 + 6 + 4);
	struct light_ctl_state *state = model->user_data;

	bt_mesh_model_msg_init(msg, BT_MESH_MODEL_OP_2(0x82, 0x68));

	net_buf_simple_add_le16(msg, state->lightness_def);
	net_buf_simple_add_le16(msg, state->temp_def);
	net_buf_simple_add_le16(msg, state->delta_uv_def);

	if (bt_mesh_model_send(model, ctx, msg, NULL, NULL)) {
		printk("Unable to send LightCTL Default Status response\n");
	}
}

/* Light CTL Setup Server message handlers */
static bool light_ctl_default_setunack(struct bt_mesh_model *model,
				       struct bt_mesh_msg_ctx *ctx,
				       struct net_buf_simple *buf)
{
	u16_t lightness, temp;
	s16_t delta_uv;
	struct net_buf_simple *msg = model->pub->msg;
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

	state->lightness_def = lightness;
	state->temp_def = temp;
	state->delta_uv_def = delta_uv;

	/* Do some work here to save values of
	 * state->lightness_def, state->temp_Def & state->delta_uv_def
	 * on SoC flash
	 */

	if (model->pub->addr != BT_MESH_ADDR_UNASSIGNED) {
		int err;

		bt_mesh_model_msg_init(msg,
				       BT_MESH_MODEL_OP_2(0x82, 0x68));

		net_buf_simple_add_le16(msg, state->lightness_def);
		net_buf_simple_add_le16(msg, state->temp_def);
		net_buf_simple_add_le16(msg, state->delta_uv_def);

		err = bt_mesh_model_publish(model);
		if (err) {
			printk("bt_mesh_model_publish err %d\n", err);
		}
	}

	return true;
}

static void light_ctl_default_set_unack(struct bt_mesh_model *model,
					struct bt_mesh_msg_ctx *ctx,
					struct net_buf_simple *buf)
{
	light_ctl_default_setunack(model, ctx, buf);
}

static void light_ctl_default_set(struct bt_mesh_model *model,
				  struct bt_mesh_msg_ctx *ctx,
				  struct net_buf_simple *buf)
{
	if (light_ctl_default_setunack(model, ctx, buf) == true) {
		light_ctl_default_get(model, ctx, buf);
	}
}

static bool light_ctl_temp_range_setunack(struct bt_mesh_model *model,
					  struct bt_mesh_msg_ctx *ctx,
					  struct net_buf_simple *buf)
{
	u16_t min, max;
	struct net_buf_simple *msg = model->pub->msg;
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
		state->temp_range_min = min;
		state->temp_range_max = max;

		/* Do some work here to save values of
		 * state->temp_range_min & state->temp_range_min
		 * on SoC flash
		 */
	} else {
		/* The provided value for Range Max cannot be set */
		state->status_code = CANNOT_SET_RANGE_MAX;
		return false;
	}

	if (model->pub->addr != BT_MESH_ADDR_UNASSIGNED) {
		int err;

		bt_mesh_model_msg_init(msg, BT_MESH_MODEL_OP_2(0x82, 0x63));
		net_buf_simple_add_u8(msg, state->status_code);
		net_buf_simple_add_le16(msg, state->temp_range_min);
		net_buf_simple_add_le16(msg, state->temp_range_max);

		err = bt_mesh_model_publish(model);
		if (err) {
			printk("bt_mesh_model_publish err %d\n", err);
		}
	}

	return true;
}

static void light_ctl_temp_range_set_unack(struct bt_mesh_model *model,
					   struct bt_mesh_msg_ctx *ctx,
					   struct net_buf_simple *buf)
{
	light_ctl_temp_range_setunack(model, ctx, buf);
}

static void light_ctl_temp_range_set(struct bt_mesh_model *model,
				     struct bt_mesh_msg_ctx *ctx,
				     struct net_buf_simple *buf)
{
	if (light_ctl_temp_range_setunack(model, ctx, buf) == true) {
		light_ctl_temp_range_get(model, ctx, buf);
	}
}

/* Light CTL Client message handlers */
static void light_ctl_status(struct bt_mesh_model *model,
			     struct bt_mesh_msg_ctx *ctx,
			     struct net_buf_simple *buf)
{
	printk("Acknownledgement from LIGHT_CTL_SRV (Lightness) = %04x",
	       net_buf_simple_pull_le16(buf));

	printk("  (Temperature) = %04x\n",
	       net_buf_simple_pull_le16(buf));
}

static void light_ctl_temp_range_status(struct bt_mesh_model *model,
					struct bt_mesh_msg_ctx *ctx,
					struct net_buf_simple *buf)
{
	printk("Acknownledgement from LIGHT_CTL_SRV (status code) = %02x",
	       net_buf_simple_pull_u8(buf));

	printk("  (temp_range_min) = %04x",
	       net_buf_simple_pull_le16(buf));

	printk("  (temp_range_max) = %04x\n",
	       net_buf_simple_pull_le16(buf));
}

static void light_ctl_temp_status(struct bt_mesh_model *model,
				  struct bt_mesh_msg_ctx *ctx,
				  struct net_buf_simple *buf)
{
	printk("Acknownledgement from LIGHT_CTL_TEMP_SRV (Temperature) = %04x",
	       net_buf_simple_pull_le16(buf));

	printk("  (Delta DV) = %04x\n",
	       net_buf_simple_pull_le16(buf));
}

static void light_ctl_default_status(struct bt_mesh_model *model,
				     struct bt_mesh_msg_ctx *ctx,
				     struct net_buf_simple *buf)
{
	printk("Acknownledgement from LIGHT_CTL_SRV (Lightness_def) = %04x",
	       net_buf_simple_pull_le16(buf));

	printk("  (Temperature_def) = %04x",
	       net_buf_simple_pull_le16(buf));

	printk("  (Delta_UV_def) = %04x\n",
	       net_buf_simple_pull_le16(buf));
}

/* Light CTL Temp. Server message handlers */
static void light_ctl_temp_get(struct bt_mesh_model *model,
			       struct bt_mesh_msg_ctx *ctx,
			       struct net_buf_simple *buf)
{
	struct net_buf_simple *msg = NET_BUF_SIMPLE(2 + 4 + 4);
	struct light_ctl_state *state = model->user_data;

	bt_mesh_model_msg_init(msg, BT_MESH_MODEL_OP_2(0x82, 0x66));

	net_buf_simple_add_le16(msg, state->temp);
	net_buf_simple_add_le16(msg, state->delta_uv);

	if (bt_mesh_model_send(model, ctx, msg, NULL, NULL)) {
		printk("Unable to send LightCTL Temp. Status response\n");
	}
}

static bool light_ctl_temp_setunack(struct bt_mesh_model *model,
				    struct bt_mesh_msg_ctx *ctx,
				    struct net_buf_simple *buf)
{
	u8_t tid;
	s16_t delta_uv;
	u16_t temp;
	s64_t now;

	struct net_buf_simple *msg = model->pub->msg;
	struct light_ctl_state *state = model->user_data;

	temp = net_buf_simple_pull_le16(buf);
	delta_uv = (s16_t) net_buf_simple_pull_le16(buf);
	tid = net_buf_simple_pull_u8(buf);

	if (temp < TEMP_MIN || temp > TEMP_MAX) {
		return false;
	}

	now = k_uptime_get();
	if (state->last_tid == tid && state->last_tx_addr == ctx->addr &&
	    (now - state->last_msg_timestamp <= K_SECONDS(6))) {
		return true;
	}

	state->last_tid = tid;
	state->last_tx_addr = ctx->addr;
	state->last_msg_timestamp = now;	

	if (temp < state->temp_range_min) {
		temp = state->temp_range_min;
	} else if (temp > state->temp_range_max) {
		temp = state->temp_range_max;
	}

	state->temp = temp;
	state->delta_uv = delta_uv;

	state_binding(IGNORE, CTL_TEMP);
	update_light_state();

	if (model->pub->addr != BT_MESH_ADDR_UNASSIGNED) {
		int err;

		bt_mesh_model_msg_init(msg,
				       BT_MESH_MODEL_OP_2(0x82, 0x66));

		net_buf_simple_add_le16(msg, state->temp);
		net_buf_simple_add_le16(msg, state->delta_uv);

		err = bt_mesh_model_publish(model);
		if (err) {
			printk("bt_mesh_model_publish err %d\n", err);
		}
	}

	return true;
}

static void light_ctl_temp_set_unack(struct bt_mesh_model *model,
				     struct bt_mesh_msg_ctx *ctx,
				     struct net_buf_simple *buf)
{
	light_ctl_temp_setunack(model, ctx, buf);
}

static void light_ctl_temp_set(struct bt_mesh_model *model,
			       struct bt_mesh_msg_ctx *ctx,
			       struct net_buf_simple *buf)
{
	if (light_ctl_temp_setunack(model, ctx, buf) == true) {
		light_ctl_temp_get(model, ctx, buf);
	}
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

/* Mapping of message handlers for Generic Level Server (0x1003) */
static const struct bt_mesh_model_op gen_level_cli_op[] = {
	{ BT_MESH_MODEL_OP_2(0x82, 0x08), 2, gen_level_status },
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
	{ BT_MESH_MODEL_OP_2(0x82, 0x5E), 5, light_ctl_set },
	{ BT_MESH_MODEL_OP_2(0x82, 0x5F), 5, light_ctl_set_unack },
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
	{ BT_MESH_MODEL_OP_3(0x00, CID_ZEPHYR), 0, vnd_get },
	{ BT_MESH_MODEL_OP_3(0x01, CID_ZEPHYR), 3, vnd_set },
	{ BT_MESH_MODEL_OP_3(0x02, CID_ZEPHYR), 3, vnd_set_unack },
	{ BT_MESH_MODEL_OP_3(0x03, CID_ZEPHYR), 6, vnd_status },
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
	BT_MESH_MODEL(BT_MESH_MODEL_ID_GEN_ONOFF_SRV,
		      gen_onoff_srv_op, &gen_onoff_srv_pub_s0,
		      &gen_onoff_srv_s0_user_data),
	BT_MESH_MODEL(BT_MESH_MODEL_ID_GEN_ONOFF_CLI,
		      gen_onoff_cli_op, &gen_onoff_cli_pub_s0,
		      NULL),

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
