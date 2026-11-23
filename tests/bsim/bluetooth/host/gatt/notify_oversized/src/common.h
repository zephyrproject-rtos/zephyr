/*
 * Copyright (c) 2026 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_TESTS_BSIM_BLUETOOTH_HOST_GATT_NOTIFY_OVERSIZED_SRC_COMMON_H_
#define ZEPHYR_TESTS_BSIM_BLUETOOTH_HOST_GATT_NOTIFY_OVERSIZED_SRC_COMMON_H_

#include <zephyr/bluetooth/att.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/toolchain.h>

#define TEST_SERVICE_UUID                                                                          \
	BT_UUID_DECLARE_128(0x01, 0x23, 0x45, 0x67, 0x89, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,      \
			    0x07, 0x08, 0x09, 0x00, 0x00)

#define TEST_CHRC_A_UUID                                                                           \
	BT_UUID_DECLARE_128(0x01, 0x23, 0x45, 0x67, 0x89, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,      \
			    0x07, 0x08, 0x09, 0xFF, 0x00)

#define TEST_CHRC_B_UUID                                                                           \
	BT_UUID_DECLARE_128(0x01, 0x23, 0x45, 0x67, 0x89, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,      \
			    0x07, 0x08, 0x09, 0xFF, 0x11)

/* Value lengths sent by the server */
#define OVERSIZED_LEN (BT_ATT_MAX_ATTRIBUTE_LEN + 1)
#define MAX_LEN       BT_ATT_MAX_ATTRIBUTE_LEN
#define SHORT_LEN     10
#define MARKER_LEN    1

/* Multiple Handle Value Notification carrying a short and an oversized value */
#define REQUIRED_ATT_MTU (1 + 4 + SHORT_LEN + 4 + OVERSIZED_LEN)

BUILD_ASSERT(CONFIG_BT_L2CAP_TX_MTU >= REQUIRED_ATT_MTU);

#endif /* ZEPHYR_TESTS_BSIM_BLUETOOTH_HOST_GATT_NOTIFY_OVERSIZED_SRC_COMMON_H_ */
