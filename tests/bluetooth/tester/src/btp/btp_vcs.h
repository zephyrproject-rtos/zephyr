/* vcs.h - Bluetooth tester headers */

/*
 * Copyright (c) 2022 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <zephyr/sys/util.h>

#define VCS_READ_SUPPORTED_COMMANDS		0x01

#define VCS_SET_VOL		0x5f
#define VCS_INIT		0x60
#define VCS_VOL_UP		0x61
#define VCS_VOL_DOWN	0x62
#define VCS_MUTE		0x63
#define VCS_UNMUTE		0x64

struct vcs_set_vol_cmd {
	uint32_t volume;
} __packed;

/* events */
#define IAS_EV_OUT_ALERT_ACTION		0x80
struct ias_alert_action_ev {
	uint32_t alert_lvl;
} __packed;
