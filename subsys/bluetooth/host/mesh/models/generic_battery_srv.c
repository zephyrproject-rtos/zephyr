/* Bluetooth Mesh Models */

/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <net/buf.h>

#include <bluetooth/mesh.h>

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_MESH_DEBUG_MODEL)
#include "common/log.h"

#define MSG_LEN 10

struct generic_battery_state
	generic_battery_state_user_data[CONFIG_BT_MESH_MODEL_BATTERY_SRV_CNT];

static __noinit u8_t
	net_buf_data_generic_battery[CONFIG_BT_MESH_MODEL_BATTERY_SRV_CNT][MSG_LEN];

static struct net_buf_simple
	net_buf_generic_battery[CONFIG_BT_MESH_MODEL_BATTERY_SRV_CNT];

struct bt_mesh_model_pub
	generic_battery_pub[CONFIG_BT_MESH_MODEL_BATTERY_SRV_CNT] = { [0 ...
	(CONFIG_BT_MESH_MODEL_BATTERY_SRV_CNT - 1)] = { .msg = NULL } };

static inline void buff_add_le24(struct net_buf_simple *buf, u32_t *value)
{
	net_buf_simple_add_mem(buf, value, 3);
}

/* Generic Battery Server message handlers */
static void gen_battery_get(struct bt_mesh_model *model,
			    struct bt_mesh_msg_ctx *ctx,
			    struct net_buf_simple *buf)
{
	struct net_buf_simple *msg = NET_BUF_SIMPLE(2 + 8 + 4);
	struct generic_battery_state *state = model->user_data;

	bt_mesh_model_msg_init(msg, BT_MESH_MODEL_OP_2(0x82, 0x24));
	net_buf_simple_add_u8(msg, state->battery_level);
	buff_add_le24(msg, &state->time_to_discharge);
	buff_add_le24(msg, &state->time_to_charge);
	net_buf_simple_add_u8(msg, state->flags);

	if (bt_mesh_model_send(model, ctx, msg, NULL, NULL)) {
		BT_ERR("Unable to send Battery Status response");
	}
}

/* Mapping of message handlers for Generic battery messages (0x100c) */
const struct bt_mesh_model_op bt_mesh_model_gen_battery_srv_op[] = {
	{ BT_MESH_MODEL_OP_2(0x82, 0x23), 0, gen_battery_get},
	BT_MESH_MODEL_OP_END,
};

int bt_mesh_model_gen_battery_srv_state_update(u8_t id, u8_t battery_level,
					       u32_t time_to_discharge,
					       u32_t time_to_charge, u8_t flags)
{
	struct generic_battery_state *gbs;

	if (id > (CONFIG_BT_MESH_MODEL_BATTERY_SRV_CNT - 1)) {
		BT_ERR("Invalid battery model id");

		return -EINVAL;
	}

	if (time_to_discharge > 0x0FFF || time_to_charge > 0x0FFF) {
		BT_ERR("Invalid battery time parameter");

		return -EINVAL;
	}

	gbs = &generic_battery_state_user_data[id];

	gbs->battery_level = battery_level;
	gbs->time_to_discharge = time_to_discharge;
	gbs->time_to_charge = time_to_charge;
	gbs->flags = flags;

	return 0;
}

int bt_mesh_model_gen_battery_srv_init(u8_t id)
{
	if (generic_battery_pub[id].msg) {
		BT_WARN("Battery model id=%d, already initialized", id);
		return -EEXIST;
	}

	generic_battery_pub[id].msg = &net_buf_generic_battery[id];
	generic_battery_pub[id].msg->data = net_buf_data_generic_battery[id];
	generic_battery_pub[id].msg->len = 0;
	generic_battery_pub[id].msg->size = MSG_LEN;
	generic_battery_pub[id].msg->__buf = net_buf_data_generic_battery[id];

	BT_DBG("Battery model id=%d, registered succesfully", id);

	return 0;
}
