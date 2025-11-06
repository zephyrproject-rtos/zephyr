/* btp_avctp.h - Bluetooth AVCTP Tester */

/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/bluetooth/addr.h>

#define BTP_AVCTP_READ_SUPPORTED_COMMANDS 0x01
struct btp_avctp_read_supported_commands_rp {
	uint8_t data[0];
} __packed;
