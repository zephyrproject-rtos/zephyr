/* gatt.h - Bluetooth tester headers */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 * Copyright (c) 2022 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/util.h>
#include <zephyr/bluetooth/addr.h>

/* GATT Service */
/* commands */
#define BTP_GATT_READ_SUPPORTED_COMMANDS	0x01
struct btp_gatt_read_supported_commands_rp {
	uint8_t data[0];
} __packed;

#define BTP_GATT_SERVICE_PRIMARY		0x00
#define BTP_GATT_SERVICE_SECONDARY		0x01

#define BTP_GATT_ADD_SERVICE			0x02
struct btp_gatt_add_service_cmd {
	uint8_t type;
	uint8_t uuid_length;
	uint8_t uuid[];
} __packed;
struct btp_gatt_add_service_rp {
	uint16_t svc_id;
} __packed;

#define BTP_GATT_ADD_CHARACTERISTIC		0x03
struct btp_gatt_add_characteristic_cmd {
	uint16_t svc_id;
	uint8_t properties;
	uint8_t permissions;
	uint8_t uuid_length;
	uint8_t uuid[];
} __packed;
struct btp_gatt_add_characteristic_rp {
	uint16_t char_id;
} __packed;

#define BTP_GATT_ADD_DESCRIPTOR			0x04
struct btp_gatt_add_descriptor_cmd {
	uint16_t char_id;
	uint8_t permissions;
	uint8_t uuid_length;
	uint8_t uuid[];
} __packed;
struct btp_gatt_add_descriptor_rp {
	uint16_t desc_id;
} __packed;

#define BTP_GATT_ADD_INCLUDED_SERVICE		0x05
struct btp_gatt_add_included_service_cmd {
	uint16_t svc_id;
} __packed;
struct btp_gatt_add_included_service_rp {
	uint16_t included_service_id;
} __packed;

#define BTP_GATT_SET_VALUE			0x06
struct btp_gatt_set_value_cmd {
	uint16_t attr_id;
	uint16_t len;
	uint8_t value[];
} __packed;

#define BTP_GATT_START_SERVER			0x07
struct btp_gatt_start_server_rp {
	uint16_t db_attr_off;
	uint8_t db_attr_cnt;
} __packed;

#define BTP_GATT_RESET_SERVER			0x08

#define BTP_GATT_SET_ENC_KEY_SIZE		0x09
struct btp_gatt_set_enc_key_size_cmd {
	uint16_t attr_id;
	uint8_t key_size;
} __packed;

/* Gatt Client */
struct btp_gatt_service {
	uint16_t start_handle;
	uint16_t end_handle;
	uint8_t uuid_length;
	uint8_t uuid[];
} __packed;

struct btp_gatt_included {
	uint16_t included_handle;
	struct btp_gatt_service service;
} __packed;

struct btp_gatt_characteristic {
	uint16_t characteristic_handle;
	uint16_t value_handle;
	uint8_t properties;
	uint8_t uuid_length;
	uint8_t uuid[];
} __packed;

struct btp_gatt_descriptor {
	uint16_t descriptor_handle;
	uint8_t uuid_length;
	uint8_t uuid[];
} __packed;

#define BTP_GATT_EXCHANGE_MTU			0x0a
struct btp_gatt_exchange_mtu_cmd {
	bt_addr_le_t address;
} __packed;

#define BTP_GATT_DISC_ALL_PRIM			0x0b
struct btp_gatt_disc_all_prim_cmd {
	bt_addr_le_t address;
} __packed;
struct btp_gatt_disc_all_prim_rp {
	uint8_t services_count;
	struct btp_gatt_service services[];
} __packed;

#define BTP_GATT_DISC_PRIM_UUID			0x0c
struct btp_gatt_disc_prim_uuid_cmd {
	bt_addr_le_t address;
	uint8_t uuid_length;
	uint8_t uuid[];
} __packed;
struct btp_gatt_disc_prim_rp {
	uint8_t services_count;
	struct btp_gatt_service services[];
} __packed;

#define BTP_GATT_FIND_INCLUDED			0x0d
struct btp_gatt_find_included_cmd {
	bt_addr_le_t address;
	uint16_t start_handle;
	uint16_t end_handle;
} __packed;
struct btp_gatt_find_included_rp {
	uint8_t services_count;
	struct btp_gatt_included included[];
} __packed;

#define BTP_GATT_DISC_ALL_CHRC			0x0e
struct btp_gatt_disc_all_chrc_cmd {
	bt_addr_le_t address;
	uint16_t start_handle;
	uint16_t end_handle;
} __packed;
struct btp_gatt_disc_chrc_rp {
	uint8_t characteristics_count;
	struct btp_gatt_characteristic characteristics[];
} __packed;

#define BTP_GATT_DISC_CHRC_UUID			0x0f
struct btp_gatt_disc_chrc_uuid_cmd {
	bt_addr_le_t address;
	uint16_t start_handle;
	uint16_t end_handle;
	uint8_t uuid_length;
	uint8_t uuid[];
} __packed;

#define BTP_GATT_DISC_ALL_DESC			0x10
struct btp_gatt_disc_all_desc_cmd {
	bt_addr_le_t address;
	uint16_t start_handle;
	uint16_t end_handle;
} __packed;
struct btp_gatt_disc_all_desc_rp {
	uint8_t descriptors_count;
	struct btp_gatt_descriptor descriptors[];
} __packed;

#define BTP_GATT_READ				0x11
struct btp_gatt_read_cmd {
	bt_addr_le_t address;
	uint16_t handle;
} __packed;
struct btp_gatt_read_rp {
	uint8_t att_response;
	uint16_t data_length;
	uint8_t data[];
} __packed;

