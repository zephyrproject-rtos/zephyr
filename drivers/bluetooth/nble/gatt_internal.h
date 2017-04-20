/** @file
 *  @brief Internal API for Generic Attribute Profile handling.
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Max number of service supported, if changed update BLE core needs to be
 * updated too!
 */
#define BLE_GATTS_MAX_SERVICES 10

void bt_gatt_init(void);

/*
 * GATT Attribute stream structure.
 *
 * This structure is a "compressed" copy of bt_gatt_attr.
 * UUID pointer and user_data pointer are used as offset into buffer itself.
 * The offset is from the beginning of the buffer. therefore a value of 0
 * means that UUID or user_data is not present.
 */
struct nble_gatts_attr {
	/* Attribute permissions */
	u16_t perm;
	/* Attribute variable data size */
	u16_t data_size;
	/* Attribute variable data: always starts with the UUID and data follows */
	u8_t data[];
};

struct nble_gatts_register_req {
	/* Base address of the attribute table in the Quark mem space */
	struct bt_gatt_attr *attr_base;
	/* Number of of attributes in this service */
	u8_t attr_count;
	/* Size of struct bt_gatt_attr */
	u8_t attr_size;
};

void nble_gatts_register_req(const struct nble_gatts_register_req *req,
			     u8_t *data, u16_t len);

struct nble_gatts_register_rsp {
	int status;
	struct bt_gatt_attr *attr_base;
	/* Number of attributes successfully added */
	u8_t attr_count;
};

struct nble_gatt_attr_handles {
	u16_t handle;
};

void on_nble_gatts_register_rsp(const struct nble_gatts_register_rsp *rsp,
				const struct nble_gatt_attr_handles *attrs,
				u8_t len);

enum nble_gatt_wr_flag {
	NBLE_GATT_WR_FLAG_REPLY	= 1,
	NBLE_GATT_WR_FLAG_PREP	= 2,
};

struct nble_gatts_write_evt {
	struct bt_gatt_attr *attr;
	u16_t conn_handle;
	u16_t offset;
	/* see nble_gatt_wr_flag */
	u8_t flag;
};

void on_nble_gatts_write_evt(const struct nble_gatts_write_evt *evt,
			     const u8_t *data, u8_t len);

struct nble_gatts_write_reply_req {
	u16_t conn_handle;
	u16_t offset;
	s32_t status;
};

void nble_gatts_write_reply_req(const struct nble_gatts_write_reply_req *req,
				const u8_t *data, u8_t len);

struct nble_gatts_write_exec_evt {
	u16_t conn_handle;
	u8_t flag;
};

void on_nble_gatts_write_exec_evt(const struct nble_gatts_write_exec_evt *evt);

struct nble_gatts_read_evt {
	struct bt_gatt_attr *attr;
	u16_t conn_handle;
	u16_t offset;
};

void on_nble_gatts_read_evt(const struct nble_gatts_read_evt *evt);

struct nble_gatts_read_reply_req {
	u16_t conn_handle;
	u16_t offset;
	s32_t status;
};

void nble_gatts_read_reply_req(const struct nble_gatts_read_reply_req *req,
			       u8_t *data, u16_t len);

struct nble_gatts_value_change_param {
	const struct bt_gatt_attr *attr;
	u16_t conn_handle;
	u16_t offset;
};

struct nble_gatts_notify_req {
	/* Function to be invoked when buffer is freed */
	bt_gatt_notify_func_t cback;
	struct nble_gatts_value_change_param params;
};

void nble_gatts_notify_req(const struct nble_gatts_notify_req *req,
			   const u8_t *data, u16_t len);

struct nble_gatts_notify_tx_evt {
	bt_gatt_notify_func_t cback;
	int status;
	u16_t conn_handle;
	struct bt_gatt_attr *attr;
};

void on_nble_gatts_notify_tx_evt(const struct nble_gatts_notify_tx_evt *evt);

struct nble_gatts_indicate_req {
	/* Function to be invoked when buffer is freed */
	bt_gatt_indicate_func_t cback;
	struct nble_gatts_value_change_param params;
};

