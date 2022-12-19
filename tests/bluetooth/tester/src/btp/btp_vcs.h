/* btp_vcs.h - Bluetooth tester headers */

/*
 * Copyright (c) 2022 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/util.h>

#define VCS_READ_SUPPORTED_COMMANDS    0x01

#define VCS_SET_VOL                    0x02
#define VCS_VOL_UP                     0x03
#define VCS_VOL_DOWN                   0x04
#define VCS_MUTE                       0x05
#define VCS_UNMUTE                     0x06

struct vcs_set_vol_cmd {
	uint8_t volume;
} __packed;
