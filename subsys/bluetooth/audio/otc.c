/** @file
 *  @brief Bluetooth Object Transfer Client
 *
 * Copyright (c) 2020 - 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* TODO: Temporarily here - clean up, and move alongside the Object Transfer Service */

#include <zephyr.h>
#include <zephyr/types.h>

#include <device.h>
#include <init.h>
#include <sys/byteorder.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/gatt.h>
#include <bluetooth/l2cap.h>

#include "../host/conn_internal.h"  /* To avoid build errors on use of struct bt_conn" */

#include "ots.h"
#include "ots_internal.h"
#include "otc.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_OTC)
#define LOG_MODULE_NAME bt_otc
#include "common/log.h"

/* TODO: KConfig options */
#define OTS_CLIENT_INST_COUNT   1

NET_BUF_POOL_FIXED_DEFINE(ots_c_read_queue, 1,
			  sizeof(struct bt_gatt_read_params), 8, NULL);

#define OTS_WORK(_w) CONTAINER_OF(_w, struct otc_instance_t, work)

static int l2cap_accept(struct bt_conn *conn, struct bt_l2cap_chan **chan);
static struct net_buf *l2cap_alloc_buf(struct bt_l2cap_chan *chan);
static int l2cap_recv(struct bt_l2cap_chan *chan, struct net_buf *buf);
static void l2cap_sent(struct bt_l2cap_chan *chan);
static void l2cap_status(struct bt_l2cap_chan *chan, atomic_t *status);
static void l2cap_connected(struct bt_l2cap_chan *chan);
static void l2cap_disconnected(struct bt_l2cap_chan *chan);

#define CREDITS         10
#define DATA_MTU        (BT_OTC_MAX_WRITE_SIZE * CREDITS)

NET_BUF_POOL_FIXED_DEFINE(otc_l2cap_pool, 1, DATA_MTU, 8, NULL);

struct l2ch {
	struct k_delayed_work recv_work;
	struct bt_l2cap_le_chan ch;
};

#define L2CH_CHAN(_chan) CONTAINER_OF(_chan, struct l2ch, ch.chan)
#define L2CH_WORK(_work) CONTAINER_OF(_work, struct l2ch, recv_work)
#define L2CAP_CHAN(_chan) _chan->ch.chan

struct dirlisting_record_t {
	uint16_t                      len;
	uint8_t                       flags;
	uint8_t                       name_len;
	struct bt_otc_obj_metadata    metadata;
};

/* L2CAP CoC*/
static struct bt_l2cap_server ots_l2cap_coc = {
	.accept     = l2cap_accept,
	.psm        = OTS_L2CAP_PSM,
	.sec_level  = BT_SECURITY_L1,
};

static struct bt_l2cap_chan_ops l2cap_ops = {
	.alloc_buf      = l2cap_alloc_buf,
	.recv           = l2cap_recv,
	.sent           = l2cap_sent,
	.status         = l2cap_status,
	.connected      = l2cap_connected,
	.disconnected   = l2cap_disconnected,
};

static struct l2ch l2ch_chan = {
	.ch.chan.ops    = &l2cap_ops,
	.ch.rx.mtu      = DATA_MTU,
};

/**@brief String literals for the OACP result codes. Used for logging output.*/
static const char * const lit_request[] = {
	"RFU",
	"Create",
	"Delete",
	"Calculate Checksum",
	"Execute",
	"Read",
	"Write",
	"Abort",
};

/**@brief String literals for the OACP result codes. Used for logging output.*/
static const char * const lit_result[] = {
	"RFU",
	"Success",
	"Op Code Not Supported",
	"Invalid Parameter",
	"Insufficient Resources",
	"Invalid Object",
	"Channel Unavailable",
	"Unsupported Type",
	"Procedure Not Permitted",
	"Object Locked",
	"Operation Failed"
};

/**@brief String literals for the OLCP request codes. Used for logging output.*/
static const char * const lit_olcp_request[] = {
	"RFU",
	"FIRST",
	"LAST",
	"PREV",
	"NEXT",
	"GOTO",
	"ORDER",
	"REQ_NUM_OBJS",
	"CLEAR_MARKING",
};

/**@brief String literals for the OLCP result codes. Used for logging output.*/
static const char * const lit_olcp_result[] = {
	"RFU",
	"Success",
	"Op Code Not Supported",
	"Invalid Parameter",
	"Operation Failed",
	"Out of Bonds",
	"Too Many Objects",
	"No Object",
	"Object ID not found",
};

struct bt_otc_internal_instance_t {
	struct bt_otc_instance_t *otc_inst;
	bool busy;
	/** Bitfield that is used to determine how much metadata to read */
	uint8_t metadata_to_read;
	/** Bitfield of how much metadata has been attempted to read */
	uint8_t metadata_read_attempted;
	/** Bitfield of how much metadata has been read */
	uint8_t metadata_read;
	int metadata_err;
	uint32_t rcvd_size;
};

/* The profile clients that uses the OTS are responsible for discovery and
 * will simply register any OTS instances as pointers, which is stored here.
 */
static struct bt_otc_internal_instance_t otc_insts[OTS_CLIENT_INST_COUNT];
NET_BUF_SIMPLE_DEFINE_STATIC(otc_tx_buf, BT_OTC_MAX_WRITE_SIZE);
static struct bt_otc_internal_instance_t *cur_inst;

static int oacp_read(struct bt_conn *conn,
		     struct bt_otc_internal_instance_t *inst);
static void read_next_metadata(struct bt_conn *conn,
			       struct bt_otc_internal_instance_t *inst);
static int read_attr(struct bt_conn *conn,
		    struct bt_otc_internal_instance_t *inst,
		    uint16_t handle, bt_gatt_read_func_t cb);

static void print_oacp_response(enum bt_ots_oacp_proc_type req_opcode,
				enum bt_ots_oacp_res_code result_code)
{
	BT_DBG("Request OP Code: %s", log_strdup(lit_request[req_opcode]));
	BT_DBG("Result Code    : %s", log_strdup(lit_result[result_code]));
}

static void print_olcp_response(enum bt_ots_olcp_proc_type req_opcode,
				enum bt_ots_olcp_res_code result_code)
{
	BT_DBG("Request OP Code: %s",
	       log_strdup(lit_olcp_request[req_opcode]));
	BT_DBG("Result Code    : %s",
	       log_strdup(lit_olcp_result[result_code]));
}

static void date_time_decode(struct net_buf_simple *buf,
			     struct bt_date_time *p_date_time)
{
	p_date_time->year = net_buf_simple_pull_le16(buf);
	p_date_time->month = net_buf_simple_pull_u8(buf);
	p_date_time->day = net_buf_simple_pull_u8(buf);
	p_date_time->hours = net_buf_simple_pull_u8(buf);
	p_date_time->minutes = net_buf_simple_pull_u8(buf);
	p_date_time->seconds = net_buf_simple_pull_u8(buf);
}

