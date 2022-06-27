/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/mesh.h>

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_MESH_DEBUG_ACCESS)
#define LOG_MODULE_NAME bt_mesh_msg
#include "common/log.h"

void bt_mesh_model_msg_init(struct net_buf_simple *msg, uint32_t opcode)
{
	net_buf_simple_init(msg, 0);

	switch (BT_MESH_MODEL_OP_LEN(opcode)) {
	case 1:
		net_buf_simple_add_u8(msg, opcode);
		break;
	case 2:
		net_buf_simple_add_be16(msg, opcode);
		break;
	case 3:
		net_buf_simple_add_u8(msg, ((opcode >> 16) & 0xff));
		/* Using LE for the CID since the model layer is defined as
		 * little-endian in the mesh spec and using BT_MESH_MODEL_OP_3
		 * will declare the opcode in this way.
		 */
		net_buf_simple_add_le16(msg, opcode & 0xffff);
		break;
	default:
		BT_WARN("Unknown opcode format");
		break;
	}
}

void bt_mesh_msg_ack_ctx_clear(struct bt_mesh_msg_ack_ctx *ack)
{
	ack->op = 0U;
	ack->user_data = NULL;
	ack->dst = BT_MESH_ADDR_UNASSIGNED;
}

int bt_mesh_msg_ack_ctx_prepare(struct bt_mesh_msg_ack_ctx *ack,
				uint32_t op, uint16_t dst, void *user_data)
{
	if (ack->op) {
		BT_WARN("Another synchronous operation pending");
		return -EBUSY;
	}

	ack->op = op;
	ack->user_data = user_data;
	ack->dst = dst;

	return 0;
}

int bt_mesh_msg_ack_ctx_wait(struct bt_mesh_msg_ack_ctx *ack, k_timeout_t timeout)
{
	int err;

	err = k_sem_take(&ack->sem, timeout);
	bt_mesh_msg_ack_ctx_clear(ack);

	if (err == -EAGAIN) {
		return -ETIMEDOUT;
	}

	return err;
}

bool bt_mesh_msg_ack_ctx_match(const struct bt_mesh_msg_ack_ctx *ack,
			       uint32_t op, uint16_t addr, void **user_data)
{
	if (ack->op != op || (BT_MESH_ADDR_IS_UNICAST(ack->dst) && ack->dst != addr)) {
		return false;
	}

	if (user_data != NULL) {
		*user_data = ack->user_data;
	}

	return true;
}
