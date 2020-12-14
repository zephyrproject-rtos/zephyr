/** @file
 *  @brief Bluetooth Object Transfer Service
 *
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <stdbool.h>
#include <device.h>
#include <init.h>
#include <stdio.h>
#include <zephyr/types.h>
#include <sys/byteorder.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/gatt.h>
#include <bluetooth/l2cap.h>

#include "ots.h"
#include "ots_internal.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_OTS)
#define LOG_MODULE_NAME bt_ots_tmp
#include "common/log.h"

#define CREDITS                       10
#define DATA_MTU                      (23 * CREDITS)


#define L2CH_CHAN(_chan) CONTAINER_OF(_chan, struct l2ch, ch.chan)
#define L2CH_WORK(_work) CONTAINER_OF(_work, struct l2ch, recv_work)
#define L2CAP_CHAN(_chan) _chan->ch.chan

static int l2cap_accept(struct bt_conn *conn, struct bt_l2cap_chan **chan);
static struct net_buf *l2cap_alloc_buf(struct bt_l2cap_chan *chan);
static int l2cap_recv(struct bt_l2cap_chan *chan, struct net_buf *buf);
static void l2cap_sent(struct bt_l2cap_chan *chan);
static void l2cap_status(struct bt_l2cap_chan *chan, atomic_t *status);
static void l2cap_connected(struct bt_l2cap_chan *chan);
static void l2cap_disconnected(struct bt_l2cap_chan *chan);
static int l2cap_obj_send(struct ots_svc_inst_t *inst, uint32_t data_len,
			  struct bt_conn *conn);

NET_BUF_POOL_FIXED_DEFINE(ots_tx_pool, 1, DATA_MTU, NULL);
NET_BUF_SIMPLE_DEFINE_STATIC(gatt_buf, CONFIG_BT_L2CAP_TX_MTU);

struct l2ch {
	struct k_delayed_work recv_work;
	struct bt_l2cap_le_chan ch;
};

struct ots_object {
	uint32_t                    alloc_len;
	bool                        is_valid;
	bool                        is_locked;
	struct bt_date_time         first_created;
	struct bt_date_time         last_modified;
	uint64_t                    id;
	bool                        in_filter1;
	bool                        in_filter2;
	bool                        in_filter3;
	struct bt_ots_obj_metadata  metadata;
};

struct object_list {
	uint32_t            obj_cnt;
	struct ots_object   objects[BT_OTS_MAX_OBJ_CNT];
};

/** Data specific for each instance */
struct ots_svc_inst_t {
	struct bt_ots_cb *cbs;
	bool                                  is_oacp_enabled;
	bool                                  is_olcp_enabled;
	bool                                  is_read_to_be_started;
	struct bt_gatt_service                service;
	struct ots_object                     *cur_obj_p;
	uint32_t                              requested_read_size;
	uint32_t                              requested_read_offset;
	struct object_list                    obj_list;
	/* TODO: Keep a shared buffer for the dirlist content, and only
	 * when request, populate the content, rather than keeping this
	 * content per instance all the time to save RAM
	 */
	uint8_t dirlisting_content[BT_OTS_DIR_LISTING_MAX_SIZE];
	struct ots_object                     *p_dirlisting_obj;
	struct bt_ots_feat                    features;
};

/** Global OTS data and states */
struct ots_inst_t {
	struct ots_svc_inst_t       svc_insts[CONFIG_BT_OTS_TEMP_SERVICE_COUNT];
	struct ots_svc_inst_t       *active_inst;
	bool                        l2cap_is_channel_available;
	uint32_t                    remaining_len;
	uint32_t                    sent_len;
	struct bt_l2cap_server      ots_l2cap_coc;
	struct bt_l2cap_chan_ops    l2cap_ops;
	struct l2ch                 l2ch_chan;
};

/* Global OTS instance that holds information about the service instances */
static struct ots_inst_t ots_inst;

static ssize_t feature_read(struct bt_conn *conn,
			    const struct bt_gatt_attr *attr, void *buf,
			    uint16_t len, uint16_t offset);

static ssize_t obj_name_read(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr, void *buf,
			     uint16_t len, uint16_t offset);

static ssize_t obj_type_read(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr, void *buf,
			     uint16_t len, uint16_t offset);

static ssize_t obj_id_read(struct bt_conn *conn,
			   const struct bt_gatt_attr *attr, void *buf,
			   uint16_t len, uint16_t offset);

static ssize_t obj_size_read(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr, void *buf,
			     uint16_t len, uint16_t offset);

static ssize_t obj_prop_read(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr, void *buf,
			     uint16_t len, uint16_t offset);

static ssize_t oacp_write(struct bt_conn *conn,
			  const struct bt_gatt_attr *attr,
			  const void *buf, uint16_t len, uint16_t offset,
			  uint8_t flags);

static void oacp_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value);

static ssize_t olcp_write(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			  const void *buf, uint16_t len, uint16_t offset,
			  uint8_t flags);

static void olcp_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value);

#define BT_OTS_SERVICE_DEFINITION(i)                                        \
	BT_GATT_SECONDARY_SERVICE(BT_UUID_OTS),                             \
	BT_GATT_CHARACTERISTIC(BT_UUID_OTS_FEATURE,                         \
		BT_GATT_CHRC_READ, BT_GATT_PERM_READ,                       \
		feature_read, NULL, &ots_inst.svc_insts[i]),                \
	BT_GATT_CHARACTERISTIC(BT_UUID_OTS_NAME,                            \
		BT_GATT_CHRC_READ, BT_GATT_PERM_READ,                       \
		obj_name_read, NULL, &ots_inst.svc_insts[i]),               \
	BT_GATT_CHARACTERISTIC(BT_UUID_OTS_TYPE,                            \
		BT_GATT_CHRC_READ, BT_GATT_PERM_READ,                       \
		obj_type_read, NULL, &ots_inst.svc_insts[i]),               \
	BT_GATT_CHARACTERISTIC(BT_UUID_OTS_ID,                              \
		BT_GATT_CHRC_READ, BT_GATT_PERM_READ,                       \
		obj_id_read, NULL, &ots_inst.svc_insts[i]),                 \
	BT_GATT_CHARACTERISTIC(BT_UUID_OTS_PROPERTIES,                      \
		BT_GATT_CHRC_READ, BT_GATT_PERM_READ,                       \
		obj_prop_read, NULL, &ots_inst.svc_insts[i]),               \
	BT_GATT_CHARACTERISTIC(BT_UUID_OTS_ACTION_CP,                       \
		BT_GATT_CHRC_WRITE | BT_GATT_CHRC_INDICATE,                 \
		BT_GATT_PERM_WRITE, NULL,                                   \
		oacp_write, &ots_inst.svc_insts[i]),                        \
	BT_GATT_CCC(oacp_cfg_changed,                                       \
		BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),                    \
	BT_GATT_CHARACTERISTIC(BT_UUID_OTS_LIST_CP,                         \
		BT_GATT_CHRC_WRITE | BT_GATT_CHRC_INDICATE,                 \
		BT_GATT_PERM_WRITE, NULL,                                   \
		olcp_write, &ots_inst.svc_insts[i]),                        \
	BT_GATT_CCC(olcp_cfg_changed,                                       \
		BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),                    \
	BT_GATT_CHARACTERISTIC(BT_UUID_OTS_SIZE,                            \
		BT_GATT_CHRC_READ, BT_GATT_PERM_READ,                       \
		obj_size_read, NULL, &ots_inst.svc_insts[i])

