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

#include <stdint.h>

#include <bluetooth/gatt.h>

/* Forward declarations */
struct nble_core_response;

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
	/**< Base address of the attribute table in the Quark mem space */
	struct bt_gatt_attr *attr_base;
	uint8_t attr_count; /**< Number of of attributes in this service */
};

/** GATT Register structure for one service */
struct nble_gatt_register_rsp {
	int status;			/**< Status of the registration op */
	struct bt_gatt_attr *attr_base;	/**< Pointer to the registered table */
	uint8_t attr_count;		/**< Number of attributes  added */
};

/** Service index and Attribute index mapping structure.
 *
 * Mapping index into attribute tables as registered with bt_gatt_register/
 * nble_gatt_register.
 */
struct nble_gatt_attr_handle_mapping {
	uint8_t svc_idx; /**< Service index */
	uint8_t attr_idx; /**< Attribute index into service attribute table */
};

enum BLE_GATTS_WR_OPS {
	BLE_GATTS_OP_NONE = 0,
	/**< 3.4.5.1 Write Request (Attribute), expects write response */
	BLE_GATTS_OP_WR,
	/**< 3.4.5.3 Write Command (Attribute) NO response sent */
	BLE_GATTS_OP_WR_CMD,
	 /**< 3.4.5.4 Write Command Signed (Attribute), NO response sent */
	BLE_GATTS_OP_WR_CMD_SIGNED,
	 /**< 3.4.6.1 Write Prepare Request, expects a prepare write request
	  * response
	  */
	BLE_GATTS_OP_WR_PREP_REQ,
	/**< 3.4.6.3 Cancel Executed Write Request, cancel and clear queue
	 * (flags = 0)
	 */
	BLE_GATTS_OP_WR_EXE_REQ_CANCEL,
	/**< 3.4.6.3 Immediately Execute Write Request */
	BLE_GATTS_OP_WR_EXE_REQ_IMM
};

/**
 * Write event context data structure.
 */
struct nble_gatt_wr_evt {
	struct bt_gatt_attr *attr;	/**< GATT Attribute */
	uint16_t conn_handle;		/**< Connection handle */
	uint16_t attr_handle;		/**< handle of attribute to write */
	uint16_t offset;		/**< offset in attribute buffer */
	uint8_t reply;			/**< 1 if reply required, 0 otherwise */
};

/**
 * Read event context data structure.
 */
struct nble_gatt_rd_evt {
	struct bt_gatt_attr *attr;	/**< GATT Attribute */
	uint16_t conn_handle;		/**< Connection handle */
	uint16_t attr_handle;		/**< handle of attribute to write */
	uint16_t offset;		/**< offset in attribute buffer */
};

