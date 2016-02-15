/** @file
 *  @brief Internal API for Generic Access Profile.
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
#include <stdbool.h>
/* For bt_addr_le_t */
#include "bluetooth/hci.h"
#include "version.h"

/* Must be the same with nble_service_gap_api.h ! */

/**< Maximum security key len (LTK, CSRK) */
#define BLE_GAP_SEC_MAX_KEY_LEN		16
#define BLE_PASSKEY_LEN			6

/* Must be the same with BLE_GAP_SM_OPTIONS ! */

/**
 * GAP security manager options for bonding/authentication procedures,
 * see Vol 3: Part H, 3.5.
 */
enum BLE_CORE_GAP_SM_OPTIONS {
	BLE_CORE_GAP_BONDING = 0x01,	/**< SMP supports bonding */
	/**< SMP requires Man In The Middle protection */
	BLE_CORE_GAP_MITM = 0x04,
	BLE_CORE_GAP_OOB = 0x08		/**< SMP supports Out Of Band data */
};

/* Must be the same with BLE_CORE_GAP_SM_PASSKEY_TYPE ! */
/**
 * Security manager passkey type.
 */
enum BLE_CORE_GAP_SM_PASSKEY_TYPE {
	BLE_CORE_GAP_SM_PK_NONE = 0,	/**< No key (may be used to reject). */
	BLE_CORE_GAP_SM_PK_PASSKEY,	/**< Sec data is a 6-digit passkey. */
	BLE_CORE_GAP_SM_PK_OOB,		/**< Sec data is 16 bytes of OOB data */
};

/* Must be the same with BLE_GAP_SM_STATUS ! */
/**
 * GAP security manager status codes.
 */
enum BLE_CORE_GAP_SM_STATUS {
	BLE_CORE_GAP_SM_ST_START_PAIRING,	/*< Pairing has started */
	BLE_CORE_GAP_SM_ST_BONDING_COMPLETE,	/*< Bonding has completed */
	BLE_CORE_GAP_SM_ST_LINK_ENCRYPTED,	/*< Link is encrypted */
	BLE_CORE_GAP_SM_ST_SECURITY_UPDATE,	/*< Link keys updated */
};

/* Must be the same with BLE_GAP_RSSI_OPS ! */
/**
 * RSSI operation definition.
 */
enum BLE_CORE_GAP_RSSI_OPS {
	BLE_CORE_GAP_RSSI_DISABLE_REPORT = 0,
	BLE_CORE_GAP_RSSI_ENABLE_REPORT
};

/** Test Mode opcodes. The same with nble_service_gap_api.h */
enum BLE_CORE_TEST_OPCODE {
	/**< Put BLE controller in HCI UART DTM test mode */
	BLE_CORE_TEST_INIT_DTM = 0x01,
	BLE_CORE_TEST_START_DTM_RX = 0x1d,	/**< LE rcv test HCI op */
	BLE_CORE_TEST_START_DTM_TX = 0x1e,	/**< LE trans test HCI op */
	BLE_CORE_TEST_END_DTM = 0x1f,		/**< End LE DTM TEST */
	/** vendor specific commands start at 0x80
	 *  Set Tx power. To be called before start of tx test
	 */
	BLE_CORE_TEST_SET_TXPOWER = 0x80,
	BLE_CORE_TEST_START_TX_CARRIER,		/**< Start Tx Carrier Test */
};

struct nble_response {
	int status;		/**< Status of the operation */
	void *user_data;
};

struct nble_gap_device_name {
	/**< Security mode for writing device name, @ref BLE_GAP_SEC_MODES */
	uint8_t sec_mode;
	/**< 0: no authorization, 1: authorization required */
	uint8_t authorization;
	uint8_t len;		/**< Device name length (0-248) */
	uint8_t name_array[20];	/**< Device */
};

struct nble_gap_connection_values {
	uint16_t interval;		/**< Conn interval (unit 1.25 ms) */
	uint16_t latency;		/**< Conn latency (unit interval) */
	uint16_t supervision_to;	/**< Conn supervision timeout (10ms) */
};


