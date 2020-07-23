/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <sys/printk.h>
#include <sys/byteorder.h>
#include <zephyr.h>

#include <bluetooth/gatt.h>
#include <bluetooth/services/ots.h>
#include "ots_internal.h"

#include <logging/log.h>

LOG_MODULE_DECLARE(bt_ots, CONFIG_BT_OTS_LOG_LEVEL);

#define OACP_PROC_TYPE_SIZE	1
#define OACP_RES_MAX_SIZE	3

static enum bt_gatt_ots_oacp_res_code oacp_read_proc_validate(
	struct bt_conn *conn,
	struct bt_ots *ots,
	struct bt_gatt_ots_oacp_proc *proc)
{
	struct bt_gatt_ots_oacp_read_params *params = &proc->read_params;

	LOG_DBG("Validating Read procedure with offset: 0x%08X and "
		"length: 0x%08X", params->offset, params->len);

	if (!ots->cur_obj) {
		return BT_GATT_OTS_OACP_RES_INV_OBJ;
	}

	if (!BT_OTS_OBJ_GET_PROP_READ(ots->cur_obj->metadata.props)) {
		return BT_GATT_OTS_OACP_RES_NOT_PERMITTED;
	}

	if (!bt_gatt_ots_l2cap_is_open(&ots->l2cap, conn)) {
		return BT_GATT_OTS_OACP_RES_CHAN_UNAVAIL;
	}

	if ((params->offset + (uint64_t) params->len) >
		ots->cur_obj->metadata.size.cur) {
		return BT_GATT_OTS_OACP_RES_INV_PARAM;
	}

	if (ots->cur_obj->state.type != BT_GATT_OTS_OBJECT_IDLE_STATE) {
		return BT_GATT_OTS_OACP_RES_OBJ_LOCKED;
	}

	ots->cur_obj->state.type = BT_GATT_OTS_OBJECT_READ_OP_STATE;
	ots->cur_obj->state.read_op.sent_len = 0;
	memcpy(&ots->cur_obj->state.read_op.oacp_params, &proc->read_params,
		sizeof(ots->cur_obj->state.read_op.oacp_params));

	LOG_DBG("Read procedure is accepted");

	return BT_GATT_OTS_OACP_RES_SUCCESS;
}

static enum bt_gatt_ots_oacp_res_code oacp_proc_validate(
	struct bt_conn *conn,
	struct bt_ots *ots,
	struct bt_gatt_ots_oacp_proc *proc)
{
	switch (proc->type) {
	case BT_GATT_OTS_OACP_PROC_READ:
		return oacp_read_proc_validate(conn, ots, proc);
	case BT_GATT_OTS_OACP_PROC_CREATE:
	case BT_GATT_OTS_OACP_PROC_DELETE:
	case BT_GATT_OTS_OACP_PROC_CHECKSUM_CALC:
	case BT_GATT_OTS_OACP_PROC_EXECUTE:
	case BT_GATT_OTS_OACP_PROC_WRITE:
	case BT_GATT_OTS_OACP_PROC_ABORT:
	default:
		return BT_GATT_OTS_OACP_RES_OPCODE_NOT_SUP;
	}
};

static enum bt_gatt_ots_oacp_res_code oacp_command_decode(
	const uint8_t *buf, uint16_t len,
	struct bt_gatt_ots_oacp_proc *proc)
{
	struct net_buf_simple net_buf;

	net_buf_simple_init_with_data(&net_buf, (void *) buf, len);

	proc->type = net_buf_simple_pull_u8(&net_buf);
	switch (proc->type) {
	case BT_GATT_OTS_OACP_PROC_CREATE:
		proc->create_params.size = net_buf_simple_pull_le32(&net_buf);
		bt_uuid_create(&proc->create_params.type.uuid, net_buf.data,
			       net_buf.len);
		net_buf_simple_pull_mem(&net_buf, net_buf.len);
		break;
	case BT_GATT_OTS_OACP_PROC_DELETE:
		break;
	case BT_GATT_OTS_OACP_PROC_CHECKSUM_CALC:
		proc->cs_calc_params.offset =
			net_buf_simple_pull_le32(&net_buf);
		proc->cs_calc_params.len =
			net_buf_simple_pull_le32(&net_buf);
		break;
	case BT_GATT_OTS_OACP_PROC_EXECUTE:
		break;
	case BT_GATT_OTS_OACP_PROC_READ:
		proc->read_params.offset =
			net_buf_simple_pull_le32(&net_buf);
		proc->read_params.len =
			net_buf_simple_pull_le32(&net_buf);
		return BT_GATT_OTS_OACP_RES_SUCCESS;
	case BT_GATT_OTS_OACP_PROC_WRITE:
		proc->write_params.offset =
			net_buf_simple_pull_le32(&net_buf);
		proc->write_params.len =
			net_buf_simple_pull_le32(&net_buf);
		proc->write_params.mode =
			net_buf_simple_pull_u8(&net_buf);
		break;
	case BT_GATT_OTS_OACP_PROC_ABORT:
	default:
		break;
	}

