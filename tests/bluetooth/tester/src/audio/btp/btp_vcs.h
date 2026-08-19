/* btp_vcs.h - Bluetooth tester headers */

/*
 * Copyright (c) 2022 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#define BTP_VCS_REGISTER_FLAG_MUTED BIT(0U)

#define BTP_VCS_READ_SUPPORTED_COMMANDS		0x01U
struct btp_vcs_read_supported_commands_rp {
	uint8_t data[0];
} __packed;

#define BTP_VCS_SET_VOL				0x02U
struct btp_vcs_set_vol_cmd {
	uint8_t volume;
} __packed;

#define BTP_VCS_VOL_UP				0x03U
#define BTP_VCS_VOL_DOWN			0x04U
#define BTP_VCS_MUTE				0x05U
#define BTP_VCS_UNMUTE				0x06U

#define BTP_VCS_REGISTER			0x07U
struct btp_vcs_register_cmd {
	uint8_t step;
	uint8_t flags;
	uint8_t volume;
} __packed;
