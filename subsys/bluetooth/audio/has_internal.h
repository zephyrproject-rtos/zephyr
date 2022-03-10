/** @file
 *  @brief Internal APIs for Bluetooth Hearing Access Profile.
 */

/*
 * Copyright (c) 2022 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <bluetooth/audio/has.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/uuid.h>

#define BT_HAS_PRESET_INDEX_NONE            0x00

/* Control Point opcodes */
#define BT_HAS_OP_READ_PRESET_REQ           0x01
#define BT_HAS_OP_READ_PRESET_RSP           0x02
#define BT_HAS_OP_PRESET_CHANGED            0x03
#define BT_HAS_OP_WRITE_PRESET_NAME         0x04
#define BT_HAS_OP_SET_ACTIVE_PRESET         0x05
#define BT_HAS_OP_SET_NEXT_PRESET           0x06
#define BT_HAS_OP_SET_PREV_PRESET           0x07
#define BT_HAS_OP_SET_ACTIVE_PRESET_SYNC    0x08
#define BT_HAS_OP_SET_NEXT_PRESET_SYNC      0x09
#define BT_HAS_OP_SET_PREV_PRESET_SYNC      0x0a

/* Application errors */
#define BT_HAS_ERR_INVALID_OP               0x80
#define BT_HAS_ERR_WRITE_NAME_NOT_ALLOWED   0x81
#define BT_HAS_ERR_PRESET_SYNC_NOT_SUPP     0x82
#define BT_HAS_ERR_OPERATION_NOT_POSSIBLE   0x83
#define BT_HAS_ERR_INVALID_PARAM_LEN        0x84

/* Hearing Aid Features bits */
#define BT_HAS_FEAT_HEARING_AID_TYPE_MASK   (BIT(0) | BIT(1))
#define BT_HAS_FEAT_BIT_PRESET_SYNC         BIT(2)
#define BT_HAS_FEAT_BIT_INDEPENDENT_PRESETS BIT(3)
#define BT_HAS_FEAT_BIT_DYNAMIC_PRESETS     BIT(4)
#define BT_HAS_FEAT_BIT_WRITABLE_PRESETS    BIT(5)

/* Preset Changed Change ID values */
#define BT_HAS_CHANGE_ID_GENERIC_UPDATE     0x00
#define BT_HAS_CHANGE_ID_PRESET_DELETED     0x01
#define BT_HAS_CHANGE_ID_PRESET_AVAILABLE   0x02
#define BT_HAS_CHANGE_ID_PRESET_UNAVAILABLE 0x03

#define BT_HAS_ATT_MTU_MIN 49

struct bt_has_cp_hdr {
	uint8_t op;
	uint8_t data[0];
} __packed;

struct bt_has_cp_read_preset_req {
	uint8_t start_index;
	uint8_t num_presets;
} __packed;

struct bt_has_cp_write_preset_name_req {
	uint8_t index;
	uint8_t name[0];
} __packed;

struct bt_has_cp_set_active_preset_req {
	uint8_t index;
} __packed;

struct bt_has_cp_read_preset_rsp {
	uint8_t is_last;
	uint8_t index;
	uint8_t properties;
	uint8_t name[0];
} __packed;

struct bt_has_cp_preset_changed {
	uint8_t change_id;
	uint8_t is_last;
} __packed;

struct bt_has_cp_generic_update {
	uint8_t prev_index;
	uint8_t index;
	uint8_t properties;
	uint8_t name[0];
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
		return "Set active preset (synchronized)";
	case BT_HAS_OP_SET_NEXT_PRESET_SYNC:
		return "Set next preset (synchronized)";
	case BT_HAS_OP_SET_PREV_PRESET_SYNC:
		return "Set previous preset (synchronized)";
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

struct bt_has {
	/** Hearing Aid Features value */
	uint8_t features;

	/** Active preset index value */
	uint8_t active_index;
};

int bt_has_client_preset_active_get(struct bt_has *has);
int bt_has_client_preset_active_set(struct bt_has *has, uint8_t idx);
int bt_has_client_preset_active_set_next(struct bt_has *has);
int bt_has_client_preset_active_set_prev(struct bt_has *has);
int bt_has_client_preset_read(struct bt_has *has, uint8_t start_idx);
int bt_has_client_preset_read_multiple(struct bt_has *has, uint8_t start_idx, uint8_t count);
int bt_has_client_preset_name_set(struct bt_has *has, uint8_t idx, const char *name);
