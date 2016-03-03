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

/**
 * GATT indication types.
 */
enum BLE_GATT_IND_TYPES {
	BLE_GATT_IND_TYPE_NONE = 0,
	BLE_GATT_IND_TYPE_NOTIFICATION,
	BLE_GATT_IND_TYPE_INDICATION,
};

/** GATT Register structure for one service */
struct nble_gatt_register_req {
	/* Base address of the attribute table in the Quark mem space */
	struct bt_gatt_attr *attr_base;
	/* Number of of attributes in this service */
	uint8_t attr_count;
};

/** GATT Register structure for one service */
struct nble_gatt_register_rsp {
	int status;
	struct bt_gatt_attr *attr_base;
	/* Number of attributes successfully added */
	uint8_t attr_count;
};

/**
 * Write event context data structure.
 */
struct nble_gatt_wr_evt {
	struct bt_gatt_attr *attr;
	uint16_t conn_handle;
	uint16_t attr_handle;
	uint16_t offset;
	/* 1 if reply required, 0 otherwise */
	uint8_t reply;
};

/**
 * Read event context data structure.
 */
struct nble_gatt_rd_evt {
	struct bt_gatt_attr *attr;
	uint16_t conn_handle;
	uint16_t attr_handle;
	uint16_t offset;
};

struct nble_gatts_rw_reply_params {
	int status;
	uint16_t conn_handle;
	uint16_t offset;
	/* 0 if read reply, otherwise write reply */
	uint8_t write_reply;
};

/**
 * Notification/Indication parameters
 */
struct nble_gatt_notif_ind_params {
	struct bt_gatt_attr *attr;
	uint16_t offset;
};

/**
 * Indication or notification.
 */

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

/**
 * Attribute handle range definition.
 */
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

/* GATTC characteristic */
struct nble_gattc_characteristic {
	uint16_t handle;
	uint8_t prop;
	uint16_t value_handle;
	struct bt_uuid_128 uuid;
};

/**
 * GATTC descriptor.
 */
struct nble_gattc_descriptor {
	uint16_t handle;
	struct bt_uuid_128 uuid;
};

struct nble_gattc_discover_rsp {
	int status;
	void *user_data;
	uint16_t conn_handle;
	uint8_t type;
};

struct nble_gatts_svc_changed_params {
	uint16_t conn_handle;
	uint16_t start_handle;
	uint16_t end_handle;
};

/**
 * Send a service change indication.
 *
 * The response to this request is received through @ref
 * on_ble_gatts_send_svc_changed_rsp
 *
 * @note Not yet supported
 *
 * @param par Service parameters
 * @param priv Pointer to private data
 */
void nble_gatts_send_svc_changed_req(const struct nble_gatts_svc_changed_params *par,
				     void *priv);

/** Register a BLE GATT Service.
 *
 * @param par Parameters of attribute data base
 * @param attr Serialized attribute buffer
 * @param attr_len Length of buffer
 */
void nble_gatt_register_req(const struct nble_gatt_register_req *par,
			    uint8_t *buf, uint16_t len);

/**
 * Reply to an authorize request.
 *
 * @param par parameters for the reply
 * @param buf  read value of the attribute
 * @param len length of buf
 */
void nble_gatts_authorize_reply_req(const struct nble_gatts_rw_reply_params *par,
				    uint8_t *buf, uint16_t len);

/**
 * Conversion table entry nble_core to host attr index
 *
 * This is returned as a table on registering.
 */
struct nble_gatt_attr_handles {
	uint16_t handle; /* handle from ble controller should be sufficient */
};

/** Response to registering a BLE GATT Service.
 *
 * The returned buffer contains an array (@ref nble_gatt_attr_idx_entry)with the
 * corresponding handles.
 *
 * @param par Parameters of attribute data base
 * @param attr Returned attributes index list
 * @param len Length of buffer
 */
void on_nble_gatt_register_rsp(const struct nble_gatt_register_rsp *par,
			       const struct nble_gatt_attr_handles *attr,
			       uint8_t len);

/**
 * Function invoked by the BLE core when a write occurs.
 *
 * @param ev Pointer to the event structure
 * @param buf Pointer to data buffer
 * @param len Buffer length
 */
void on_nble_gatts_write_evt(const struct nble_gatt_wr_evt *ev,
			     const uint8_t *buf, uint8_t len);

/**
 * Send notification.
 *
 * The response to this request is received
 *
 * @param par Notification parameters
 * @param data Indication data to write
 * @param length Length of indication - may be 0, in this case already
 * stored data is sent
 */
void nble_gatt_send_notif_req(const struct nble_gatt_send_notif_params *par,
			      uint8_t *data, uint16_t length);

/**
 * Send indication.
 *
 * The response to this request is received
 *
 * @param par Indication parameters
 * @param data Indication data to write
 * @param length Length of indication - may be 0, in this case already
 * stored data is sent
 */
void nble_gatt_send_ind_req(const struct nble_gatt_send_ind_params *par,
			    uint8_t *data, uint8_t length);

/** Discover parameters. */
struct nble_discover_params {
	void *user_data;
	struct bt_uuid_128 uuid;
	struct nble_gatt_handle_range handle_range;
	uint16_t conn_handle;
	uint8_t type;
};

/**
 * Discover service.
 *
 * @param req Request structure.
 */
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
	uint16_t conn_handle;
	uint16_t handle;
	uint16_t offset;
};

struct nble_gattc_read_rsp {
	uint16_t conn_handle;
	int status;
	uint16_t handle;
	uint16_t offset;
};

struct nble_gattc_write_params {
	uint16_t conn_handle;
	uint16_t handle;
	uint16_t offset;
	/* different than 0 if response required */
	uint8_t with_resp;
};

struct nble_gattc_write_rsp {
	uint16_t conn_handle;
	int status;
	uint16_t handle;
	uint16_t len;
};


/**
 * Read characteristic on remote server.
 *
 * @param params Request structure.
 * @param priv Pointer to private data.
 */
void nble_gattc_read_req(const struct nble_gattc_read_params *params,
			 void *priv);

/**
 * Write characteristic on server.
 *
 * @param params Write parameters
 * @param buf Characteristic value to write.
 * @param len Characteristic value length. If length is bigger then ATT MTU
 * size, the controller fragment buffer itself.
 * @param priv Pointer to private data.
 */
void nble_gattc_write_req(const struct nble_gattc_write_params *params,
			  const uint8_t *buf, uint8_t len, void *priv);

struct nble_gattc_value_evt {
	uint16_t conn_handle;
	int status;
	uint16_t handle;
	uint8_t type;
};

void bt_gatt_disconnected(struct bt_conn *conn);