struct nble_gatts_rw_reply_params {
	int status;			/**< Reply status, from errno */
	uint16_t conn_handle;		/**< Connection handle */
	uint16_t offset;		/**< Offset in attribute buffer */
	uint8_t write_reply;		/**< 0 if read reply, otherwise rsp */
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

struct nble_gatt_send_notif_ind_params {
	void *cback;
	uint16_t conn_handle;
	struct nble_gatt_notif_ind_params params;
};

enum nble_gatts_notif_ind_type {
	MSG_ID_BLE_GATTS_SEND_NOTIF_RSP,    /**< Notification type */
	MSG_ID_BLE_GATTS_SEND_IND_RSP,      /**< Indication type */
};

struct nble_gatt_notif_ind_rsp {
	bt_gatt_notify_func_t cback;
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

/**
 * Primary service
 */
struct nble_gattc_prim_svc {
	uint16_t handle;				/**< Attribute handle */
	struct nble_gatt_handle_range handle_range;	/**< handle range */
	struct bt_uuid_128 uuid;			/**< Attribute's UUID */
};

/**
 * Generic GATTC response message.
 */
struct nble_gattc_rsp {
	int status;
	uint16_t conn_handle;
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

struct nble_gattc_disc_rsp {
	int status;
	uint16_t conn_handle;
	uint8_t type;			/**< @ref BLE_GATT_DISC_TYPES */
};

struct nble_gattc_evt {
	uint16_t conn_handle;
	int status;
};

/**
 * Included service.
 */
struct nble_gattc_incl_svc {
	uint16_t handle;			/**< Handle of included svc */
	struct nble_gatt_handle_range handle_range;	/**< handle range */
	struct bt_uuid_128 uuid;			/**< Service UUID */
};

/* GATTC characteristic */
struct nble_gattc_characteristic {
	uint16_t handle;		/**< Characteristic Definition handle */
	uint8_t prop;			/**< Characteristic property */
	uint16_t value_handle;		/**< Characteristic value handle */
	struct bt_uuid_128 uuid;	/**< Characteristic UUID */
};


/**
 * GATTC descriptor.
 */
struct nble_gattc_descriptor {
	uint16_t handle;			/**< descriptor handle */
	struct bt_uuid_128 uuid;		/**< uuid of the descriptor */
};

struct nble_gattc_attr {
	uint8_t type;		/**< @ref BLE_GATT_DISC_TYPES */
	union {
		struct nble_gattc_prim_svc prim;
		struct nble_gattc_incl_svc incls;
		struct nble_gattc_characteristic chars;
		struct nble_gattc_descriptor desc;
	};
};

struct nble_gatts_set_attribute_params {
	uint16_t value_handle;		/* mandatory */
	uint16_t offset;		/* by default 0 */
};

struct nble_gatts_get_attribute_params {
	uint16_t value_handle;		/* mandatory */
};

struct nble_gatts_attribute_rsp {
	int status;			/**< Status of the operation. */
	uint16_t value_handle;		/* mandatory */
	void *priv;
};

/**
 * Set an attribute value.
 *
 * The response to this request is received through @ref
 * on_ble_gatts_send_svc_changed_rsp
 *
 * @param par Attribute parameters
 * @param data Attribute value data to write
 * @param len Length of attribute value to write
 * @param priv Pointer to private data
 */
void nble_gatts_set_attribute_value_req(const struct nble_gatts_set_attribute_params *par,
					uint8_t *data, uint8_t len, void *priv);

/**
 * Response to @ref nble_gatts_send_svc_changed_req.
 *
 * @param rsp Response
 */
void on_ble_gatts_set_attribute_value_rsp(const struct nble_gatts_attribute_rsp *rsp);

/**
 * Get an attribute value.
 *
 * The response to this request is received through @ref
 * on_ble_gatts_get_attribute_value_rsp
 *
 * @param par Getting attribute parameters
 * @param priv Pointer to private data
 */
void nble_gatts_get_attribute_value_req(const struct nble_gatts_get_attribute_params *par,
					void *priv);

/**
 * Response to @ref nble_gatts_get_attribute_value_req.
 *
 * @param rsp Response
 * @param data Attribute value
 * @param length Length of attribute value
 */
void on_ble_gatts_get_attribute_value_rsp(const struct nble_gatts_attribute_rsp *rsp,
					  uint8_t *data, uint8_t length);

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

/**
 * Response to @ref nble_gatts_send_svc_changed_req.
 *
 * @param par Response
 */
void on_ble_gatts_send_svc_changed_rsp(const struct nble_core_response *par);

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
void on_ble_gatts_write_evt(const struct nble_gatt_wr_evt *ev,
			    const uint8_t *buf, uint8_t len);

/**
 * Retrieves handle based on attribute array and index of attribute
 *
 * @param attrs Attribute array
 * @param index Index of attribute
 *
 * @return Handle of attribute or 0 if not found
 */
uint16_t nble_attr_idx_to_handle(const struct bt_gatt_attr *attrs,
				 uint8_t index);

/**
 * Send notification.
 *
 * The response to this request is received through @ref
 * on_ble_gatts_send_notif_ind_rsp
 *
 * @param par Notification parameters
 * @param data Indication data to write
 * @param length Length of indication - may be 0, in this case already
 * stored data is sent
 */
void nble_gatt_send_notif_req(const struct nble_gatt_send_notif_ind_params *par,
			      uint8_t *data, uint16_t length);

/**
 * Send indication.
 *
 * The response to this request is received through @ref
 * on_ble_gatts_send_notif_ind_rsp
 *
 * @param par Indication parameters
 * @param data Indication data to write
 * @param length Length of indication - may be 0, in this case already
 * stored data is sent
 */
void nble_gatt_send_ind_req(const struct nble_gatt_send_notif_ind_params *par,
			    uint8_t *data, uint8_t length);

/**
 * Response to @ref nble_gatts_send_ind_req and @ref nble_gatts_send_notif_req
 *
 * @param par Response
 */
void on_ble_gatts_send_notif_ind_rsp(const struct nble_gatt_notif_ind_rsp *par);

/** Discover parameters. */
struct nble_discover_params {
	struct bt_uuid_128 uuid;			/**< Attribute UUID */
	struct nble_gatt_handle_range handle_range;	/**< Discover range */
	uint16_t conn_handle;				/**< Connection handl */
	/**< Discover type @ref BLE_GATT_DISC_TYPES */
	uint8_t type;
};

/**
 * Discover service.
 *
 * @param req Request structure.
 * @param user_data Pointer to private data.
 */
void nble_gattc_discover_req(const struct nble_discover_params *req,
			     void *user_data);

/**
 * Response to @ref nble_gattc_discover_req.
 *
 * @param rsp Response
 * @param data Pointer to the data
 * @param len Length of the data
 */
void on_ble_gattc_discover_rsp(const struct nble_gattc_disc_rsp *rsp,
			       const uint8_t *data, uint8_t len);


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
	uint16_t conn_handle;	/**< Connection handle*/
	uint16_t char_handle;	/**< Handle of the attribute to be read */
	uint16_t offset;	/**< Offset into the attr value to be read */
};

struct nble_gattc_read_rsp {
	uint16_t conn_handle;
	int status;
	uint16_t handle;	/**< handle of char attr read */
	uint16_t offset;	/**< offset of data returned */
};

struct nble_gattc_write_params {
	uint16_t conn_handle; /**< Connection handle*/
	uint16_t char_handle; /**< Handle of the attribute to be read */
	uint16_t offset;      /**< Offset into the attr value to be write */
	bool with_resp;       /**< Equal to true is response is needed */
};

struct nble_gattc_write_rsp {
	uint16_t conn_handle;
	int status;
	uint16_t char_handle;
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
 * Response to @ref nble_gattc_read_req.
 *
 * @param ev Pointer to the event structure
 * @param data Pointer to the data byte stream
 * @param data_len Length of the data byte stream
 * @param priv Pointer to private data.
 */
void on_ble_gattc_read_rsp(const struct nble_gattc_read_rsp *ev,
			   uint8_t *data, uint8_t data_len, void *priv);

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

/**
 * Response to @ref nble_gattc_write_req.
 *
 * @param ev Pointer to the event structure
 * @param priv Pointer to private data.
 */
void on_ble_gattc_write_rsp(const struct nble_gattc_write_rsp *ev,
			    void *priv);

#if defined(CONFIG_BLUETOOTH_GATT_CLIENT)
void bt_gatt_connected(struct bt_conn *conn);
void bt_gatt_disconnected(struct bt_conn *conn);
#endif

struct nble_gattc_value_evt {
	uint16_t conn_handle;
	int status;
	/**< handle of characteristic being notified/indicated */
	uint16_t handle;
	/**< notification versus indication, @ref BLE_GATT_IND_TYPES */
	uint8_t type;
};

/**
 * Function invoked by BLE service for value event
 *
 * @param ev Pointer to the event structure
 * @param buf Pointer to the data byte stream
 * @param len Length of the data byte stream
 */
void on_ble_gattc_value_evt(const struct nble_gattc_value_evt *ev,
			    uint8_t *buf, uint8_t len);

struct nble_gattc_to_evt {
	uint16_t conn_handle;
	uint16_t reason;	/**< GATT timeout reason */
};

/**
 * Function invoked by BLE service for gattc timeout protocol error
 *
 * @param ev Pointer to the event structure
 */
void on_ble_gattc_to_evt(const struct nble_gattc_to_evt *ev);
