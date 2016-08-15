/*
 * Copyright (c) 2016 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
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

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <toolchain.h>

#include "defines.h"
#include "ticker.h"
#include "mem.h"
#include "ecb.h"
#include "ccm.h"
#include "radio.h"
#include "pdu.h"
#include "ctrl.h"
#include "ll.h"

#include "hci.h"

#include "debug.h"

#define HCI_PACKET_SIZE_MAX 255

enum {
	HCI_CMD = 0x01,
	HCI_DATA = 0x02,
	HCI_EVT = 0x04,
};

enum {
	HCI_OGF_LINK_CONTROL = 0x01,
	HCI_OGF_CONTROL_AND_BASEBAND = 0x03,
	HCI_OGF_INFORMATIONAL = 0x04,
	HCI_OGF_LE_CONTROLLER = 0x08,
	HCI_OGF_VENDOR_SPECIFIC = 0x3F,
};

enum {
	HCI_OCF_DISCONNECT = 0x0006,
	HCI_OCF_READ_REMOTE_VERSION_INFO = 0x001D,
};

enum {
	HCI_OCF_SET_EVENT_MASK = 0x0001,
	HCI_OCF_RESET = 0x0003,
	HCI_OCF_DELETE_STORED_LINK_KEY = 0x0012,
	HCI_OCF_READ_LOCAL_NAME = 0x0014,
	HCI_OCF_READ_CLASS_OF_DEVICE = 0x0023,
	HCI_OCF_READ_VOICE_SETTING = 0x0025,
	HCI_OCF_WRITE_LE_HOST_SUPPORTED = 0x006D,
};

enum {
	HCI_OCF_READ_LOCAL_VERSION = 0x0001,
	HCI_OCF_READ_LOCAL_SUPPORTED_COMMANDS = 0x0002,
	HCI_OCF_READ_LOCAL_SUPPORTED_FEATURES = 0x0003,
	HCI_OCF_READ_BUFFER_SIZE = 0x0005,
	HCI_OCF_READ_BD_ADDR = 0x0009,
};

enum {
	HCI_OCF_LE_SET_EVENT_MASK = 0x0001,
	HCI_OCF_LE_READ_BUFFER_SIZE = 0x0002,
	HCI_OCF_LE_READ_LOCAL_SUPPORTED_FEATURES = 0x0003,
	HCI_OCF_LE_SET_RANDOM_ADDRESS = 0x0005,
	HCI_OCF_LE_SET_ADV_PARAMS = 0x0006,
	HCI_OCF_LE_READ_ADV_CHL_TX_POWER = 0x0007,
	HCI_OCF_LE_SET_ADV_DATA = 0x0008,
	HCI_OCF_LE_SET_SCAN_RESP_DATA = 0x0009,
	HCI_OCF_LE_SET_ADV_ENABLE = 0x000A,
	HCI_OCF_LE_SET_SCAN_PARAMS = 0x000B,
	HCI_OCF_LE_SET_SCAN_ENABLE = 0x000C,
	HCI_OCF_LE_CREATE_CONNECTION = 0x000D,
	HCI_OCF_LE_CREATE_CONNECTION_CANCEL = 0x000E,
	HCI_OCF_LE_READ_WHITELIST_SIZE = 0x000F,
	HCI_OCF_LE_CLEAR_WHITELIST = 0x0010,
	HCI_OCF_LE_ADD_DEVICE_TO_WHITELIST = 0x0011,
	HCI_OCF_LE_CONNECTION_UPDATE = 0x0013,
	HCI_OCF_LE_SET_HOST_CHL_CLASSN = 0x0014,
	HCI_OCF_LE_READ_REMOTE_USED_FEATURES = 0x0016,
	HCI_OCF_LE_ENCRYPT = 0x0017,
	HCI_OCF_LE_RAND = 0x0018,
	HCI_OCF_LE_START_ENCRYPTION = 0x0019,
	HCI_OCF_LE_LTK_REQUEST_REPLY = 0x001A,
	HCI_OCF_LE_LTK_NEGATIVE_REPLY = 0x001B,
	HCI_OCF_LE_READ_SUPPORTED_STATES = 0x001C,
	HCI_OCF_LE_REMOTE_CONN_PARAM_REQ_REPLY = 0x0020,
	HCI_OCF_LE_REMOTE_CONN_PARAM_REQ_NEG_REPLY = 0x0021,
	HCI_OCF_LE_SET_DATA_LENGTH = 0x0022,
	HCI_OCF_LE_READ_SUGGESTED_DEFAULT_DATA_LENGTH = 0x0023,
	HCI_OCF_LE_WRITE_SUGGESTED_DEFAULT_DATA_LENGTH = 0x0024,
	HCI_OCF_LE_READ_MAXIMUM_DATA_LENGTH = 0x002F,
};

enum {
	HCI_OCF_NRF_SET_BD_ADDR = 0x0003,
	HCI_OCF_NRF_CONFIG_ACTIVE_SIGNAL = 0x0005,
};

struct __packed hci_cmd_opcode {
	uint16_t ocf:10;
	uint16_t ogf:6;
};

struct __packed hci_cmd_disconnect {
	uint16_t handle;
	uint8_t reason;
};

struct __packed hci_cmd_le_set_rnd_addr {
	uint8_t addr[BDADDR_SIZE];
};

struct __packed hci_cmd_le_set_adv_params {
	uint16_t interval_min;
	uint16_t interval_max;
	uint8_t type;
	uint8_t own_addr_type;
	uint8_t direct_addr_type;
	uint8_t direct_addr[BDADDR_SIZE];
	uint8_t channel_map;
	uint8_t filter_policy;
};

struct __packed hci_cmd_le_set_adv_data {
	uint8_t len;
	uint8_t data[31];
};

struct __packed hci_cmd_le_set_scan_data {
	uint8_t len;
	uint8_t data[31];
};

struct __packed hci_cmd_le_set_adv_enable {
	uint8_t enable;
};

struct __packed hci_cmd_le_set_scan_params {
	uint8_t type;
	uint16_t interval;
	uint16_t window;
	uint8_t own_addr_type;
	uint8_t filter_policy;
};

struct __packed hci_cmd_le_set_scan_enable {
	uint8_t enable;
};

struct __packed hci_cmd_le_create_conn {
	uint16_t scan_interval;
	uint16_t scan_window;
	uint8_t filter_policy;
	uint8_t peer_addr_type;
	uint8_t peer_addr[BDADDR_SIZE];
	uint8_t own_addr_type;
	uint16_t interval_min;
	uint16_t interval_max;
	uint16_t latency;
	uint16_t timeout;
	uint16_t min_ce_len;
	uint16_t max_ce_len;
};

struct __packed hci_cmd_le_add_dev_to_wlist {
	uint8_t addr_type;
	uint8_t addr[BDADDR_SIZE];
};

struct __packed hci_cmd_le_conn_update {
	uint16_t handle;
	uint16_t interval_min;
	uint16_t interval_max;
	uint16_t latency;
	uint16_t timeout;
	uint16_t min_ce_len;
	uint16_t max_ce_len;
};

struct __packed hci_cmd_le_set_host_chl_classn {
	uint8_t channel_map[5];
};

struct __packed hci_cmd_le_read_remote_used_feats {
	uint16_t handle;
};

struct __packed hci_cmd_le_encrypt {
	uint8_t key[16];
	uint8_t plaintext[16];
};

struct __packed hci_cmd_le_start_encryption {
	uint16_t handle;
	uint8_t rand[8];
	uint8_t ediv[2];
	uint8_t ltk[16];
};

struct __packed hci_cmd_le_ltk_reply {
	uint16_t handle;
	uint8_t ltk[16];
};

struct __packed hci_cmd_le_ltk_neg_reply {
	uint16_t handle;
};

struct __packed hci_cmd_le_remote_conn_param_req_reply {
	uint16_t handle;
	uint16_t interval_min;
	uint16_t interval_max;
	uint16_t latency;
	uint16_t timeout;
	uint16_t min_ce_len;
	uint16_t max_ce_len;
};

struct __packed hci_cmd_le_remote_conn_param_req_neg_reply {
	uint16_t handle;
	uint8_t reason;
};

struct __packed hci_cmd_le_set_data_length {
	uint16_t handle;
	uint16_t tx_octets;
	uint16_t tx_time;
};

struct __packed hci_cmd_nrf_set_bd_addr {
	uint8_t addr[BDADDR_SIZE];
};

struct __packed hci_cmd_nrf_cfg_active_sig {
	uint8_t state;
	uint8_t distance;
};

struct __packed hci_cmd {
	struct hci_cmd_opcode opcode;
	uint8_t len;
	union __packed {
		struct hci_cmd_disconnect disconnect;
		struct hci_cmd_le_set_rnd_addr le_set_rnd_addr;
		struct hci_cmd_le_set_adv_params le_set_adv_params;
		struct hci_cmd_le_set_adv_data le_set_adv_data;
		struct hci_cmd_le_set_scan_data le_set_scan_data;
		struct hci_cmd_le_set_adv_enable le_set_adv_enable;
		struct hci_cmd_le_set_scan_params le_set_scan_params;
		struct hci_cmd_le_set_scan_enable le_set_scan_enable;
		struct hci_cmd_le_create_conn le_create_conn;
		struct hci_cmd_le_add_dev_to_wlist le_add_dev_to_wlist;
		struct hci_cmd_le_conn_update le_conn_update;
		struct hci_cmd_le_set_host_chl_classn le_set_host_chl_classn;

		struct hci_cmd_le_read_remote_used_feats
			le_read_remote_used_feats;
		struct hci_cmd_le_encrypt le_encrypt;
		struct hci_cmd_le_start_encryption le_start_encryption;
		struct hci_cmd_le_ltk_reply le_ltk_reply;
		struct hci_cmd_le_ltk_neg_reply le_ltk_neg_reply;

		struct hci_cmd_le_remote_conn_param_req_reply
			le_remote_conn_param_req_reply;

		struct hci_cmd_le_remote_conn_param_req_neg_reply
			le_remote_conn_param_req_neg_reply;
		struct hci_cmd_le_set_data_length le_set_data_length;
		struct hci_cmd_nrf_set_bd_addr nrf_set_bd_addr;

		struct hci_cmd_nrf_cfg_active_sig
			nrf_cfg_active_sig;
	} params;
};

/*****************************************************************************
 * HCI EVENTS
*****************************************************************************/
enum {
	HCI_EVT_CODE_DISCONNECTION_COMPLETE = 0x05,
	HCI_EVT_CODE_ENCRYPTION_CHANGE = 0x08,
	HCI_EVT_CODE_READ_REMOTE_VERSION_INFO_COMPLETE = 0x0C,
	HCI_EVT_CODE_COMMAND_COMPLETE = 0x0E,
	HCI_EVT_CODE_COMMAND_STATUS = 0x0F,
	HCI_EVT_CODE_NUM_COMPLETE = 0x13,
	HCI_EVT_CODE_ENCRYPTION_KEY_REFRESH_COMPLETE = 0x30,
	HCI_EVT_CODE_LE_META = 0x3E,
	HCI_EVT_CODE_APTO_EXPIRED = 0x57,
};

