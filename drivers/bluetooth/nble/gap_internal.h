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

/* Maximum security key len (LTK, CSRK) */
#define BLE_GAP_SEC_MAX_KEY_LEN		16

#define BT_SMP_AUTH_NONE			0x00
#define BT_SMP_AUTH_BONDING			0x01
#define BT_SMP_AUTH_MITM			0x04
#define BT_SMP_AUTH_SC				0x08

enum NBLE_GAP_SM_PASSKEY_TYPE {
	NBLE_GAP_SM_REJECT = 0,
	NBLE_GAP_SM_PK_PASSKEY,
	NBLE_GAP_SM_PK_OOB,
};

enum NBLE_GAP_SM_EVT {
	NBLE_GAP_SM_EVT_START_PAIRING,
	NBLE_GAP_SM_EVT_BONDING_COMPLETE,
	NBLE_GAP_SM_EVT_LINK_ENCRYPTED,
	NBLE_GAP_SM_EVT_LINK_SECURITY_CHANGE,
};

struct nble_response {
	int status;
	void *user_data;
};

struct nble_gap_device_name {
	/* Security mode for writing device name */
	uint8_t sec_mode;
	/* 0: no authorization, 1: authorization required */
	uint8_t authorization;
	/* Device name length (0-248) */
	uint8_t len;
	uint8_t name_array[20];
};

struct nble_gap_connection_values {
	/* Connection interval (unit 1.25 ms) */
	uint16_t interval;
	/* Connection latency (unit interval) */
	uint16_t latency;
	/* Connection supervision timeout (unit 10ms)*/
	uint16_t supervision_to;
};

struct nble_gap_connection_params {
	/* minimal connection interval: range 0x0006 to 0x0c80 (unit 1.25ms) */
	uint16_t interval_min;
	/* maximum connection interval: range 0x0006 to 0x0c80 must be bigger then min! */
	uint16_t interval_max;
	/* maximum connection slave latency: 0x0000 to 0x01f3 */
	uint16_t slave_latency;
	/* link supervision timeout: 0x000a to 0x0c80 (unit 10ms) */
	uint16_t link_sup_to;
};

struct nble_gap_scan_parameters {
	/* If 1, perform active scanning (scan requests). */
	uint8_t active;
	/* If 1, ignore unknown devices (non whitelisted). */
	uint8_t selective;
	/* Scan interval between 0x0004 and 0x4000 in 0.625ms units (2.5ms to 10.24s). */
	uint16_t interval;
	/* Scan window between 0x0004 and 0x4000 in 0.625ms units (2.5ms to 10.24s). */
	uint16_t window;
	/* Scan timeout between 0x0001 and 0xFFFF in seconds, 0x0000 disables timeout. */
	uint16_t timeout;
};

struct nble_gap_service_write_params {
	/* GAP Characteristics attribute type  @ref BLE_GAP_SVC_ATTR_TYPE */
	uint16_t attr_type;
	union {
		struct nble_gap_device_name name;
		/* Appearance UUID */
		uint16_t appearance;
		/* Preferred Peripheral Connection Parameters */
		struct nble_gap_connection_params conn_params;
		/* Central Address Resolution support 0: no, 1: yes */
		uint8_t car;
	};
};

struct nble_service_read_bda_response {
	int status;
	/* If @ref status ok */
	bt_addr_le_t bd;
	void *user_data;
};

struct nble_service_write_response {
	int status;
	/* GAP Characteristics attribute type  @ref BLE_GAP_SVC_ATTR_TYPE */
	uint16_t attr_type;
	void *user_data;
};

struct nble_gap_service_read_params {
	/* Type of GAP data characteristic to read @ref BLE_GAP_SVC_ATTR_TYPE */
	uint16_t attr_type;
};

struct nble_debug_params {
	uint32_t u0;
	uint32_t u1;
};

struct nble_debug_resp {
	int status;
	uint32_t u0;
	uint32_t u1;
	void *user_data;
};

typedef void (*nble_set_bda_cb_t)(int status, void *user_data);

struct nble_set_bda_params {
	bt_addr_le_t bda;
	nble_set_bda_cb_t cb;
	void *user_data;
};

struct nble_set_bda_rsp {
	nble_set_bda_cb_t cb;
	void *user_data;
	int status;
};

struct bt_eir_data {
	uint8_t len;
	uint8_t data[31];
};

struct nble_gap_adv_params {
	uint16_t timeout;
	/* min interval 0xffff: use default 0x0800 */
	uint16_t interval_min;
	/* max interval 0xffff: use default 0x0800 */
	uint16_t interval_max;
	/* advertisement types @ref GAP_ADV_TYPES */
	uint8_t type;
	/* filter policy to apply with white list */
	uint8_t filter_policy;
	/* bd address of peer device in case of directed advertisement */
	bt_addr_le_t peer_bda;
};

struct nble_gap_ad_data_params {
	/* Advertisement data, maybe 0 (length) */
	struct bt_eir_data ad;
	/* Scan response data, maybe 0 (length) */
	struct bt_eir_data sd;
};