#if CONFIG_BT_OTS_TEMP_SERVICE_COUNT > 0
static struct bt_gatt_attr ots_svc_attrs[
	CONFIG_BT_OTS_TEMP_SERVICE_COUNT][19] = {
	{ BT_OTS_SERVICE_DEFINITION(0) },
#if CONFIG_BT_OTS_TEMP_SERVICE_COUNT > 1
	{ BT_OTS_SERVICE_DEFINITION(1) },
#if CONFIG_BT_OTS_TEMP_SERVICE_COUNT > 2
	{ BT_OTS_SERVICE_DEFINITION(2) },
#if CONFIG_BT_OTS_TEMP_SERVICE_COUNT > 3
	{ BT_OTS_SERVICE_DEFINITION(3) },
#if CONFIG_BT_OTS_TEMP_SERVICE_COUNT > 4
	{ BT_OTS_SERVICE_DEFINITION(4) },
#endif /* CONFIG_BT_OTS_TEMP_SERVICE_COUNT > 4 */
#endif /* CONFIG_BT_OTS_TEMP_SERVICE_COUNT > 3 */
#endif /* CONFIG_BT_OTS_TEMP_SERVICE_COUNT > 2 */
#endif /* CONFIG_BT_OTS_TEMP_SERVICE_COUNT > 1 */
};
#endif /* CONFIG_BT_OTS_TEMP_SERVICE_COUNT > 0 */



static struct ots_svc_inst_t *lookup_inst_by_attr(
	const struct bt_gatt_attr *attr)
{
	__ASSERT(attr, "Attr pointer shall be non-NULL");
	struct ots_svc_inst_t *inst;

	for (int i = 0; i < ARRAY_SIZE(ots_inst.svc_insts); i++) {
		inst = &ots_inst.svc_insts[i];

		for (int j = 0; j < inst->service.attr_count; j++) {
			if (inst->service.attrs[j].user_data ==
				attr->user_data) {
				return inst;
			}
		}
	}
	return NULL;
}

static uint32_t on_dir_listing_content(struct ots_svc_inst_t *inst,
				       uint8_t **data, uint32_t len,
				       uint32_t offset)
{
	*data = &inst->dirlisting_content[offset];
	return (inst->p_dirlisting_obj->metadata.size - offset);
}

/* L2CAP CoC*/
static int l2cap_recv(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	/* TODO: Implement object write */
	BT_DBG("Received data on L2CAP CoC, not EXPECTED, ignoring");
	return 0;
}

static struct net_buf *l2cap_alloc_buf(struct bt_l2cap_chan *chan)
{
	BT_DBG("L2CAP alloc requested for receiving, not EXPECTED, ignore");
	return NULL;
}

static int l2cap_accept(struct bt_conn *conn, struct bt_l2cap_chan **chan)
{
	BT_DBG("Incoming conn %p", conn);
	ots_inst.l2cap_is_channel_available = false;

	if (ots_inst.l2ch_chan.ch.chan.conn) {
		BT_DBG("No channels available");
		return -ENOMEM;
	}

	*chan = &ots_inst.l2ch_chan.ch.chan;

	return 0;
}

static void l2cap_connected(struct bt_l2cap_chan *chan)
{
	if (chan == &ots_inst.l2ch_chan.ch.chan) {
		BT_DBG("Channel %p connected", chan);
		ots_inst.l2cap_is_channel_available = true;
	} else {
		BT_DBG("Unknown channel connected");
	}
}

static void l2cap_disconnected(struct bt_l2cap_chan *chan)
{
	if (chan == &ots_inst.l2ch_chan.ch.chan) {
		BT_DBG("Channel %p disconnected", chan);
		ots_inst.l2cap_is_channel_available = false;
	} else {
		BT_DBG("Unknown channel disconnected");
	}
}

static int resume_send(struct bt_conn *conn)
{
	int ret;
	struct net_buf *buf;
	uint32_t max_tx_len = MIN(ots_inst.l2ch_chan.ch.tx.mtu,
				  DATA_MTU - BT_L2CAP_CHAN_SEND_RESERVE);
	struct ots_svc_inst_t *active_inst = ots_inst.active_inst;
	uint8_t *p_data = NULL;
	uint32_t len = ots_inst.remaining_len;
	struct ots_object *cur_obj;

	if (!active_inst) {
		BT_WARN("No active service instance");
		ots_inst.remaining_len = 0;
		ots_inst.sent_len = 0;
		return -ENOEXEC;
	}

	if (ots_inst.remaining_len == 0) {
		ots_inst.active_inst = NULL;
		ots_inst.sent_len = 0;
		return 0;
	}

	cur_obj = active_inst->cur_obj_p;

	if (len > max_tx_len) {
		len = max_tx_len;
	}

	if (active_inst->cbs->obj_read) {
		uint32_t buf_len;

		if (cur_obj->id == BT_OTS_DIRLISTING_ID) {
			buf_len = on_dir_listing_content(active_inst, &p_data,
							 len,
							 ots_inst.sent_len);
		} else {
			buf_len = active_inst->cbs->obj_read(
				active_inst, conn, cur_obj->id, &p_data, len,
				ots_inst.sent_len);
		}


		if (!buf_len || !p_data) {
			BT_WARN("content callback returned 0 length");
			return -ENOEXEC;
		}

		if (buf_len < len) {
			len = buf_len;
		}

		buf = net_buf_alloc(&ots_tx_pool, K_FOREVER);
		net_buf_reserve(buf, BT_L2CAP_CHAN_SEND_RESERVE);
		net_buf_add_mem(buf, p_data, len);
	} else {
		buf = net_buf_alloc(&ots_tx_pool, K_FOREVER);
		net_buf_reserve(buf, BT_L2CAP_CHAN_SEND_RESERVE);
		net_buf_add_mem(buf, &p_data[ots_inst.sent_len], len);
	}
	BT_DBG("--- sending %u bytes on L2CAP CoC ---", len);
	ret = bt_l2cap_chan_send(&ots_inst.l2ch_chan.ch.chan, buf);

	if (ret < 0) {
		BT_DBG("Unable to send: %d", -ret);
		net_buf_unref(buf);
		return -ENOEXEC;
	}

	ots_inst.sent_len      += len;
	ots_inst.remaining_len -= len;
	return len;

}