static struct bt_otc_internal_instance_t *lookup_inst_by_handle(uint16_t handle)
{
	for (int i = 0; i < ARRAY_SIZE(otc_insts); i++) {
		if (otc_insts[i].otc_inst &&
		    otc_insts[i].otc_inst->start_handle <= handle &&
		    otc_insts[i].otc_inst->end_handle >= handle) {
			return &otc_insts[i];
		}
	}

	BT_DBG("Could not find OTS instance with handle 0x%04x", handle);

	return NULL;
}

static void on_object_selected(struct bt_conn *conn,
			       enum bt_ots_olcp_res_code res,
			       struct bt_otc_instance_t *otc_inst)
{
	memset(&otc_inst->cur_object, 0, sizeof(otc_inst->cur_object));
	otc_inst->cur_object.id = BT_OTC_UNKNOWN_ID;

	if (otc_inst->cb->obj_selected) {
		otc_inst->cb->obj_selected(conn, res, otc_inst);
	}

	BT_DBG("Object selected");
}

static void olcp_ind_handler(struct bt_conn *conn,
			     struct bt_otc_instance_t *otc_inst,
			     const void *data, uint16_t length)
{
	enum bt_ots_olcp_proc_type op_code;
	struct net_buf_simple net_buf;

	net_buf_simple_init_with_data(&net_buf, (void *)data, length);

	op_code = net_buf_simple_pull_u8(&net_buf);

	BT_DBG("OLCP indication");

	if (op_code == BT_OTS_OLCP_PROC_RESP) {
		enum bt_ots_olcp_proc_type req_opcode =
			net_buf_simple_pull_u8(&net_buf);
		enum bt_ots_olcp_res_code result_code =
			net_buf_simple_pull_u8(&net_buf);

		print_olcp_response(req_opcode, result_code);

		switch (req_opcode) {
		case BT_OTS_OLCP_PROC_FIRST:
			BT_DBG("First");
			on_object_selected(conn, result_code, otc_inst);
			break;
		case BT_OTS_OLCP_PROC_LAST:
			BT_DBG("Last");
			on_object_selected(conn, result_code, otc_inst);
			break;
		case BT_OTS_OLCP_PROC_PREV:
			BT_DBG("Previous");
			on_object_selected(conn, result_code, otc_inst);
			break;
		case BT_OTS_OLCP_PROC_NEXT:
			BT_DBG("Next");
			on_object_selected(conn, result_code, otc_inst);
			break;
		case BT_OTS_OLCP_PROC_GOTO:
			BT_DBG("Goto");
			on_object_selected(conn, result_code, otc_inst);
			break;
		case BT_OTS_OLCP_PROC_ORDER:
			BT_DBG("Order");
			on_object_selected(conn, result_code, otc_inst);
			break;
		case BT_OTS_OLCP_PROC_REQ_NUM_OBJS:
			BT_DBG("Request number of objects");
			if (net_buf.len == sizeof(uint32_t)) {
				uint32_t obj_cnt =
					net_buf_simple_pull_le32(&net_buf);
				BT_DBG("Number of objects %u", obj_cnt);
			}
			break;
		case BT_OTS_OLCP_PROC_CLEAR_MARKING:
			BT_DBG("Clear marking");
			break;
		default:
			BT_DBG("Invalid indication req opcode %u", req_opcode);
			break;
		}
	} else {
		BT_DBG("Invalid indication opcode %u", op_code);
	}
}

static void oacp_ind_handler(struct bt_conn *conn,
			     struct bt_otc_instance_t *otc_inst,
			     const void *data, uint16_t length)
{
	enum bt_ots_oacp_proc_type op_code;
	struct net_buf_simple net_buf;

	net_buf_simple_init_with_data(&net_buf, (void *)data, length);

	op_code = net_buf_simple_pull_u8(&net_buf);

	BT_DBG("OACP indication");

	if (op_code == BT_OTS_OACP_PROC_RESP) {
		enum bt_ots_oacp_proc_type req_opcode  =
			net_buf_simple_pull_u8(&net_buf);
		enum bt_ots_oacp_res_code  result_code =
			net_buf_simple_pull_u8(&net_buf);
		print_oacp_response(req_opcode, result_code);
	} else {
		BT_DBG("Invalid indication opcode %u", op_code);
	}
}

uint8_t bt_otc_indicate_handler(struct bt_conn *conn,
				struct bt_gatt_subscribe_params *params,
				const void *data, uint16_t length)
{
	uint16_t handle = params->value_handle;
	struct bt_otc_internal_instance_t *inst =
		lookup_inst_by_handle(handle);

	/* TODO: Can we somehow avoid exposing this
	 * callback via the public API?
	 */

	if (!inst) {
		BT_ERR("Instance not found");
		return BT_GATT_ITER_STOP;
	}

	inst->busy = false;

	if (data) {
		if (handle == inst->otc_inst->olcp_handle) {
			olcp_ind_handler(conn, inst->otc_inst, data, length);
		} else if (handle == inst->otc_inst->oacp_handle) {
			oacp_ind_handler(conn, inst->otc_inst, data, length);
		}
	}
	return BT_GATT_ITER_CONTINUE;
}

static uint8_t read_feature_cb(struct bt_conn *conn, uint8_t err,
			       struct bt_gatt_read_params *params,
			       const void *data, uint16_t length)
{
	uint8_t cb_err = err;
	struct bt_otc_internal_instance_t *inst =
		lookup_inst_by_handle(params->single.handle);
	struct net_buf_simple net_buf;

	net_buf_simple_init_with_data(&net_buf, (void *)data, length);

	if (!inst) {
		BT_ERR("Instance not found");
		return BT_GATT_ITER_STOP;
	}

	inst->busy = false;

	if (err) {
		BT_DBG("err: 0x%02X", err);
	} else if (data) {
		if (length == OTS_FEATURE_LEN) {
			inst->otc_inst->features.oacp =
				net_buf_simple_pull_le32(&net_buf);

			inst->otc_inst->features.olcp =
				net_buf_simple_pull_le32(&net_buf);

			BT_DBG("features : oacp 0x%x, olcp 0x%x",
			       inst->otc_inst->features.oacp,
			       inst->otc_inst->features.olcp);
		} else {
			BT_DBG("Invalid length %u (expected %u)",
			       length, OTS_FEATURE_LEN);
			cb_err = BT_ATT_ERR_INVALID_ATTRIBUTE_LEN;
		}
	}

	return BT_GATT_ITER_STOP;
}