enum BLE_GAP_SVC_ATTR_TYPE {
	GAP_SVC_ATTR_NAME = 0,		/**< Device Name, UUID 0x2a00 */
	GAP_SVC_ATTR_APPEARANCE,	/**< Appearance, UUID 0x2a01 */
	/**< Peripheral Preferred Connection Parameters (PPCP), UUID 0x2a04 */
	GAP_SVC_ATTR_PPCP = 4,
	/**< Central Address Resolution (CAR), UUID 0x2aa6, BT 4.2 */
	GAP_SVC_ATTR_CAR = 0xa6,
};

/**
 * Connection requested parameters.
 */
struct nble_gap_connection_params {
	/**< minimal conne interval: range 0x0006 to 0x0c80 (unit 1.25ms) */
	uint16_t interval_min;
	/**< max conn interv: range 0x0006 to 0x0c80 must be bigger then min */
	uint16_t interval_max;
	/**< maximum connection slave latency: 0x0000 to 0x01f3 */
	uint16_t slave_latency;
	/**< link supervision timeout: 0x000a to 0x0c80 (unit 10ms) */
	uint16_t link_sup_to;
};

/**
 * Connection scan requested parameters.
 */
struct nble_gap_scan_parameters {
	uint8_t     active;	/**< If 1, perform active scan (scan req) */
	uint8_t     selective;	/**< If 1, ignore unknown dev (non whitelist) */
	/**< Scan interval between 0x0004 and 0x4000 in 0.625ms units
	 * (2.5ms to 10.24s).
	 */
	uint16_t    interval;
	/**< Scan window between 0x0004 and 0x4000 in 0.625ms units
	 * (2.5ms to 10.24s).
	 */
	uint16_t    window;
	/**< Scan timeout between 0x0001 and 0xFFFF in seconds,
	 * 0x0000 disables timeout.
	 */
	uint16_t    timeout;
};

struct nble_gap_service_write_params {
	/**< GAP Characteristics attribute type  @ref BLE_GAP_SVC_ATTR_TYPE */
	uint16_t attr_type;
	union {
		struct nble_gap_device_name name;
		uint16_t appearance;			/**< Appearance UUID */
		/**< Preferred Peripheral Connection Parameters */
		struct nble_gap_connection_params conn_params;
		/**< Central Address Resolution support 0: no, 1: yes */
		uint8_t car;
	};
};

struct nble_service_read_bda_response {
	int status;		/**< Status of the operation */
	bt_addr_le_t bd;	/**< If @ref status ok */
	void *user_data;
};

struct nble_service_write_response {
	int status;		/**< Status of the operation */
	/**< GAP Characteristics attribute type  @ref BLE_GAP_SVC_ATTR_TYPE */
	uint16_t attr_type;
	void *user_data;	/**< Pointer to the user data of the request */
};

struct nble_gap_service_read_params {
	/**< Type of GAP data charact to read @ref BLE_GAP_SVC_ATTR_TYPE */
	uint16_t attr_type;
};

struct debug_params {
	uint32_t u0;	/** user parameter */
	uint32_t u1;	/** user parameter */
};

struct debug_response {
	int status;		/**< Status of the operation */
	uint32_t u0;		/** user parameter */
	uint32_t u1;		/** user parameter */
	void *user_data;	/**< Pointer to the user data of the request */
};

struct nble_wr_config_params {
	bt_addr_le_t bda;
	uint8_t bda_present;
	int8_t tx_power;
	/* Central supported range */
	struct nble_gap_connection_params central_conn_params;
};

/**
 * Advertisement parameters.
 */

/* Complete encoded eir data structure */
struct bt_eir_data {
	uint8_t len;
	uint8_t data[31];
};

struct nble_gap_adv_params {
	uint16_t timeout;
	uint16_t interval_min;	/**< min interval 0xffff: use default 0x0800 */
	uint16_t interval_max;	/**< max interval 0xffff: use default 0x0800 */
	uint8_t type;		/**< advertisement types @ref GAP_ADV_TYPES */
	uint8_t filter_policy;	/**< filter policy to apply with white list */
	/**< bd address of peer device in case of directed advertisement */
	bt_addr_le_t peer_bda;
	struct bt_eir_data ad;	/**< Advertisement data, maybe 0 (length) */
	struct bt_eir_data sd;	/**< Scan response data, maybe 0 (length) */
};

