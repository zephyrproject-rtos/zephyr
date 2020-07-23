/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BT_GATT_OTS_INTERNAL_H_
#define BT_GATT_OTS_INTERNAL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/types.h>
#include "ots_l2cap_internal.h"
#include "ots_oacp_internal.h"
#include "ots_olcp_internal.h"

/**@brief OTS Attribute Protocol Application Error codes. */
enum bt_gatt_ots_att_err_codes {
	/** An attempt was made to write a value that is invalid or
	 *  not supported by this Server for a reason other than
	 *  the attribute permissions.
	 */
	BT_GATT_OTS_WRITE_REQUEST_REJECTED     = 0x80,
	/** An attempt was made to read or write to an Object Metadata
	 *  characteristic while the Current Object was an Invalid Object.
	 */
	BT_GATT_OTS_OBJECT_NOT_SELECTED        = 0x81,
	/** The Server is unable to service the Read Request or Write Request
	 *  because it exceeds the concurrency limit of the service.
	 */
	BT_GATT_OTS_CONCURRENCY_LIMIT_EXCEEDED = 0x82,
	/** The requested object name was rejected because
	 *  the name was already in use by an existing object on the Server.
	 */
	BT_GATT_OTS_OBJECT_NAME_ALREADY_EXISTS = 0x83,
};

enum bt_gatt_ots_object_state_type {
	BT_GATT_OTS_OBJECT_IDLE_STATE,

	BT_GATT_OTS_OBJECT_READ_OP_STATE,
};

struct bt_gatt_ots_object_state {
	enum bt_gatt_ots_object_state_type type;
	union {
		struct bt_gatt_ots_object_read_op {
			struct bt_gatt_ots_oacp_read_params oacp_params;
			uint32_t sent_len;
		} read_op;
	};
};

struct bt_gatt_ots_object {
	uint64_t id;
	struct bt_ots_obj_metadata metadata;
	struct bt_gatt_ots_object_state state;
};

struct bt_gatt_ots_indicate {
	struct bt_gatt_indicate_params params;
	struct bt_gatt_attr attr;
	struct _bt_gatt_ccc ccc;
	bool is_enabled;
};

struct bt_ots {
	struct bt_ots_feat features;
	struct bt_gatt_ots_object *cur_obj;
	struct bt_gatt_service *service;
	struct bt_gatt_ots_indicate oacp_ind;
	struct bt_gatt_ots_indicate olcp_ind;
	struct bt_gatt_ots_l2cap l2cap;
	struct bt_ots_cb *cb;
	void *obj_manager;
};

#ifdef __cplusplus
}
#endif

#endif /* BT_GATT_OTS_INTERNAL_H_ */
