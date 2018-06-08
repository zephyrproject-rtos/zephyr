#ifndef _DOER_H
#define _DOER_H

void process_message(struct bt_mesh_model *model,
		     struct bt_mesh_msg_ctx *ctx, 
		     struct net_buf_simple *buf, 
		     uint16_t opcode);

#endif
