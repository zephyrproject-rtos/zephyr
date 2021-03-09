/** @file
 *  @brief Internal header for Bluetooth BASS.
 */

/*
 * Copyright (c) 2019 Bose Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_BASS_INTERNAL_
#define ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_BASS_INTERNAL_
#include <zephyr/types.h>
#include <bluetooth/conn.h>
#include "bass.h"

#define BASS_SCAN_STATE_NOT_SCANNING   0x00
#define BASS_SCAN_STATE_SCANNING       0x01

#define BASS_OP_SCAN_STOP              0x00
#define BASS_OP_SCAN_START             0x01
#define BASS_OP_ADD_SRC                0x02
#define BASS_OP_MOD_SRC                0x03
#define BASS_OP_BROADCAST_CODE         0x04
#define BASS_OP_REM_SRC                0x05

#define BASS_SCAN_STATE_IDLE           0x00
#define BASS_SCAN_STATE_SCANNING       0x01
#define BASS_SCAN_STATE_FAILED         0x02
#define BASS_SCAN_STATE_SYNCED         0x03

#define BASS_BIG_ENC_STATE_NO_ENC      0x00
#define BASS_BIG_ENC_STATE_ENC         0x01
#define BASS_BIG_ENC_STATE_DEC         0x02

#define BASS_PA_REQ_NO_SYNC            0x00
#define BASS_PA_REQ_SYNC               0x01
#define BASS_PA_REQ_SYNC_PAST          0x02

#define BASS_VALID_OPCODE(opcode) \
	((opcode) >= BASS_OP_SCAN_STOP && (opcode) <= BASS_OP_REM_SRC)

#define BASS_ACTUAL_SIZE(x) \
	((uint32_t)(sizeof(x) - (BASS_MAX_METADATA_LEN - (x).metadata_len)))

struct bass_cp_scan_stop_t {
	uint8_t opcode;
} __packed;

struct bass_cp_scan_start_t {
	uint8_t opcode;
} __packed;

struct bass_cp_add_src_t {
	uint8_t opcode;
	bt_addr_le_t addr;
	uint8_t adv_sid;
	uint8_t pa_sync;
	uint32_t bis_sync;
	uint8_t metadata_len;
	uint8_t metadata[BASS_MAX_METADATA_LEN];
} __packed;

struct bass_cp_mod_src_t {
	uint8_t opcode;
	uint8_t src_id;
	uint8_t pa_sync;
	uint32_t bis_sync;
	uint8_t metadata_len;
	uint8_t metadata[BASS_MAX_METADATA_LEN];
} __packed;

struct bass_cp_broadcase_code_t {
	uint8_t opcode;
	uint8_t src_id;
	uint8_t broadcast_code[16];
} __packed;

struct bass_cp_rem_src_t {
	uint8_t opcode;
	uint8_t src_id;
} __packed;

union bass_cp_t {
	uint8_t opcode;
	struct bass_cp_scan_stop_t scan_stop;
	struct bass_cp_scan_start_t scan_start;
	struct bass_cp_add_src_t add_src;
	struct bass_cp_mod_src_t mod_src;
	struct bass_cp_broadcase_code_t broadcast_code;
	struct bass_cp_rem_src_t rem_src;
};

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_BASS_INTERNAL_ */
