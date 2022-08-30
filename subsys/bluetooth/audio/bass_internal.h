/** @file
 *  @brief Internal header for Bluetooth BASS.
 */

/*
 * Copyright (c) 2019 Bose Corporation
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/audio/bass.h>

#define BT_BASS_SCAN_STATE_NOT_SCANNING   0x00
#define BT_BASS_SCAN_STATE_SCANNING       0x01

#define BT_BASS_OP_SCAN_STOP              0x00
#define BT_BASS_OP_SCAN_START             0x01
#define BT_BASS_OP_ADD_SRC                0x02
#define BT_BASS_OP_MOD_SRC                0x03
#define BT_BASS_OP_BROADCAST_CODE         0x04
#define BT_BASS_OP_REM_SRC                0x05

#define BT_BASS_SCAN_STATE_IDLE           0x00
#define BT_BASS_SCAN_STATE_SCANNING       0x01
#define BT_BASS_SCAN_STATE_FAILED         0x02
#define BT_BASS_SCAN_STATE_SYNCED         0x03

#define BT_BASS_PA_REQ_NO_SYNC            0x00
#define BT_BASS_PA_REQ_SYNC_PAST          0x01
#define BT_BASS_PA_REQ_SYNC               0x02

#define BT_BASS_BROADCAST_ID_SIZE          3

#define BT_BASS_VALID_OPCODE(opcode) \
	((opcode) >= BT_BASS_OP_SCAN_STOP && (opcode) <= BT_BASS_OP_REM_SRC)

struct bt_bass_cp_scan_stop {
	uint8_t opcode;
} __packed;

struct bt_bass_cp_scan_start {
	uint8_t opcode;
} __packed;

struct bt_bass_cp_subgroup {
	uint32_t bis_sync;
	uint8_t metadata_len;
	uint8_t metadata[0];
} __packed;

struct bt_bass_cp_add_src {
	uint8_t opcode;
	bt_addr_le_t addr;
	uint8_t adv_sid;
	uint8_t broadcast_id[BT_BASS_BROADCAST_ID_SIZE];
	uint8_t pa_sync;
	uint16_t pa_interval;
	uint8_t num_subgroups;
	struct bt_bass_cp_subgroup subgroups[0];
} __packed;

struct bt_bass_cp_mod_src {
	uint8_t opcode;
	uint8_t src_id;
	uint8_t pa_sync;
	uint16_t pa_interval;
	uint8_t num_subgroups;
	struct bt_bass_cp_subgroup subgroups[0];
} __packed;

struct bt_bass_cp_broadcase_code {
	uint8_t opcode;
	uint8_t src_id;
	uint8_t broadcast_code[16];
} __packed;

struct bt_bass_cp_rem_src {
	uint8_t opcode;
	uint8_t src_id;
} __packed;

union bt_bass_cp {
	uint8_t opcode;
	struct bt_bass_cp_scan_stop scan_stop;
	struct bt_bass_cp_scan_start scan_start;
	struct bt_bass_cp_add_src add_src;
	struct bt_bass_cp_mod_src mod_src;
	struct bt_bass_cp_broadcase_code broadcast_code;
	struct bt_bass_cp_rem_src rem_src;
};
