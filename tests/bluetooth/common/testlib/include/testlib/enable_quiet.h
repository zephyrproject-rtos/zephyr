/* Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_TESTS_BLUETOOTH_COMMON_TESTLIB_INCLUDE_TESTLIB_BT_ENABLE_QUIET_H_
#define ZEPHYR_TESTS_BLUETOOTH_COMMON_TESTLIB_INCLUDE_TESTLIB_BT_ENABLE_QUIET_H_

/**
 * Wraps @ref bt_enable() and sets log levels for bt_hci_core and bt_id
 * to LOG_LEVEL_ERR for the duration of the call. This avoids printing
 * some noise that @ref bt_enable() generates.
 */
int bt_testlib_enable_quiet(void);

#endif /* ZEPHYR_TESTS_BLUETOOTH_COMMON_TESTLIB_INCLUDE_TESTLIB_BT_ENABLE_QUIET_H_ */
