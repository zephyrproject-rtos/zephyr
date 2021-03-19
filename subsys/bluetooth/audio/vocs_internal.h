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
#define VOCS_MAX_DESC_SIZE CONFIG_BT_VOCS_MAX_OUTPUT_DESCRIPTION_SIZE
#else
#define VOCS_MAX_DESC_SIZE 1
#endif /* CONFIG_BT_VOCS */

/* VOCS opcodes */
#define VOCS_OPCODE_SET_OFFSET                  0x01

struct vocs_control_t {
	uint8_t opcode;
	uint8_t counter;
	int16_t offset;
} __packed;

struct vocs_state_t {
	int16_t offset;
	uint8_t change_counter;
} __packed;

struct vocs_client {
	struct vocs_state_t state;
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
	uint8_t subscribe_cnt;

	bool busy;
	struct vocs_control_t cp;
	struct bt_gatt_write_params write_params;
	struct bt_gatt_read_params read_params;
	struct bt_vocs_cb *cb;
	struct bt_gatt_discover_params discover_params;
	struct bt_conn *conn;
};

struct vocs_server {
	struct vocs_state_t state;
	uint32_t location;
	bool initialized;
	char output_desc[VOCS_MAX_DESC_SIZE];
	struct bt_vocs_cb *cb;

	struct bt_gatt_service *service_p;
};

struct bt_vocs {
	union {
		struct vocs_server srv;
		struct vocs_client cli;
	};
};

int bt_vocs_client_state_get(struct bt_conn *conn, struct bt_vocs *inst);
int bt_vocs_client_state_set(struct bt_conn *conn, struct bt_vocs *inst, int16_t offset);
int bt_vocs_client_location_get(struct bt_conn *conn, struct bt_vocs *inst);
int bt_vocs_client_location_set(struct bt_conn *conn, struct bt_vocs *inst, uint32_t location);
int bt_vocs_client_description_get(struct bt_conn *conn, struct bt_vocs *inst);
int bt_vocs_client_description_set(struct bt_conn *conn, struct bt_vocs *inst,
				   const char *description);

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_VOCS_INTERNAL_ */
