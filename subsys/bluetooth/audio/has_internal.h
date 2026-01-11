/** @file
 *  @brief Internal APIs for Bluetooth Hearing Access Profile.
 */

/*
 * Copyright (c) 2022 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util_macro.h>

/* Control Point opcodes */
#define BT_HAS_OP_READ_PRESET_REQ        0x01
#define BT_HAS_OP_READ_PRESET_RSP        0x02
#define BT_HAS_OP_PRESET_CHANGED         0x03
#define BT_HAS_OP_WRITE_PRESET_NAME      0x04
#define BT_HAS_OP_SET_ACTIVE_PRESET      0x05
#define BT_HAS_OP_SET_NEXT_PRESET        0x06
#define BT_HAS_OP_SET_PREV_PRESET        0x07
#define BT_HAS_OP_SET_ACTIVE_PRESET_SYNC 0x08
#define BT_HAS_OP_SET_NEXT_PRESET_SYNC   0x09
#define BT_HAS_OP_SET_PREV_PRESET_SYNC   0x0a

/* Application error codes */
#define BT_HAS_ERR_INVALID_OPCODE         0x80
#define BT_HAS_ERR_WRITE_NAME_NOT_ALLOWED 0x81
#define BT_HAS_ERR_PRESET_SYNC_NOT_SUPP   0x82
#define BT_HAS_ERR_OPERATION_NOT_POSSIBLE 0x83
#define BT_HAS_ERR_INVALID_PARAM_LEN      0x84

/* Hearing Aid Feature bits */
#define BT_HAS_FEAT_HEARING_AID_TYPE_LSB  BIT(0)
#define BT_HAS_FEAT_HEARING_AID_TYPE_MSB  BIT(1)
#define BT_HAS_FEAT_PRESET_SYNC_SUPP      BIT(2)
#define BT_HAS_FEAT_INDEPENDENT_PRESETS   BIT(3)
#define BT_HAS_FEAT_DYNAMIC_PRESETS       BIT(4)
#define BT_HAS_FEAT_WRITABLE_PRESETS_SUPP BIT(5)

#define BT_HAS_FEAT_HEARING_AID_TYPE_MASK (BT_HAS_FEAT_HEARING_AID_TYPE_LSB | \
					   BT_HAS_FEAT_HEARING_AID_TYPE_MSB)

/* Preset Changed Change ID values */
#define BT_HAS_CHANGE_ID_GENERIC_UPDATE     0x00
#define BT_HAS_CHANGE_ID_PRESET_DELETED     0x01
#define BT_HAS_CHANGE_ID_PRESET_AVAILABLE   0x02
#define BT_HAS_CHANGE_ID_PRESET_UNAVAILABLE 0x03

#define BT_HAS_IS_LAST 0x01

struct bt_has {
	/** Hearing Aid Features value */
	uint8_t features;

	/** Active preset index value */
	uint8_t active_index;

	/* Whether the service has been registered or not */
	bool registered;
};

struct bt_has_cp_hdr {
	uint8_t opcode;
	uint8_t data[];
} __packed;

struct bt_has_cp_read_presets_req {
	uint8_t start_index;
	uint8_t num_presets;
} __packed;

struct bt_has_cp_read_preset_rsp {
	uint8_t is_last;
	uint8_t index;
	uint8_t properties;
	uint8_t name[];
} __packed;

struct bt_has_cp_preset_changed {
	uint8_t change_id;
	uint8_t is_last;
	uint8_t additional_params[];
} __packed;

struct bt_has_cp_generic_update {
	uint8_t prev_index;
	uint8_t index;
	uint8_t properties;
	uint8_t name[];
} __packed;

struct bt_has_cp_write_preset_name {
	uint8_t index;
	uint8_t name[];
} __packed;

struct bt_has_cp_set_active_preset {
	uint8_t index;
} __packed;

static inline const char *bt_has_op_str(uint8_t op)
{
	switch (op) {
	case BT_HAS_OP_READ_PRESET_REQ:
		return "Read preset request";
	case BT_HAS_OP_READ_PRESET_RSP:
		return "Read preset response";
	case BT_HAS_OP_PRESET_CHANGED:
		return "Preset changed";
	case BT_HAS_OP_WRITE_PRESET_NAME:
		return "Write preset name";
	case BT_HAS_OP_SET_ACTIVE_PRESET:
		return "Set active preset";
	case BT_HAS_OP_SET_NEXT_PRESET:
		return "Set next preset";
	case BT_HAS_OP_SET_PREV_PRESET:
		return "Set previous preset";
	case BT_HAS_OP_SET_ACTIVE_PRESET_SYNC:
		return "Set active preset (sync)";
	case BT_HAS_OP_SET_NEXT_PRESET_SYNC:
		return "Set next preset (sync)";
	case BT_HAS_OP_SET_PREV_PRESET_SYNC:
		return "Set previous preset (sync)";
	default:
		return "Unknown";
	}
}

static inline const char *bt_has_change_id_str(uint8_t change_id)
{
	switch (change_id) {
	case BT_HAS_CHANGE_ID_GENERIC_UPDATE:
		return "Generic update";
	case BT_HAS_CHANGE_ID_PRESET_DELETED:
		return "Preset deleted";
	case BT_HAS_CHANGE_ID_PRESET_AVAILABLE:
		return "Preset available";
	case BT_HAS_CHANGE_ID_PRESET_UNAVAILABLE:
		return "Preset unavailable";
	default:
		return "Unknown changeId";
	}
}

enum has_client_flags {
	HAS_CLIENT_DISCOVER_IN_PROGRESS,
	HAS_CLIENT_CP_OPERATION_IN_PROGRESS,

	HAS_CLIENT_NUM_FLAGS, /* keep as last */
};

struct bt_has_client {
	/** Common profile reference object */
	struct bt_has has;

	/** Profile connection reference */
	struct bt_conn *conn;

	/** Internal flags */
	ATOMIC_DEFINE(flags, HAS_CLIENT_NUM_FLAGS);

	/* GATT procedure parameters */
	union {
		struct {
			struct bt_uuid_16 uuid;
			union {
				struct bt_gatt_read_params read;
				struct bt_gatt_discover_params discover;
			};
		};
		struct bt_gatt_write_params write;
	} params;

	struct bt_gatt_subscribe_params features_subscription;
	struct bt_gatt_subscribe_params control_point_subscription;
	struct bt_gatt_subscribe_params active_index_subscription;
};
