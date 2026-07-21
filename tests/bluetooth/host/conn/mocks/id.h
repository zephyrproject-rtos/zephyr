/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/fff.h>

/* List of fakes used by this unit tester */
#define ID_MOCKS_FFF_FAKES_LIST(FAKE) FAKE(bt_id_scan_random_addr_check)

DECLARE_FAKE_VALUE_FUNC(bool, bt_id_scan_random_addr_check);