struct nble_log_s {
	uint8_t param0;
	uint8_t param1;
	uint8_t param2;
	uint8_t param3;
};

void nble_log(const struct nble_log_s *p_param, char *p_buf, uint8_t buflen);

void nble_core_delete_conn_params_timer(void);

void on_nble_up(void);

/**
 * Write GAP Service Attribute Characteristics.
 *
 * The response to this request is received through
 * @ref on_nble_gap_service_write_rsp
 *
 * @param par Data of the characteristic to write
 * @param user_data User data
 */
void nble_gap_service_write_req(const struct nble_gap_service_write_params *par,
			       void *user_data);

/**
 * Response to @ref nble_gap_read_bda_req.
 *
 * @param par Response
 */
void on_nble_gap_read_bda_rsp(const struct nble_service_read_bda_response *par);

/**
 * Response to @ref nble_gap_service_write_req.
 *
 * @param par Response
 */
void on_nble_gap_service_write_rsp(const struct nble_service_write_response *par);

/**
 * Send generic debug command
 *
 * The response to this request is received through @ref on_nble_gap_dbg_rsp
 *
 * @param par Debug parameters
 * @param user_data User data
 */
void nble_gap_dbg_req(const struct debug_params *par, void *user_data);

/**
 * Response to @ref nble_gap_dbg_req.
 *
 * @param par Response
 */
void on_nble_gap_dbg_rsp(const struct debug_response *par);

/**
 * Set Enable configuration parameters (BD address, etc).
 *
 * The response to this request is received through
 * @ref on_nble_set_enable_config_rsp
 *
 * This shall put the controller stack into a usable (enabled) state.
 * Hence this should be called first!
 *
 * @param config BLE write configuration
 * @param user_data User data
 *
 */
void nble_set_enable_config_req(const struct nble_wr_config_params *config,
				void *user_data);

/**
 * Start advertising.
 *
 * The response to this request is received through
 * @ref on_nble_gap_start_advertise_rsp
 *
 * @param par Advertisement
 * @param p_adv_data Pointer to advertisement and scan response data
 */
void nble_gap_start_advertise_req(struct nble_gap_adv_params *par);

/**
 * Response to @ref nble_gap_start_advertise_req.
 *
 * @param par Response
 */
void on_nble_gap_start_advertise_rsp(const struct nble_response *par);

/**
 * Request to stop advertisement
 *
 * @param user_data Pointer to private data
 */
void nble_gap_stop_advertise_req(void *user_data);

/**
 * Response to @ref nble_gap_stop_advertise_req.
 *
 * @param par Response
 */
void on_nble_gap_stop_advertise_rsp(const struct nble_response *par);

/**
 * Read BD address from Controller.
 *
 * The response to this request is received through @ref on_nble_gap_read_bda_rsp
 *
 * @param priv Pointer to private data
 */
void nble_gap_read_bda_req(void *priv);

struct nble_gap_irk_info {
	uint8_t irk[BLE_GAP_SEC_MAX_KEY_LEN];
};

/**
 * Write white list to the BLE controller.
 *
 * The response to this request is received through
 * @ref on_nble_gap_wr_white_list_rsp
 *
 * Store white in BLE controller. It needs to be done BEFORE starting
 * advertisement or start scanning
 *
 * @param bd_array Array of bd addresses
 * @param bd_array_size Size of bd addresses array
 * @param irk_array Array of irk keys (for address resolution offload)
 * @param irk_array_size Size of irk keys array
 * @param priv Pointer to private data
 */
void nble_gap_wr_white_list_req(bt_addr_le_t *bd_array, uint8_t bd_array_size,
				struct nble_gap_irk_info *irk_array,
				uint8_t irk_array_size, void *priv);

/**
 * Response to @ref nble_gap_wr_white_list_req.
 *
 * @param par Response
 */
void on_nble_gap_wr_white_list_rsp(const struct nble_response *par);

/**
 * Clear previously stored white list.
 *
 * The response to this request is received through
 * @ref on_nble_gap_clr_white_list_rsp
 *
 * @param priv Pointer to private data
 */
void nble_gap_clr_white_list_req(void *priv);

/**
 * Response to @ref nble_gap_clr_white_list_req.
 *
 * @param par Response
 */
void on_nble_gap_clr_white_list_rsp(const struct nble_response *par);

