/* bttester.h - Bluetooth tester headers */

/*
 * Copyright (c) 2015-2016 Intel Corporation
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

#include <misc/util.h>

#define BTP_MTU 1024

#define BTP_INDEX_NONE		0xff

#define BTP_SERVICE_ID_CORE	0
#define BTP_SERVICE_ID_GAP	1
#define BTP_SERVICE_ID_GATT	2
#define BTP_SERVICE_ID_L2CAP	3

#define BTP_STATUS_SUCCESS	0x00
#define BTP_STATUS_FAILED	0x01
#define BTP_STATUS_UNKNOWN_CMD	0x02
#define BTP_STATUS_NOT_READY	0x03

#define SYS_LOG_LEVEL SYS_LOG_LEVEL_DEBUG
#define SYS_LOG_DOMAIN "bttester"
#include <misc/sys_log.h>

struct btp_hdr {
	uint8_t  service;
	uint8_t  opcode;
	uint8_t  index;
	uint16_t len;
	uint8_t  data[0];
} __packed;

#define BTP_STATUS			0x00
struct btp_status {
	uint8_t code;
} __packed;

/* Core Service */
#define CORE_READ_SUPPORTED_COMMANDS	0x01
struct core_read_supported_commands_rp {
	uint8_t data[0];
} __packed;

#define CORE_READ_SUPPORTED_SERVICES	0x02
struct core_read_supported_services_rp {
	uint8_t data[0];
} __packed;

#define CORE_REGISTER_SERVICE		0x03
struct core_register_service_cmd {
	uint8_t id;
} __packed;

/* events */
#define CORE_EV_IUT_READY		0x80

/* GAP Service */
/* commands */
#define GAP_READ_SUPPORTED_COMMANDS	0x01
struct gap_read_supported_commands_rp {
	uint8_t data[0];
} __packed;

#define GAP_READ_CONTROLLER_INDEX_LIST	0x02
struct gap_read_controller_index_list_rp {
	uint8_t num;
	uint8_t index[0];
} __packed;

#define GAP_SETTINGS_POWERED		0
#define GAP_SETTINGS_CONNECTABLE	1
#define GAP_SETTINGS_FAST_CONNECTABLE	2
#define GAP_SETTINGS_DISCOVERABLE	3
#define GAP_SETTINGS_BONDABLE		4
#define GAP_SETTINGS_LINK_SEC_3		5
#define GAP_SETTINGS_SSP		6
#define GAP_SETTINGS_BREDR		7
#define GAP_SETTINGS_HS			8
#define GAP_SETTINGS_LE			9
#define GAP_SETTINGS_ADVERTISING	10
#define GAP_SETTINGS_SC			11
#define GAP_SETTINGS_DEBUG_KEYS		12
#define GAP_SETTINGS_PRIVACY		13
#define GAP_SETTINGS_CONTROLLER_CONFIG	14
#define GAP_SETTINGS_STATIC_ADDRESS	15

#define GAP_READ_CONTROLLER_INFO	0x03
struct gap_read_controller_info_rp {
	uint8_t  address[6];
	uint32_t supported_settings;
	uint32_t current_settings;
	uint8_t  cod[3];
	uint8_t  name[249];
	uint8_t  short_name[11];
} __packed;

#define GAP_RESET			0x04
struct gap_reset_rp {
	uint32_t current_settings;
} __packed;

#define GAP_SET_POWERED			0x05
struct gap_set_powered_cmd {
	uint8_t powered;
} __packed;
struct gap_set_powered_rp {
	uint32_t current_settings;
} __packed;

#define GAP_SET_CONNECTABLE		0x06
struct gap_set_connectable_cmd {
	uint8_t connectable;
} __packed;
struct gap_set_connectable_rp {
	uint32_t current_settings;
} __packed;