struct btp_gatt_char_value {
	uint16_t handle;
	uint8_t data_len;
	uint8_t data[0];
} __packed;

#define BTP_GATT_READ_UUID			0x12
struct btp_gatt_read_uuid_cmd {
	bt_addr_le_t address;
	uint16_t start_handle;
	uint16_t end_handle;
	uint8_t uuid_length;
	uint8_t uuid[];
} __packed;
struct btp_gatt_read_uuid_rp {
	uint8_t att_response;
	uint8_t values_count;
	struct btp_gatt_char_value values[0];
} __packed;

#define BTP_GATT_READ_LONG			0x13
struct btp_gatt_read_long_cmd {
	bt_addr_le_t address;
	uint16_t handle;
	uint16_t offset;
} __packed;
struct btp_gatt_read_long_rp {
	uint8_t att_response;
	uint16_t data_length;
	uint8_t data[];
} __packed;

#define BTP_GATT_READ_MULTIPLE			0x14
struct btp_gatt_read_multiple_cmd {
	bt_addr_le_t address;
	uint8_t handles_count;
	uint16_t handles[];
} __packed;
struct btp_gatt_read_multiple_rp {
	uint8_t att_response;
	uint16_t data_length;
	uint8_t data[];
} __packed;

#define BTP_GATT_WRITE_WITHOUT_RSP		0x15
struct btp_gatt_write_without_rsp_cmd {
	bt_addr_le_t address;
	uint16_t handle;
	uint16_t data_length;
	uint8_t data[];
} __packed;

#define BTP_GATT_SIGNED_WRITE_WITHOUT_RSP	0x16
struct btp_gatt_signed_write_without_rsp_cmd {
	bt_addr_le_t address;
	uint16_t handle;
	uint16_t data_length;
	uint8_t data[];
} __packed;

#define BTP_GATT_WRITE				0x17
struct btp_gatt_write_cmd {
	bt_addr_le_t address;
	uint16_t handle;
	uint16_t data_length;
	uint8_t data[];
} __packed;
struct btp_gatt_write_rp {
	uint8_t att_response;
} __packed;

#define BTP_GATT_WRITE_LONG			0x18
struct btp_gatt_write_long_cmd {
	bt_addr_le_t address;
	uint16_t handle;
	uint16_t offset;
	uint16_t data_length;
	uint8_t data[];
} __packed;
struct btp_gatt_write_long_rp {
	uint8_t att_response;
} __packed;

#define BTP_GATT_RELIABLE_WRITE			0x19
struct btp_gatt_reliable_write_cmd {
	bt_addr_le_t address;
	uint16_t handle;
	uint16_t offset;
	uint16_t data_length;
	uint8_t data[];
} __packed;
struct btp_gatt_reliable_write_rp {
	uint8_t att_response;
} __packed;

#define BTP_GATT_CFG_NOTIFY			0x1a
#define BTP_GATT_CFG_INDICATE			0x1b
struct btp_gatt_cfg_notify_cmd {
	bt_addr_le_t address;
	uint8_t enable;
	uint16_t ccc_handle;
} __packed;

#define BTP_GATT_GET_ATTRIBUTES			0x1c
struct btp_gatt_get_attributes_cmd {
	uint16_t start_handle;
	uint16_t end_handle;
	uint8_t type_length;
	uint8_t type[];
} __packed;
struct btp_gatt_get_attributes_rp {
	uint8_t attrs_count;
	uint8_t attrs[];
} __packed;
struct btp_gatt_attr {
	uint16_t handle;
	uint8_t permission;
	uint8_t type_length;
	uint8_t type[];
} __packed;

#define BTP_GATT_GET_ATTRIBUTE_VALUE		0x1d
struct btp_gatt_get_attribute_value_cmd {
	bt_addr_le_t address;
	uint16_t handle;
} __packed;
struct btp_gatt_get_attribute_value_rp {
	uint8_t att_response;
	uint16_t value_length;
	uint8_t value[];
} __packed;

#define BTP_GATT_CHANGE_DB_REMOVE		0x00
#define BTP_GATT_CHANGE_DB_ADD			0x01
#define BTP_GATT_CHANGE_DB_ANY			0x02

#define BTP_GATT_CHANGE_DB			0x1e
struct btp_gatt_change_db_cmd {
	uint16_t start_handle;
	uint16_t end_handle;
	uint8_t operation;
} __packed;

#define BTP_GATT_EATT_CONNECT			0x1f
struct btp_gatt_eatt_connect_cmd {
	bt_addr_le_t address;
	uint8_t num_channels;
} __packed;

#define BTP_GATT_READ_MULTIPLE_VAR		0x20
struct btp_gatt_read_multiple_var_cmd {
	bt_addr_le_t address;
	uint8_t handles_count;
	uint16_t handles[];
} __packed;
struct btp_gatt_read_multiple_var_rp {
	uint8_t att_response;
	uint16_t data_length;
	uint8_t data[];
} __packed;

#define BTP_GATT_NOTIFY_MULTIPLE		0x21
struct btp_gatt_cfg_notify_mult_cmd {
	bt_addr_le_t address;
	uint16_t cnt;
	uint16_t attr_id[];
} __packed;
/* GATT events */
#define BTP_GATT_EV_NOTIFICATION		0x80
struct btp_gatt_notification_ev {
	bt_addr_le_t address;
	uint8_t type;
	uint16_t handle;
	uint16_t data_length;
	uint8_t data[];
} __packed;

#define BTP_GATT_EV_ATTR_VALUE_CHANGED		0x81
struct btp_gatt_attr_value_changed_ev {
	uint16_t handle;
	uint16_t data_length;
	uint8_t data[];
} __packed;
