/**
 * Common macros used by both sides of the test
 *
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>

#define CHRC_SIZE 60

#define TEST_SERVICE_UUID                                                                          \
	BT_UUID_DECLARE_128(0x01, 0x23, 0x45, 0x67, 0x89, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,      \
			    0x07, 0x08, 0x09, 0x00, 0x00)

#define TEST_CHRC_UUID                                                                             \
	BT_UUID_DECLARE_128(0x01, 0x23, 0x45, 0x67, 0x89, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,      \
			    0x07, 0x08, 0x09, 0xFF, 0x11)

#define NOTIFICATION_COUNT 30