#define GAP_SET_FAST_CONNECTABLE	0x07
struct gap_set_fast_connectable_cmd {
	uint8_t fast_connectable;
} __packed;
struct gap_set_fast_connectable_rp {
	uint32_t current_settings;
} __packed;

#define GAP_NON_DISCOVERABLE		0x00
#define GAP_GENERAL_DISCOVERABLE	0x01
#define GAP_LIMITED_DISCOVERABLE	0x02

#define GAP_SET_DISCOVERABLE		0x08
struct gap_set_discoverable_cmd {
	uint8_t discoverable;
} __packed;
struct gap_set_discoverable_rp {
	uint32_t current_settings;
} __packed;

#define GAP_SET_BONDABLE		0x09
struct gap_set_bondable_cmd {
	uint8_t gap_set_bondable_cmd;
} __packed;
struct gap_set_bondable_rp {
	uint32_t current_settings;
} __packed;

#define GAP_START_ADVERTISING	0x0a
struct gap_start_advertising_cmd {
	uint8_t adv_data_len;
	uint8_t scan_rsp_len;
	uint8_t adv_data[0];
	uint8_t scan_rsp[0];
} __packed;
struct gap_start_advertising_rp {
	uint32_t current_settings;
} __packed;

#define GAP_STOP_ADVERTISING		0x0b
struct gap_stop_advertising_rp {
	uint32_t current_settings;
} __packed;

#define GAP_DISCOVERY_FLAG_LE			0x01
#define GAP_DISCOVERY_FLAG_BREDR		0x02
#define GAP_DISCOVERY_FLAG_LIMITED		0x04
#define GAP_DISCOVERY_FLAG_LE_ACTIVE_SCAN	0x08
#define GAP_DISCOVERY_FLAG_LE_OBSERVE		0x10

#define GAP_START_DISCOVERY		0x0c
struct gap_start_discovery_cmd {
	uint8_t flags;
} __packed;

#define GAP_STOP_DISCOVERY		0x0d

#define GAP_CONNECT			0x0e
struct gap_connect_cmd {
	uint8_t address_type;
	uint8_t address[6];
} __packed;

#define GAP_DISCONNECT			0x0f
struct gap_disconnect_cmd {
	uint8_t  address_type;
	uint8_t  address[6];
} __packed;

#define GAP_IO_CAP_DISPLAY_ONLY		0
#define GAP_IO_CAP_DISPLAY_YESNO	1
#define GAP_IO_CAP_KEYBOARD_ONLY	2
#define GAP_IO_CAP_NO_INPUT_OUTPUT	3
#define GAP_IO_CAP_KEYBOARD_DISPLAY	4

#define GAP_SET_IO_CAP			0x10
struct gap_set_io_cap_cmd {
	uint8_t io_cap;
} __packed;

#define GAP_PAIR			0x11
struct gap_pair_cmd {
	uint8_t address_type;
	uint8_t address[6];
} __packed;

#define GAP_UNPAIR			0x12
struct gap_unpair_cmd {
	uint8_t address_type;
	uint8_t address[6];
} __packed;

#define GAP_PASSKEY_ENTRY		0x13
struct gap_passkey_entry_cmd {
	uint8_t  address_type;
	uint8_t  address[6];
	uint32_t passkey;
} __packed;

#define GAP_PASSKEY_CONFIRM		0x14
struct gap_passkey_confirm_cmd {
	uint8_t address_type;
	uint8_t address[6];
	uint8_t match;
} __packed;

/* events */
#define GAP_EV_NEW_SETTINGS		0x80
struct gap_new_settings_ev {
	uint32_t current_settings;
} __packed;

#define GAP_DEVICE_FOUND_FLAG_RSSI	0x01
#define GAP_DEVICE_FOUND_FLAG_AD	0x02
#define GAP_DEVICE_FOUND_FLAG_SD	0x04