enum {
	HCI_EVT_ERROR_CODE_SUCCESS = 0x00,
	HCI_EVT_ERROR_CODE_UNKNOWN_HCI_COMMAND = 0x01,
	HCI_EVT_ERROR_CODE_PIN_OR_KEY_MISSING = 0x06,
	HCI_EVT_ERROR_CODE_MEM_CAPACITY_EXCEEDED = 0x07,
	HCI_EVT_ERROR_CODE_COMMAND_DISALLOWED = 0x0C,
};

struct __packed hci_evt_cmd_cmplt_unknown_hci_command {
	uint8_t status;
};

struct __packed hci_evt_disconnect_cmplt {
	uint8_t status;
	uint16_t conn_handle;
	uint8_t reason;
};

struct __packed hci_evt_encryption_change {
	uint8_t status;
	uint16_t conn_handle;
	uint8_t enabled;
};

struct __packed hci_evt_read_remote_version_info_cmplt {
	uint8_t status;
	uint16_t conn_handle;
	uint8_t version_number;
	uint16_t company_id;
	uint16_t sub_version_number;
};

struct __packed hci_evt_cmd_cmplt_set_event_mask {
	uint8_t status;
};

struct __packed hci_evt_cmd_cmplt_reset {
	uint8_t status;
};

struct __packed hci_evt_cmd_cmplt_delete_stored_link_key {
	uint8_t status;
	uint8_t num_keys_deleted;
};

struct __packed hci_evt_cmd_cmplt_read_local_name {
	uint8_t status;
	uint8_t local_name[1];
};

struct __packed hci_evt_cmd_cmplt_read_class_of_device {
	uint8_t status;
	uint8_t class_of_device[3];
};

struct __packed hci_evt_cmd_cmplt_read_voice_setting {
	uint8_t status;
	uint8_t voice_setting[2];
};

struct __packed hci_evt_cmd_cmplt_read_local_version {
	uint8_t status;
	uint8_t hci_version;
	uint16_t hci_revision;
	uint8_t lmp_version;
	uint16_t manufacturer_name;
	uint16_t lmp_subversion;
};

struct __packed hci_evt_cmd_cmplt_read_local_sup_cmds {
	uint8_t status;
	uint8_t value[64];
};

struct __packed hci_evt_cmd_cmplt_rd_local_sup_features {
	uint8_t status;
	uint8_t features[8];
};

struct __packed hci_evt_cmd_cmplt_read_buffer_size {
	uint8_t status;
	uint16_t acl_data_length;
	uint8_t sco_data_length;
	uint16_t num_acl_data;
	uint16_t num_sco_data;
};

struct __packed hci_evt_cmd_cmplt_read_bd_addr {
	uint8_t status;
	uint8_t bd_addr[6];
};

struct __packed hci_evt_cmd_cmplt_le_set_event_mask {
	uint8_t status;
};

struct __packed hci_evt_cmd_cmplt_le_read_buffer_size {
	uint8_t status;
	uint16_t acl_data_length;
	uint8_t acl_data_num;
};

struct __packed hci_evt_cmd_cmplt_le_rd_loc_sup_features {
	uint8_t status;
	uint8_t features[8];
};

struct __packed hci_evt_cmd_cmplt_le_set_rnd_addr {
	uint8_t status;
};

struct __packed hci_evt_cmd_cmplt_le_set_adv_params {
	uint8_t status;
};

struct __packed hci_evt_cmd_cmplt_le_rd_adv_chl_tx_pwr {
	uint8_t status;
	uint8_t transmit_power_level;
};

struct __packed hci_evt_cmd_cmplt_le_set_adv_data {
	uint8_t status;
};

struct __packed hci_evt_cmd_cmplt_le_set_scan_resp_data {
	uint8_t status;
};

struct __packed hci_evt_cmd_cmplt_le_set_adv_enable {
	uint8_t status;
};

struct __packed hci_evt_cmd_cmplt_le_set_scan_params {
	uint8_t status;
};

struct __packed hci_evt_cmd_cmplt_le_set_scan_enable {
	uint8_t status;
};

struct __packed hci_evt_cmd_cmplt_le_create_conn_cancel {
	uint8_t status;
};

struct __packed hci_evt_cmd_cmplt_le_read_whitelist_size {
	uint8_t status;
	uint8_t whitelist_size;
};

struct __packed hci_evt_cmd_cmplt_le_clear_whitelist {
	uint8_t status;
};

struct __packed hci_evt_cmd_cmplt_le_add_device_to_whitelist {
	uint8_t status;
};

struct __packed hci_evt_cmd_cmplt_le_set_host_chl_classn {
	uint8_t status;
};

struct __packed hci_evt_cmd_cmplt_le_encrypt {
	uint8_t status;
	uint8_t encrypted[16];
};

struct __packed hci_evt_cmd_cmplt_le_rand {
	uint8_t status;
	uint8_t rand[8];
};

struct __packed hci_evt_cmd_cmplt_le_ltk_reply {
	uint8_t status;
	uint16_t conn_handle;
};

struct __packed hci_evt_cmd_cmplt_le_ltk_negative_reply {
	uint8_t status;
	uint16_t conn_handle;
};

struct __packed hci_evt_cmd_cmplt_le_read_supported_states {
	uint8_t status;
	uint64_t le_states;
};

struct __packed hci_evt_cmd_cmplt_le_remote_conn_param_req_reply {
	uint8_t status;
	uint16_t conn_handle;
};

struct __packed hci_evt_cmd_cmplt_le_remote_conn_param_req_neg_reply {
	uint8_t status;
	uint16_t conn_handle;
};

struct __packed hci_evt_cmd_cmplt_le_set_data_length {
	uint8_t status;
	uint16_t conn_handle;
};

struct __packed hci_evt_cmd_cmplt_nrf_set_bd_addr {
	uint8_t status;
};

struct __packed hci_evt_cmd_cmplt_nrf_cfg_active_sig {
	uint8_t status;
};

struct __packed hci_evt_cmd_cmplt {
	uint8_t num_cmd_pkt;
	struct hci_cmd_opcode opcode;
	union __packed hci_evt_cmd_cmplt_params {

		struct hci_evt_cmd_cmplt_unknown_hci_command
			unknown_hci_command;
		struct hci_evt_cmd_cmplt_reset reset;
		struct hci_evt_cmd_cmplt_set_event_mask set_event_mask;

		struct hci_evt_cmd_cmplt_delete_stored_link_key
			delete_stored_link_key;
		struct hci_evt_cmd_cmplt_read_local_name read_local_name;

		struct hci_evt_cmd_cmplt_read_class_of_device
			read_class_of_device;

		struct hci_evt_cmd_cmplt_read_voice_setting
			read_voice_setting;

		struct hci_evt_cmd_cmplt_read_local_version
			read_local_version;

		struct hci_evt_cmd_cmplt_read_local_sup_cmds
			read_local_sup_cmds;

		struct hci_evt_cmd_cmplt_rd_local_sup_features
			rd_local_sup_features;
		struct hci_evt_cmd_cmplt_read_buffer_size read_buffer_size;
		struct hci_evt_cmd_cmplt_read_bd_addr read_bd_addr;

		struct hci_evt_cmd_cmplt_le_read_buffer_size
			le_read_buffer_size;

		struct hci_evt_cmd_cmplt_le_rd_loc_sup_features
			le_rd_loc_sup_features;
		struct hci_evt_cmd_cmplt_le_set_rnd_addr le_set_rnd_addr;
		struct hci_evt_cmd_cmplt_le_set_adv_params le_set_adv_params;

		struct hci_evt_cmd_cmplt_le_rd_adv_chl_tx_pwr
			le_rd_adv_chl_tx_pwr;
		struct hci_evt_cmd_cmplt_le_set_adv_data le_set_adv_data;
		struct hci_evt_cmd_cmplt_le_set_scan_resp_data
			le_set_scan_resp_data;
		struct hci_evt_cmd_cmplt_le_set_adv_enable le_set_adv_enable;

		struct hci_evt_cmd_cmplt_le_set_scan_params
			le_set_scan_params;

		struct hci_evt_cmd_cmplt_le_set_scan_enable
			le_set_scan_enable;

		struct hci_evt_cmd_cmplt_le_create_conn_cancel
			le_create_conn_cancel;

		struct hci_evt_cmd_cmplt_le_read_whitelist_size
			le_read_whitelist_size;

		struct hci_evt_cmd_cmplt_le_clear_whitelist
			le_clear_whitelist;

		struct hci_evt_cmd_cmplt_le_add_device_to_whitelist
			le_add_dev_to_wlist;

		struct hci_evt_cmd_cmplt_le_set_host_chl_classn
			le_set_host_chl_classn;
		struct hci_evt_cmd_cmplt_le_encrypt le_encrypt;
		struct hci_evt_cmd_cmplt_le_rand le_rand;
		struct hci_evt_cmd_cmplt_le_ltk_reply le_ltk_reply;

		struct hci_evt_cmd_cmplt_le_ltk_negative_reply
			le_ltk_neg_reply;

		struct hci_evt_cmd_cmplt_le_read_supported_states
			le_read_supported_states;