void nble_gatts_indicate_req(const struct nble_gatts_indicate_req *req,
			     const u8_t *data, u8_t len);

struct nble_gatts_indicate_rsp {
	bt_gatt_indicate_func_t cback;
	struct bt_gatt_attr *attr;
	int status;
	u16_t conn_handle;
};

void on_nble_gatts_indicate_rsp(const struct nble_gatts_indicate_rsp *rsp);

#define DISCOVER_FLAGS_UUID_PRESENT 1

struct nble_gatt_handle_range {
	u16_t start_handle;
	u16_t end_handle;
};

struct nble_gattc_discover_req {
	void *user_data;
	struct bt_uuid_128 uuid;
	struct nble_gatt_handle_range handle_range;
	u16_t conn_handle;
	u8_t type;
	u8_t flags;
};

void nble_gattc_discover_req(const struct nble_gattc_discover_req *req);

struct nble_gattc_primary {
	u16_t handle;
	struct nble_gatt_handle_range range;
	struct bt_uuid_128 uuid;
};

struct nble_gattc_included {
	u16_t handle;
	struct nble_gatt_handle_range range;
	struct bt_uuid_128 uuid;
};

struct nble_gattc_characteristic {
	u16_t handle;
	u8_t prop;
	u16_t value_handle;
	struct bt_uuid_128 uuid;
};

struct nble_gattc_descriptor {
	u16_t handle;
	struct bt_uuid_128 uuid;
};

struct nble_gattc_discover_rsp {
	s32_t status;
	void *user_data;
	u16_t conn_handle;
	u8_t type;
};

void on_nble_gattc_discover_rsp(const struct nble_gattc_discover_rsp *rsp,
				const u8_t *data, u8_t len);

struct nble_gattc_read_req {
	void *user_data;
	u16_t conn_handle;
	u16_t handle;
	u16_t offset;
};

void nble_gattc_read_req(const struct nble_gattc_read_req *req);

struct nble_gattc_read_rsp {
	int status;
	void *user_data;
	u16_t conn_handle;
	u16_t handle;
	u16_t offset;
};

void on_nble_gattc_read_rsp(const struct nble_gattc_read_rsp *rsp,
			    u8_t *data, u8_t len);

struct nble_gattc_read_multi_req {
	void *user_data;
	u16_t conn_handle;
};

void nble_gattc_read_multi_req(const struct nble_gattc_read_multi_req *req,
			       const u16_t *handles, u16_t len);

void on_nble_gattc_read_multi_rsp(const struct nble_gattc_read_rsp *rsp,
				  u8_t *data, u8_t len);

struct nble_gattc_write_param;

typedef void (*nble_att_func_t)(struct bt_conn *conn, u8_t err,
				const struct nble_gattc_write_param *par);

struct nble_gattc_write_param {
	/* Function invoked upon write response */
	nble_att_func_t func;
	/* User specific data */
	void *user_data[2];
};

struct nble_gattc_write_req {
	u16_t conn_handle;
	u16_t handle;
	u16_t offset;
	/* different than 0 if response required */
	u8_t with_resp;
	struct nble_gattc_write_param wr_params;
};

void nble_gattc_write_req(const struct nble_gattc_write_req *req,
			  const u8_t *data, u16_t len);

struct nble_gattc_write_rsp {
	int status;
	u16_t conn_handle;
	u16_t handle;
	struct nble_gattc_write_param wr_params;
};

void on_nble_gattc_write_rsp(const struct nble_gattc_write_rsp *rsp);

void bt_gatt_connected(struct bt_conn *conn);
void bt_gatt_disconnected(struct bt_conn *conn);

enum NBLE_GATTC_EVT {
	NBLE_GATTC_EVT_NOTIFICATION,
	NBLE_GATTC_EVT_INDICATION,
};

struct nble_gattc_value_evt {
	int status;
	u16_t conn_handle;
	u16_t handle;
	/* see NBLE_GATTC_VALUE_EVT */
	u8_t type;
};

void on_nble_gattc_value_evt(const struct nble_gattc_value_evt *evt,
			     u8_t *data, u8_t len);
