#ifndef _DOER_H
#define _DOER_H

extern int8_t  gen_onoff_srv_state, last_gen_onoff_srv_state;
extern int16_t gen_level_srv_state, last_gen_level_srv_state;

void doer(struct bt_mesh_model *, struct bt_mesh_msg_ctx *, struct net_buf_simple *, uint16_t);

#endif
