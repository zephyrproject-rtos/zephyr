/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/addr.h>
#include <zephyr/fff.h>
#include <zephyr/kernel.h>

/* List of fakes used by this unit tester */
#define RPA_FFF_FAKES_LIST(FAKE) FAKE(bt_rpa_create)

DECLARE_FAKE_VALUE_FUNC(int, bt_rpa_create, const uint8_t *, bt_addr_t *);
