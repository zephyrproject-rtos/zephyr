/* btp_vcs.h - Bluetooth tester headers */

/*
 * Copyright (c) 2022 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/util.h>

#define BTP_VCS_READ_SUPPORTED_COMMANDS    0x01

#define BTP_VCS_SET_VOL                    0x02
#define BTP_VCS_VOL_UP                     0x03
#define BTP_VCS_VOL_DOWN                   0x04
#define BTP_VCS_MUTE                       0x05
#define BTP_VCS_UNMUTE                     0x06

struct btp_vcs_set_vol_cmd {
	uint8_t volume;
} __packed;
