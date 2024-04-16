/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/fff.h>
#include <zephyr/kernel.h>

/* List of fakes used by this unit tester */
#define CRYPTO_FFF_FAKES_LIST(FAKE) FAKE(bt_rand)

DECLARE_FAKE_VALUE_FUNC(int, bt_rand, void *, size_t);