struct nble_gap_connect_update_params {
	uint16_t conn_handle;
	struct nble_gap_connection_params params;
};

/**
 * Update connection.
 *
 * The response to this request is received through
 * @ref on_nble_gap_conn_update_rsp
 *
 * This function's behavior depends on the role of the connection:
 * - in peripheral mode, it sends an L2CAP signaling connection parameter
 *   update request based the values in <i>p_conn_param</i> argument,
 *   and the action can be taken by the central at link layer
 * - in central mode, it will send a link layer command to change the
 *   connection values based on the values in <i>p_conn_param</i> argument
 *   where the connection interval is interval_min.
 *
 * When the connection is updated, function event on_nble_gap_conn_update_evt
 * is called.
 *
 * @param par Connection parameters
 * @param user_data User data
 */
void nble_gap_conn_update_req(const struct nble_gap_connect_update_params *par,
			      void *user_data);

/**
 * Response to @ref nble_gap_conn_update_req.
 *
 * @param par Response
 */
void on_nble_gap_conn_update_rsp(const struct nble_response *par);

struct nble_gap_connect_req_params {
	bt_addr_le_t bda;
	struct nble_gap_connection_params conn_params;
	struct nble_gap_scan_parameters scan_params;
};

struct nble_gap_disconnect_req_params {
	uint16_t conn_handle;	/**< Connection handle */
	uint8_t reason;		/**< Reason of the disconnect */
};

/**
 * Disconnect connection (peripheral or central role).
 *
 * The response to this request is received through
 * @ref on_nble_gap_disconnect_rsp
 *
 * @param par Connection to terminate
 * @param user_data User data
 */
void nble_gap_disconnect_req(const struct nble_gap_disconnect_req_params *par,
			     void *user_data);
/**
 * Response to @ref nble_gap_disconnect_req.
 *
 * @param par Response
 */
void on_nble_gap_disconnect_rsp(const struct nble_response *par);

/**
 * Read GAP Service Characteristics.
 *
 * The response to this request is received through
 * @ref on_nble_gap_service_read_rsp
 *
 * @param nble_gap_service_read GAP service characteristic to read
 * @param user_data Pointer to private data
 */
void nble_gap_service_read_req(const struct nble_gap_service_read_params *par,
			       void *user_data);

/**
 * Response to @ref nble_gap_service_read_req.
 *
 * @param par Response
 */
void on_nble_gap_service_read_rsp(const struct nble_response *par);

/**
 * Security manager configuration parameters.
 *
 * options and io_caps will define there will be a passkey request or not.
 * It is assumed that io_caps and options are compatible.
 */
struct nble_gap_sm_config_params {
	/**< Sec options (@ref BLE_GAP_SM_OPTIONS) */
	uint8_t options;
	/**< I/O Capabilities to allow passkey exchange
	 * (@ref BLE_GAP_IO_CAPABILITIES)
	 */
	uint8_t io_caps;
	uint8_t key_size;	/**< Maximum encryption key size (7-16) */
	uint8_t oob_present;
};

/**
 * Configuring the security manager.
 *
 * The response to this request is received through
 * @ref on_nble_gap_sm_config_rsp
 *
 * @param par Local authentication/bonding parameters
 */
void nble_gap_sm_config_req(const struct nble_gap_sm_config_params *par);

struct nble_gap_sm_config_rsp {
	void *user_data;	/**< Pointer to user data structure */
	int status;		/**< Result of sec manager initialization */
	uint32_t state;		/**< State of bond DB */
};

/**
 * Response to @ref nble_gap_sm_config_req.
 *
 * @param par Response
 */
void on_nble_gap_sm_config_rsp(struct nble_gap_sm_config_rsp *par);

/**
 * Security manager pairing parameters.
 */
struct nble_core_gap_sm_pairing_params {
	/**< authentication level see @ref BLE_GAP_SM_OPTIONS */
	uint8_t auth_level;
};

struct nble_gap_sm_security_params {
	struct bt_conn *conn;
	/**< Connection on which bonding procedure is executed */
	uint16_t conn_handle;
	/**< Local authentication/bonding parameters */
	struct nble_core_gap_sm_pairing_params params;
};

