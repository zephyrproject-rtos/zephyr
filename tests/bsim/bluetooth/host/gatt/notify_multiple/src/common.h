/**
 * Common functions and helpers for BSIM GATT tests
 *
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/uuid.h>

#define FORCE_FLAG(flag, val) (void)atomic_set(&flag, (atomic_t)val)

#define CHRC_SIZE 10
#define LONG_CHRC_SIZE 40

#define TEST_SERVICE_UUID                                                                          \
	BT_UUID_DECLARE_128(0x01, 0x23, 0x45, 0x67, 0x89, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,      \
			    0x07, 0x08, 0x09, 0x00, 0x00)

#define TEST_CHRC_UUID                                                                             \
	BT_UUID_DECLARE_128(0x01, 0x23, 0x45, 0x67, 0x89, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,      \
			    0x07, 0x08, 0x09, 0xFF, 0x00)

#define TEST_LONG_CHRC_UUID                                                                        \
	BT_UUID_DECLARE_128(0x01, 0x23, 0x45, 0x67, 0x89, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,      \
			    0x07, 0x08, 0x09, 0xFF, 0x11)

#define NOTIFICATION_COUNT 10
BUILD_ASSERT(NOTIFICATION_COUNT % 2 == 0);
