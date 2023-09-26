/** @file
 *  @brief Internal APIs for Bluetooth VOCS.
 *
 * Copyright (c) 2020 Bose Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_VOCS_INTERNAL_
#define ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_VOCS_INTERNAL_
#include <zephyr/types.h>

#if defined(CONFIG_BT_VOCS)
#define BT_VOCS_MAX_DESC_SIZE CONFIG_BT_VOCS_MAX_OUTPUT_DESCRIPTION_SIZE
#else
#define BT_VOCS_MAX_DESC_SIZE 1
#endif /* CONFIG_BT_VOCS */

/* VOCS opcodes */
#define BT_VOCS_OPCODE_SET_OFFSET                  0x01

struct bt_vocs_control {
	uint8_t opcode;
	uint8_t counter;
	int16_t offset;
} __packed;

struct bt_vocs_state {
	int16_t offset;
	uint8_t change_counter;
} __packed;

struct bt_vocs {
	bool client_instance;
};

struct bt_vocs_client {
	struct bt_vocs vocs;
	struct bt_vocs_state state;
	bool location_writable;
	uint32_t location;
	bool desc_writable;
	bool active;

	uint16_t start_handle;
	uint16_t end_handle;
	uint16_t state_handle;
	uint16_t location_handle;
	uint16_t control_handle;
	uint16_t desc_handle;
	struct bt_gatt_subscribe_params state_sub_params;
	struct bt_gatt_subscribe_params location_sub_params;
	struct bt_gatt_subscribe_params desc_sub_params;
	bool cp_retried;

	bool busy;
	struct bt_vocs_control cp;
	struct bt_gatt_write_params write_params;
	struct bt_gatt_read_params read_params;
	struct bt_vocs_cb *cb;
	struct bt_gatt_discover_params discover_params;
	struct bt_conn *conn;
};

enum bt_vocs_notify {
	NOTIFY_STATE,
	NOTIFY_LOCATION,
	NOTIFY_OUTPUT_DESC,
	NOTIFY_NUM,
};

struct bt_vocs_server {
	struct bt_vocs vocs;
	struct bt_vocs_state state;
	uint32_t location;
	bool initialized;
	char output_desc[BT_VOCS_MAX_DESC_SIZE];
	struct bt_vocs_cb *cb;

	struct bt_gatt_service *service_p;

	ATOMIC_DEFINE(notify, NOTIFY_NUM);
	struct k_work_delayable notify_work;
};

int bt_vocs_client_state_get(struct bt_vocs_client *inst);
int bt_vocs_client_state_set(struct bt_vocs_client *inst, int16_t offset);
int bt_vocs_client_location_get(struct bt_vocs_client *inst);
int bt_vocs_client_location_set(struct bt_vocs_client *inst, uint32_t location);
int bt_vocs_client_description_get(struct bt_vocs_client *inst);
int bt_vocs_client_description_set(struct bt_vocs_client *inst, const char *description);

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_VOCS_INTERNAL_ */
