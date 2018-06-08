#include "common.h"

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

BT_MESH_MODEL_PUB_DEFINE(gen_onoff_srv_pub_root, NULL, 2 + 2);
BT_MESH_MODEL_PUB_DEFINE(gen_onoff_cli_pub_root, NULL, 2 + 2);
BT_MESH_MODEL_PUB_DEFINE(gen_level_srv_pub_root, NULL, 2 + 2);
BT_MESH_MODEL_PUB_DEFINE(gen_level_cli_pub_root, NULL, 2 + 2 + 2);

BT_MESH_MODEL_PUB_DEFINE(gen_onoff_srv_pub_s0, NULL, 2 + 2);
BT_MESH_MODEL_PUB_DEFINE(gen_onoff_cli_pub_s0, NULL, 2 + 2);

struct server_state   gen_onoff_srv_user_data_root = {
	.previous = 0xFFFFFFFF, /* it should be anything except {0,1} */
	.model_instance = 1,
};

struct server_state   gen_level_srv_user_data_root = {

	.previous = 0xFFFFFFFF,	/* it should be anything
				 * except {-32768 to 32767}
				*/
	.model_instance = 1,
};

struct server_state   gen_onoff_srv_user_data_s0 = {
	.previous = 0xFFFFFFFF,	/* it should be anything except {0,1} */
	.model_instance = 2,
};

/* ----------------------- message handlers (Start) ----------------------- */

static void gen_onoff_get(struct bt_mesh_model *model,
			  struct bt_mesh_msg_ctx *ctx,
			  struct net_buf_simple *buf)
{
	struct net_buf_simple *msg = NET_BUF_SIMPLE(2 + 1 + 4);
	struct server_state *state = model->user_data;

	bt_mesh_model_msg_init(msg, BT_MESH_MODEL_OP_2(0x82, 0x04));

	net_buf_simple_add_u8(msg, state->current);

	if (bt_mesh_model_send(model, ctx, msg, NULL, NULL)) {
		printk("Unable to send ONOFF_SRV Status response\n");
	}
}

static void gen_onoff_set_unack(struct bt_mesh_model *model,
			  struct bt_mesh_msg_ctx *ctx,
			  struct net_buf_simple *buf)
{
	int err = 0;
	struct net_buf_simple *msg = model->pub->msg;
	struct server_state *state = model->user_data;

	state->current = net_buf_simple_pull_u8(buf);

	if (state->previous == state->current) {
		return;
	}

	if (state->model_instance == 0x01) {
		light_state_current.OnOff = state->current;
		update_light_state();

	} else if (state->model_instance == 0x02) {
		if (state->current == 0x01) {
			nvs_light_state_save();
		}
	}

	if (model->pub->addr != BT_MESH_ADDR_UNASSIGNED) {
		bt_mesh_model_msg_init(msg,
				       BT_MESH_MODEL_OP_2(0x82, 0x04));
		net_buf_simple_add_u8(msg, state->current);

		err = bt_mesh_model_publish(model);

		if (err) {
			printk("bt_mesh_model_publish err %d\n", err);
		}
	}

	state->previous = state->current;
}

static void gen_onoff_set(struct bt_mesh_model *model,
			  struct bt_mesh_msg_ctx *ctx,
			  struct net_buf_simple *buf)
{
	gen_onoff_set_unack(model, ctx, buf);
	gen_onoff_get(model, ctx, buf);
}

static void gen_onoff_cli_status(struct bt_mesh_model *model,
			  struct bt_mesh_msg_ctx *ctx,
			  struct net_buf_simple *buf)
{
	uint8_t tmp8 = net_buf_simple_pull_u8(buf);
	printk("Acknownledgement from GEN_ONOFF_SRV = %u\n", tmp8);
}

static void gen_level_get(struct bt_mesh_model *model,
			  struct bt_mesh_msg_ctx *ctx,
			  struct net_buf_simple *buf)
{
	struct net_buf_simple *msg = NET_BUF_SIMPLE(10);
	struct server_state *state = model->user_data;

	bt_mesh_model_msg_init(msg, BT_MESH_MODEL_OP_2(0x82, 0x08));

	net_buf_simple_add_le16(msg, state->current);

	if (bt_mesh_model_send(model, ctx, msg, NULL, NULL)) {
		printk("Unable to send LEVEL_SRV Status response\n");
	}
}

static void gen_level_set_unack(struct bt_mesh_model *model,
			  struct bt_mesh_msg_ctx *ctx,
			  struct net_buf_simple *buf)
{
	int err;
	struct net_buf_simple *msg = model->pub->msg;
	struct server_state *state = model->user_data;

	state->current = net_buf_simple_pull_le16(buf);

	if (state->previous == state->current) {
		return;
	}

	if (state->model_instance == 0x01) {
		light_state_current.power = state->current;
		update_light_state();
	}

