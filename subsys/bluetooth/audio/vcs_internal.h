/**
 * @file
 * @brief Internal Header for Bluetooth Volumen Control Service (VCS).
 *
 * Copyright (c) 2020 Bose Corporation
 * Copyright (c) 2020-2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_VCS_INTERNAL_
#define ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_VCS_INTERNAL_

/* VCS opcodes */
#define BT_VCS_OPCODE_REL_VOL_DOWN                      0x00
#define BT_VCS_OPCODE_REL_VOL_UP                        0x01
#define BT_VCS_OPCODE_UNMUTE_REL_VOL_DOWN               0x02
#define BT_VCS_OPCODE_UNMUTE_REL_VOL_UP                 0x03
#define BT_VCS_OPCODE_SET_ABS_VOL                       0x04
#define BT_VCS_OPCODE_UNMUTE                            0x05
#define BT_VCS_OPCODE_MUTE                              0x06

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

int bt_vcs_client_get(struct bt_conn *conn, struct bt_vcs *client);
int bt_vcs_client_read_vol_state(struct bt_conn *conn);
int bt_vcs_client_read_flags(struct bt_conn *conn);
int bt_vcs_client_vol_down(struct bt_conn *conn);
int bt_vcs_client_vol_up(struct bt_conn *conn);
int bt_vcs_client_unmute_vol_down(struct bt_conn *conn);
int bt_vcs_client_unmute_vol_up(struct bt_conn *conn);
int bt_vcs_client_set_volume(struct bt_conn *conn, uint8_t volume);
int bt_vcs_client_unmute(struct bt_conn *conn);
int bt_vcs_client_mute(struct bt_conn *conn);

bool bt_vcs_client_valid_vocs_inst(struct bt_conn *conn, struct bt_vocs *vocs);
bool bt_vcs_client_valid_aics_inst(struct bt_conn *conn, struct bt_aics *aics);
#endif /* ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_VCS_INTERNAL_*/
