/* Copyright (c) 2025 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_TESTS_BLUETOOTH_COMMON_TESTLIB_INCLUDE_TESTLIB_BT_ENABLE_QUIET_H_
#define ZEPHYR_TESTS_BLUETOOTH_COMMON_TESTLIB_INCLUDE_TESTLIB_BT_ENABLE_QUIET_H_

/**
 * @brief bt_enable() with minimal logging
 *
 * Wraps @ref bt_enable() and temporarily sets the log levels for
 * the "bt_hci_core" and "bt_id" modules to LOG_LEVEL_ERR for the
 * duration of the call. This reduces noise normally printed by
 * @ref bt_enable().
 *
 * @return The return value of @ref bt_enable().
 */
int bt_testlib_silent_bt_enable(void);

#endif /* ZEPHYR_TESTS_BLUETOOTH_COMMON_TESTLIB_INCLUDE_TESTLIB_BT_ENABLE_QUIET_H_ */