/**
 * Initiate the bonding procedure (central).
 *
 * The response to this request is received through
 * @ref on_nble_gap_sm_pairing_rsp
 *
 * @param par Connection to initiate with its parameters
 */
void nble_gap_sm_security_req(const struct nble_gap_sm_security_params *par);

/**
 * Response to @ref nble_gap_sm_pairing_req.
 *
 * @param par Response
 */
void on_nble_gap_sm_pairing_rsp(const struct nble_response *par);

/**
 * Security reply to incoming security request.
 */
struct nble_core_gap_sm_passkey {
	/**< Security data type in this reply @ref BLE_GAP_SM_PASSKEY_TYPE */
	uint8_t type;
	union {
		uint8_t passkey[6];	/**< 6 digits (string) */
		uint8_t oob[16];	/**< 16 bytes of OOB security data */
	};
};

struct nble_gap_sm_key_reply_req_params {
	/**< Connection on which bonding is going on */
	uint16_t conn_handle;
	struct nble_core_gap_sm_passkey params;	/**< Bonding security reply */
};

/**
 * Reply to an incoming passkey request event.
 *
 * The response to this request is received through
 * @ref on_nble_gap_sm_passkey_reply_rsp
 *
 * @param par Connection on which bonding is going on and  bonding security
 * reply
 */
void nble_gap_sm_passkey_reply_req(const struct nble_gap_sm_key_reply_req_params *par);

struct nble_gap_sm_clear_bond_req_params {
	bt_addr_le_t addr;
};

/**
 * Clear bonds
 *
 * The response to this request is received through
 * @ref on_nble_gap_sm_clear_bonds_rsp
 *
 * @param par Parameters
 */
void nble_gap_sm_clear_bonds_req(const struct nble_gap_sm_clear_bond_req_params *par);

/**
 * Response to @ref nble_gap_sm_clear_bonds_req.
 *
 * @param par Response
 */
void on_nble_gap_sm_clear_bonds_rsp(const struct nble_response *par);

struct nble_gap_sm_response {
	int status;
	struct bt_conn *conn;
};

/**
 * RSSI report parameters
 */
struct nble_rssi_report_params {
	uint16_t conn_handle;	/**< Connection handle */
	uint8_t op;		/**< RSSI operation @ref BLE_GAP_RSSI_OPS */
	uint8_t delta_dBm;	/**< minimum RSSI dBm change report new val */
	/**< number of delta_dBm changes before sending a new RSSI report */
	uint8_t min_count;
};

/**
 * Enable or disable the reporting of the RSSI value.
 *
 * The response to this request is received through
 * @ref on_nble_gap_set_rssi_report_rsp
 *
 * @param params RSSI report parameters
 * @param user_data Pointer to user data
 */
void nble_gap_set_rssi_report_req(const struct nble_rssi_report_params *par,
				  void *user_data);

/**
 * Response to @ref nble_gap_set_rssi_report_req.
 *
 * @param par Response
 */
void on_nble_gap_set_rssi_report_rsp(const struct nble_response *par);

enum BLE_GAP_SCAN_OPTIONS {
	BLE_GAP_SCAN_DEFAULT = 0,
	BLE_GAP_SCAN_ACTIVE = 0x01,
	BLE_GAP_SCAN_WHITE_LISTED = 0x02,
};

struct nble_gap_scan_params {
	uint16_t interval;
	uint16_t window;
	uint8_t scan_type;
	uint8_t use_whitelist;
};

/**
 * Start scanning for BLE devices doing advertisement.
 *
 * The response to this request is received through
 * @ref on_nble_gap_start_scan_rsp
 *
 * @param par Scan parameters
 */
void nble_gap_start_scan_req(const struct nble_gap_scan_params *par);

/**
 * Response to @ref nble_gap_start_scan_req.
 *
 * @param par Response
 */
void on_nble_gap_start_scan_rsp(const struct nble_response *par);

/**
 * Stop scanning.
 *
 * The response to this request is received through
 * @ref on_nble_gap_stop_scan_rsp
 */
void nble_gap_stop_scan_req(void);

/**
 * Response to @ref nble_gap_stop_scan_req.
 *
 * @param par Response
 */
void on_nble_gap_stop_scan_rsp(const struct nble_response *par);

