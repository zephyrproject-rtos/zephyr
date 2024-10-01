/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_TESTS_BSIM_BLUETOOTH_HOST_MISC_SAMPLE_TEST_SRC_DATA_H_
#define ZEPHYR_TESTS_BSIM_BLUETOOTH_HOST_MISC_SAMPLE_TEST_SRC_DATA_H_

#include <zephyr/bluetooth/uuid.h>

static uint8_t payload_1[] = {0xab, 0xcd};
static uint8_t payload_2[] = {0x13, 0x37};

/* Both payloads are assumed to be the same size in order to simplify the test
 * procedure.
 */
BUILD_ASSERT(sizeof(payload_1) == sizeof(payload_2),
	     "Both payloads should be of equal length");

#define test_service_uuid                                                                          \
	BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0xf0debc9a, 0x7856, 0x3412, 0x7856, 0x341278563412))
#define test_characteristic_uuid                                                                   \
	BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0xf2debc9a, 0x7856, 0x3412, 0x7856, 0x341278563412))

#endif /* ZEPHYR_TESTS_BSIM_BLUETOOTH_HOST_MISC_SAMPLE_TEST_SRC_DATA_H_ */