int bt_otc_register(struct bt_otc_instance_t *otc_inst)
{
	static bool l2cap_registered;

	for (int i = 0; i < ARRAY_SIZE(otc_insts); i++) {

		if (otc_insts[i].otc_inst) {
			continue;
		}

		BT_DBG("%u", i);

		if (!l2cap_registered) {
			int err = bt_l2cap_server_register(&ots_l2cap_coc);

			if (err < 0 && err != -EADDRINUSE) {
				BT_ERR("Unable to register OTS_psm (0x%4x)",
				ots_l2cap_coc.psm);
				return -ENOEXEC;
			}

			BT_DBG("L2CAP psm 0x%4x sec_level %u registered",
			ots_l2cap_coc.psm, ots_l2cap_coc.sec_level);

			l2cap_registered = true;
		}

		otc_insts[i].otc_inst = otc_inst;

		return 0;
	}
	return -ENOMEM;
}

int bt_otc_unregister(uint8_t index)
{
	if (index < ARRAY_SIZE(otc_insts)) {
		memset(&otc_insts[index], 0, sizeof(otc_insts[index]));
	} else {
		return -EINVAL;
	}

	return 0;
}

int bt_otc_read_feature(struct bt_conn *conn,
			struct bt_otc_instance_t *otc_inst)
{
	if (OTS_CLIENT_INST_COUNT > 0) {
		struct bt_otc_internal_instance_t *inst;
		int err;

		if (!conn) {
			BT_WARN("Invalid Connection");
			return -ENOTCONN;
		} else if (!otc_inst) {
			BT_ERR("Invalid OTC instance");
			return -EINVAL;
		} else if (!otc_inst->feature_handle) {
			BT_DBG("Handle not set");
			return -EINVAL;
		}

		inst = lookup_inst_by_handle(otc_inst->start_handle);

		if (!inst) {
			BT_ERR("Invalid OTC instance");
			return -EINVAL;
		} else if (inst->busy) {
			return -EBUSY;
		}

		otc_inst->read_proc.func = read_feature_cb;
		otc_inst->read_proc.handle_count = 1;
		otc_inst->read_proc.single.handle = otc_inst->feature_handle;
		otc_inst->read_proc.single.offset = 0U;

		err = bt_gatt_read(conn, &otc_inst->read_proc);
		if (!err) {
			inst->busy = true;
		}
		return err;
	}

	BT_DBG("Not supported");
	return -EOPNOTSUPP;
}

static void write_olcp_cb(struct bt_conn *conn, uint8_t err,
			  struct bt_gatt_write_params *params)
{
	struct bt_otc_internal_instance_t *inst =
		lookup_inst_by_handle(params->handle);

	BT_DBG("Write %s (0x%02X)", err ? "failed" : "successful", err);

	if (!inst) {
		BT_ERR("Instance not found");
		return;
	}
	inst->busy = false;
}

static int write_olcp(struct bt_otc_internal_instance_t *inst,
		      struct bt_conn *conn, enum bt_ots_olcp_proc_type opcode,
		      uint8_t *params, uint8_t param_len)
{
	int err;

	net_buf_simple_reset(&otc_tx_buf);

	net_buf_simple_add_u8(&otc_tx_buf, opcode);

	if (param_len && params) {
		net_buf_simple_add_mem(&otc_tx_buf, params, param_len);
	}

	inst->otc_inst->write_params.offset = 0;
	inst->otc_inst->write_params.data = otc_tx_buf.data;
	inst->otc_inst->write_params.length = otc_tx_buf.len;
	inst->otc_inst->write_params.handle = inst->otc_inst->olcp_handle;
	inst->otc_inst->write_params.func = write_olcp_cb;

	err = bt_gatt_write(conn, &inst->otc_inst->write_params);

	if (!err) {
		inst->busy = true;
	}
	return err;
}

int bt_otc_select_id(struct bt_conn *conn, struct bt_otc_instance_t *otc_inst,
		     uint64_t obj_id)
{
	if (OTS_CLIENT_INST_COUNT > 0) {
		struct bt_otc_internal_instance_t *inst;
		uint8_t param[BT_OTS_OBJ_ID_SIZE];

		if (!conn) {
			BT_WARN("Invalid Connection");
			return -ENOTCONN;
		} else if (!otc_inst) {
			BT_ERR("Invalid OTC instance");
			return -EINVAL;
		} else if (!otc_inst->olcp_handle) {
			BT_DBG("Handle not set");
			return -EINVAL;
		}

		inst = lookup_inst_by_handle(otc_inst->start_handle);

		if (!inst) {
			BT_ERR("Invalid OTC instance");
			return -EINVAL;
		} else if (inst->busy) {
			return -EBUSY;
		}

		/* TODO: Should not update this before ID is read */
		otc_inst->cur_object.id = obj_id;
		sys_put_le48(obj_id, param);

		return write_olcp(inst, conn, BT_OTS_OLCP_PROC_GOTO,
				  param, BT_OTS_OBJ_ID_SIZE);
	}

	BT_DBG("Not supported");
	return -EOPNOTSUPP;
}

int bt_otc_select_first(struct bt_conn *conn,
			struct bt_otc_instance_t *otc_inst)
{
	if (OTS_CLIENT_INST_COUNT > 0) {
		struct bt_otc_internal_instance_t *inst;

		if (!conn) {
			BT_WARN("Invalid Connection");
			return -ENOTCONN;
		} else if (!otc_inst) {
			BT_ERR("Invalid OTC instance");
			return -EINVAL;
		} else if (!otc_inst->olcp_handle) {
			BT_DBG("Handle not set");
			return -EINVAL;
		}

		inst = lookup_inst_by_handle(otc_inst->start_handle);

		if (!inst) {
			BT_ERR("Invalid OTC instance");
			return -EINVAL;
		} else if (inst->busy) {
			return -EBUSY;
		}

		return write_olcp(inst, conn, BT_OTS_OLCP_PROC_FIRST,
				  NULL, 0);
	}

	BT_DBG("Not supported");
	return -EOPNOTSUPP;
}

int bt_otc_select_last(struct bt_conn *conn, struct bt_otc_instance_t *otc_inst)
{
	if (OTS_CLIENT_INST_COUNT > 0) {
		struct bt_otc_internal_instance_t *inst;

		if (!conn) {
			BT_WARN("Invalid Connection");
			return -ENOTCONN;
		} else if (!otc_inst) {
			BT_ERR("Invalid OTC instance");
			return -EINVAL;
		} else if (!otc_inst->olcp_handle) {
			BT_DBG("Handle not set");
			return -EINVAL;
		}

		inst = lookup_inst_by_handle(otc_inst->start_handle);

		if (!inst) {
			BT_ERR("Invalid OTC instance");
			return -EINVAL;
		} else if (inst->busy) {
			return -EBUSY;
		}

		return write_olcp(inst, conn, BT_OTS_OLCP_PROC_LAST,
				  NULL, 0);

	}

	BT_DBG("Not supported");
	return -EOPNOTSUPP;
}

