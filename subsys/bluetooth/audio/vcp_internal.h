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

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_VCP_INTERNAL_*/