		struct hci_evt_cmd_cmplt_le_remote_conn_param_req_reply
			le_remote_conn_param_req_reply;
		struct hci_evt_cmd_cmplt_le_remote_conn_param_req_neg_reply
			le_remote_conn_param_req_neg_reply;

		struct hci_evt_cmd_cmplt_le_set_data_length
			le_set_data_length;
		struct hci_evt_cmd_cmplt_nrf_set_bd_addr nrf_set_bd_addr;

		struct hci_evt_cmd_cmplt_nrf_cfg_active_sig
			nrf_cfg_active_sig;
	} params;
};

struct __packed hci_evt_cmd_status {
	uint8_t status;
	uint8_t num_cmd_pkt;
	struct hci_cmd_opcode opcode;
};

struct __packed hci_evt_num_cmplt {
	uint8_t num_handles;
	uint8_t handles_nums[1];
};

struct __packed hci_evt_encryption_key_refresh_cmplt {
	uint8_t status;
	uint16_t conn_handle;
};

enum {
	HCI_EVT_LE_META_CONNECTION_COMPLETE = 0x01,
	HCI_EVT_LE_META_ADV_REPORT,
	HCI_EVT_LE_META_CONNECTION_UPDATE_COMPLETE,
	HCI_EVT_LE_META_READ_REMOTE_USED_FEATURE_COMPLETE,
	HCI_EVT_LE_META_LONG_TERM_KEY_REQUEST,
	HCI_EVT_LE_META_REMOTE_CONNECTION_PARAMETER_REQUEST,
	HCI_EVT_LE_META_LENGTH_CHANGE,
};

struct __packed hci_evt_le_meta_conn_complete {
	uint8_t status;
	uint16_t conn_handle;
	uint8_t role;
	uint8_t addr_type;
	uint8_t addr[BDADDR_SIZE];
	uint16_t interval;
	uint16_t latency;
	uint16_t timeout;
	uint8_t mca;
};

struct __packed hci_evt_le_meta_adv_report {
	uint8_t num_reports;
	uint8_t reports[1];
};

struct __packed hci_evt_le_meta_conn_update_complete {
	uint8_t status;
	uint16_t conn_handle;
	uint16_t interval;
	uint16_t latency;
	uint16_t timeout;
};

struct __packed hci_evt_le_meta_read_remote_used_features {
	uint8_t status;
	uint16_t conn_handle;
	uint8_t features[8];
};

struct __packed hci_evt_le_meta_long_term_key_request {
	uint16_t conn_handle;
	uint8_t rand[8];
	uint8_t ediv[2];
};

struct __packed hci_evt_le_meta_remote_conn_param_request {
	uint16_t conn_handle;
	uint16_t interval_min;
	uint16_t interval_max;
	uint16_t latency;
	uint16_t timeout;
};

struct __packed hci_evt_le_meta_length_change {
	uint16_t conn_handle;
	uint16_t max_tx_octets;
	uint16_t max_tx_time;
	uint16_t max_rx_octets;
	uint16_t max_rx_time;
};

struct __packed hci_evt_le_meta {
	uint8_t subevent_code;
	union __packed {
		struct hci_evt_le_meta_conn_complete conn_cmplt;
		struct hci_evt_le_meta_adv_report adv_report;
		struct hci_evt_le_meta_conn_update_complete conn_update_cmplt;

		struct hci_evt_le_meta_read_remote_used_features
			remote_used_features;

		struct hci_evt_le_meta_long_term_key_request
			long_term_key_request;

		struct hci_evt_le_meta_remote_conn_param_request
			remote_conn_param_request;
		struct hci_evt_le_meta_length_change length_change;
	} subevent;
};

struct __packed hci_evt_apto_expired {
	uint16_t conn_handle;
};

struct __packed hci_evt {
	uint8_t code;
	uint8_t len;
	union __packed {
		struct hci_evt_disconnect_cmplt disconnect_cmplt;
		struct hci_evt_encryption_change encryption_change;

		struct hci_evt_read_remote_version_info_cmplt
			read_remote_version_info_cmplt;
		struct hci_evt_cmd_cmplt cmd_cmplt;
		struct hci_evt_cmd_status cmd_status;
		struct hci_evt_num_cmplt num_cmplt;

		struct hci_evt_encryption_key_refresh_cmplt
			encryption_key_refresh_cmplt;
		struct hci_evt_le_meta le_meta;
		struct hci_evt_apto_expired apto_expired;
	} params;
};

struct __packed hci_data {
	uint16_t handle:12;
	uint16_t pb:2;
	uint16_t bc:2;
	uint16_t len;
	uint8_t data[1];
};

static struct {
	uint16_t rx_len;
	uint8_t rx[HCI_PACKET_SIZE_MAX];
	uint8_t tx[HCI_PACKET_SIZE_MAX];
} hci_context;

#define HCI_EVT_LEN(evt) ((uint8_t)(1 + offsetof(struct hci_evt, params) + \
			evt->len))

#define HCI_DATA_LEN(dat)((uint8_t)(1 + offsetof(struct hci_data, data) + \
			       dat->len))

#define HCI_CC_LEN(s)((uint8_t)(offsetof(struct hci_evt_cmd_cmplt, params) + \
			sizeof(struct s)))

static void link_control_cmd_handle(struct hci_cmd *cmd, uint8_t *len)
{
	uint32_t status;
	struct hci_evt *evt;
	struct hci_evt_cmd_status *cs;
	struct hci_evt_cmd_cmplt *cc;
	union hci_evt_cmd_cmplt_params *ccp;

	evt = (struct hci_evt *)&hci_context.tx[1];
	cs = &evt->params.cmd_status;
	cc = &evt->params.cmd_cmplt;
	ccp = &evt->params.cmd_cmplt.params;

	switch (cmd->opcode.ocf) {
	case HCI_OCF_DISCONNECT:

		status = radio_terminate_ind_send(
				cmd->params.disconnect.handle,
				cmd->params.disconnect.reason);

		evt->code = HCI_EVT_CODE_COMMAND_STATUS;
		evt->len = sizeof(struct hci_evt_cmd_status);
		cs->num_cmd_pkt = 1;
		cs->opcode = cmd->opcode;

		cs->status = (!status) ?  HCI_EVT_ERROR_CODE_SUCCESS :
			HCI_EVT_ERROR_CODE_COMMAND_DISALLOWED;

		break;

	case HCI_OCF_READ_REMOTE_VERSION_INFO:

		status = radio_version_ind_send(cmd->params.disconnect.handle);

		evt->code = HCI_EVT_CODE_COMMAND_STATUS;
		evt->len = sizeof(struct hci_evt_cmd_status);
		cs->num_cmd_pkt = 1;
		cs->opcode = cmd->opcode;

		cs->status = (!status) ?
			HCI_EVT_ERROR_CODE_SUCCESS :
			HCI_EVT_ERROR_CODE_COMMAND_DISALLOWED;

		break;

	default:
		evt->code = HCI_EVT_CODE_COMMAND_COMPLETE;
		evt->len = HCI_CC_LEN(hci_evt_cmd_cmplt_unknown_hci_command);
		cc->num_cmd_pkt = 1;
		cc->opcode = cmd->opcode;

		ccp->unknown_hci_command.status =
			HCI_EVT_ERROR_CODE_UNKNOWN_HCI_COMMAND;

		break;
	}

	*len = HCI_EVT_LEN(evt);
}

static void ctrl_bb_cmd_handle(struct hci_cmd *cmd, uint8_t *len)
{
	struct hci_evt *evt;
	struct hci_evt_cmd_cmplt *cc;
	union hci_evt_cmd_cmplt_params *ccp;

	evt = (struct hci_evt *)&hci_context.tx[1];
	cc = &evt->params.cmd_cmplt;
	ccp = &evt->params.cmd_cmplt.params;

	switch (cmd->opcode.ocf) {
	case HCI_OCF_SET_EVENT_MASK:

		/** TODO */

		evt->code = HCI_EVT_CODE_COMMAND_COMPLETE;
		evt->len = HCI_CC_LEN(hci_evt_cmd_cmplt_set_event_mask);
		cc->num_cmd_pkt = 1;
		cc->opcode = cmd->opcode;

		ccp->set_event_mask.status = HCI_EVT_ERROR_CODE_SUCCESS;

		break;

	case HCI_OCF_RESET:

		/** TODO */

		evt->code = HCI_EVT_CODE_COMMAND_COMPLETE;
		evt->len = HCI_CC_LEN(hci_evt_cmd_cmplt_reset);
		cc->num_cmd_pkt = 1;
		cc->opcode = cmd->opcode;

		ccp->reset.status = HCI_EVT_ERROR_CODE_SUCCESS;

		break;

	default:
		evt->code = HCI_EVT_CODE_COMMAND_COMPLETE;
		evt->len = HCI_CC_LEN(hci_evt_cmd_cmplt_unknown_hci_command);
		cc->num_cmd_pkt = 1;
		cc->opcode = cmd->opcode;

		ccp->unknown_hci_command.status =
			HCI_EVT_ERROR_CODE_UNKNOWN_HCI_COMMAND;

		break;
	}

	*len = HCI_EVT_LEN(evt);
}