/**
 * Connect to a Remote Device.
 *
 * The response to this request is received through @ref on_nble_gap_connect_rsp
 *
 * @param req connection parameters @ref nble_gap_connect_req_params
 * @param user_data Pointer to private user data
 */
void nble_gap_connect_req(const struct nble_gap_connect_req_params *req,
			  void *user_data);

/**
 * Response to @ref nble_gap_connect_req.
 *
 * @param par Response
 */
void on_nble_gap_connect_rsp(const struct nble_response *rsp);

struct nble_gap_cancel_connect_params {
	const bt_addr_le_t bd;
};

/**
 * Cancel an ongoing connection attempt.
 *
 * The response to this request is received through
 * @ref on_nble_gap_cancel_connect_rsp
 *
 * @param user_data Pointer to user data
 */
void nble_gap_cancel_connect_req(void *user_data);

/**
 * Response to @ref nble_gap_cancel_connect_req.
 *
 * @param par Response
 */
void on_nble_gap_cancel_connect_rsp(const struct nble_response *par);

enum BLE_GAP_SET_OPTIONS {
	BLE_GAP_SET_CH_MAP = 0,		/**< Set channel map */
};

struct nble_gap_channel_map {
	uint16_t conn_handle;	/**< conn on which to change channel map */
	uint8_t map[5];		/**< 37 bits are used of the 40 bits (LSB) */
};


struct nble_gap_set_option_params {
	uint8_t op;		/**< Option to set @ref BLE_GAP_SET_OPTIONS */
	union {
		struct nble_gap_channel_map ch_map;
	};
};

/**
 * Set a gap option (channel map etc) on a connection.
 *
 * The response to this request is received through
 * @ref on_nble_gap_set_option_rsp
 *
 * @param par Contains gap options parameters
 * @param user_data Pointer to user data
 */
void nble_gap_set_option_req(const struct nble_gap_set_option_params *par,
			     void *user_data);

/**
 * Response to @ref nble_gap_set_option_req.
 *
 * @param par Response
 */
void on_nble_gap_set_option_rsp(const struct nble_response *par);

/** Generic request op codes.
 * This allows to access some non connection related commands like DTM.
 */
enum BLE_GAP_GEN_OPS {
	DUMMY_VALUE = 0,		/**< Not used now. */
};

/** Generic command parameters. */
struct nble_gap_gen_cmd_params {
	uint8_t op_code;		/**< @ref BLE_GAP_GEN_OPS */
};

/**
 * Generic command
 *
 * The response to this request is received through
 * @ref on_nble_gap_generic_cmd_rsp
 *
 * @param par Contains Generic command parameters.
 * @param user_data Pointer to user data
 */
void nble_gap_generic_cmd_req(const struct nble_gap_gen_cmd_params *par,
			      void *priv);

/**
 * Response to @ref nble_gap_generic_cmd_req.
 *
 * @param par Response
 */
void on_nble_gap_generic_cmd_rsp(const struct nble_response *par);

/**
 * Get nble_core version.
 *
 * The response to this request is received through @ref on_ble_get_version_rsp
 *
 * @param rsp Pointer to response data structure
 */
void nble_get_version_req(void *user_data);

struct nble_version_response {
	struct version_header version;
	void *user_data;	/**< Pointer to response data structure */
};

/**
 * Response to @ref nble_get_version_req.
 *
 * @param par Response
 */
void on_nble_get_version_rsp(const struct nble_version_response *par);

/**
 * Init DTM mode.
 *
 * The response to this request is received through
 * @ref on_nble_gap_dtm_init_rsp
 *
 * @param user_data Pointer to response data structure
 */
void nble_gap_dtm_init_req(void *user_data);

struct nble_gap_connect_evt {
	uint16_t conn_handle;
	struct nble_gap_connection_values conn_values;
	uint8_t role;
	bt_addr_le_t peer_bda;
};

/**
 * Function invoked by the BLE service when a new connection is established.
 *
 * @param ev Pointer to the event structure.
 */
void on_nble_gap_connect_evt(const struct nble_gap_connect_evt *ev);

struct nble_gap_disconnect_evt {
	uint16_t conn_handle;	/**< Connection handle */
	uint8_t hci_reason;	/**< HCI disconnect reason */
};