struct nble_log_s {
	uint8_t param0;
	uint8_t param1;
	uint8_t param2;
	uint8_t param3;
};

void nble_log(const struct nble_log_s *param, char *buf, uint8_t buflen);

void on_nble_up(void);

void nble_gap_service_write_req(const struct nble_gap_service_write_params *);

void on_nble_gap_read_bda_rsp(const struct nble_service_read_bda_response *par);

void on_nble_gap_service_write_rsp(const struct nble_service_write_response *par);

void nble_gap_dbg_req(const struct nble_debug_params *par, void *user_data);

void on_nble_gap_dbg_rsp(const struct nble_debug_resp *rsp);

void on_nble_set_bda_rsp(const struct nble_set_bda_rsp *par);

void nble_set_bda_req(const struct nble_set_bda_params *par);

void nble_gap_set_adv_data_req(struct nble_gap_ad_data_params *ad_data_params);

void nble_gap_set_adv_params_req(struct nble_gap_adv_params *adv_params);

void nble_gap_start_adv_req(void);

void on_nble_gap_start_advertise_rsp(const struct nble_response *params);

void nble_gap_stop_adv_req(void *user_data);

void nble_gap_read_bda_req(void *priv);

struct nble_gap_irk_info {
	uint8_t irk[BLE_GAP_SEC_MAX_KEY_LEN];
};

void nble_gap_wr_white_list_req(bt_addr_le_t *bd_array, uint8_t bd_array_size,
				struct nble_gap_irk_info *irk_array,
				uint8_t irk_array_size, void *priv);

void nble_gap_clr_white_list_req(void *priv);

struct nble_gap_connect_update_params {
	uint16_t conn_handle;
	struct nble_gap_connection_params params;
};

void nble_set_bda_req(const struct nble_set_bda_params *params);

void nble_gap_conn_update_req(const struct nble_gap_connect_update_params *par);

struct nble_gap_connect_req_params {
	bt_addr_le_t bda;
	struct nble_gap_connection_params conn_params;
	struct nble_gap_scan_parameters scan_params;
};

struct nble_gap_disconnect_req_params {
	uint16_t conn_handle;
	uint8_t reason;
};

void nble_gap_disconnect_req(const struct nble_gap_disconnect_req_params *);

void on_nble_gap_disconnect_rsp(const struct nble_response *par);

void nble_gap_service_read_req(const struct nble_gap_service_read_params *par,
			       void *user_data);

struct nble_gap_sm_config_params {
	/* Security options (@ref BLE_GAP_SM_OPTIONS) */
	uint8_t options;
	/* I/O Capabilities to allow passkey exchange (@ref BLE_GAP_IO_CAPABILITIES) */
	uint8_t io_caps;
	/* Maximum encryption key size (7-16) */
	uint8_t key_size;
	uint8_t oob_present;
};

void nble_gap_sm_config_req(const struct nble_gap_sm_config_params *par);

struct nble_gap_sm_config_rsp {
	void *user_data;
	int status;
	bool sm_bond_dev_avail;
};

struct nble_gap_sm_pairing_params {
	/* authentication level see @ref BLE_GAP_SM_OPTIONS */
	uint8_t auth_level;
};

struct nble_gap_sm_security_params {
	struct bt_conn *conn;
	/* Connection on which bonding procedure is executed */
	uint16_t conn_handle;
	/* Local authentication/bonding parameters */
	struct nble_gap_sm_pairing_params params;
};

void nble_gap_sm_security_req(const struct nble_gap_sm_security_params *par);

struct nble_gap_sm_passkey {
	uint8_t type;
	union {
		uint32_t passkey;
		uint8_t oob[16];
		uint8_t reason;
	};
};

struct nble_gap_sm_key_reply_req_params {
	struct bt_conn *conn;
	uint16_t conn_handle;
	struct nble_gap_sm_passkey params;
};

void nble_gap_sm_passkey_reply_req(const struct nble_gap_sm_key_reply_req_params *par);

struct nble_gap_sm_clear_bond_req_params {
	bt_addr_le_t addr;
};

void nble_gap_sm_clear_bonds_req(const struct nble_gap_sm_clear_bond_req_params *par);

struct nble_gap_sm_response {
	int status;
	struct bt_conn *conn;
};

struct nble_rssi_report_params {
	uint16_t conn_handle;
	/* RSSI operation @ref BLE_GAP_RSSI_OPS */
	uint8_t op;
	/* Channel for RSSI enabling */
	uint8_t channel;
	/* minimum RSSI dBm change to report a new RSSI value */
	uint8_t delta_dBm;
	/* number of delta_dBm changes before sending a new RSSI report */
	uint8_t min_count;
};

void nble_gap_set_rssi_report_req(const struct nble_rssi_report_params *par,
				  void *user_data);

void on_nble_gap_set_rssi_report_rsp(const struct nble_response *par);

struct nble_gap_scan_params {
	uint16_t interval;
	uint16_t window;
	uint8_t scan_type;
	uint8_t use_whitelist;
};