static void info_cmd_handle(struct hci_cmd *cmd, uint8_t *len)
{
	struct hci_evt *evt;
	struct hci_evt_cmd_cmplt *cc;
	union hci_evt_cmd_cmplt_params *ccp;

	evt = (struct hci_evt *)&hci_context.tx[1];
	cc = &evt->params.cmd_cmplt;
	ccp = &evt->params.cmd_cmplt.params;

	switch (cmd->opcode.ocf) {
	case HCI_OCF_READ_LOCAL_VERSION:
		evt->code = HCI_EVT_CODE_COMMAND_COMPLETE;
		evt->len = HCI_CC_LEN(hci_evt_cmd_cmplt_read_local_version);
		cc->num_cmd_pkt = 1;
		cc->opcode = cmd->opcode;

		ccp->read_local_version.status = HCI_EVT_ERROR_CODE_SUCCESS;
		ccp->read_local_version.hci_version = 0;
		ccp->read_local_version.hci_revision = 0;
		ccp->read_local_version.lmp_version = RADIO_BLE_VERSION_NUMBER;
		ccp->read_local_version.manufacturer_name =
			RADIO_BLE_COMPANY_ID;
		ccp->read_local_version.lmp_subversion =
			RADIO_BLE_SUB_VERSION_NUMBER;
		break;

	case HCI_OCF_READ_LOCAL_SUPPORTED_COMMANDS:
		evt->code = HCI_EVT_CODE_COMMAND_COMPLETE;
		evt->len = HCI_CC_LEN(hci_evt_cmd_cmplt_read_local_sup_cmds);
		cc->num_cmd_pkt = 1;
		cc->opcode = cmd->opcode;

		ccp->read_local_sup_cmds.status = HCI_EVT_ERROR_CODE_SUCCESS;
		memset(&ccp->read_local_sup_cmds.value[0], 0,
			sizeof(ccp->read_local_sup_cmds.value));
		/* Disconnect. */
		ccp->read_local_sup_cmds.value[0] = (1 << 5);
		/* Set Event Mask, and Reset. */
		ccp->read_local_sup_cmds.value[5] = (1 << 6) | (1 << 7);
		/* Read Local Version Info, Read Local Supported Features. */
		ccp->read_local_sup_cmds.value[14] = (1 << 3) | (1 << 5);
		/* Read BD ADDR. */
		ccp->read_local_sup_cmds.value[15] = (1 << 1);
		/* All LE commands in this octet. */
		ccp->read_local_sup_cmds.value[25] = 0xF7;
		/* All LE commands in this octet. */
		ccp->read_local_sup_cmds.value[26] = 0xFF;
		/* All LE commands in this octet,
		 * except LE Remove Device From White List
		 */
		ccp->read_local_sup_cmds.value[27] = 0xFD;
		/* LE Start Encryption, LE Long Term Key Req Reply,
		 * and LE Long Term Key Req Neg Reply.
		 */
		ccp->read_local_sup_cmds.value[28] = (1 << 0) | (1 << 1) |
							(1 << 2);
		/* LE Remote Conn Param Req and Neg Reply, LE Set Data Length,
		 * and LE Read Suggested Data Length.
		 */
		ccp->read_local_sup_cmds.value[33] = (1 << 4) | (1 << 5) |
							(1 << 6) | (1 << 7);
		/* LE Write Suggested Data Length. */
		ccp->read_local_sup_cmds.value[34] = (1 << 0);
		/* LE Read Maximum Data Length. */
		ccp->read_local_sup_cmds.value[35] = (1 << 3);
		break;

	case HCI_OCF_READ_LOCAL_SUPPORTED_FEATURES:
		evt->code = HCI_EVT_CODE_COMMAND_COMPLETE;
		evt->len = HCI_CC_LEN(hci_evt_cmd_cmplt_rd_local_sup_features);
		cc->num_cmd_pkt = 1;
		cc->opcode = cmd->opcode;

		ccp->rd_local_sup_features.status = HCI_EVT_ERROR_CODE_SUCCESS;
		memset(&ccp->rd_local_sup_features.features[0], 0x00,
			sizeof(ccp->rd_local_sup_features.features));
		/* BR/EDR not supported and LE supported */
		ccp->rd_local_sup_features.features[4] = (1 << 5) | (1 << 6);
		break;

	case HCI_OCF_READ_BD_ADDR:
		evt->code = HCI_EVT_CODE_COMMAND_COMPLETE;
		evt->len = HCI_CC_LEN(hci_evt_cmd_cmplt_read_bd_addr);
		cc->num_cmd_pkt = 1;
		cc->opcode = cmd->opcode;

		ccp->read_bd_addr.status = HCI_EVT_ERROR_CODE_SUCCESS;

		ll_address_get(0, &ccp->read_bd_addr.bd_addr[0]);
		break;

	default:
		evt->code = HCI_EVT_CODE_COMMAND_COMPLETE;
		evt->len = HCI_CC_LEN(hci_evt_cmd_cmplt_unknown_hci_command);
		cc->num_cmd_pkt = 1;
		cc->opcode = cmd->opcode;

		ccp->unknown_hci_command.status =
			 HCI_EVT_ERROR_CODE_UNKNOWN_HCI_COMMAND;
		break;
	}

	*len = HCI_EVT_LEN(evt);
}

