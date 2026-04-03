/**
 * @file
 * @brief Internal Header for Bluetooth Volume Control Service (VCS).
 *
 * Copyright (c) 2020 Bose Corporation
 * Copyright (c) 2020-2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_VCP_INTERNAL_
#define ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_VCP_INTERNAL_

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/sys/atomic.h>

/* VCS opcodes */
#define BT_VCP_OPCODE_REL_VOL_DOWN                      0x00
#define BT_VCP_OPCODE_REL_VOL_UP                        0x01
#define BT_VCP_OPCODE_UNMUTE_REL_VOL_DOWN               0x02
#define BT_VCP_OPCODE_UNMUTE_REL_VOL_UP                 0x03
#define BT_VCP_OPCODE_SET_ABS_VOL                       0x04
#define BT_VCP_OPCODE_UNMUTE                            0x05
#define BT_VCP_OPCODE_MUTE                              0x06

struct vcs_state {
	uint8_t volume;
	uint8_t mute;
	uint8_t change_counter;
} __packed;

struct vcs_control {
	uint8_t opcode;
	uint8_t counter;
} __packed;

struct vcs_control_vol {
	struct vcs_control cp;
	uint8_t volume;
} __packed;

enum bt_vcp_vol_ctlr_flag {
	BT_VCP_VOL_CTLR_FLAG_BUSY,
	BT_VCP_VOL_CTLR_FLAG_CP_RETRIED,

	BT_VCP_VOL_CTLR_FLAG_NUM_FLAGS, /* keep as last */
};

struct bt_vcp_vol_ctlr {
	struct vcs_state state;
	uint8_t vol_flags;

	uint16_t start_handle;
	uint16_t end_handle;
	uint16_t state_handle;
	uint16_t control_handle;
	uint16_t vol_flag_handle;
	struct bt_gatt_subscribe_params state_sub_params;
	struct bt_gatt_discover_params state_sub_disc_params;
	struct bt_gatt_subscribe_params vol_flag_sub_params;
	struct bt_gatt_discover_params vol_flag_sub_disc_params;

	struct vcs_control_vol cp_val;
	struct bt_gatt_write_params write_params;
	struct bt_gatt_read_params read_params;
	struct bt_gatt_discover_params discover_params;
	struct bt_uuid_16 uuid;
	struct bt_conn *conn;

#if defined(CONFIG_BT_VCP_VOL_CTLR_VOCS)
	uint8_t vocs_inst_cnt;
	struct bt_vocs *vocs[CONFIG_BT_VCP_VOL_CTLR_MAX_VOCS_INST];
#endif /* CONFIG_BT_VCP_VOL_CTLR_VOCS */
#if defined(CONFIG_BT_VCP_VOL_CTLR_AICS)
	uint8_t aics_inst_cnt;
	struct bt_aics *aics[CONFIG_BT_VCP_VOL_CTLR_MAX_AICS_INST];
#endif /* CONFIG_BT_VCP_VOL_CTLR_AICS */

	ATOMIC_DEFINE(flags, BT_VCP_VOL_CTLR_FLAG_NUM_FLAGS);
};

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_VCP_INTERNAL_*/
