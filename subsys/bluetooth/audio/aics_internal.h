/** @file
 *  @brief Internal APIs for Bluetooth AICS.
 */

/*
 * Copyright (c) 2020 Bose Corporation
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_AICS_INTERNAL_
#define ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_AICS_INTERNAL_
#include <stdbool.h>
#include <stdint.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/types.h>

#if defined(CONFIG_BT_AICS)
#define BT_AICS_MAX_DESC_SIZE CONFIG_BT_AICS_MAX_INPUT_DESCRIPTION_SIZE
#else
#define BT_AICS_MAX_DESC_SIZE 1
#endif /* CONFIG_BT_AICS */

/* AICS opcodes */
#define BT_AICS_OPCODE_SET_GAIN                    0x01
#define BT_AICS_OPCODE_UNMUTE                      0x02
#define BT_AICS_OPCODE_MUTE                        0x03
#define BT_AICS_OPCODE_SET_MANUAL                  0x04
#define BT_AICS_OPCODE_SET_AUTO                    0x05

/* AICS status */
#define BT_AICS_STATUS_INACTIVE                    0x00
#define BT_AICS_STATUS_ACTIVE                      0x01

#define BT_AICS_INPUT_MODE_IMMUTABLE(gain_mode) \
	((gain_mode) == BT_AICS_MODE_MANUAL_ONLY || (gain_mode) == BT_AICS_MODE_AUTO_ONLY)

#define BT_AICS_INPUT_MODE_SETTABLE(gain_mode) \
	((gain_mode) == BT_AICS_MODE_AUTO || (gain_mode) == BT_AICS_MODE_MANUAL)

struct bt_aics_control {
	uint8_t opcode;
	uint8_t counter;
} __packed;

struct bt_aics_gain_control {
	struct bt_aics_control cp;
	int8_t gain_setting;
} __packed;

enum bt_aics_client_flag {
	BT_AICS_CLIENT_FLAG_BUSY,
	BT_AICS_CLIENT_FLAG_CP_RETRIED,
	BT_AICS_CLIENT_FLAG_DESC_WRITABLE,
	BT_AICS_CLIENT_FLAG_ACTIVE,

	BT_AICS_CLIENT_FLAG_NUM_FLAGS, /* keep as last */
};

struct bt_aics_client {
	uint8_t change_counter;
	uint8_t gain_mode;

	uint16_t start_handle;
	uint16_t end_handle;
	uint16_t state_handle;
	uint16_t gain_handle;
	uint16_t type_handle;
	uint16_t status_handle;
	uint16_t control_handle;
	uint16_t desc_handle;
	struct bt_gatt_subscribe_params state_sub_params;
	struct bt_gatt_subscribe_params status_sub_params;
	struct bt_gatt_subscribe_params desc_sub_params;

	struct bt_aics_gain_control cp_val;
	struct bt_gatt_write_params write_params;
	struct bt_gatt_read_params read_params;
	struct bt_gatt_discover_params discover_params;
	struct bt_aics_cb *cb;
	struct bt_conn *conn;

	ATOMIC_DEFINE(flags, BT_AICS_CLIENT_FLAG_NUM_FLAGS);
};

struct bt_aics_state {
	int8_t gain;
	uint8_t mute;
	uint8_t gain_mode;
	uint8_t change_counter;
} __packed;

struct bt_aics_gain_settings {
	uint8_t units;
	int8_t minimum;
	int8_t maximum;
} __packed;

enum bt_aics_notify {
	AICS_NOTIFY_STATE,
	AICS_NOTIFY_DESCRIPTION,
	AICS_NOTIFY_STATUS,
	AICS_NOTIFY_NUM,
};

struct bt_aics_server {
	struct bt_aics_state state;
	struct bt_aics_gain_settings gain_settings;
	bool initialized;
	uint8_t type;
	uint8_t status;
	struct bt_aics *inst;
	char description[BT_AICS_MAX_DESC_SIZE];
	struct bt_aics_cb *cb;

	struct bt_gatt_service *service_p;

	ATOMIC_DEFINE(notify, AICS_NOTIFY_NUM);
	struct k_work_delayable notify_work;
};

/* Struct used as a common type for the api */
struct bt_aics {
	bool client_instance;
	union {
		struct bt_aics_server srv;
		struct bt_aics_client cli;
	};

};

uint8_t aics_client_notify_handler(struct bt_conn *conn,
				   struct bt_gatt_subscribe_params *params,
				   const void *data, uint16_t length);
int bt_aics_client_register(struct bt_aics *inst);
int bt_aics_client_unregister(struct bt_aics *inst);
int bt_aics_client_state_get(struct bt_aics *inst);
int bt_aics_client_gain_setting_get(struct bt_aics *inst);
int bt_aics_client_type_get(struct bt_aics *inst);
int bt_aics_client_status_get(struct bt_aics *inst);
int bt_aics_client_unmute(struct bt_aics *inst);
int bt_aics_client_mute(struct bt_aics *inst);
int bt_aics_client_manual_gain_set(struct bt_aics *inst);
int bt_aics_client_automatic_gain_set(struct bt_aics *inst);
int bt_aics_client_gain_set(struct bt_aics *inst, int8_t gain);
int bt_aics_client_description_get(struct bt_aics *inst);
int bt_aics_client_description_set(struct bt_aics *inst,
				   const char *description);

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_AICS_INTERNAL_ */