	if (model->pub->addr != BT_MESH_ADDR_UNASSIGNED) {

		bt_mesh_model_msg_init(msg,
				       BT_MESH_MODEL_OP_2(0x82, 0x08));

		net_buf_simple_add_le16(msg, state->current);

		err = bt_mesh_model_publish(model);

		if (err) {
			printk("bt_mesh_model_publish err %d\n", err);
		}
	}

	state->previous = state->current;
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
	int err;
	struct net_buf_simple *msg = model->pub->msg;
	struct server_state *state = model->user_data;

	int32_t tmp32 = state->current + net_buf_simple_pull_le32(buf);
	uint8_t tid = net_buf_simple_pull_u8(buf);

	if (state->last_tid != tid) {
		state->tid_discard = 0;
	} else if (state->last_tid == tid &&
		   state->tid_discard == 1) {
		return;
	}

	state->last_tid = tid;
	state->tid_discard = 1;

	if (tmp32 < -32768) {
		tmp32 = -32768;

	} else if (tmp32 > 32767) {
		tmp32 = 32767;
	}

	state->current = (int16_t)tmp32;

	printk("Level -> %d\n", state->current);

	if (state->model_instance == 0x01) {
		light_state_current.power = state->current;
		update_light_state();
	}

	if (model->pub->addr != BT_MESH_ADDR_UNASSIGNED) {

		bt_mesh_model_msg_init(msg,
				       BT_MESH_MODEL_OP_2(0x82, 0x08));

		net_buf_simple_add_le16(msg, state->current);

		err = bt_mesh_model_publish(model);

		if (err) {
			printk("bt_mesh_model_publish err %d\n", err);
		}
	}

	state->previous = state->current;
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

static void gen_level_cli_status(struct bt_mesh_model *model,
				 struct bt_mesh_msg_ctx *ctx,
				 struct net_buf_simple *buf)
{
	int16_t tmp16 = net_buf_simple_pull_le16(buf);
	printk("Acknownledgement from GEN_LEVEL_SRV = %d\n", tmp16);
}

/* ----------------------- message handlers (End) ----------------------- */

static const struct bt_mesh_model_op gen_onoff_srv_op[] = {
	{ BT_MESH_MODEL_OP_2(0x82, 0x01), 0, gen_onoff_get },
	{ BT_MESH_MODEL_OP_2(0x82, 0x02), 2, gen_onoff_set },
	{ BT_MESH_MODEL_OP_2(0x82, 0x03), 2, gen_onoff_set_unack },
	BT_MESH_MODEL_OP_END,
};

static const struct bt_mesh_model_op gen_onoff_cli_op[] = {
	{ BT_MESH_MODEL_OP_2(0x82, 0x04), 1, gen_onoff_cli_status },
	BT_MESH_MODEL_OP_END,
};

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

static const struct bt_mesh_model_op gen_level_cli_op[] = {
	{ BT_MESH_MODEL_OP_2(0x82, 0x08), 2, gen_level_cli_status },
	BT_MESH_MODEL_OP_END,
};

struct bt_mesh_model root_models[] = {

	BT_MESH_MODEL_CFG_SRV(&cfg_srv),
	BT_MESH_MODEL_HEALTH_SRV(&health_srv, &health_pub),

	BT_MESH_MODEL(BT_MESH_MODEL_ID_GEN_ONOFF_SRV,
		      gen_onoff_srv_op, &gen_onoff_srv_pub_root,
		      &gen_onoff_srv_user_data_root),
	BT_MESH_MODEL(BT_MESH_MODEL_ID_GEN_ONOFF_CLI,
		      gen_onoff_cli_op, &gen_onoff_cli_pub_root,
		      NULL),

	BT_MESH_MODEL(BT_MESH_MODEL_ID_GEN_LEVEL_SRV,
		      gen_level_srv_op, &gen_level_srv_pub_root,
		      &gen_level_srv_user_data_root),
	BT_MESH_MODEL(BT_MESH_MODEL_ID_GEN_LEVEL_CLI,
		      gen_level_cli_op, &gen_level_cli_pub_root,
		      NULL),
};

static struct bt_mesh_model s0_models[] = {

	BT_MESH_MODEL(BT_MESH_MODEL_ID_GEN_ONOFF_SRV,
		      gen_onoff_srv_op, &gen_onoff_srv_pub_s0,
		      &gen_onoff_srv_user_data_s0),
	BT_MESH_MODEL(BT_MESH_MODEL_ID_GEN_ONOFF_CLI,
		      gen_onoff_cli_op, &gen_onoff_cli_pub_s0,
		      NULL),
};

static struct bt_mesh_elem elements[] = {
	BT_MESH_ELEM(0, root_models, BT_MESH_MODEL_NONE),
	BT_MESH_ELEM(0, s0_models, BT_MESH_MODEL_NONE),
};

const struct bt_mesh_comp comp = {
	.cid = CID_INTEL,
	.elem = elements,
	.elem_count = ARRAY_SIZE(elements),
};