static void l2cap_status(struct bt_l2cap_chan *chan, atomic_t *status)
{
	int credits = atomic_get(&ots_inst.l2ch_chan.ch.tx.credits);

	BT_DBG("Channel %p status %u, credits %i", chan, *status, credits);
}

static void l2cap_sent(struct bt_l2cap_chan *chan)
{
	int credits = atomic_get(&ots_inst.l2ch_chan.ch.tx.credits);
	int err = 0;

	BT_DBG("Out channel %p transmitted, credits %i, remaining length %i",
	       chan, credits, (int)ots_inst.remaining_len);

	if (ots_inst.remaining_len > 0) {
		BT_DBG("still %u bytes to send, resume sending",
		       ots_inst.remaining_len);

		err = resume_send(chan->conn);
	} else {
		ots_inst.active_inst = NULL;
		BT_DBG("Read completed");

		for (int i = 0; i < ARRAY_SIZE(ots_inst.svc_insts); i++) {
			struct ots_svc_inst_t *inst = &ots_inst.svc_insts[i];

			if (inst->is_read_to_be_started) {
				BT_DBG("Executing read for next service inst");
				l2cap_obj_send(inst, inst->requested_read_size,
					       chan->conn);
			}
		}
	}

	if (err) {
		BT_ERR("Could not send next data block %d", err);
	}
}

static int l2cap_obj_send(struct ots_svc_inst_t *inst, uint32_t data_len,
			  struct bt_conn *conn)
{
	int credits;
	int ret;

	if (!ots_inst.l2cap_is_channel_available) {
		return -EBUSY;
	} else if (ots_inst.active_inst) {
		return -EBUSY;
	}

	ots_inst.active_inst = inst;

	ots_inst.remaining_len  = data_len;
	ots_inst.sent_len       = 0;

	credits = atomic_get(&ots_inst.l2ch_chan.ch.tx.credits);

	BT_DBG("sending on L2CAP CoC, available credits %d", credits);

	if (credits == 0) {
		BT_DBG("no credits available");
	}

	ret = resume_send(conn);

	if (ret < 0) {
		BT_WARN("Unable to send: %d", -ret);
		ots_inst.active_inst = NULL;
		return ret;
	}

	BT_DBG("sent %i bytes", ret);

	return 0;
}

/******************** Object Action Control Point ****************************/
void oacp_ind_cb(struct bt_conn *conn, struct bt_gatt_indicate_params *params,
		 uint8_t err)
{
	struct ots_svc_inst_t *inst = lookup_inst_by_attr(params->attr);

	if (!inst) {
		BT_WARN("Could not find instance from attr");
		return;
	}

	BT_DBG("oacp indication callback, err: 0x%4x", err);
	if (inst->is_read_to_be_started) {
		uint32_t offset = inst->requested_read_offset;
		int err_code;

		BT_DBG("Start sending object, size %u, offset %u",
		       inst->requested_read_size, offset);

		err_code = l2cap_obj_send(inst, inst->requested_read_size,
					  conn);

		if (err_code) {
			BT_DBG("l2cap_obj_send returned 0x%x", -err_code);
			if (err_code == -EBUSY) {
				BT_INFO("Queueing instance object to be sent");
			}
		} else {
			inst->is_read_to_be_started = false;
		}
	}
}

static enum bt_ots_oacp_res_code oacp_read_proc(struct ots_svc_inst_t *inst,
						uint32_t offset,
						uint32_t length)
{
	inst->is_read_to_be_started = false;
	inst->requested_read_size = 0;

	if (inst->cur_obj_p == NULL) {
		BT_DBG("read object: current object points to NULL");
		return BT_OTS_OACP_RES_INV_OBJ;
	}

	if (inst->cur_obj_p->is_valid == false) {
		BT_DBG("read object: current object is invalid");
		return BT_OTS_OACP_RES_INV_OBJ;
	}

	if (!BT_OTS_OBJ_GET_PROP_READ(inst->cur_obj_p->metadata.properties)) {
		return BT_OTS_OACP_RES_NOT_PERMITTED;
	}

	if (ots_inst.l2cap_is_channel_available == false) {
		return BT_OTS_OACP_RES_CHAN_UNAVAIL;
	}

	if (length + offset > inst->cur_obj_p->metadata.size) {
		return BT_OTS_OACP_RES_INV_PARAM;
	}

	inst->requested_read_size   = length;
	inst->requested_read_offset = offset;

	if (inst->cur_obj_p->is_locked) {
		return BT_OTS_OACP_RES_OBJ_LOCKED;
	}

	inst->is_read_to_be_started = true;
	return BT_OTS_OACP_RES_SUCCESS;
}

static int oacp_response_send(struct ots_svc_inst_t *inst,
			      enum bt_ots_oacp_proc_type req_op_code,
			      enum bt_ots_oacp_res_code result_code)
{
	static struct bt_gatt_indicate_params oacp_ind = { 0 };

	net_buf_simple_reset(&gatt_buf);

	net_buf_simple_add_u8(&gatt_buf, BT_OTS_OACP_PROC_RESP);
	net_buf_simple_add_u8(&gatt_buf, req_op_code);
	net_buf_simple_add_u8(&gatt_buf, result_code);

	/* Notification Attribute UUID type */
	oacp_ind.uuid = BT_UUID_OTS_ACTION_CP;
	/* Indicate Attribute object*/
	oacp_ind.attr = inst->service.attrs;
	/* Indicate Value callback */
	oacp_ind.func = oacp_ind_cb;
	/* Indicate Value data*/
	oacp_ind.data = gatt_buf.data;
	/* Indicate Value length*/
	oacp_ind.len = gatt_buf.len;

	BT_DBG("sending indication on OACP");
	return bt_gatt_indicate(NULL, &oacp_ind);
}

static enum bt_ots_oacp_res_code oacp_do_proc(struct ots_svc_inst_t *inst,
					      struct bt_ots_oacp_proc *p_proc)
{
	enum bt_ots_oacp_res_code oacp_status;

