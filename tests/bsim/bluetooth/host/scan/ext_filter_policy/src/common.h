/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_TESTS_BSIM_BLUETOOTH_HOST_SCAN_EXT_FILTER_POLICY_COMMON_H_
#define ZEPHYR_TESTS_BSIM_BLUETOOTH_HOST_SCAN_EXT_FILTER_POLICY_COMMON_H_

#include <zephyr/bluetooth/addr.h>

/* Target address of the directed advertisements sent by the peer.
 *
 * The two most significant bits of the address are 0b01, so a scanner sees it
 * as a resolvable private address. It is not derived from any IRK, so no
 * Controller can resolve it, which is the case the extended scanner filter
 * policies exist to report.
 */
#define TEST_TARGET_ADDR                                                                           \
	((bt_addr_le_t){.type = BT_ADDR_LE_RANDOM,                                                 \
			.a = {.val = {0x9a, 0x3d, 0xe0, 0x55, 0xb1, 0x7c}}})

#endif /* ZEPHYR_TESTS_BSIM_BLUETOOTH_HOST_SCAN_EXT_FILTER_POLICY_COMMON_H_ */
