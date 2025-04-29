/* gap.h - Bluetooth tester headers */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 * Copyright (c) 2022 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <stdint.h>

#include <zephyr/bluetooth/addr.h>

/* GAP Service */
/* commands */
#define BTP_GAP_READ_SUPPORTED_COMMANDS		0x01
struct btp_gap_read_supported_commands_rp {
	uint8_t data[0];
} __packed;

#define BTP_GAP_READ_CONTROLLER_INDEX_LIST	0x02
struct btp_gap_read_controller_index_list_rp {
	uint8_t num;
	uint8_t index[];
} __packed;

#define BTP_GAP_SETTINGS_POWERED		0
#define BTP_GAP_SETTINGS_CONNECTABLE		1
#define BTP_GAP_SETTINGS_FAST_CONNECTABLE	2
#define BTP_GAP_SETTINGS_DISCOVERABLE		3
#define BTP_GAP_SETTINGS_BONDABLE		4
#define BTP_GAP_SETTINGS_LINK_SEC_3		5
#define BTP_GAP_SETTINGS_SSP			6
#define BTP_GAP_SETTINGS_BREDR			7
#define BTP_GAP_SETTINGS_HS			8
#define BTP_GAP_SETTINGS_LE			9
#define BTP_GAP_SETTINGS_ADVERTISING		10
#define BTP_GAP_SETTINGS_SC			11
#define BTP_GAP_SETTINGS_DEBUG_KEYS		12
#define BTP_GAP_SETTINGS_PRIVACY		13
#define BTP_GAP_SETTINGS_CONTROLLER_CONFIG	14
#define BTP_GAP_SETTINGS_STATIC_ADDRESS		15
#define BTP_GAP_SETTINGS_SC_ONLY		16
#define BTP_GAP_SETTINGS_EXTENDED_ADVERTISING	17

#define BTP_GAP_READ_CONTROLLER_INFO		0x03
struct btp_gap_read_controller_info_rp {
	bt_addr_t  address;
	uint32_t supported_settings;
	uint32_t current_settings;
	uint8_t  cod[3];
	uint8_t  name[249];
	uint8_t  short_name[11];
} __packed;

#define BTP_GAP_RESET				0x04
struct btp_gap_reset_rp {
	uint32_t current_settings;
} __packed;

#define BTP_GAP_SET_POWERED			0x05
struct btp_gap_set_powered_cmd {
	uint8_t powered;
} __packed;
struct btp_gap_set_powered_rp {
	uint32_t current_settings;
} __packed;

#define BTP_GAP_SET_CONNECTABLE			0x06
struct btp_gap_set_connectable_cmd {
	uint8_t connectable;
} __packed;
struct btp_gap_set_connectable_rp {
	uint32_t current_settings;
} __packed;

#define BTP_GAP_SET_FAST_CONNECTABLE		0x07
struct btp_gap_set_fast_connectable_cmd {
	uint8_t fast_connectable;
} __packed;
struct btp_gap_set_fast_connectable_rp {
	uint32_t current_settings;
} __packed;

#define BTP_GAP_NON_DISCOVERABLE		0x00
#define BTP_GAP_GENERAL_DISCOVERABLE		0x01
#define BTP_GAP_LIMITED_DISCOVERABLE		0x02

#define BTP_GAP_SET_DISCOVERABLE		0x08
struct btp_gap_set_discoverable_cmd {
	uint8_t discoverable;
} __packed;
struct btp_gap_set_discoverable_rp {
	uint32_t current_settings;
} __packed;

#define BTP_GAP_SET_BONDABLE			0x09
struct btp_gap_set_bondable_cmd {
	uint8_t bondable;
} __packed;
struct btp_gap_set_bondable_rp {
	uint32_t current_settings;
} __packed;

#define BTP_GAP_ADDR_TYPE_IDENTITY			0
#define BTP_GAP_ADDR_TYPE_RESOLVABLE_PRIVATE		1
#define BTP_GAP_ADDR_TYPE_NON_RESOLVABLE_PRIVATE	2

#define BTP_GAP_START_ADVERTISING		0x0a
struct btp_gap_start_advertising_cmd {
	uint8_t adv_data_len;
	uint8_t scan_rsp_len;
	uint8_t adv_sr_data[];
/*
 * This command is very unfortunate because it has two fields after variable
 * data. Those needs to be handled explicitly by handler.
 * uint32_t duration;
 * uint8_t own_addr_type;
 */
} __packed;
struct btp_gap_start_advertising_rp {
	uint32_t current_settings;
} __packed;

#define BTP_GAP_STOP_ADVERTISING		0x0b
struct btp_gap_stop_advertising_rp {
	uint32_t current_settings;
} __packed;