static void controller_cmd_handle(struct hci_cmd *cmd, uint8_t *len,
		uint8_t **out)
{
	uint32_t status;
	uint32_t error_code;
	struct hci_evt *evt;
	uint8_t const c_adv_type[] = {
		PDU_ADV_TYPE_ADV_IND, PDU_ADV_TYPE_DIRECT_IND,
		PDU_ADV_TYPE_SCAN_IND, PDU_ADV_TYPE_NONCONN_IND };
	struct hci_evt_cmd_cmplt *cc;
	union hci_evt_cmd_cmplt_params *ccp;
	struct hci_cmd_le_create_conn *le_cc;
	struct hci_cmd_le_remote_conn_param_req_reply *le_cp_req_rep;
	struct hci_cmd_le_remote_conn_param_req_neg_reply *le_cp_req_neg_rep;

	evt = (struct hci_evt *)&hci_context.tx[1];
	cc = &evt->params.cmd_cmplt;
	ccp = &evt->params.cmd_cmplt.params;
	switch (cmd->opcode.ocf) {

	case HCI_OCF_LE_SET_EVENT_MASK:
		evt->code = HCI_EVT_CODE_COMMAND_COMPLETE;
		evt->len = HCI_CC_LEN(hci_evt_cmd_cmplt_le_set_event_mask);
		cc->num_cmd_pkt = 1;
		cc->opcode = cmd->opcode;

		ccp->set_event_mask.status = HCI_EVT_ERROR_CODE_SUCCESS;
		break;

	case HCI_OCF_LE_READ_BUFFER_SIZE:
		evt->code = HCI_EVT_CODE_COMMAND_COMPLETE;
		evt->len = HCI_CC_LEN(hci_evt_cmd_cmplt_le_read_buffer_size);
		cc->num_cmd_pkt = 1;
		cc->opcode = cmd->opcode;

		ccp->le_read_buffer_size.status = HCI_EVT_ERROR_CODE_SUCCESS;

		ccp->le_read_buffer_size.acl_data_length =
			RADIO_LL_LENGTH_OCTETS_RX_MAX;
		ccp->le_read_buffer_size.acl_data_num =
			RADIO_PACKET_COUNT_TX_MAX;
		break;

	case HCI_OCF_LE_READ_LOCAL_SUPPORTED_FEATURES:
		evt->code = HCI_EVT_CODE_COMMAND_COMPLETE;
		evt->len = HCI_CC_LEN(hci_evt_cmd_cmplt_le_rd_loc_sup_features);
		cc->num_cmd_pkt = 1;
		cc->opcode = cmd->opcode;

		ccp->le_rd_loc_sup_features.status = HCI_EVT_ERROR_CODE_SUCCESS;

		memset(&ccp->le_rd_loc_sup_features.features[0], 0x00,
			sizeof(ccp->le_rd_loc_sup_features.features));
		ccp->le_rd_loc_sup_features.features[0] = RADIO_BLE_FEATURES;
		break;

	case HCI_OCF_LE_SET_RANDOM_ADDRESS:
		ll_address_set(1, &cmd->params.le_set_rnd_addr.addr[0]);

		evt->code = HCI_EVT_CODE_COMMAND_COMPLETE;
		evt->len = HCI_CC_LEN(hci_evt_cmd_cmplt_le_set_rnd_addr);
		cc->num_cmd_pkt = 1;
		cc->opcode = cmd->opcode;

		ccp->le_set_rnd_addr.status = HCI_EVT_ERROR_CODE_SUCCESS;
		break;

	case HCI_OCF_LE_SET_ADV_PARAMS:

		ll_adv_params_set(cmd->params.le_set_adv_params.interval_min,
			c_adv_type[cmd->params.le_set_adv_params.type],
			cmd->params.le_set_adv_params.own_addr_type,
			cmd->params.le_set_adv_params.direct_addr_type,
			&cmd->params.le_set_adv_params.direct_addr[0],
			cmd->params.le_set_adv_params.channel_map,
			cmd->params.le_set_adv_params.filter_policy);

		evt->code = HCI_EVT_CODE_COMMAND_COMPLETE;
		evt->len = HCI_CC_LEN(hci_evt_cmd_cmplt_le_set_adv_params);
		cc->num_cmd_pkt = 1;
		cc->opcode = cmd->opcode;

		ccp->le_set_adv_params.status = HCI_EVT_ERROR_CODE_SUCCESS;
		break;

	case HCI_OCF_LE_READ_ADV_CHL_TX_POWER:
		evt->code = HCI_EVT_CODE_COMMAND_COMPLETE;
		evt->len = HCI_CC_LEN(hci_evt_cmd_cmplt_le_rd_adv_chl_tx_pwr);
		cc->num_cmd_pkt = 1;
		cc->opcode = cmd->opcode;

		ccp->le_rd_adv_chl_tx_pwr.status = HCI_EVT_ERROR_CODE_SUCCESS;
		ccp->le_rd_adv_chl_tx_pwr.transmit_power_level = 0;
		break;

	case HCI_OCF_LE_SET_ADV_DATA:
		ll_adv_data_set(cmd->params.le_set_adv_data.len,
				&cmd->params.le_set_adv_data.data[0]);

		evt->code = HCI_EVT_CODE_COMMAND_COMPLETE;
		evt->len = HCI_CC_LEN(hci_evt_cmd_cmplt_le_set_adv_data);
		cc->num_cmd_pkt = 1;
		cc->opcode = cmd->opcode;

		ccp->le_set_adv_data.status = HCI_EVT_ERROR_CODE_SUCCESS;
		break;

	case HCI_OCF_LE_SET_SCAN_RESP_DATA:
		ll_scan_data_set(cmd->params.le_set_scan_data.len,
					&cmd->params.le_set_scan_data.data[0]);

		evt->code = HCI_EVT_CODE_COMMAND_COMPLETE;
		evt->len = HCI_CC_LEN(hci_evt_cmd_cmplt_le_set_scan_resp_data);
		cc->num_cmd_pkt = 1;
		cc->opcode = cmd->opcode;

		ccp->le_set_scan_resp_data.status = HCI_EVT_ERROR_CODE_SUCCESS;
		break;

	case HCI_OCF_LE_SET_ADV_ENABLE:
		status = ll_adv_enable(cmd->params.le_set_adv_enable.enable);

		evt->code = HCI_EVT_CODE_COMMAND_COMPLETE;
		evt->len = HCI_CC_LEN(hci_evt_cmd_cmplt_le_set_adv_enable);
		cc->num_cmd_pkt = 1;
		cc->opcode = cmd->opcode;

		ccp->le_set_adv_enable.status = (status == 0) ?
			HCI_EVT_ERROR_CODE_SUCCESS :
			HCI_EVT_ERROR_CODE_COMMAND_DISALLOWED;
		break;

	case HCI_OCF_LE_SET_SCAN_PARAMS:
		ll_scan_params_set(cmd->params.le_set_scan_params.
				   type,
				   cmd->params.le_set_scan_params.
				   interval,
				   cmd->params.le_set_scan_params.
				   window,
				   cmd->params.le_set_scan_params.
				   own_addr_type,
				   cmd->params.le_set_scan_params.
				   filter_policy);

		evt->code = HCI_EVT_CODE_COMMAND_COMPLETE;
		evt->len = HCI_CC_LEN(hci_evt_cmd_cmplt_le_set_scan_params);
		cc->num_cmd_pkt = 1;
		cc->opcode = cmd->opcode;

		ccp->le_set_scan_params.status = HCI_EVT_ERROR_CODE_SUCCESS;

		break;

	case HCI_OCF_LE_SET_SCAN_ENABLE:
		status = ll_scan_enable(cmd->params.le_set_scan_enable.enable);

		evt->code = HCI_EVT_CODE_COMMAND_COMPLETE;
		evt->len = HCI_CC_LEN(hci_evt_cmd_cmplt_le_set_scan_enable);
		cc->num_cmd_pkt = 1;
		cc->opcode = cmd->opcode;

		ccp->le_set_scan_enable.status = (status == 0) ?
			HCI_EVT_ERROR_CODE_SUCCESS :
			HCI_EVT_ERROR_CODE_COMMAND_DISALLOWED;
		break;

	case HCI_OCF_LE_CREATE_CONNECTION:
		le_cc = &cmd->params.le_create_conn;

		status = ll_create_connection(le_cc->scan_interval,
					 le_cc->scan_window,
					 le_cc->filter_policy,
					 le_cc->peer_addr_type,
					 &le_cc->peer_addr[0],
					 le_cc->own_addr_type,
					 le_cc->interval_max,
					 le_cc->latency,
					 le_cc->timeout);

		evt->code = HCI_EVT_CODE_COMMAND_STATUS;
		evt->len = sizeof(struct hci_evt_cmd_status);
		evt->params.cmd_status.num_cmd_pkt = 1;
		evt->params.cmd_status.opcode = cmd->opcode;

		evt->params.cmd_status.status = (status == 0) ?
			HCI_EVT_ERROR_CODE_SUCCESS :
			HCI_EVT_ERROR_CODE_COMMAND_DISALLOWED;
		break;

	case HCI_OCF_LE_CREATE_CONNECTION_CANCEL:
		status = radio_connect_disable();

		evt->code = HCI_EVT_CODE_COMMAND_COMPLETE;
		evt->len = HCI_CC_LEN(hci_evt_cmd_cmplt_le_create_conn_cancel);
		cc->num_cmd_pkt = 1;
		cc->opcode = cmd->opcode;

		ccp->le_create_conn_cancel.status = (status == 0) ?
			HCI_EVT_ERROR_CODE_SUCCESS :
			HCI_EVT_ERROR_CODE_COMMAND_DISALLOWED;
		break;

	case HCI_OCF_LE_READ_WHITELIST_SIZE:
		evt->code = HCI_EVT_CODE_COMMAND_COMPLETE;
		evt->len = HCI_CC_LEN(hci_evt_cmd_cmplt_le_read_whitelist_size);
		cc->num_cmd_pkt = 1;
		cc->opcode = cmd->opcode;

		ccp->le_read_whitelist_size.status = HCI_EVT_ERROR_CODE_SUCCESS;
		ccp->le_read_whitelist_size.whitelist_size = 8;

		*out = &hci_context.tx[0];
		break;

	case HCI_OCF_LE_CLEAR_WHITELIST:

		radio_filter_clear();

		evt->code = HCI_EVT_CODE_COMMAND_COMPLETE;
		evt->len = HCI_CC_LEN(hci_evt_cmd_cmplt_le_clear_whitelist);
		cc->num_cmd_pkt = 1;
		cc->opcode = cmd->opcode;

		ccp->le_clear_whitelist.status = HCI_EVT_ERROR_CODE_SUCCESS;
		break;

	case HCI_OCF_LE_ADD_DEVICE_TO_WHITELIST:

		error_code = radio_filter_add(cmd->params.le_add_dev_to_wlist.
				     addr_type,
				     &cmd->params.le_add_dev_to_wlist.addr[0]);

		evt->code = HCI_EVT_CODE_COMMAND_COMPLETE;
		evt->len = HCI_CC_LEN(
			hci_evt_cmd_cmplt_le_add_device_to_whitelist);
		cc->num_cmd_pkt = 1;
		cc->opcode = cmd->opcode;

		ccp->le_add_dev_to_wlist.status = (error_code == 0) ?
			HCI_EVT_ERROR_CODE_SUCCESS :
			HCI_EVT_ERROR_CODE_MEM_CAPACITY_EXCEEDED;
		break;

	case HCI_OCF_LE_CONNECTION_UPDATE:
		/** @todo if peer supports LE Conn Param Req,
		* use Req cmd (1) instead of Initiate cmd (0).
		*/
		status = radio_conn_update(cmd->params.le_conn_update.handle,
					   0, 0,
					   cmd->params.le_conn_update.
					   interval_max,
					   cmd->params.le_conn_update.latency,
					   cmd->params.le_conn_update.timeout);

		evt->code = HCI_EVT_CODE_COMMAND_STATUS;
		evt->len = sizeof(struct hci_evt_cmd_status);
		evt->params.cmd_status.num_cmd_pkt = 1;
		evt->params.cmd_status.opcode = cmd->opcode;

		evt->params.cmd_status.status = (status == 0) ?
			HCI_EVT_ERROR_CODE_SUCCESS :
			HCI_EVT_ERROR_CODE_COMMAND_DISALLOWED;
		break;

	case HCI_OCF_LE_SET_HOST_CHL_CLASSN:
		error_code = radio_chm_update(&cmd->params.
				le_set_host_chl_classn.channel_map[0]);

		evt->code = HCI_EVT_CODE_COMMAND_COMPLETE;
		evt->len = HCI_CC_LEN(hci_evt_cmd_cmplt_le_set_host_chl_classn);
		cc->num_cmd_pkt = 1;
		cc->opcode = cmd->opcode;

		ccp->le_set_host_chl_classn.status = (error_code == 0) ?
			HCI_EVT_ERROR_CODE_SUCCESS :
			HCI_EVT_ERROR_CODE_COMMAND_DISALLOWED;
		break;

	case HCI_OCF_LE_READ_REMOTE_USED_FEATURES:
		status = radio_feature_req_send(cmd->params.
					   le_read_remote_used_feats.handle);

		evt->code = HCI_EVT_CODE_COMMAND_STATUS;
		evt->len = sizeof(struct hci_evt_cmd_status);
		evt->params.cmd_status.num_cmd_pkt = 1;
		evt->params.cmd_status.opcode = cmd->opcode;

		evt->params.cmd_status.status = (status == 0) ?
			HCI_EVT_ERROR_CODE_SUCCESS :
			HCI_EVT_ERROR_CODE_COMMAND_DISALLOWED;
		break;

	case HCI_OCF_LE_ENCRYPT:
		evt->code = HCI_EVT_CODE_COMMAND_COMPLETE;
		evt->len = HCI_CC_LEN(hci_evt_cmd_cmplt_le_encrypt);
		cc->num_cmd_pkt = 1;
		cc->opcode = cmd->opcode;

		ccp->le_encrypt.status = HCI_EVT_ERROR_CODE_SUCCESS;
		ecb_encrypt(&cmd->params.le_encrypt.key[0],
				&cmd->params.le_encrypt.plaintext[0],
			    &ccp->le_encrypt.encrypted[0], 0);
		break;

	case HCI_OCF_LE_RAND:
		evt->code = HCI_EVT_CODE_COMMAND_COMPLETE;
		evt->len = HCI_CC_LEN(hci_evt_cmd_cmplt_le_rand);
		cc->num_cmd_pkt = 1;
		cc->opcode = cmd->opcode;

		ccp->le_rand.status = HCI_EVT_ERROR_CODE_SUCCESS;

		/** TODO fill rand */
		break;

	case HCI_OCF_LE_START_ENCRYPTION:
		status = radio_enc_req_send(cmd->params.
				       le_start_encryption.handle,
				       &cmd->params.le_start_encryption.rand[0],
				       &cmd->params.le_start_encryption.ediv[0],
				       &cmd->params.le_start_encryption.ltk[0]);

		evt->code = HCI_EVT_CODE_COMMAND_STATUS;
		evt->len = sizeof(struct hci_evt_cmd_status);
		evt->params.cmd_status.num_cmd_pkt = 1;
		evt->params.cmd_status.opcode = cmd->opcode;

		evt->params.cmd_status.status = (status == 0) ?
			HCI_EVT_ERROR_CODE_SUCCESS :
			HCI_EVT_ERROR_CODE_COMMAND_DISALLOWED;
		break;

	case HCI_OCF_LE_LTK_REQUEST_REPLY:
		status =
		    radio_start_enc_req_send(cmd->params.le_ltk_reply.handle,
					     HCI_EVT_ERROR_CODE_SUCCESS,
					     &cmd->params.
					     le_ltk_reply.ltk[0]);

		evt->code = HCI_EVT_CODE_COMMAND_COMPLETE;
		evt->len = HCI_CC_LEN(hci_evt_cmd_cmplt_le_ltk_reply);
		cc->num_cmd_pkt = 1;
		cc->opcode = cmd->opcode;

		ccp->le_ltk_reply.status = (status == 0) ?
			HCI_EVT_ERROR_CODE_SUCCESS :
			HCI_EVT_ERROR_CODE_COMMAND_DISALLOWED;
		ccp->le_ltk_reply.conn_handle = cmd->params.le_ltk_reply.handle;
		break;

	case HCI_OCF_LE_LTK_NEGATIVE_REPLY:
		status =
		radio_start_enc_req_send(cmd->params.le_ltk_neg_reply.handle,
			HCI_EVT_ERROR_CODE_PIN_OR_KEY_MISSING,
			0);

		evt->code = HCI_EVT_CODE_COMMAND_COMPLETE;
		evt->len = HCI_CC_LEN(hci_evt_cmd_cmplt_le_ltk_negative_reply);
		cc->num_cmd_pkt = 1;
		cc->opcode = cmd->opcode;

		ccp->le_ltk_neg_reply.status = (status == 0) ?
			HCI_EVT_ERROR_CODE_SUCCESS :
			HCI_EVT_ERROR_CODE_COMMAND_DISALLOWED;
		ccp->le_ltk_neg_reply.conn_handle =
		    cmd->params.le_ltk_neg_reply.handle;
		break;

	case HCI_OCF_LE_READ_SUPPORTED_STATES:
		evt->code = HCI_EVT_CODE_COMMAND_COMPLETE;
		evt->len =
			HCI_CC_LEN(hci_evt_cmd_cmplt_le_read_supported_states);
		cc->num_cmd_pkt = 1;
		cc->opcode = cmd->opcode;

		ccp->le_read_supported_states.status =
			HCI_EVT_ERROR_CODE_SUCCESS;
		ccp->le_read_supported_states.le_states = 0x000000001FFFFFFF;
		break;

	case HCI_OCF_LE_REMOTE_CONN_PARAM_REQ_REPLY:
		le_cp_req_rep = &cmd->params.le_remote_conn_param_req_reply;
		status = radio_conn_update(le_cp_req_rep->handle, 2, 0,
					   le_cp_req_rep->interval_max,
					   le_cp_req_rep->latency,
					   le_cp_req_rep->timeout);

		evt->code = HCI_EVT_CODE_COMMAND_COMPLETE;
		evt->len =
		HCI_CC_LEN(hci_evt_cmd_cmplt_le_remote_conn_param_req_reply);
		cc->num_cmd_pkt = 1;
		cc->opcode = cmd->opcode;

		ccp->le_remote_conn_param_req_reply.status = (status == 0) ?
			HCI_EVT_ERROR_CODE_SUCCESS :
			HCI_EVT_ERROR_CODE_COMMAND_DISALLOWED;
		ccp->le_remote_conn_param_req_reply.conn_handle =
		    cmd->params.le_remote_conn_param_req_reply.handle;
		break;

	case HCI_OCF_LE_REMOTE_CONN_PARAM_REQ_NEG_REPLY:
		le_cp_req_neg_rep =
			&cmd->params.le_remote_conn_param_req_neg_reply;
		/** @todo add reject_ext_ind support in ctrl.c */
		ASSERT(0);

		status = radio_conn_update(le_cp_req_neg_rep->handle, 2,
				      le_cp_req_neg_rep->reason, 0, 0, 0);

		evt->code = HCI_EVT_CODE_COMMAND_COMPLETE;
		evt->len = HCI_CC_LEN(
		hci_evt_cmd_cmplt_le_remote_conn_param_req_neg_reply);
		cc->num_cmd_pkt = 1;
		cc->opcode = cmd->opcode;

		ccp->le_remote_conn_param_req_neg_reply.status = (status == 0) ?
			HCI_EVT_ERROR_CODE_SUCCESS :
			HCI_EVT_ERROR_CODE_COMMAND_DISALLOWED;
		ccp->le_remote_conn_param_req_neg_reply.conn_handle =
		    cmd->params.le_remote_conn_param_req_neg_reply.handle;
		break;

	case HCI_OCF_LE_SET_DATA_LENGTH:
		/** @todo add reject_ext_ind support in ctrl.c */
		ASSERT(0);

		status = radio_length_req_send(
				cmd->params.le_set_data_length.handle,
				cmd->params.le_set_data_length.tx_octets);

		evt->code = HCI_EVT_CODE_COMMAND_COMPLETE;
		evt->len = HCI_CC_LEN(hci_evt_cmd_cmplt_le_set_data_length);
		cc->num_cmd_pkt = 1;
		cc->opcode = cmd->opcode;

		ccp->le_set_data_length.status = (status == 0) ?
			HCI_EVT_ERROR_CODE_SUCCESS :
			HCI_EVT_ERROR_CODE_COMMAND_DISALLOWED;
		ccp->le_set_data_length.conn_handle =
			cmd->params.le_set_data_length.handle;
		break;

	default:
		evt->code = HCI_EVT_CODE_COMMAND_COMPLETE;
		evt->len = HCI_CC_LEN(hci_evt_cmd_cmplt_unknown_hci_command);
		cc->num_cmd_pkt = 1;
		cc->opcode = cmd->opcode;

		ccp->unknown_hci_command.status =
			HCI_EVT_ERROR_CODE_UNKNOWN_HCI_COMMAND;
		break;
	}

	*len = HCI_EVT_LEN(evt);
}

