/* btp_mics.h - Bluetooth tester headers */

/*
 * Copyright (c) 2023 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* MICS commands */
#define BTP_MICS_READ_SUPPORTED_COMMANDS	0x01
struct btp_mics_read_supported_commands_rp {
	uint8_t data[0];
} __packed;

#define BTP_MICS_DEV_MUTE_DISABLE		0x02
#define BTP_MICS_DEV_MUTE_READ			0x03
#define BTP_MICS_DEV_MUTE			0x04
#define BTP_MICS_DEV_UNMUTE			0x05

#define BTP_MICS_MUTE_STATE_EV			0x80
struct btp_mics_mute_state_ev {
	uint8_t mute;
} __packed;