#define BTP_GAP_DISCOVERY_FLAG_LE		0x01
#define BTP_GAP_DISCOVERY_FLAG_BREDR		0x02
#define BTP_GAP_DISCOVERY_FLAG_LIMITED		0x04
#define BTP_GAP_DISCOVERY_FLAG_LE_ACTIVE_SCAN	0x08
#define BTP_GAP_DISCOVERY_FLAG_LE_OBSERVE	0x10
#define BTP_GAP_DISCOVERY_FLAG_OWN_ID_ADDR	0x20

#define BTP_GAP_START_DISCOVERY			0x0c
struct btp_gap_start_discovery_cmd {
	uint8_t flags;
} __packed;

#define BTP_GAP_STOP_DISCOVERY			0x0d

#define BTP_GAP_CONNECT				0x0e
struct btp_gap_connect_cmd {
	bt_addr_le_t address;
	uint8_t own_addr_type;
} __packed;

#define BTP_GAP_DISCONNECT			0x0f
struct btp_gap_disconnect_cmd {
	bt_addr_le_t address;
} __packed;

#define BTP_GAP_IO_CAP_DISPLAY_ONLY		0
#define BTP_GAP_IO_CAP_DISPLAY_YESNO		1
#define BTP_GAP_IO_CAP_KEYBOARD_ONLY		2
#define BTP_GAP_IO_CAP_NO_INPUT_OUTPUT		3
#define BTP_GAP_IO_CAP_KEYBOARD_DISPLAY		4

#define BTP_GAP_SET_IO_CAP			0x10
struct btp_gap_set_io_cap_cmd {
	uint8_t io_cap;
} __packed;

#define BTP_GAP_PAIR				0x11
struct btp_gap_pair_cmd {
	bt_addr_le_t address;
} __packed;

#define BTP_GAP_UNPAIR				0x12
struct btp_gap_unpair_cmd {
	bt_addr_le_t address;
} __packed;

#define BTP_GAP_PASSKEY_ENTRY			0x13
struct btp_gap_passkey_entry_cmd {
	bt_addr_le_t address;
	uint32_t passkey;
} __packed;

#define BTP_GAP_PASSKEY_CONFIRM			0x14
struct btp_gap_passkey_confirm_cmd {
	bt_addr_le_t address;
	uint8_t match;
} __packed;

#define BTP_GAP_START_DIRECTED_ADV_HD		BIT(0)
#define BTP_GAP_START_DIRECTED_ADV_OWN_ID	BIT(1)
#define BTP_GAP_START_DIRECTED_ADV_PEER_RPA	BIT(2)

#define BTP_GAP_START_DIRECTED_ADV		0x15
struct btp_gap_start_directed_adv_cmd {
	bt_addr_le_t address;
	uint16_t options;
} __packed;
struct btp_gap_start_directed_adv_rp {
	uint32_t current_settings;
} __packed;

#define BTP_GAP_CONN_PARAM_UPDATE		0x16
struct btp_gap_conn_param_update_cmd {
	bt_addr_le_t address;
	uint16_t interval_min;
	uint16_t interval_max;
	uint16_t latency;
	uint16_t timeout;
} __packed;

#define BTP_GAP_PAIRING_CONSENT			0x17
struct btp_gap_pairing_consent_cmd {
	bt_addr_le_t address;
	uint8_t consent;
} __packed;

#define BTP_GAP_OOB_LEGACY_SET_DATA		0x18
struct btp_gap_oob_legacy_set_data_cmd {
	uint8_t oob_data[16];
} __packed;

#define BTP_GAP_OOB_SC_GET_LOCAL_DATA		0x19
struct btp_gap_oob_sc_get_local_data_rp {
	uint8_t rand[16];
	uint8_t conf[16];
} __packed;

#define BTP_GAP_OOB_SC_SET_REMOTE_DATA		0x1a
struct btp_gap_oob_sc_set_remote_data_cmd {
	uint8_t rand[16];
	uint8_t conf[16];
} __packed;

#define BTP_GAP_SET_MITM			0x1b
struct btp_gap_set_mitm {
	uint8_t mitm;
} __packed;

#define BTP_GAP_SET_FILTER_LIST			0x1c
struct btp_gap_set_filter_list {
	uint8_t cnt;
	bt_addr_le_t addr[];
} __packed;

#define BTP_GAP_SET_PRIVACY			0x1d
#define BTP_GAP_SET_SC_ONLY			0x1e
#define BTP_GAP_SET_SC				0x1f
#define BTP_GAP_SET_MIN_ENC_KEY_SIZE		0x20

#define BTP_GAP_SET_EXTENDED_ADVERTISING	0x21
struct btp_gap_set_extended_advertising_cmd {
	uint8_t settings;
} __packed;
struct btp_gap_set_extended_advertising_rp {
	uint32_t current_settings;
} __packed;