	LOG_WRN("OACP unsupported procedure type");
	return BT_GATT_OTS_OACP_RES_OPCODE_NOT_SUP;
}

static bool oacp_command_len_verify(struct bt_gatt_ots_oacp_proc *proc,
				    uint16_t len)
{
	uint16_t ref_len = OACP_PROC_TYPE_SIZE;

	switch (proc->type) {
	case BT_GATT_OTS_OACP_PROC_CREATE:
	{
		struct bt_ots_obj_type *type;

		ref_len += sizeof(proc->create_params.size);

		type = &proc->create_params.type;
		if (type->uuid.type == BT_UUID_TYPE_16) {
			ref_len += sizeof(type->uuid_16.val);
		} else if (type->uuid.type == BT_UUID_TYPE_128) {
			ref_len += sizeof(type->uuid_128.val);
		} else {
			return true;
		}
	} break;
	case BT_GATT_OTS_OACP_PROC_DELETE:
		break;
	case BT_GATT_OTS_OACP_PROC_CHECKSUM_CALC:
		ref_len += sizeof(proc->cs_calc_params);
		break;
	case BT_GATT_OTS_OACP_PROC_EXECUTE:
		break;
	case BT_GATT_OTS_OACP_PROC_READ:
		ref_len += sizeof(proc->read_params);
		break;
	case BT_GATT_OTS_OACP_PROC_WRITE:
		ref_len += sizeof(proc->write_params);
		break;
	case BT_GATT_OTS_OACP_PROC_ABORT:
		break;
	default:
		return true;
	}

	return (len == ref_len);
}

static void oacp_read_proc_cb(struct bt_gatt_ots_l2cap *l2cap_ctx,
			      struct bt_conn *conn)
{
	int err;
	uint8_t *obj_chunk;
	uint32_t offset;
	uint32_t len;
	struct bt_ots *ots;
	struct bt_gatt_ots_object_read_op *read_op;

	ots     = CONTAINER_OF(l2cap_ctx, struct bt_ots, l2cap);
	read_op = &ots->cur_obj->state.read_op;
	offset  = read_op->oacp_params.offset + read_op->sent_len;

	if (read_op->sent_len >= read_op->oacp_params.len) {
		LOG_DBG("OACP Read Op over L2CAP is completed");

		if (read_op->sent_len > read_op->oacp_params.len) {
			LOG_WRN("More bytes sent that the client requested");
		}

		ots->cur_obj->state.type = BT_GATT_OTS_OBJECT_IDLE_STATE;
		ots->cb->obj_read(ots, conn, ots->cur_obj->id, NULL, 0,
				  offset);
		return;
	}

	len = read_op->oacp_params.len - read_op->sent_len;
	len = ots->cb->obj_read(ots, conn, ots->cur_obj->id, &obj_chunk, len,
				offset);

	ots->l2cap.tx_done = oacp_read_proc_cb;
	err = bt_gatt_ots_l2cap_send(&ots->l2cap, obj_chunk, len);
	if (err) {
		LOG_ERR("L2CAP CoC error: %d while trying to execute OACP "
			"Read procedure", err);
		ots->cur_obj->state.type = BT_GATT_OTS_OBJECT_IDLE_STATE;
	} else {
		read_op->sent_len += len;
	}
}

static void oacp_read_proc_execute(struct bt_ots *ots,
				   struct bt_conn *conn)
{
	struct bt_gatt_ots_oacp_read_params *params =
		&ots->cur_obj->state.read_op.oacp_params;

	if (!ots->cur_obj) {
		LOG_ERR("Invalid Current Object on OACP Read procedure");
		return;
	}

	LOG_DBG("Executing Read procedure with offset: 0x%08X and "
		"length: 0x%08X", params->offset, params->len);