	if (p_proc == NULL) {
		return BT_OTS_OACP_RES_INV_PARAM;
	}

	switch (p_proc->type) {
	case BT_OTS_OACP_PROC_WRITE:
		/* Unsupported op code.*/
		oacp_status = BT_OTS_OACP_RES_OPCODE_NOT_SUP;
		break;
	case BT_OTS_OACP_PROC_READ:
		oacp_status = oacp_read_proc(inst,
					     p_proc->read_params.offset,
					     p_proc->read_params.length);
		break;
	case BT_OTS_OACP_PROC_CREATE:
		/* Unsupported op code.*/
		oacp_status = BT_OTS_OACP_RES_OPCODE_NOT_SUP;
		break;
	case BT_OTS_OACP_PROC_ABORT:
		/* Unsupported op code.*/
		oacp_status = BT_OTS_OACP_RES_OPCODE_NOT_SUP;
		break;
	default:
		/* Unsupported op code.*/
		oacp_status = BT_OTS_OACP_RES_OPCODE_NOT_SUP;
	}
	return oacp_status;
}

static enum bt_ots_oacp_res_code decode_oacp_command(
	const void *buf, uint16_t len, uint16_t offset,
	struct bt_ots_oacp_proc *p_proc)
{
	struct net_buf_simple net_buf;

	net_buf_simple_init_with_data(&net_buf, (void *)buf, len);

	p_proc->type = net_buf_simple_pull_u8(&net_buf);

	switch (p_proc->type) {
	case BT_OTS_OACP_PROC_READ:
		BT_DBG("len: %d", len);
		if (len != sizeof(p_proc->read_params.offset) +
				sizeof(p_proc->read_params.length) +
				sizeof(p_proc->type)) {
			return BT_OTS_OACP_RES_INV_PARAM;
		}

		p_proc->read_params.offset = net_buf_simple_pull_le32(&net_buf);
		p_proc->read_params.length = net_buf_simple_pull_le32(&net_buf);
		break;
	case BT_OTS_OACP_PROC_WRITE:
		return BT_OTS_OACP_RES_OPCODE_NOT_SUP;
	case BT_OTS_OACP_PROC_CREATE:
		return BT_OTS_OACP_RES_OPCODE_NOT_SUP;
	default:
		return BT_OTS_OACP_RES_OPCODE_NOT_SUP;
	}
	return BT_OTS_OACP_RES_SUCCESS;
}

static ssize_t oacp_write(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			  const void *buf, uint16_t len, uint16_t offset,
			  uint8_t flags)
{
	struct ots_svc_inst_t *inst = (struct ots_svc_inst_t *)attr->user_data;
	enum bt_ots_oacp_res_code oacp_status = { 0 };
	struct bt_ots_oacp_proc oacp_proc = { 0 };

	if (offset != 0) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (!inst->is_oacp_enabled) {
		BT_DBG("OACP indications not enabled.");
		return BT_GATT_ERR(BT_ATT_ERR_CCC_IMPROPER_CONF);
	}

	oacp_status = decode_oacp_command(buf, len, offset, &oacp_proc);
	BT_DBG("Opcode: 0x%x, status 0x%x", oacp_proc.type, oacp_status);

	if (oacp_status == BT_OTS_OACP_RES_SUCCESS) {
		BT_DBG("decoding OACP SUCCESS");
		if (oacp_proc.type == BT_OTS_OACP_PROC_READ) {
			BT_DBG("Received Read %d bytes at offset %d",
			       oacp_proc.read_params.length,
			       oacp_proc.read_params.offset);
		}

		oacp_status = oacp_do_proc(inst, &oacp_proc);
		BT_DBG("oacp_do_proc status 0x%x", oacp_status);
	}

	oacp_response_send(inst, oacp_proc.type, oacp_status);
	return len;
}

static void oacp_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	struct ots_svc_inst_t *inst = lookup_inst_by_attr(attr);

	if (!inst) {
		BT_WARN("Could not find instance from CCC");
		return;
	}

	BT_DBG("OACP CCCD value 0x%04x", value);
	inst->is_oacp_enabled = false;

	if (value == BT_GATT_CCC_INDICATE) {
		inst->is_oacp_enabled = true;
	}
}


static ssize_t feature_read(struct bt_conn *conn,
			    const struct bt_gatt_attr *attr, void *buf,
			    uint16_t len, uint16_t offset)
{
	struct ots_svc_inst_t *inst = (struct ots_svc_inst_t *)attr->user_data;

	net_buf_simple_reset(&gatt_buf);

	net_buf_simple_add_le32(&gatt_buf, inst->features.oacp);
	net_buf_simple_add_le32(&gatt_buf, inst->features.olcp);

	BT_DBG("oacp feature: 0x%04x", inst->features.oacp);
	BT_DBG("olcp feature: 0x%04x", inst->features.olcp);

	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 gatt_buf.data, gatt_buf.len);
}

static ssize_t obj_name_read(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr, void *buf,
			     uint16_t len, uint16_t offset)
{
	struct ots_svc_inst_t *inst = (struct ots_svc_inst_t *)attr->user_data;

	if (inst->cur_obj_p == NULL) {
		BT_DBG("read object name: no object selected!");
		return BT_GATT_ERR(BT_OTS_OBJECT_NOT_SELECTED);
	}

	if (!inst->cur_obj_p->is_valid) {
		BT_DBG("read object name: invalid object!");
		return BT_GATT_ERR(BT_OTS_OBJECT_NOT_SELECTED);
	}

	BT_DBG("object name: %s", log_strdup(inst->cur_obj_p->metadata.name));

	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 inst->cur_obj_p->metadata.name,
				 strlen(inst->cur_obj_p->metadata.name));
}

static ssize_t obj_type_read(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr, void *buf,
			     uint16_t len, uint16_t offset)
{
	struct ots_svc_inst_t *inst = (struct ots_svc_inst_t *)attr->user_data;

	net_buf_simple_reset(&gatt_buf);

	if (inst->cur_obj_p == NULL) {
		return BT_GATT_ERR(BT_OTS_OBJECT_NOT_SELECTED);
	}

	if (!inst->cur_obj_p->is_valid) {
		return BT_GATT_ERR(BT_OTS_OBJECT_NOT_SELECTED);
	}

	if (inst->cur_obj_p->metadata.type.uuid.type == BT_UUID_TYPE_16) {
		BT_DBG("Object type: 0x%4x",
		       inst->cur_obj_p->metadata.type.u16.val);
		net_buf_simple_add_le16(&gatt_buf,
					inst->cur_obj_p->metadata.type.u16.val);
	} else {
		BT_DBG("Object type:128bits UUID");
		net_buf_simple_add_mem(
			&gatt_buf, inst->cur_obj_p->metadata.type.u128.val,
			sizeof(inst->cur_obj_p->metadata.type.u128.val));
	}

	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 gatt_buf.data, gatt_buf.len);
}

