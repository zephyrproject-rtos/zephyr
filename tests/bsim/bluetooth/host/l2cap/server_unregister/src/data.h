/* Copyright (c) 2026 Xiaomi Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_TESTS_BSIM_BLUETOOTH_HOST_L2CAP_SERVER_UNREGISTER_SRC_DATA_H_
#define ZEPHYR_TESTS_BSIM_BLUETOOTH_HOST_L2CAP_SERVER_UNREGISTER_SRC_DATA_H_

/* Both sides use the same PSM number, so that an outgoing channel on the DUT
 * carries the PSM of the DUT's own server without having been accepted by it.
 */
#define TEST_DATA_L2CAP_PSM 0x0080
#define TEST_DATA_DUT_ADDR  BT_TESTLIB_ADDR_LE_RANDOM_C0_00_00_00_00_(0x01)

#endif /* ZEPHYR_TESTS_BSIM_BLUETOOTH_HOST_L2CAP_SERVER_UNREGISTER_SRC_DATA_H_ */