int bt_otc_select_next(struct bt_conn *conn, struct bt_otc_instance_t *otc_inst)
{
	if (OTS_CLIENT_INST_COUNT > 0) {
		struct bt_otc_internal_instance_t *inst;

		if (!conn) {
			BT_WARN("Invalid Connection");
			return -ENOTCONN;
		} else if (!otc_inst) {
			BT_ERR("Invalid OTC instance");
			return -EINVAL;
		} else if (!otc_inst->olcp_handle) {
			BT_DBG("Handle not set");
			return -EINVAL;
		}

		inst = lookup_inst_by_handle(otc_inst->start_handle);

		if (!inst) {
			BT_ERR("Invalid OTC instance");
			return -EINVAL;
		} else if (inst->busy) {
			return -EBUSY;
		}

		return write_olcp(inst, conn, BT_OTS_OLCP_PROC_NEXT,
				  NULL, 0);
	}

	BT_DBG("Not supported");
	return -EOPNOTSUPP;
}

int bt_otc_select_prev(struct bt_conn *conn, struct bt_otc_instance_t *otc_inst)
{
	if (OTS_CLIENT_INST_COUNT > 0) {
		struct bt_otc_internal_instance_t *inst;

		if (!conn) {
			BT_WARN("Invalid Connection");
			return -ENOTCONN;
		} else if (!otc_inst) {
			BT_ERR("Invalid OTC instance");
			return -EINVAL;
		} else if (!otc_inst->olcp_handle) {
			BT_DBG("Handle not set");
			return -EINVAL;
		}

		inst = lookup_inst_by_handle(otc_inst->start_handle);

		if (!inst) {
			BT_ERR("Invalid OTC instance");
			return -EINVAL;
		} else if (inst->busy) {
			return -EBUSY;
		}

		return write_olcp(inst, conn, BT_OTS_OLCP_PROC_PREV,
				  NULL, 0);
	}

	BT_DBG("Not supported");
	return -EOPNOTSUPP;
}

static uint8_t read_object_size_cb(struct bt_conn *conn, uint8_t err,
				   struct bt_gatt_read_params *params,
				   const void *data, uint16_t length)
{
	struct bt_otc_internal_instance_t *inst =
		lookup_inst_by_handle(params->single.handle);
	struct net_buf_simple net_buf;

	net_buf_simple_init_with_data(&net_buf, (void *)data, length);

	BT_DBG("handle %d, length %u", params->single.handle, length);

	if (!inst) {
		BT_ERR("Instance not found");
		return BT_GATT_ITER_STOP;
	}

	if (err) {
		BT_DBG("err: 0x%02X", err);
	} else if (data) {
		if (length != OTS_SIZE_LEN) {
			BT_DBG("Invalid length %u (expected %u)",
			       length, OTS_SIZE_LEN);
			err = BT_ATT_ERR_INVALID_ATTRIBUTE_LEN;
		} else {
			struct bt_otc_obj_metadata *cur_object =
				&inst->otc_inst->cur_object;
			cur_object->current_size =
				net_buf_simple_pull_le32(&net_buf);
			cur_object->alloc_size =
				net_buf_simple_pull_le32(&net_buf);

			BT_DBG("Object Size : current size %u, "
			       "allocated size %u",
			       cur_object->current_size,
			       cur_object->alloc_size);

			if (cur_object->current_size == 0) {
				BT_WARN("Obj size read returned a current "
					"size of 0");
			} else if (cur_object->current_size >
					cur_object->alloc_size &&
				   cur_object->alloc_size != 0) {
				BT_WARN("Allocated size %u is smaller than "
					"current size %u",
					cur_object->alloc_size,
					cur_object->current_size);
			}

			BT_OTC_SET_METADATA_REQ_SIZE(inst->metadata_read, true);
		}
	}

	if (err) {
		BT_WARN("err: 0x%02X", err);
		if (!inst->metadata_err) {
			inst->metadata_err = err;
		}
	}

	read_next_metadata(conn, inst);

	return BT_GATT_ITER_STOP;
}

static uint8_t read_obj_id_cb(struct bt_conn *conn, uint8_t err,
			      struct bt_gatt_read_params *params,
			      const void *data, uint16_t length)
{
	struct bt_otc_internal_instance_t *inst =
		lookup_inst_by_handle(params->single.handle);
	struct net_buf_simple net_buf;

	net_buf_simple_init_with_data(&net_buf, (void *)data, length);

	BT_DBG("handle %d, length %u", params->single.handle, length);

	if (!inst) {
		BT_ERR("Instance not found");
		return BT_GATT_ITER_STOP;
	}

	if (err) {
		BT_DBG("err: 0x%02X", err);
	} else if (data) {
		if (length == BT_OTS_ID_LEN) {
			uint64_t obj_id = net_buf_simple_pull_le48(&net_buf);
			char t[BT_OTS_OBJ_ID_STR_LEN];
			struct bt_otc_obj_metadata *cur_object =
				&inst->otc_inst->cur_object;

			(void)bt_ots_obj_id_to_str(obj_id, t, sizeof(t));
			BT_DBG("Object Id : %s", log_strdup(t));

			if (cur_object->id != BT_OTC_UNKNOWN_ID &&
			    cur_object->id != obj_id) {
				char str[BT_OTS_OBJ_ID_STR_LEN];

				(void)bt_ots_obj_id_to_str(cur_object->id, str,
							   sizeof(str));
				BT_INFO("Read Obj Id %s not selected obj Id %s",
					log_strdup(t), log_strdup(str));
			} else {
				BT_INFO("Read Obj Id confirmed correct Obj Id");
				cur_object->id = obj_id;

				BT_OTC_SET_METADATA_REQ_ID(inst->metadata_read,
							   true);
			}
		} else {
			BT_DBG("Invalid length %u (expected %u)",
			       length, BT_OTS_ID_LEN);
			err = BT_ATT_ERR_INVALID_ATTRIBUTE_LEN;
		}
	}

	if (err) {
		BT_WARN("err: 0x%02X", err);
		if (!inst->metadata_err) {
			inst->metadata_err = err;
		}
	}

	read_next_metadata(conn, inst);

	return BT_GATT_ITER_STOP;
}

static uint8_t read_obj_name_cb(struct bt_conn *conn, uint8_t err,
				struct bt_gatt_read_params *params,
				const void *data, uint16_t length)
{
	struct bt_otc_internal_instance_t *inst =
		lookup_inst_by_handle(params->single.handle);

	BT_DBG("handle %d, length %u", params->single.handle, length);

	if (!inst) {
		BT_ERR("Instance not found");
		return BT_GATT_ITER_STOP;
	}

	if (data) {
		if (length <= BT_OTS_NAME_MAX_SIZE) {
			memcpy(inst->otc_inst->cur_object.name, data, length);
			inst->otc_inst->cur_object.name[length] = '\0';
		} else {
			BT_WARN("Invalid length %u (expected max %u)",
				length, BT_OTS_NAME_MAX_SIZE);
			err = BT_ATT_ERR_INVALID_ATTRIBUTE_LEN;
		}
	}

	if (err) {
		BT_WARN("err: 0x%02X", err);
		if (!inst->metadata_err) {
			inst->metadata_err = err;
		}
	}

	read_next_metadata(conn, inst);

	return BT_GATT_ITER_STOP;
}

