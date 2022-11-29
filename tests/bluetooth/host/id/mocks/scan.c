/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include <mocks/scan.h>

DEFINE_FAKE_VALUE_FUNC(int, bt_le_scan_set_enable, uint8_t);
