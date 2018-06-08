#include "common.h"

void process_message(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
		     struct net_buf_simple *buf, uint16_t opcode)
{
	uint8_t tid, tmp8;
	int16_t tmp16;
	s32_t err = 0, tmp32;
	struct net_buf_simple *msg;
	struct server_state *state = model->user_data;

	switch (opcode) {
	case 0x8201:	/* GEN_ONOFF_SRV_GET */

		msg = NET_BUF_SIMPLE(2 + 1 + 4);

		bt_mesh_model_msg_init(msg, BT_MESH_MODEL_OP_2(0x82, 0x04));

		net_buf_simple_add_u8(msg, state->current);

		if (bt_mesh_model_send(model, ctx, msg, NULL, NULL)) {
			printk("Unable to send ONOFF_SRV Status response\n");
		}

		break;

	case 0x8203:	/* GEN_ONOFF_SRV_UNACK */

		msg = model->pub->msg;

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
				printk("bt_mesh_model_publish err %d", err);
			}
		}

		state->previous = state->current;

		break;

	case 0x8204:	/* GEN_ONOFF_SRV_STATUS */

		tmp8 = net_buf_simple_pull_u8(buf);

		printk("Acknownledgement from GEN_ONOFF_SRV = %u\n", tmp8);

		break;

	case 0x8205:	/* GEN_LEVEL_SRV_GET */

		msg = NET_BUF_SIMPLE(10);

		bt_mesh_model_msg_init(msg, BT_MESH_MODEL_OP_2(0x82, 0x08));

		net_buf_simple_add_le16(msg, state->current);

		if (bt_mesh_model_send(model, ctx, msg, NULL, NULL)) {
			printk("Unable to send LEVEL_SRV Status response\n");
		}

		break;

	case 0x8207:	/* GEN_LEVEL_SRV_UNACK */

		msg = model->pub->msg;

		tmp16 = net_buf_simple_pull_le16(buf);

		state->current = tmp16;

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
				printk("bt_mesh_model_publish err %d", err);
			}
		}

		state->previous = state->current;

		break;

	case 0x8208:	/* GEN_LEVEL_SRV_STATUS */

		tmp16 = net_buf_simple_pull_le16(buf);
		printk("Acknownledgement from GEN_LEVEL_SRV = %d\n", tmp16);

		break;

	case 0x820A:	/* GEN_DELTA_SRV_UNACK */

		tmp32 = state->current + net_buf_simple_pull_le32(buf);
		tid = net_buf_simple_pull_u8(buf);

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

		printk("\n\r Level -> %d", state->current);

		if (state->model_instance == 0x01) {
			light_state_current.power = state->current;
			update_light_state();
		}

		state->previous = state->current;

		break;

	case 0x820C:	/* GEN_MOVE_SRV_UNACK */

		break;

	default:

		break;

	}
}
