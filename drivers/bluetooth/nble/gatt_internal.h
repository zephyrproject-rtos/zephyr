/** @file
 *  @brief Internal API for Generic Attribute Profile handling.
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* Max number of service supported, if changed update BLE core needs to be
 * updated too!
 */
#define BLE_GATTS_MAX_SERVICES 10

/*
 * Internal APIs used between host and BLE controller
 * Typically they are required if gatt.h APIs can not be mapped 1:1 onto
 * controller API
 */

enum BLE_GATT_IND_TYPES {
	BLE_GATT_IND_TYPE_NONE = 0,
	BLE_GATT_IND_TYPE_NOTIFICATION,
	BLE_GATT_IND_TYPE_INDICATION,
};

struct nble_gatt_register_req {
	/* Base address of the attribute table in the Quark mem space */
	struct bt_gatt_attr *attr_base;
	/* Number of of attributes in this service */
	uint8_t attr_count;
};

struct nble_gatt_register_rsp {
	int status;
	struct bt_gatt_attr *attr_base;
	/* Number of attributes successfully added */
	uint8_t attr_count;
};

enum nble_gatt_wr_flag {
	NBLE_GATT_WR_FLAG_REPLY	= 1,
	NBLE_GATT_WR_FLAG_PREP	= 2,
};

struct nble_gatt_wr_evt {
	struct bt_gatt_attr *attr;
	uint16_t conn_handle;
	uint16_t offset;
	uint8_t flag;		/* Cf. enum nble_gatt_wr_flag */
};

struct nble_gatt_wr_exec_evt {
	uint16_t conn_handle;
	uint8_t flag;
};

struct nble_gatt_rd_evt {
	struct bt_gatt_attr *attr;
	uint16_t conn_handle;
	uint16_t offset;
};

struct nble_gatts_rd_reply_params {
	uint16_t conn_handle;
	uint16_t offset;
	int32_t status;
};

struct nble_gatts_wr_reply_params {
	uint16_t conn_handle;
	int32_t status;
};

struct nble_gatt_notif_ind_params {
	const struct bt_gatt_attr *attr;
	uint16_t offset;
};

struct nble_gatt_send_notif_params {
	/* Function to be invoked when buffer is freed */
	bt_gatt_notify_func_t cback;
	uint16_t conn_handle;
	struct nble_gatt_notif_ind_params params;
};

struct nble_gatt_notif_rsp {
	bt_gatt_notify_func_t cback;
	int status;
	uint16_t conn_handle;
	struct bt_gatt_attr *attr;
};

struct nble_gatt_send_ind_params {
	/* Function to be invoked when buffer is freed */
	bt_gatt_indicate_func_t cback;
	uint16_t conn_handle;
	struct nble_gatt_notif_ind_params params;
};

struct nble_gatt_ind_rsp {
	bt_gatt_indicate_func_t cback;
	int status;
	uint16_t conn_handle;
	struct bt_gatt_attr *attr;
};

struct nble_gatt_handle_range {
	uint16_t start_handle;
	uint16_t end_handle;
};

struct nble_gattc_primary {
	uint16_t handle;
	struct nble_gatt_handle_range range;
	struct bt_uuid_128 uuid;
};

struct nble_gattc_included {
	uint16_t handle;
	struct nble_gatt_handle_range range;
	struct bt_uuid_128 uuid;
};

struct nble_gattc_characteristic {
	uint16_t handle;
	uint8_t prop;
	uint16_t value_handle;
	struct bt_uuid_128 uuid;
};

struct nble_gattc_descriptor {
	uint16_t handle;
	struct bt_uuid_128 uuid;
};

struct nble_gattc_discover_rsp {
	int32_t status;
	void *user_data;
	uint16_t conn_handle;
	uint8_t type;
};

struct nble_gatts_svc_changed_params {
	uint16_t conn_handle;
	uint16_t start_handle;
	uint16_t end_handle;
};

void nble_gatts_send_svc_changed_req(const struct nble_gatts_svc_changed_params *par,
				     void *priv);

void nble_gatts_rd_reply_req(const struct nble_gatts_rd_reply_params *,
			     uint8_t *, uint16_t);

void nble_gatts_wr_reply_req(const struct nble_gatts_wr_reply_params *p_params);

void nble_gatt_register_req(const struct nble_gatt_register_req *par,
			    uint8_t *buf, uint16_t len);

struct nble_gatt_attr_handles {
	uint16_t handle;
};

void on_nble_gatt_register_rsp(const struct nble_gatt_register_rsp *par,
			       const struct nble_gatt_attr_handles *attr,
			       uint8_t len);

void on_nble_gatts_write_evt(const struct nble_gatt_wr_evt *ev,
			     const uint8_t *buf, uint8_t len);

void nble_gatt_send_notif_req(const struct nble_gatt_send_notif_params *par,
			      const uint8_t *data, uint16_t length);

void nble_gatt_send_ind_req(const struct nble_gatt_send_ind_params *par,
			    const uint8_t *data, uint8_t length);

#define DISCOVER_FLAGS_UUID_PRESENT 1

struct nble_discover_params {
	void *user_data;
	struct bt_uuid_128 uuid;
	struct nble_gatt_handle_range handle_range;
	uint16_t conn_handle;
	uint8_t type;
	uint8_t flags;
};

void nble_gattc_discover_req(const struct nble_discover_params *req);

/** GATT Attribute stream structure.
 *
 * This structure is a "compressed" copy of @ref bt_gatt_attr.
 * UUID pointer and user_data pointer are used as offset into buffer itself.
 * The offset is from the beginning of the buffer. therefore a value of 0
 * means that UUID or user_data is not present. */
struct nble_gatt_attr {
	/** Attribute permissions */
	uint16_t perm;
	/** Attribute variable data size */
	uint16_t data_size;
	/**
	 * Attribute variable data: always starts with the UUID and
	 * data follows
	 */
	uint8_t data[0];
};

struct nble_gattc_read_params {
	void *user_data;
	uint16_t conn_handle;
	uint16_t handle;
	uint16_t offset;
};

struct nble_gattc_read_multiple_params {
	void *user_data;
	uint16_t conn_handle;
};

struct nble_gattc_read_rsp {
	int status;
	void *user_data;
	uint16_t conn_handle;
	uint16_t handle;
	uint16_t offset;
};

/* forward declaration */
struct bt_gatt_write_params;

typedef void (*bt_att_func_t)(struct bt_conn *conn, uint8_t err,
		     const struct bt_gatt_write_params *wr_params);

struct bt_gatt_write_params {
	/* Function invoked upon write response */
	bt_att_func_t func;
	/* User specific data */
	void *user_data[2];
};

struct nble_gattc_write_params {
	uint16_t conn_handle;
	uint16_t handle;
	uint16_t offset;
	/* different than 0 if response required */
	uint8_t with_resp;
	struct bt_gatt_write_params wr_params;
};

struct nble_gattc_write_rsp {
	uint16_t conn_handle;
	int status;
	uint16_t handle;
	uint16_t len;
	struct bt_gatt_write_params wr_params;
};

void nble_gattc_read_req(const struct nble_gattc_read_params *);

void nble_gattc_write_req(const struct nble_gattc_write_params *params,
			  const uint8_t *buf, uint8_t len);

void bt_gatt_disconnected(struct bt_conn *conn);

struct nble_gattc_value_evt {
	uint16_t conn_handle;
	int status;
	uint16_t handle;
	uint8_t type;
};