/**
 * Function invoked by the BLE service when a connection is lost.
 *
 * @param ev Pointer to the event structure.
 */
void on_nble_gap_disconnect_evt(const struct nble_gap_disconnect_evt *ev);


/**
 * Updated connection event.
 */
struct nble_gap_conn_update_evt {
	uint16_t conn_handle;
	struct nble_gap_connection_values conn_values;
};

/**
 * Function invoked by the BLE service when a connection is updated.
 *
 * @param ev Pointer to the event structure.
 */
void on_nble_gap_conn_update_evt(const struct nble_gap_conn_update_evt *ev);

struct nble_gap_adv_report_evt {
	bt_addr_le_t addr;
	int8_t rssi;
	uint8_t adv_type;
};

struct nble_gap_rssi_evt {
	uint16_t conn_handle;	/**< Connection handle */
	int8_t rssi_lvl;	/**< RSSI level (compared to 0 dBm) */
};

/**
 * Function invoked by the BLE service upon RSSI event.
 *
 * @param ev Pointer to the event structure.
 */
void on_nble_gap_rssi_evt(const struct nble_gap_rssi_evt *ev);

struct nble_gap_timout_evt {
	uint16_t conn_handle;	/**< Connection handle */
	/**< reason for timeout @ref BLE_SVC_GAP_TIMEOUT_REASON */
	int reason;
};

/**
 * Function invoked by the BLE service upon timeout event.
 *
 * @param ev Pointer to the event structure.
 */
void on_nble_gap_to_evt(const struct nble_gap_timout_evt *ev);

struct nble_gap_sm_passkey_req_evt {
	uint16_t conn_handle;	/**< Connection handle */
	/**< Passkey or OBB data see @ref BLE_GAP_SM_PASSKEY_TYPE */
	uint8_t key_type;
};

/**
 * Function invoked by the BLE service upon security manager passkey
 * request event.
 *
 * @param ev Pointer to the event structure.
 */
void on_nble_gap_sm_passkey_req_evt(const struct nble_gap_sm_passkey_req_evt *);

struct nble_gap_sm_passkey_disp_evt {
	uint16_t conn_handle;			/**< Connection handle */
	uint8_t passkey[BLE_PASSKEY_LEN];	/**< Passkey to be displayed */
};

/**
 * Function invoked by the BLE service upon security manager display event.
 *
 * @param ev Pointer to the event structure.
 */
void on_nble_gap_sm_passkey_display_evt(const struct nble_gap_sm_passkey_disp_evt *ev);

struct nble_gap_sm_status_evt {
	uint16_t conn_handle;	/**< Connection handle */
	/**< Security manager status @ref BLE_GAP_SM_STATUS */
	uint8_t status;
	/**< Result of SM procedure, non-null indicates failure */
	uint8_t gap_status;
};

/**
 * Function invoked by the BLE service upon a security manager event.
 *
 * @param ev Pointer to the event structure.
 */
void on_nble_gap_sm_status_evt(const struct nble_gap_sm_status_evt *ev);

/**
 * Response to @ref nble_set_enable_config_req.
 *
 * @param par Response
 */
void on_ble_set_enable_config_rsp(const struct nble_response *par);

/**
 * Get the list of bonded devices
 *
 * @param user_data User Data
 */
void nble_get_bonded_device_list_req(void *user_data);

/**@brief Structure containing list of bonded devices. */
struct nble_core_bonded_devices {
	uint8_t       addr_count;	/**< Count of device addr in array. */
};

struct nble_gap_sm_bond_info;

typedef void (*ble_bond_info_cb_t)(const struct nble_gap_sm_bond_info *info,
				   const bt_addr_le_t *addr, uint16_t len,
				   void *user_data);

struct nble_gap_sm_bond_info_param {
	ble_bond_info_cb_t cb;
	void *user_data;
	bool include_bonded_addrs;
};

void nble_gap_sm_bond_info_req(const struct nble_gap_sm_bond_info_param *params);

struct nble_gap_sm_bond_info {
	int err;
	uint8_t addr_count;
	uint8_t irk_count;
};

struct nble_gap_sm_bond_info_rsp {
	ble_bond_info_cb_t cb;
	void *user_data;
	struct nble_gap_sm_bond_info info;
};
