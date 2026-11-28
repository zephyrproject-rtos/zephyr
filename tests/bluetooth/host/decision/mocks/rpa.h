/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/fff.h>

/* List of fakes used by this unit tester */
#define RPA_FFF_FAKES_LIST(FAKE) FAKE(bt_rpa_ah)

DECLARE_FAKE_VALUE_FUNC(int, bt_rpa_ah, const uint8_t *, const uint8_t *, uint8_t *);