static uint8_t read_obj_type_cb(struct bt_conn *conn, uint8_t err,
				struct bt_gatt_read_params *params,
				const void *data, uint16_t length)
{
	struct bt_otc_internal_instance_t *inst =
		lookup_inst_by_handle(params->single.handle);

	BT_DBG("handle %d, length %u", params->single.handle, length);

	if (!inst) {
		BT_ERR("Instance not found");
		return BT_GATT_ITER_STOP;
	}

	if (data) {
		if (length == BT_UUID_SIZE_128 || length == BT_UUID_SIZE_16) {
			char uuid_str[BT_UUID_STR_LEN];
			struct bt_uuid *uuid =
				&inst->otc_inst->cur_object.type_uuid.uuid;

			bt_uuid_create(uuid, data, length);

			bt_uuid_to_str(uuid, uuid_str, sizeof(uuid_str));
			BT_DBG("UUID type read: %s", log_strdup(uuid_str));

			BT_OTC_SET_METADATA_REQ_TYPE(inst->metadata_read, true);
		} else {
			BT_WARN("Invalid length %u (expected max %u)",
				length, OTS_TYPE_MAX_LEN);
			err = BT_ATT_ERR_INVALID_ATTRIBUTE_LEN;
		}
	}

	if (err) {
		BT_WARN("err: 0x%02X", err);
		if (!inst->metadata_err) {
			inst->metadata_err = err;
		}
	}

	read_next_metadata(conn, inst);

	return BT_GATT_ITER_STOP;
}


static uint8_t read_obj_created_cb(struct bt_conn *conn, uint8_t err,
				   struct bt_gatt_read_params *params,
				   const void *data, uint16_t length)
{
	struct bt_otc_internal_instance_t *inst =
		lookup_inst_by_handle(params->single.handle);
	struct net_buf_simple net_buf;

	net_buf_simple_init_with_data(&net_buf, (void *)data, length);

	BT_DBG("handle %d, length %u", params->single.handle, length);

	if (!inst) {
		BT_ERR("Instance not found");
		return BT_GATT_ITER_STOP;
	}

	if (data) {
		if (length <= DATE_TIME_FIELD_SIZE) {
			date_time_decode(
				&net_buf,
				&inst->otc_inst->cur_object.first_created);
		} else {
			BT_WARN("Invalid length %u (expected max %u)",
				length, DATE_TIME_FIELD_SIZE);
			err = BT_ATT_ERR_INVALID_ATTRIBUTE_LEN;
		}
	}

	if (err) {
		BT_WARN("err: 0x%02X", err);
		if (!inst->metadata_err) {
			inst->metadata_err = err;
		}
	}

	read_next_metadata(conn, inst);

	return BT_GATT_ITER_STOP;
}

static uint8_t read_obj_modified_cb(struct bt_conn *conn, uint8_t err,
				    struct bt_gatt_read_params *params,
				    const void *data, uint16_t length)
{
	struct bt_otc_internal_instance_t *inst =
		lookup_inst_by_handle(params->single.handle);
	struct net_buf_simple net_buf;

	net_buf_simple_init_with_data(&net_buf, (void *)data, length);

	BT_DBG("handle %d, length %u", params->single.handle, length);

	if (!inst) {
		BT_ERR("Instance not found");
		return BT_GATT_ITER_STOP;
	}

	if (data) {
		if (length <= DATE_TIME_FIELD_SIZE) {
			date_time_decode(&net_buf,
					 &inst->otc_inst->cur_object.modified);
		} else {
			BT_WARN("Invalid length %u (expected max %u)",
				length, DATE_TIME_FIELD_SIZE);
			err = BT_ATT_ERR_INVALID_ATTRIBUTE_LEN;
		}
	}

	if (err) {
		BT_WARN("err: 0x%02X", err);
		if (!inst->metadata_err) {
			inst->metadata_err = err;
		}
	}

	read_next_metadata(conn, inst);

	return BT_GATT_ITER_STOP;
}

static int read_attr(struct bt_conn *conn,
		    struct bt_otc_internal_instance_t *inst,
		    uint16_t handle, bt_gatt_read_func_t cb)
{
	if (!handle) {
		BT_DBG("Handle not set");
		return -EINVAL;
	} else if (cb == NULL) {
		BT_ERR("No callback set");
		return -EINVAL;
	}

	/* TODO: With EATT we can request multiple metadata to be read at once*/
	inst->otc_inst->read_proc.func = cb;
	inst->otc_inst->read_proc.handle_count = 1;
	inst->otc_inst->read_proc.single.handle = handle;
	inst->otc_inst->read_proc.single.offset = 0;

	return bt_gatt_read(conn, &inst->otc_inst->read_proc);
}

static uint8_t read_obj_properties_cb(struct bt_conn *conn, uint8_t err,
				      struct bt_gatt_read_params *params,
				      const void *data, uint16_t length)
{
	uint8_t cb_err = err;
	struct bt_otc_internal_instance_t *inst =
		lookup_inst_by_handle(params->single.handle);
	struct net_buf_simple net_buf;

	net_buf_simple_init_with_data(&net_buf, (void *)data, length);

	BT_INFO("handle %d, length %u", params->single.handle, length);

	if (!inst) {
		BT_ERR("Instance not found");
		return BT_GATT_ITER_STOP;
	}

	if (err) {
		BT_WARN("err: 0x%02X", err);
	} else if (data && length <= OTS_PROPERTIES_LEN) {
		struct bt_otc_obj_metadata *cur_object =
			&inst->otc_inst->cur_object;

		cur_object->properties = net_buf_simple_pull_le32(&net_buf);

		BT_INFO("Object properties (raw) : 0x%x",
			cur_object->properties);

		if (!BT_OTS_OBJ_GET_PROP_READ(cur_object->properties)) {
			BT_WARN("Obj properties: Obj read not supported");
		}

		BT_OTC_SET_METADATA_REQ_PROPS(inst->metadata_read, true);
	} else {
		BT_WARN("Invalid length %u (expected %u)",
			length, OTS_PROPERTIES_LEN);
		cb_err = BT_ATT_ERR_INVALID_ATTRIBUTE_LEN;
	}

	if (err) {
		BT_WARN("err: 0x%02X", err);
		if (!inst->metadata_err) {
			inst->metadata_err = err;
		}
	}

	read_next_metadata(conn, inst);

	return BT_GATT_ITER_STOP;
}