static void vs_cmd_handle(struct hci_cmd *cmd,
		uint8_t *len, uint8_t **out)
{
	struct hci_evt *evt;
	struct hci_evt_cmd_cmplt *cc;
	union hci_evt_cmd_cmplt_params *ccp;

	evt = (struct hci_evt *)&hci_context.tx[1];
	cc = &evt->params.cmd_cmplt;
	ccp = &evt->params.cmd_cmplt.params;

	switch (cmd->opcode.ocf) {
	case HCI_OCF_NRF_SET_BD_ADDR:
		ll_address_set(0, &cmd->params.nrf_set_bd_addr.addr[0]);

		hci_context.tx[0] = HCI_EVT;

		evt = (struct hci_evt *)&hci_context.tx[1];
		evt->code = HCI_EVT_CODE_COMMAND_COMPLETE;
		evt->len = HCI_CC_LEN(hci_evt_cmd_cmplt_nrf_set_bd_addr);
		cc->num_cmd_pkt = 1;
		cc->opcode = cmd->opcode;

		ccp->nrf_set_bd_addr.status = HCI_EVT_ERROR_CODE_SUCCESS;

		*out = &hci_context.tx[0];
		break;

	case HCI_OCF_NRF_CONFIG_ACTIVE_SIGNAL:
		radio_ticks_active_to_start_set(TICKER_US_TO_TICKS
						(cmd->params.nrf_cfg_active_sig.
						 distance * 1000));

		hci_context.tx[0] = HCI_EVT;

		evt->code = HCI_EVT_CODE_COMMAND_COMPLETE;
		evt->len = HCI_CC_LEN(hci_evt_cmd_cmplt_nrf_cfg_active_sig);
		cc->num_cmd_pkt = 1;
		cc->opcode = cmd->opcode;

		ccp->nrf_cfg_active_sig.status = HCI_EVT_ERROR_CODE_SUCCESS;

		*out = &hci_context.tx[0];
		break;

	default:
		hci_context.tx[0] = HCI_EVT;

		evt->code = HCI_EVT_CODE_COMMAND_COMPLETE;
		evt->len = HCI_CC_LEN(hci_evt_cmd_cmplt_unknown_hci_command);
		cc->num_cmd_pkt = 1;
		cc->opcode = cmd->opcode;

		ccp->unknown_hci_command.status =
			HCI_EVT_ERROR_CODE_UNKNOWN_HCI_COMMAND;

		*out = &hci_context.tx[0];
		break;
	}

	*len = HCI_EVT_LEN(evt);
}

static void hci_cmd_handle(struct hci_cmd *cmd, uint8_t *len, uint8_t **out)
{
	struct hci_evt *evt;
	struct hci_evt_cmd_cmplt *cc;
	union hci_evt_cmd_cmplt_params *ccp;

	*out = &hci_context.tx[0];
	hci_context.tx[0] = HCI_EVT;
	evt = (struct hci_evt *)&hci_context.tx[1];
	cc = &evt->params.cmd_cmplt;
	ccp = &evt->params.cmd_cmplt.params;

	switch (cmd->opcode.ogf) {
	case HCI_OGF_LINK_CONTROL:
		link_control_cmd_handle(cmd, len);
		break;
	case HCI_OGF_CONTROL_AND_BASEBAND:
		ctrl_bb_cmd_handle(cmd, len);
		break;
	case HCI_OGF_INFORMATIONAL:
		info_cmd_handle(cmd, len);
		break;
	case HCI_OGF_LE_CONTROLLER:
		controller_cmd_handle(cmd, len, out);
		break;

	case HCI_OGF_VENDOR_SPECIFIC:
		vs_cmd_handle(cmd, len, out);
		break;

	default:
		evt->code = HCI_EVT_CODE_COMMAND_COMPLETE;
		evt->len = HCI_CC_LEN(hci_evt_cmd_cmplt_unknown_hci_command);
		cc->num_cmd_pkt = 1;
		cc->opcode = cmd->opcode;

		ccp->unknown_hci_command.status =
		    HCI_EVT_ERROR_CODE_UNKNOWN_HCI_COMMAND;

		*len = HCI_EVT_LEN(evt);
		break;
	}
}

static void hci_data_handle(void)
{
	struct hci_data *data;

	if (!(hci_context.rx_len > offsetof(struct hci_data, data))) {
		return;
	}

	data = (struct hci_data *)&hci_context.rx[1];
	if (!(hci_context.rx_len >=
	    (1 + offsetof(struct hci_data, data) +
	     data->len))) {
		return;
	}

	struct radio_pdu_node_tx *radio_pdu_node_tx;

	radio_pdu_node_tx = radio_tx_mem_acquire();
	if (radio_pdu_node_tx) {
		struct pdu_data *pdu_data;

		pdu_data = (struct pdu_data *)radio_pdu_node_tx->pdu_data;
		if (data->pb == 0x00 || data->pb == 0x02) {
			pdu_data->ll_id = PDU_DATA_LLID_DATA_START;
		} else {
			pdu_data->ll_id = PDU_DATA_LLID_DATA_CONTINUE;
		}
		pdu_data->len = data->len;
		memcpy(&pdu_data->payload.lldata[0],
			 &data->data[0],
			 data->len);
		if (radio_tx_mem_enqueue(data->handle, radio_pdu_node_tx)) {
			radio_tx_mem_release
			    (radio_pdu_node_tx);
		}
	}

	hci_context.rx_len = 0;
}

