/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/fff.h>

#include <host/scan.h>

/* List of fakes used by this unit tester */
#define SCAN_MOCKS_FFF_FAKES_LIST(FAKE)                                                            \
	FAKE(bt_le_scan_user_add)                                                                  \
	FAKE(bt_le_scan_user_remove)                                                               \
	FAKE(bt_le_explicit_scanner_running)

DECLARE_FAKE_VALUE_FUNC(int, bt_le_scan_user_add, enum bt_le_scan_user);
DECLARE_FAKE_VALUE_FUNC(int, bt_le_scan_user_remove, enum bt_le_scan_user);
DECLARE_FAKE_VALUE_FUNC(bool, bt_le_explicit_scanner_running);