static void write_oacp_cp_cb(struct bt_conn *conn, uint8_t err,
			     struct bt_gatt_write_params *params)
{
	struct bt_otc_internal_instance_t *inst =
		lookup_inst_by_handle(params->handle);

	BT_DBG("Write %s (0x%02X)", err ? "failed" : "successful", err);

	if (!inst) {
		BT_ERR("Instance not found");
		return;
	}

	inst->busy = false;
}


static int bt_otc_connect(struct bt_conn *conn)
{
	int err;

	if (!conn) {
		BT_WARN("Invalid Connection");
		return -ENOTCONN;
	} else if (l2ch_chan.ch.chan.conn) {
		BT_INFO("Channel already in use");
		return -ENOEXEC;
	}

	BT_INFO("Connecting L2CAP Connection Oriented Channel .........");
	err = bt_l2cap_chan_connect(conn, &l2ch_chan.ch.chan, OTS_L2CAP_PSM);

	if (err < 0) {
		BT_WARN("Unable to connect to psm %u (err %u)",
				OTS_L2CAP_PSM, err);
	} else {
		BT_INFO("L2CAP connection pending");
	}

	return err;
}

static int oacp_read(struct bt_conn *conn,
		     struct bt_otc_internal_instance_t *inst)
{
	int err;
	uint32_t offset = 0;
	uint32_t length;

	if (!inst->otc_inst->oacp_handle) {
		BT_DBG("Handle not set");
		return -EINVAL;
	} else if (inst->busy) {
		return -EBUSY;
	} else if (cur_inst) {
		return -EBUSY;
	}

	/* TODO: How do we ensure that the L2CAP is connected in time for the
	 * transfer?
	 */
	err = bt_otc_connect(conn);

	if (err && err != -ENOEXEC) {
		return -ENOTCONN;
	}

	net_buf_simple_reset(&otc_tx_buf);

	/* OP Code */
	net_buf_simple_add_u8(&otc_tx_buf, BT_OTS_OACP_PROC_READ);

	/* Offset */
	net_buf_simple_add_le32(&otc_tx_buf, offset);

	/* Len */
	length = inst->otc_inst->cur_object.current_size - offset;
	net_buf_simple_add_le32(&otc_tx_buf, length);

	inst->otc_inst->write_params.offset = 0;
	inst->otc_inst->write_params.data = otc_tx_buf.data;
	inst->otc_inst->write_params.length = otc_tx_buf.len;
	inst->otc_inst->write_params.handle = inst->otc_inst->oacp_handle;
	inst->otc_inst->write_params.func = write_oacp_cp_cb;

	err = bt_gatt_write(conn, &inst->otc_inst->write_params);

	if (!err) {
		inst->busy = true;
	}

	cur_inst = inst;
	inst->rcvd_size = 0;

	return err;
}

int bt_otc_read(struct bt_conn *conn, struct bt_otc_instance_t *otc_inst)
{
	struct bt_otc_internal_instance_t *inst;

	if (!conn) {
		BT_WARN("Invalid Connection");
		return -ENOTCONN;
	} else if (!otc_inst) {
		BT_ERR("Invalid OTC instance");
		return -EINVAL;
	}

	inst = lookup_inst_by_handle(otc_inst->start_handle);

	if (!inst) {
		BT_ERR("Invalid OTC instance");
		return -EINVAL;
	}

	if (otc_inst->cur_object.current_size == 0) {
		BT_WARN("Unknown object size");
		return -EINVAL;
	}

	return oacp_read(conn, inst);
}

static void read_next_metadata(struct bt_conn *conn,
			       struct bt_otc_internal_instance_t *inst)
{
	uint8_t metadata_remaining =
		inst->metadata_to_read ^ inst->metadata_read_attempted;
	int err = 0;

	BT_DBG("Attempting to read metadata 0x%02X", metadata_remaining);

	if (BT_OTC_GET_METADATA_REQ_NAME(metadata_remaining)) {
		BT_OTC_SET_METADATA_REQ_NAME(inst->metadata_read_attempted,
					     true);
		err = read_attr(conn, inst, inst->otc_inst->obj_name_handle,
				read_obj_name_cb);
	} else if (BT_OTC_GET_METADATA_REQ_TYPE(metadata_remaining)) {
		BT_OTC_SET_METADATA_REQ_TYPE(inst->metadata_read_attempted,
					     true);
		err = read_attr(conn, inst, inst->otc_inst->obj_type_handle,
				read_obj_type_cb);
	} else if (BT_OTC_GET_METADATA_REQ_SIZE(metadata_remaining)) {
		BT_OTC_SET_METADATA_REQ_SIZE(inst->metadata_read_attempted,
					     true);
		err = read_attr(conn, inst, inst->otc_inst->obj_size_handle,
				read_object_size_cb);
	} else if (BT_OTC_GET_METADATA_REQ_CREATED(metadata_remaining)) {
		BT_OTC_SET_METADATA_REQ_CREATED(inst->metadata_read_attempted,
						true);
		err = read_attr(conn, inst, inst->otc_inst->obj_created_handle,
				read_obj_created_cb);
	} else if (BT_OTC_GET_METADATA_REQ_MODIFIED(metadata_remaining)) {
		BT_OTC_SET_METADATA_REQ_MODIFIED(inst->metadata_read_attempted,
						 true);
		err = read_attr(conn, inst, inst->otc_inst->obj_modified_handle,
				read_obj_modified_cb);
	} else if (BT_OTC_GET_METADATA_REQ_ID(metadata_remaining)) {
		BT_OTC_SET_METADATA_REQ_ID(inst->metadata_read_attempted, true);
		err = read_attr(conn, inst, inst->otc_inst->obj_id_handle,
				read_obj_id_cb);
	} else if (BT_OTC_GET_METADATA_REQ_PROPS(metadata_remaining)) {
		BT_OTC_SET_METADATA_REQ_PROPS(inst->metadata_read_attempted,
					      true);
		err = read_attr(conn, inst,
				inst->otc_inst->obj_properties_handle,
				read_obj_properties_cb);
	} else {
		inst->busy = false;
		if (inst->otc_inst->cb->metadata_cb) {
			inst->otc_inst->cb->metadata_cb(
				conn, inst->metadata_err, inst->otc_inst,
				inst->metadata_read);
		}
		return;
	}

	if (err) {
		BT_DBG("Metadata read failed (%d), trying next", err);
		read_next_metadata(conn, inst);
	}
}

