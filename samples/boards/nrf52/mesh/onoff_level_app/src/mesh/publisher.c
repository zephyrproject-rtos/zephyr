#include "common.h"

void publish(struct k_work *work)
{
	static unsigned char tid_onoff = 0;
	static unsigned char tid_level = 0;
	int err = 0;
	
	if(CHECKB(NRF_P0->IN,11) == 0)
	{
		bt_mesh_model_msg_init(root_models[3].pub->msg, BT_MESH_MODEL_OP_2(0x82, 0x03));
		net_buf_simple_add_u8(root_models[3].pub->msg, 0x01);
		net_buf_simple_add_u8(root_models[3].pub->msg, tid_onoff++);
		err = bt_mesh_model_publish(&root_models[3]);
	}
	else if(CHECKB(NRF_P0->IN,12) == 0)
	{
		bt_mesh_model_msg_init(root_models[3].pub->msg, BT_MESH_MODEL_OP_2(0x82, 0x03));
		net_buf_simple_add_u8(root_models[3].pub->msg, 0x00);
		net_buf_simple_add_u8(root_models[3].pub->msg, tid_onoff++);
		err = bt_mesh_model_publish(&root_models[3]);	
	} 
	else if(CHECKB(NRF_P0->IN,24) == 0)
	{
		bt_mesh_model_msg_init(root_models[5].pub->msg,  BT_MESH_MODEL_OP_2(0x82, 0x07));
		net_buf_simple_add_le16(root_models[5].pub->msg, 25);
		net_buf_simple_add_u8(root_models[5].pub->msg, tid_level++);
		err = bt_mesh_model_publish(&root_models[5]);
	}
	else if(CHECKB(NRF_P0->IN,25) == 0)
	{
		bt_mesh_model_msg_init(root_models[5].pub->msg,  BT_MESH_MODEL_OP_2(0x82, 0x07));
		net_buf_simple_add_le16(root_models[5].pub->msg, 100);
		net_buf_simple_add_u8(root_models[5].pub->msg, tid_level++);
		err = bt_mesh_model_publish(&root_models[5]);
	}    
   
	if (err) 
	{
		printk("bt_mesh_model_publish: err: %d\n\r", err);
	}
}