#define BTP_GAP_PADV_CONFIGURE			0x22
/* bitmap of flags*/
#define BTP_GAP_PADV_INCLUDE_TX_POWER		BIT(0)
struct btp_gap_padv_configure_cmd {
	uint8_t flags;
	uint16_t interval_min;
	uint16_t interval_max;
} __packed;
struct btp_gap_padv_configure_rp {
	uint32_t current_settings;
} __packed;

#define BTP_GAP_PADV_START			0x23
struct btp_gap_padv_start_cmd {
	uint8_t flags;
} __packed;
struct btp_gap_padv_start_rp {
	uint32_t current_settings;
} __packed;

#define BTP_GAP_PADV_STOP			0x24
struct btp_gap_padv_stop_cmd {
} __packed;
struct btp_gap_padv_stop_rp {
	uint32_t current_settings;
} __packed;

#define BTP_GAP_PADV_SET_DATA			0x25
struct btp_gap_padv_set_data_cmd {
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_GAP_PADV_CREATE_SYNC_FLAG_REPORTS_DISABLED	0x01
#define BTP_GAP_PADV_CREATE_SYNC_FLAG_FILTER_DUPLICATES	0x02

#define BTP_GAP_PADV_CREATE_SYNC		0x26
struct btp_gap_padv_create_sync_cmd {
	bt_addr_le_t address;
	uint8_t advertiser_sid;
	uint16_t skip;
	uint16_t sync_timeout;
	uint8_t flags;
} __packed;

#define BTP_GAP_PADV_SYNC_TRANSFER_SET_INFO	0x27
struct btp_gap_padv_sync_transfer_set_info_cmd {
	bt_addr_le_t address;
	uint16_t service_data;
} __packed;

#define BTP_GAP_PADV_SYNC_TRANSFER_START	0x28
struct btp_gap_padv_sync_transfer_start_cmd {
	uint16_t sync_handle;
	bt_addr_le_t address;
	uint16_t service_data;
} __packed;

#define BTP_GAP_PADV_SYNC_TRANSFER_RECV		0x29
struct btp_gap_padv_sync_transfer_recv_cmd {
	bt_addr_le_t address;
	uint16_t skip;
	uint16_t sync_timeout;
	uint8_t flags;
} __packed;

#define BTP_GAP_PAIR_V2_MODE_1			0x01
#define BTP_GAP_PAIR_V2_MODE_2			0x02
#define BTP_GAP_PAIR_V2_MODE_3			0x03
#define BTP_GAP_PAIR_V2_MODE_4			0x04
#define BTP_GAP_PAIR_V2_MODE_ANY		0xFF

#define BTP_GAP_PAIR_V2_LEVEL_0			0x00
#define BTP_GAP_PAIR_V2_LEVEL_1			0x01
#define BTP_GAP_PAIR_V2_LEVEL_2			0x02
#define BTP_GAP_PAIR_V2_LEVEL_3			0x03
#define BTP_GAP_PAIR_V2_LEVEL_4			0x04
#define BTP_GAP_PAIR_V2_LEVEL_ANY		0xFF

#define BTP_GAP_PAIR_V2_FLAG_FORCE_PAIR		BIT(0)

#define BTP_GAP_PAIR_V2				0x2A
struct btp_gap_pair_v2_cmd {
	bt_addr_le_t address;
	uint8_t mode;
	uint8_t level;
	uint8_t flags;
} __packed;

#define BTP_GAP_SET_RPA_TIMEOUT                 0x30
struct btp_gap_set_rpa_timeout_cmd {
	uint16_t rpa_timeout;
} __packed;

/* events */
#define BTP_GAP_EV_NEW_SETTINGS			0x80
struct btp_gap_new_settings_ev {
	uint32_t current_settings;
} __packed;

#define BTP_GAP_DEVICE_FOUND_FLAG_RSSI		0x01
#define BTP_GAP_DEVICE_FOUND_FLAG_AD		0x02
#define BTP_GAP_DEVICE_FOUND_FLAG_SD		0x04

#define BTP_GAP_EV_DEVICE_FOUND			0x81
struct btp_gap_device_found_ev {
	bt_addr_le_t address;
	int8_t   rssi;
	uint8_t  flags;
	uint16_t eir_data_len;
	uint8_t  eir_data[];
} __packed;

#define BTP_GAP_EV_DEVICE_CONNECTED		0x82
struct btp_gap_device_connected_ev {
	bt_addr_le_t address;
	uint16_t interval;
	uint16_t latency;
	uint16_t timeout;
} __packed;

#define BTP_GAP_EV_DEVICE_DISCONNECTED		0x83
struct btp_gap_device_disconnected_ev {
	bt_addr_le_t address;
} __packed;

#define BTP_GAP_EV_PASSKEY_DISPLAY		0x84
struct btp_gap_passkey_display_ev {
	bt_addr_le_t address;
	uint32_t passkey;
} __packed;

#define BTP_GAP_EV_PASSKEY_ENTRY_REQ		0x85
struct btp_gap_passkey_entry_req_ev {
	bt_addr_le_t address;
} __packed;

#define BTP_GAP_EV_PASSKEY_CONFIRM_REQ		0x86
struct btp_gap_passkey_confirm_req_ev {
	bt_addr_le_t address;
	uint32_t passkey;
} __packed;

#define BTP_GAP_EV_IDENTITY_RESOLVED		0x87
struct btp_gap_identity_resolved_ev {
	bt_addr_le_t address;
	bt_addr_le_t identity_address;
} __packed;

#define BTP_GAP_EV_CONN_PARAM_UPDATE		0x88
struct btp_gap_conn_param_update_ev {
	bt_addr_le_t address;
	uint16_t interval;
	uint16_t latency;
	uint16_t timeout;
} __packed;

#define BTP_GAP_SEC_LEVEL_UNAUTH_ENC		0x01
#define BTP_GAP_SEC_LEVEL_AUTH_ENC		0x02
#define BTP_GAP_SEC_LEVEL_AUTH_SC		0x03

#define BTP_GAP_EV_SEC_LEVEL_CHANGED		0x89
struct btp_gap_sec_level_changed_ev {
	bt_addr_le_t address;
	uint8_t sec_level;
} __packed;

#define BTP_GAP_EV_PAIRING_CONSENT_REQ		0x8a
struct btp_gap_pairing_consent_req_ev {
	bt_addr_le_t address;
} __packed;

#define BTP_GAP_EV_BOND_LOST			0x8b
struct btp_gap_bond_lost_ev {
	bt_addr_le_t address;
} __packed;

#define BTP_GAP_EV_PAIRING_FAILED		0x8c
struct btp_gap_bond_pairing_failed_ev {
	bt_addr_le_t address;
	uint8_t reason;
} __packed;

#define BTP_GAP_EV_PERIODIC_SYNC_ESTABLISHED	0x8d
struct btp_gap_ev_periodic_sync_established_ev {
	bt_addr_le_t address;
	uint16_t sync_handle;
	uint8_t status;
} __packed;

#define BTP_GAP_EV_PERIODIC_SYNC_LOST		0x8e
struct btp_gap_ev_periodic_sync_lost_ev {
	uint16_t sync_handle;
	uint8_t reason;
} __packed;

#define BTP_GAP_EV_PERIODIC_REPORT		0x8f
struct btp_gap_ev_periodic_report_ev {
	uint16_t sync_handle;
	uint8_t tx_power;
	uint8_t rssi;
	uint8_t cte_type;
	uint8_t data_status;
	uint8_t data_len;
	uint8_t data[];
} __packed;

#define BTP_GAP_EV_PERIODIC_TRANSFER_RECEIVED	0x90
struct btp_gap_ev_periodic_transfer_received_ev {
	uint16_t sync_handle;
	uint8_t tx_power;
	uint8_t rssi;
	uint8_t cte_type;
	uint8_t data_status;
	uint8_t data_len;
	uint8_t data[];
} __packed;

#define BTP_GAP_EV_ENCRYPTION_CHANGE		0x91
struct btp_gap_encryption_change_ev {
	bt_addr_le_t address;
	uint8_t enabled;
	uint8_t key_size;
} __packed;

#if defined(CONFIG_BT_EXT_ADV)
struct bt_le_per_adv_param;
struct bt_le_per_adv_sync_param;
struct bt_le_adv_param;
struct bt_data;
struct bt_le_ext_adv *tester_gap_ext_adv_get(uint8_t ext_adv_idx);
struct bt_le_per_adv_sync *tester_gap_padv_get(void);
int tester_gap_create_adv_instance(struct bt_le_adv_param *param, uint8_t own_addr_type,
				   const struct bt_data *ad, size_t ad_len,
				   const struct bt_data *sd, size_t sd_len,
				   uint32_t *settings, struct bt_le_ext_adv **ext_adv);
int tester_gap_stop_ext_adv(struct bt_le_ext_adv *ext_adv);
int tester_gap_start_ext_adv(struct bt_le_ext_adv *ext_adv);
int tester_gap_padv_configure(struct bt_le_ext_adv *ext_adv,
			      const struct bt_le_per_adv_param *param);
int tester_gap_padv_set_data(struct bt_le_ext_adv *ext_adv, struct bt_data *per_ad, uint8_t ad_len);
int tester_gap_padv_start(struct bt_le_ext_adv *ext_adv);
int tester_gap_padv_stop(struct bt_le_ext_adv *ext_adv);
int tester_gap_padv_create_sync(struct bt_le_per_adv_sync_param *create_params);
int tester_gap_padv_stop_sync(void);
#endif /* defined(CONFIG_BT_EXT_ADV) */