void hci_handle(uint8_t x, uint8_t *len, uint8_t **out)
{
	struct hci_cmd *cmd;

	hci_context.rx[hci_context.rx_len++] = x;

	*len = 0;
	*out = 0;
	if (!(hci_context.rx_len > 0)) {
		return;
	}

	switch (hci_context.rx[0]) {
	case HCI_CMD:
		if (!(hci_context.rx_len > offsetof(struct hci_cmd, params))) {
			break;
		}

		cmd = (struct hci_cmd *)&hci_context.rx[1];
		if (hci_context.rx_len >=
		    (1 + offsetof(struct hci_cmd, params) + cmd->len)) {
			hci_cmd_handle(cmd, len, out);
			hci_context.rx_len = 0;
		}
		break;

	case HCI_DATA:
		hci_data_handle();
		break;

	default:
		hci_context.rx_len = 0;
		break;
	}
}


static void encode_control(uint8_t *buf, uint8_t *len, uint8_t **out)
{
	struct hci_evt *evt;
	struct pdu_adv *adv;
	struct pdu_data *pdu_data;
	uint8_t *report;
	uint8_t data_len;
	const uint8_t c_adv_type[] = { 0x00, 0x01, 0x03, 0xff, 0x04,
				     0xff, 0x02 };
	uint16_t instance;
	struct radio_le_conn_update_cmplt *le_conn_update_cmplt;
	struct radio_le_conn_cmplt *radio_le_conn_cmplt;
	struct radio_pdu_node_rx *radio_pdu_node_rx;

	radio_pdu_node_rx = (struct radio_pdu_node_rx *)buf;
	instance = radio_pdu_node_rx->hdr.handle;

	switch (radio_pdu_node_rx->hdr.type) {
	case NODE_RX_TYPE_REPORT:
		adv = (struct pdu_adv *)radio_pdu_node_rx->pdu_data;

		hci_context.tx[0] = HCI_EVT;

		evt = (struct hci_evt *)&hci_context.tx[1];
		evt->code = HCI_EVT_CODE_LE_META;
		evt->len = offsetof(struct hci_evt_le_meta,
				    subevent.adv_report.reports);

		evt->params.le_meta.subevent_code = HCI_EVT_LE_META_ADV_REPORT;
		evt->params.le_meta.subevent.adv_report.num_reports = 1;
		report = &evt->params.le_meta.subevent.adv_report.reports[0];

		*report++ = c_adv_type[adv->type];
		*report++ = adv->tx_addr;
		memcpy(&report[0], &adv->payload.adv_ind.addr[0],
			 BDADDR_SIZE);
		report += BDADDR_SIZE;
		if (adv->type != PDU_ADV_TYPE_DIRECT_IND) {
			data_len = (adv->len - BDADDR_SIZE);
		} else {
			data_len = 0;
		}
		*report++ = data_len;
		memcpy(&report[0], &adv->payload.adv_ind.data[0], data_len);
		report += data_len;
		/* RSSI */
		*report++ = buf[offsetof(struct radio_pdu_node_rx, pdu_data) +
			offsetof(struct pdu_adv, payload) + adv->len];

		evt->len += (report -
			     &evt->params.le_meta.subevent.adv_report.
			     reports[0]);

		*len = HCI_EVT_LEN(evt);
		*out = &hci_context.tx[0];
		break;

	case NODE_RX_TYPE_CONNECTION:
		pdu_data = (struct pdu_data *)radio_pdu_node_rx->pdu_data;
		radio_le_conn_cmplt = (struct radio_le_conn_cmplt *)
				       (pdu_data->payload.lldata);

		hci_context.tx[0] = HCI_EVT;

		evt = (struct hci_evt *)&hci_context.tx[1];
		evt->code = HCI_EVT_CODE_LE_META;
		evt->len = (offsetof(struct hci_evt_le_meta, subevent) +
			 sizeof(struct hci_evt_le_meta_conn_complete));
		evt->params.le_meta.subevent_code =
		    HCI_EVT_LE_META_CONNECTION_COMPLETE;
		evt->params.le_meta.subevent.conn_cmplt.status =
		    radio_le_conn_cmplt->status;
		evt->params.le_meta.subevent.conn_cmplt.conn_handle = instance;
		evt->params.le_meta.subevent.conn_cmplt.role =
		    radio_le_conn_cmplt->role;
		evt->params.le_meta.subevent.conn_cmplt.addr_type =
		    radio_le_conn_cmplt->peer_addr_type;
		memcpy(&evt->params.le_meta.subevent.conn_cmplt.addr[0],
		       &radio_le_conn_cmplt->peer_addr[0],
		       BDADDR_SIZE);
		evt->params.le_meta.subevent.conn_cmplt.interval =
		    radio_le_conn_cmplt->interval;
		evt->params.le_meta.subevent.conn_cmplt.latency =
		    radio_le_conn_cmplt->latency;
		evt->params.le_meta.subevent.conn_cmplt.timeout =
		    radio_le_conn_cmplt->timeout;
		evt->params.le_meta.subevent.conn_cmplt.mca =
		    radio_le_conn_cmplt->mca;

		*len = HCI_EVT_LEN(evt);
		*out = &hci_context.tx[0];
		break;

	case NODE_RX_TYPE_TERMINATE:
		hci_context.tx[0] = HCI_EVT;

		evt = (struct hci_evt *)&hci_context.tx[1];
		evt->code = HCI_EVT_CODE_DISCONNECTION_COMPLETE;
		evt->len = sizeof(struct hci_evt_disconnect_cmplt);
		evt->params.disconnect_cmplt.status =
		    HCI_EVT_ERROR_CODE_SUCCESS;
		evt->params.disconnect_cmplt.conn_handle = instance;
		evt->params.disconnect_cmplt.reason =
		    *((uint8_t *)radio_pdu_node_rx->pdu_data);

		*len = HCI_EVT_LEN(evt);
		*out = &hci_context.tx[0];
		break;

	case NODE_RX_TYPE_CONN_UPDATE:
		pdu_data = (struct pdu_data *)radio_pdu_node_rx->pdu_data;
		le_conn_update_cmplt = (struct radio_le_conn_update_cmplt *)
			(pdu_data->payload.lldata);
		hci_context.tx[0] = HCI_EVT;

		evt = (struct hci_evt *)&hci_context.tx[1];
		evt->code = HCI_EVT_CODE_LE_META;
		evt->len = (offsetof(struct hci_evt_le_meta, subevent) +
			 sizeof(struct hci_evt_le_meta_conn_update_complete));
		evt->params.le_meta.subevent_code =
			HCI_EVT_LE_META_CONNECTION_UPDATE_COMPLETE;
		evt->params.le_meta.subevent.conn_update_cmplt.status =
			le_conn_update_cmplt->status;
		evt->params.le_meta.subevent.conn_update_cmplt.conn_handle =
			instance;
		evt->params.le_meta.subevent.conn_update_cmplt.interval =
			le_conn_update_cmplt->interval;
		evt->params.le_meta.subevent.conn_update_cmplt.latency =
			le_conn_update_cmplt->latency;
		evt->params.le_meta.subevent.conn_update_cmplt.timeout =
			le_conn_update_cmplt->timeout;

		*len = HCI_EVT_LEN(evt);
		*out = &hci_context.tx[0];
		break;

	case NODE_RX_TYPE_ENC_REFRESH:
		hci_context.tx[0] = HCI_EVT;

		evt = (struct hci_evt *)&hci_context.tx[1];
		evt->code = HCI_EVT_CODE_ENCRYPTION_KEY_REFRESH_COMPLETE;
		evt->len = sizeof(struct hci_evt_encryption_key_refresh_cmplt);
		evt->params.encryption_key_refresh_cmplt.status =
			HCI_EVT_ERROR_CODE_SUCCESS;
		evt->params.encryption_key_refresh_cmplt.conn_handle = instance;

		*len = HCI_EVT_LEN(evt);
		*out = &hci_context.tx[0];
		break;

	case NODE_RX_TYPE_APTO:
		hci_context.tx[0] = HCI_EVT;

		evt = (struct hci_evt *)&hci_context.tx[1];
		evt->code = HCI_EVT_CODE_APTO_EXPIRED;
		evt->len = sizeof(struct hci_evt_apto_expired);
		evt->params.apto_expired.conn_handle = instance;

		*len = HCI_EVT_LEN(evt);
		*out = &hci_context.tx[0];
		break;

	case NODE_RX_TYPE_RSSI:
  /** @todo */
		break;

	case NODE_RX_TYPE_PROFILE:
  /** @todo */
		break;

	default:
		ASSERT(0);
		break;
	}
}

