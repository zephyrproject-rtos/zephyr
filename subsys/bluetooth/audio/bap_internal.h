/** @file
 *  @brief Internal header for Bluetooth BAP.
 */

/*
 * Copyright (c) 2019 Bose Corporation
 * Copyright (c) 2021-2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/bap.h>

#define BT_BAP_BASS_SCAN_STATE_NOT_SCANNING   0x00
#define BT_BAP_BASS_SCAN_STATE_SCANNING       0x01

#define BT_BAP_BASS_OP_SCAN_STOP              0x00
#define BT_BAP_BASS_OP_SCAN_START             0x01
#define BT_BAP_BASS_OP_ADD_SRC                0x02
#define BT_BAP_BASS_OP_MOD_SRC                0x03
#define BT_BAP_BASS_OP_BROADCAST_CODE         0x04
#define BT_BAP_BASS_OP_REM_SRC                0x05

#define BT_BAP_BASS_SCAN_STATE_IDLE           0x00
#define BT_BAP_BASS_SCAN_STATE_SCANNING       0x01
#define BT_BAP_BASS_SCAN_STATE_FAILED         0x02
#define BT_BAP_BASS_SCAN_STATE_SYNCED         0x03

#define BT_BAP_BASS_PA_REQ_NO_SYNC            0x00
#define BT_BAP_BASS_PA_REQ_SYNC_PAST          0x01
#define BT_BAP_BASS_PA_REQ_SYNC               0x02

#define BT_BAP_BASS_VALID_OPCODE(opcode) ((opcode) <= BT_BAP_BASS_OP_REM_SRC)

struct bt_bap_bass_cp_scan_stop {
	uint8_t opcode;
} __packed;

struct bt_bap_bass_cp_scan_start {
	uint8_t opcode;
} __packed;

struct bt_bap_bass_cp_subgroup {
	uint32_t bis_sync;
	uint8_t metadata_len;
	uint8_t metadata[0];
} __packed;

struct bt_bap_bass_cp_add_src {
	uint8_t opcode;
	bt_addr_le_t addr;
	uint8_t adv_sid;
	uint8_t broadcast_id[BT_AUDIO_BROADCAST_ID_SIZE];
	uint8_t pa_sync;
	uint16_t pa_interval;
	uint8_t num_subgroups;
	struct bt_bap_bass_cp_subgroup subgroups[0];
} __packed;

struct bt_bap_bass_cp_mod_src {
	uint8_t opcode;
	uint8_t src_id;
	uint8_t pa_sync;
	uint16_t pa_interval;
	uint8_t num_subgroups;
	struct bt_bap_bass_cp_subgroup subgroups[0];
} __packed;

struct bt_bap_bass_cp_broadcase_code {
	uint8_t opcode;
	uint8_t src_id;
	uint8_t broadcast_code[16];
} __packed;

struct bt_bap_bass_cp_rem_src {
	uint8_t opcode;
	uint8_t src_id;
} __packed;

union bt_bap_bass_cp {
	uint8_t opcode;
	struct bt_bap_bass_cp_scan_stop scan_stop;
	struct bt_bap_bass_cp_scan_start scan_start;
	struct bt_bap_bass_cp_add_src add_src;
	struct bt_bap_bass_cp_mod_src mod_src;
	struct bt_bap_bass_cp_broadcase_code broadcast_code;
	struct bt_bap_bass_cp_rem_src rem_src;
};

static inline const char *bt_bap_pa_state_str(uint8_t state)
{
	switch (state) {
	case BT_BAP_PA_STATE_NOT_SYNCED:
		return "Not synchronized to PA";
	case BT_BAP_PA_STATE_INFO_REQ:
		return "SyncInfo Request";
	case BT_BAP_PA_STATE_SYNCED:
		return "Synchronized to PA";
	case BT_BAP_PA_STATE_FAILED:
		return "Failed to synchronize to PA";
	case BT_BAP_PA_STATE_NO_PAST:
		return "No PAST";
	default:
		return "unknown state";
	}
}

static inline const char *bt_bap_big_enc_state_str(uint8_t state)
{
	switch (state) {
	case BT_BAP_BIG_ENC_STATE_NO_ENC:
		return "Not encrypted";
	case BT_BAP_BIG_ENC_STATE_BCODE_REQ:
		return "Broadcast_Code required";
	case BT_BAP_BIG_ENC_STATE_DEC:
		return "Decrypting";
	case BT_BAP_BIG_ENC_STATE_BAD_CODE:
		return "Bad_Code (incorrect encryption key)";
	default:
		return "unknown state";
	}
}
