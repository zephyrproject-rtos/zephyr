/* btp_mcs.h - Bluetooth tester headers */

/*
 * Copyright (c) 2023 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/bluetooth/services/ots.h>

/* MCS commands */
#define BTP_MCS_READ_SUPPORTED_COMMANDS		0x01
struct btp_mcs_read_supported_commands_rp {
	uint8_t data[0];
} __packed;

#define BTP_MCS_CMD_SEND			0x02
struct btp_mcs_send_cmd {
	uint8_t opcode;
	uint8_t use_param;
	int32_t param;
} __packed;

#define BTP_MCS_CURRENT_TRACK_OBJ_ID_GET	0x03
struct btp_mcs_current_track_obj_id_rp {
	uint8_t id[BT_OTS_OBJ_ID_SIZE];
} __packed;

#define BTP_MCS_NEXT_TRACK_OBJ_ID_GET		0x04
struct btp_mcs_next_track_obj_id_rp {
	uint8_t id[BT_OTS_OBJ_ID_SIZE];
} __packed;

#define BTP_MCS_INACTIVE_STATE_SET		0x05
struct btp_mcs_state_set_rp {
	uint8_t state;
} __packed;

#define BTP_MCS_PARENT_GROUP_SET		0x06
