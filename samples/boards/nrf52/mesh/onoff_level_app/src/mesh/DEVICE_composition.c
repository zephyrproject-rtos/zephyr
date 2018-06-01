#include "common.h"

static struct bt_mesh_cfg_srv cfg_srv = 
{
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

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

static struct bt_mesh_health_srv health_srv = 
{
};

BT_MESH_HEALTH_PUB_DEFINE(health_pub, 0);

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

BT_MESH_MODEL_PUB_DEFINE(gen_onoff_srv_pub_root, NULL, 2 + 2);
BT_MESH_MODEL_PUB_DEFINE(gen_onoff_cli_pub_root, NULL, 2 + 2);
BT_MESH_MODEL_PUB_DEFINE(gen_level_srv_pub_root, NULL, 2 + 2);
BT_MESH_MODEL_PUB_DEFINE(gen_level_cli_pub_root, NULL, 2 + 2 + 2);

BT_MESH_MODEL_PUB_DEFINE(gen_onoff_srv_pub_s0, NULL, 2 + 2);
BT_MESH_MODEL_PUB_DEFINE(gen_onoff_cli_pub_s0, NULL, 2 + 2);

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

struct server_state   gen_onoff_srv_user_data_root = {.data = 13,};
struct server_state   gen_level_srv_user_data_root;
struct server_state   gen_onoff_srv_user_data_s0 = {.data = 14,};

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

static const struct bt_mesh_model_op gen_onoff_srv_op[] = 
{
	{ BT_MESH_MODEL_OP_2(0x82, 0x01), 0, gen_onoff_get },
	{ BT_MESH_MODEL_OP_2(0x82, 0x02), 2, gen_onoff_set },
	{ BT_MESH_MODEL_OP_2(0x82, 0x03), 2, gen_onoff_set_unack },
	BT_MESH_MODEL_OP_END,
};

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

static const struct bt_mesh_model_op gen_onoff_cli_op[] = 
{
	{ BT_MESH_MODEL_OP_2(0x82, 0x04), 1, gen_onoff_cli_status },
	BT_MESH_MODEL_OP_END,
};

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

static const struct bt_mesh_model_op gen_level_srv_op[] = 
{
	{ BT_MESH_MODEL_OP_2(0x82, 0x05), 0, gen_level_get },
	{ BT_MESH_MODEL_OP_2(0x82, 0x06), 3, gen_level_set },
	{ BT_MESH_MODEL_OP_2(0x82, 0x07), 3, gen_level_set_unack },
	{ BT_MESH_MODEL_OP_2(0x82, 0x09), 5, gen_delta_set },
	{ BT_MESH_MODEL_OP_2(0x82, 0x0A), 5, gen_delta_set_unack },
	{ BT_MESH_MODEL_OP_2(0x82, 0x0B), 3, gen_move_set },
	{ BT_MESH_MODEL_OP_2(0x82, 0x0C), 3, gen_move_set_unack },
	BT_MESH_MODEL_OP_END,
};

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

static const struct bt_mesh_model_op gen_level_cli_op[] = 
{
	{ BT_MESH_MODEL_OP_2(0x82, 0x08), 2, gen_level_cli_status },
	BT_MESH_MODEL_OP_END,
};

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

struct bt_mesh_model root_models[] = 
{
	BT_MESH_MODEL_CFG_SRV(&cfg_srv),
	BT_MESH_MODEL_HEALTH_SRV(&health_srv, &health_pub),

	BT_MESH_MODEL(BT_MESH_MODEL_ID_GEN_ONOFF_SRV, gen_onoff_srv_op, &gen_onoff_srv_pub_root, &gen_onoff_srv_user_data_root),
	BT_MESH_MODEL(BT_MESH_MODEL_ID_GEN_ONOFF_CLI, gen_onoff_cli_op, &gen_onoff_cli_pub_root, NULL),

	BT_MESH_MODEL(BT_MESH_MODEL_ID_GEN_LEVEL_SRV, gen_level_srv_op, &gen_level_srv_pub_root, &gen_level_srv_user_data_root),
	BT_MESH_MODEL(BT_MESH_MODEL_ID_GEN_LEVEL_CLI, gen_level_cli_op, &gen_level_cli_pub_root, NULL),
};


struct bt_mesh_model s0_models[] = {
	BT_MESH_MODEL(BT_MESH_MODEL_ID_GEN_ONOFF_SRV, gen_onoff_srv_op, &gen_onoff_srv_pub_s0, &gen_onoff_srv_user_data_s0),
	BT_MESH_MODEL(BT_MESH_MODEL_ID_GEN_ONOFF_CLI, gen_onoff_cli_op, &gen_onoff_cli_pub_s0, NULL),
};

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

struct bt_mesh_elem elements[] = 
{
	BT_MESH_ELEM(0, root_models, BT_MESH_MODEL_NONE),
	BT_MESH_ELEM(0, s0_models, BT_MESH_MODEL_NONE),
};

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

const struct bt_mesh_comp comp = 
{
	.cid = CID_INTEL,
	.elem = elements,
	.elem_count = ARRAY_SIZE(elements),
};


//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void gen_onoff_get(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf)
{
	doer(model, ctx, buf, 0x8201);
}

void gen_onoff_set_unack(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf)
{	
	doer(model, ctx, buf,0x8203);
}

void gen_onoff_set(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf)
{
	gen_onoff_set_unack(model, ctx, buf);
	gen_onoff_get(model, ctx, buf);       
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void gen_onoff_cli_status(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf)
{
	doer(model, ctx, buf, 0x8204);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void gen_level_get(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf)
{
	doer(model, ctx, buf, 0x8205);
}

void gen_level_set_unack(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf)
{
	doer(model, ctx, buf,0x8207);
}

void gen_level_set(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf)
{
	gen_level_set_unack(model, ctx, buf);
	gen_level_get(model, ctx, buf);  
}

void gen_delta_set_unack(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf)
{
	doer(model, ctx, buf,0x820A);
}

void gen_delta_set(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf)
{
	gen_delta_set_unack(model, ctx, buf);
	gen_level_get(model, ctx, buf);
}

void gen_move_set_unack(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf)
{
	doer(model, ctx, buf,0x820C);
}

void gen_move_set(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf)
{
	gen_move_set_unack(model, ctx, buf);
	gen_level_get(model, ctx, buf);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void gen_level_cli_status(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf)
{
	doer(model, ctx, buf, 0x8208);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
