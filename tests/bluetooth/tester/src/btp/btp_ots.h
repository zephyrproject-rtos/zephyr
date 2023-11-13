/* btp_ots.h - Bluetooth tester headers */

/*
 * Copyright (c) 2023 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* OTS commands */
#define BTP_OTS_READ_SUPPORTED_COMMANDS		0x01
struct btp_ots_read_supported_commands_rp {
	uint8_t data[0];
} __packed;
