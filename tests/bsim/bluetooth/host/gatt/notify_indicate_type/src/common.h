/**
 * Common definitions for notify/indicate type detection test.
 *
 * Copyright (c) 2025 Andrew Leech
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>
#include <zephyr/bluetooth/gatt.h>

#ifndef BT_GATT_SUBSCRIBE_HAS_RECEIVED_OPCODE
#error "This test requires the received_opcode feature (BT_GATT_SUBSCRIBE_HAS_RECEIVED_OPCODE)"
#endif

#define CHRC_SIZE 10

/* Use UUIDs distinct from the notify/ test to avoid confusion. */
#define TEST_SERVICE_UUID                                                                          \
	BT_UUID_DECLARE_128(0x01, 0x23, 0x45, 0x67, 0x89, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,      \
			    0x07, 0x08, 0x09, 0xA0, 0x00)

#define TEST_CHRC_UUID                                                                             \
	BT_UUID_DECLARE_128(0x01, 0x23, 0x45, 0x67, 0x89, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,      \
			    0x07, 0x08, 0x09, 0xA0, 0x01)