static ssize_t obj_id_read(struct bt_conn *conn,
		const struct bt_gatt_attr *attr, void *buf,
		uint16_t len, uint16_t offset)
{
	struct ots_svc_inst_t *inst = (struct ots_svc_inst_t *)attr->user_data;

	net_buf_simple_reset(&gatt_buf);

	if (inst->cur_obj_p == NULL) {
		return BT_GATT_ERR(BT_OTS_OBJECT_NOT_SELECTED);
	}

	if (!inst->cur_obj_p->is_valid) {
		return BT_GATT_ERR(BT_OTS_OBJECT_NOT_SELECTED);
	}

	/* BT_DBG does not support printing 64 bit types */
	if (IS_ENABLED(CONFIG_BT_DEBUG_OTS)) {
		char t[UINT48_STR_LEN];

		u64_to_uint48array_str(inst->cur_obj_p->id, t);
		BT_DBG("Current ObjId read: 0x%s", log_strdup(t));
	}

	net_buf_simple_add_le48(&gatt_buf, inst->cur_obj_p->id);

	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 gatt_buf.data, gatt_buf.len);
}

static ssize_t obj_size_read(struct bt_conn *conn,
		const struct bt_gatt_attr *attr, void *buf,
		uint16_t len, uint16_t offset)
{
	struct ots_svc_inst_t *inst = (struct ots_svc_inst_t *)attr->user_data;

	net_buf_simple_reset(&gatt_buf);

	if (inst->cur_obj_p == NULL) {
		return BT_GATT_ERR(BT_OTS_OBJECT_NOT_SELECTED);
	}

	if (!inst->cur_obj_p->is_valid) {
		return BT_GATT_ERR(BT_OTS_OBJECT_NOT_SELECTED);
	}

	BT_DBG("Object's current Size: %d", inst->cur_obj_p->metadata.size);
	BT_DBG("Object's allocated Size: %d", inst->cur_obj_p->alloc_len);

	net_buf_simple_add_le32(&gatt_buf, inst->cur_obj_p->metadata.size);
	net_buf_simple_add_le32(&gatt_buf, inst->cur_obj_p->alloc_len);

	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 gatt_buf.data, gatt_buf.len);
}

static ssize_t obj_prop_read(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr, void *buf,
			     uint16_t len, uint16_t offset)
{
	struct ots_svc_inst_t *inst = (struct ots_svc_inst_t *)attr->user_data;

	net_buf_simple_reset(&gatt_buf);

	if (inst->cur_obj_p == NULL) {
		return BT_GATT_ERR(BT_OTS_OBJECT_NOT_SELECTED);
	}

	if (!inst->cur_obj_p->is_valid) {
		return BT_GATT_ERR(BT_OTS_OBJECT_NOT_SELECTED);
	}

	BT_DBG("Object's metadata.properties: 0x%x",
	       inst->cur_obj_p->metadata.properties);

	net_buf_simple_add_le32(&gatt_buf,
				inst->cur_obj_p->metadata.properties);

	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 gatt_buf.data, gatt_buf.len);
}


/***************** Object List Control Point *******************************/
static bool ots_lf_is_obj_valid(struct ots_object *p_obj)
{
	return !(p_obj->in_filter1 || p_obj->in_filter2 || p_obj->in_filter3) &&
		p_obj->is_valid;
}

static enum bt_ots_olcp_res_code olcp_first(struct ots_svc_inst_t *inst,
					    struct bt_conn *conn)
{
	bool found = false;

	for (int i = 0; i < ARRAY_SIZE(inst->obj_list.objects); i++) {
		if (ots_lf_is_obj_valid(&inst->obj_list.objects[i])) {
			inst->cur_obj_p = &inst->obj_list.objects[i];
			found = true;
			break;
		}
	}

	if (!found) {
		return BT_OTS_OLCP_RES_NO_OBJECT;
	}

	if (inst->cbs->obj_selected) {
		inst->cbs->obj_selected(inst, conn, inst->cur_obj_p->id);
	}

	return BT_OTS_OLCP_RES_SUCCESS;
}

static enum bt_ots_olcp_res_code olcp_last(struct ots_svc_inst_t *inst,
					   struct bt_conn *conn)
{
	bool found = false;

	if (!inst->obj_list.obj_cnt) {
		return BT_OTS_OLCP_RES_NO_OBJECT;
	}

	for (int i = ARRAY_SIZE(inst->obj_list.objects) - 1; i >= 0; i--) {
		if (ots_lf_is_obj_valid(&inst->obj_list.objects[i])) {
			inst->cur_obj_p = &inst->obj_list.objects[i];
			found = true;
			break;
		}
	}

	if (!found) {
		return BT_OTS_OLCP_RES_NO_OBJECT;
	}

	if (inst->cbs->obj_selected) {
		inst->cbs->obj_selected(inst, conn, inst->cur_obj_p->id);
	}

	return BT_OTS_OLCP_RES_SUCCESS;
}

static enum bt_ots_olcp_res_code olcp_goto(struct ots_svc_inst_t *inst,
					   uint64_t obj_id,
					   struct bt_conn *conn)
{
	uint64_t pos = obj_id;

	if (obj_id != BT_OTS_DIRLISTING_ID && obj_id < OTS_ID_RANGE_START) {
		return BT_OTS_OLCP_RES_INVALID_PARAMETER;
	}

	if (obj_id == BT_OTS_DIRLISTING_ID) {
		pos = BT_OTS_DIRLISTING_ID;
	} else {
		pos = pos - OTS_ID_RANGE_START;
		if (pos >= BT_OTS_MAX_OBJ_CNT) {
			return BT_OTS_OLCP_RES_OUT_OF_BONDS;
		}

		/*List Filter check*/
		if (!ots_lf_is_obj_valid(&inst->obj_list.objects[pos])) {
			return BT_OTS_OLCP_RES_OBJECT_ID_NOT_FOUND;
		}
	}

	inst->cur_obj_p = &inst->obj_list.objects[pos];

	if (inst->cbs->obj_selected) {
		inst->cbs->obj_selected(inst, conn, inst->cur_obj_p->id);
	}

