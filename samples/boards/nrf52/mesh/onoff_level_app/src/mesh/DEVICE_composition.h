#ifndef _DEVICE_COMPOSITION_H
#define _DEVICE_COMPOSITION_H

struct server_state {
	int current;
	int previous;
	int model_instance;
	int last_tid;
	uint8_t tid_discard;
};

extern struct server_state gen_onoff_srv_user_data_root;
extern struct server_state gen_level_srv_user_data_root;

extern struct bt_mesh_model root_models[];

extern const struct bt_mesh_comp comp;

#endif