#define GAP_EV_DEVICE_FOUND		0x81
struct gap_device_found_ev {
	uint8_t  address[6];
	uint8_t  address_type;
	int8_t   rssi;
	uint8_t  flags;
	uint16_t eir_data_len;
	uint8_t  eir_data[0];
} __packed;

#define GAP_EV_DEVICE_CONNECTED		0x82
struct gap_device_connected_ev {
	uint8_t address[6];
	uint8_t address_type;
} __packed;

#define GAP_EV_DEVICE_DISCONNECTED	0x83
struct gap_device_disconnected_ev {
	uint8_t address[6];
	uint8_t address_type;
} __packed;

#define GAP_EV_PASSKEY_DISPLAY		0x84
struct gap_passkey_display_ev {
	uint8_t  address_type;
	uint8_t  address[6];
	uint32_t passkey;
} __packed;

#define GAP_EV_PASSKEY_ENTRY_REQ	0x85
struct gap_passkey_entry_req_ev {
	uint8_t address_type;
	uint8_t address[6];
} __packed;

#define GAP_EV_PASSKEY_CONFIRM_REQ	0x86
struct gap_passkey_confirm_req_ev {
	uint8_t  address_type;
	uint8_t  address[6];
	uint32_t passkey;
} __packed;

#define GAP_EV_IDENTITY_RESOLVED	0x87
struct gap_identity_resolved_ev {
	uint8_t address_type;
	uint8_t address[6];
	uint8_t identity_address_type;
	uint8_t identity_address[6];
} __packed;

/* GATT Service */
/* commands */
#define GATT_READ_SUPPORTED_COMMANDS	0x01
struct gatt_read_supported_commands_rp {
	uint8_t data[0];
} __packed;

#define GATT_SERVICE_PRIMARY		0x00
#define GATT_SERVICE_SECONDARY		0x01

#define GATT_ADD_SERVICE		0x02
struct gatt_add_service_cmd {
	uint8_t type;
	uint8_t uuid_length;
	uint8_t uuid[0];
} __packed;
struct gatt_add_service_rp {
	uint16_t svc_id;
} __packed;

#define GATT_ADD_CHARACTERISTIC		0x03
struct gatt_add_characteristic_cmd {
	uint16_t svc_id;
	uint8_t properties;
	uint8_t permissions;
	uint8_t uuid_length;
	uint8_t uuid[0];
} __packed;
struct gatt_add_characteristic_rp {
	uint16_t char_id;
} __packed;

#define GATT_ADD_DESCRIPTOR		0x04
struct gatt_add_descriptor_cmd {
	uint16_t char_id;
	uint8_t permissions;
	uint8_t uuid_length;
	uint8_t uuid[0];
} __packed;
struct gatt_add_descriptor_rp {
	uint16_t desc_id;
} __packed;

#define GATT_ADD_INCLUDED_SERVICE	0x05
struct gatt_add_included_service_cmd {
	uint16_t svc_id;
} __packed;
struct gatt_add_included_service_rp {
	uint16_t included_service_id;
} __packed;

#define GATT_SET_VALUE			0x06
	struct gatt_set_value_cmd {
	uint16_t attr_id;
	uint16_t len;
	uint8_t value[0];
} __packed;

#define GATT_START_SERVER		0x07
struct gatt_start_server_rp {
	uint16_t db_attr_off;
	uint8_t db_attr_cnt;
} __packed;

#define GATT_SET_ENC_KEY_SIZE		0x09
struct gatt_set_enc_key_size_cmd {
	uint16_t attr_id;
	uint8_t key_size;
} __packed;

/* Gatt Client */
struct gatt_service {
	uint16_t start_handle;
	uint16_t end_handle;
	uint8_t uuid_length;
	uint8_t uuid[0];
} __packed;

struct gatt_included {
	uint16_t included_handle;
	struct gatt_service service;
} __packed;

struct gatt_characteristic {
	uint16_t characteristic_handle;
	uint16_t value_handle;
	uint8_t properties;
	uint8_t uuid_length;
	uint8_t uuid[0];
} __packed;