static void encode_data_ctrl(struct radio_pdu_node_rx *radio_pdu_node_rx,
		uint8_t *len, uint8_t **out)
{
	uint16_t instance;
	struct hci_evt *evt;
	struct hci_evt_le_meta_read_remote_used_features *rem_used_feats;
	struct hci_evt_le_meta_long_term_key_request *long_term_key_req;
	struct hci_evt_le_meta_remote_conn_param_request *rem_cp_req;
	struct hci_evt_le_meta_length_change *len_change;
	struct pdu_data *pdu_data;

	pdu_data = (struct pdu_data *)radio_pdu_node_rx->pdu_data;
	instance = radio_pdu_node_rx->hdr.handle;

	evt = (struct hci_evt *)&hci_context.tx[1];

	switch (pdu_data->payload.llctrl.opcode) {
	case PDU_DATA_LLCTRL_TYPE_ENC_REQ:
		hci_context.tx[0] = HCI_EVT;
		long_term_key_req =
			&evt->params.le_meta.subevent.long_term_key_request;

		evt->code = HCI_EVT_CODE_LE_META;
		evt->len = (offsetof(struct hci_evt_le_meta, subevent) +
			sizeof(struct hci_evt_le_meta_long_term_key_request));
		evt->params.le_meta.subevent_code =
		    HCI_EVT_LE_META_LONG_TERM_KEY_REQUEST;
		long_term_key_req->conn_handle = instance;
		memcpy(&long_term_key_req->rand[0],
			 &pdu_data->payload.llctrl.ctrldata.enc_req.rand[0],
			 sizeof(long_term_key_req->rand));
		long_term_key_req->ediv[0] =
		    pdu_data->payload.llctrl.ctrldata.enc_req.ediv[0];
		long_term_key_req->ediv[1] =
		    pdu_data->payload.llctrl.ctrldata.enc_req.ediv[1];

		*len = HCI_EVT_LEN(evt);
		*out = &hci_context.tx[0];
		break;

	case PDU_DATA_LLCTRL_TYPE_START_ENC_RSP:
		hci_context.tx[0] = HCI_EVT;

		evt->code = HCI_EVT_CODE_ENCRYPTION_CHANGE;
		evt->len = sizeof(struct hci_evt_encryption_change);
		evt->params.encryption_change.status =
		    HCI_EVT_ERROR_CODE_SUCCESS;
		evt->params.encryption_change.conn_handle = instance;
		evt->params.encryption_change.enabled = 1;

		*len = HCI_EVT_LEN(evt);
		*out = &hci_context.tx[0];
		break;

	case PDU_DATA_LLCTRL_TYPE_FEATURE_RSP:
		hci_context.tx[0] = HCI_EVT;
		rem_used_feats =
			&evt->params.le_meta.subevent.remote_used_features;

		evt->code = HCI_EVT_CODE_LE_META;
		evt->len = (offsetof(struct hci_evt_le_meta, subevent) +
				sizeof(
		struct hci_evt_le_meta_read_remote_used_features));
		evt->params.le_meta.subevent_code =
		    HCI_EVT_LE_META_READ_REMOTE_USED_FEATURE_COMPLETE;
		rem_used_feats->status = HCI_EVT_ERROR_CODE_SUCCESS;
		rem_used_feats->conn_handle = instance;
		memcpy(&rem_used_feats->features[0],
		       &pdu_data->payload.llctrl.ctrldata.feature_rsp.
		       features[0],
		       sizeof(rem_used_feats->features));

		*len = HCI_EVT_LEN(evt);
		*out = &hci_context.tx[0];
		break;

	case PDU_DATA_LLCTRL_TYPE_VERSION_IND:
		hci_context.tx[0] = HCI_EVT;

		evt->code = HCI_EVT_CODE_READ_REMOTE_VERSION_INFO_COMPLETE;
		evt->len = sizeof(struct
				  hci_evt_read_remote_version_info_cmplt);
		evt->params.read_remote_version_info_cmplt.status =
			HCI_EVT_ERROR_CODE_SUCCESS;
		evt->params.read_remote_version_info_cmplt.conn_handle =
			instance;
		evt->params.read_remote_version_info_cmplt.version_number =
			pdu_data->payload.llctrl.ctrldata.
			version_ind.version_number;
		evt->params.read_remote_version_info_cmplt.company_id =
			pdu_data->payload.llctrl.ctrldata.
			version_ind.company_id;
		evt->params.read_remote_version_info_cmplt.sub_version_number =
			pdu_data->payload.llctrl.ctrldata.
			version_ind.sub_version_number;

		*len = HCI_EVT_LEN(evt);
		*out = &hci_context.tx[0];
		break;

	case PDU_DATA_LLCTRL_TYPE_REJECT_IND:
		hci_context.tx[0] = HCI_EVT;

		evt->code = HCI_EVT_CODE_ENCRYPTION_CHANGE;
		evt->len = sizeof(struct hci_evt_encryption_change);
		evt->params.encryption_change.status =
		    pdu_data->payload.llctrl.ctrldata.reject_ind.error_code;
		evt->params.encryption_change.conn_handle = instance;
		evt->params.encryption_change.enabled = 0;

		*len = HCI_EVT_LEN(evt);
		*out = &hci_context.tx[0];
		break;

	case PDU_DATA_LLCTRL_TYPE_CONN_PARAM_REQ:
		hci_context.tx[0] = HCI_EVT;
		rem_cp_req =
			&evt->params.le_meta.subevent.remote_conn_param_request;

		evt->code = HCI_EVT_CODE_LE_META;
		evt->len = (offsetof(struct hci_evt_le_meta,
			    subevent) + sizeof(struct
			    hci_evt_le_meta_remote_conn_param_request));
		evt->params.le_meta.subevent_code =
			HCI_EVT_LE_META_REMOTE_CONNECTION_PARAMETER_REQUEST;
		rem_cp_req->conn_handle = instance;
		rem_cp_req->interval_min =
			pdu_data->payload.llctrl.ctrldata.conn_param_req.
			interval_min;
		rem_cp_req->interval_max =
			pdu_data->payload.llctrl.ctrldata.conn_param_req.
			interval_max;
		rem_cp_req->latency =
			pdu_data->payload.llctrl.ctrldata.conn_param_req.
			latency;
		rem_cp_req->timeout =
			pdu_data->payload.llctrl.ctrldata.conn_param_req.
			timeout;

		*len = HCI_EVT_LEN(evt);
		*out = &hci_context.tx[0];
		break;

	case PDU_DATA_LLCTRL_TYPE_LENGTH_REQ:
	case PDU_DATA_LLCTRL_TYPE_LENGTH_RSP:
		hci_context.tx[0] = HCI_EVT;
		len_change = &evt->params.le_meta.subevent.length_change;

		evt->code = HCI_EVT_CODE_LE_META;
		evt->len = (offsetof(struct hci_evt_le_meta, subevent) +
			    sizeof(struct hci_evt_le_meta_length_change));
		evt->params.le_meta.subevent_code =
		    HCI_EVT_LE_META_LENGTH_CHANGE;
		len_change->conn_handle = instance;
		len_change->max_tx_octets =
		    pdu_data->payload.llctrl.ctrldata.length_rsp.max_tx_octets;
		len_change->max_tx_time = pdu_data->payload.llctrl.ctrldata.
		    length_rsp.max_tx_time;
		len_change->max_rx_octets = pdu_data->payload.llctrl.ctrldata.
		    length_rsp.max_rx_octets;
		len_change->max_rx_time = pdu_data->payload.llctrl.ctrldata.
		    length_rsp.max_rx_time;

		*len = HCI_EVT_LEN(evt);
		*out = &hci_context.tx[0];

#if (TEST_DATA_LENGTH && TEST_TX)
		{
			extern uint16_t g_data_length;

			g_data_length =
			    pdu_data->payload.llctrl.ctrldata.
			    length_rsp.max_tx_octets;
		}
#endif
		break;

	default:
		ASSERT(0);
		break;
	}

}

static void encode_data(uint8_t *buf, uint8_t *len, uint8_t **out)
{

	uint16_t instance;
	struct hci_data *data;
	struct radio_pdu_node_rx *radio_pdu_node_rx;
	struct pdu_data *pdu_data;

	radio_pdu_node_rx = (struct radio_pdu_node_rx *)buf;
	pdu_data = (struct pdu_data *)radio_pdu_node_rx->pdu_data;
	instance = radio_pdu_node_rx->hdr.handle;

	switch (pdu_data->ll_id) {
	case PDU_DATA_LLID_DATA_CONTINUE:
	case PDU_DATA_LLID_DATA_START:
#if !TEST_DROP_RX
		hci_context.tx[0] = HCI_DATA;

		data = (struct hci_data *)&hci_context.tx[1];
		data->handle = instance;
		if (pdu_data->ll_id == PDU_DATA_LLID_DATA_START) {
			data->pb = 0x02;
		} else {
			data->pb = 0x01;
		}
		data->bc = 0;
		data->len = pdu_data->len;
		memcpy(&data->data[0], &pdu_data->payload.lldata[0],
			 pdu_data->len);

		*len = HCI_DATA_LEN(data);
		*out = &hci_context.tx[0];
#else
		if (s_rx_cnt != pdu_data->payload.lldata[0]) {
			s_rx_cnt = pdu_data->payload.lldata[0];

			ASSERT(0);
		} else {
			uint8_t index;

			for (index = 0; index < pdu_data->len; index++) {
				ASSERT(pdu_data->payload.lldata[index] ==
				       (uint8_t)(s_rx_cnt + index))
			}

			s_rx_cnt++;
		}
#endif
		break;

	case PDU_DATA_LLID_CTRL:
		encode_data_ctrl(radio_pdu_node_rx, len, out);
		break;

	default:
		ASSERT(0);
		break;
	}

}

void hci_encode(uint8_t *buf, uint8_t *len, uint8_t **out)
{
	struct radio_pdu_node_rx *radio_pdu_node_rx;

	radio_pdu_node_rx = (struct radio_pdu_node_rx *)buf;
	*len = 0;
	*out = 0;

	if (radio_pdu_node_rx->hdr.type != NODE_RX_TYPE_DC_PDU) {
		encode_control(buf, len, out);
	} else {
		encode_data(buf, len, out);
	}
}

void hci_encode_num_cmplt(uint16_t instance, uint8_t num, uint8_t *len,
			  uint8_t **out)
{
	struct hci_evt *evt;
	uint8_t *handles_nums;
	uint8_t num_handles;

	num_handles = 1;

	hci_context.tx[0] = HCI_EVT;

	evt = (struct hci_evt *)&hci_context.tx[1];
	evt->code = HCI_EVT_CODE_NUM_COMPLETE;
	evt->len = (offsetof(struct hci_evt_num_cmplt, handles_nums) +
		    (sizeof(uint16_t) * 2 * num_handles));

	evt->params.num_cmplt.num_handles = num_handles;
	handles_nums = &evt->params.num_cmplt.handles_nums[0];
	handles_nums[0] = instance & 0xFF;
	handles_nums[1] = (instance >> 8) & 0xFF;
	handles_nums[2] = num & 0xFF;
	handles_nums[3] = (num >> 8) & 0xFF;

	*len = HCI_EVT_LEN(evt);
	*out = &hci_context.tx[0];
}