void nble_gap_start_scan_req(const struct nble_gap_scan_params *par);

void nble_gap_stop_scan_req(void);

void on_nble_gap_scan_start_stop_rsp(const struct nble_response *par);

void nble_gap_connect_req(const struct nble_gap_connect_req_params *req,
			  void *user_data);

void on_nble_gap_connect_rsp(const struct nble_response *rsp);

void nble_gap_cancel_connect_req(void *user_data);

void on_nble_gap_cancel_connect_rsp(const struct nble_response *par);

enum BLE_GAP_SET_OPTIONS {
	BLE_GAP_SET_CH_MAP = 0,
};

struct nble_gap_channel_map {
	/* connection on which to change channel map */
	uint16_t conn_handle;
	/* 37 bits are used of the 40 bits (LSB) */
	uint8_t map[5];
};

struct nble_gap_set_option_params {
	/* Option to set @ref BLE_GAP_SET_OPTIONS */
	uint8_t op;
	union {
		struct nble_gap_channel_map ch_map;
	};
};

void nble_gap_set_option_req(const struct nble_gap_set_option_params *par,
			     void *user_data);

/*
 * Generic request op codes.
 * This allows to access some non connection related commands like DTM.
 */
enum BLE_GAP_GEN_OPS {
	/* Not used now. */
	DUMMY_VALUE = 0,
};

/** Generic command parameters. */
struct nble_gap_gen_cmd_params {
	/* @ref BLE_GAP_GEN_OPS */
	uint8_t op_code;
};

void nble_gap_generic_cmd_req(const struct nble_gap_gen_cmd_params *par,
			      void *priv);

struct nble_uas_rssi_calibrate {
	float distance;
};
void nble_uas_rssi_calibrate_req(const struct nble_uas_rssi_calibrate *);

struct nble_uas_bucket_change {
	uint8_t distance;
};
void on_nble_uas_bucket_change(const struct nble_uas_bucket_change *);

void on_nble_gap_generic_cmd_rsp(const struct nble_response *par);

void nble_get_version_req(void *user_data);

struct nble_version_response {
	struct version_header version;
	void *user_data;
};

void on_nble_get_version_rsp(const struct nble_version_response *par);

void nble_gap_dtm_init_req(void *user_data);

struct nble_gap_tx_power_params {
	int8_t tx_power;
};

void nble_gap_tx_power_req(const struct nble_gap_tx_power_params *params);

struct nble_gap_connect_evt {
	uint16_t conn_handle;
	struct nble_gap_connection_values conn_values;
	/* 0 if connected as master, otherwise as slave */
	uint8_t role_slave;
	/* Address of peer device */
	bt_addr_le_t peer_bda;
};

void on_nble_gap_connect_evt(const struct nble_gap_connect_evt *ev);

struct nble_gap_disconnect_evt {
	uint16_t conn_handle;
	uint8_t hci_reason;
};

void on_nble_gap_disconnect_evt(const struct nble_gap_disconnect_evt *ev);

struct nble_gap_conn_update_evt {
	uint16_t conn_handle;
	struct nble_gap_connection_values conn_values;
};

void on_nble_gap_conn_update_evt(const struct nble_gap_conn_update_evt *ev);

struct nble_gap_adv_report_evt {
	bt_addr_le_t addr;
	int8_t rssi;
	uint8_t adv_type;
};

struct nble_gap_dir_adv_timeout_evt {
	uint16_t conn_handle;
	uint16_t error;
};

void on_nble_gap_dir_adv_timeout_evt(const struct nble_gap_dir_adv_timeout_evt *evt);

#define BLE_GAP_RSSI_EVT_SIZE	32
struct nble_gap_rssi_evt {
	uint16_t conn_handle;
	int8_t rssi_data[BLE_GAP_RSSI_EVT_SIZE];
};

void on_nble_gap_rssi_evt(const struct nble_gap_rssi_evt *ev);

struct nble_gap_sm_passkey_req_evt {
	uint16_t conn_handle;
	uint8_t key_type;
};

void on_nble_gap_sm_passkey_req_evt(const struct nble_gap_sm_passkey_req_evt *);

struct nble_gap_sm_passkey_disp_evt {
	uint16_t conn_handle;
	uint32_t passkey;
};

void on_nble_gap_sm_passkey_display_evt(const struct nble_gap_sm_passkey_disp_evt *evt);

struct nble_link_sec {
	bt_security_t sec_level;
	uint8_t enc_size;
};

struct nble_gap_sm_status_evt {
	uint16_t conn_handle;
	uint8_t evt_type;
	int status;
	struct nble_link_sec enc_link_sec;
};

void on_nble_gap_sm_status_evt(const struct nble_gap_sm_status_evt *ev);

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

struct nble {
	/* Local Bluetooth LE Device Address */
	bt_addr_le_t addr;
	const struct bt_conn_auth_cb *auth;
	bool keep_adv;
};

extern struct nble nble;