	if (ots->cb->obj_read) {
		oacp_read_proc_cb(&ots->l2cap, conn);
	} else {
		ots->cur_obj->state.type = BT_GATT_OTS_OBJECT_IDLE_STATE;
		LOG_ERR("OTS Read operation failed: "
			"there is no OTS Read callback");
	}
}

static void oacp_ind_cb(struct bt_conn *conn,
			const struct bt_gatt_attr *attr,
			uint8_t err)
{
	struct bt_ots *ots = (struct bt_ots *) attr->user_data;

	LOG_DBG("Received OACP Indication ACK with status: 0x%04X", err);

	switch (ots->cur_obj->state.type) {
	case BT_GATT_OTS_OBJECT_READ_OP_STATE:
		oacp_read_proc_execute(ots, conn);
		break;
	default:
		LOG_ERR("Unsupported OTS state: %d", ots->cur_obj->state.type);
		break;
	}
}

static int oacp_ind_send(const struct bt_gatt_attr *oacp_attr,
			 enum bt_gatt_ots_oacp_proc_type req_op_code,
			 enum bt_gatt_ots_oacp_res_code oacp_status)
{
	uint8_t oacp_res[OACP_RES_MAX_SIZE];
	uint16_t oacp_res_len = 0;
	struct bt_ots *ots = (struct bt_ots *) oacp_attr->user_data;

	/* Encode OACP Response */
	oacp_res[oacp_res_len++] = BT_GATT_OTS_OACP_PROC_RESP;
	oacp_res[oacp_res_len++] = req_op_code;
	oacp_res[oacp_res_len++] = oacp_status;

	/* Prepare indication parameters */
	memset(&ots->oacp_ind.params, 0, sizeof(ots->oacp_ind.params));
	memcpy(&ots->oacp_ind.attr, oacp_attr, sizeof(ots->oacp_ind.attr));
	ots->oacp_ind.params.attr = &ots->oacp_ind.attr;
	ots->oacp_ind.params.func = oacp_ind_cb;
	ots->oacp_ind.params.data = oacp_res;
	ots->oacp_ind.params.len  = oacp_res_len;

	LOG_DBG("Sending OACP indication");

	return bt_gatt_indicate(NULL, &ots->oacp_ind.params);
}

ssize_t bt_gatt_ots_oacp_write(struct bt_conn *conn,
				   const struct bt_gatt_attr *attr,
				   const void *buf, uint16_t len,
				   uint16_t offset, uint8_t flags)
{
	enum bt_gatt_ots_oacp_res_code oacp_status;
	struct bt_gatt_ots_oacp_proc oacp_proc;
	struct bt_ots *ots = (struct bt_ots *) attr->user_data;

	LOG_DBG("Object Action Control Point GATT Write Operation");

	if (!ots->oacp_ind.is_enabled) {
		LOG_WRN("OACP indications not enabled");
		return BT_GATT_ERR(BT_ATT_ERR_CCC_IMPROPER_CONF);
	}

	if (offset != 0) {
		LOG_ERR("Invalid offset of OACP Write Request");
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	oacp_status = oacp_command_decode(buf, len, &oacp_proc);

	if (!oacp_command_len_verify(&oacp_proc, len)) {
		LOG_ERR("Invalid length of OACP Write Request for 0x%02X "
			"Op Code", oacp_proc.type);
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	if (oacp_status == BT_GATT_OTS_OACP_RES_SUCCESS) {
		oacp_status = oacp_proc_validate(conn, ots, &oacp_proc);
	}

	if (oacp_status != BT_GATT_OTS_OACP_RES_SUCCESS) {
		LOG_WRN("OACP Write error status: 0x%02X", oacp_status);
	}

	oacp_ind_send(attr, oacp_proc.type, oacp_status);
	return len;
}

void bt_gatt_ots_oacp_cfg_changed(const struct bt_gatt_attr *attr,
				      uint16_t value)
{
	struct bt_gatt_ots_indicate *oacp_ind =
	    CONTAINER_OF((struct _bt_gatt_ccc *) attr->user_data,
			 struct bt_gatt_ots_indicate, ccc);

	LOG_DBG("Object Action Control Point CCCD value: 0x%04X", value);

	oacp_ind->is_enabled = false;
	if (value == BT_GATT_CCC_INDICATE) {
		oacp_ind->is_enabled = true;
	}
}
