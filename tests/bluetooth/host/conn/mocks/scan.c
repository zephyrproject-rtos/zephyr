/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/conn.h>

#include "scan.h"

DEFINE_FAKE_VALUE_FUNC(int, bt_le_scan_user_add, enum bt_le_scan_user);
DEFINE_FAKE_VALUE_FUNC(int, bt_le_scan_user_remove, enum bt_le_scan_user);
DEFINE_FAKE_VALUE_FUNC(bool, bt_le_explicit_scanner_running);
