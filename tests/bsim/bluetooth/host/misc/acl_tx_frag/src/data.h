/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_TESTS_BSIM_BLUETOOTH_HOST_MISC_ACL_TX_FRAG_SRC_DATA_H_
#define ZEPHYR_TESTS_BSIM_BLUETOOTH_HOST_MISC_ACL_TX_FRAG_SRC_DATA_H_

#include <zephyr/bluetooth/uuid.h>

#define TEST_ITERATIONS 3

/* overhead: opcode + ATT handle + L2CAP PDU header */
#define GATT_PAYLOAD_SIZE (CONFIG_BT_L2CAP_TX_MTU - 1 - 2 - 4)

#define test_service_uuid                                                                          \
	BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0xf0debc9a, 0x7856, 0x3412, 0x7856, 0x341278563412))
#define test_characteristic_uuid                                                                   \
	BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0xf2debc9a, 0x7856, 0x3412, 0x7856, 0x341278563412))

#endif /* ZEPHYR_TESTS_BSIM_BLUETOOTH_HOST_MISC_ACL_TX_FRAG_SRC_DATA_H_ */