	return BT_OTS_OLCP_RES_SUCCESS;
}

static enum bt_ots_olcp_res_code olcp_prev(struct ots_svc_inst_t *inst,
					   struct bt_conn *conn)
{
	bool found = false;
	/* We assume current object is the first object in the list, and if not
	 * we update this boolean
	 */
	bool cur_obj_is_first = true;
	struct ots_object *p_obj;
	struct ots_object *first_obj = &inst->obj_list.objects[0];

	if (!inst->cur_obj_p) {
		return BT_OTS_OLCP_RES_OPERATION_FAILED;
	}

	/* Don't change the object is the current object is the first object */
	for (p_obj = inst->cur_obj_p - 1; p_obj >= first_obj; p_obj--) {
		if (ots_lf_is_obj_valid(p_obj)) {
			inst->cur_obj_p = p_obj;
			found = true;
			break;
		} else if (p_obj->is_valid) {
			cur_obj_is_first = false;
		}
	}

	if (!found && !cur_obj_is_first) {
		return BT_OTS_OLCP_RES_NO_OBJECT;
	}

	if (inst->cbs->obj_selected) {
		inst->cbs->obj_selected(inst, conn, inst->cur_obj_p->id);
	}

	return BT_OTS_OLCP_RES_SUCCESS;
}

static enum bt_ots_olcp_res_code olcp_next(struct ots_svc_inst_t *inst,
					   struct bt_conn *conn)
{
	bool found = false;
	/* We assume current object is the last object in the list, and if not
	 * we update this boolean
	 */
	bool cur_obj_is_last = true;
	struct ots_object *p_obj;
	struct ots_object *last_obj =
		&inst->obj_list.objects[ARRAY_SIZE(inst->obj_list.objects)];

	for (p_obj = inst->cur_obj_p + 1; p_obj < last_obj; p_obj++) {
		if (ots_lf_is_obj_valid(p_obj) == true) {
			inst->cur_obj_p = p_obj;
			found = true;
			break;
		} else if (p_obj->is_valid) {
			cur_obj_is_last = false;
		}
	}

	if (!found && !cur_obj_is_last) {
		return BT_OTS_OLCP_RES_NO_OBJECT;
	}

	inst->cur_obj_p = p_obj;

	if (inst->cbs->obj_selected) {
		inst->cbs->obj_selected(inst, conn, inst->cur_obj_p->id);
	}

	return BT_OTS_OLCP_RES_SUCCESS;
}

void olcp_ind_cb(struct bt_conn *conn, struct bt_gatt_indicate_params *params,
		 uint8_t err)
{
	BT_DBG("olcp indication callback, err: 0x%4x", err);
}

static int olcp_response_send(struct ots_svc_inst_t *inst,
			      enum bt_ots_olcp_proc_type req_op_code,
			      enum bt_ots_olcp_res_code result_code)
{
	struct bt_gatt_indicate_params olcp_ind = { 0 };

	net_buf_simple_reset(&gatt_buf);

	net_buf_simple_add_u8(&gatt_buf, BT_OTS_OLCP_PROC_RESP);
	net_buf_simple_add_u8(&gatt_buf, req_op_code);
	net_buf_simple_add_u8(&gatt_buf, result_code);

	/* Indication parameters */
	olcp_ind.uuid = BT_UUID_OTS_LIST_CP;
	olcp_ind.attr = inst->service.attrs;
	olcp_ind.func = olcp_ind_cb;
	olcp_ind.data = gatt_buf.data;
	olcp_ind.len  = gatt_buf.len;

	BT_DBG("sending indication on OLCP");
	return bt_gatt_indicate(NULL, &olcp_ind);
}

static enum bt_ots_olcp_res_code olcp_do_proc(struct ots_svc_inst_t *inst,
					      struct bt_ots_olcp_proc *p_proc,
					      struct bt_conn *conn)
{
	enum bt_ots_olcp_res_code olcp_status;

	if (p_proc == NULL) {
		return BT_OTS_OLCP_RES_INVALID_PARAMETER;
	}

	switch (p_proc->type) {
	case BT_OTS_OLCP_PROC_FIRST:
		olcp_status = olcp_first(inst, conn);
		break;
	case BT_OTS_OLCP_PROC_LAST:
		olcp_status = olcp_last(inst, conn);
		break;
	case BT_OTS_OLCP_PROC_PREV:
		olcp_status = olcp_prev(inst, conn);
		break;
	case BT_OTS_OLCP_PROC_NEXT:
		olcp_status = olcp_next(inst, conn);
		break;
	case BT_OTS_OLCP_PROC_GOTO:
		olcp_status = olcp_goto(inst, p_proc->goto_id, conn);
		break;
	default:
		olcp_status = BT_OTS_OLCP_RES_PROC_NOT_SUP;
		break;
	}
	return olcp_status;
}

static enum bt_ots_olcp_res_code decode_olcp_command(
	const void *buf, uint16_t len, uint16_t offset,
	struct bt_ots_olcp_proc *p_proc)
{
	enum bt_ots_olcp_res_code res_code;
	struct net_buf_simple net_buf;

	net_buf_simple_init_with_data(&net_buf, (void *)buf, len);

	p_proc->type = net_buf_simple_pull_u8(&net_buf);

	switch (p_proc->type) {
	case BT_OTS_OLCP_PROC_FIRST:
	case BT_OTS_OLCP_PROC_LAST:
	case BT_OTS_OLCP_PROC_PREV:
	case BT_OTS_OLCP_PROC_NEXT:
		res_code = BT_OTS_OLCP_RES_SUCCESS;
		break;
	case BT_OTS_OLCP_PROC_GOTO:
		if (len != sizeof(p_proc->type) + UINT48_LEN) {
			return BT_OTS_OLCP_RES_INVALID_PARAMETER;
		}
		p_proc->goto_id = net_buf_simple_pull_le48(&net_buf);
		res_code = BT_OTS_OLCP_RES_SUCCESS;
		break;
	default:
		res_code = BT_OTS_OLCP_RES_PROC_NOT_SUP;
		break;
	}
	return res_code;
}

