/** @file
 *  @brief Header for Bluetooth BASS.
 *
 * Copyright (c) 2020 Bose Corporation
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_BASS_
#define ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_BASS_
#include <zephyr/types.h>
#include <bluetooth/conn.h>

#if IS_ENABLED(CONFIG_BT_BASS)
#define BT_BASS_MAX_METADATA_LEN CONFIG_BT_BASS_MAX_METADATA_LEN
#define BT_BASS_MAX_SUBGROUPS    CONFIG_BT_BASS_MAX_SUBGROUPS
#else
#define BT_BASS_MAX_METADATA_LEN 0
#define BT_BASS_MAX_SUBGROUPS    0
#endif

#define BT_BASS_BROADCAST_CODE_SIZE        16

#define BT_BASS_PA_STATE_NOT_SYNCED        0x00
#define BT_BASS_PA_STATE_INFO_REQ          0x01
#define BT_BASS_PA_STATE_SYNCED            0x02
#define BT_BASS_PA_STATE_FAILED            0x03
#define BT_BASS_PA_STATE_NO_PAST           0x04

#define BT_BASS_BIG_ENC_STATE_NO_ENC       0x00
#define BT_BASS_BIG_ENC_STATE_BCODE_REQ    0x01
#define BT_BASS_BIG_ENC_STATE_DEC          0x02
#define BT_BASS_BIG_ENC_STATE_BAD_CODE     0x03

#define BT_BASS_ERR_OPCODE_NOT_SUPPORTED   0x80
#define BT_BASS_ERR_INVALID_SRC_ID         0x81

#define BT_BASS_PA_INTERVAL_UNKNOWN        0xFFFF

#define BT_BASS_BROADCAST_MAX_ID           0xFFFFFF

#define BT_BASS_BIS_SYNC_NO_PREF           0xFFFFFFFF

struct bt_bass_subgroup {
	uint32_t bis_sync;
	uint32_t requested_bis_sync;
	uint8_t metadata_len;
	uint8_t metadata[BT_BASS_MAX_METADATA_LEN];
};

/* TODO: Only expose this as an opaque type */
struct bt_bass_recv_state {
	uint8_t src_id;
	bt_addr_le_t addr;
	uint8_t adv_sid;
	uint8_t req_pa_sync_value;
	uint8_t pa_sync_state;
	uint8_t encrypt_state;
	uint32_t broadcast_id; /* 24 bits */
	uint8_t bad_code[BT_BASS_BROADCAST_CODE_SIZE];
	uint8_t num_subgroups;
	struct bt_bass_subgroup subgroups[BT_BASS_MAX_SUBGROUPS];
};

struct bt_bass_cb {
	void (*pa_synced)(struct bt_bass_recv_state *recv_state,
			  const struct bt_le_per_adv_sync_synced_info *info);
	void (*pa_term)(struct bt_bass_recv_state *recv_state,
			const struct bt_le_per_adv_sync_term_info *info);
	void (*pa_recv)(struct bt_bass_recv_state *recv_state,
			const struct bt_le_per_adv_sync_recv_info *info,
			struct net_buf_simple *buf);
	void (*biginfo)(struct bt_bass_recv_state *recv_state,
			const struct bt_iso_biginfo *biginfo);
};

/**
 * @brief Registers the callbacks used by BASS.
 */
void bt_bass_register_cb(struct bt_bass_cb *cb);

/**
 * @brief Set the sync state of a receive state in the server
 *
 * @param src_id         The source id used to identify the receive state.
 * @param pa_sync_state  The sync state of the PA.
 * @param bis_synced     Array of bitfields to set the BIS sync state for each
 *                       subgroup.
 * @param encrypted      The BIG encryption state.
 * @return int           Error value. 0 on success, ERRNO on fail.
 */
int bt_bass_set_sync_state(uint8_t src_id, uint8_t pa_sync_state,
			   uint32_t bis_synced[BT_BASS_MAX_SUBGROUPS],
			   uint8_t encrypted);

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_BASS_ */