int bt_otc_obj_metadata_read(struct bt_conn *conn,
			     struct bt_otc_instance_t *otc_inst,
			     uint8_t metadata)
{
	struct bt_otc_internal_instance_t *inst;

	if (!conn) {
		BT_WARN("Invalid Connection");
		return -ENOTCONN;
	} else if (!otc_inst) {
		BT_ERR("Invalid OTC instance");
		return -EINVAL;
	} else if (!metadata) {
		BT_WARN("No metadata to read");
		return -ENOEXEC;
	}

	inst = lookup_inst_by_handle(otc_inst->start_handle);

	if (!inst) {
		BT_ERR("Invalid OTC instance");
		return -EINVAL;
	} else if (inst->busy) {
		return -EBUSY;
	}

	inst->metadata_read = 0;
	inst->metadata_to_read = metadata & BT_OTC_METADATA_REQ_ALL;
	inst->metadata_read_attempted = 0;

	inst->busy = true;
	read_next_metadata(conn, inst);

	return 0;
}

static int decode_record(struct net_buf_simple *buf,
			 struct dirlisting_record_t *rec)
{
	uint16_t start_len = buf->len;

	rec->len = net_buf_simple_pull_le16(buf);

	if (rec->len > buf->len) {
		BT_WARN("incorrect DirListing record length %u, "
			"longer than remaining size %u",
			rec->len, buf->len);
		return -EINVAL;
	}

	if ((start_len - buf->len) + BT_OTS_OBJ_ID_SIZE > rec->len) {
		BT_WARN("incorrect DirListing record, reclen %u too short, "
			"includes only record length",
			rec->len);
		return -EINVAL;
	}

	rec->metadata.id = net_buf_simple_pull_le48(buf);

	if (IS_ENABLED(CONFIG_BT_DEBUG_OTC)) {
		char t[BT_OTS_OBJ_ID_STR_LEN];

		(void)bt_ots_obj_id_to_str(rec->metadata.id, t, sizeof(t));
		BT_DBG("Object ID 0x%s", log_strdup(t));
	}

	if ((start_len - buf->len) + sizeof(uint8_t) > rec->len) {
		BT_WARN("incorrect DirListing record, reclen %u too short, "
			"includes only record length + ObjId",
			rec->len);
		return -EINVAL;
	}

	rec->name_len = net_buf_simple_pull_u8(buf);

	if (rec->name_len > 0) {
		uint8_t *name;

		if ((start_len - buf->len) + rec->name_len > rec->len) {
			BT_WARN("incorrect DirListing record, remaining length "
				"%u shorter than name length %u",
				rec->len - (start_len - buf->len),
				rec->name_len);
			return -EINVAL;
		}

		if (rec->name_len >= sizeof(rec->metadata.name)) {
			BT_WARN("Name length %u too long, invalid record",
				rec->name_len);
			return -EINVAL;
		}

		name = net_buf_simple_pull_mem(buf, rec->name_len);
		memcpy(rec->metadata.name, name, rec->name_len);
	}

	rec->metadata.name[rec->name_len] = '\0';
	rec->flags = 0;

	if ((start_len - buf->len) + sizeof(uint8_t) > rec->len) {
		BT_WARN("incorrect DirListing record, reclen %u too short, "
			"does not include flags", rec->len);
		return -EINVAL;
	}

	rec->flags = net_buf_simple_pull_u8(buf);
	BT_DBG("flags 0x%x", rec->flags);

	if (BT_OTS_DIRLIST_GET_FLAG_TYPE_128(rec->flags)) {
		uint8_t *uuid;

		if ((start_len - buf->len) + BT_UUID_SIZE_128 > rec->len) {
			BT_WARN("incorrect DirListing record, reclen %u "
				"flags indicates uuid128, too short",
				rec->len);
			BT_INFO("flags 0x%x", rec->flags);
			return -EINVAL;
		}

		uuid = net_buf_simple_pull_mem(buf, BT_UUID_SIZE_128);
		bt_uuid_create(&rec->metadata.type_uuid.uuid,
			       uuid, BT_UUID_SIZE_128);
	} else {
		if ((start_len - buf->len) + BT_UUID_SIZE_16 > rec->len) {
			BT_WARN("incorrect DirListing record, reclen %u "
				"flags indicates uuid16, too short",
				rec->len);
			BT_INFO("flags 0x%x", rec->flags);
			return -EINVAL;
		}

		rec->metadata.type_uuid.uuid_16.val =
			net_buf_simple_pull_le16(buf);
	}

	if (BT_OTS_DIRLIST_GET_FLAG_CUR_SIZE(rec->flags)) {
		if ((start_len - buf->len) + sizeof(uint32_t) > rec->len) {
			BT_WARN("incorrect DirListing record, reclen %u "
				"flags indicates cur_size, too short",
				rec->len);
			BT_INFO("flags 0x%x", rec->flags);
			return -EINVAL;
		}

		rec->metadata.current_size = net_buf_simple_pull_le32(buf);
	}

	if (BT_OTS_DIRLIST_GET_FLAG_ALLOC_SIZE(rec->flags)) {
		if ((start_len - buf->len) + sizeof(uint32_t) > rec->len) {
			BT_WARN("incorrect DirListing record, reclen %u "
				"flags indicates alloc_size, too short",
				rec->len);
			BT_INFO("flags 0x%x", rec->flags);
			return -EINVAL;
		}

		rec->metadata.alloc_size = net_buf_simple_pull_le32(buf);
	}

	if (BT_OTS_DIRLIST_GET_FLAG_FIRST_CREATED(rec->flags)) {
		if ((start_len - buf->len) + DATE_TIME_FIELD_SIZE > rec->len) {
			BT_WARN("incorrect DirListing record, reclen %u "
				"too short flags indicates first_created",
				rec->len);
			BT_INFO("flags 0x%x", rec->flags);
			return -EINVAL;
		}

		date_time_decode(buf, &rec->metadata.first_created);
	}

	if (BT_OTS_DIRLIST_GET_FLAG_LAST_MODIFIED(rec->flags)) {
		if ((start_len - buf->len) + DATE_TIME_FIELD_SIZE > rec->len) {
			BT_WARN("incorrect DirListing record, reclen %u "
				"flags indicates las_mod, too short",
				rec->len);
			BT_INFO("flags 0x%x", rec->flags);
			return -EINVAL;
		}

		date_time_decode(buf, &rec->metadata.modified);
	}

	if (BT_OTS_DIRLIST_GET_FLAG_PROPERTIES(rec->flags)) {
		if ((start_len - buf->len) + sizeof(uint32_t) > rec->len) {
			BT_WARN("incorrect DirListing record, reclen %u "
				"flags indicates properties, too short",
				rec->len);
			BT_INFO("flags 0x%x", rec->flags);
			return -EINVAL;
		}

		rec->metadata.properties = net_buf_simple_pull_le32(buf);
	}

	return rec->len;
}