static ssize_t olcp_write(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			  const void *buf, uint16_t len, uint16_t offset,
			  uint8_t flags)
{
	struct ots_svc_inst_t *inst = (struct ots_svc_inst_t *)attr->user_data;

	enum bt_ots_olcp_res_code olcp_status;
	struct bt_ots_olcp_proc   olcp_proc = { 0 };

	if (offset != 0) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (!inst->is_olcp_enabled) {
		BT_DBG("OLCP indications not enabled.");
		return BT_GATT_ERR(BT_ATT_ERR_CCC_IMPROPER_CONF);
	}

	olcp_status = decode_olcp_command(buf, len, offset, &olcp_proc);
	BT_DBG("OLCP: Opcode: 0x%x, status 0x%x", olcp_proc.type, olcp_status);

	if (olcp_status == BT_OTS_OLCP_RES_SUCCESS) {
		olcp_status = olcp_do_proc(inst, &olcp_proc, conn);
		BT_DBG("ots_olcp_do_proc status 0x%x", olcp_status);
	}

	olcp_response_send(inst, olcp_proc.type, olcp_status);
	return len;
}

static void olcp_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	struct ots_svc_inst_t *inst = lookup_inst_by_attr(attr);

	if (!inst) {
		BT_WARN("Could not find instance from CCC");
		return;
	}

	BT_DBG("OLCP CCCD value 0x%04x", value);
	inst->is_olcp_enabled = false;

	if (value == BT_GATT_CCC_INDICATE) {
		inst->is_olcp_enabled = true;
	}
}

static void encode_record(struct ots_object *p_obj,
			  struct net_buf_simple *net_buf,
			  uint8_t **pp_index_dirlisting_size)
{
	uint8_t flags = 0;
	uint8_t *record_length_p = net_buf->data;
	uint16_t start_len = net_buf->len;

	BT_OTS_DIRLIST_SET_FLAG_PROPERTIES(flags, true);
	BT_OTS_DIRLIST_SET_FLAG_CUR_SIZE(flags, true);
	if (p_obj->metadata.type.uuid.type == BT_UUID_TYPE_128) {
		BT_OTS_DIRLIST_SET_FLAG_TYPE_128(flags, true);
	}

	/*skip 16bits at the beginning of the record for the record's length*/
	net_buf_simple_add(net_buf, sizeof(uint16_t));

	net_buf_simple_add_le48(net_buf, p_obj->id);

	net_buf_simple_add_u8(net_buf, strlen(p_obj->metadata.name));

	net_buf_simple_add_mem(net_buf, p_obj->metadata.name,
			       strlen(p_obj->metadata.name));

	net_buf_simple_add_u8(net_buf, flags);

	/*encode Object type*/
	if (p_obj->metadata.type.uuid.type == BT_UUID_TYPE_16) {
		net_buf_simple_add_le16(net_buf, p_obj->metadata.type.u16.val);
	} else {
		net_buf_simple_add_mem(net_buf, p_obj->metadata.type.u128.val,
				       BT_UUID_SIZE_128);
	}

	/* encode Object Current size if required*/
	if (p_obj->id == OTS_DIR_LISTING_ID) {
		/* save the index, update it later*/
		*pp_index_dirlisting_size = net_buf->data;
		net_buf_simple_add(net_buf, sizeof(uint32_t));
	} else {
		net_buf_simple_add_le32(net_buf, p_obj->metadata.size);
	}

	net_buf_simple_add_le32(net_buf, p_obj->metadata.properties);

	/*encode the record length at the beginning*/
	sys_put_le16(net_buf->len - start_len, record_length_p);
}

static void encode_object_list(struct ots_svc_inst_t *inst)
{
	/* the position of where the size of dirlisting object must be saved*/
	uint8_t *p_index_dirlisting_size = NULL;
	struct net_buf_simple net_buf;

	/* Init with size 0 to reset data */
	net_buf_simple_init_with_data(&net_buf, inst->dirlisting_content,
				      sizeof(inst->dirlisting_content));
	net_buf.len = 0;

	for (uint32_t i = 0; i < ARRAY_SIZE(inst->obj_list.objects); i++) {
		if (ots_lf_is_obj_valid(&inst->obj_list.objects[i])) {
			encode_record(&inst->obj_list.objects[i], &net_buf,
				      &p_index_dirlisting_size);
		}
	}

	inst->p_dirlisting_obj->metadata.size = net_buf.len;

	/*re-encode the directory listing object size with the is new size*/
	sys_put_le32(inst->p_dirlisting_obj->metadata.size,
		     p_index_dirlisting_size);
}

static struct ots_object *get_object_by_id(struct ots_svc_inst_t *inst,
					   uint64_t id)
{
	uint64_t pos = id - OTS_ID_RANGE_START;
	struct ots_object *obj;

	if ((id < OTS_ID_RANGE_START) || (pos >= BT_OTS_MAX_OBJ_CNT)) {
		return NULL;
	}

	obj = &inst->obj_list.objects[pos];

	if (!obj->is_valid || obj->id != id) {
		return NULL;
	}

	return obj;
}

static void create_dir_listing_object(struct ots_svc_inst_t *inst)
{
	static char *dir_listing_name = OTS_DIR_LISTING_NAME;

	inst->p_dirlisting_obj = &inst->obj_list.objects[0];
	inst->obj_list.obj_cnt = 1;

	inst->p_dirlisting_obj->is_valid = true;

	inst->p_dirlisting_obj->metadata.name = dir_listing_name;

	inst->p_dirlisting_obj->id = OTS_DIR_LISTING_ID;
	inst->p_dirlisting_obj->metadata.type.uuid.type = BT_UUID_TYPE_16;
	inst->p_dirlisting_obj->metadata.type.u16.val = OTS_DIR_LISTING_TYPE;
	BT_OTS_OBJ_SET_PROP_READ(inst->p_dirlisting_obj->metadata.properties,
				 true);

	inst->p_dirlisting_obj->alloc_len = BT_OTS_DIR_LISTING_MAX_SIZE;

	encode_object_list(inst);
}

/* Register the service */
static int bt_ots_init(const struct device *unused)
{
	ots_inst.ots_l2cap_coc.accept    = l2cap_accept;
	ots_inst.ots_l2cap_coc.psm       = OTS_L2CAP_PSM;
	ots_inst.ots_l2cap_coc.sec_level = BT_SECURITY_L1;
	ots_inst.l2cap_ops.alloc_buf     = l2cap_alloc_buf;
	ots_inst.l2cap_ops.recv          = l2cap_recv;
	ots_inst.l2cap_ops.sent          = l2cap_sent;
	ots_inst.l2cap_ops.status        = l2cap_status;
	ots_inst.l2cap_ops.connected     = l2cap_connected;
	ots_inst.l2cap_ops.disconnected  = l2cap_disconnected;
	ots_inst.l2ch_chan.ch.chan.ops   = &ots_inst.l2cap_ops;
	ots_inst.l2ch_chan.ch.rx.mtu     = DATA_MTU;

	if (bt_l2cap_server_register(&ots_inst.ots_l2cap_coc) < 0) {
		BT_DBG("Unable to register OTS_psm (0x%4x)",
		       ots_inst.ots_l2cap_coc.psm);
		return -ENOEXEC;
	}

	BT_DBG("L2CAP psm 0x%4x sec_level %u registered",
	       ots_inst.ots_l2cap_coc.psm, ots_inst.ots_l2cap_coc.sec_level);
	return 0;
}

