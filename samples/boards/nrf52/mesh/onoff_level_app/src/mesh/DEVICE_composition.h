#ifndef _DEVICE_COMPOSITION_H
#define _DEVICE_COMPOSITION_H

struct server_state
{
	int current;
	int previous;
	int model_instance;
	int last_tid;
};

extern struct server_state   gen_onoff_srv_user_data_root;
extern struct server_state   gen_level_srv_user_data_root;

extern struct bt_mesh_model root_models[];
//extern struct bt_mesh_model s0_models[];

//extern struct bt_mesh_elem elements[];

extern const struct bt_mesh_comp comp;

void gen_onoff_get(struct bt_mesh_model *, struct bt_mesh_msg_ctx *, struct net_buf_simple *);
void gen_onoff_set_unack(struct bt_mesh_model *, struct bt_mesh_msg_ctx *, struct net_buf_simple *);
void gen_onoff_set(struct bt_mesh_model *, struct bt_mesh_msg_ctx *, struct net_buf_simple *);
void gen_onoff_cli_status(struct bt_mesh_model *, struct bt_mesh_msg_ctx *, struct net_buf_simple *);

void gen_level_get(struct bt_mesh_model *, struct bt_mesh_msg_ctx *, struct net_buf_simple *);
void gen_level_set_unack(struct bt_mesh_model *, struct bt_mesh_msg_ctx *, struct net_buf_simple *);
void gen_level_set(struct bt_mesh_model *, struct bt_mesh_msg_ctx *, struct net_buf_simple *);
void gen_level_cli_status(struct bt_mesh_model *, struct bt_mesh_msg_ctx *, struct net_buf_simple *);
void gen_delta_set_unack(struct bt_mesh_model *, struct bt_mesh_msg_ctx *, struct net_buf_simple *);
void gen_delta_set(struct bt_mesh_model *, struct bt_mesh_msg_ctx *, struct net_buf_simple *);
void gen_move_set_unack(struct bt_mesh_model *, struct bt_mesh_msg_ctx *, struct net_buf_simple *);
void gen_move_set(struct bt_mesh_model *, struct bt_mesh_msg_ctx *, struct net_buf_simple *);

#endif
