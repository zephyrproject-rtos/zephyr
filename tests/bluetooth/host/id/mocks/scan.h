/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/fff.h>
#include <zephyr/kernel.h>

/* List of fakes used by this unit tester */
#define SCAN_FFF_FAKES_LIST(FAKE) FAKE(bt_le_scan_set_enable)

DECLARE_FAKE_VALUE_FUNC(int, bt_le_scan_set_enable, uint8_t);