int bt_otc_decode_dirlisting(uint8_t *data, uint16_t length,
			     bt_otc_dirlisting_cb cb)
{
	struct net_buf_simple net_buf;
	uint8_t count = 0;
	struct dirlisting_record_t record;

	net_buf_simple_init_with_data(&net_buf, (void *)data, length);

	if (!data || length == 0) {
		return -EINVAL;
	}

	while (net_buf.len) {
		int ret;

		count++;

		if (net_buf.len < sizeof(uint16_t)) {
			BT_WARN("incorrect DirListing record, len %u too short",
				net_buf.len);
			return -EINVAL;
		}

		BT_DBG("Decoding record %u", count);
		ret = decode_record(&net_buf, &record);

		if (ret < 0) {
			BT_WARN("DirListing, record %u invalid", count);
			return ret;
		}

		ret = cb(&record.metadata);

		if (ret == BT_OTC_STOP) {
			break;
		}
	}

	return count;
}

void bt_otc_metadata_display(struct bt_otc_obj_metadata *metadata,
			     uint16_t count)
{
	BT_INFO("--- Displaying %u metadata records ---", count);

	for (int i = 0; i < count; i++) {
		char t[BT_OTS_OBJ_ID_STR_LEN];

		(void)bt_ots_obj_id_to_str(metadata->id, t, sizeof(t));
		BT_INFO("Object ID: 0x%s", log_strdup(t));
		BT_INFO("Object name: %s", log_strdup(metadata->name));
		BT_INFO("Object Current Size: %u", metadata->current_size);
		BT_INFO("Object Allocate Size: %u", metadata->alloc_size);

		if (!bt_uuid_cmp(&metadata->type_uuid.uuid,
				 BT_UUID_OTS_TYPE_MPL_ICON)) {
			BT_INFO("Type: Icon Obj Type");
		} else if (!bt_uuid_cmp(&metadata->type_uuid.uuid,
					BT_UUID_OTS_TYPE_TRACK_SEGMENT)) {
			BT_INFO("Type: Track Segment Obj Type");
		} else if (!bt_uuid_cmp(&metadata->type_uuid.uuid,
					BT_UUID_OTS_TYPE_TRACK)) {
			BT_INFO("Type: Track Obj Type");
		} else if (!bt_uuid_cmp(&metadata->type_uuid.uuid,
					BT_UUID_OTS_TYPE_GROUP)) {
			BT_INFO("Type: Group Obj Type");
		} else if (!bt_uuid_cmp(&metadata->type_uuid.uuid,
					BT_UUID_OTS_DIRECTORY_LISTING)) {
			BT_INFO("Type: Directory Listing");
		}


		BT_INFO("Properties:0x%x", metadata->properties);

		if (BT_OTS_OBJ_GET_PROP_APPEND(metadata->properties)) {
			BT_INFO(" - append permitted");
		}

		if (BT_OTS_OBJ_GET_PROP_DELETE(metadata->properties)) {
			BT_INFO(" - delete permitted");
		}

		if (BT_OTS_OBJ_GET_PROP_EXECUTE(metadata->properties)) {
			BT_INFO(" - execute permitted");
		}

		if (BT_OTS_OBJ_GET_PROP_MARKED(metadata->properties)) {
			BT_INFO(" - marked");
		}

		if (BT_OTS_OBJ_GET_PROP_PATCH(metadata->properties)) {
			BT_INFO(" - patch permitted");
		}

		if (BT_OTS_OBJ_GET_PROP_READ(metadata->properties)) {
			BT_INFO(" - read permitted");
		}

		if (BT_OTS_OBJ_GET_PROP_TRUNCATE(metadata->properties)) {
			BT_INFO(" - truncate permitted");
		}

		if (BT_OTS_OBJ_GET_PROP_WRITE(metadata->properties)) {
			BT_INFO(" - write permitted");
		}
	}
}

static int l2cap_recv(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	uint32_t offset = cur_inst->rcvd_size;
	bool is_complete = false;
	struct bt_otc_obj_metadata *cur_object =
		&cur_inst->otc_inst->cur_object;
	int cb_ret;

	BT_DBG("Incoming data channel: %p, len: %u", chan, buf->len);
	BT_DBG("Offset: %d", offset);

	cur_inst->rcvd_size += buf->len;

	if (cur_inst->rcvd_size >= cur_object->current_size) {
		is_complete = true;
	}

	if (cur_inst->rcvd_size > cur_object->current_size) {
		BT_WARN("Received %u but expected maximum %u",
			cur_inst->rcvd_size, cur_object->current_size);
	}


	cb_ret = cur_inst->otc_inst->cb->content_cb(chan->conn, offset,
						    buf->len, buf->data,
						    is_complete, 0);

	if (is_complete) {
		uint32_t rcv_size = cur_object->current_size;
		int err;

		BT_DBG("Received the whole object (%u bytes). "
		       "Disconnecting L2CAP CoC", rcv_size);
		err = bt_l2cap_chan_disconnect(&l2ch_chan.ch.chan);

		if (err < 0) {
			BT_WARN("bt_l2cap_chan_disconnect returned error %d",
				err);
		}

		cur_inst = NULL;
	} else if (cb_ret == BT_OTC_STOP) {
		uint32_t rcv_size = cur_object->current_size;
		int err;

		BT_DBG("Stopped receiving after%u bytes. "
		       "Disconnecting L2CAP CoC", rcv_size);
		err = bt_l2cap_chan_disconnect(&l2ch_chan.ch.chan);

		if (err < 0) {
			BT_WARN("bt_l2cap_chan_disconnect returned error %d",
				err);
		}

		cur_inst = NULL;

	}
	return 0;
}

static struct net_buf *l2cap_alloc_buf(struct bt_l2cap_chan *chan)
{
	BT_DBG("L2CAP CoC Buffer alloc requested");
	return net_buf_alloc(&otc_l2cap_pool, K_FOREVER);
}

static int l2cap_accept(struct bt_conn *conn, struct bt_l2cap_chan **chan)
{
	BT_DBG("Incoming conn %p", conn);

	if (l2ch_chan.ch.chan.conn) {
		BT_DBG("No channels available");
		return -ENOMEM;
	}

	*chan = &l2ch_chan.ch.chan;

	return 0;
}

static void l2cap_connected(struct bt_l2cap_chan *chan)
{
	BT_INFO("Channel %p connected", chan);
}

static void l2cap_disconnected(struct bt_l2cap_chan *chan)
{
	BT_INFO("Channel %p disconnected", chan);
}

static void l2cap_sent(struct bt_l2cap_chan *chan)
{
	BT_DBG("Outgoing data ...");
}

static void l2cap_status(struct bt_l2cap_chan *chan, atomic_t *status)
{
	int credits = atomic_get(&l2ch_chan.ch.tx.credits);

	BT_DBG("Channel %p status %lu, credits %d", chan, *status, credits);
}