struct gatt_descriptor {
	uint16_t descriptor_handle;
	uint8_t uuid_length;
	uint8_t uuid[0];
} __packed;

#define GATT_EXCHANGE_MTU		0x0a

#define GATT_DISC_PRIM_UUID		0x0c
struct gatt_disc_prim_uuid_cmd {
	uint8_t address_type;
	uint8_t address[6];
	uint8_t uuid_length;
	uint8_t uuid[0];
} __packed;
struct gatt_disc_prim_uuid_rp {
	uint8_t services_count;
	struct gatt_service services[0];
} __packed;

#define GATT_FIND_INCLUDED		0x0d
struct gatt_find_included_cmd {
	uint8_t address_type;
	uint8_t address[6];
	uint16_t start_handle;
	uint16_t end_handle;
} __packed;
struct gatt_find_included_rp {
	uint8_t services_count;
	struct gatt_included included[0];
} __packed;

#define GATT_DISC_ALL_CHRC		0x0e
struct gatt_disc_all_chrc_cmd {
	uint8_t address_type;
	uint8_t address[6];
	uint16_t start_handle;
	uint16_t end_handle;
} __packed;
struct gatt_disc_chrc_rp {
	uint8_t characteristics_count;
	struct gatt_characteristic characteristics[0];
} __packed;

#define GATT_DISC_CHRC_UUID		0x0f
struct gatt_disc_chrc_uuid_cmd {
	uint8_t address_type;
	uint8_t address[6];
	uint16_t start_handle;
	uint16_t end_handle;
	uint8_t uuid_length;
	uint8_t uuid[0];
} __packed;

#define GATT_DISC_ALL_DESC		0x10
struct gatt_disc_all_desc_cmd {
	uint8_t address_type;
	uint8_t address[6];
	uint16_t start_handle;
	uint16_t end_handle;
} __packed;
struct gatt_disc_all_desc_rp {
	uint8_t descriptors_count;
	struct gatt_descriptor descriptors[0];
} __packed;

#define GATT_READ			0x11
struct gatt_read_cmd {
	uint8_t address_type;
	uint8_t address[6];
	uint16_t handle;
} __packed;
struct gatt_read_rp {
	uint8_t att_response;
	uint16_t data_length;
	uint8_t data[0];
} __packed;

#define GATT_READ_LONG			0x13
struct gatt_read_long_cmd {
	uint8_t address_type;
	uint8_t address[6];
	uint16_t handle;
	uint16_t offset;
} __packed;

#define GATT_READ_MULTIPLE		0x14
struct gatt_read_multiple_cmd {
	uint8_t address_type;
	uint8_t address[6];
	uint8_t handles_count;
	uint16_t handles[0];
} __packed;

#define GATT_WRITE_WITHOUT_RSP		0x15
struct gatt_write_without_rsp_cmd {
	uint8_t address_type;
	uint8_t address[6];
	uint16_t handle;
	uint16_t data_length;
	uint8_t data[0];
} __packed;

#define GATT_SIGNED_WRITE_WITHOUT_RSP	0x16
struct gatt_signed_write_without_rsp_cmd {
	uint8_t address_type;
	uint8_t address[6];
	uint16_t handle;
	uint16_t data_length;
	uint8_t data[0];
} __packed;

#define GATT_WRITE			0x17
struct gatt_write_cmd {
	uint8_t address_type;
	uint8_t address[6];
	uint16_t handle;
	uint16_t data_length;
	uint8_t data[0];
} __packed;

#define GATT_WRITE_LONG			0x18
struct gatt_write_long_cmd {
	uint8_t address_type;
	uint8_t address[6];
	uint16_t handle;
	uint16_t offset;
	uint16_t data_length;
	uint8_t data[0];
} __packed;

