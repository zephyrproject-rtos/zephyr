/**
 * @file
 * @brief Internal APIs for Bluetooth CSIP
 *
 * Copyright (c) 2021-2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/audio/csip.h>


#define BT_CSIP_SIRK_TYPE_ENCRYPTED             0x00
#define BT_CSIP_SIRK_TYPE_PLAIN                 0x01

#define BT_CSIP_RELEASE_VALUE                   0x01
#define BT_CSIP_LOCK_VALUE                      0x02

struct bt_csip_set_sirk {
	uint8_t type;
	uint8_t value[BT_CSIP_SET_SIRK_SIZE];
} __packed;

struct bt_csip_set_coordinator_svc_inst *bt_csip_set_coordinator_lookup_instance_by_index(
	const struct bt_conn *conn, uint8_t idx);

struct bt_csip_set_coordinator_svc_inst {
	uint8_t set_lock;

	uint16_t start_handle;
	uint16_t end_handle;
	uint16_t set_sirk_handle;
	uint16_t set_size_handle;
	uint16_t set_lock_handle;
	uint16_t rank_handle;

	uint8_t idx;
	struct bt_gatt_subscribe_params sirk_sub_params;
	struct bt_gatt_discover_params sirk_sub_disc_params;
	struct bt_gatt_subscribe_params size_sub_params;
	struct bt_gatt_discover_params size_sub_disc_params;
	struct bt_gatt_subscribe_params lock_sub_params;
	struct bt_gatt_discover_params lock_sub_disc_params;

	struct bt_conn *conn;
	struct bt_csip_set_coordinator_set_info *set_info;
};

struct bt_csip_set_coordinator_csis_inst *bt_csip_set_coordinator_csis_inst_by_handle(
	struct bt_conn *conn, uint16_t start_handle);
