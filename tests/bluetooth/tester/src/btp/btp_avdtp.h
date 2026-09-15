/* btp_avdtp.h - Bluetooth tester headers */

/*
 * Copyright (c) 2025 Intel Corporation
 * Copyright (c) 2025 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/l2cap.h>

#define BTP_AVDTP_READ_SUPPORTED_COMMANDS 0x01
struct btp_avdtp_read_supported_commands_rp {
	uint8_t data[0];
} __packed;