#define GATT_CFG_NOTIFY			0x1a
#define GATT_CFG_INDICATE		0x1b
struct gatt_cfg_notify_cmd {
	uint8_t address_type;
	uint8_t address[6];
	uint8_t enable;
	uint16_t ccc_handle;
} __packed;

/* GATT events */
#define GATT_EV_NOTIFICATION		0x80
struct gatt_notification_ev {
	uint8_t address_type;
	uint8_t address[6];
	uint8_t type;
	uint16_t handle;
	uint16_t data_length;
	uint8_t data[0];
} __packed;

static inline void tester_set_bit(uint8_t *addr, unsigned int bit)
{
	uint8_t *p = addr + (bit / 8);

	*p |= BIT(bit % 8);
}

static inline uint8_t tester_test_bit(const uint8_t *addr, unsigned int bit)
{
	const uint8_t *p = addr + (bit / 8);

	return *p & BIT(bit % 8);
}

/* L2CAP Service */
/* commands */
#define L2CAP_READ_SUPPORTED_COMMANDS	0x01
struct l2cap_read_supported_commands_rp {
	uint8_t data[0];
} __packed;

#define L2CAP_CONNECT			0x02
struct l2cap_connect_cmd {
	uint8_t address_type;
	uint8_t address[6];
	uint16_t psm;
} __packed;

struct l2cap_connect_rp {
	uint8_t chan_id;
} __packed;

#define L2CAP_DISCONNECT		0x03
struct l2cap_disconnect_cmd {
	uint8_t chan_id;
} __packed;

#define L2CAP_SEND_DATA			0x04
struct l2cap_send_data_cmd {
	uint8_t chan_id;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define L2CAP_TRANSPORT_BREDR		0x00
#define L2CAP_TRANSPORT_LE		0x01

#define L2CAP_LISTEN			0x05
struct l2cap_listen_cmd {
	uint16_t psm;
	uint8_t transport;
} __packed;

#define L2CAP_ACCEPT_CONNECTION		0x06
struct l2cap_accept_connection_cmd {
	uint8_t chan_id;
	uint16_t result;
} __packed;

/* events */
#define L2CAP_EV_CONNECTION_REQ		0x80
struct l2cap_connection_req_ev {
	uint8_t chan_id;
	uint16_t psm;
	uint8_t address_type;
	uint8_t address[6];
} __packed;

#define L2CAP_EV_CONNECTED		0x81
struct l2cap_connected_ev {
	uint8_t chan_id;
	uint16_t psm;
	uint8_t address_type;
	uint8_t address[6];
} __packed;

#define L2CAP_EV_DISCONNECTED		0x82
struct l2cap_disconnected_ev {
	uint16_t result;
	uint8_t chan_id;
	uint16_t psm;
	uint8_t address_type;
	uint8_t address[6];
} __packed;

#define L2CAP_EV_DATA_RECEIVED		0x83
struct l2cap_data_received_ev {
	uint8_t chan_id;
	uint16_t data_length;
	uint8_t data[0];
} __packed;

void tester_init(void);
void tester_rsp(uint8_t service, uint8_t opcode, uint8_t index, uint8_t status);
void tester_send(uint8_t service, uint8_t opcode, uint8_t index, uint8_t *data,
		 size_t len);

uint8_t tester_init_gap(void);
void tester_handle_gap(uint8_t opcode, uint8_t index, uint8_t *data,
		       uint16_t len);
uint8_t tester_init_gatt(void);
void tester_handle_gatt(uint8_t opcode, uint8_t index, uint8_t *data,
			uint16_t len);

#if defined(CONFIG_BLUETOOTH_L2CAP_DYNAMIC_CHANNEL)
uint8_t tester_init_l2cap(void);
void tester_handle_l2cap(uint8_t opcode, uint8_t index, uint8_t *data,
			 uint16_t len);
#endif /* CONFIG_BLUETOOTH_L2CAP_DYNAMIC_CHANNEL */
