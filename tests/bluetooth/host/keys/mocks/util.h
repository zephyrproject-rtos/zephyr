/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/fff.h>

/* List of fakes used by this unit tester */
#define UTIL_FFF_FAKES_LIST(FAKE)       \
		FAKE(u8_to_dec)                 \

DECLARE_FAKE_VALUE_FUNC(uint8_t, u8_to_dec, char *, uint8_t, uint8_t);