/****************************** PUBLIC API ******************************/

struct ots_svc_inst_t *bt_ots_register_service(
	struct bt_ots_service_register_t *service_reg)
{
	struct ots_svc_inst_t *inst = NULL;
	int i;
	int err;

	__ASSERT(service_reg->cb, "Callbacks shall be non-NULL");
	__ASSERT(service_reg->cb->obj_created,
		 "Created callback shall be non-NULL");
	__ASSERT(service_reg->cb->obj_read, "obj_read shall be set");

	for (i = 0; i < ARRAY_SIZE(ots_inst.svc_insts); i++) {
		if (!ots_inst.svc_insts[i].cbs) {
			inst = &ots_inst.svc_insts[i];
			break;
		}
	}

	if (inst) {
		inst->cbs = service_reg->cb;
		inst->service = (struct bt_gatt_service)
				BT_GATT_SERVICE(ots_svc_attrs[i]);

		err = bt_gatt_service_register(&inst->service);

		BT_DBG("TEST");
		if (err) {
			BT_DBG("Could not register OTS service: %d", err);
		}


		for (int i = 0; i < ARRAY_SIZE(inst->obj_list.objects); i++) {
			struct ots_object *p_obj = &inst->obj_list.objects[i];
			uint64_t id = OTS_ID_RANGE_START + i;

			p_obj->id = id;
		}

		inst->features = service_reg->features;

		create_dir_listing_object(inst);

		/*by default, select the directory listing object*/
		inst->cur_obj_p = inst->p_dirlisting_obj;
	}

	return inst;
}

int bt_ots_unregister_service(struct ots_svc_inst_t *inst)
{
	if (!inst) {
		return -EINVAL;
	}

	for (int i = 0; i < ARRAY_SIZE(ots_inst.svc_insts); i++) {
		if (&ots_inst.svc_insts[i] == inst) {
			memset(inst, 0, sizeof(*inst));
			return 0;
		}
	}

	return -EINVAL;
}

void *bt_ots_get_incl(struct ots_svc_inst_t *inst)
{
	return (void *)inst->service.attrs;
}

int bt_ots_add_object(struct ots_svc_inst_t *inst,
		      struct bt_ots_obj_metadata *p_metadata)
{
	struct ots_object *obj = NULL;

	__ASSERT(p_metadata->name, "name shall be set");

	if (inst->obj_list.obj_cnt >= BT_OTS_MAX_OBJ_CNT) {
		BT_DBG("No more space to add objects");
		return -ENOMEM;
	}

	for (int i = 0; i < BT_OTS_MAX_OBJ_CNT; i++) {
		if (!inst->obj_list.objects[i].is_valid) {
			obj = &inst->obj_list.objects[i];
			inst->obj_list.obj_cnt++;
			break;
		}
	}

	if (!obj) {
		/* Should not happen - It would only happen if an object has
		 * been marked as invalid while not decrementing the obj_cnt
		 */
		BT_DBG("No free object found");
		return -ENOMEM;
	}

	if (p_metadata->type.uuid.type != BT_UUID_TYPE_16 &&
	    p_metadata->type.uuid.type != BT_UUID_TYPE_128) {
		BT_ERR("Invalid UUID type %u", p_metadata->type.uuid.type);
		return -EINVAL;
	}

	obj->alloc_len = p_metadata->size;
	obj->is_valid = true;
	memcpy(&obj->metadata, p_metadata, sizeof(obj->metadata));

	inst->cbs->obj_created(inst, NULL, obj->id, &obj->metadata);

	/* Update the directory listing object */
	encode_object_list(inst);
	return 0;
}

int bt_ots_remove_object(struct ots_svc_inst_t *inst, uint64_t id)
{
	struct ots_object *obj = get_object_by_id(inst, id);

	if (!obj) {
		return -EINVAL;
	}

	obj->is_valid = false;

	return 0;
}

struct bt_ots_obj_metadata *bt_ots_get_object(struct ots_svc_inst_t *inst,
					      uint64_t id)
{
	struct ots_object *obj = get_object_by_id(inst, id);

	if (obj) {
		return &obj->metadata;
	}

	return NULL;
}

int bt_ots_update_object(struct ots_svc_inst_t *inst, uint64_t id,
			 struct bt_ots_obj_metadata *p_metadata)
{
	struct ots_object *obj;

	__ASSERT(p_metadata->name, "name shall be set");

	obj = get_object_by_id(inst, id);

	if (!obj) {
		return -EINVAL;
	}

	memcpy(&obj->metadata, p_metadata, sizeof(obj->metadata));

	return 0;
}

#if defined(CONFIG_BT_DEBUG_OTS)

void bt_ots_dump_directory_listing(struct ots_svc_inst_t *inst)
{
	struct ots_object *p_dirlisting_obj = inst->p_dirlisting_obj;

	/* TODO: Consider using something like `bt_otc_decode_dirlisting` to
	 * pretty print the dirlisting content
	 */

	BT_HEXDUMP_DBG(inst->dirlisting_content,
		       p_dirlisting_obj->metadata.size,
		       "Content");
}

#endif /* defined(CONFIG_BT_DEBUG_OTS) */

#if defined(CONFIG_BT_DEBUG_OTS)

void bt_ots_dump_metadata(struct bt_ots_obj_metadata *p_metadata)
{
	char uuidstr[BT_UUID_STR_LEN];

	BT_DBG("Object size: %d", p_metadata->size);

	if (p_metadata->type.uuid.type == BT_UUID_TYPE_16) {
		BT_DBG("Object type (UUID): %s, (0x%4x)",
		       bt_ots_uuid_type_str(p_metadata->type.u16),
		       p_metadata->type.u16.val);
	} else {
		bt_uuid_to_str(&p_metadata->type.uuid, uuidstr, sizeof(uuidstr));
		BT_DBG("Object type UUID: %s", log_strdup(uuidstr));
	}
	BT_DBG("Object name: %s", log_strdup(p_metadata->name));
}
#endif /* defined(CONFIG_BT_DEBUG_OTS) */

DEVICE_DEFINE(bt_ots, "bt_ots", &bt_ots_init, NULL, NULL, NULL,
	      APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, NULL);
